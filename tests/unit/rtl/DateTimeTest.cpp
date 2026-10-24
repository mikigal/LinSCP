// TDateTime and the date/time routines of SysUtils
//---------------------------------------------------------------------------
#include <SysUtils.hpp>

#include <gtest/gtest.h>

#include <cmath>
#include <ctime>
//---------------------------------------------------------------------------
namespace
{
// Like EncodeDateTime; adding EncodeTime to a negative date would go back in time
TDateTime Make(Word Year, Word Month, Word Day, Word Hour = 0, Word Min = 0, Word Sec = 0, Word MSec = 0)
{
  return TDateTime(Year, Month, Day, Hour, Min, Sec, MSec);
}

class DateTimeTest : public testing::Test
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

class TimeZone
{
public:
  explicit TimeZone(const char * Name)
  {
    const char * Old = getenv("TZ");
    FHad = (Old != nullptr);
    if (FHad)
    {
      FOld = Old;
    }
    setenv("TZ", Name, 1);
    tzset();
  }

  ~TimeZone()
  {
    if (FHad)
    {
      setenv("TZ", FOld.c_str(), 1);
    }
    else
    {
      unsetenv("TZ");
    }
    tzset();
  }

private:
  bool FHad;
  std::string FOld;
};
} // namespace
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, EncodeDate)
{
  EXPECT_EQ(EncodeDate(1899, 12, 30).Val, 0);
  EXPECT_EQ(EncodeDate(1900, 1, 1).Val, 2);
  EXPECT_EQ(EncodeDate(2000, 1, 1).Val, 36526);
  EXPECT_EQ(EncodeDate(1, 1, 1).Val, -693593);
  EXPECT_EQ(EncodeDate(9999, 12, 31).Val, 2958465);
  EXPECT_EQ(EncodeDate(1970, 1, 1).Val, UnixDateDelta);
  EXPECT_NO_THROW(EncodeDate(2000, 2, 29));
  EXPECT_THROW(EncodeDate(2001, 2, 29), EConvertError);
  EXPECT_THROW(EncodeDate(1900, 2, 29), EConvertError);
  EXPECT_THROW(EncodeDate(2000, 13, 1), EConvertError);
  EXPECT_THROW(EncodeDate(0, 1, 1), EConvertError);
  EXPECT_TRUE(IsLeapYear(2000));
  EXPECT_FALSE(IsLeapYear(1900));
  EXPECT_TRUE(IsLeapYear(2024));
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, DecodeDateRoundTrip)
{
  for (int Day = -693593; Day <= 2958465; Day += 997)
  {
    Word Year, Month, MDay;
    DecodeDate(TDateTime(Day), Year, Month, MDay);
    ASSERT_EQ(EncodeDate(Year, Month, MDay).Val, Day) << Year << "-" << Month << "-" << MDay;
  }
  Word Year, Month, Day, DOW;
  EXPECT_TRUE(DecodeDateFully(EncodeDate(2024, 2, 29), Year, Month, Day, DOW));
  EXPECT_EQ(Year, 2024);
  EXPECT_EQ(Month, 2);
  EXPECT_EQ(Day, 29);
  // 2024-02-29 was a Thursday
  EXPECT_EQ(DOW, 5);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, NegativeValues)
{
  // -1.25 is 1899-12-29 06:00: the fraction counts forward from midnight
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DecodeDate(TDateTime(-1.25), Year, Month, Day);
  DecodeTime(TDateTime(-1.25), Hour, Min, Sec, MSec);
  EXPECT_EQ(Year, 1899);
  EXPECT_EQ(Month, 12);
  EXPECT_EQ(Day, 29);
  EXPECT_EQ(Hour, 6);
  EXPECT_EQ(Make(1899, 12, 29, 6).Val, -1.25);
  TTimeStamp Stamp = DateTimeToTimeStamp(TDateTime(-1.25));
  EXPECT_EQ(Stamp.Date, DateDelta - 1);
  EXPECT_EQ(Stamp.Time, 6 * 3600 * 1000);
  EXPECT_EQ(TimeStampToDateTime(Stamp).Val, -1.25);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, EncodeTime)
{
  TDateTime T = EncodeTime(12, 30, 15, 500);
  EXPECT_NEAR(T.Val, (12 * 3600 + 30 * 60 + 15.5) / 86400.0, 1e-12);
  Word Hour, Min, Sec, MSec;
  DecodeTime(T, Hour, Min, Sec, MSec);
  EXPECT_EQ(Hour, 12);
  EXPECT_EQ(Min, 30);
  EXPECT_EQ(Sec, 15);
  EXPECT_EQ(MSec, 500);
  EXPECT_THROW(EncodeTime(24, 0, 0, 0), EConvertError);
  EXPECT_THROW(EncodeTime(0, 60, 0, 0), EConvertError);
  // Rounding to whole milliseconds
  DecodeTime(TDateTime(EncodeTime(23, 59, 59, 999).Val + 1e-10), Hour, Min, Sec, MSec);
  EXPECT_EQ(MSec, 999);
  TTimeStamp Stamp = DateTimeToTimeStamp(TDateTime(0));
  EXPECT_EQ(Stamp.Date, DateDelta);
  EXPECT_EQ(Stamp.Time, 0);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, DayOfWeekAndIncMonth)
{
  // 2026-10-24 is a Saturday
  EXPECT_EQ(DayOfWeek(EncodeDate(2026, 10, 24)), 7);
  EXPECT_EQ(DayOfWeek(EncodeDate(2026, 10, 25)), 1);
  EXPECT_EQ(EncodeDate(2026, 10, 24).DayOfWeek(), 7);

  EXPECT_EQ(IncMonth(EncodeDate(2024, 1, 31)).Val, EncodeDate(2024, 2, 29).Val);
  EXPECT_EQ(IncMonth(EncodeDate(2024, 3, 31), -1).Val, EncodeDate(2024, 2, 29).Val);
  EXPECT_EQ(IncMonth(EncodeDate(2024, 5, 15), 12).Val, EncodeDate(2025, 5, 15).Val);
  EXPECT_EQ(IncMonth(EncodeDate(2024, 1, 15), -13).Val, EncodeDate(2022, 12, 15).Val);
  EXPECT_EQ(IncMonth(EncodeDate(2024, 12, 15), 1).Val, EncodeDate(2025, 1, 15).Val);
  EXPECT_NEAR(IncMonth(Make(2024, 1, 1, 12)).Val, Make(2024, 2, 1, 12).Val, 1e-9);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, ReplaceDateAndTime)
{
  TDateTime Value = Make(2020, 5, 6, 7, 8, 9);
  ReplaceTime(Value, EncodeTime(1, 2, 3, 0));
  EXPECT_NEAR(Value.Val, Make(2020, 5, 6, 1, 2, 3).Val, 1e-9);
  ReplaceDate(Value, EncodeDate(2021, 1, 1));
  EXPECT_NEAR(Value.Val, Make(2021, 1, 1, 1, 2, 3).Val, 1e-9);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, FormatDateTime)
{
  TDateTime Value = Make(2026, 10, 24, 13, 5, 7, 89);
  EXPECT_EQ(FormatDateTime(L"yyyy-mm-dd hh:nn:ss.zzz", Value), L"2026-10-24 13:05:07.089");
  EXPECT_EQ(FormatDateTime(L"d/m/yy", Value), L"24/10/26");
  EXPECT_EQ(FormatDateTime(L"dd mmm yyyy", Value), L"24 Oct 2026");
  EXPECT_EQ(FormatDateTime(L"dddd, mmmm d", Value), L"Saturday, October 24");
  EXPECT_EQ(FormatDateTime(L"ddd", Value), L"Sat");
  EXPECT_EQ(FormatDateTime(L"h:mm am/pm", Value), L"1:05 pm");
  EXPECT_EQ(FormatDateTime(L"hh:mm AM/PM", Value), L"01:05 PM");
  EXPECT_EQ(FormatDateTime(L"h:mm a/p", Value), L"1:05 p");
  EXPECT_EQ(FormatDateTime(L"h:mm ampm", Value), L"1:05 PM");
  // m after h is minutes, otherwise month
  EXPECT_EQ(FormatDateTime(L"h:m", Value), L"13:5");
  EXPECT_EQ(FormatDateTime(L"mm", Value), L"10");
  EXPECT_EQ(FormatDateTime(L"\"at\" yyyy 'o''clock'", Value), L"at 2026 oclock");
  EXPECT_EQ(FormatDateTime(L"YYYY-MM-DD", Value), L"2026-10-24");
  EXPECT_EQ(FormatDateTime(L"z", Value), L"89");
  EXPECT_EQ(FormatDateTime(L"c", Value), L"10/24/2026 13:05:07");
  EXPECT_EQ(FormatDateTime(L"c", EncodeDate(2026, 10, 24)), L"10/24/2026");
  EXPECT_EQ(FormatDateTime(L"ddddd", Value), L"10/24/2026");
  EXPECT_EQ(FormatDateTime(L"t", Value), L"13:05");
  EXPECT_EQ(FormatDateTime(L"tt", Value), L"13:05:07");
  EXPECT_EQ(FormatDateTime(L"yy", EncodeDate(2005, 1, 1)), L"05");
  EXPECT_EQ(FormatDateTime(L"h am/pm", Make(2026, 1, 1, 0)), L"12 am");
  EXPECT_EQ(FormatDateTime(L"h am/pm", Make(2026, 1, 1, 12)), L"12 pm");
  EXPECT_EQ(DateTimeToStr(Value), L"10/24/2026 13:05:07");
  EXPECT_EQ(DateToStr(Value), L"10/24/2026");
  EXPECT_EQ(TimeToStr(Value), L"13:05:07");

  TFormatSettings Settings = TFormatSettings::Invariant();
  Settings.DateSeparator = L'.';
  Settings.TimeSeparator = L'-';
  EXPECT_EQ(FormatDateTime(L"dd/mm/yyyy hh:nn", Value, Settings), L"24.10.2026 13-05");
  EXPECT_EQ(Value.FormatString(L"yyyy"), L"2026");
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, Parsing)
{
  EXPECT_NEAR(StrToDateTime(L"10/24/2026 13:05:07").Val, Make(2026, 10, 24, 13, 5, 7).Val, 1e-9);
  EXPECT_EQ(StrToDate(L"10/24/2026").Val, EncodeDate(2026, 10, 24).Val);
  EXPECT_EQ(StrToDate(L"2/3/2001").Val, EncodeDate(2001, 2, 3).Val);
  EXPECT_THROW(StrToDate(L"13/24/2026"), EConvertError);
  EXPECT_THROW(StrToDate(L"x"), EConvertError);
  EXPECT_NEAR(StrToTime(L"1:05 PM").Val, EncodeTime(13, 5, 0, 0).Val, 1e-9);
  EXPECT_NEAR(StrToTime(L"12:00 AM").Val, 0, 1e-9);
  EXPECT_NEAR(StrToTime(L"23:59:59.5").Val, EncodeTime(23, 59, 59, 5).Val, 1e-9);
  TDateTime Value;
  EXPECT_FALSE(TryStrToTime(L"25:00", Value));
  EXPECT_FALSE(TryStrToDateTime(L"10/24/2026 x", Value));
  EXPECT_TRUE(TryStrToDateTime(L"13:05", Value));
  EXPECT_NEAR(Value.Val, EncodeTime(13, 5, 0, 0).Val, 1e-9);

  TFormatSettings Settings = TFormatSettings::Invariant();
  Settings.ShortDateFormat = L"dd/mm/yyyy";
  Settings.DateSeparator = L'.';
  EXPECT_EQ(StrToDate(L"24.10.2026", Settings).Val, EncodeDate(2026, 10, 24).Val);
  Settings.ShortDateFormat = L"yyyy/mm/dd";
  Settings.DateSeparator = L'-';
  EXPECT_EQ(StrToDate(L"2026-10-24", Settings).Val, EncodeDate(2026, 10, 24).Val);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, Conversions)
{
  SYSTEMTIME SystemTime;
  TDateTime Value = Make(2026, 10, 24, 13, 5, 7, 89);
  DateTimeToSystemTime(Value, SystemTime);
  EXPECT_EQ(SystemTime.wYear, 2026);
  EXPECT_EQ(SystemTime.wMonth, 10);
  EXPECT_EQ(SystemTime.wDay, 24);
  EXPECT_EQ(SystemTime.wDayOfWeek, 6);
  EXPECT_EQ(SystemTime.wHour, 13);
  EXPECT_EQ(SystemTime.wMilliseconds, 89);
  EXPECT_NEAR(SystemTimeToDateTime(SystemTime).Val, Value.Val, 1e-9);

  TDateTime Even = Make(2026, 10, 24, 13, 5, 8);
  EXPECT_NEAR(FileDateToDateTime(DateTimeToFileDate(Even)).Val, Even.Val, 1e-9);
  EXPECT_EQ(DateTimeToFileDate(EncodeDate(1970, 1, 1)), 0);

  EXPECT_EQ(rtl::DateTimeFromUnixTime(0, 0, false).Val, UnixDateDelta);
  EXPECT_NEAR(rtl::DateTimeFromUnixTime(86400 + 43200, 0, false).Val, UnixDateDelta + 1.5, 1e-12);
  EXPECT_NEAR(rtl::DateTimeFromUnixTime(-43200, 0, false).Val, Make(1969, 12, 31, 12).Val, 1e-12);
  {
    TimeZone Zone("Europe/Warsaw");
    EXPECT_NEAR(rtl::DateTimeFromUnixTime(0, 0, true).Val, Make(1970, 1, 1, 1).Val, 1e-12);
    // Summer time
    EXPECT_NEAR(rtl::DateTimeFromUnixTime(1750000000, 0, true).Val,
      rtl::DateTimeFromUnixTime(1750000000 + 2 * 3600, 0, false).Val, 1e-12);
  }
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, Now)
{
  TimeZone Zone("UTC");
  time_t Before = time(nullptr);
  TDateTime Value = Now();
  time_t After = time(nullptr);
  double Seconds = (Value.Val - UnixDateDelta) * SecsPerDay;
  EXPECT_GE(Seconds, static_cast<double>(Before) - 0.001);
  EXPECT_LE(Seconds, static_cast<double>(After) + 1);
  EXPECT_EQ(Date().Val, std::trunc(Value.Val));
  EXPECT_LT(Time().Val, 1.0);
}
//---------------------------------------------------------------------------
TEST_F(DateTimeTest, TDateTimeClass)
{
  TDateTime A(2026, 10, 24);
  TDateTime B(static_cast<unsigned short>(12), 0, 0, 0);
  EXPECT_EQ(A.Val, EncodeDate(2026, 10, 24).Val);
  EXPECT_EQ(B.Val, 0.5);
  TDateTime C = A + B;
  EXPECT_EQ(C.Val, A.Val + 0.5);
  EXPECT_TRUE(C > A);
  EXPECT_TRUE(A == TDateTime(A.Val));
  C -= 1;
  EXPECT_EQ(C.Val, A.Val - 0.5);
  unsigned short Year, Month, Day, Hour, Min, Sec, MSec;
  TDateTime(2026, 10, 24, 13, 5, 7, 89).DecodeDate(&Year, &Month, &Day);
  TDateTime(2026, 10, 24, 13, 5, 7, 89).DecodeTime(&Hour, &Min, &Sec, &MSec);
  EXPECT_EQ(Year, 2026);
  EXPECT_EQ(Day, 24);
  EXPECT_EQ(Min, 5);
  EXPECT_EQ(MSec, 89);
  EXPECT_EQ(A.DateString(), L"10/24/2026");
  EXPECT_EQ(static_cast<double>(A), A.Val);
  EXPECT_EQ(TDateTime(L"10/24/2026", TDateTime::Date).Val, A.Val);
}
//---------------------------------------------------------------------------
TEST(FormatSettings, Locales)
{
  TFormatSettings English = TFormatSettings::Create(1033);
  EXPECT_EQ(English.ShortDateFormat, L"M/d/yyyy");
  EXPECT_EQ(English.LongMonthNames[0], L"January");
  EXPECT_EQ(English.CurrencyString, L"$");

  TFormatSettings C = TFormatSettings::Create(L"C");
  EXPECT_EQ(C.DecimalSeparator, L'.');
  EXPECT_EQ(C.ShortDateFormat, L"MM/dd/yyyy");

  // A real locale, if installed
  TFormatSettings Polish = TFormatSettings::Create(L"pl_PL.UTF-8");
  if (Polish.ShortDateFormat != L"MM/dd/yyyy")
  {
    EXPECT_EQ(Polish.DecimalSeparator, L',');
    EXPECT_EQ(Polish.DateSeparator, L'.');
    EXPECT_EQ(Polish.ShortDateFormat, L"dd/mm/yyyy");
    EXPECT_EQ(FormatDateTime(L"ddddd", EncodeDate(2026, 10, 24), Polish), L"24.10.2026");
    EXPECT_EQ(Polish.LongMonthNames[0], L"styczeń");
  }
}
