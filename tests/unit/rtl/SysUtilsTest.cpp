// SysUtils: integer and string routines, file names, exceptions, resources
//---------------------------------------------------------------------------
#include <SysUtils.hpp>

#include <gtest/gtest.h>

#include <cerrno>
#include <cstring>
//---------------------------------------------------------------------------
TEST(SysUtils, IntToStr)
{
  EXPECT_EQ(IntToStr(0), L"0");
  EXPECT_EQ(IntToStr(-17), L"-17");
  EXPECT_EQ(IntToStr(static_cast<__int64>(-9223372036854775807LL - 1)), L"-9223372036854775808");
  EXPECT_EQ(IntToStr(static_cast<long>(5)), L"5");
  EXPECT_EQ(IntToStr(static_cast<size_t>(5)), L"5");
  EXPECT_EQ(UIntToStr(4294967295u), L"4294967295");
  EXPECT_EQ(IntToHex(255, 4), L"00FF");
  EXPECT_EQ(IntToHex(255, 1), L"FF");
  EXPECT_EQ(IntToHex(-1, 2), L"FFFFFFFF");
  EXPECT_EQ(IntToHex(static_cast<__int64>(-1), 2), L"FFFFFFFFFFFFFFFF");
  EXPECT_EQ(IntToHex(0u, 0), L"0");
}
//---------------------------------------------------------------------------
TEST(SysUtils, StrToInt)
{
  EXPECT_EQ(StrToInt(L"123"), 123);
  EXPECT_EQ(StrToInt(L"  12"), 12);
  EXPECT_EQ(StrToInt(L"+5"), 5);
  EXPECT_EQ(StrToInt(L"-5"), -5);
  EXPECT_EQ(StrToInt(L"$FF"), 255);
  EXPECT_EQ(StrToInt(L"0x1A"), 26);
  EXPECT_EQ(StrToInt(L"x10"), 16);
  EXPECT_EQ(StrToInt(L"-$10"), -16);
  EXPECT_EQ(StrToInt(L"$FFFFFFFF"), -1);
  EXPECT_EQ(StrToInt(L"2147483647"), 2147483647);
  EXPECT_EQ(StrToInt(L"-2147483648"), -2147483647 - 1);
  EXPECT_THROW(StrToInt(L"2147483648"), EConvertError);
  EXPECT_THROW(StrToInt(L"$100000000"), EConvertError);
  EXPECT_THROW(StrToInt(L"12 "), EConvertError);
  EXPECT_THROW(StrToInt(L""), EConvertError);
  EXPECT_THROW(StrToInt(L"-"), EConvertError);
  EXPECT_THROW(StrToInt(L"1.5"), EConvertError);
  EXPECT_THROW(StrToInt(L"$"), EConvertError);
  try
  {
    StrToInt(L"abc");
  }
  catch (EConvertError & E)
  {
    EXPECT_EQ(E.Message, L"'abc' is not a valid integer value");
  }
  EXPECT_EQ(StrToIntDef(L"x", -1), -1);
  int Value = 0;
  EXPECT_TRUE(TryStrToInt(L"7", Value));
  EXPECT_EQ(Value, 7);
  EXPECT_FALSE(TryStrToInt(L"7x", Value));

  EXPECT_EQ(StrToInt64(L"9223372036854775807"), 9223372036854775807LL);
  EXPECT_EQ(StrToInt64(L"-9223372036854775808"), -9223372036854775807LL - 1);
  EXPECT_THROW(StrToInt64(L"9223372036854775808"), EConvertError);
  EXPECT_EQ(StrToInt64(L"$FFFFFFFFFFFFFFFF"), -1);
  EXPECT_EQ(StrToInt64Def(L"", 3), 3);
  EXPECT_EQ(StrToUInt64(L"18446744073709551615"), 18446744073709551615ULL);
  EXPECT_THROW(StrToUInt64(L"-1"), EConvertError);
}
//---------------------------------------------------------------------------
TEST(SysUtils, Booleans)
{
  EXPECT_EQ(BoolToStr(true), L"-1");
  EXPECT_EQ(BoolToStr(false), L"0");
  EXPECT_EQ(BoolToStr(true, true), L"True");
  EXPECT_EQ(BoolToStr(false, true), L"False");
  EXPECT_TRUE(StrToBool(L"true"));
  EXPECT_FALSE(StrToBool(L"FALSE"));
  EXPECT_FALSE(StrToBool(L"0"));
  EXPECT_TRUE(StrToBool(L"5"));
  EXPECT_THROW(StrToBool(L"x"), EConvertError);
  EXPECT_TRUE(StrToBoolDef(L"x", true));
}
//---------------------------------------------------------------------------
TEST(SysUtils, CaseAndComparison)
{
  EXPECT_EQ(UpperCase(L"a\u00E4b"), L"A\u00E4B");
  EXPECT_EQ(LowerCase(L"A\u00C4B"), L"a\u00C4b");
  EXPECT_EQ(AnsiUpperCase(L"a\u00E4b"), L"A\u00C4B");
  EXPECT_EQ(AnsiLowerCase(L"A\u00C4B"), L"a\u00E4b");

  EXPECT_LT(CompareStr(L"a", L"b"), 0);
  EXPECT_GT(CompareStr(L"a", L"B"), 0);
  EXPECT_LT(CompareStr(L"ab", L"abc"), 0);
  EXPECT_EQ(CompareStr(L"x", L"x"), 0);
  EXPECT_LT(CompareText(L"a", L"B"), 0);
  EXPECT_EQ(CompareText(L"ABC", L"abc"), 0);
  EXPECT_TRUE(SameText(L"ABC", L"abc"));
  EXPECT_FALSE(SameText(L"\u00E4", L"\u00C4"));
  EXPECT_TRUE(AnsiSameText(L"\u00E4", L"\u00C4"));
  EXPECT_FALSE(SameStr(L"a", L"A"));
  EXPECT_LT(AnsiCompareStr(L"a", L"A"), 0);
  EXPECT_LT(AnsiCompareStr(L"a", L"B"), 0);
  EXPECT_LT(AnsiCompareStr(L"A", L"b"), 0);
  EXPECT_EQ(AnsiCompareStr(L"a", L"a"), 0);
  EXPECT_LT(AnsiCompareText(L"abc", L"ABD"), 0);
  EXPECT_EQ(AnsiCompareText(L"\u017C", L"\u017B"), 0);
  // File names are case-sensitive
  EXPECT_FALSE(SameFileName(L"a.txt", L"A.TXT"));
  EXPECT_NE(AnsiCompareFileName(L"a", L"A"), 0);
}
//---------------------------------------------------------------------------
TEST(SysUtils, Trim)
{
  EXPECT_EQ(Trim(L"  a  "), L"a");
  EXPECT_EQ(Trim(L"\x01\x1F x \x7F"), L"x \x7F");
  EXPECT_EQ(TrimLeft(L"  a  "), L"a  ");
  EXPECT_EQ(TrimRight(L"  a  "), L"  a");
  EXPECT_EQ(Trim(L"   "), L"");
}
//---------------------------------------------------------------------------
TEST(SysUtils, Quoting)
{
  EXPECT_EQ(QuotedStr(L"it's"), L"'it''s'");
  EXPECT_EQ(AnsiQuotedStr(L"a\"b", L'"'), L"\"a\"\"b\"");
  UnicodeString Source = L"'a''b' rest";
  const wchar_t * P = Source.c_str();
  EXPECT_EQ(AnsiExtractQuotedStr(P, L'\''), L"a'b");
  EXPECT_EQ(UnicodeString(P), L" rest");
  EXPECT_EQ(AnsiDequotedStr(L"'a''b'", L'\''), L"a'b");
  EXPECT_EQ(AnsiDequotedStr(L"abc", L'\''), L"abc");
  EXPECT_EQ(AnsiDequotedStr(L"'abc", L'\''), L"abc");
}
//---------------------------------------------------------------------------
TEST(SysUtils, StringReplace)
{
  EXPECT_EQ(StringReplace(L"aXbXc", L"X", L"-", TReplaceFlags()), L"a-bXc");
  EXPECT_EQ(StringReplace(L"aXbXc", L"X", L"-", TReplaceFlags() << rfReplaceAll), L"a-b-c");
  EXPECT_EQ(StringReplace(L"aXbxc", L"x", L"-", TReplaceFlags() << rfReplaceAll << rfIgnoreCase), L"a-b-c");
  EXPECT_EQ(StringReplace(L"aaa", L"a", L"aa", TReplaceFlags() << rfReplaceAll), L"aaaaaa");
  EXPECT_EQ(StringReplace(L"abc", L"", L"x", TReplaceFlags() << rfReplaceAll), L"abc");
}
//---------------------------------------------------------------------------
TEST(SysUtils, CStrings)
{
  wchar_t Buffer[10];
  StrLCopy(Buffer, L"abcdef", 3);
  EXPECT_EQ(UnicodeString(Buffer), L"abc");
  EXPECT_EQ(StrLen(L"abc"), 3u);
  EXPECT_EQ(StrLIComp(L"ABCx", L"abcy", 3), 0);
  EXPECT_NE(StrLIComp(L"ABCx", L"abcy", 4), 0);
  EXPECT_EQ(StrIComp(L"Abc", L"aBC"), 0);
  EXPECT_LT(StrComp(L"a", L"b"), 0);
  EXPECT_EQ(*StrScan(L"abc", L'b'), L'b');
  EXPECT_EQ(StrScan(L"abc", L'x'), nullptr);
  EXPECT_EQ(*StrEnd(L"abc"), L'\0');
}
//---------------------------------------------------------------------------
TEST(SysUtils, FileNames)
{
  EXPECT_EQ(IncludeTrailingBackslash(L"/a"), L"/a/");
  EXPECT_EQ(IncludeTrailingBackslash(L"/a/"), L"/a/");
  EXPECT_EQ(IncludeTrailingPathDelimiter(L""), L"/");
  EXPECT_EQ(ExcludeTrailingBackslash(L"/a/"), L"/a");
  EXPECT_EQ(ExcludeTrailingBackslash(L"/a//"), L"/a/");
  EXPECT_EQ(ExcludeTrailingPathDelimiter(L"/"), L"");

  EXPECT_EQ(ExtractFilePath(L"/a/b/c.txt"), L"/a/b/");
  EXPECT_EQ(ExtractFilePath(L"c.txt"), L"");
  EXPECT_EQ(ExtractFileDir(L"/a/b/c.txt"), L"/a/b");
  EXPECT_EQ(ExtractFileDir(L"/a"), L"/");
  EXPECT_EQ(ExtractFileDir(L"a"), L"");
  EXPECT_EQ(ExtractFileName(L"/a/b/c.txt"), L"c.txt");
  EXPECT_EQ(ExtractFileName(L"/a/b/"), L"");
  // A backslash is an ordinary character on Linux
  EXPECT_EQ(ExtractFileName(L"/x/a\\b"), L"a\\b");
  EXPECT_EQ(ExtractFileExt(L"/a/b/c.txt"), L".txt");
  EXPECT_EQ(ExtractFileExt(L"/a.b/c"), L"");
  EXPECT_EQ(ExtractFileExt(L"archive.tar.gz"), L".gz");
  EXPECT_EQ(ExtractFileExt(L".bashrc"), L".bashrc");
  EXPECT_EQ(ExtractFileDrive(L"/a/b"), L"");
  EXPECT_EQ(ChangeFileExt(L"/a/b.txt", L".bak"), L"/a/b.bak");
  EXPECT_EQ(ChangeFileExt(L"/a.d/b", L".x"), L"/a.d/b.x");
  EXPECT_EQ(ChangeFileExt(L"b.txt", L""), L"b");
  EXPECT_EQ(ChangeFilePath(L"/a/b.txt", L"/c"), L"/c/b.txt");
  EXPECT_TRUE(IsRelativePath(L"a/b"));
  EXPECT_FALSE(IsRelativePath(L"/a"));

  EXPECT_EQ(ExpandFileName(L"/a/./b/../c"), L"/a/c");
  EXPECT_EQ(ExpandFileName(L"/a//b/"), L"/a/b/");
  EXPECT_EQ(ExpandFileName(L"/.."), L"/");
  EXPECT_EQ(ExpandFileName(L"x"), IncludeTrailingPathDelimiter(GetCurrentDir()) + L"x");
  EXPECT_EQ(ExpandUNCFileName(L"/a/../b"), L"/b");

  EXPECT_EQ(ExtractRelativePath(L"/a/b/", L"/a/c/d.txt"), L"../c/d.txt");
  EXPECT_EQ(ExtractRelativePath(L"/a/b/", L"/a/b/c.txt"), L"c.txt");
  EXPECT_EQ(ExtractRelativePath(L"/a/b/x.txt", L"/a/b/e/f.txt"), L"e/f.txt");
}
//---------------------------------------------------------------------------
TEST(SysUtils, Exceptions)
{
  try
  {
    throw EAbort(L"stop");
  }
  catch (Exception & E)
  {
    EXPECT_EQ(E.Message, L"stop");
    EXPECT_EQ(E.ClassName(), L"EAbort");
    EXPECT_EQ(E.QualifiedClassName(), L"System.Sysutils.EAbort");
    EXPECT_NE(dynamic_cast<EAbort *>(&E), nullptr);
    EXPECT_TRUE(E.InheritsFrom(__classid(Exception)));
    EXPECT_TRUE(E.InheritsFrom(__classid(EAbort)));
    EXPECT_FALSE(E.InheritsFrom(__classid(EConvertError)));
  }

  Exception E(L"%s has %d items", ARRAYOFCONST((L"list", 3)));
  EXPECT_EQ(E.Message, L"list has 3 items");
  E.Message = L"changed";
  EXPECT_EQ(E.Message, L"changed");
  EXPECT_EQ(E.ToString(), L"changed");
  Exception WithHelp(L"x", 42);
  EXPECT_EQ(WithHelp.HelpContext, 42);

  EXPECT_THROW(Abort(), EAbort);
  try
  {
    Abort();
  }
  catch (EAbort & E)
  {
    EXPECT_EQ(E.Message, L"Operation aborted");
  }

  // Derived exceptions inherit all constructors
  ERangeError Range(L"%d", ARRAYOFCONST((1)), 7);
  EXPECT_EQ(Range.Message, L"1");
  EXPECT_EQ(Range.HelpContext, 7);
  EXPECT_TRUE(Range.InheritsFrom(__classid(EIntError)));
  EXPECT_TRUE(Range.InheritsFrom(__classid(EExternal)));
}
//---------------------------------------------------------------------------
TEST(SysUtils, OSErrors)
{
  try
  {
    RaiseLastOSError(ERROR_ACCESS_DENIED);
  }
  catch (EOSError & E)
  {
    EXPECT_EQ(E.ErrorCode, static_cast<unsigned int>(ERROR_ACCESS_DENIED));
    EXPECT_EQ(E.Message, L"System Error.  Code: 5.\n" + UnicodeString(strerror(EACCES)));
  }
  SetLastError(ERROR_FILE_NOT_FOUND);
  EXPECT_THROW(RaiseLastOSError(), EOSError);
  EXPECT_EQ(SysErrorMessage(ERROR_FILE_NOT_FOUND), UnicodeString(strerror(ENOENT)));
  EXPECT_EQ(SysErrorMessage(ERROR_NO_MORE_FILES), L"There are no more files");

  EXPECT_EQ(rtl::ErrnoToWinError(ENOENT), static_cast<DWORD>(ERROR_FILE_NOT_FOUND));
  EXPECT_EQ(rtl::ErrnoToWinError(EEXIST), static_cast<DWORD>(ERROR_ALREADY_EXISTS));
  EXPECT_EQ(rtl::WinErrorToErrno(ERROR_FILE_EXISTS), EEXIST);
  // errno values without a Windows equivalent survive a round trip
  DWORD Encoded = rtl::ErrnoToWinError(EPROTO);
  EXPECT_NE(Encoded & rtl::ErrnoFlag, 0u);
  EXPECT_EQ(rtl::WinErrorToErrno(Encoded), EPROTO);
  EXPECT_EQ(SysErrorMessage(Encoded), UnicodeString(strerror(EPROTO)));
  // WinSock errors
  EXPECT_EQ(rtl::ErrnoToWinError(ECONNREFUSED), static_cast<DWORD>(WSAECONNREFUSED));
  EXPECT_EQ(rtl::ErrnoToWinError(EINVAL), static_cast<DWORD>(ERROR_INVALID_PARAMETER));
  EXPECT_EQ(rtl::ErrnoToWinSockError(EINVAL), static_cast<DWORD>(WSAEINVAL));
  EXPECT_EQ(rtl::ErrnoToWinSockError(EWOULDBLOCK), static_cast<DWORD>(WSAEWOULDBLOCK));
  EXPECT_EQ(rtl::WinErrorToErrno(WSAECONNRESET), ECONNRESET);
  EXPECT_EQ(SysErrorMessage(WSAETIMEDOUT), UnicodeString(strerror(ETIMEDOUT)));
  EXPECT_EQ(SysErrorMessage(WSAHOST_NOT_FOUND), L"No such host is known");
}
//---------------------------------------------------------------------------
TEST(SysUtils, ResourceStrings)
{
  static const rtl::TResourceString Strings[] =
  {
    { 60001, L"Hello" },
    { 60002, L"File %s not found (%d)" },
  };
  rtl::RegisterResourceStrings(Strings, std::size(Strings));
  EXPECT_EQ(LoadStr(60001), L"Hello");
  EXPECT_EQ(LoadStr(60999), L"");
  EXPECT_EQ(FmtLoadStr(60002, ARRAYOFCONST((L"a.txt", 2))), L"File a.txt not found (2)");
  EXPECT_EQ(UnicodeString::LoadStr(60001), L"Hello");
  Exception E(static_cast<NativeUInt>(60002), ARRAYOFCONST((L"b", 3)));
  EXPECT_EQ(E.Message, L"File b not found (3)");
  TResStringRec Rec = { nullptr, 60001 };
  EXPECT_EQ(LoadResString(&Rec), L"Hello");
}
//---------------------------------------------------------------------------
TEST(SysUtils, ResourceStringTranslations)
{
  static const rtl::TResourceString Strings[] = { { 60101, L"Open" } };
  static const rtl::TResourceStringName Names[] = { { 60101, "TEST_OPEN" } };
  rtl::RegisterResourceStrings(Strings, std::size(Strings));
  rtl::RegisterResourceStringNames(Names, std::size(Names));
  EXPECT_EQ(rtl::FindResourceStringIdent("TEST_OPEN"), 60101);
  EXPECT_EQ(rtl::FindResourceStringIdent("TEST_MISSING"), -1);
  // A translation replaces the text, kept by the RTL
  {
    UnicodeString Text = L"Otw\u00f3rz";
    rtl::SetResourceString(60101, Text);
  }
  EXPECT_EQ(LoadStr(60101), L"Otw\u00f3rz");
  rtl::SetResourceString(60101, L"Ouvrir");
  EXPECT_EQ(LoadStr(60101), L"Ouvrir");
  // The registered texts again
  rtl::ResetResourceStrings();
  EXPECT_EQ(LoadStr(60101), L"Open");
}
//---------------------------------------------------------------------------
TEST(SysUtils, EmptyStr)
{
  EXPECT_TRUE(EmptyStr.IsEmpty());
}
