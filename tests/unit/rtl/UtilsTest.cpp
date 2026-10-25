// System.StrUtils, System.Math, System.DateUtils, System.TimeSpan
//---------------------------------------------------------------------------
#include <System.StrUtils.hpp>
#include <System.Math.hpp>
#include <System.DateUtils.hpp>
#include <System.Win.Registry.hpp>
#include <Vcl.Menus.hpp>

#include <gtest/gtest.h>

#include <cmath>
//---------------------------------------------------------------------------
TEST(StrUtils, ContainsStartsEnds)
{
  EXPECT_TRUE(ContainsStr(L"Hello", L"ell"));
  EXPECT_FALSE(ContainsStr(L"Hello", L"ELL"));
  EXPECT_FALSE(ContainsStr(L"Hello", L""));
  EXPECT_TRUE(ContainsText(L"Hello", L"ELL"));
  EXPECT_TRUE(ContainsText(L"Za\u017C\u00F3\u0142\u0107", L"\u017B\u00D3"));
  EXPECT_TRUE(StartsStr(L"He", L"Hello"));
  EXPECT_FALSE(StartsStr(L"he", L"Hello"));
  EXPECT_TRUE(StartsStr(L"", L"Hello"));
  EXPECT_FALSE(StartsStr(L"Hello!", L"Hello"));
  EXPECT_TRUE(StartsText(L"he", L"Hello"));
  EXPECT_TRUE(EndsStr(L"lo", L"Hello"));
  EXPECT_FALSE(EndsStr(L"LO", L"Hello"));
  EXPECT_TRUE(EndsText(L"LO", L"Hello"));
  EXPECT_FALSE(EndsText(L"xHello", L"Hello"));
}
//---------------------------------------------------------------------------
TEST(StrUtils, Substrings)
{
  EXPECT_EQ(LeftStr(L"Hello", 2), L"He");
  EXPECT_EQ(LeftStr(L"Hello", 10), L"Hello");
  EXPECT_EQ(RightStr(L"Hello", 2), L"lo");
  EXPECT_EQ(RightStr(L"Hello", 10), L"Hello");
  EXPECT_EQ(RightStr(L"Hello", 0), L"");
  EXPECT_EQ(MidStr(L"Hello", 2, 3), L"ell");
  EXPECT_EQ(PosEx(L"l", L"Hello", 1), 3);
  EXPECT_EQ(PosEx(L"l", L"Hello", 4), 4);
  EXPECT_EQ(PosEx(L"l", L"Hello", 5), 0);
  EXPECT_EQ(PosEx(L"l", L"Hello", 0), 0);
  EXPECT_EQ(PosEx(L"l", L"Hello", 6), 0);
  EXPECT_EQ(ReplaceStr(L"a.b.c", L".", L"::"), L"a::b::c");
  EXPECT_EQ(ReplaceText(L"aXbx", L"x", L"-"), L"a-b-");
  EXPECT_EQ(DupeString(L"ab", 3), L"ababab");
  EXPECT_EQ(ReverseString(L"abc"), L"cba");
  EXPECT_EQ(StuffString(L"Hello", 2, 3, L"EY"), L"HEYo");
  EXPECT_EQ(IfThen(true, L"y", L"n"), L"y");
  EXPECT_EQ(IfThen(false, L"y"), L"");
  TStringDynArray Parts = SplitString(L"a,b;;c", L",;");
  ASSERT_EQ(Parts.Length, 4);
  EXPECT_EQ(Parts[0], L"a");
  EXPECT_EQ(Parts[2], L"");
  EXPECT_EQ(Parts[3], L"c");
}
//---------------------------------------------------------------------------
TEST(StrUtils, OpenArrays)
{
  UnicodeString Values[] = { L"one", L"Two", L"three" };
  EXPECT_EQ(IndexStr(L"Two", Values, 2), 1);
  EXPECT_EQ(IndexStr(L"two", Values, 2), -1);
  EXPECT_EQ(IndexText(L"two", Values, 2), 1);
  EXPECT_TRUE(MatchStr(L"three", Values, 2));
  EXPECT_FALSE(MatchText(L"four", Values, 2));
  EXPECT_EQ(IndexStr(L"x", EXISTINGARRAY(Values)), -1);
  EXPECT_EQ(IndexStr(L"b", OPENARRAY(UnicodeString, (L"a", L"b"))), 1);
}
//---------------------------------------------------------------------------
TEST(Math, MinMaxAndRanges)
{
  EXPECT_EQ(Min(3, 5), 3);
  EXPECT_EQ(Max(3, 5), 5);
  EXPECT_EQ(Min(3, static_cast<__int64>(5)), 3);
  EXPECT_EQ(Max(static_cast<long>(7), static_cast<long long>(2)), 7);
  EXPECT_EQ(Min(static_cast<size_t>(3), static_cast<unsigned int>(9)), 3u);
  EXPECT_DOUBLE_EQ(Max(1.5, 2), 2.0);
  EXPECT_TRUE(InRange(5, 1, 10));
  EXPECT_FALSE(InRange(11, 1, 10));
  EXPECT_EQ(EnsureRange(11, 1, 10), 10);
  EXPECT_EQ(EnsureRange(-1, 1, 10), 1);
}
//---------------------------------------------------------------------------
TEST(Math, Rounding)
{
  EXPECT_EQ(Floor(2.7), 2);
  EXPECT_EQ(Floor(-2.2), -3);
  EXPECT_EQ(Ceil(2.2), 3);
  EXPECT_EQ(Ceil(-2.7), -2);
  EXPECT_DOUBLE_EQ(RoundTo(1234567, 3), 1235000);
  EXPECT_DOUBLE_EQ(RoundTo(1.234, -2), 1.23);
  EXPECT_DOUBLE_EQ(RoundTo(2.5, 0), 2);
  EXPECT_DOUBLE_EQ(SimpleRoundTo(2.5, 0), 3);
  EXPECT_DOUBLE_EQ(SimpleRoundTo(-2.5, 0), -3);
  EXPECT_DOUBLE_EQ(SimpleRoundTo(1.2345), 1.23);
  EXPECT_DOUBLE_EQ(static_cast<double>(IntPower(2, 10)), 1024);
  EXPECT_DOUBLE_EQ(static_cast<double>(IntPower(2, -2)), 0.25);
  EXPECT_DOUBLE_EQ(static_cast<double>(Power(9, 0.5)), 3);
  EXPECT_EQ(Sign(-3.0), -1);
  EXPECT_TRUE(SameValue(0.1 + 0.2, 0.3));
  EXPECT_FALSE(SameValue(1.0, 1.0001));
  EXPECT_TRUE(IsZero(1e-30));
  EXPECT_TRUE(IsNan(NaN));
  EXPECT_TRUE(IsInfinite(Infinity));
  EXPECT_EQ(CompareValue(1, 2), LessThanValue);
  EXPECT_EQ(MaxIntValue(OPENARRAY(int, (3, 9, 2))), 9);
  EXPECT_EQ(IfThen(true, 1, 2), 1);
}
//---------------------------------------------------------------------------
TEST(DateUtils, Parts)
{
  TDateTime Value = EncodeDateTime(2026, 10, 24, 13, 5, 7, 89);
  EXPECT_EQ(YearOf(Value), 2026);
  EXPECT_EQ(MonthOf(Value), 10);
  EXPECT_EQ(DayOf(Value), 24);
  EXPECT_EQ(HourOf(Value), 13);
  EXPECT_EQ(MinuteOf(Value), 5);
  EXPECT_EQ(SecondOf(Value), 7);
  EXPECT_EQ(MilliSecondOf(Value), 89);
  // ISO day of the week: Saturday
  EXPECT_EQ(DayOfTheWeek(Value), DaySaturday);
  EXPECT_EQ(DayOfTheWeek(EncodeDate(2026, 10, 26)), DayMonday);
  EXPECT_EQ(DayOfTheYear(EncodeDate(2026, 2, 1)), 32);
  EXPECT_EQ(MilliSecondOfTheSecond(Value), 89);
  EXPECT_EQ(MilliSecondOfTheMinute(Value), 7089u);
  EXPECT_EQ(MilliSecondOfTheHour(Value), (5 * 60 + 7) * 1000u + 89);
  EXPECT_EQ(MilliSecondOfTheDay(Value), ((13 * 60 + 5) * 60 + 7) * 1000u + 89);
  EXPECT_EQ(MilliSecondOfTheYear(EncodeDateTime(2026, 1, 2, 0, 0, 1, 0)), 86401000);
  EXPECT_EQ(DateOf(Value).Val, EncodeDate(2026, 10, 24).Val);
  EXPECT_NEAR(TimeOf(Value).Val, EncodeTime(13, 5, 7, 89).Val, 1e-12);
  EXPECT_EQ(DaysInAMonth(2024, 2), 29);
  EXPECT_EQ(DaysInAYear(2023), 365);
  EXPECT_TRUE(IsSameDay(Value, EncodeDate(2026, 10, 24)));
  EXPECT_FALSE(IsSameDay(Value, EncodeDate(2026, 10, 25)));
  EXPECT_TRUE(IsPM(Value));
  EXPECT_TRUE(IsValidDate(2024, 2, 29));
  EXPECT_FALSE(IsValidDate(2023, 2, 29));
  EXPECT_THROW(EncodeDateTime(2023, 2, 29, 0, 0, 0, 0), EConvertError);
  EXPECT_EQ(MinutesBetween(StartOfTheDay(Value), EndOfTheDay(Value)), 1439);
}
//---------------------------------------------------------------------------
TEST(DateUtils, BetweenAndInc)
{
  TDateTime A = EncodeDateTime(2026, 10, 24, 13, 0, 0, 0);
  TDateTime B = EncodeDateTime(2026, 10, 24, 14, 30, 15, 500);
  EXPECT_EQ(MilliSecondsBetween(A, B), 5415500);
  EXPECT_EQ(MilliSecondsBetween(B, A), 5415500);
  EXPECT_EQ(SecondsBetween(A, B), 5415);
  EXPECT_EQ(MinutesBetween(A, B), 90);
  EXPECT_EQ(HoursBetween(A, B), 1);
  EXPECT_EQ(DaysBetween(A, IncDay(A, 3)), 3);
  EXPECT_EQ(DaysBetween(A, IncSecond(IncDay(A, 3), -1)), 2);
  EXPECT_EQ(YearsBetween(EncodeDate(2020, 1, 1), EncodeDate(2026, 6, 1)), 6);
  EXPECT_EQ(MonthsBetween(EncodeDate(2026, 1, 1), EncodeDate(2026, 3, 15)), 2);
  EXPECT_NEAR(IncMilliSecond(A, 1500).Val, EncodeDateTime(2026, 10, 24, 13, 0, 1, 500).Val, 1e-9);
  EXPECT_NEAR(IncMinute(A, -61).Val, EncodeDateTime(2026, 10, 24, 11, 59, 0, 0).Val, 1e-9);
  EXPECT_NEAR(IncHour(A, 11).Val, EncodeDateTime(2026, 10, 25, 0, 0, 0, 0).Val, 1e-9);
  EXPECT_EQ(IncYear(EncodeDate(2024, 2, 29)).Val, EncodeDate(2025, 2, 28).Val);
  EXPECT_EQ(IncWeek(EncodeDate(2026, 10, 24)).Val, EncodeDate(2026, 10, 31).Val);
  // Crossing the TDateTime origin (negative values) works
  TDateTime BeforeOrigin = IncHour(EncodeDate(1899, 12, 30), -18);
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DecodeDateTime(BeforeOrigin, Year, Month, Day, Hour, Min, Sec, MSec);
  EXPECT_EQ(Day, 29);
  EXPECT_EQ(Hour, 6);
  EXPECT_EQ(HoursBetween(BeforeOrigin, EncodeDate(1899, 12, 30)), 18);
  EXPECT_EQ(CompareDateTime(A, B), LessThanValue);
  EXPECT_TRUE(SameDateTime(A, TDateTime(A.Val + OneMillisecond / 2)));
}
//---------------------------------------------------------------------------
TEST(DateUtils, Unix)
{
  EXPECT_EQ(DateTimeToUnix(EncodeDate(1970, 1, 1)), 0);
  EXPECT_EQ(DateTimeToUnix(EncodeDateTime(2026, 10, 24, 13, 5, 7, 0)), 1792847107);
  EXPECT_EQ(DateTimeToUnix(EncodeDateTime(1969, 12, 31, 23, 59, 59, 0)), -1);
  EXPECT_NEAR(UnixToDateTime(1792847107).Val, EncodeDateTime(2026, 10, 24, 13, 5, 7, 0).Val, 1e-9);
  EXPECT_NEAR(UnixToDateTime(-86400).Val, EncodeDate(1969, 12, 31).Val, 1e-9);
}
//---------------------------------------------------------------------------
TEST(DateUtils, ISO8601)
{
  TDateTime Expected = EncodeDateTime(2026, 10, 24, 13, 5, 7, 0);
  EXPECT_NEAR(ISO8601ToDate(L"2026-10-24T13:05:07Z").Val, Expected.Val, 1e-9);
  EXPECT_NEAR(ISO8601ToDate(L"2026-10-24T15:05:07+02:00").Val, Expected.Val, 1e-9);
  EXPECT_NEAR(ISO8601ToDate(L"2026-10-24T12:05:07-0100").Val, Expected.Val, 1e-9);
  EXPECT_NEAR(ISO8601ToDate(L"20261024T130507Z").Val, Expected.Val, 1e-9);
  EXPECT_NEAR(ISO8601ToDate(L"2026-10-24T13:05:07.250Z").Val, IncMilliSecond(Expected, 250).Val, 1e-9);
  EXPECT_EQ(ISO8601ToDate(L"2026-10-24").Val, EncodeDate(2026, 10, 24).Val);
  EXPECT_THROW(ISO8601ToDate(L"2026-13-01"), EConvertError);
  EXPECT_THROW(ISO8601ToDate(L"garbage"), EConvertError);
  EXPECT_EQ(DateToISO8601(IncMilliSecond(Expected, 5)), L"2026-10-24T13:05:07.005Z");
}
//---------------------------------------------------------------------------
TEST(DateUtils, TimeZone)
{
  setenv("TZ", "Europe/Warsaw", 1);
  tzset();
  TDateTime Summer = EncodeDateTime(2026, 7, 1, 12, 0, 0, 0);
  TDateTime Winter = EncodeDateTime(2026, 1, 1, 12, 0, 0, 0);
  EXPECT_EQ(TTimeZone::Local->GetUtcOffset(Summer).TotalHours, 2);
  EXPECT_EQ(TTimeZone::Local->GetUtcOffset(Winter).TotalHours, 1);
  EXPECT_TRUE(TTimeZone::Local->IsDaylightTime(Summer));
  EXPECT_FALSE(TTimeZone::Local->IsDaylightTime(Winter));
  EXPECT_NEAR(TTimeZone::Local->ToUniversalTime(Summer).Val, EncodeDateTime(2026, 7, 1, 10, 0, 0, 0).Val, 1e-9);
  EXPECT_NEAR(TTimeZone::Local->ToLocalTime(EncodeDateTime(2026, 1, 1, 11, 0, 0, 0)).Val, Winter.Val, 1e-9);
  EXPECT_EQ(TTimeZone::Local->GetAbbreviation(Summer), L"CEST");
  EXPECT_EQ(DateTimeToUnix(Summer, false), DateTimeToUnix(EncodeDateTime(2026, 7, 1, 10, 0, 0, 0)));
  EXPECT_NEAR(UnixToDateTime(DateTimeToUnix(Winter, false), false).Val, Winter.Val, 1e-9);
  EXPECT_NEAR(ISO8601ToDate(L"2026-07-01T10:00:00Z", false).Val, Summer.Val, 1e-9);
  EXPECT_EQ(DateToISO8601(Summer, false), L"2026-07-01T12:00:00.000+02:00");
  EXPECT_EQ(TTimeZone::Local->ID, L"Europe/Warsaw");
  setenv("TZ", "UTC", 1);
  tzset();
}
//---------------------------------------------------------------------------
TEST(TimeSpan, Basics)
{
  TTimeSpan Span = TTimeSpan::FromSeconds(-5400);
  EXPECT_EQ(Span.Hours, -1);
  EXPECT_EQ(Span.Minutes, -30);
  EXPECT_EQ(Span.Seconds, 0);
  EXPECT_DOUBLE_EQ(Span.TotalHours, -1.5);
  EXPECT_TRUE(Span <= TTimeSpan::Zero);
  EXPECT_FALSE(TTimeSpan::FromSeconds(1) <= TTimeSpan::Zero);
  EXPECT_EQ(TTimeSpan::FromSeconds(3661).ToString(), L"01:01:01");
  EXPECT_EQ(TTimeSpan(1, 2, 3, 4).ToString(), L"1.02:03:04");
  EXPECT_EQ(TTimeSpan::FromMilliseconds(1.5).Ticks, 20000);
  EXPECT_EQ(TTimeSpan::FromDays(1).Days, 1);
  TTimeSpan Difference = TTimeSpan::Subtract(EncodeDateTime(2026, 1, 1, 1, 0, 0, 0), EncodeDate(2026, 1, 1));
  EXPECT_DOUBLE_EQ(Difference.TotalMinutes, 60);
  EXPECT_EQ((Difference + Difference).Hours, 2);
}
//---------------------------------------------------------------------------
TEST(UtilsTest, StripHotkey)
{
  EXPECT_EQ(StripHotkey(L"&Yes"), L"Yes");
  EXPECT_EQ(StripHotkey(L"N&o to All"), L"No to All");
  EXPECT_EQ(StripHotkey(L"Save && &Exit"), L"Save & Exit");
  EXPECT_EQ(StripHotkey(L"Plain"), L"Plain");
}
//---------------------------------------------------------------------------
TEST(UtilsTest, EmptyRegistry)
{
  TRegistry Registry(KEY_READ);
  Registry.RootKey = HKEY_LOCAL_MACHINE;
  EXPECT_EQ(Registry.RootKey, HKEY_LOCAL_MACHINE);
  EXPECT_FALSE(Registry.OpenKey(L"SOFTWARE", false));
  EXPECT_FALSE(Registry.OpenKey(L"LinSCP", true));
  EXPECT_FALSE(Registry.KeyExists(L"SOFTWARE"));
  EXPECT_FALSE(Registry.ValueExists(L"Value"));
  EXPECT_THROW(Registry.ReadString(L"Value"), ERegistryException);
  EXPECT_THROW(Registry.WriteInteger(L"Value", 1), ERegistryException);
  std::unique_ptr<TStringList> Names(new TStringList());
  Names->Add(L"x");
  Registry.GetKeyNames(Names.get());
  EXPECT_EQ(Names->Count, 0);
  EXPECT_EQ(Registry.CurrentKey, nullptr);
}
//---------------------------------------------------------------------------
TEST(UtilsTest, BorlandCompatibility)
{
  EXPECT_TRUE(True);
  EXPECT_FALSE(False);
  for (int Index = 0; Index < 100; Index++)
  {
    int Value = random(7);
    EXPECT_GE(Value, 0);
    EXPECT_LT(Value, 7);
  }
  EXPECT_EQ(Format(System_Sysconst_SOSError, ARRAYOFCONST((5, L"Access denied.", L""))), L"System Error.  Code: 5.\nAccess denied.");
}
