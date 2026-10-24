// Remote files: parsing of "ls -l" listings (SCP), permissions, remote paths.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <RemoteFiles.h>
#include <Terminal.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
// A file parsed from a listing line, of a terminal that is not connected
// (the time of a listing is adjusted by the session's DST mode)
class TListingTest : public testing::Test
{
protected:
  void SetUp() override
  {
    FSessionData.reset(new TSessionData(L""));
    FTerminal.reset(new TTerminal(FSessionData.get(), Configuration));
  }

  std::unique_ptr<TRemoteFile> Parse(const UnicodeString & Line)
  {
    std::unique_ptr<TRemoteFile> File(new TRemoteFile());
    File->Terminal = FTerminal.get();
    File->ListingStr = Line;
    return File;
  }

  std::unique_ptr<TSessionData> FSessionData;
  std::unique_ptr<TTerminal> FTerminal;
};
}
//---------------------------------------------------------------------------
TEST_F(TListingTest, RegularFile)
{
  std::unique_ptr<TRemoteFile> File = Parse(L"-rw-r--r--   1 alice  staff      1234 Jan  2  2020 file.txt");
  EXPECT_EQ(File->FileName, L"file.txt");
  EXPECT_EQ(File->Size, 1234);
  EXPECT_EQ(File->Type, L'-');
  EXPECT_FALSE(File->IsDirectory);
  EXPECT_FALSE(File->IsSymLink);
  EXPECT_EQ(File->Owner.Name, L"alice");
  EXPECT_EQ(File->Group.Name, L"staff");
  EXPECT_EQ(File->RightsStr, L"rw-r--r--");
  EXPECT_EQ(File->Rights->Number, 0644);
  EXPECT_EQ(File->ModificationFmt, mfMDY);
  EXPECT_EQ(double(File->Modification), double(EncodeDateVerbose(2020, 1, 2)));
}
//---------------------------------------------------------------------------
TEST_F(TListingTest, DirectoryAndLink)
{
  std::unique_ptr<TRemoteFile> Directory = Parse(L"drwxr-xr-x  2 root root 4096 Mar 15  2021 dir");
  EXPECT_TRUE(Directory->IsDirectory);
  EXPECT_EQ(Directory->FileName, L"dir");

  std::unique_ptr<TRemoteFile> Link = Parse(L"lrwxrwxrwx  1 root root    7 Mar 15  2021 link -> target");
  EXPECT_TRUE(Link->IsSymLink);
  EXPECT_EQ(Link->FileName, L"link");
  EXPECT_EQ(Link->LinkTo, L"target");
}
//---------------------------------------------------------------------------
TEST_F(TListingTest, SpacesAndNonAscii)
{
  EXPECT_EQ(Parse(L"-rw-r--r-- 1 u g 5 Jan  2  2020 my file.txt")->FileName, L"my file.txt");
  EXPECT_EQ(Parse(L"-rw-r--r-- 1 u g 5 Jan  2  2020 \u017e\u00e1ba \U0001F600")->FileName, L"\u017e\u00e1ba \U0001F600");
}
//---------------------------------------------------------------------------
TEST_F(TListingTest, FullTime)
{
  FSessionData->DSTMode = dstmUnix;
  std::unique_ptr<TRemoteFile> File =
    Parse(L"-rw-r--r-- 1 u g 5 2020-01-02 10:30:15.000000000 +0000 f");
  EXPECT_EQ(File->FileName, L"f");
  EXPECT_EQ(File->ModificationFmt, mfFull);
  EXPECT_EQ(
    double(File->Modification), double(EncodeDateVerbose(2020, 1, 2) + EncodeTimeVerbose(10, 30, 15, 0)));
}
//---------------------------------------------------------------------------
TEST_F(TListingTest, RecentFileWithoutYear)
{
  FSessionData->DSTMode = dstmUnix;
  std::unique_ptr<TRemoteFile> File = Parse(L"-rw-r--r-- 1 u g 5 Jan  1 00:00 f");
  EXPECT_EQ(File->ModificationFmt, mfMDHM);
  // The year is the current one, or the previous one for dates in the future
  Word Year, Month, Day;
  DecodeDate(File->Modification, Year, Month, Day);
  Word CurrentYear, CurrentMonth, CurrentDay;
  DecodeDate(Date(), CurrentYear, CurrentMonth, CurrentDay);
  EXPECT_EQ(Year, CurrentYear);
  EXPECT_EQ(Month, 1);
  EXPECT_EQ(Day, 1);
}
//---------------------------------------------------------------------------
TEST_F(TListingTest, Invalid)
{
  EXPECT_THROW(Parse(L"total 12"), ETerminal);
  EXPECT_THROW(Parse(L"garbage"), ETerminal);
}
//---------------------------------------------------------------------------
TEST(RightsTest, TextAndNumber)
{
  TRights Rights;
  Rights.Text = L"rwxr-x---";
  EXPECT_EQ(Rights.Number, 0750);
  EXPECT_EQ(Rights.Octal, L"0750");
  Rights.Number = 0644;
  EXPECT_EQ(Rights.Text, L"rw-r--r--");
  Rights.Octal = L"4755";
  EXPECT_EQ(Rights.Text, L"rwsr-xr-x");
  Rights.Text = L"rwxrwxrwt";
  EXPECT_EQ(Rights.Number, 01777);
  EXPECT_EQ(Rights.ModeStr, L"u+rwx-s,g+rwx-s,o+rwxt");
  EXPECT_EQ(TRights(0600).Text, L"rw-------");
}
//---------------------------------------------------------------------------
TEST(RemotePathsTest, Paths)
{
  EXPECT_EQ(UnixIncludeTrailingBackslash(L"/home"), L"/home/");
  EXPECT_EQ(UnixExcludeTrailingBackslash(L"/home/"), L"/home");
  EXPECT_EQ(UnixExcludeTrailingBackslash(L"/"), L"/");
  EXPECT_EQ(UnixExtractFileName(L"/home/user/a.txt"), L"a.txt");
  EXPECT_EQ(UnixExtractFilePath(L"/home/user/a.txt"), L"/home/user/");
  EXPECT_EQ(UnixExtractFileDir(L"/home/user/a.txt"), L"/home/user");
  // (of a name only: with a path, the upstream function returns a wrong
  // part of it; its only caller passes a name)
  EXPECT_EQ(UnixExtractFileExt(L"a.tar.gz"), L".gz");
  EXPECT_EQ(UnixCombinePaths(L"/home", L"user"), L"/home/user");
  EXPECT_EQ(AbsolutePath(L"/home/user", L"../other/./x"), L"/home/other/x");
  EXPECT_EQ(AbsolutePath(L"/home/user", L"/etc"), L"/etc");
  EXPECT_TRUE(UnixIsAbsolutePath(L"/etc"));
  EXPECT_FALSE(UnixIsAbsolutePath(L"etc"));
  EXPECT_TRUE(UnixIsChildPath(L"/home", L"/home/user/a"));
  EXPECT_FALSE(UnixIsChildPath(L"/home", L"/homer"));
  EXPECT_TRUE(UnixSamePath(L"/home/", L"/home"));
  EXPECT_TRUE(IsUnixRootPath(L"/"));
  EXPECT_TRUE(IsUnixHiddenFile(L".bashrc"));
  EXPECT_FALSE(IsUnixHiddenFile(L".."));
}
//---------------------------------------------------------------------------
