// Time functions of the Windows API, on the system time zone (tzdata).
//
// GetTimeZoneInformation(ForYear) describe the time zone the way Windows
// does: the bias of standard time and the daylight saving transitions of a
// year as recurring rules (month, weekday, week of the month, local time
// before the transition), found by scanning the year with localtime_r.
//---------------------------------------------------------------------------
#ifndef WinApiTimeApiH
#define WinApiTimeApiH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <time.h>
//---------------------------------------------------------------------------
#define TIME_ZONE_ID_UNKNOWN 0
#define TIME_ZONE_ID_STANDARD 1
#define TIME_ZONE_ID_DAYLIGHT 2
#define TIME_ZONE_ID_INVALID (static_cast<DWORD>(0xFFFFFFFF))
//---------------------------------------------------------------------------
struct DYNAMIC_TIME_ZONE_INFORMATION
{
  LONG Bias;
  WCHAR StandardName[32];
  SYSTEMTIME StandardDate;
  LONG StandardBias;
  WCHAR DaylightName[32];
  SYSTEMTIME DaylightDate;
  LONG DaylightBias;
  WCHAR TimeZoneKeyName[128];
  BOOLEAN DynamicDaylightTimeDisabled;
};
typedef DYNAMIC_TIME_ZONE_INFORMATION * PDYNAMIC_TIME_ZONE_INFORMATION;
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
void GetSystemTime(LPSYSTEMTIME SystemTime);
void GetLocalTime(LPSYSTEMTIME SystemTime);
void GetSystemTimeAsFileTime(LPFILETIME SystemTimeAsFileTime);
BOOL FileTimeToSystemTime(const FILETIME * FileTime, LPSYSTEMTIME SystemTime);
BOOL SystemTimeToFileTime(const SYSTEMTIME * SystemTime, LPFILETIME FileTime);
BOOL FileTimeToLocalFileTime(const FILETIME * FileTime, LPFILETIME LocalFileTime);
BOOL LocalFileTimeToFileTime(const FILETIME * LocalFileTime, LPFILETIME FileTime);
LONG CompareFileTime(const FILETIME * FileTime1, const FILETIME * FileTime2);
// The time zone information must be NULL (the system time zone)
BOOL SystemTimeToTzSpecificLocalTime(
  const TIME_ZONE_INFORMATION * TimeZoneInformation, const SYSTEMTIME * UniversalTime, LPSYSTEMTIME LocalTime);
BOOL TzSpecificLocalTimeToSystemTime(
  const TIME_ZONE_INFORMATION * TimeZoneInformation, const SYSTEMTIME * LocalTime, LPSYSTEMTIME UniversalTime);
DWORD GetTimeZoneInformation(LPTIME_ZONE_INFORMATION TimeZoneInformation);
// The dynamic time zone must be NULL (the system time zone)
BOOL GetTimeZoneInformationForYear(
  USHORT Year, PDYNAMIC_TIME_ZONE_INFORMATION DynamicTimeZoneInformation, LPTIME_ZONE_INFORMATION TimeZoneInformation);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
namespace winapi
{
FILETIME TimespecToFileTime(const struct timespec & Time);
struct timespec FileTimeToTimespec(const FILETIME & Time);
}
//---------------------------------------------------------------------------
#endif
