// L3 tests of the import of sites (TAppConfiguration's import of PuTTY's
// sessions, dialogs/ImportSessionsDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <SessionData.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
void WriteFile(const QString & FileName, const QByteArray & Content)
{
  QDir().mkpath(QFileInfo(FileName).path());
  QFile File(FileName);
  ASSERT_TRUE(File.open(QIODevice::WriteOnly));
  File.write(Content);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(ImportSessionsTest, PuttySessions)
{
  QString Putty = QDir(QString::fromLocal8Bit(qgetenv("HOME"))).filePath(QStringLiteral(".putty"));
  WriteFile(Putty + QStringLiteral("/sessions/my%20server"),
    "HostName=putty.example.com\nPortNumber=2222\nUserName=alice\nProtocol=ssh\n");
  WriteFile(Putty + QStringLiteral("/sessions/Default%20Settings"), "HostName=\n");
  EXPECT_EQ(ToQString(TAppConfiguration::GetPuttyFolder()), Putty);

  std::unique_ptr<TStoredSessionList> Sessions(new TStoredSessionList(true));
  UnicodeString Error;
  std::unique_ptr<TStoredSessionList> Imported(AppConfiguration->SelectPuttySessionsForImport(Sessions.get(), Error));
  EXPECT_TRUE(Error.IsEmpty()) << ToQString(Error).toStdString();
  ASSERT_EQ(Imported->Count, 1);
  TSessionData * Data = Imported->Sessions[0];
  EXPECT_EQ(Data->Name, UnicodeString(L"my server"));
  EXPECT_EQ(Data->HostName, UnicodeString(L"putty.example.com"));
  EXPECT_EQ(Data->PortNumber, 2222);
  EXPECT_EQ(Data->UserName, UnicodeString(L"alice"));
  EXPECT_TRUE(Data->Selected);
  QDir(Putty).removeRecursively();
}
//---------------------------------------------------------------------------
