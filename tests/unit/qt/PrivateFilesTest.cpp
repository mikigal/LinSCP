// L3 tests of the private files of the application layer (app/AppTools.cpp,
// app/AppConfiguration.cpp): the temporary directories are the user's only,
// also in a shared directory such as /tmp. The configuration file is tested
// end to end (integration/test_gui.py).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <AppInterface.h>

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
mode_t Mode(const QString & Path)
{
  struct stat Info;
  return (lstat(QFile::encodeName(Path).constData(), &Info) == 0) ? (Info.st_mode & 07777) : 0;
}
//---------------------------------------------------------------------------
QString TemporaryDir()
{
  return ToQString(ExcludeTrailingBackslash(AppConfiguration->TemporaryDir()));
}
//---------------------------------------------------------------------------
// $TMPDIR and DDTemporaryDirectory of their own
class TemporaryFoldersTest : public testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_TRUE(FTemp.isValid());
    const char * TmpDir = getenv("TMPDIR");
    FHadTmpDir = (TmpDir != nullptr);
    FPreviousTmpDir = FHadTmpDir ? TmpDir : "";
    setenv("TMPDIR", QFile::encodeName(FTemp.path()).constData(), 1);
    FPreviousDirectory = AppConfiguration->DDTemporaryDirectory;
    AppConfiguration->DDTemporaryDirectory = L"";
  }

  void TearDown() override
  {
    AppConfiguration->DDTemporaryDirectory = FPreviousDirectory;
    if (FHadTmpDir)
    {
      setenv("TMPDIR", FPreviousTmpDir.c_str(), 1);
    }
    else
    {
      unsetenv("TMPDIR");
    }
  }

  // The user's own directory in $TMPDIR
  QString Base() const
  {
    return FTemp.path() + QStringLiteral("/linscp-") + QString::number(geteuid());
  }

  QTemporaryDir FTemp;
  bool FHadTmpDir = false;
  std::string FPreviousTmpDir;
  UnicodeString FPreviousDirectory;
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(PrivateFilesTest, MakePrivate)
{
  QTemporaryDir Temp;
  ASSERT_TRUE(Temp.isValid());
  QString File = Temp.path() + QStringLiteral("/file");
  QFile Created(File);
  ASSERT_TRUE(Created.open(QIODevice::WriteOnly));
  Created.close();
  std::string Native = QFile::encodeName(File).toStdString();
  ASSERT_EQ(chmod(Native.c_str(), 0644), 0);
  EXPECT_TRUE(MakePrivate(ToUnicodeString(File)));
  EXPECT_EQ(Mode(File), 0600u);
  // Read-only stays read-only
  ASSERT_EQ(chmod(Native.c_str(), 0444), 0);
  EXPECT_TRUE(MakePrivate(ToUnicodeString(File)));
  EXPECT_EQ(Mode(File), 0400u);
  ASSERT_EQ(chmod(QFile::encodeName(Temp.path()).constData(), 0755), 0);
  EXPECT_TRUE(MakePrivate(ToUnicodeString(Temp.path() + QStringLiteral("/"))));
  EXPECT_EQ(Mode(Temp.path()), 0700u);
  EXPECT_FALSE(MakePrivate(ToUnicodeString(Temp.path() + QStringLiteral("/missing"))));

  // A new private directory, not an existing one
  QString Dir = Temp.path() + QStringLiteral("/dir");
  EXPECT_TRUE(CreatePrivateDirectory(ToUnicodeString(Dir)));
  EXPECT_EQ(Mode(Dir), 0700u);
  EXPECT_FALSE(CreatePrivateDirectory(ToUnicodeString(Dir)));
  EXPECT_TRUE(IsOwnDirectory(ToUnicodeString(Dir)));
  QString Link = Temp.path() + QStringLiteral("/link");
  ASSERT_EQ(symlink(QFile::encodeName(Dir).constData(), QFile::encodeName(Link).constData()), 0);
  EXPECT_FALSE(IsOwnDirectory(ToUnicodeString(Link)));
  EXPECT_FALSE(ForcePrivateDirectory(ToUnicodeString(Link)));
  // With its parents
  EXPECT_TRUE(ForcePrivateDirectory(ToUnicodeString(Temp.path() + QStringLiteral("/a/b"))));
  EXPECT_EQ(Mode(Temp.path() + QStringLiteral("/a/b")), 0700u);
}
//---------------------------------------------------------------------------
TEST_F(TemporaryFoldersTest, DefaultIsPrivate)
{
  QString Dir = TemporaryDir();
  EXPECT_TRUE(Dir.startsWith(Base() + QStringLiteral("/scp"))) << Dir.toStdString();
  EXPECT_EQ(ToQString(AppConfiguration->ExpandedTemporaryDirectory()), Base());
  EXPECT_EQ(Mode(Base()), 0700u);
  EXPECT_EQ(Mode(Dir), 0700u);
  // A new one each time
  QString Other = TemporaryDir();
  EXPECT_NE(Other, Dir);
  EXPECT_EQ(Mode(Other), 0700u);

  // A directory of the user's made private again
  ASSERT_EQ(chmod(QFile::encodeName(Base()).constData(), 0755), 0);
  TemporaryDir();
  EXPECT_EQ(Mode(Base()), 0700u);
}
//---------------------------------------------------------------------------
TEST_F(TemporaryFoldersTest, LinkedBaseIsRefused)
{
  // Another user could create the predictable name first, as a link to
  // their directory
  QTemporaryDir Elsewhere;
  ASSERT_TRUE(Elsewhere.isValid());
  ASSERT_EQ(symlink(QFile::encodeName(Elsewhere.path()).constData(), QFile::encodeName(Base()).constData()), 0);
  EXPECT_THROW(AppConfiguration->TemporaryDir(), ExtException);
  EXPECT_THROW(AppConfiguration->EnsureTemporaryDirectory(), ExtException);
  EXPECT_TRUE(QDir(Elsewhere.path()).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
}
//---------------------------------------------------------------------------
TEST_F(TemporaryFoldersTest, SharedDirectory)
{
  // A DDTemporaryDirectory shared with other users, as /tmp
  QString Shared = FTemp.path() + QStringLiteral("/shared");
  ASSERT_TRUE(QDir().mkdir(Shared));
  ASSERT_EQ(chmod(QFile::encodeName(Shared).constData(), 01777), 0);
  AppConfiguration->DDTemporaryDirectory = ToUnicodeString(Shared);
  QString Dir = TemporaryDir();
  EXPECT_TRUE(Dir.startsWith(Shared + QStringLiteral("/scp"))) << Dir.toStdString();
  EXPECT_EQ(Mode(Dir), 0700u);
  EXPECT_EQ(Mode(Shared), 01777u);

  // The cleanup takes the user's directories only, not links (their
  // targets would be emptied)
  QTemporaryDir Elsewhere;
  ASSERT_TRUE(Elsewhere.isValid());
  QFile Kept(Elsewhere.path() + QStringLiteral("/kept.txt"));
  ASSERT_TRUE(Kept.open(QIODevice::WriteOnly));
  Kept.close();
  ASSERT_TRUE(QDir(Shared).mkdir(QStringLiteral("scp12345")));
  QString Link = Shared + QStringLiteral("/scp54321");
  ASSERT_EQ(symlink(QFile::encodeName(Elsewhere.path()).constData(), QFile::encodeName(Link).constData()), 0);
  std::unique_ptr<TStrings> Folders(AppConfiguration->FindTemporaryFolders());
  ASSERT_NE(Folders.get(), nullptr);
  QStringList Found;
  for (int Index = 0; Index < Folders->Count; Index++)
  {
    Found.append(ToQString(Folders->Strings[Index]));
  }
  EXPECT_TRUE(Found.contains(Shared + QStringLiteral("/scp12345"))) << Found.join(QLatin1Char(',')).toStdString();
  EXPECT_TRUE(Found.contains(Dir)) << Found.join(QLatin1Char(',')).toStdString();
  EXPECT_FALSE(Found.contains(Link)) << Found.join(QLatin1Char(',')).toStdString();
  AppConfiguration->CleanupTemporaryFolders(Folders.get());
  EXPECT_FALSE(QFile::exists(Shared + QStringLiteral("/scp12345")));
  EXPECT_TRUE(QFile::exists(Elsewhere.path() + QStringLiteral("/kept.txt")));
}
//---------------------------------------------------------------------------
