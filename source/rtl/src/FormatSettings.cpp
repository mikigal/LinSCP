// SysUtils: TFormatSettings
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <langinfo.h>
#include <locale.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
namespace
{
const wchar_t * EnglishShortMonthNames[12] =
  { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };
const wchar_t * EnglishLongMonthNames[12] =
  { L"January", L"February", L"March", L"April", L"May", L"June", L"July", L"August", L"September",
    L"October", L"November", L"December" };
const wchar_t * EnglishShortDayNames[7] = { L"Sun", L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat" };
const wchar_t * EnglishLongDayNames[7] =
  { L"Sunday", L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday" };

void SetEnglishNames(TFormatSettings & Settings)
{
  for (int Index = 0; Index < 12; Index++)
  {
    Settings.ShortMonthNames[Index] = EnglishShortMonthNames[Index];
    Settings.LongMonthNames[Index] = EnglishLongMonthNames[Index];
  }
  for (int Index = 0; Index < 7; Index++)
  {
    Settings.ShortDayNames[Index] = EnglishShortDayNames[Index];
    Settings.LongDayNames[Index] = EnglishLongDayNames[Index];
  }
}

TFormatSettings EnglishUS()
{
  TFormatSettings Result = TFormatSettings::Invariant();
  Result.CurrencyString = L"$";
  Result.ShortDateFormat = L"M/d/yyyy";
  Result.LongDateFormat = L"dddd, MMMM d, yyyy";
  Result.ShortTimeFormat = L"h:mm AMPM";
  Result.LongTimeFormat = L"h:mm:ss AMPM";
  return Result;
}

UnicodeString LangInfo(nl_item Item, locale_t Locale)
{
  const char * Value = nl_langinfo_l(Item, Locale);
  return (Value != nullptr) ? UnicodeString(rtl::DecodeUtf8(Value, strlen(Value))) : UnicodeString();
}

// Converts an strftime() format to a Delphi date/time format. The first
// separator becomes Separator and is written as Placeholder ('/' or ':').
UnicodeString ConvertFormat(const UnicodeString & Format, wchar_t & Separator, wchar_t Placeholder)
{
  UnicodeString Result;
  Separator = L'\0';
  for (int I = 1; I <= Format.Length(); I++)
  {
    wchar_t Ch = Format[I];
    if ((Ch == L'%') && (I < Format.Length()))
    {
      I++;
      wchar_t Conv = Format[I];
      // Skip glibc flags like %-d or %Ey
      while (((Conv == L'-') || (Conv == L'_') || (Conv == L'0') || (Conv == L'E') || (Conv == L'O')) && (I < Format.Length()))
      {
        I++;
        Conv = Format[I];
      }
      switch (Conv)
      {
        case L'd': Result += L"dd"; break;
        case L'e': Result += L"d"; break;
        case L'm': Result += L"mm"; break;
        case L'y': Result += L"yy"; break;
        case L'Y': Result += L"yyyy"; break;
        case L'b':
        case L'h': Result += L"mmm"; break;
        case L'B': Result += L"mmmm"; break;
        case L'a': Result += L"ddd"; break;
        case L'A': Result += L"dddd"; break;
        case L'H': Result += L"hh"; break;
        case L'k': Result += L"h"; break;
        case L'I': Result += L"hh"; break;
        case L'l': Result += L"h"; break;
        case L'M': Result += L"nn"; break;
        case L'S': Result += L"ss"; break;
        case L'p':
        case L'P': Result += L"AMPM"; break;
        case L'D': Result += L"mm/dd/yy"; if (Separator == L'\0') Separator = L'/'; break;
        case L'F': Result += L"yyyy/mm/dd"; if (Separator == L'\0') Separator = L'-'; break;
        case L'T': Result += L"hh:nn:ss"; if (Separator == L'\0') Separator = L':'; break;
        case L'R': Result += L"hh:nn"; if (Separator == L'\0') Separator = L':'; break;
        case L'r': Result += L"hh:nn:ss AMPM"; if (Separator == L'\0') Separator = L':'; break;
        case L'n': Result += L" "; break;
        case L't': Result += L" "; break;
        case L'%': Result += L"'%'"; break;
        default: break;
      }
    }
    else if ((Ch == L' ') || (Ch == L','))
    {
      Result += Ch;
    }
    else if (((Ch == L'.') || (Ch == L'/') || (Ch == L'-') || (Ch == L':')) && ((Separator == L'\0') || (Separator == Ch)))
    {
      Separator = Ch;
      Result += Placeholder;
    }
    else
    {
      Result += L"'" + UnicodeString(Ch) + L"'";
    }
  }
  return Result;
}

UnicodeString WithoutSeconds(const UnicodeString & TimeFormat)
{
  UnicodeString Result = TimeFormat;
  int P = Result.Pos(L":ss");
  if (P > 0)
  {
    Result.Delete(P, 3);
  }
  return Result;
}

TFormatSettings FromLocale(locale_t Locale)
{
  TFormatSettings Result = TFormatSettings::Invariant();
  wchar_t Separator;
  UnicodeString DateFormat = ConvertFormat(LangInfo(D_FMT, Locale), Separator, L'/');
  if (!DateFormat.IsEmpty())
  {
    Result.ShortDateFormat = DateFormat;
    Result.LongDateFormat = DateFormat;
    if (Separator != L'\0')
    {
      Result.DateSeparator = Separator;
    }
  }
  UnicodeString TimeFormat = ConvertFormat(LangInfo(T_FMT, Locale), Separator, L':');
  if (!TimeFormat.IsEmpty())
  {
    Result.LongTimeFormat = TimeFormat;
    Result.ShortTimeFormat = WithoutSeconds(TimeFormat);
    if (Separator != L'\0')
    {
      Result.TimeSeparator = Separator;
    }
  }
  UnicodeString AM = LangInfo(AM_STR, Locale);
  UnicodeString PM = LangInfo(PM_STR, Locale);
  if (!AM.IsEmpty() && !PM.IsEmpty())
  {
    Result.TimeAMString = AM;
    Result.TimePMString = PM;
  }
  for (int Index = 0; Index < 12; Index++)
  {
    UnicodeString Short = LangInfo(ABMON_1 + Index, Locale);
#ifdef ALTMON_1
    // Nominative forms (MON_x are genitive in some languages), as Windows' LOCALE_SMONTHNAMEx
    UnicodeString Long = LangInfo(ALTMON_1 + Index, Locale);
    if (Long.IsEmpty())
    {
      Long = LangInfo(MON_1 + Index, Locale);
    }
#else
    UnicodeString Long = LangInfo(MON_1 + Index, Locale);
#endif
    if (!Short.IsEmpty() && !Long.IsEmpty())
    {
      Result.ShortMonthNames[Index] = Short;
      Result.LongMonthNames[Index] = Long;
    }
  }
  for (int Index = 0; Index < 7; Index++)
  {
    UnicodeString Short = LangInfo(ABDAY_1 + Index, Locale);
    UnicodeString Long = LangInfo(DAY_1 + Index, Locale);
    if (!Short.IsEmpty() && !Long.IsEmpty())
    {
      Result.ShortDayNames[Index] = Short;
      Result.LongDayNames[Index] = Long;
    }
  }
  UnicodeString Decimal = LangInfo(RADIXCHAR, Locale);
  if (Decimal.Length() == 1)
  {
    Result.DecimalSeparator = Decimal[1];
  }
  UnicodeString Thousands = LangInfo(THOUSEP, Locale);
  if (Thousands.Length() == 1)
  {
    Result.ThousandSeparator = Thousands[1];
  }
  else if (Result.DecimalSeparator == L',')
  {
    Result.ThousandSeparator = L'.';
  }
  // "-" prefix: before the amount, "+": after it, ".": replaces the radix
  UnicodeString Currency = LangInfo(CRNCYSTR, Locale);
  if (Currency.Length() > 1)
  {
    Result.CurrencyString = Currency.SubString(2, MaxInt);
    Result.CurrencyFormat = (Currency[1] == L'+') ? 3 : 0;
  }
  return Result;
}

bool IsPosixLocale(const char * Name)
{
  return (Name == nullptr) || (strcmp(Name, "C") == 0) || (strcmp(Name, "POSIX") == 0) ||
    (strncmp(Name, "C.", 2) == 0);
}
} // namespace
//---------------------------------------------------------------------------
TFormatSettings TFormatSettings::Invariant()
{
  TFormatSettings Result;
  Result.CurrencyFormat = 0;
  Result.NegCurrFormat = 0;
  Result.ThousandSeparator = L',';
  Result.DecimalSeparator = L'.';
  Result.CurrencyDecimals = 2;
  Result.DateSeparator = L'/';
  Result.TimeSeparator = L':';
  Result.ListSeparator = L',';
  Result.CurrencyString = L"\u00A4";
  Result.ShortDateFormat = L"MM/dd/yyyy";
  Result.LongDateFormat = L"dddd, dd MMMMM yyyy HH:mm:ss";
  Result.TimeAMString = L"AM";
  Result.TimePMString = L"PM";
  Result.ShortTimeFormat = L"HH:mm";
  Result.LongTimeFormat = L"HH:mm:ss";
  SetEnglishNames(Result);
  Result.TwoDigitYearCenturyWindow = 50;
  return Result;
}
//---------------------------------------------------------------------------
TFormatSettings TFormatSettings::Create(const UnicodeString & LocaleName)
{
  std::string Name = rtl::EncodeUtf8(LocaleName.c_str(), LocaleName.Length());
  locale_t Locale = newlocale(LC_ALL_MASK, Name.c_str(), static_cast<locale_t>(0));
  if (Locale == static_cast<locale_t>(0))
  {
    return Invariant();
  }
  const char * TimeName = (Name.empty() ? setlocale(LC_TIME, nullptr) : Name.c_str());
  if (Name.empty())
  {
    // The locale of the environment, as the process would get it
    const char * Env = getenv("LC_ALL");
    if ((Env == nullptr) || (*Env == '\0'))
    {
      Env = getenv("LC_TIME");
    }
    if ((Env == nullptr) || (*Env == '\0'))
    {
      Env = getenv("LANG");
    }
    TimeName = ((Env != nullptr) && (*Env != '\0')) ? Env : "C";
  }
  TFormatSettings Result = IsPosixLocale(TimeName) ? Invariant() : FromLocale(Locale);
  freelocale(Locale);
  return Result;
}
//---------------------------------------------------------------------------
TFormatSettings TFormatSettings::Create()
{
  return Create(UnicodeString());
}
//---------------------------------------------------------------------------
TFormatSettings TFormatSettings::Create(LCID Locale)
{
  switch (Locale)
  {
    case 1033: return EnglishUS();
    // LOCALE_INVARIANT
    case 127: return Invariant();
  }
  return Create();
}
//---------------------------------------------------------------------------
TFormatSettings FormatSettings __attribute__((init_priority(200))) = TFormatSettings::Create();
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
