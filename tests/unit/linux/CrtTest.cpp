// Functions of the Microsoft C runtime in the platform layer.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>

#include <gtest/gtest.h>

#include <cstring>
//---------------------------------------------------------------------------
TEST(CrtTest, CaseInsensitiveComparison)
{
  EXPECT_EQ(_wcsicmp(L"Hello", L"hELLO"), 0);
  EXPECT_LT(_wcsicmp(L"abc", L"ABD"), 0);
  EXPECT_GT(_wcsicmp(L"abcd", L"ABC"), 0);
  // Beyond ASCII, as the RTL
  EXPECT_EQ(_wcsicmp(L"Żółw", L"żÓŁW"), 0);
  EXPECT_EQ(_wcsnicmp(L"FEAT list", L"feat", 4), 0);
  EXPECT_NE(_wcsnicmp(L"FEAT list", L"feat", 5), 0);
  EXPECT_EQ(_stricmp("MLSD", "mlsd"), 0);
  EXPECT_EQ(_strnicmp("UTF8 ON", "utf8 off", 6), 0);
  EXPECT_NE(strnicmp("UTF8 ON", "utf8 off", 7), 0);
}
//---------------------------------------------------------------------------
TEST(CrtTest, Conversions)
{
  EXPECT_EQ(_wtoi(L"  42abc"), 42);
  EXPECT_EQ(_wtoi(L"x"), 0);
  EXPECT_EQ(_wtoi64(L"-9000000000"), -9000000000LL);
  EXPECT_EQ(_atoi64("12345678901"), 12345678901LL);
  char Text[] = "MixEd 123";
  EXPECT_EQ(strlwr(Text), Text);
  EXPECT_STREQ(Text, "mixed 123");
  const wchar_t * String = L"ab";
  EXPECT_EQ(_wcsinc(String), String + 1);
}
//---------------------------------------------------------------------------
TEST(CrtTest, Formatting)
{
  char Buffer[8];
  EXPECT_EQ(_snprintf(Buffer, sizeof(Buffer), "%u", 2121u), 4);
  EXPECT_STREQ(Buffer, "2121");
  // Not terminated when it fills the buffer, -1 when it does not fit
  memset(Buffer, 'x', sizeof(Buffer));
  EXPECT_EQ(_snprintf(Buffer, 4, "%s", "abcd"), 4);
  EXPECT_EQ(memcmp(Buffer, "abcdx", 5), 0);
  EXPECT_EQ(_snprintf(Buffer, 4, "%s", "abcdef"), -1);

  wchar_t Wide[32];
  // %s is a wide string in a wide format, as on Windows
  EXPECT_EQ(_snwprintf(Wide, 32, L"%s=%d", L"port", 21), 7);
  EXPECT_STREQ(Wide, L"port=21");
  EXPECT_EQ(snwprintf(Wide, 3, L"%s", L"long"), -1);
  EXPECT_EQ(swprintf(Wide, L"%d:%s", 7, L"x"), 3);
  EXPECT_STREQ(Wide, L"7:x");
}
//---------------------------------------------------------------------------
