// TFileMasks: WinSCP's file masks (https://winscp.net/eng/docs/file_mask):
// include/exclude masks, directory and path masks, size and time
// conditions.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <FileMasks.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
bool RemoteFile(const UnicodeString & Masks, const UnicodeString & FileName)
{
  return TFileMasks(Masks).Matches(FileName, false, false);
}
//---------------------------------------------------------------------------
bool RemoteDirectory(const UnicodeString & Masks, const UnicodeString & FileName)
{
  return TFileMasks(Masks).Matches(FileName, false, true);
}
//---------------------------------------------------------------------------
bool LocalFile(const UnicodeString & Masks, const UnicodeString & FileName)
{
  return TFileMasks(Masks).Matches(FileName, true, false);
}
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, FileNameMasks)
{
  EXPECT_TRUE(RemoteFile(L"*.txt", L"a.txt"));
  EXPECT_FALSE(RemoteFile(L"*.txt", L"a.txt.bak"));
  EXPECT_TRUE(RemoteFile(L"*.txt; *.doc", L"b.doc"));
  EXPECT_FALSE(RemoteFile(L"*.txt; *.doc", L"c.bin"));
  EXPECT_TRUE(RemoteFile(L"a?c", L"abc"));
  EXPECT_FALSE(RemoteFile(L"a?c", L"abbc"));
  EXPECT_TRUE(RemoteFile(L"[a-c]*", L"b1"));
  EXPECT_FALSE(RemoteFile(L"[a-c]*", L"d1"));
  // Case-insensitive, as in WinSCP
  EXPECT_TRUE(RemoteFile(L"*.TXT", L"a.txt"));
  // "*.*" is any file, "*." a file without an extension
  EXPECT_TRUE(RemoteFile(L"*.*", L"README"));
  EXPECT_TRUE(RemoteFile(L"*.", L"README"));
  EXPECT_FALSE(RemoteFile(L"*.", L"a.txt"));
  // An empty mask matches everything
  EXPECT_TRUE(RemoteFile(L"", L"a.txt"));
  EXPECT_TRUE(RemoteDirectory(L"", L"dir"));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, ExcludeMasks)
{
  EXPECT_TRUE(RemoteFile(L"* | *.bak", L"a.txt"));
  EXPECT_FALSE(RemoteFile(L"* | *.bak", L"a.bak"));
  EXPECT_FALSE(RemoteFile(L"| *.bak", L"a.bak"));
  EXPECT_TRUE(RemoteFile(L"| *.bak", L"a.txt"));
  EXPECT_FALSE(RemoteFile(L"*.txt | a*", L"abc.txt"));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, DirectoryMasks)
{
  // Directory masks end with a slash and do not apply to files
  EXPECT_FALSE(RemoteDirectory(L"| .git/", L"/home/user/.git"));
  EXPECT_TRUE(RemoteDirectory(L"| .git/", L"/home/user/src"));
  EXPECT_TRUE(RemoteFile(L"| .git/", L"/home/user/.git"));
  // File masks do not apply to directories
  EXPECT_TRUE(RemoteDirectory(L"*.txt", L"/home/user/src"));
  EXPECT_FALSE(RemoteFile(L"*.txt", L"/home/user/src"));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, PathMasks)
{
  EXPECT_TRUE(RemoteFile(L"/data/*.txt", L"/data/a.txt"));
  EXPECT_FALSE(RemoteFile(L"/data/*.txt", L"/other/a.txt"));
  EXPECT_TRUE(RemoteFile(L"*/logs/*", L"/var/app/logs/today"));
  EXPECT_FALSE(RemoteFile(L"*/logs/*", L"/var/app/today"));
  // Local paths use slashes as well
  EXPECT_TRUE(LocalFile(L"/tmp/*.txt", L"/tmp/a.txt"));
  EXPECT_FALSE(LocalFile(L"/tmp/*.txt", L"/var/a.txt"));
  EXPECT_TRUE(LocalFile(L"*.txt", L"/tmp/sub/a.txt"));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, SizeMasks)
{
  TFileMasks Masks(L">1K");
  TFileMasks::TParams Params;
  Params.Size = 1024;
  EXPECT_FALSE(Masks.Matches(L"a", false, false, &Params));
  Params.Size = 1025;
  EXPECT_TRUE(Masks.Matches(L"a", false, false, &Params));
  // Without the file's parameters, a size mask does not match
  EXPECT_FALSE(Masks.Matches(L"a", false, false, NULL));

  TFileMasks Range(L"*.log>=1K<=2K");
  Params.Size = 2048;
  EXPECT_TRUE(Range.Matches(L"a.log", false, false, &Params));
  Params.Size = 2049;
  EXPECT_FALSE(Range.Matches(L"a.log", false, false, &Params));
  Params.Size = 1024;
  EXPECT_FALSE(Range.Matches(L"a.txt", false, false, &Params));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, TimeMasks)
{
  TFileMasks Masks(L">2020-01-01");
  TFileMasks::TParams Params;
  Params.Modification = EncodeDateVerbose(2020, 1, 2);
  EXPECT_TRUE(Masks.Matches(L"a", false, false, &Params));
  Params.Modification = EncodeDateVerbose(2019, 12, 31);
  EXPECT_FALSE(Masks.Matches(L"a", false, false, &Params));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, Errors)
{
  EXPECT_THROW(TFileMasks(L"*.txt | *.bak | *.tmp"), EFileMasksException);
  EXPECT_THROW(TFileMasks(L">abc"), EFileMasksException);
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, MaskFileName)
{
  EXPECT_EQ(MaskFileName(L"file.txt", L"*.bak"), L"file.bak");
  EXPECT_EQ(MaskFileName(L"file.txt", L"*.*"), L"file.txt");
  EXPECT_EQ(MaskFileName(L"file.txt", L"*"), L"file.txt");
  EXPECT_EQ(MaskFileName(L"file.txt", L"other.dat"), L"other.dat");
  EXPECT_TRUE(IsFileNameMask(L"*.bak"));
  EXPECT_FALSE(IsFileNameMask(L"file.bak"));
  EXPECT_FALSE(IsEffectiveFileNameMask(L"*.*"));
  EXPECT_TRUE(IsEffectiveFileNameMask(L"*.bak"));
}
//---------------------------------------------------------------------------
TEST(FileMasksTest, Composition)
{
  EXPECT_TRUE(TFileMasks::IsMask(L"*.txt"));
  EXPECT_FALSE(TFileMasks::IsMask(L"a.txt"));
  EXPECT_EQ(TFileMasks::NormalizeMask(L"*.*"), L"");
  EXPECT_EQ(TFileMasks::NormalizeMask(L"*.txt"), L"*.txt");
  std::unique_ptr<TStringList> Include(new TStringList());
  Include->Add(L"*.txt");
  Include->Add(L"*.doc");
  std::unique_ptr<TStringList> Exclude(new TStringList());
  Exclude->Add(L"a*");
  std::unique_ptr<TStringList> Empty(new TStringList());
  EXPECT_EQ(TFileMasks::ComposeMaskStr(Include.get(), Exclude.get(), Empty.get(), Empty.get()), L"*.txt; *.doc | a*");
}
//---------------------------------------------------------------------------
