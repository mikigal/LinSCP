// The System.DateUtils unit, including TTimeZone.
//---------------------------------------------------------------------------
#ifndef RtlDateUtilsH
#define RtlDateUtilsH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
#include <rtl/TimeSpan.h>
#include <rtl/Types.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Dateutils
{
//---------------------------------------------------------------------------
const double OneHour = 1.0 / HoursPerDay;
const double OneMinute = 1.0 / MinsPerDay;
const double OneSecond = 1.0 / SecsPerDay;
const double OneMillisecond = 1.0 / MSecsPerDay;
const double ApproxDaysPerMonth = 30.4375;
const double ApproxDaysPerYear = 365.25;
const int DaysPerWeek = 7;
const int MonthsPerYear = 12;

const Word DayMonday = 1;
const Word DayTuesday = 2;
const Word DayWednesday = 3;
const Word DayThursday = 4;
const Word DayFriday = 5;
const Word DaySaturday = 6;
const Word DaySunday = 7;
//---------------------------------------------------------------------------
TDateTime DateOf(const TDateTime AValue);
TDateTime TimeOf(const TDateTime AValue);
bool IsInLeapYear(const TDateTime AValue);
bool IsPM(const TDateTime AValue);
bool IsAM(const TDateTime AValue);
bool IsValidDate(const Word AYear, const Word AMonth, const Word ADay);
bool IsValidTime(const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond);
bool IsValidDateTime(const Word AYear, const Word AMonth, const Word ADay,
  const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond);
Word DaysInYear(const TDateTime AValue);
Word DaysInMonth(const TDateTime AValue);
Word DaysInAYear(const Word AYear);
Word DaysInAMonth(const Word AYear, const Word AMonth);
TDateTime Today();
TDateTime Yesterday();
TDateTime Tomorrow();
bool IsToday(const TDateTime AValue);
bool IsSameDay(const TDateTime AValue, const TDateTime ABasis);

Word YearOf(const TDateTime AValue);
Word MonthOf(const TDateTime AValue);
Word DayOf(const TDateTime AValue);
Word HourOf(const TDateTime AValue);
Word MinuteOf(const TDateTime AValue);
Word SecondOf(const TDateTime AValue);
Word MilliSecondOf(const TDateTime AValue);
// ISO 8601: Monday = 1 ... Sunday = 7
Word DayOfTheWeek(const TDateTime AValue);
Word DayOfTheMonth(const TDateTime AValue);
Word DayOfTheYear(const TDateTime AValue);
Word HourOfTheDay(const TDateTime AValue);
Word MinuteOfTheHour(const TDateTime AValue);
unsigned int MinuteOfTheDay(const TDateTime AValue);
Word SecondOfTheMinute(const TDateTime AValue);
unsigned int SecondOfTheHour(const TDateTime AValue);
unsigned int SecondOfTheDay(const TDateTime AValue);
Word MilliSecondOfTheSecond(const TDateTime AValue);
unsigned int MilliSecondOfTheMinute(const TDateTime AValue);
unsigned int MilliSecondOfTheHour(const TDateTime AValue);
unsigned int MilliSecondOfTheDay(const TDateTime AValue);
__int64 MilliSecondOfTheMonth(const TDateTime AValue);
__int64 MilliSecondOfTheYear(const TDateTime AValue);
TDateTime StartOfTheDay(const TDateTime AValue);
TDateTime EndOfTheDay(const TDateTime AValue);
TDateTime StartOfTheMonth(const TDateTime AValue);
TDateTime StartOfTheYear(const TDateTime AValue);

__int64 YearsBetween(const TDateTime ANow, const TDateTime AThen);
__int64 MonthsBetween(const TDateTime ANow, const TDateTime AThen);
__int64 WeeksBetween(const TDateTime ANow, const TDateTime AThen);
__int64 DaysBetween(const TDateTime ANow, const TDateTime AThen);
__int64 HoursBetween(const TDateTime ANow, const TDateTime AThen);
__int64 MinutesBetween(const TDateTime ANow, const TDateTime AThen);
__int64 SecondsBetween(const TDateTime ANow, const TDateTime AThen);
__int64 MilliSecondsBetween(const TDateTime ANow, const TDateTime AThen);
double YearSpan(const TDateTime ANow, const TDateTime AThen);
double MonthSpan(const TDateTime ANow, const TDateTime AThen);
double WeekSpan(const TDateTime ANow, const TDateTime AThen);
double DaySpan(const TDateTime ANow, const TDateTime AThen);
double HourSpan(const TDateTime ANow, const TDateTime AThen);
double MinuteSpan(const TDateTime ANow, const TDateTime AThen);
double SecondSpan(const TDateTime ANow, const TDateTime AThen);
double MilliSecondSpan(const TDateTime ANow, const TDateTime AThen);
bool WithinPastDays(const TDateTime ANow, const TDateTime AThen, const int ADays);
bool WithinPastHours(const TDateTime ANow, const TDateTime AThen, const __int64 AHours);
bool WithinPastMinutes(const TDateTime ANow, const TDateTime AThen, const __int64 AMinutes);
bool WithinPastSeconds(const TDateTime ANow, const TDateTime AThen, const __int64 ASeconds);
bool WithinPastMilliSeconds(const TDateTime ANow, const TDateTime AThen, const __int64 AMilliSeconds);

TDateTime IncYear(const TDateTime AValue, const int ANumberOfYears = 1);
TDateTime IncWeek(const TDateTime AValue, const int ANumberOfWeeks = 1);
TDateTime IncDay(const TDateTime AValue, const int ANumberOfDays = 1);
TDateTime IncHour(const TDateTime AValue, const __int64 ANumberOfHours = 1);
TDateTime IncMinute(const TDateTime AValue, const __int64 ANumberOfMinutes = 1);
TDateTime IncSecond(const TDateTime AValue, const __int64 ANumberOfSeconds = 1);
TDateTime IncMilliSecond(const TDateTime AValue, const __int64 ANumberOfMilliSeconds = 1);

TDateTime EncodeDateTime(const Word AYear, const Word AMonth, const Word ADay,
  const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond);
bool TryEncodeDateTime(const Word AYear, const Word AMonth, const Word ADay,
  const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond, TDateTime & AValue);
void DecodeDateTime(const TDateTime AValue, Word & AYear, Word & AMonth, Word & ADay,
  Word & AHour, Word & AMinute, Word & ASecond, Word & AMilliSecond);

TValueRelationship CompareDateTime(const TDateTime A, const TDateTime B);
bool SameDateTime(const TDateTime A, const TDateTime B);
TValueRelationship CompareDate(const TDateTime A, const TDateTime B);
bool SameDate(const TDateTime A, const TDateTime B);
TValueRelationship CompareTime(const TDateTime A, const TDateTime B);
bool SameTime(const TDateTime A, const TDateTime B);

// AInputIsUTC/AReturnUTC false: the value is in local time
__int64 DateTimeToUnix(const TDateTime AValue, bool AInputIsUTC = true);
TDateTime UnixToDateTime(const __int64 AValue, bool AReturnUTC = true);
__int64 DateTimeToMilliseconds(const TDateTime ADateTime);
TDateTime ISO8601ToDate(const UnicodeString AISODate, bool AReturnUTC = true);
bool TryISO8601ToDate(const UnicodeString AISODate, TDateTime & Value, bool AReturnUTC = true);
UnicodeString DateToISO8601(const TDateTime ADate, bool AInputIsUTC = true);
//---------------------------------------------------------------------------
enum TLocalTimeType { lttStandard, lttDaylight, lttAmbiguous, lttInvalid };
//---------------------------------------------------------------------------
class TTimeZone
{
public:
  virtual ~TTimeZone() {}

  // The system time zone (TZ / /etc/localtime)
  static TTimeZone * const Local;

  TTimeSpan GetUtcOffset(const TDateTime ADateTime, const bool ForceDaylight = false);
  TDateTime ToUniversalTime(const TDateTime ADateTime, const bool ForceDaylight = false);
  TDateTime ToLocalTime(const TDateTime ADateTime);
  bool IsDaylightTime(const TDateTime ADateTime, const bool ForceDaylight = false);
  bool IsStandardTime(const TDateTime ADateTime, const bool ForceDaylight = false);
  TLocalTimeType GetLocalTimeType(const TDateTime ADateTime);
  UnicodeString GetAbbreviation(const TDateTime ADateTime, const bool ForceDaylight = false);
  UnicodeString GetDisplayName(const TDateTime ADateTime, const bool ForceDaylight = false);
  UnicodeString GetID();
  RTL_PROPERTY_GET(ID, GetID)
  __declspec(property(get = __property_get_ID)) UnicodeString ID;
};
//---------------------------------------------------------------------------
} // namespace Dateutils
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Dateutils;
//---------------------------------------------------------------------------
#endif
