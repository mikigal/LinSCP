// SysUtils: dates and times, TDateTime
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <cmath>
#include <time.h>
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
System::TDateTime DateTimeFromUnixTime(long long Seconds, long Nanoseconds, bool Local)
{
  if (Local)
  {
    time_t Time = static_cast<time_t>(Seconds);
    tm Info;
    if (localtime_r(&Time, &Info) != nullptr)
    {
      Seconds += Info.tm_gmtoff;
    }
  }
  long long Days = Seconds / System::Sysutils::SecsPerDay;
  long long Rest = Seconds % System::Sysutils::SecsPerDay;
  if (Rest < 0)
  {
    Rest += System::Sysutils::SecsPerDay;
    Days--;
  }
  System::Sysutils::TTimeStamp Stamp;
  Stamp.Date = static_cast<int>(Days + System::Sysutils::UnixDateDelta + System::Sysutils::DateDelta);
  Stamp.Time = static_cast<int>(Rest * 1000 + Nanoseconds / 1000000);
  return System::Sysutils::TimeStampToDateTime(Stamp);
}
//---------------------------------------------------------------------------
} // namespace rtl
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
namespace
{
const Word MonthDays[2][12] =
{
  { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
  { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

double ComposeDateTime(double Date, double Time)
{
  return (Date < 0) ? (Date - std::fabs(Time - std::trunc(Time))) : (Date + std::fabs(Time - std::trunc(Time)));
}
} // namespace
//---------------------------------------------------------------------------
bool IsLeapYear(Word Year)
{
  return ((Year % 4) == 0) && (((Year % 100) != 0) || ((Year % 400) == 0));
}
//---------------------------------------------------------------------------
bool TryEncodeDate(Word Year, Word Month, Word Day, TDateTime & Date)
{
  if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) && (Day >= 1) &&
      (Day <= MonthDays[IsLeapYear(Year)][Month - 1]))
  {
    int Days = Day;
    for (int I = 0; I < Month - 1; I++)
    {
      Days += MonthDays[IsLeapYear(Year)][I];
    }
    int Y = Year - 1;
    Date = TDateTime(static_cast<double>(Y * 365 + Y / 4 - Y / 100 + Y / 400 + Days - DateDelta));
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
TDateTime EncodeDate(Word Year, Word Month, Word Day)
{
  TDateTime Result;
  if (!TryEncodeDate(Year, Month, Day, Result))
  {
    throw EConvertError(L"Invalid argument to date encode");
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TryEncodeTime(Word Hour, Word Min, Word Sec, Word MSec, TDateTime & Time)
{
  if ((Hour < HoursPerDay) && (Min < MinsPerHour) && (Sec < SecsPerMin) && (MSec < MSecsPerSec))
  {
    Time = TDateTime((Hour * 3600000.0 + Min * 60000.0 + Sec * 1000.0 + MSec) / MSecsPerDay);
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
TDateTime EncodeTime(Word Hour, Word Min, Word Sec, Word MSec)
{
  TDateTime Result;
  if (!TryEncodeTime(Hour, Min, Sec, MSec, Result))
  {
    throw EConvertError(L"Invalid argument to time encode");
  }
  return Result;
}
//---------------------------------------------------------------------------
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime)
{
  long long Temp = static_cast<long long>(std::nearbyint(DateTime.Val * MSecsPerDay));
  TTimeStamp Result;
  Result.Date = static_cast<int>(DateDelta + Temp / MSecsPerDay);
  Result.Time = static_cast<int>(std::llabs(Temp) % MSecsPerDay);
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TimeStampToDateTime(const TTimeStamp & TimeStamp)
{
  return TDateTime(ComposeDateTime(TimeStamp.Date - DateDelta, static_cast<double>(TimeStamp.Time) / MSecsPerDay));
}
//---------------------------------------------------------------------------
Comp TimeStampToMSecs(const TTimeStamp & TimeStamp)
{
  return static_cast<double>(TimeStamp.Date) * MSecsPerDay + TimeStamp.Time;
}
//---------------------------------------------------------------------------
TTimeStamp MSecsToTimeStamp(Comp MSecs)
{
  TTimeStamp Result;
  Result.Date = static_cast<int>(std::trunc(MSecs / MSecsPerDay));
  Result.Time = static_cast<int>(MSecs - static_cast<double>(Result.Date) * MSecsPerDay);
  return Result;
}
//---------------------------------------------------------------------------
bool DecodeDateFully(const TDateTime DateTime, Word & Year, Word & Month, Word & Day, Word & DOW)
{
  const int D1 = 365;
  const int D4 = D1 * 4 + 1;
  const int D100 = D4 * 25 - 1;
  const int D400 = D100 * 4 + 1;
  int T = DateTimeToTimeStamp(DateTime).Date;
  if (T <= 0)
  {
    Year = Month = Day = DOW = 0;
    return false;
  }
  DOW = static_cast<Word>(T % 7 + 1);
  T--;
  int Y = 1;
  while (T >= D400)
  {
    T -= D400;
    Y += 400;
  }
  int I = T / D100;
  int D = T % D100;
  if (I == 4)
  {
    I--;
    D += D100;
  }
  Y += I * 100;
  I = D / D4;
  D = D % D4;
  Y += I * 4;
  I = D / D1;
  D = D % D1;
  if (I == 4)
  {
    I--;
    D += D1;
  }
  Y += I;
  bool Leap = IsLeapYear(static_cast<Word>(Y));
  int M = 0;
  while (D >= MonthDays[Leap][M])
  {
    D -= MonthDays[Leap][M];
    M++;
  }
  Year = static_cast<Word>(Y);
  Month = static_cast<Word>(M + 1);
  Day = static_cast<Word>(D + 1);
  return Leap;
}
//---------------------------------------------------------------------------
void DecodeDate(const TDateTime DateTime, Word & Year, Word & Month, Word & Day)
{
  Word Dummy;
  DecodeDateFully(DateTime, Year, Month, Day, Dummy);
}
//---------------------------------------------------------------------------
void DecodeTime(const TDateTime DateTime, Word & Hour, Word & Min, Word & Sec, Word & MSec)
{
  int Time = DateTimeToTimeStamp(DateTime).Time;
  int MinCount = Time / (SecsPerMin * MSecsPerSec);
  int MSecCount = Time % (SecsPerMin * MSecsPerSec);
  Hour = static_cast<Word>(MinCount / MinsPerHour);
  Min = static_cast<Word>(MinCount % MinsPerHour);
  Sec = static_cast<Word>(MSecCount / MSecsPerSec);
  MSec = static_cast<Word>(MSecCount % MSecsPerSec);
}
//---------------------------------------------------------------------------
int DayOfWeek(const TDateTime DateTime)
{
  return DateTimeToTimeStamp(DateTime).Date % 7 + 1;
}
//---------------------------------------------------------------------------
TDateTime Now()
{
  timespec Time;
  clock_gettime(CLOCK_REALTIME, &Time);
  // Millisecond precision, like GetLocalTime
  return rtl::DateTimeFromUnixTime(Time.tv_sec, (Time.tv_nsec / 1000000) * 1000000, true);
}
//---------------------------------------------------------------------------
TDateTime Date()
{
  return TDateTime(std::trunc(Now().Val));
}
//---------------------------------------------------------------------------
TDateTime Time()
{
  double Value = Now().Val;
  return TDateTime(Value - std::trunc(Value));
}
//---------------------------------------------------------------------------
TDateTime GetTime()
{
  return Time();
}
//---------------------------------------------------------------------------
void ReplaceTime(TDateTime & DateTime, const TDateTime NewTime)
{
  double Date = std::trunc(DateTime.Val);
  double Time = std::fabs(NewTime.Val - std::trunc(NewTime.Val));
  DateTime = TDateTime((Date >= 0) ? (Date + Time) : (Date - Time));
}
//---------------------------------------------------------------------------
void ReplaceDate(TDateTime & DateTime, const TDateTime NewDate)
{
  TDateTime Temp = NewDate;
  ReplaceTime(Temp, DateTime);
  DateTime = Temp;
}
//---------------------------------------------------------------------------
TDateTime IncMonth(const TDateTime DateTime, int NumberOfMonths)
{
  Word Year, Month, Day;
  DecodeDate(DateTime, Year, Month, Day);
  int Sign = (NumberOfMonths >= 0) ? 1 : -1;
  int Y = Year + NumberOfMonths / 12;
  int M = Month + NumberOfMonths % 12;
  if ((M < 1) || (M > 12))
  {
    Y += Sign;
    M -= 12 * Sign;
  }
  int D = Day;
  int MaxDay = MonthDays[IsLeapYear(static_cast<Word>(Y))][M - 1];
  if (D > MaxDay)
  {
    D = MaxDay;
  }
  TDateTime Result = EncodeDate(static_cast<Word>(Y), static_cast<Word>(M), static_cast<Word>(D));
  ReplaceTime(Result, DateTime);
  return Result;
}
//---------------------------------------------------------------------------
void DateTimeToSystemTime(const TDateTime DateTime, SYSTEMTIME & SystemTime)
{
  Word DOW;
  DecodeDateFully(DateTime, SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay, DOW);
  SystemTime.wDayOfWeek = static_cast<WORD>(DOW - 1);
  DecodeTime(DateTime, SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
}
//---------------------------------------------------------------------------
TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime)
{
  TDateTime Date = EncodeDate(SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay);
  TDateTime Time = EncodeTime(SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond, SystemTime.wMilliseconds);
  return TDateTime(ComposeDateTime(Date.Val, Time.Val));
}
//---------------------------------------------------------------------------
TDateTime FileDateToDateTime(int FileDate)
{
  unsigned int Hi = static_cast<unsigned int>(FileDate) >> 16;
  unsigned int Lo = static_cast<unsigned int>(FileDate) & 0xFFFF;
  TDateTime Date = EncodeDate(static_cast<Word>((Hi >> 9) + 1980), static_cast<Word>((Hi >> 5) & 15), static_cast<Word>(Hi & 31));
  TDateTime Time = EncodeTime(static_cast<Word>(Lo >> 11), static_cast<Word>((Lo >> 5) & 63), static_cast<Word>((Lo & 31) << 1), 0);
  return TDateTime(ComposeDateTime(Date.Val, Time.Val));
}
//---------------------------------------------------------------------------
int DateTimeToFileDate(TDateTime DateTime)
{
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DecodeDate(DateTime, Year, Month, Day);
  if ((Year < 1980) || (Year > 2107))
  {
    return 0;
  }
  DecodeTime(DateTime, Hour, Min, Sec, MSec);
  unsigned int Hi = ((Year - 1980) << 9) | (Month << 5) | Day;
  unsigned int Lo = (Hour << 11) | (Min << 5) | (Sec >> 1);
  return static_cast<int>((Hi << 16) | Lo);
}
//---------------------------------------------------------------------------
// FormatDateTime
//---------------------------------------------------------------------------
namespace
{
UnicodeString PadNumber(int Value, int Digits)
{
  UnicodeString Result = IntToStr(Value);
  while (Result.Length() < Digits)
  {
    Result = L"0" + Result;
  }
  return Result;
}

bool StartsWithText(const UnicodeString & Format, int I, const wchar_t * Text)
{
  int Len = static_cast<int>(wcslen(Text));
  return SameText(Format.SubString(I, Len), Text);
}

// Whether the format uses a 12-hour clock (am/pm, a/p or ampm outside of quotes)
bool Uses12HourClock(const UnicodeString & Format)
{
  wchar_t Quote = L'\0';
  for (int I = 1; I <= Format.Length(); I++)
  {
    wchar_t Ch = Format[I];
    if (Quote != L'\0')
    {
      if (Ch == Quote)
      {
        Quote = L'\0';
      }
    }
    else if ((Ch == L'\'') || (Ch == L'"'))
    {
      Quote = Ch;
    }
    else if (StartsWithText(Format, I, L"am/pm") || StartsWithText(Format, I, L"a/p") || StartsWithText(Format, I, L"ampm"))
    {
      return true;
    }
  }
  return false;
}

void AppendFormat(UnicodeString & Result, const UnicodeString & Format, TDateTime DateTime,
  const TFormatSettings & Settings, int Level)
{
  if (Level > 2)
  {
    return;
  }
  Word Year, Month, Day, DOW;
  DecodeDateFully(DateTime, Year, Month, Day, DOW);
  Word Hour, Min, Sec, MSec;
  DecodeTime(DateTime, Hour, Min, Sec, MSec);
  bool Clock12 = Uses12HourClock(Format);

  wchar_t LastToken = L' ';
  int Len = Format.Length();
  int I = 1;
  while (I <= Len)
  {
    wchar_t Starter = Format[I];
    wchar_t Token = rtl::UpperChar(Starter);
    if ((Token >= L'A') && (Token <= L'Z'))
    {
      if ((Token == L'M') && (LastToken == L'H'))
      {
        Token = L'N';
      }
      LastToken = Token;
    }
    // Number of repetitions of the same (case-insensitive) letter
    auto Count = [&]()
      {
        int Start = I;
        while ((I <= Len) && (rtl::UpperChar(Format[I]) == rtl::UpperChar(Starter)))
        {
          I++;
        }
        return I - Start;
      };

    switch (Token)
    {
      case L'Y':
        {
          int N = Count();
          Result += (N <= 2) ? PadNumber(Year % 100, 2) : PadNumber(Year, 4);
        }
        break;

      case L'M':
        {
          int N = Count();
          switch (N)
          {
            case 1: Result += IntToStr(Month); break;
            case 2: Result += PadNumber(Month, 2); break;
            case 3: Result += Settings.ShortMonthNames[Month - 1]; break;
            default: Result += Settings.LongMonthNames[Month - 1]; break;
          }
        }
        break;

      case L'D':
        {
          int N = Count();
          switch (N)
          {
            case 1: Result += IntToStr(Day); break;
            case 2: Result += PadNumber(Day, 2); break;
            case 3: Result += Settings.ShortDayNames[DOW - 1]; break;
            case 4: Result += Settings.LongDayNames[DOW - 1]; break;
            case 5: AppendFormat(Result, Settings.ShortDateFormat, DateTime, Settings, Level + 1); break;
            default: AppendFormat(Result, Settings.LongDateFormat, DateTime, Settings, Level + 1); break;
          }
        }
        break;

      case L'E':
      case L'G':
        {
          // Eras are not supported; the Gregorian calendar year is used
          int N = Count();
          if (Token == L'E')
          {
            Result += (N <= 1) ? IntToStr(Year) : PadNumber(Year, N);
          }
        }
        break;

      case L'H':
        {
          int N = Count();
          int H = Hour;
          if (Clock12)
          {
            H = H % 12;
            if (H == 0)
            {
              H = 12;
            }
          }
          Result += (N <= 1) ? IntToStr(H) : PadNumber(H, 2);
        }
        break;

      case L'N':
        {
          int N = Count();
          Result += (N <= 1) ? IntToStr(Min) : PadNumber(Min, 2);
        }
        break;

      case L'S':
        {
          int N = Count();
          Result += (N <= 1) ? IntToStr(Sec) : PadNumber(Sec, 2);
        }
        break;

      case L'Z':
        {
          int N = Count();
          Result += (N <= 1) ? IntToStr(MSec) : PadNumber(MSec, 3);
        }
        break;

      case L'T':
        {
          int N = Count();
          AppendFormat(Result, (N <= 1) ? Settings.ShortTimeFormat : Settings.LongTimeFormat, DateTime, Settings, Level + 1);
        }
        break;

      case L'C':
        Count();
        AppendFormat(Result, Settings.ShortDateFormat, DateTime, Settings, Level + 1);
        if ((Hour != 0) || (Min != 0) || (Sec != 0) || (MSec != 0))
        {
          Result += L' ';
          AppendFormat(Result, Settings.LongTimeFormat, DateTime, Settings, Level + 1);
        }
        break;

      case L'A':
        if (StartsWithText(Format, I, L"am/pm"))
        {
          bool Upper = (Format[I] == L'A');
          Result += (Hour >= 12) ? (Upper ? L"PM" : L"pm") : (Upper ? L"AM" : L"am");
          I += 5;
        }
        else if (StartsWithText(Format, I, L"a/p"))
        {
          bool Upper = (Format[I] == L'A');
          Result += (Hour >= 12) ? (Upper ? L"P" : L"p") : (Upper ? L"A" : L"a");
          I += 3;
        }
        else if (StartsWithText(Format, I, L"ampm"))
        {
          Result += (Hour >= 12) ? Settings.TimePMString : Settings.TimeAMString;
          I += 4;
        }
        else
        {
          Result += Starter;
          I++;
        }
        break;

      case L'/':
        if (Settings.DateSeparator != L'\0')
        {
          Result += Settings.DateSeparator;
        }
        I++;
        break;

      case L':':
        if (Settings.TimeSeparator != L'\0')
        {
          Result += Settings.TimeSeparator;
        }
        I++;
        break;

      case L'\'':
      case L'"':
        {
          int Start = I + 1;
          I = Start;
          while ((I <= Len) && (Format[I] != Starter))
          {
            I++;
          }
          Result += Format.SubString(Start, I - Start);
          if (I <= Len)
          {
            I++;
          }
        }
        break;

      default:
        Result += Starter;
        I++;
        break;
    }
  }
}
} // namespace
//---------------------------------------------------------------------------
void DateTimeToString(UnicodeString & Result, const UnicodeString Format, TDateTime DateTime,
  const TFormatSettings & AFormatSettings)
{
  Result = UnicodeString();
  AppendFormat(Result, Format.IsEmpty() ? UnicodeString(L"C") : Format, DateTime, AFormatSettings, 0);
}
//---------------------------------------------------------------------------
void DateTimeToString(UnicodeString & Result, const UnicodeString Format, TDateTime DateTime)
{
  DateTimeToString(Result, Format, DateTime, FormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString FormatDateTime(const UnicodeString Format, TDateTime DateTime, const TFormatSettings & AFormatSettings)
{
  UnicodeString Result;
  DateTimeToString(Result, Format, DateTime, AFormatSettings);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString FormatDateTime(const UnicodeString Format, TDateTime DateTime)
{
  return FormatDateTime(Format, DateTime, FormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString DateToStr(const TDateTime DateTime, const TFormatSettings & AFormatSettings)
{
  return FormatDateTime(AFormatSettings.ShortDateFormat, DateTime, AFormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString DateToStr(const TDateTime DateTime)
{
  return DateToStr(DateTime, FormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString TimeToStr(const TDateTime DateTime, const TFormatSettings & AFormatSettings)
{
  return FormatDateTime(AFormatSettings.LongTimeFormat, DateTime, AFormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString TimeToStr(const TDateTime DateTime)
{
  return TimeToStr(DateTime, FormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString DateTimeToStr(const TDateTime DateTime, const TFormatSettings & AFormatSettings)
{
  return FormatDateTime(UnicodeString(), DateTime, AFormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString DateTimeToStr(const TDateTime DateTime)
{
  return DateTimeToStr(DateTime, FormatSettings);
}
//---------------------------------------------------------------------------
// Parsing
//---------------------------------------------------------------------------
namespace
{
enum TDateOrder { doMDY, doDMY, doYMD };

TDateOrder GetDateOrder(const UnicodeString & DateFormat)
{
  for (int I = 1; I <= DateFormat.Length(); I++)
  {
    switch (rtl::UpperChar(DateFormat[I]))
    {
      case L'E':
      case L'Y': return doYMD;
      case L'M': return doMDY;
      case L'D': return doDMY;
    }
  }
  return doMDY;
}

void ScanBlanks(const UnicodeString & S, int & Pos)
{
  while ((Pos <= S.Length()) && (S[Pos] == L' '))
  {
    Pos++;
  }
}

bool ScanNumber(const UnicodeString & S, int & Pos, int & Number, int & Digits)
{
  ScanBlanks(S, Pos);
  int Start = Pos;
  Number = 0;
  while ((Pos <= S.Length()) && (S[Pos] >= L'0') && (S[Pos] <= L'9') && (Number < 1000000))
  {
    Number = Number * 10 + (S[Pos] - L'0');
    Pos++;
  }
  Digits = Pos - Start;
  return (Digits > 0);
}

bool ScanChar(const UnicodeString & S, int & Pos, wchar_t Ch)
{
  ScanBlanks(S, Pos);
  if ((Pos <= S.Length()) && (S[Pos] == Ch))
  {
    Pos++;
    return true;
  }
  return false;
}

bool ScanString(const UnicodeString & S, int & Pos, const UnicodeString & Symbol)
{
  if (!Symbol.IsEmpty())
  {
    ScanBlanks(S, Pos);
    if (AnsiSameText(Symbol, S.SubString(Pos, Symbol.Length())))
    {
      Pos += Symbol.Length();
      return true;
    }
  }
  return false;
}

bool ScanDate(const UnicodeString & S, int & Pos, TDateTime & Date, const TFormatSettings & Settings)
{
  TDateOrder Order = GetDateOrder(Settings.ShortDateFormat);
  int N1, N2, N3 = 0;
  int L1, L2, L3 = 0;
  int Y, M, D;
  int YearDigits = 0;
  if (!(ScanNumber(S, Pos, N1, L1) && ScanChar(S, Pos, Settings.DateSeparator) && ScanNumber(S, Pos, N2, L2)))
  {
    return false;
  }
  if (ScanChar(S, Pos, Settings.DateSeparator))
  {
    if (!ScanNumber(S, Pos, N3, L3))
    {
      return false;
    }
    switch (Order)
    {
      case doMDY: Y = N3; YearDigits = L3; M = N1; D = N2; break;
      case doDMY: Y = N3; YearDigits = L3; M = N2; D = N1; break;
      default: Y = N1; YearDigits = L1; M = N2; D = N3; break;
    }
    if ((YearDigits <= 2) && (Y <= 99))
    {
      Word CurrentYear, Dummy1, Dummy2;
      DecodeDate(Sysutils::Date(), CurrentYear, Dummy1, Dummy2);
      int Century = CurrentYear - Settings.TwoDigitYearCenturyWindow;
      Y += Century / 100 * 100;
      if ((Settings.TwoDigitYearCenturyWindow > 0) && (Y < Century))
      {
        Y += 100;
      }
    }
  }
  else
  {
    Word CurrentYear, Dummy1, Dummy2;
    DecodeDate(Sysutils::Date(), CurrentYear, Dummy1, Dummy2);
    Y = CurrentYear;
    if (Order == doDMY)
    {
      D = N1;
      M = N2;
    }
    else
    {
      M = N1;
      D = N2;
    }
  }
  ScanChar(S, Pos, Settings.DateSeparator);
  if ((Y < 0) || (Y > 9999) || (M < 0) || (M > 12) || (D < 0) || (D > 31))
  {
    return false;
  }
  return TryEncodeDate(static_cast<Word>(Y), static_cast<Word>(M), static_cast<Word>(D), Date);
}

bool ScanTime(const UnicodeString & S, int & Pos, TDateTime & Time, const TFormatSettings & Settings)
{
  int Hour, Min = 0, Sec = 0, MSec = 0;
  int Digits;
  int AmPm = 0;
  if (ScanString(S, Pos, Settings.TimeAMString) || ScanString(S, Pos, L"AM"))
  {
    AmPm = -1;
  }
  else if (ScanString(S, Pos, Settings.TimePMString) || ScanString(S, Pos, L"PM"))
  {
    AmPm = 1;
  }
  if (!ScanNumber(S, Pos, Hour, Digits))
  {
    return false;
  }
  if (ScanChar(S, Pos, Settings.TimeSeparator))
  {
    if (!ScanNumber(S, Pos, Min, Digits))
    {
      return false;
    }
    if (ScanChar(S, Pos, Settings.TimeSeparator))
    {
      if (!ScanNumber(S, Pos, Sec, Digits))
      {
        return false;
      }
      if (ScanChar(S, Pos, Settings.DecimalSeparator))
      {
        if (!ScanNumber(S, Pos, MSec, Digits))
        {
          return false;
        }
      }
    }
  }
  if (AmPm == 0)
  {
    if (ScanString(S, Pos, Settings.TimeAMString) || ScanString(S, Pos, L"AM"))
    {
      AmPm = -1;
    }
    else if (ScanString(S, Pos, Settings.TimePMString) || ScanString(S, Pos, L"PM"))
    {
      AmPm = 1;
    }
  }
  if (AmPm != 0)
  {
    if ((Hour == 0) || (Hour > 12))
    {
      return false;
    }
    if (AmPm < 0)
    {
      if (Hour == 12)
      {
        Hour = 0;
      }
    }
    else if (Hour < 12)
    {
      Hour += 12;
    }
  }
  if ((Hour > 23) || (Min > 59) || (Sec > 59) || (MSec > 999))
  {
    return false;
  }
  return TryEncodeTime(static_cast<Word>(Hour), static_cast<Word>(Min), static_cast<Word>(Sec), static_cast<Word>(MSec), Time);
}
} // namespace
//---------------------------------------------------------------------------
bool TryStrToDate(const UnicodeString S, TDateTime & Value, const TFormatSettings & AFormatSettings)
{
  int Pos = 1;
  TDateTime Date;
  if (ScanDate(S, Pos, Date, AFormatSettings))
  {
    ScanBlanks(S, Pos);
    if (Pos > S.Length())
    {
      Value = Date;
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
bool TryStrToDate(const UnicodeString S, TDateTime & Value)
{
  return TryStrToDate(S, Value, FormatSettings);
}
//---------------------------------------------------------------------------
bool TryStrToTime(const UnicodeString S, TDateTime & Value, const TFormatSettings & AFormatSettings)
{
  int Pos = 1;
  TDateTime Time;
  if (ScanTime(S, Pos, Time, AFormatSettings))
  {
    ScanBlanks(S, Pos);
    if (Pos > S.Length())
    {
      Value = Time;
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
bool TryStrToTime(const UnicodeString S, TDateTime & Value)
{
  return TryStrToTime(S, Value, FormatSettings);
}
//---------------------------------------------------------------------------
bool TryStrToDateTime(const UnicodeString S, TDateTime & Value, const TFormatSettings & AFormatSettings)
{
  int Pos = 1;
  TDateTime Date;
  if (!ScanDate(S, Pos, Date, AFormatSettings))
  {
    return TryStrToTime(S, Value, AFormatSettings);
  }
  ScanBlanks(S, Pos);
  TDateTime Time;
  if (Pos <= S.Length())
  {
    if (!ScanTime(S, Pos, Time, AFormatSettings))
    {
      return false;
    }
    ScanBlanks(S, Pos);
    if (Pos <= S.Length())
    {
      return false;
    }
  }
  Value = TDateTime(ComposeDateTime(Date.Val, Time.Val));
  return true;
}
//---------------------------------------------------------------------------
bool TryStrToDateTime(const UnicodeString S, TDateTime & Value)
{
  return TryStrToDateTime(S, Value, FormatSettings);
}
//---------------------------------------------------------------------------
TDateTime StrToDate(const UnicodeString S, const TFormatSettings & AFormatSettings)
{
  TDateTime Result;
  if (!TryStrToDate(S, Result, AFormatSettings))
  {
    throw EConvertError(L"'" + S + L"' is not a valid date");
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime StrToDate(const UnicodeString S)
{
  return StrToDate(S, FormatSettings);
}
//---------------------------------------------------------------------------
TDateTime StrToTime(const UnicodeString S, const TFormatSettings & AFormatSettings)
{
  TDateTime Result;
  if (!TryStrToTime(S, Result, AFormatSettings))
  {
    throw EConvertError(L"'" + S + L"' is not a valid time");
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime StrToTime(const UnicodeString S)
{
  return StrToTime(S, FormatSettings);
}
//---------------------------------------------------------------------------
TDateTime StrToDateTime(const UnicodeString S, const TFormatSettings & AFormatSettings)
{
  TDateTime Result;
  if (!TryStrToDateTime(S, Result, AFormatSettings))
  {
    throw EConvertError(L"'" + S + L"' is not a valid date and time");
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime StrToDateTime(const UnicodeString S)
{
  return StrToDateTime(S, FormatSettings);
}
//---------------------------------------------------------------------------
} // namespace Sysutils
//---------------------------------------------------------------------------
// TDateTime
//---------------------------------------------------------------------------
TDateTime TDateTime::CurrentDate() { return Sysutils::Date(); }
TDateTime TDateTime::CurrentTime() { return Sysutils::Time(); }
TDateTime TDateTime::CurrentDateTime() { return Sysutils::Now(); }
TDateTime TDateTime::FileDateToDateTime(int FileDate) { return Sysutils::FileDateToDateTime(FileDate); }
//---------------------------------------------------------------------------
TDateTime::TDateTime(const UnicodeString & Src, TDateTimeFlag Flag)
{
  switch (Flag)
  {
    case Date: Val = Sysutils::StrToDate(Src).Val; break;
    case Time: Val = Sysutils::StrToTime(Src).Val; break;
    default: Val = Sysutils::StrToDateTime(Src).Val; break;
  }
}
//---------------------------------------------------------------------------
TDateTime::TDateTime(unsigned short Year, unsigned short Month, unsigned short Day)
{
  Val = Sysutils::EncodeDate(Year, Month, Day).Val;
}
//---------------------------------------------------------------------------
TDateTime::TDateTime(unsigned short Hour, unsigned short Min, unsigned short Sec, unsigned short MSec)
{
  Val = Sysutils::EncodeTime(Hour, Min, Sec, MSec).Val;
}
//---------------------------------------------------------------------------
TDateTime::TDateTime(unsigned short Year, unsigned short Month, unsigned short Day,
  unsigned short Hour, unsigned short Min, unsigned short Sec, unsigned short MSec)
{
  Val = Sysutils::ComposeDateTime(Sysutils::EncodeDate(Year, Month, Day).Val, Sysutils::EncodeTime(Hour, Min, Sec, MSec).Val);
}
//---------------------------------------------------------------------------
TDateTime::operator UnicodeString() const
{
  return DateTimeString();
}
//---------------------------------------------------------------------------
UnicodeString TDateTime::FormatString(const UnicodeString & Format) const
{
  return Sysutils::FormatDateTime(Format, *this);
}
//---------------------------------------------------------------------------
UnicodeString TDateTime::DateString() const
{
  return Sysutils::DateToStr(*this);
}
//---------------------------------------------------------------------------
UnicodeString TDateTime::TimeString() const
{
  return Sysutils::TimeToStr(*this);
}
//---------------------------------------------------------------------------
UnicodeString TDateTime::DateTimeString() const
{
  return Sysutils::DateTimeToStr(*this);
}
//---------------------------------------------------------------------------
int TDateTime::DayOfWeek() const
{
  return Sysutils::DayOfWeek(*this);
}
//---------------------------------------------------------------------------
int TDateTime::FileDate() const
{
  return Sysutils::DateTimeToFileDate(*this);
}
//---------------------------------------------------------------------------
void TDateTime::DecodeDate(unsigned short * Year, unsigned short * Month, unsigned short * Day) const
{
  Sysutils::DecodeDate(*this, *Year, *Month, *Day);
}
//---------------------------------------------------------------------------
void TDateTime::DecodeTime(unsigned short * Hour, unsigned short * Min, unsigned short * Sec, unsigned short * MSec) const
{
  Sysutils::DecodeTime(*this, *Hour, *Min, *Sec, *MSec);
}
//---------------------------------------------------------------------------
} // namespace System
