// Transfer settings: file name changes, transfer mode, resume, file masks.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <CopyParam.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(CopyParamTest, Defaults)
{
  TCopyParamType CopyParam;
  EXPECT_EQ(CopyParam.TransferMode, tmBinary);
  EXPECT_EQ(CopyParam.FileNameCase, ncNoChange);
  EXPECT_TRUE(CopyParam.PreserveTime);
  EXPECT_EQ(CopyParam.ResumeSupport, rsSmart);
  EXPECT_EQ(CopyParam.FileMask, L"*.*");
}
//---------------------------------------------------------------------------
TEST(CopyParamTest, FileNameCase)
{
  TCopyParamType CopyParam;
  CopyParam.FileNameCase = ncUpperCase;
  EXPECT_EQ(CopyParam.ChangeFileName(L"Abc.txt", osRemote, true), L"ABC.TXT");
  CopyParam.FileNameCase = ncLowerCase;
  EXPECT_EQ(CopyParam.ChangeFileName(L"Abc.TXT", osRemote, true), L"abc.txt");
  CopyParam.FileNameCase = ncFirstUpperCase;
  EXPECT_EQ(CopyParam.ChangeFileName(L"aBC.TXT", osRemote, true), L"Abc.txt");
}
//---------------------------------------------------------------------------
TEST(CopyParamTest, OperationMask)
{
  TCopyParamType CopyParam;
  CopyParam.FileMask = L"*.bak";
  // Applies to the files selected for the operation only
  EXPECT_EQ(CopyParam.ChangeFileName(L"a.txt", osRemote, true), L"a.bak");
  EXPECT_EQ(CopyParam.ChangeFileName(L"a.txt", osRemote, false), L"a.txt");
}
//---------------------------------------------------------------------------
TEST(CopyParamTest, TransferMode)
{
  TCopyParamType CopyParam;
  TFileMasks::TParams Params;
  EXPECT_FALSE(CopyParam.UseAsciiTransfer(L"a.txt", osLocal, Params));
  CopyParam.TransferMode = tmAscii;
  EXPECT_TRUE(CopyParam.UseAsciiTransfer(L"a.bin", osLocal, Params));
  CopyParam.TransferMode = tmAutomatic;
  CopyParam.AsciiFileMask = TFileMasks(L"*.txt; *.html");
  EXPECT_TRUE(CopyParam.UseAsciiTransfer(L"/tmp/a.txt", osLocal, Params));
  EXPECT_TRUE(CopyParam.UseAsciiTransfer(L"/home/a.html", osRemote, Params));
  EXPECT_FALSE(CopyParam.UseAsciiTransfer(L"/tmp/a.bin", osLocal, Params));
}
//---------------------------------------------------------------------------
TEST(CopyParamTest, Resume)
{
  TCopyParamType CopyParam;
  CopyParam.ResumeSupport = rsSmart;
  CopyParam.ResumeThreshold = 100 * 1024;
  EXPECT_FALSE(CopyParam.AllowResume(1024, L"a.txt"));
  EXPECT_TRUE(CopyParam.AllowResume(1024 * 1024, L"a.txt"));
  CopyParam.ResumeSupport = rsOn;
  EXPECT_TRUE(CopyParam.AllowResume(1, L"a.txt"));
  CopyParam.ResumeSupport = rsOff;
  EXPECT_FALSE(CopyParam.AllowResume(1024 * 1024, L"a.txt"));
}
//---------------------------------------------------------------------------
TEST(CopyParamTest, IncludeMask)
{
  TCopyParamType CopyParam;
  CopyParam.IncludeFileMask = TFileMasks(L"*.txt | /tmp/skip/");
  TFileMasks::TParams Params;
  EXPECT_TRUE(CopyParam.AllowTransfer(L"/tmp/a.txt", osLocal, false, Params, false));
  EXPECT_FALSE(CopyParam.AllowTransfer(L"/tmp/a.bin", osLocal, false, Params, false));
  EXPECT_FALSE(CopyParam.AllowTransfer(L"/tmp/skip", osLocal, true, Params, false));
  CopyParam.ExcludeHiddenFiles = true;
  EXPECT_FALSE(CopyParam.AllowTransfer(L"/tmp/.a.txt", osLocal, false, Params, true));
}
//---------------------------------------------------------------------------
TEST(CopyParamTest, SpeedLimit)
{
  EXPECT_EQ(GetSpeedLimit(L"Unlimited"), 0u);
  EXPECT_EQ(GetSpeedLimit(L"1024"), 1024u * 1024u);
  EXPECT_EQ(SetSpeedLimit(0), L"Unlimited");
  EXPECT_EQ(SetSpeedLimit(512 * 1024), L"512");
}
//---------------------------------------------------------------------------
