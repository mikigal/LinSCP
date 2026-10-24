// Time functions of the platform layer, on the system time zone database.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>

#include "TestUtils.h"

#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
bool HasZone(const char * Zone)
{
  return access((std::string("/usr/share/zoneinfo/") + Zone).c_str(), R_OK) == 0;
}

void ExpectRule(const SYSTEMTIME & Rule, WORD Month, WORD DayOfWeek, WORD Week, WORD Hour)
{
  EXPECT_EQ(Rule.wYear, 0);
  EXPECT_EQ(Rule.wMonth, Month);
  EXPECT_EQ(Rule.wDayOfWeek, DayOfWeek);
  EXPECT_EQ(Rule.wDay, Week);
  EXPECT_EQ(Rule.wHour, Hour);
  EXPECT_EQ(Rule.wMinute, 0);
}
} // namespace
//---------------------------------------------------------------------------
TEST(TimeApiTest, FileTimeAndSystemTime)
{
  FILETIME Zero = { 0, 0 };
  SYSTEMTIME System;
  ASSERT_TRUE(FileTimeToSystemTime(&Zero, &System));
  EXPECT_EQ(System.wYear, 1601);
  EXPECT_EQ(System.wMonth, 1);
  EXPECT_EQ(System.wDay, 1);
  // A Monday
  EXPECT_EQ(System.wDayOfWeek, 1);

  SYSTEMTIME Source = { 2024, 2, 0, 29, 23, 59, 58, 123 };
  FILETIME FileTime;
  ASSERT_TRUE(SystemTimeToFileTime(&Source, &FileTime));
  ASSERT_TRUE(FileTimeToSystemTime(&FileTime, &System));
  EXPECT_EQ(System.wYear, 2024);
  EXPECT_EQ(System.wMonth, 2);
  EXPECT_EQ(System.wDay, 29);
  EXPECT_EQ(System.wDayOfWeek, 4);
  EXPECT_EQ(System.wHour, 23);
  EXPECT_EQ(System.wMinute, 59);
  EXPECT_EQ(System.wSecond, 58);
  EXPECT_EQ(System.wMilliseconds, 123);

  struct timespec Spec = winapi::FileTimeToTimespec(FileTime);
  EXPECT_EQ(Spec.tv_sec, 1709251198);
  EXPECT_EQ(Spec.tv_nsec, 123000000);
  FILETIME Back = winapi::TimespecToFileTime(Spec);
  EXPECT_EQ(CompareFileTime(&Back, &FileTime), 0);

  SYSTEMTIME Invalid = { 2023, 2, 0, 29, 0, 0, 0, 0 };
  EXPECT_FALSE(SystemTimeToFileTime(&Invalid, &FileTime));
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, BeforeTheUnixEpoch)
{
  SYSTEMTIME Source = { 1900, 12, 0, 31, 10, 20, 30, 400 };
  FILETIME FileTime;
  ASSERT_TRUE(SystemTimeToFileTime(&Source, &FileTime));
  SYSTEMTIME System;
  ASSERT_TRUE(FileTimeToSystemTime(&FileTime, &System));
  EXPECT_EQ(System.wYear, 1900);
  EXPECT_EQ(System.wSecond, 30);
  EXPECT_EQ(System.wMilliseconds, 400);
  struct timespec Spec = winapi::FileTimeToTimespec(FileTime);
  EXPECT_LT(Spec.tv_sec, 0);
  EXPECT_EQ(Spec.tv_nsec, 400000000);
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, LocalFileTimeUsesTheCurrentOffset)
{
  if (!HasZone("Etc/GMT-3"))
  {
    GTEST_SKIP() << "no time zone database";
  }
  TEnvironmentScope Zone("TZ", "Etc/GMT-3");
  SYSTEMTIME Source = { 2024, 1, 0, 1, 0, 0, 0, 0 };
  FILETIME Universal, Local, Back;
  SystemTimeToFileTime(&Source, &Universal);
  ASSERT_TRUE(FileTimeToLocalFileTime(&Universal, &Local));
  SYSTEMTIME System;
  FileTimeToSystemTime(&Local, &System);
  EXPECT_EQ(System.wHour, 3);
  ASSERT_TRUE(LocalFileTimeToFileTime(&Local, &Back));
  EXPECT_EQ(CompareFileTime(&Back, &Universal), 0);
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, TzSpecificLocalTime)
{
  if (!HasZone("Europe/Warsaw"))
  {
    GTEST_SKIP() << "no time zone database";
  }
  TEnvironmentScope Zone("TZ", "Europe/Warsaw");
  SYSTEMTIME Summer = { 2024, 7, 0, 1, 12, 0, 0, 0 };
  SYSTEMTIME Winter = { 2024, 1, 0, 1, 12, 0, 0, 0 };
  SYSTEMTIME Local, Universal;
  ASSERT_TRUE(SystemTimeToTzSpecificLocalTime(NULL, &Summer, &Local));
  EXPECT_EQ(Local.wHour, 14);
  ASSERT_TRUE(SystemTimeToTzSpecificLocalTime(NULL, &Winter, &Local));
  EXPECT_EQ(Local.wHour, 13);
  ASSERT_TRUE(TzSpecificLocalTimeToSystemTime(NULL, &Local, &Universal));
  EXPECT_EQ(Universal.wHour, 12);
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, TimeZoneRulesEurope)
{
  if (!HasZone("Europe/Warsaw"))
  {
    GTEST_SKIP() << "no time zone database";
  }
  TEnvironmentScope Zone("TZ", "Europe/Warsaw");
  TIME_ZONE_INFORMATION Information;
  ASSERT_TRUE(GetTimeZoneInformationForYear(2024, NULL, &Information));
  EXPECT_EQ(Information.Bias, -60);
  EXPECT_EQ(Information.StandardBias, 0);
  EXPECT_EQ(Information.DaylightBias, -60);
  // Last Sunday of March, 02:00 standard time; last Sunday of October, 03:00 daylight time
  ExpectRule(Information.DaylightDate, 3, 0, 5, 2);
  ExpectRule(Information.StandardDate, 10, 0, 5, 3);
  EXPECT_EQ(UnicodeString(Information.StandardName), L"CET");
  EXPECT_EQ(UnicodeString(Information.DaylightName), L"CEST");

  DWORD Id = GetTimeZoneInformation(&Information);
  EXPECT_TRUE((Id == TIME_ZONE_ID_STANDARD) || (Id == TIME_ZONE_ID_DAYLIGHT));
  EXPECT_EQ(Information.Bias, -60);
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, TimeZoneRulesAmerica)
{
  if (!HasZone("America/New_York"))
  {
    GTEST_SKIP() << "no time zone database";
  }
  TEnvironmentScope Zone("TZ", "America/New_York");
  TIME_ZONE_INFORMATION Information;
  ASSERT_TRUE(GetTimeZoneInformationForYear(2024, NULL, &Information));
  EXPECT_EQ(Information.Bias, 300);
  EXPECT_EQ(Information.DaylightBias, -60);
  // Second Sunday of March, first Sunday of November, both 02:00
  ExpectRule(Information.DaylightDate, 3, 0, 2, 2);
  ExpectRule(Information.StandardDate, 11, 0, 1, 2);
  // Before 2007, the first Sunday of April and the last Sunday of October
  ASSERT_TRUE(GetTimeZoneInformationForYear(2005, NULL, &Information));
  ExpectRule(Information.DaylightDate, 4, 0, 1, 2);
  ExpectRule(Information.StandardDate, 10, 0, 5, 2);
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, TimeZoneRulesSouthernHemisphere)
{
  if (!HasZone("Australia/Sydney"))
  {
    GTEST_SKIP() << "no time zone database";
  }
  TEnvironmentScope Zone("TZ", "Australia/Sydney");
  TIME_ZONE_INFORMATION Information;
  ASSERT_TRUE(GetTimeZoneInformationForYear(2024, NULL, &Information));
  EXPECT_EQ(Information.Bias, -600);
  EXPECT_EQ(Information.DaylightBias, -60);
  // First Sunday of October 02:00, first Sunday of April 03:00
  ExpectRule(Information.DaylightDate, 10, 0, 1, 2);
  ExpectRule(Information.StandardDate, 4, 0, 1, 3);
}
//---------------------------------------------------------------------------
TEST(TimeApiTest, TimeZoneWithoutDaylightSavingTime)
{
  TEnvironmentScope Zone("TZ", "UTC");
  TIME_ZONE_INFORMATION Information;
  EXPECT_EQ(GetTimeZoneInformation(&Information), static_cast<DWORD>(TIME_ZONE_ID_UNKNOWN));
  EXPECT_EQ(Information.Bias, 0);
  EXPECT_EQ(Information.DaylightDate.wMonth, 0);
  if (HasZone("Asia/Tokyo"))
  {
    TEnvironmentScope Tokyo("TZ", "Asia/Tokyo");
    ASSERT_TRUE(GetTimeZoneInformationForYear(2024, NULL, &Information));
    EXPECT_EQ(Information.Bias, -540);
    EXPECT_EQ(Information.DaylightDate.wMonth, 0);
    EXPECT_EQ(Information.StandardDate.wMonth, 0);
  }
}
//---------------------------------------------------------------------------
