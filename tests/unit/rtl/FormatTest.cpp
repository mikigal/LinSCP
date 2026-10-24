// Format(), FloatToStr(), FormatFloat() and StrToFloat() with Delphi semantics
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Variants.hpp>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
class FormatTest : public testing::Test
{
protected:
  void SetUp() override
  {
    FSaved = FormatSettings;
    FormatSettings = TFormatSettings::Invariant();
  }

  void TearDown() override
  {
    FormatSettings = FSaved;
  }

  TFormatSettings FSaved;
};
} // namespace
//---------------------------------------------------------------------------
#define F(S, ARGS) Format(S, ARRAYOFCONST(ARGS))
//---------------------------------------------------------------------------
TEST_F(FormatTest, Integers)
{
  EXPECT_EQ(F(L"%d", (5)), L"5");
  EXPECT_EQ(F(L"%5d|", (42)), L"   42|");
  EXPECT_EQ(F(L"%-5d|", (42)), L"42   |");
  EXPECT_EQ(F(L"%.3d", (5)), L"005");
  EXPECT_EQ(F(L"%.3d", (-5)), L"-005");
  EXPECT_EQ(F(L"%2.2d:%2.2d", (5, 7)), L"05:07");
  EXPECT_EQ(F(L"%d", (-2147483647 - 1)), L"-2147483648");
  EXPECT_EQ(F(L"%d", (static_cast<__int64>(1) << 40)), L"1099511627776");
  EXPECT_EQ(F(L"%d", (18446744073709551615ULL)), L"18446744073709551615");
  EXPECT_EQ(F(L"%u", (-1)), L"4294967295");
  EXPECT_EQ(F(L"%u", (5u)), L"5");
  EXPECT_EQ(F(L"%x", (255)), L"FF");
  EXPECT_EQ(F(L"%x", (-1)), L"FFFFFFFF");
  EXPECT_EQ(F(L"%x", (static_cast<__int64>(-1))), L"FFFFFFFFFFFFFFFF");
  EXPECT_EQ(F(L"%2.2X", (10)), L"0A");
  EXPECT_EQ(F(L"%.4x", (255)), L"00FF");
  EXPECT_EQ(F(L"%d", (true ? 1 : 0)), L"1");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, Strings)
{
  UnicodeString U = L"uni";
  AnsiString A = "ansi";
  UTF8String Utf8 = UnicodeString(L"\u017C");
  EXPECT_EQ(F(L"%s %s %s %s %s %s", (U, L"wide", "narrow", A, L'w', 'c')), L"uni wide narrow ansi w c");
  EXPECT_EQ(F(L"%s", (Utf8)), L"\u017C");
  EXPECT_EQ(F(L"[%5s][%-5s][%.2s][%3.1s]", (L"ab", L"ab", L"abcdef", L"xyz")), L"[   ab][ab   ][ab][  x]");
  EXPECT_EQ(F(L"%s", (UnicodeString())), L"");
  EXPECT_EQ(F(L"100%%", ()), L"100%");
  EXPECT_EQ(F(L"no args", ()), L"no args");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, IndexesAndStars)
{
  EXPECT_EQ(F(L"%1:s %0:s", (L"a", L"b")), L"b a");
  EXPECT_EQ(F(L"%0:s %s", (L"a", L"b")), L"a b");
  EXPECT_EQ(F(L"%1:s %s %0:s", (L"a", L"b", L"c")), L"b c a");
  EXPECT_EQ(F(L"%*d", (5, 42)), L"   42");
  EXPECT_EQ(F(L"%-*s|", (4, L"ab")), L"ab  |");
  EXPECT_EQ(F(L"%.*f", (1, 2.25)), L"2.3");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, Floats)
{
  EXPECT_EQ(F(L"%f", (1.5)), L"1.50");
  EXPECT_EQ(F(L"%.0f", (2.5)), L"3");
  EXPECT_EQ(F(L"%.0f", (-2.5)), L"-3");
  EXPECT_EQ(F(L"%.1f", (0.05)), L"0.1");
  EXPECT_EQ(F(L"%.2f", (1.005)), L"1.00");
  EXPECT_EQ(F(L"%.2f", (-0.001)), L"0.00");
  EXPECT_EQ(F(L"%6.1f", (3.14159)), L"   3.1");
  EXPECT_EQ(F(L"%.3f", (1234.0)), L"1234.000");
  EXPECT_EQ(F(L"%e", (1234.5)), L"1.23450000000000E+003");
  EXPECT_EQ(F(L"%.3e", (1234.5)), L"1.23E+003");
  EXPECT_EQ(F(L"%.3e", (-0.00012345)), L"-1.23E-004");
  EXPECT_EQ(F(L"%g", (123.456)), L"123.456");
  EXPECT_EQ(F(L"%g", (0.0001)), L"0.0001");
  EXPECT_EQ(F(L"%n", (1234567.891)), L"1,234,567.89");
  EXPECT_EQ(F(L"%14.0n", (1234567.0)), L"     1,234,567");
  EXPECT_EQ(F(L"%.1n", (-999.95)), L"-1,000.0");
  EXPECT_EQ(F(L"%m", (1.5)), L"\u00A41.50");
  EXPECT_EQ(F(L"%f", (static_cast<float>(0.5))), L"0.50");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, PointersAndVariants)
{
  void * P = reinterpret_cast<void *>(0xABCDEF);
  EXPECT_EQ(F(L"%p", (P)), L"0000000000ABCDEF");
  Variant V = 42;
  EXPECT_EQ(F(L"%d %s", (V, V)), L"42 42");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, Errors)
{
  EXPECT_THROW(F(L"%d", (L"x")), EConvertError);
  EXPECT_THROW(F(L"%d", ()), EConvertError);
  EXPECT_THROW(F(L"%s %s", (L"a")), EConvertError);
  EXPECT_THROW(F(L"%z", (1)), EConvertError);
  EXPECT_THROW(F(L"%f", (1)), EConvertError);
  EXPECT_THROW(F(L"%", ()), EConvertError);
  try
  {
    F(L"%d", (L"x"));
  }
  catch (EConvertError & E)
  {
    EXPECT_EQ(E.Message, L"Format '%d' invalid or incompatible with argument");
  }
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, ArgumentsAreEvaluatedOnce)
{
  int Counter = 0;
  UnicodeString Result = F(L"%d", (++Counter));
  EXPECT_EQ(Result, L"1");
  EXPECT_EQ(Counter, 1);
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, Settings)
{
  TFormatSettings Settings = TFormatSettings::Invariant();
  Settings.DecimalSeparator = L',';
  Settings.ThousandSeparator = L'.';
  EXPECT_EQ(Format(L"%.2f %n", ARRAYOFCONST((1.5, 1234.5))), L"1.50 1,234.50");
  EXPECT_EQ(Format(L"%.2f %n", ARRAYOFCONST((1.5, 1234.5)), Settings), L"1,50 1.234,50");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, FloatToStr)
{
  EXPECT_EQ(FloatToStr(0), L"0");
  EXPECT_EQ(FloatToStr(100), L"100");
  EXPECT_EQ(FloatToStr(0.1), L"0.1");
  EXPECT_EQ(FloatToStr(-2.5), L"-2.5");
  EXPECT_EQ(FloatToStr(123.456), L"123.456");
  EXPECT_EQ(FloatToStr(1.0 / 3), L"0.333333333333333");
  EXPECT_EQ(FloatToStr(2.0 / 3), L"0.666666666666667");
  EXPECT_EQ(FloatToStr(0.0001), L"0.0001");
  EXPECT_EQ(FloatToStr(0.00001), L"1E-5");
  EXPECT_EQ(FloatToStr(1.5e-10), L"1.5E-10");
  EXPECT_EQ(FloatToStr(123456789012345.0), L"123456789012345");
  EXPECT_EQ(FloatToStr(1e15), L"1E15");
  EXPECT_EQ(FloatToStr(1e20), L"1E20");
  EXPECT_EQ(FloatToStr(0.1 + 0.2), L"0.3");
  EXPECT_EQ(FloatToStrF(1234.5678, ffFixed, 18, 2), L"1234.57");
  EXPECT_EQ(FloatToStrF(1234.5678, ffNumber, 18, 1), L"1,234.6");
  EXPECT_EQ(FloatToStrF(1234.5678, ffExponent, 4, 2), L"1.235E+03");
  EXPECT_EQ(FloatToStrF(1234.5678, ffGeneral, 6, 0), L"1234.57");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, FormatFloat)
{
  EXPECT_EQ(FormatFloat(L"#,##0", 1234567), L"1,234,567");
  EXPECT_EQ(FormatFloat(L"#,##0", 12), L"12");
  EXPECT_EQ(FormatFloat(L"#,##0", 0), L"0");
  EXPECT_EQ(FormatFloat(L"0.00", 2.5), L"2.50");
  EXPECT_EQ(FormatFloat(L"0.00", 1.005), L"1.00");
  EXPECT_EQ(FormatFloat(L"0.###", 1.5), L"1.5");
  EXPECT_EQ(FormatFloat(L"#.##", 0.5), L".5");
  EXPECT_EQ(FormatFloat(L"000", 7), L"007");
  EXPECT_EQ(FormatFloat(L"0.00;(0.00)", -1.5), L"(1.50)");
  EXPECT_EQ(FormatFloat(L"0.00;(0.00);zero", 0), L"zero");
  EXPECT_EQ(FormatFloat(L"0.0E+00", 12345), L"1.2E+04");
  EXPECT_EQ(FormatFloat(L"'Total: '0", 5), L"Total: 5");
  EXPECT_EQ(FormatFloat(L"0 kB", 12), L"12 kB");
  EXPECT_EQ(FormatFloat(L"", 1.5), L"1.5");
}
//---------------------------------------------------------------------------
TEST_F(FormatTest, StrToFloat)
{
  EXPECT_DOUBLE_EQ(static_cast<double>(StrToFloat(L"1.5")), 1.5);
  EXPECT_DOUBLE_EQ(static_cast<double>(StrToFloat(L" -1.5 ")), -1.5);
  EXPECT_DOUBLE_EQ(static_cast<double>(StrToFloat(L"1e3")), 1000.0);
  EXPECT_DOUBLE_EQ(static_cast<double>(StrToFloat(L".5")), 0.5);
  EXPECT_THROW(StrToFloat(L"1,5"), EConvertError);
  EXPECT_THROW(StrToFloat(L"abc"), EConvertError);
  EXPECT_THROW(StrToFloat(L"1e"), EConvertError);
  double D;
  EXPECT_FALSE(TryStrToFloat(L"", D));
  EXPECT_TRUE(TryStrToFloat(L"2", D));
  EXPECT_EQ(D, 2.0);
  EXPECT_DOUBLE_EQ(static_cast<double>(StrToFloatDef(L"x", 3.5)), 3.5);
  TFormatSettings Settings = TFormatSettings::Invariant();
  Settings.DecimalSeparator = L',';
  EXPECT_DOUBLE_EQ(static_cast<double>(StrToFloat(L"1,5", Settings)), 1.5);
}
