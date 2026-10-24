// UnicodeString, AnsiStringT and character set conversions
//---------------------------------------------------------------------------
#include <System.hpp>
#include <SysUtils.hpp>

#include <gtest/gtest.h>

#include <unordered_map>
//---------------------------------------------------------------------------
TEST(UnicodeString, Construction)
{
  UnicodeString Empty;
  EXPECT_TRUE(Empty.IsEmpty());
  EXPECT_EQ(Empty.Length(), 0);
  ASSERT_NE(Empty.c_str(), nullptr);
  EXPECT_EQ(Empty.c_str()[0], L'\0');

  EXPECT_EQ(UnicodeString(static_cast<const wchar_t *>(nullptr)), L"");
  EXPECT_EQ(UnicodeString(L"abc"), L"abc");
  UnicodeString WithNul(L"a\0b", 3);
  EXPECT_EQ(WithNul.Length(), 3);
  EXPECT_EQ(WithNul[3], L'b');
  EXPECT_EQ(UnicodeString(L"abcdef", 3), L"abc");
  EXPECT_EQ(UnicodeString("za\xC5\xBC\xC3\xB3\xC5\x82\xC4\x87").Length(), 6);
  EXPECT_EQ(UnicodeString(L'x'), L"x");
  EXPECT_EQ(UnicodeString('a'), L"a");
  EXPECT_EQ(UnicodeString(42), L"42");
  EXPECT_EQ(UnicodeString(-5), L"-5");
  EXPECT_EQ(UnicodeString(18446744073709551615ULL), L"18446744073709551615");
  EXPECT_EQ(UnicodeString(1.5), L"1.5");
  // Non-BMP characters are a single wchar_t
  EXPECT_EQ(UnicodeString(L"\U0001F600").Length(), 1);
}
//---------------------------------------------------------------------------
TEST(UnicodeString, IndexingIsOneBased)
{
  UnicodeString S = L"Hello";
  EXPECT_EQ(S[1], L'H');
  EXPECT_EQ(S[5], L'o');
  S[1] = L'J';
  EXPECT_EQ(S, L"Jello");
  EXPECT_THROW(S[0], ERangeError);
  EXPECT_THROW(S[6], ERangeError);
  const UnicodeString & C = S;
  EXPECT_THROW(C[0], ERangeError);
}
//---------------------------------------------------------------------------
TEST(UnicodeString, SubStringHasCopySemantics)
{
  UnicodeString S = L"Hello";
  EXPECT_EQ(S.SubString(2, 3), L"ell");
  EXPECT_EQ(S.SubString(1, 5), L"Hello");
  // Index below 1 is treated as 1, the count is not reduced (Delphi's Copy)
  EXPECT_EQ(S.SubString(0, 3), L"Hel");
  EXPECT_EQ(S.SubString(-5, 3), L"Hel");
  EXPECT_EQ(S.SubString(4, 100), L"lo");
  EXPECT_EQ(S.SubString(4, MaxInt), L"lo");
  EXPECT_EQ(S.SubString(6, 1), L"");
  EXPECT_EQ(S.SubString(1, 0), L"");
  EXPECT_EQ(S.SubString(1, -1), L"");
  EXPECT_EQ(S.SubString(3), L"llo");
}
//---------------------------------------------------------------------------
TEST(UnicodeString, PosDeleteInsert)
{
  UnicodeString S = L"Hello";
  EXPECT_EQ(S.Pos(L"ll"), 3);
  EXPECT_EQ(S.Pos(L"H"), 1);
  EXPECT_EQ(S.Pos(L"x"), 0);
  EXPECT_EQ(S.Pos(L""), 0);

  EXPECT_EQ(UnicodeString(L"Hello").Delete(2, 2), L"Hlo");
  EXPECT_EQ(UnicodeString(L"Hello").Delete(0, 2), L"Hello");
  EXPECT_EQ(UnicodeString(L"Hello").Delete(5, 10), L"Hell");
  EXPECT_EQ(UnicodeString(L"Hello").Delete(6, 1), L"Hello");
  EXPECT_EQ(UnicodeString(L"Hello").Delete(2, 0), L"Hello");

  EXPECT_EQ(UnicodeString(L"Hello").Insert(L"XX", 1), L"XXHello");
  EXPECT_EQ(UnicodeString(L"Hello").Insert(L"XX", 3), L"HeXXllo");
  EXPECT_EQ(UnicodeString(L"Hello").Insert(L"XX", 6), L"HelloXX");
  EXPECT_EQ(UnicodeString(L"Hello").Insert(L"XX", 100), L"HelloXX");
  EXPECT_EQ(UnicodeString(L"Hello").Insert(L"XX", 0), L"XXHello");
}
//---------------------------------------------------------------------------
TEST(UnicodeString, SetLength)
{
  UnicodeString S = L"Hello";
  S.SetLength(2);
  EXPECT_EQ(S, L"He");
  S.SetLength(4);
  EXPECT_EQ(S.Length(), 4);
  EXPECT_EQ(S[3], L'\0');
  S.SetLength(-1);
  EXPECT_TRUE(S.IsEmpty());
  // The buffer is writable through c_str(), as in C++Builder
  S.SetLength(3);
  wcscpy(S.c_str(), L"abc");
  EXPECT_EQ(S, L"abc");
}
//---------------------------------------------------------------------------
TEST(UnicodeString, TrimAndCase)
{
  EXPECT_EQ(UnicodeString(L" \t a b \r\n").Trim(), L"a b");
  EXPECT_EQ(UnicodeString(L"  a ").TrimLeft(), L"a ");
  EXPECT_EQ(UnicodeString(L"  a ").TrimRight(), L"  a");
  // Only characters up to ' ' are blanks
  EXPECT_EQ(UnicodeString(L"\u00A0a").Trim(), L"\u00A0a");
  EXPECT_EQ(UnicodeString(L"za\u017C\u00F3\u0142\u0107").UpperCase(), L"ZA\u017B\u00D3\u0141\u0106");
  EXPECT_EQ(UnicodeString(L"ZA\u017B\u00D3\u0141\u0106").LowerCase(), L"za\u017C\u00F3\u0142\u0107");
}
//---------------------------------------------------------------------------
TEST(UnicodeString, Delimiters)
{
  UnicodeString S = L"a/b\\c";
  EXPECT_EQ(S.LastDelimiter(L"/\\"), 4);
  EXPECT_EQ(S.LastDelimiter(L"/"), 2);
  EXPECT_EQ(S.LastDelimiter(L"x"), 0);
  EXPECT_TRUE(S.IsDelimiter(L"/", 2));
  EXPECT_FALSE(S.IsDelimiter(L"/", 1));
  EXPECT_FALSE(S.IsDelimiter(L"/", 0));
  EXPECT_FALSE(S.IsDelimiter(L"/", 10));
  EXPECT_TRUE(S.IsPathDelimiter(2));
  EXPECT_FALSE(S.IsPathDelimiter(4));
}
//---------------------------------------------------------------------------
TEST(UnicodeString, Comparison)
{
  UnicodeString A = L"abc";
  EXPECT_TRUE(A == L"abc");
  EXPECT_TRUE(L"abc" == A);
  EXPECT_TRUE(A != L"abd");
  EXPECT_TRUE(A < UnicodeString(L"abd"));
  // Ordinal
  EXPECT_TRUE(UnicodeString(L"B") < UnicodeString(L"a"));
  EXPECT_EQ(A.CompareIC(L"ABC"), 0);
  EXPECT_LT(A.CompareIC(L"ABD"), 0);
  EXPECT_LT(UnicodeString(L"a").Compare(L"B"), 0);
  EXPECT_EQ(A.Compare(L"abc"), 0);
}
//---------------------------------------------------------------------------
TEST(UnicodeString, Concatenation)
{
  UnicodeString S = L"b";
  EXPECT_EQ(L"a" + S, L"ab");
  EXPECT_EQ(S + L"c", L"bc");
  EXPECT_EQ(S + S, L"bb");
  EXPECT_EQ(S + L'x', L"bx");
  EXPECT_EQ("n" + S, L"nb");
  S += L"c";
  S += L'd';
  S += UnicodeString(L"e");
  EXPECT_EQ(S, L"bcde");
  EXPECT_EQ(S + 5, L"bcde5");
}
//---------------------------------------------------------------------------
TEST(UnicodeString, Printf)
{
  UnicodeString S;
  S.sprintf(L"%s=%d", L"x", 5);
  EXPECT_EQ(S, L"x=5");
  EXPECT_EQ(S.sprintf(L"%hs|%S", "narrow", "too"), L"narrow|too");
  EXPECT_EQ(S.sprintf(L"%5.2f|%-4s|%4s", 3.14159, L"ab", L"cd"), L" 3.14|ab  |  cd");
  EXPECT_EQ(S.sprintf(L"%Ld %I64u %lld", 1LL << 40, 1ULL << 63, -1LL),
    L"1099511627776 9223372036854775808 -1");
  EXPECT_EQ(S.sprintf(L"%x %X %08x %o", 255, 255, 255, 8), L"ff FF 000000ff 10");
  // long is 32-bit on Windows
  EXPECT_EQ(S.sprintf(L"%ld %lu %lx", -2, 4000000000U, 0xFFFFFFFFU), L"-2 4000000000 ffffffff");
  EXPECT_EQ(S.sprintf(L"%c%c%%", L'\u0142', L'b'), L"\u0142b%");
  EXPECT_EQ(S.sprintf(L"%.2s|%*d", L"abcdef", 4, 7), L"ab|   7");
  EXPECT_EQ(S.sprintf(L"%s", static_cast<const wchar_t *>(nullptr)), L"(null)");
  EXPECT_EQ(S.sprintf(L"%hs", "za\xC5\xBC"), L"za\u017C");
  S.cat_sprintf(L"-%d", 1);
  EXPECT_EQ(S, L"za\u017C-1");
}
//---------------------------------------------------------------------------
TEST(UnicodeString, Misc)
{
  EXPECT_EQ(UnicodeString::StringOfChar(L'-', 3), L"---");
  EXPECT_EQ(UnicodeString::StringOfChar(L'-', 0), L"");
  EXPECT_EQ(UnicodeString(L"42").ToInt(), 42);
  EXPECT_EQ(UnicodeString(L"x").ToIntDef(7), 7);
  EXPECT_THROW(UnicodeString(L"x").ToInt(), EConvertError);

  std::unordered_map<UnicodeString, int> Map;
  Map[L"a"] = 1;
  Map[L"b"] = 2;
  EXPECT_EQ(Map[L"a"], 1);

  int Count = 0;
  for (wchar_t Ch : UnicodeString(L"abc"))
  {
    Count += (Ch != 0) ? 1 : 0;
  }
  EXPECT_EQ(Count, 3);
}
//---------------------------------------------------------------------------
TEST(AnsiString, Utf8ByDefault)
{
  UnicodeString Wide = L"za\u017C\u00F3\u0142\u0107";
  AnsiString Ansi = Wide;
  EXPECT_EQ(Ansi.Length(), 10);
  EXPECT_EQ(UnicodeString(Ansi), Wide);
  UTF8String Utf8 = Wide;
  EXPECT_EQ(Utf8.Std(), Ansi.Std());
  RawByteString Raw = Utf8;
  EXPECT_EQ(Raw.Std(), Ansi.Std());
  AnsiString Back = Raw;
  EXPECT_EQ(Back.Std(), Ansi.Std());
  EXPECT_EQ(Ansi.CodePage(), 65001);
}
//---------------------------------------------------------------------------
TEST(AnsiString, BinaryData)
{
  RawByteString Raw("a\0b", 3);
  EXPECT_EQ(Raw.Length(), 3);
  EXPECT_EQ(Raw[2], '\0');
  // Invalid UTF-8 survives a round trip through UnicodeString
  RawByteString Invalid("\xFF\xFE\xC3", 3);
  UnicodeString Wide = Invalid;
  EXPECT_EQ(Wide.Length(), 3);
  RawByteString Again = Wide;
  EXPECT_EQ(Again.Std(), Invalid.Std());
}
//---------------------------------------------------------------------------
TEST(AnsiString, OtherCodePages)
{
  AnsiStringT<1250> Cp1250 = UnicodeString(L"\u0105\u0119");
  ASSERT_EQ(Cp1250.Length(), 2);
  EXPECT_EQ(static_cast<unsigned char>(Cp1250[1]), 0xB9);
  EXPECT_EQ(static_cast<unsigned char>(Cp1250[2]), 0xEA);
  EXPECT_EQ(UnicodeString(Cp1250), L"\u0105\u0119");
  // Converting between code pages goes through Unicode
  UTF8String Utf8 = Cp1250;
  EXPECT_EQ(Utf8.Std(), "\xC4\x85\xC4\x99");
  // Not representable
  AnsiStringT<1250> Unknown = UnicodeString(L"\u4E2D");
  EXPECT_EQ(Unknown.Std(), "?");
}
//---------------------------------------------------------------------------
TEST(AnsiString, Methods)
{
  AnsiString S = "  Hello World ";
  EXPECT_EQ(S.Trim(), "Hello World");
  EXPECT_EQ(S.Trim().SubString(7, 5), "World");
  EXPECT_EQ(S.Pos("World"), 9);
  EXPECT_EQ(AnsiString("abc").Delete(2, 1), "ac");
  EXPECT_EQ(AnsiString("ac").Insert("b", 2), "abc");
  EXPECT_EQ(AnsiString("Abc").LowerCase(), "abc");
  EXPECT_EQ(AnsiString("\xC5\xBC").UpperCase(), "\xC5\xBB");
  EXPECT_EQ(AnsiString("a/b/c").LastDelimiter("/"), 4);
  EXPECT_EQ(AnsiString("12").ToInt(), 12);
  EXPECT_EQ(AnsiString(5), "5");
  EXPECT_EQ(AnsiString("x") + "y", "xy");
  EXPECT_EQ("x" + AnsiString("y"), "xy");
  EXPECT_THROW(AnsiString("ab")[3], ERangeError);

  AnsiString P;
  P.sprintf("%s-%d-%ls", "a", 1, L"\u017C");
  EXPECT_EQ(P, "a-1-\xC5\xBC");
  EXPECT_EQ(RawByteString::Format("<%d>%s", ARRAYOFCONST((5, RawByteString("x")))), "<5>x");
}
//---------------------------------------------------------------------------
TEST(Conversion, Utf8DecodingIsLossless)
{
  auto Decode = [](const char * S, size_t Len) { return rtl::DecodeUtf8(S, Len); };
  EXPECT_EQ(Decode("\xC5\xBC", 2), L"\u017C");
  EXPECT_EQ(Decode("\xF0\x9F\x98\x80", 4), L"\U0001F600");
  // Invalid lead byte, truncated sequence, overlong form, encoded surrogate, beyond U+10FFFF
  EXPECT_EQ(Decode("\xFF", 1), std::wstring(1, static_cast<wchar_t>(0xDCFF)));
  EXPECT_EQ(Decode("a\xC5", 2), (std::wstring{L'a', static_cast<wchar_t>(0xDCC5)}));
  EXPECT_EQ(Decode("\xC0\x80", 2), (std::wstring{static_cast<wchar_t>(0xDCC0), static_cast<wchar_t>(0xDC80)}));
  EXPECT_EQ(Decode("\xED\xA0\x80", 3).size(), 3u);
  EXPECT_EQ(Decode("\xF4\x90\x80\x80", 4).size(), 4u);

  const char * Samples[] = { "\xFF\xFE", "a\xC5", "\xC0\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80", "ok\xE2\x82" };
  for (const char * Sample : Samples)
  {
    std::wstring Wide = rtl::DecodeUtf8(Sample, strlen(Sample));
    EXPECT_EQ(rtl::EncodeUtf8(Wide.data(), Wide.size()), Sample);
  }
}
//---------------------------------------------------------------------------
TEST(Conversion, Utf8Encoding)
{
  // A UTF-16 surrogate pair is combined, a lone surrogate replaced
  std::wstring Pair{static_cast<wchar_t>(0xD83D), static_cast<wchar_t>(0xDE00)};
  EXPECT_EQ(rtl::EncodeUtf8(Pair.data(), Pair.size()), "\xF0\x9F\x98\x80");
  std::wstring Lone{static_cast<wchar_t>(0xD83D)};
  EXPECT_EQ(rtl::EncodeUtf8(Lone.data(), Lone.size()), "\xEF\xBF\xBD");
}
//---------------------------------------------------------------------------
TEST(Conversion, Utf16)
{
  std::wstring Wide = L"a\U0001F600\u017C";
  std::u16string Utf16 = rtl::EncodeUtf16(Wide.data(), Wide.size());
  EXPECT_EQ(Utf16.size(), 4u);
  EXPECT_EQ(rtl::DecodeUtf16(Utf16.data(), Utf16.size()), Wide);
}
//---------------------------------------------------------------------------
TEST(Conversion, CodePages)
{
  EXPECT_EQ(rtl::DecodeCodePage(1252, "\x80", 1), L"\u20AC");
  EXPECT_EQ(rtl::EncodeCodePage(1252, L"\u20AC", 1), "\x80");
  EXPECT_EQ(rtl::DecodeCodePage(28592, "\xB1", 1), L"\u0105");
  EXPECT_EQ(rtl::CodePageName(1250), "CP1250");
  EXPECT_EQ(rtl::CodePageName(28592), "ISO-8859-2");
  EXPECT_EQ(rtl::CodePageName(0), "UTF-8");
  // Undefined in CP1252, kept reversibly
  std::wstring Undefined = rtl::DecodeCodePage(1252, "\x81", 1);
  EXPECT_EQ(rtl::EncodeCodePage(1252, Undefined.data(), Undefined.size()), "\x81");
}
//---------------------------------------------------------------------------
TEST(Conversion, CaseMapping)
{
  EXPECT_EQ(rtl::UpperChar(L'a'), L'A');
  EXPECT_EQ(rtl::UpperChar(L'\u00E4'), L'\u00C4');
  EXPECT_EQ(rtl::UpperChar(L'\u03B1'), L'\u0391');
  EXPECT_EQ(rtl::LowerChar(L'\u0141'), L'\u0142');
  EXPECT_EQ(rtl::UpperChar(L'1'), L'1');
}
