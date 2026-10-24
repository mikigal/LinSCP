// The core's general helpers (core/Common.cpp).
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(CommonTest, Strings)
{
  UnicodeString Str = L"a, b,c";
  EXPECT_EQ(CutToChar(Str, L',', true), L"a");
  EXPECT_EQ(CutToChar(Str, L',', true), L"b");
  EXPECT_EQ(CutToChar(Str, L',', true), L"c");
  EXPECT_TRUE(Str.IsEmpty());
  EXPECT_TRUE(IsNumber(L"123"));
  EXPECT_FALSE(IsNumber(L"12a"));
  EXPECT_EQ(DelimitStr(L"a\"b"), L"a\\\"b");
  EXPECT_EQ(StripPathQuotes(L"\"/tmp/a b\""), L"/tmp/a b");
  EXPECT_EQ(AddPathQuotes(L"/tmp/a b"), L"\"/tmp/a b\"");
  EXPECT_EQ(AddPathQuotes(L"/tmp/ab"), L"/tmp/ab");
  EXPECT_EQ(EscapeHotkey(L"A&B"), L"A&&B");
}
//---------------------------------------------------------------------------
TEST(CommonTest, Urls)
{
  EXPECT_EQ(EncodeUrlString(L"a b/c@d"), L"a%20b%2Fc%40d");
  EXPECT_EQ(DecodeUrlChars(L"a%20b%2Fc%40d"), L"a b/c@d");
  EXPECT_EQ(DecodeUrlChars(L"%C5%BE"), L"\u017e");
  EXPECT_TRUE(IsHttpUrl(L"http://example.com/"));
  EXPECT_FALSE(IsHttpUrl(L"https://example.com/"));
  EXPECT_TRUE(IsHttpOrHttpsUrl(L"https://example.com/"));
  EXPECT_FALSE(IsHttpOrHttpsUrl(L"sftp://example.com/"));
  EXPECT_EQ(ChangeUrlProtocol(L"http://example.com/", L"https"), L"https://example.com/");
  EXPECT_TRUE(IsDomainOrSubdomain(L"www.example.com", L"example.com"));
  EXPECT_FALSE(IsDomainOrSubdomain(L"wwwexample.com", L"example.com"));
}
//---------------------------------------------------------------------------
TEST(CommonTest, Sizes)
{
  __int64 Size = 0;
  EXPECT_TRUE(TryStrToSize(L"10K", Size));
  EXPECT_EQ(Size, 10 * 1024);
  EXPECT_TRUE(TryStrToSize(L"2M", Size));
  EXPECT_EQ(Size, 2 * 1024 * 1024);
  EXPECT_FALSE(TryStrToSize(L"abc", Size));
  EXPECT_EQ(SizeToStr(10 * 1024), L"10K");
  EXPECT_EQ(SizeToStr(1000), L"1000");
}
//---------------------------------------------------------------------------
TEST(CommonTest, Checksums)
{
  EXPECT_TRUE(SameChecksum(L"ABCDEF", L"abcdef", false));
  EXPECT_FALSE(SameChecksum(L"ABCDEF", L"abcdee", false));
}
//---------------------------------------------------------------------------
TEST(CommonTest, Version)
{
  EXPECT_EQ(TrimVersion(L"1.2.0.0"), L"1.2");
  EXPECT_EQ(TrimVersion(L"1.2.3"), L"1.2.3");
}
//---------------------------------------------------------------------------
