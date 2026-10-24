// The core's resource strings, generated from source/resource/TextsCore*.rc
// by tools/rc2cpp, are found by LoadStr.
//---------------------------------------------------------------------------
#include <System.hpp>
#include <SysUtils.hpp>

#include <TextsCore.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(ResourceStringsTest, SingleLine)
{
  EXPECT_EQ(LoadStr(CONNECTION_FAILED), L"Connection failed.");
  EXPECT_EQ(LoadStr(CHANGE_DIR_ERROR), L"Error changing directory to '%s'.");
}
//---------------------------------------------------------------------------
TEST(ResourceStringsTest, ConcatenatedLines)
{
  UnicodeString Help = LoadStr(SCRIPT_HELP_HELP);
  EXPECT_TRUE(Help.Pos(L"help [ <command> [ <command2> ... ] ]\n  Displays list") == 1) << Help;
  EXPECT_EQ(Help[Help.Length()], L'\n');
}
//---------------------------------------------------------------------------
TEST(ResourceStringsTest, NonAscii)
{
  EXPECT_EQ(LoadStr(PUTTY_COPYRIGHT).SubString(1, 11), L"Copyright ©");
}
//---------------------------------------------------------------------------
TEST(ResourceStringsTest, Formatted)
{
  EXPECT_EQ(FmtLoadStr(CHANGE_DIR_ERROR, ARRAYOFCONST((L"/tmp"))), L"Error changing directory to '/tmp'.");
}
//---------------------------------------------------------------------------
