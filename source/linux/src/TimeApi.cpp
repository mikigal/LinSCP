// Time functions of the Windows API, see winapi/TimeApi.h.
//---------------------------------------------------------------------------
#include <winapi/TimeApi.h>

#include <time.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// 100ns intervals between 1601-01-01 and 1970-01-01
const long long FileTimeUnixDelta = 116444736000000000LL;
const long long TicksPerSecond = 10000000LL;
//---------------------------------------------------------------------------
long long FileTimeTicks(const FILETIME & Time)
{
  return static_cast<long long>((static_cast<unsigned long long>(Time.dwHighDateTime) << 32) | Time.dwLowDateTime);
}
//---------------------------------------------------------------------------
FILETIME TicksToFileTime(long long Ticks)
{
  FILETIME Result;
  Result.dwLowDateTime = static_cast<DWORD>(static_cast<unsigned long long>(Ticks) & 0xFFFFFFFF);
  Result.dwHighDateTime = static_cast<DWORD>(static_cast<unsigned long long>(Ticks) >> 32);
  return Result;
}
//---------------------------------------------------------------------------
// Seconds since the epoch of a time in file time ticks, and the remainder
void SplitTicks(long long Ticks, time_t & Seconds, long long & Remainder)
{
  long long Unix = Ticks - FileTimeUnixDelta;
  long long Whole = Unix / TicksPerSecond;
  Remainder = Unix % TicksPerSecond;
  if (Remainder < 0)
  {
    Remainder += TicksPerSecond;
    Whole--;
  }
  Seconds = static_cast<time_t>(Whole);
}
//---------------------------------------------------------------------------
void TmToSystemTime(const struct tm & Tm, long long RemainderTicks, SYSTEMTIME & SystemTime)
{
  SystemTime.wYear = static_cast<WORD>(Tm.tm_year + 1900);
  SystemTime.wMonth = static_cast<WORD>(Tm.tm_mon + 1);
  SystemTime.wDayOfWeek = static_cast<WORD>(Tm.tm_wday);
  SystemTime.wDay = static_cast<WORD>(Tm.tm_mday);
  SystemTime.wHour = static_cast<WORD>(Tm.tm_hour);
  SystemTime.wMinute = static_cast<WORD>(Tm.tm_min);
  SystemTime.wSecond = static_cast<WORD>(Tm.tm_sec);
  SystemTime.wMilliseconds = static_cast<WORD>(RemainderTicks / 10000);
}
//---------------------------------------------------------------------------
bool IsLeapYear(int Year)
{
  return ((Year % 4) == 0) && (((Year % 100) != 0) || ((Year % 400) == 0));
}
//---------------------------------------------------------------------------
int DaysInMonth(int Year, int Month)
{
  static const int Days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return ((Month == 2) && IsLeapYear(Year)) ? 29 : Days[Month - 1];
}
//---------------------------------------------------------------------------
bool SystemTimeToTm(const SYSTEMTIME & SystemTime, struct tm & Tm)
{
  if ((SystemTime.wYear < 1601) || (SystemTime.wYear > 30827) ||
      (SystemTime.wMonth < 1) || (SystemTime.wMonth > 12) ||
      (SystemTime.wDay < 1) || (SystemTime.wDay > DaysInMonth(SystemTime.wYear, SystemTime.wMonth)) ||
      (SystemTime.wHour > 23) || (SystemTime.wMinute > 59) || (SystemTime.wSecond > 59) ||
      (SystemTime.wMilliseconds > 999))
  {
    return false;
  }
  Tm = {};
  Tm.tm_year = SystemTime.wYear - 1900;
  Tm.tm_mon = SystemTime.wMonth - 1;
  Tm.tm_mday = SystemTime.wDay;
  Tm.tm_hour = SystemTime.wHour;
  Tm.tm_min = SystemTime.wMinute;
  Tm.tm_sec = SystemTime.wSecond;
  return true;
}
//---------------------------------------------------------------------------
long CurrentUtcOffset()
{
  time_t Now = time(nullptr);
  struct tm Local;
  localtime_r(&Now, &Local);
  return Local.tm_gmtoff;
}
//---------------------------------------------------------------------------
struct TTransition
{
  time_t Time;
  long OffsetBefore;
  long OffsetAfter;
  bool ToDaylight;
};
//---------------------------------------------------------------------------
void FindTransitions(int Year, long & StandardOffset, long & DaylightOffset, bool & HasDaylight,
  TTransition & ToDaylight, TTransition & ToStandard, bool & Found)
{
  struct tm Start = {};
  Start.tm_year = Year - 1900;
  Start.tm_mday = 1;
  time_t From = timegm(&Start);
  Start.tm_year++;
  time_t To = timegm(&Start);

  struct tm Local;
  localtime_r(&From, &Local);
  HasDaylight = (Local.tm_isdst > 0);
  StandardOffset = HasDaylight ? 0 : Local.tm_gmtoff;
  DaylightOffset = HasDaylight ? Local.tm_gmtoff : 0;
  bool StandardKnown = !HasDaylight;
  bool FoundToDaylight = false;
  bool FoundToStandard = false;
  int PreviousDst = Local.tm_isdst;
  long PreviousOffset = Local.tm_gmtoff;
  const time_t Step = 3600;
  for (time_t Time = From + Step; Time <= To; Time += Step)
  {
    localtime_r(&Time, &Local);
    if ((Local.tm_isdst > 0) != (PreviousDst > 0))
    {
      // Exact second of the transition within the step
      time_t Low = Time - Step;
      time_t High = Time;
      while (High - Low > 1)
      {
        time_t Middle = Low + (High - Low) / 2;
        struct tm MiddleLocal;
        localtime_r(&Middle, &MiddleLocal);
        if ((MiddleLocal.tm_isdst > 0) == (PreviousDst > 0))
        {
          Low = Middle;
        }
        else
        {
          High = Middle;
        }
      }
      TTransition Transition = { High, PreviousOffset, Local.tm_gmtoff, (Local.tm_isdst > 0) };
      if (Transition.ToDaylight && !FoundToDaylight && (High < To))
      {
        ToDaylight = Transition;
        FoundToDaylight = true;
      }
      else if (!Transition.ToDaylight && !FoundToStandard && (High < To))
      {
        ToStandard = Transition;
        FoundToStandard = true;
      }
    }
    if (Local.tm_isdst > 0)
    {
      HasDaylight = true;
      DaylightOffset = Local.tm_gmtoff;
    }
    else if (!StandardKnown)
    {
      StandardOffset = Local.tm_gmtoff;
      StandardKnown = true;
    }
    PreviousDst = Local.tm_isdst;
    PreviousOffset = Local.tm_gmtoff;
  }
  if (!StandardKnown)
  {
    // Daylight saving time all year, treat it as standard time
    StandardOffset = DaylightOffset;
    HasDaylight = false;
  }
  Found = FoundToDaylight && FoundToStandard;
}
//---------------------------------------------------------------------------
// The transition as a recurring rule, in the local time before it
SYSTEMTIME TransitionRule(const TTransition & Transition)
{
  time_t LocalBefore = Transition.Time + Transition.OffsetBefore;
  struct tm Tm;
  gmtime_r(&LocalBefore, &Tm);
  SYSTEMTIME Result = {};
  Result.wMonth = static_cast<WORD>(Tm.tm_mon + 1);
  Result.wDayOfWeek = static_cast<WORD>(Tm.tm_wday);
  int Week = (Tm.tm_mday - 1) / 7 + 1;
  if (Tm.tm_mday + 7 > DaysInMonth(Tm.tm_year + 1900, Tm.tm_mon + 1))
  {
    // The last occurrence in the month
    Week = 5;
  }
  Result.wDay = static_cast<WORD>(Week);
  Result.wHour = static_cast<WORD>(Tm.tm_hour);
  Result.wMinute = static_cast<WORD>(Tm.tm_min);
  Result.wSecond = static_cast<WORD>(Tm.tm_sec);
  return Result;
}
//---------------------------------------------------------------------------
// Time zone abbreviations are ASCII
void SetName(WCHAR (&Target)[32], const char * Name)
{
  size_t Index = 0;
  for (; (Name[Index] != '\0') && (Index < 31); Index++)
  {
    Target[Index] = static_cast<WCHAR>(static_cast<unsigned char>(Name[Index]));
  }
  Target[Index] = L'\0';
}
//---------------------------------------------------------------------------
bool DescribeTimeZone(int Year, TIME_ZONE_INFORMATION & Information)
{
  tzset();
  long StandardOffset;
  long DaylightOffset;
  bool HasDaylight;
  TTransition ToDaylight = {};
  TTransition ToStandard = {};
  bool Found;
  FindTransitions(Year, StandardOffset, DaylightOffset, HasDaylight, ToDaylight, ToStandard, Found);
  Information = {};
  Information.Bias = -StandardOffset / 60;
  SetName(Information.StandardName, tzname[0]);
  SetName(Information.DaylightName, tzname[1]);
  if (HasDaylight && Found)
  {
    Information.DaylightBias = -(DaylightOffset - StandardOffset) / 60;
    Information.DaylightDate = TransitionRule(ToDaylight);
    Information.StandardDate = TransitionRule(ToStandard);
  }
  return HasDaylight && Found;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
namespace winapi
{
//---------------------------------------------------------------------------
FILETIME TimespecToFileTime(const struct timespec & Time)
{
  return TicksToFileTime(
    static_cast<long long>(Time.tv_sec) * TicksPerSecond + Time.tv_nsec / 100 + FileTimeUnixDelta);
}
//---------------------------------------------------------------------------
struct timespec FileTimeToTimespec(const FILETIME & Time)
{
  time_t Seconds;
  long long Remainder;
  SplitTicks(FileTimeTicks(Time), Seconds, Remainder);
  struct timespec Result;
  Result.tv_sec = Seconds;
  Result.tv_nsec = static_cast<long>(Remainder * 100);
  return Result;
}
//---------------------------------------------------------------------------
} // namespace winapi
//---------------------------------------------------------------------------
void GetSystemTimeAsFileTime(LPFILETIME SystemTimeAsFileTime)
{
  struct timespec Now;
  clock_gettime(CLOCK_REALTIME, &Now);
  *SystemTimeAsFileTime = winapi::TimespecToFileTime(Now);
}
//---------------------------------------------------------------------------
void GetSystemTime(LPSYSTEMTIME SystemTime)
{
  FILETIME Now;
  GetSystemTimeAsFileTime(&Now);
  FileTimeToSystemTime(&Now, SystemTime);
}
//---------------------------------------------------------------------------
void GetLocalTime(LPSYSTEMTIME SystemTime)
{
  struct timespec Now;
  clock_gettime(CLOCK_REALTIME, &Now);
  struct tm Local;
  localtime_r(&Now.tv_sec, &Local);
  TmToSystemTime(Local, Now.tv_nsec / 100, *SystemTime);
}
//---------------------------------------------------------------------------
BOOL FileTimeToSystemTime(const FILETIME * FileTime, LPSYSTEMTIME SystemTime)
{
  long long Ticks = FileTimeTicks(*FileTime);
  if (Ticks < 0)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  time_t Seconds;
  long long Remainder;
  SplitTicks(Ticks, Seconds, Remainder);
  struct tm Tm;
  if (gmtime_r(&Seconds, &Tm) == nullptr)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  TmToSystemTime(Tm, Remainder, *SystemTime);
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL SystemTimeToFileTime(const SYSTEMTIME * SystemTime, LPFILETIME FileTime)
{
  struct tm Tm;
  if (!SystemTimeToTm(*SystemTime, Tm))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  long long Seconds = timegm(&Tm);
  *FileTime = TicksToFileTime(
    Seconds * TicksPerSecond + SystemTime->wMilliseconds * 10000LL + FileTimeUnixDelta);
  return TRUE;
}
//---------------------------------------------------------------------------
// As on Windows, with the current offset from UTC, whatever the date
BOOL FileTimeToLocalFileTime(const FILETIME * FileTime, LPFILETIME LocalFileTime)
{
  *LocalFileTime = TicksToFileTime(FileTimeTicks(*FileTime) + CurrentUtcOffset() * TicksPerSecond);
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL LocalFileTimeToFileTime(const FILETIME * LocalFileTime, LPFILETIME FileTime)
{
  *FileTime = TicksToFileTime(FileTimeTicks(*LocalFileTime) - CurrentUtcOffset() * TicksPerSecond);
  return TRUE;
}
//---------------------------------------------------------------------------
LONG CompareFileTime(const FILETIME * FileTime1, const FILETIME * FileTime2)
{
  unsigned long long Time1 = static_cast<unsigned long long>(FileTimeTicks(*FileTime1));
  unsigned long long Time2 = static_cast<unsigned long long>(FileTimeTicks(*FileTime2));
  return (Time1 < Time2) ? -1 : ((Time1 > Time2) ? 1 : 0);
}
//---------------------------------------------------------------------------
BOOL SystemTimeToTzSpecificLocalTime(
  const TIME_ZONE_INFORMATION * TimeZoneInformation, const SYSTEMTIME * UniversalTime, LPSYSTEMTIME LocalTime)
{
  struct tm Tm;
  if ((TimeZoneInformation != NULL) || !SystemTimeToTm(*UniversalTime, Tm))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  time_t Seconds = timegm(&Tm);
  struct tm Local;
  localtime_r(&Seconds, &Local);
  TmToSystemTime(Local, UniversalTime->wMilliseconds * 10000LL, *LocalTime);
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL TzSpecificLocalTimeToSystemTime(
  const TIME_ZONE_INFORMATION * TimeZoneInformation, const SYSTEMTIME * LocalTime, LPSYSTEMTIME UniversalTime)
{
  struct tm Tm;
  if ((TimeZoneInformation != NULL) || !SystemTimeToTm(*LocalTime, Tm))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  Tm.tm_isdst = -1;
  time_t Seconds = mktime(&Tm);
  struct tm Universal;
  gmtime_r(&Seconds, &Universal);
  TmToSystemTime(Universal, LocalTime->wMilliseconds * 10000LL, *UniversalTime);
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetTimeZoneInformation(LPTIME_ZONE_INFORMATION TimeZoneInformation)
{
  time_t Now = time(nullptr);
  struct tm Local;
  localtime_r(&Now, &Local);
  if (!DescribeTimeZone(Local.tm_year + 1900, *TimeZoneInformation))
  {
    return TIME_ZONE_ID_UNKNOWN;
  }
  return (Local.tm_isdst > 0) ? TIME_ZONE_ID_DAYLIGHT : TIME_ZONE_ID_STANDARD;
}
//---------------------------------------------------------------------------
BOOL GetTimeZoneInformationForYear(
  USHORT Year, PDYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZoneInformation, LPTIME_ZONE_INFORMATION TimeZoneInformation)
{
  if (DynamicTimeZoneInformation != NULL)
  {
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
  }
  DescribeTimeZone(Year, *TimeZoneInformation);
  return TRUE;
}
//---------------------------------------------------------------------------
