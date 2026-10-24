// System.DateUtils and System.TimeSpan
//---------------------------------------------------------------------------
#include <rtl/DateUtils.h>
#include <rtl/Math.h>

#include <cmath>
#include <climits>
#include <time.h>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
namespace Timespan
{
//---------------------------------------------------------------------------
const TTimeSpan TTimeSpan::Zero(0);
const TTimeSpan TTimeSpan::MaxValue(LLONG_MAX);
const TTimeSpan TTimeSpan::MinValue(LLONG_MIN);
//---------------------------------------------------------------------------
namespace
{
TTimeSpan Interval(double Value, double MillisecondsScale)
{
  double Milliseconds = Value * MillisecondsScale;
  Milliseconds += (Value >= 0) ? 0.5 : -0.5;
  if ((Milliseconds > static_cast<double>(LLONG_MAX / TTimeSpan::TicksPerMillisecond)) ||
      (Milliseconds < static_cast<double>(LLONG_MIN / TTimeSpan::TicksPerMillisecond)))
  {
    throw EArgumentOutOfRangeException(L"Time span value out of range");
  }
  return TTimeSpan(static_cast<__int64>(std::trunc(Milliseconds)) * TTimeSpan::TicksPerMillisecond);
}
} // namespace
//---------------------------------------------------------------------------
TTimeSpan::TTimeSpan(int Hours, int Minutes, int Seconds) :
  FTicks((static_cast<__int64>(Hours) * 3600 + static_cast<__int64>(Minutes) * 60 + Seconds) * TicksPerSecond)
{
}
//---------------------------------------------------------------------------
TTimeSpan::TTimeSpan(int Days, int Hours, int Minutes, int Seconds) :
  TTimeSpan(Days, Hours, Minutes, Seconds, 0)
{
}
//---------------------------------------------------------------------------
TTimeSpan::TTimeSpan(int Days, int Hours, int Minutes, int Seconds, int Milliseconds) :
  FTicks(((static_cast<__int64>(Days) * 86400 + static_cast<__int64>(Hours) * 3600 +
    static_cast<__int64>(Minutes) * 60 + Seconds) * 1000 + Milliseconds) * TicksPerMillisecond)
{
}
//---------------------------------------------------------------------------
TTimeSpan TTimeSpan::FromDays(double Value) { return Interval(Value, MSecsPerDay); }
TTimeSpan TTimeSpan::FromHours(double Value) { return Interval(Value, 3600000); }
TTimeSpan TTimeSpan::FromMinutes(double Value) { return Interval(Value, 60000); }
TTimeSpan TTimeSpan::FromSeconds(double Value) { return Interval(Value, 1000); }
TTimeSpan TTimeSpan::FromMilliseconds(double Value) { return Interval(Value, 1); }
//---------------------------------------------------------------------------
TTimeSpan TTimeSpan::Subtract(const TDateTime D1, const TDateTime D2)
{
  return TTimeSpan((Dateutils::DateTimeToMilliseconds(D1) - Dateutils::DateTimeToMilliseconds(D2)) * TicksPerMillisecond);
}
//---------------------------------------------------------------------------
UnicodeString TTimeSpan::ToString() const
{
  // [-][d.]hh:mm:ss[.fffffff]
  unsigned long long Ticks = (FTicks < 0) ? (0ULL - static_cast<unsigned long long>(FTicks)) : FTicks;
  unsigned long long Days = Ticks / TicksPerDay;
  unsigned long long Rest = Ticks % TicksPerDay;
  UnicodeString Result = (FTicks < 0) ? L"-" : L"";
  if (Days > 0)
  {
    Result += UIntToStr(Days) + L".";
  }
  Result += Format(L"%.2d:%.2d:%.2d", ARRAYOFCONST((
    static_cast<int>(Rest / TicksPerHour), static_cast<int>((Rest / TicksPerMinute) % 60),
    static_cast<int>((Rest / TicksPerSecond) % 60))));
  unsigned long long Fraction = Rest % TicksPerSecond;
  if (Fraction > 0)
  {
    Result += Format(L".%.7d", ARRAYOFCONST((static_cast<int>(Fraction))));
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Timespan
//---------------------------------------------------------------------------
namespace Dateutils
{
//---------------------------------------------------------------------------
namespace
{
TDateTime FromMilliseconds(__int64 Milliseconds)
{
  TTimeStamp Stamp;
  __int64 Date = Milliseconds / MSecsPerDay;
  __int64 Time = Milliseconds % MSecsPerDay;
  if (Time < 0)
  {
    Time += MSecsPerDay;
    Date--;
  }
  Stamp.Date = static_cast<int>(Date);
  Stamp.Time = static_cast<int>(Time);
  return TimeStampToDateTime(Stamp);
}

__int64 UnixMilliseconds(const TDateTime UtcValue)
{
  return DateTimeToMilliseconds(UtcValue) - DateTimeToMilliseconds(TDateTime(UnixDateDelta));
}

TDateTime FromUnixMilliseconds(__int64 Milliseconds)
{
  return FromMilliseconds(Milliseconds + DateTimeToMilliseconds(TDateTime(UnixDateDelta)));
}

// Seconds since the epoch of a local time, through mktime()
bool LocalToEpoch(const TDateTime ADateTime, time_t & Epoch, tm & Info)
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DecodeDateTime(ADateTime, Year, Month, Day, Hour, Min, Sec, MSec);
  tm Local = {};
  Local.tm_year = Year - 1900;
  Local.tm_mon = Month - 1;
  Local.tm_mday = Day;
  Local.tm_hour = Hour;
  Local.tm_min = Min;
  Local.tm_sec = Sec;
  Local.tm_isdst = -1;
  Epoch = mktime(&Local);
  Info = Local;
  return (Epoch != static_cast<time_t>(-1)) || (Local.tm_year == 69);
}
} // namespace
//---------------------------------------------------------------------------
TDateTime DateOf(const TDateTime AValue) { return TDateTime(std::trunc(AValue.Val)); }
TDateTime TimeOf(const TDateTime AValue) { return TDateTime(std::fabs(AValue.Val - std::trunc(AValue.Val))); }
bool IsInLeapYear(const TDateTime AValue) { return IsLeapYear(YearOf(AValue)); }
bool IsPM(const TDateTime AValue) { return (HourOf(AValue) >= 12); }
bool IsAM(const TDateTime AValue) { return (HourOf(AValue) < 12); }
//---------------------------------------------------------------------------
bool IsValidDate(const Word AYear, const Word AMonth, const Word ADay)
{
  TDateTime Dummy;
  return TryEncodeDate(AYear, AMonth, ADay, Dummy);
}
//---------------------------------------------------------------------------
bool IsValidTime(const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond)
{
  TDateTime Dummy;
  return TryEncodeTime(AHour, AMinute, ASecond, AMilliSecond, Dummy);
}
//---------------------------------------------------------------------------
bool IsValidDateTime(const Word AYear, const Word AMonth, const Word ADay,
  const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond)
{
  return IsValidDate(AYear, AMonth, ADay) && IsValidTime(AHour, AMinute, ASecond, AMilliSecond);
}
//---------------------------------------------------------------------------
Word DaysInAYear(const Word AYear) { return IsLeapYear(AYear) ? 366 : 365; }
//---------------------------------------------------------------------------
Word DaysInAMonth(const Word AYear, const Word AMonth)
{
  static const Word Days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return ((AMonth == 2) && IsLeapYear(AYear)) ? 29 : Days[(AMonth - 1) % 12];
}
//---------------------------------------------------------------------------
Word DaysInYear(const TDateTime AValue) { return DaysInAYear(YearOf(AValue)); }
Word DaysInMonth(const TDateTime AValue) { return DaysInAMonth(YearOf(AValue), MonthOf(AValue)); }
TDateTime Today() { return Date(); }
TDateTime Yesterday() { return TDateTime(Date().Val - 1); }
TDateTime Tomorrow() { return TDateTime(Date().Val + 1); }
bool IsToday(const TDateTime AValue) { return IsSameDay(AValue, Date()); }
bool IsSameDay(const TDateTime AValue, const TDateTime ABasis) { return (DateOf(AValue).Val == DateOf(ABasis).Val); }
//---------------------------------------------------------------------------
Word YearOf(const TDateTime AValue) { Word Y, M, D; DecodeDate(AValue, Y, M, D); return Y; }
Word MonthOf(const TDateTime AValue) { Word Y, M, D; DecodeDate(AValue, Y, M, D); return M; }
Word DayOf(const TDateTime AValue) { Word Y, M, D; DecodeDate(AValue, Y, M, D); return D; }
Word HourOf(const TDateTime AValue) { Word H, M, S, MS; DecodeTime(AValue, H, M, S, MS); return H; }
Word MinuteOf(const TDateTime AValue) { Word H, M, S, MS; DecodeTime(AValue, H, M, S, MS); return M; }
Word SecondOf(const TDateTime AValue) { Word H, M, S, MS; DecodeTime(AValue, H, M, S, MS); return S; }
Word MilliSecondOf(const TDateTime AValue) { Word H, M, S, MS; DecodeTime(AValue, H, M, S, MS); return MS; }
//---------------------------------------------------------------------------
Word DayOfTheWeek(const TDateTime AValue)
{
  return static_cast<Word>((DateTimeToTimeStamp(AValue).Date - 1) % 7 + 1);
}
//---------------------------------------------------------------------------
Word DayOfTheMonth(const TDateTime AValue) { return DayOf(AValue); }
Word DayOfTheYear(const TDateTime AValue)
{
  return static_cast<Word>(std::trunc(AValue.Val - StartOfTheYear(AValue).Val) + 1);
}
Word HourOfTheDay(const TDateTime AValue) { return HourOf(AValue); }
Word MinuteOfTheHour(const TDateTime AValue) { return MinuteOf(AValue); }
unsigned int MinuteOfTheDay(const TDateTime AValue) { return HourOf(AValue) * 60u + MinuteOf(AValue); }
Word SecondOfTheMinute(const TDateTime AValue) { return SecondOf(AValue); }
unsigned int SecondOfTheHour(const TDateTime AValue) { return MinuteOf(AValue) * 60u + SecondOf(AValue); }
unsigned int SecondOfTheDay(const TDateTime AValue) { return MinuteOfTheDay(AValue) * 60u + SecondOf(AValue); }
Word MilliSecondOfTheSecond(const TDateTime AValue) { return MilliSecondOf(AValue); }
unsigned int MilliSecondOfTheMinute(const TDateTime AValue) { return SecondOf(AValue) * 1000u + MilliSecondOf(AValue); }
unsigned int MilliSecondOfTheHour(const TDateTime AValue) { return SecondOfTheHour(AValue) * 1000u + MilliSecondOf(AValue); }
unsigned int MilliSecondOfTheDay(const TDateTime AValue) { return SecondOfTheDay(AValue) * 1000u + MilliSecondOf(AValue); }
__int64 MilliSecondOfTheMonth(const TDateTime AValue) { return MilliSecondsBetween(AValue, StartOfTheMonth(AValue)); }
__int64 MilliSecondOfTheYear(const TDateTime AValue) { return MilliSecondsBetween(AValue, StartOfTheYear(AValue)); }
TDateTime StartOfTheDay(const TDateTime AValue) { return DateOf(AValue); }
//---------------------------------------------------------------------------
TDateTime EndOfTheDay(const TDateTime AValue)
{
  return FromMilliseconds(DateTimeToMilliseconds(DateOf(AValue)) + MSecsPerDay - 1);
}
//---------------------------------------------------------------------------
TDateTime StartOfTheMonth(const TDateTime AValue)
{
  Word Y, M, D;
  DecodeDate(AValue, Y, M, D);
  return EncodeDate(Y, M, 1);
}
//---------------------------------------------------------------------------
TDateTime StartOfTheYear(const TDateTime AValue)
{
  return EncodeDate(YearOf(AValue), 1, 1);
}
//---------------------------------------------------------------------------
__int64 DateTimeToMilliseconds(const TDateTime ADateTime)
{
  TTimeStamp Stamp = DateTimeToTimeStamp(ADateTime);
  return static_cast<__int64>(Stamp.Date) * MSecsPerDay + Stamp.Time;
}
//---------------------------------------------------------------------------
__int64 MilliSecondsBetween(const TDateTime ANow, const TDateTime AThen)
{
  __int64 Result = DateTimeToMilliseconds(ANow) - DateTimeToMilliseconds(AThen);
  return (Result < 0) ? -Result : Result;
}
//---------------------------------------------------------------------------
__int64 SecondsBetween(const TDateTime ANow, const TDateTime AThen) { return MilliSecondsBetween(ANow, AThen) / MSecsPerSec; }
__int64 MinutesBetween(const TDateTime ANow, const TDateTime AThen) { return MilliSecondsBetween(ANow, AThen) / (MSecsPerSec * SecsPerMin); }
__int64 HoursBetween(const TDateTime ANow, const TDateTime AThen) { return MilliSecondsBetween(ANow, AThen) / (MSecsPerSec * SecsPerHour); }
__int64 DaysBetween(const TDateTime ANow, const TDateTime AThen) { return MilliSecondsBetween(ANow, AThen) / MSecsPerDay; }
__int64 WeeksBetween(const TDateTime ANow, const TDateTime AThen) { return DaysBetween(ANow, AThen) / DaysPerWeek; }
__int64 MonthsBetween(const TDateTime ANow, const TDateTime AThen) { return static_cast<__int64>(std::trunc(MonthSpan(ANow, AThen))); }
__int64 YearsBetween(const TDateTime ANow, const TDateTime AThen) { return static_cast<__int64>(std::trunc(YearSpan(ANow, AThen))); }
double MilliSecondSpan(const TDateTime ANow, const TDateTime AThen) { return static_cast<double>(MilliSecondsBetween(ANow, AThen)); }
double SecondSpan(const TDateTime ANow, const TDateTime AThen) { return MilliSecondSpan(ANow, AThen) / MSecsPerSec; }
double MinuteSpan(const TDateTime ANow, const TDateTime AThen) { return SecondSpan(ANow, AThen) / SecsPerMin; }
double HourSpan(const TDateTime ANow, const TDateTime AThen) { return SecondSpan(ANow, AThen) / SecsPerHour; }
double DaySpan(const TDateTime ANow, const TDateTime AThen) { return MilliSecondSpan(ANow, AThen) / MSecsPerDay; }
double WeekSpan(const TDateTime ANow, const TDateTime AThen) { return DaySpan(ANow, AThen) / DaysPerWeek; }
double MonthSpan(const TDateTime ANow, const TDateTime AThen) { return DaySpan(ANow, AThen) / ApproxDaysPerMonth; }
double YearSpan(const TDateTime ANow, const TDateTime AThen) { return DaySpan(ANow, AThen) / ApproxDaysPerYear; }
bool WithinPastDays(const TDateTime ANow, const TDateTime AThen, const int ADays) { return DaysBetween(ANow, AThen) <= ADays; }
bool WithinPastHours(const TDateTime ANow, const TDateTime AThen, const __int64 AHours) { return HoursBetween(ANow, AThen) <= AHours; }
bool WithinPastMinutes(const TDateTime ANow, const TDateTime AThen, const __int64 AMinutes) { return MinutesBetween(ANow, AThen) <= AMinutes; }
bool WithinPastSeconds(const TDateTime ANow, const TDateTime AThen, const __int64 ASeconds) { return SecondsBetween(ANow, AThen) <= ASeconds; }
bool WithinPastMilliSeconds(const TDateTime ANow, const TDateTime AThen, const __int64 AMilliSeconds) { return MilliSecondsBetween(ANow, AThen) <= AMilliSeconds; }
//---------------------------------------------------------------------------
TDateTime IncMilliSecond(const TDateTime AValue, const __int64 ANumberOfMilliSeconds)
{
  return FromMilliseconds(DateTimeToMilliseconds(AValue) + ANumberOfMilliSeconds);
}
//---------------------------------------------------------------------------
TDateTime IncSecond(const TDateTime AValue, const __int64 ANumberOfSeconds) { return IncMilliSecond(AValue, ANumberOfSeconds * MSecsPerSec); }
TDateTime IncMinute(const TDateTime AValue, const __int64 ANumberOfMinutes) { return IncMilliSecond(AValue, ANumberOfMinutes * MSecsPerSec * SecsPerMin); }
TDateTime IncHour(const TDateTime AValue, const __int64 ANumberOfHours) { return IncMilliSecond(AValue, ANumberOfHours * MSecsPerSec * SecsPerHour); }
TDateTime IncDay(const TDateTime AValue, const int ANumberOfDays) { return IncMilliSecond(AValue, static_cast<__int64>(ANumberOfDays) * MSecsPerDay); }
TDateTime IncWeek(const TDateTime AValue, const int ANumberOfWeeks) { return IncDay(AValue, ANumberOfWeeks * DaysPerWeek); }
TDateTime IncYear(const TDateTime AValue, const int ANumberOfYears) { return IncMonth(AValue, ANumberOfYears * MonthsPerYear); }
//---------------------------------------------------------------------------
bool TryEncodeDateTime(const Word AYear, const Word AMonth, const Word ADay,
  const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond, TDateTime & AValue)
{
  TDateTime Date;
  TDateTime Time;
  if (!TryEncodeDate(AYear, AMonth, ADay, Date) || !TryEncodeTime(AHour, AMinute, ASecond, AMilliSecond, Time))
  {
    return false;
  }
  AValue = TDateTime(AYear, AMonth, ADay, AHour, AMinute, ASecond, AMilliSecond);
  return true;
}
//---------------------------------------------------------------------------
TDateTime EncodeDateTime(const Word AYear, const Word AMonth, const Word ADay,
  const Word AHour, const Word AMinute, const Word ASecond, const Word AMilliSecond)
{
  TDateTime Result;
  if (!TryEncodeDateTime(AYear, AMonth, ADay, AHour, AMinute, ASecond, AMilliSecond, Result))
  {
    throw EConvertError(L"Invalid argument to date encode");
  }
  return Result;
}
//---------------------------------------------------------------------------
void DecodeDateTime(const TDateTime AValue, Word & AYear, Word & AMonth, Word & ADay,
  Word & AHour, Word & AMinute, Word & ASecond, Word & AMilliSecond)
{
  DecodeDate(AValue, AYear, AMonth, ADay);
  DecodeTime(AValue, AHour, AMinute, ASecond, AMilliSecond);
}
//---------------------------------------------------------------------------
TValueRelationship CompareDateTime(const TDateTime A, const TDateTime B)
{
  if (SameDateTime(A, B))
  {
    return EqualsValue;
  }
  return (A.Val < B.Val) ? LessThanValue : GreaterThanValue;
}
//---------------------------------------------------------------------------
bool SameDateTime(const TDateTime A, const TDateTime B)
{
  return (std::fabs(A.Val - B.Val) < OneMillisecond);
}
//---------------------------------------------------------------------------
TValueRelationship CompareDate(const TDateTime A, const TDateTime B)
{
  double DA = std::trunc(A.Val);
  double DB = std::trunc(B.Val);
  return (DA == DB) ? EqualsValue : ((DA < DB) ? LessThanValue : GreaterThanValue);
}
//---------------------------------------------------------------------------
bool SameDate(const TDateTime A, const TDateTime B)
{
  return (CompareDate(A, B) == EqualsValue);
}
//---------------------------------------------------------------------------
TValueRelationship CompareTime(const TDateTime A, const TDateTime B)
{
  double TA = TimeOf(A).Val;
  double TB = TimeOf(B).Val;
  if (std::fabs(TA - TB) < OneMillisecond)
  {
    return EqualsValue;
  }
  return (TA < TB) ? LessThanValue : GreaterThanValue;
}
//---------------------------------------------------------------------------
bool SameTime(const TDateTime A, const TDateTime B)
{
  return (CompareTime(A, B) == EqualsValue);
}
//---------------------------------------------------------------------------
__int64 DateTimeToUnix(const TDateTime AValue, bool AInputIsUTC)
{
  TDateTime Utc = AInputIsUTC ? AValue : TTimeZone::Local->ToUniversalTime(AValue);
  // Truncated towards zero, as SecondsBetween
  return UnixMilliseconds(Utc) / MSecsPerSec;
}
//---------------------------------------------------------------------------
TDateTime UnixToDateTime(const __int64 AValue, bool AReturnUTC)
{
  TDateTime Result = FromUnixMilliseconds(AValue * MSecsPerSec);
  return AReturnUTC ? Result : TTimeZone::Local->ToLocalTime(Result);
}
//---------------------------------------------------------------------------
// ISO 8601
//---------------------------------------------------------------------------
namespace
{
bool ReadDigits(const UnicodeString & S, int & Pos, int Count, int & Value)
{
  Value = 0;
  for (int Index = 0; Index < Count; Index++)
  {
    if ((Pos > S.Length()) || (S[Pos] < L'0') || (S[Pos] > L'9'))
    {
      return false;
    }
    Value = Value * 10 + (S[Pos] - L'0');
    Pos++;
  }
  return true;
}

bool Peek(const UnicodeString & S, int Pos, wchar_t Ch)
{
  return (Pos <= S.Length()) && (S[Pos] == Ch);
}
} // namespace
//---------------------------------------------------------------------------
bool TryISO8601ToDate(const UnicodeString AISODate, TDateTime & Value, bool AReturnUTC)
{
  UnicodeString S = AISODate.Trim();
  int Pos = 1;
  int Year, Month = 1, Day = 1;
  if (!ReadDigits(S, Pos, 4, Year))
  {
    return false;
  }
  bool Extended = Peek(S, Pos, L'-');
  if (Extended)
  {
    Pos++;
  }
  if ((Pos <= S.Length()) && (S[Pos] != L'T') && (S[Pos] != L't') && (S[Pos] != L' '))
  {
    if (!ReadDigits(S, Pos, 2, Month))
    {
      return false;
    }
    if (Extended && Peek(S, Pos, L'-'))
    {
      Pos++;
    }
    if ((Pos <= S.Length()) && (S[Pos] >= L'0') && (S[Pos] <= L'9') && !ReadDigits(S, Pos, 2, Day))
    {
      return false;
    }
  }
  int Hour = 0, Minute = 0, Second = 0, MilliSecond = 0;
  int OffsetMinutes = 0;
  if ((Pos <= S.Length()) && ((S[Pos] == L'T') || (S[Pos] == L't') || (S[Pos] == L' ')))
  {
    Pos++;
    if (!ReadDigits(S, Pos, 2, Hour))
    {
      return false;
    }
    if (Peek(S, Pos, L':'))
    {
      Pos++;
    }
    if ((Pos <= S.Length()) && (S[Pos] >= L'0') && (S[Pos] <= L'9'))
    {
      if (!ReadDigits(S, Pos, 2, Minute))
      {
        return false;
      }
      if (Peek(S, Pos, L':'))
      {
        Pos++;
      }
      if ((Pos <= S.Length()) && (S[Pos] >= L'0') && (S[Pos] <= L'9'))
      {
        if (!ReadDigits(S, Pos, 2, Second))
        {
          return false;
        }
        if (Peek(S, Pos, L'.') || Peek(S, Pos, L','))
        {
          Pos++;
          int Scale = 100;
          while ((Pos <= S.Length()) && (S[Pos] >= L'0') && (S[Pos] <= L'9'))
          {
            MilliSecond += (S[Pos] - L'0') * Scale;
            Scale /= 10;
            Pos++;
          }
        }
      }
    }
    if (Peek(S, Pos, L'Z') || Peek(S, Pos, L'z'))
    {
      Pos++;
    }
    else if (Peek(S, Pos, L'+') || Peek(S, Pos, L'-'))
    {
      int Sign = (S[Pos] == L'-') ? -1 : 1;
      Pos++;
      int OffsetHours, OffsetMins = 0;
      if (!ReadDigits(S, Pos, 2, OffsetHours))
      {
        return false;
      }
      if (Peek(S, Pos, L':'))
      {
        Pos++;
      }
      if ((Pos <= S.Length()) && !ReadDigits(S, Pos, 2, OffsetMins))
      {
        return false;
      }
      OffsetMinutes = Sign * (OffsetHours * 60 + OffsetMins);
    }
  }
  if (Pos <= S.Length())
  {
    return false;
  }
  TDateTime Local;
  if (!TryEncodeDateTime(static_cast<Word>(Year), static_cast<Word>(Month), static_cast<Word>(Day),
        static_cast<Word>(Hour), static_cast<Word>(Minute), static_cast<Word>(Second),
        static_cast<Word>(MilliSecond), Local))
  {
    return false;
  }
  TDateTime Utc = IncMinute(Local, -OffsetMinutes);
  Value = AReturnUTC ? Utc : TTimeZone::Local->ToLocalTime(Utc);
  return true;
}
//---------------------------------------------------------------------------
TDateTime ISO8601ToDate(const UnicodeString AISODate, bool AReturnUTC)
{
  TDateTime Result;
  if (!TryISO8601ToDate(AISODate, Result, AReturnUTC))
  {
    throw EConvertError(L"'" + AISODate + L"' is not a valid ISO 8601 date");
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString DateToISO8601(const TDateTime ADate, bool AInputIsUTC)
{
  UnicodeString Result = FormatDateTime(L"yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz", ADate);
  if (AInputIsUTC)
  {
    return Result + L"Z";
  }
  int Offset = static_cast<int>(TTimeZone::Local->GetUtcOffset(ADate).TotalMinutes);
  wchar_t Sign = (Offset < 0) ? L'-' : L'+';
  Offset = std::abs(Offset);
  return Result + Sign + Format(L"%.2d:%.2d", ARRAYOFCONST((Offset / 60, Offset % 60)));
}
//---------------------------------------------------------------------------
// TTimeZone
//---------------------------------------------------------------------------
namespace
{
TTimeZone LocalZone;
} // namespace
//---------------------------------------------------------------------------
TTimeZone * const TTimeZone::Local = &LocalZone;
//---------------------------------------------------------------------------
TTimeSpan TTimeZone::GetUtcOffset(const TDateTime ADateTime, const bool)
{
  time_t Epoch;
  tm Info;
  if (!LocalToEpoch(ADateTime, Epoch, Info))
  {
    return TTimeSpan::Zero;
  }
  return TTimeSpan::FromSeconds(static_cast<double>(Info.tm_gmtoff));
}
//---------------------------------------------------------------------------
TDateTime TTimeZone::ToUniversalTime(const TDateTime ADateTime, const bool ForceDaylight)
{
  return IncSecond(ADateTime, -static_cast<__int64>(GetUtcOffset(ADateTime, ForceDaylight).TotalSeconds));
}
//---------------------------------------------------------------------------
TDateTime TTimeZone::ToLocalTime(const TDateTime ADateTime)
{
  __int64 Milliseconds = UnixMilliseconds(ADateTime);
  __int64 Seconds = Milliseconds / MSecsPerSec;
  __int64 Rest = Milliseconds % MSecsPerSec;
  if (Rest < 0)
  {
    Rest += MSecsPerSec;
    Seconds--;
  }
  return rtl::DateTimeFromUnixTime(Seconds, static_cast<long>(Rest * 1000000), true);
}
//---------------------------------------------------------------------------
bool TTimeZone::IsDaylightTime(const TDateTime ADateTime, const bool)
{
  time_t Epoch;
  tm Info;
  return LocalToEpoch(ADateTime, Epoch, Info) && (Info.tm_isdst > 0);
}
//---------------------------------------------------------------------------
bool TTimeZone::IsStandardTime(const TDateTime ADateTime, const bool ForceDaylight)
{
  return !IsDaylightTime(ADateTime, ForceDaylight);
}
//---------------------------------------------------------------------------
TLocalTimeType TTimeZone::GetLocalTimeType(const TDateTime ADateTime)
{
  return IsDaylightTime(ADateTime) ? lttDaylight : lttStandard;
}
//---------------------------------------------------------------------------
UnicodeString TTimeZone::GetAbbreviation(const TDateTime ADateTime, const bool)
{
  time_t Epoch;
  tm Info;
  return (LocalToEpoch(ADateTime, Epoch, Info) && (Info.tm_zone != nullptr)) ? UnicodeString(Info.tm_zone) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString TTimeZone::GetDisplayName(const TDateTime ADateTime, const bool ForceDaylight)
{
  return GetAbbreviation(ADateTime, ForceDaylight);
}
//---------------------------------------------------------------------------
UnicodeString TTimeZone::GetID()
{
  UnicodeString Result = GetEnvironmentVariable(L"TZ");
  if (Result.IsEmpty())
  {
    UnicodeString Target;
    if (FileGetSymLinkTarget(L"/etc/localtime", Target))
    {
      int P = Target.Pos(L"zoneinfo/");
      Result = (P > 0) ? Target.SubString(P + 9, MaxInt) : Target;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Dateutils
} // namespace System
