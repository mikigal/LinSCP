// Format() and floating point conversions with Delphi semantics
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
#include <rtl/Variants.h>
#include <rtl/Printf.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
namespace
{
// Significant digits of Extended, as in Delphi
const int MaxPrecision = 18;

struct TFloatRec
{
  // Value = 0.Digits * 10^Exponent; zero has empty Digits
  int Exponent = 0;
  bool Negative = false;
  bool Nan = false;
  bool Infinite = false;
  std::string Digits;
};

// Delphi's FloatToDecimal: at most Precision significant digits and at most
// Decimals digits after the decimal point, rounded half away from zero.
TFloatRec FloatToDecimal(long double Value, int Precision, int Decimals)
{
  TFloatRec Result;
  Result.Negative = std::signbit(Value);
  if (std::isnan(Value))
  {
    Result.Nan = true;
    return Result;
  }
  if (std::isinf(Value))
  {
    Result.Infinite = true;
    return Result;
  }
  if (Value == 0)
  {
    return Result;
  }

  char Buffer[64];
  rtl::WithCLocale([&]()
    {
      return snprintf(Buffer, sizeof(Buffer), "%.*Le", MaxPrecision - 1, std::fabs(Value));
    });
  // d.ddddde[+-]x
  std::string Digits;
  const char * P = Buffer;
  while (*P != 'e')
  {
    if ((*P >= '0') && (*P <= '9'))
    {
      Digits.push_back(*P);
    }
    P++;
  }
  int Exponent = atoi(P + 1) + 1;

  if (Precision > MaxPrecision)
  {
    Precision = MaxPrecision;
  }
  long long Keep = Precision;
  if ((Decimals < 9999) && (static_cast<long long>(Exponent) + Decimals < Keep))
  {
    Keep = static_cast<long long>(Exponent) + Decimals;
  }
  if (Keep < 0)
  {
    return TFloatRec{0, Result.Negative, false, false, std::string()};
  }

  bool RoundUp = (Keep < static_cast<long long>(Digits.size())) && (Digits[Keep] >= '5');
  Digits.resize(Keep);
  if (RoundUp)
  {
    int Index = static_cast<int>(Keep) - 1;
    while ((Index >= 0) && (Digits[Index] == '9'))
    {
      Digits[Index] = '0';
      Index--;
    }
    if (Index >= 0)
    {
      Digits[Index]++;
    }
    else
    {
      // All digits were nines (or none were kept): the rest are zeros now
      Digits.insert(Digits.begin(), '1');
      Exponent++;
    }
  }
  while (!Digits.empty() && (Digits.back() == '0'))
  {
    Digits.pop_back();
  }
  Result.Digits = Digits;
  Result.Exponent = Digits.empty() ? 0 : Exponent;
  return Result;
}

UnicodeString SpecialValue(const TFloatRec & Rec)
{
  if (Rec.Nan)
  {
    return L"NAN";
  }
  return Rec.Negative ? L"-INF" : L"INF";
}

wchar_t DigitAt(const TFloatRec & Rec, int Index)
{
  return ((Index >= 0) && (Index < static_cast<int>(Rec.Digits.size()))) ? static_cast<wchar_t>(Rec.Digits[Index]) : L'0';
}

UnicodeString ExponentText(int Exponent, int MinDigits, bool PlusSign)
{
  UnicodeString Result = L"E";
  if (Exponent < 0)
  {
    Result += L'-';
    Exponent = -Exponent;
  }
  else if (PlusSign)
  {
    Result += L'+';
  }
  UnicodeString Number = IntToStr(Exponent);
  if (MinDigits > 4)
  {
    MinDigits = 4;
  }
  while (Number.Length() < MinDigits)
  {
    Number = L"0" + Number;
  }
  return Result + Number;
}

// Integer part and Digits decimals of a value rounded by FloatToDecimal
UnicodeString FixedText(const TFloatRec & Rec, int Digits, bool Thousands, const TFormatSettings & Settings)
{
  UnicodeString IntPart;
  if (Rec.Exponent <= 0)
  {
    IntPart = L"0";
  }
  else
  {
    for (int Index = 0; Index < Rec.Exponent; Index++)
    {
      IntPart += DigitAt(Rec, Index);
    }
  }
  if (Thousands && (Settings.ThousandSeparator != L'\0'))
  {
    UnicodeString Grouped;
    int Count = 0;
    for (int Index = IntPart.Length(); Index >= 1; Index--)
    {
      if ((Count > 0) && ((Count % 3) == 0))
      {
        Grouped.Insert(UnicodeString(Settings.ThousandSeparator), 1);
      }
      Grouped.Insert(UnicodeString(IntPart[Index]), 1);
      Count++;
    }
    IntPart = Grouped;
  }
  UnicodeString Result = IntPart;
  if (Digits > 0)
  {
    Result += Settings.DecimalSeparator;
    for (int Index = 0; Index < Digits; Index++)
    {
      Result += DigitAt(Rec, Rec.Exponent + Index);
    }
  }
  return Result;
}

UnicodeString CurrencyText(const UnicodeString & Number, bool Negative, const TFormatSettings & Settings)
{
  const UnicodeString & C = Settings.CurrencyString;
  if (!Negative)
  {
    switch (Settings.CurrencyFormat)
    {
      case 1: return Number + C;
      case 2: return C + L" " + Number;
      case 3: return Number + L" " + C;
      default: return C + Number;
    }
  }
  switch (Settings.NegCurrFormat)
  {
    case 1: return L"-" + C + Number;
    case 2: return C + L"-" + Number;
    case 3: return C + Number + L"-";
    case 4: return L"(" + Number + C + L")";
    case 5: return L"-" + Number + C;
    case 6: return Number + L"-" + C;
    case 7: return Number + C + L"-";
    case 8: return L"-" + Number + L" " + C;
    case 9: return L"-" + C + L" " + Number;
    case 10: return Number + L" " + C + L"-";
    case 11: return C + L" " + Number + L"-";
    case 12: return C + L" -" + Number;
    case 13: return Number + L"- " + C;
    case 14: return L"(" + C + L" " + Number + L")";
    case 15: return L"(" + Number + L" " + C + L")";
    default: return L"(" + C + Number + L")";
  }
}

UnicodeString FloatToText(long double Value, TFloatFormat Format, int Precision, int Digits,
  const TFormatSettings & Settings)
{
  if (Precision < 2)
  {
    Precision = 2;
  }
  if (Precision > MaxPrecision)
  {
    Precision = MaxPrecision;
  }
  if (Digits < 0)
  {
    Digits = 0;
  }

  switch (Format)
  {
    case ffExponent:
      {
        TFloatRec Rec = FloatToDecimal(Value, Precision, 9999);
        if (Rec.Nan || Rec.Infinite)
        {
          return SpecialValue(Rec);
        }
        UnicodeString Result;
        if (Rec.Negative && !Rec.Digits.empty())
        {
          Result += L'-';
        }
        Result += DigitAt(Rec, 0);
        Result += Settings.DecimalSeparator;
        for (int Index = 1; Index < Precision; Index++)
        {
          Result += DigitAt(Rec, Index);
        }
        int Exponent = Rec.Digits.empty() ? 0 : (Rec.Exponent - 1);
        return Result + ExponentText(Exponent, Digits, true);
      }

    case ffFixed:
    case ffNumber:
    case ffCurrency:
      {
        if (Digits > 18)
        {
          Digits = 18;
        }
        TFloatRec Rec = FloatToDecimal(Value, MaxPrecision, Digits);
        if (Rec.Nan || Rec.Infinite)
        {
          return SpecialValue(Rec);
        }
        if (Rec.Exponent > Precision)
        {
          return FloatToText(Value, ffGeneral, Precision, Digits, Settings);
        }
        UnicodeString Number = FixedText(Rec, Digits, (Format != ffFixed), Settings);
        bool Negative = Rec.Negative && !Rec.Digits.empty();
        if (Format == ffCurrency)
        {
          return CurrencyText(Number, Negative, Settings);
        }
        return Negative ? (L"-" + Number) : Number;
      }

    default:
      {
        TFloatRec Rec = FloatToDecimal(Value, Precision, 9999);
        if (Rec.Nan || Rec.Infinite)
        {
          return SpecialValue(Rec);
        }
        if (Rec.Digits.empty())
        {
          return L"0";
        }
        UnicodeString Result = Rec.Negative ? L"-" : L"";
        int DigitCount = static_cast<int>(Rec.Digits.size());
        if ((Rec.Exponent > Precision) || (Rec.Exponent < -3))
        {
          Result += DigitAt(Rec, 0);
          if (DigitCount > 1)
          {
            Result += Settings.DecimalSeparator;
            for (int Index = 1; Index < DigitCount; Index++)
            {
              Result += DigitAt(Rec, Index);
            }
          }
          return Result + ExponentText(Rec.Exponent - 1, Digits, false);
        }
        if (Rec.Exponent <= 0)
        {
          Result += L'0';
          Result += Settings.DecimalSeparator;
          for (int Index = Rec.Exponent; Index < 0; Index++)
          {
            Result += L'0';
          }
          for (int Index = 0; Index < DigitCount; Index++)
          {
            Result += DigitAt(Rec, Index);
          }
          return Result;
        }
        for (int Index = 0; Index < Rec.Exponent; Index++)
        {
          Result += DigitAt(Rec, Index);
        }
        if (DigitCount > Rec.Exponent)
        {
          Result += Settings.DecimalSeparator;
          for (int Index = Rec.Exponent; Index < DigitCount; Index++)
          {
            Result += DigitAt(Rec, Index);
          }
        }
        return Result;
      }
  }
}

[[noreturn]] void FormatError(const UnicodeString & Format)
{
  throw EConvertError(L"Format '" + Format + L"' invalid or incompatible with argument");
}

[[noreturn]] void ArgumentMissing(const UnicodeString & Format)
{
  throw EConvertError(L"No argument for format '" + Format + L"'");
}

UnicodeString VarRecToString(const TVarRec & Arg, const UnicodeString & Format)
{
  switch (Arg.VType)
  {
    case vtUnicodeString: return *Arg.VUnicodeString;
    case vtPWideChar: return UnicodeString(Arg.VPWideChar);
    case vtPChar: return UnicodeString(Arg.VPChar);
    case vtChar: return UnicodeString(Arg.VChar);
    case vtWideChar: return UnicodeString(Arg.VWideChar);
    case vtAnsiString:
      return UnicodeString(rtl::DecodeCodePage(Arg.VCodePage, Arg.VAnsiString->c_str(), Arg.VAnsiString->Length()));
    case vtVariant: return VarToStr(*Arg.VVariant);
  }
  FormatError(Format);
}

bool VarRecToInteger(const TVarRec & Arg, long long & Value, bool & IsInt64, bool & IsUnsigned64)
{
  IsInt64 = false;
  IsUnsigned64 = false;
  switch (Arg.VType)
  {
    case vtInteger:
      Value = Arg.VInteger;
      return true;
    case vtInt64:
      Value = Arg.VInt64;
      IsInt64 = true;
      return true;
    case vtUInt64:
      Value = Arg.VInt64;
      IsInt64 = true;
      IsUnsigned64 = true;
      return true;
    case vtVariant:
      if (VarIsOrdinal(*Arg.VVariant))
      {
        Value = static_cast<long long>(*Arg.VVariant);
        IsInt64 = true;
        return true;
      }
      break;
  }
  return false;
}

bool VarRecToFloat(const TVarRec & Arg, long double & Value)
{
  switch (Arg.VType)
  {
    case vtExtended:
      Value = Arg.VExtended;
      return true;
    case vtVariant:
      if (VarIsNumeric(*Arg.VVariant))
      {
        Value = static_cast<double>(*Arg.VVariant);
        return true;
      }
      break;
  }
  return false;
}

std::wstring HexDigits(unsigned long long Value)
{
  static const wchar_t Digits[] = L"0123456789ABCDEF";
  std::wstring Result;
  do
  {
    Result.insert(Result.begin(), Digits[Value & 0xF]);
    Value >>= 4;
  }
  while (Value != 0);
  return Result;
}
} // namespace
//---------------------------------------------------------------------------
UnicodeString Format(const UnicodeString Format, const TVarRec * Args, const int Args_High,
  const TFormatSettings & AFormatSettings)
{
  int ArgCount = (Args != nullptr) ? (Args_High + 1) : 0;
  int ArgIndex = 0;
  UnicodeString Result;
  int Len = Format.Length();
  int I = 1;

  auto NextArg = [&]() -> const TVarRec &
    {
      if ((ArgIndex < 0) || (ArgIndex >= ArgCount))
      {
        ArgumentMissing(Format);
      }
      return Args[ArgIndex++];
    };

  auto ReadNumber = [&](int & Value) -> bool
    {
      if ((I <= Len) && (Format[I] == L'*'))
      {
        const TVarRec & Arg = NextArg();
        if (Arg.VType != vtInteger)
        {
          FormatError(Format);
        }
        Value = Arg.VInteger;
        I++;
        return true;
      }
      bool Any = false;
      Value = 0;
      while ((I <= Len) && (Format[I] >= L'0') && (Format[I] <= L'9'))
      {
        Value = Value * 10 + (Format[I] - L'0');
        if (Value > 99999)
        {
          FormatError(Format);
        }
        Any = true;
        I++;
      }
      return Any;
    };

  while (I <= Len)
  {
    wchar_t Ch = Format[I];
    if (Ch != L'%')
    {
      int Start = I;
      while ((I <= Len) && (Format[I] != L'%'))
      {
        I++;
      }
      Result += Format.SubString(Start, I - Start);
      continue;
    }
    I++;
    if (I > Len)
    {
      FormatError(Format);
    }
    if (Format[I] == L'%')
    {
      Result += L'%';
      I++;
      continue;
    }

    // [index:]
    int Number = 0;
    bool HasNumber = ReadNumber(Number);
    if (HasNumber && (I <= Len) && (Format[I] == L':'))
    {
      ArgIndex = Number;
      I++;
      HasNumber = false;
    }
    // [-]
    bool LeftJustify = false;
    if (!HasNumber && (I <= Len) && (Format[I] == L'-'))
    {
      LeftJustify = true;
      I++;
    }
    // [width]
    int Width = -1;
    if (HasNumber)
    {
      Width = Number;
    }
    else if (ReadNumber(Number))
    {
      Width = Number;
    }
    // [.prec]
    int Precision = -1;
    if ((I <= Len) && (Format[I] == L'.'))
    {
      I++;
      if (!ReadNumber(Number))
      {
        Number = 0;
      }
      Precision = Number;
    }
    if (I > Len)
    {
      FormatError(Format);
    }
    wchar_t Type = rtl::UpperChar(Format[I]);
    I++;

    UnicodeString Value;
    switch (Type)
    {
      case L'D':
      case L'U':
      case L'X':
        {
          long long IntValue;
          bool IsInt64;
          bool IsUnsigned64;
          if (!VarRecToInteger(NextArg(), IntValue, IsInt64, IsUnsigned64))
          {
            FormatError(Format);
          }
          bool Negative = false;
          std::wstring Digits;
          if (Type == L'X')
          {
            unsigned long long Bits =
              IsInt64 ? static_cast<unsigned long long>(IntValue) : static_cast<unsigned int>(IntValue);
            Digits = HexDigits(Bits);
          }
          else if ((Type == L'U') || IsUnsigned64)
          {
            unsigned long long Bits =
              IsInt64 ? static_cast<unsigned long long>(IntValue) : static_cast<unsigned int>(IntValue);
            Digits = std::to_wstring(Bits);
          }
          else
          {
            Negative = (IntValue < 0);
            Digits = Negative ? std::to_wstring(0 - static_cast<unsigned long long>(IntValue)) : std::to_wstring(IntValue);
          }
          if ((Precision >= 0) && (static_cast<int>(Digits.size()) < Precision))
          {
            Digits.insert(0, Precision - Digits.size(), L'0');
          }
          Value = UnicodeString(Negative ? (L"-" + Digits) : Digits);
        }
        break;

      case L'E':
      case L'F':
      case L'G':
      case L'N':
      case L'M':
        {
          long double FloatValue;
          if (!VarRecToFloat(NextArg(), FloatValue))
          {
            FormatError(Format);
          }
          switch (Type)
          {
            case L'E':
              Value = FloatToText(FloatValue, ffExponent, (Precision < 0) ? 15 : Precision, 3, AFormatSettings);
              break;
            case L'F':
              Value = FloatToText(FloatValue, ffFixed, MaxPrecision, (Precision < 0) ? 2 : Precision, AFormatSettings);
              break;
            case L'G':
              Value = FloatToText(FloatValue, ffGeneral, (Precision < 0) ? 15 : Precision, 3, AFormatSettings);
              break;
            case L'N':
              Value = FloatToText(FloatValue, ffNumber, MaxPrecision, (Precision < 0) ? 2 : Precision, AFormatSettings);
              break;
            default:
              Value = FloatToText(FloatValue, ffCurrency, MaxPrecision,
                (Precision < 0) ? AFormatSettings.CurrencyDecimals : Precision, AFormatSettings);
              break;
          }
        }
        break;

      case L'P':
        {
          const TVarRec & Arg = NextArg();
          if ((Arg.VType != vtPointer) && (Arg.VType != vtPChar) && (Arg.VType != vtPWideChar) &&
              (Arg.VType != vtObject) && (Arg.VType != vtClass))
          {
            FormatError(Format);
          }
          std::wstring Digits = HexDigits(reinterpret_cast<uintptr_t>(Arg.VPointer));
          Digits.insert(0, sizeof(void *) * 2 - Digits.size(), L'0');
          Value = UnicodeString(Digits);
        }
        break;

      case L'S':
        Value = VarRecToString(NextArg(), Format);
        if ((Precision >= 0) && (Precision < Value.Length()))
        {
          Value.SetLength(Precision);
        }
        break;

      default:
        FormatError(Format);
    }

    if ((Width >= 0) && (Value.Length() < Width))
    {
      UnicodeString Padding = UnicodeString::StringOfChar(L' ', Width - Value.Length());
      Value = LeftJustify ? (Value + Padding) : (Padding + Value);
    }
    Result += Value;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString Format(const UnicodeString Format, const TVarRec * Args, const int Args_High)
{
  return Sysutils::Format(Format, Args, Args_High, FormatSettings);
}
//---------------------------------------------------------------------------
void FmtStr(UnicodeString & Result, const UnicodeString Format, const TVarRec * Args, const int Args_High)
{
  Result = Sysutils::Format(Format, Args, Args_High);
}
//---------------------------------------------------------------------------
UnicodeString FloatToStrF(Extended Value, TFloatFormat Format, int Precision, int Digits,
  const TFormatSettings & AFormatSettings)
{
  return FloatToText(Value, Format, Precision, Digits, AFormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString FloatToStrF(Extended Value, TFloatFormat Format, int Precision, int Digits)
{
  return FloatToText(Value, Format, Precision, Digits, FormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString FloatToStr(Extended Value, const TFormatSettings & AFormatSettings)
{
  return FloatToText(Value, ffGeneral, 15, 0, AFormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString FloatToStr(Extended Value)
{
  return FloatToStr(Value, FormatSettings);
}
//---------------------------------------------------------------------------
// FormatFloat
//---------------------------------------------------------------------------
namespace
{
struct TPicture
{
  UnicodeString Prefix;
  UnicodeString Suffix;
  int MinIntDigits = 0;
  int MinDecimals = 0;
  int MaxDecimals = 0;
  bool Thousands = false;
  bool Scientific = false;
  bool ScientificPlus = false;
  int MinExpDigits = 0;
  wchar_t ExpChar = L'E';
};

std::vector<UnicodeString> SplitSections(const UnicodeString & Format)
{
  std::vector<UnicodeString> Result;
  UnicodeString Current;
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
    else if (Ch == L';')
    {
      Result.push_back(Current);
      Current = UnicodeString();
      continue;
    }
    Current += Ch;
  }
  Result.push_back(Current);
  return Result;
}

TPicture ParsePicture(const UnicodeString & Section)
{
  TPicture Result;
  bool InDecimals = false;
  bool SeenDigit = false;
  bool AfterNumber = false;
  int I = 1;
  int Len = Section.Length();
  while (I <= Len)
  {
    wchar_t Ch = Section[I];
    UnicodeString & Literal = AfterNumber ? Result.Suffix : Result.Prefix;
    if ((Ch == L'\'') || (Ch == L'"'))
    {
      int End = I + 1;
      while ((End <= Len) && (Section[End] != Ch))
      {
        End++;
      }
      (SeenDigit ? Result.Suffix : Result.Prefix) += Section.SubString(I + 1, End - I - 1);
      if (SeenDigit)
      {
        AfterNumber = true;
      }
      I = End + 1;
      continue;
    }
    if (!AfterNumber && ((Ch == L'0') || (Ch == L'#')))
    {
      SeenDigit = true;
      if (InDecimals)
      {
        Result.MaxDecimals++;
        if (Ch == L'0')
        {
          Result.MinDecimals = Result.MaxDecimals;
        }
      }
      else if (Ch == L'0')
      {
        Result.MinIntDigits++;
      }
      I++;
      continue;
    }
    if (!AfterNumber && (Ch == L'.'))
    {
      InDecimals = true;
      SeenDigit = true;
      I++;
      continue;
    }
    if (!AfterNumber && (Ch == L','))
    {
      Result.Thousands = true;
      I++;
      continue;
    }
    if (SeenDigit && !AfterNumber && ((Ch == L'E') || (Ch == L'e')) && (I < Len) &&
        ((Section[I + 1] == L'+') || (Section[I + 1] == L'-')))
    {
      Result.Scientific = true;
      Result.ExpChar = Ch;
      Result.ScientificPlus = (Section[I + 1] == L'+');
      I += 2;
      while ((I <= Len) && (Section[I] == L'0'))
      {
        Result.MinExpDigits++;
        I++;
      }
      AfterNumber = true;
      continue;
    }
    if (SeenDigit)
    {
      AfterNumber = true;
      Result.Suffix += Ch;
    }
    else
    {
      Literal += Ch;
    }
    I++;
  }
  return Result;
}

UnicodeString ApplyPicture(const TPicture & Picture, long double Value, bool WithSign, const TFormatSettings & Settings)
{
  UnicodeString Number;
  bool Negative = false;
  if (Picture.Scientific)
  {
    int Digits = Picture.MinIntDigits + Picture.MaxDecimals;
    if (Digits < 1)
    {
      Digits = 1;
    }
    TFloatRec Rec = FloatToDecimal(Value, Digits, 9999);
    Negative = Rec.Negative && !Rec.Digits.empty();
    int IntDigits = (Picture.MinIntDigits > 0) ? Picture.MinIntDigits : 1;
    for (int Index = 0; Index < IntDigits; Index++)
    {
      Number += DigitAt(Rec, Index);
    }
    int DecimalCount = static_cast<int>(Rec.Digits.size()) - IntDigits;
    if (DecimalCount < Picture.MinDecimals)
    {
      DecimalCount = Picture.MinDecimals;
    }
    if (DecimalCount > Picture.MaxDecimals)
    {
      DecimalCount = Picture.MaxDecimals;
    }
    if (DecimalCount > 0)
    {
      Number += Settings.DecimalSeparator;
      for (int Index = 0; Index < DecimalCount; Index++)
      {
        Number += DigitAt(Rec, IntDigits + Index);
      }
    }
    int Exponent = Rec.Digits.empty() ? 0 : (Rec.Exponent - IntDigits);
    UnicodeString Exp = ExponentText(Exponent, Picture.MinExpDigits, Picture.ScientificPlus);
    Exp[1] = Picture.ExpChar;
    Number += Exp;
  }
  else
  {
    TFloatRec Rec = FloatToDecimal(Value, MaxPrecision, Picture.MaxDecimals);
    if (Rec.Nan || Rec.Infinite)
    {
      return SpecialValue(Rec);
    }
    Negative = Rec.Negative && !Rec.Digits.empty();
    UnicodeString IntPart;
    for (int Index = 0; Index < Rec.Exponent; Index++)
    {
      IntPart += DigitAt(Rec, Index);
    }
    while (IntPart.Length() < Picture.MinIntDigits)
    {
      IntPart = L"0" + IntPart;
    }
    if (Picture.Thousands && (Settings.ThousandSeparator != L'\0'))
    {
      UnicodeString Grouped;
      int Count = 0;
      for (int Index = IntPart.Length(); Index >= 1; Index--)
      {
        if ((Count > 0) && ((Count % 3) == 0))
        {
          Grouped.Insert(UnicodeString(Settings.ThousandSeparator), 1);
        }
        Grouped.Insert(UnicodeString(IntPart[Index]), 1);
        Count++;
      }
      IntPart = Grouped;
    }
    UnicodeString Decimals;
    for (int Index = 0; Index < Picture.MaxDecimals; Index++)
    {
      Decimals += DigitAt(Rec, Rec.Exponent + Index);
    }
    while ((Decimals.Length() > Picture.MinDecimals) && (Decimals[Decimals.Length()] == L'0'))
    {
      Decimals.SetLength(Decimals.Length() - 1);
    }
    Number = IntPart;
    if (!Decimals.IsEmpty())
    {
      Number += Settings.DecimalSeparator;
      Number += Decimals;
    }
  }
  UnicodeString Result = Picture.Prefix + Number + Picture.Suffix;
  if (WithSign && Negative)
  {
    Result = L"-" + Result;
  }
  return Result;
}
} // namespace
//---------------------------------------------------------------------------
UnicodeString FormatFloat(const UnicodeString Format, Extended Value, const TFormatSettings & AFormatSettings)
{
  if (Format.IsEmpty())
  {
    return FloatToStr(Value, AFormatSettings);
  }
  if (std::isnan(Value) || std::isinf(Value))
  {
    return SpecialValue(FloatToDecimal(Value, MaxPrecision, 0));
  }
  std::vector<UnicodeString> Sections = SplitSections(Format);
  // Section 1: positive (and all values if alone); 2: negative; 3: zero
  TPicture Positive = ParsePicture(Sections[0]);
  if ((Sections.size() >= 3) && !Sections[2].IsEmpty())
  {
    TPicture Zero = ParsePicture(Sections[2]);
    TFloatRec Rounded = FloatToDecimal(Value, MaxPrecision, Positive.MaxDecimals);
    if (Rounded.Digits.empty())
    {
      return ApplyPicture(Zero, 0, false, AFormatSettings);
    }
  }
  if ((Value < 0) && (Sections.size() >= 2) && !Sections[1].IsEmpty())
  {
    return ApplyPicture(ParsePicture(Sections[1]), -Value, false, AFormatSettings);
  }
  return ApplyPicture(Positive, Value, true, AFormatSettings);
}
//---------------------------------------------------------------------------
UnicodeString FormatFloat(const UnicodeString Format, Extended Value)
{
  return FormatFloat(Format, Value, FormatSettings);
}
//---------------------------------------------------------------------------
// Text to float
//---------------------------------------------------------------------------
namespace
{
bool TextToFloat(const UnicodeString & S, long double & Value, const TFormatSettings & Settings)
{
  std::string Ascii;
  int Len = S.Length();
  int I = 1;
  while ((I <= Len) && (S[I] == L' '))
  {
    I++;
  }
  if ((I <= Len) && ((S[I] == L'+') || (S[I] == L'-')))
  {
    Ascii.push_back(static_cast<char>(S[I]));
    I++;
  }
  bool AnyDigit = false;
  while ((I <= Len) && (S[I] >= L'0') && (S[I] <= L'9'))
  {
    Ascii.push_back(static_cast<char>(S[I]));
    AnyDigit = true;
    I++;
  }
  if ((I <= Len) && (S[I] == Settings.DecimalSeparator))
  {
    Ascii.push_back('.');
    I++;
    while ((I <= Len) && (S[I] >= L'0') && (S[I] <= L'9'))
    {
      Ascii.push_back(static_cast<char>(S[I]));
      AnyDigit = true;
      I++;
    }
  }
  if (!AnyDigit)
  {
    return false;
  }
  if ((I <= Len) && ((S[I] == L'E') || (S[I] == L'e')))
  {
    std::string Exponent = "e";
    int J = I + 1;
    if ((J <= Len) && ((S[J] == L'+') || (S[J] == L'-')))
    {
      Exponent.push_back(static_cast<char>(S[J]));
      J++;
    }
    bool AnyExpDigit = false;
    while ((J <= Len) && (S[J] >= L'0') && (S[J] <= L'9'))
    {
      Exponent.push_back(static_cast<char>(S[J]));
      AnyExpDigit = true;
      J++;
    }
    if (!AnyExpDigit)
    {
      return false;
    }
    Ascii += Exponent;
    I = J;
  }
  while ((I <= Len) && (S[I] == L' '))
  {
    I++;
  }
  if (I <= Len)
  {
    return false;
  }
  Value = rtl::WithCLocale([&]() { return strtold(Ascii.c_str(), nullptr); });
  return true;
}
} // namespace
//---------------------------------------------------------------------------
bool TryStrToFloat(const UnicodeString S, Extended & Value, const TFormatSettings & AFormatSettings)
{
  return TextToFloat(S, Value, AFormatSettings);
}
//---------------------------------------------------------------------------
bool TryStrToFloat(const UnicodeString S, double & Value, const TFormatSettings & AFormatSettings)
{
  long double Result;
  bool Success = TextToFloat(S, Result, AFormatSettings);
  if (Success)
  {
    Value = static_cast<double>(Result);
  }
  return Success;
}
//---------------------------------------------------------------------------
bool TryStrToFloat(const UnicodeString S, Extended & Value)
{
  return TryStrToFloat(S, Value, FormatSettings);
}
//---------------------------------------------------------------------------
bool TryStrToFloat(const UnicodeString S, double & Value)
{
  return TryStrToFloat(S, Value, FormatSettings);
}
//---------------------------------------------------------------------------
Extended StrToFloat(const UnicodeString S, const TFormatSettings & AFormatSettings)
{
  Extended Result;
  if (!TryStrToFloat(S, Result, AFormatSettings))
  {
    throw EConvertError(L"'" + S + L"' is not a valid floating point value");
  }
  return Result;
}
//---------------------------------------------------------------------------
Extended StrToFloat(const UnicodeString S)
{
  return StrToFloat(S, FormatSettings);
}
//---------------------------------------------------------------------------
Extended StrToFloatDef(const UnicodeString S, const Extended Default, const TFormatSettings & AFormatSettings)
{
  Extended Result;
  return TryStrToFloat(S, Result, AFormatSettings) ? Result : Default;
}
//---------------------------------------------------------------------------
Extended StrToFloatDef(const UnicodeString S, const Extended Default)
{
  return StrToFloatDef(S, Default, FormatSettings);
}
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
