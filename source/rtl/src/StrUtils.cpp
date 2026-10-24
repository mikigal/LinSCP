// System.StrUtils and System.Math
//---------------------------------------------------------------------------
#include <rtl/StrUtils.h>
#include <rtl/Math.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Strutils
{
//---------------------------------------------------------------------------
bool ContainsStr(const UnicodeString AText, const UnicodeString ASubText)
{
  return (AText.Pos(ASubText) > 0);
}
//---------------------------------------------------------------------------
bool ContainsText(const UnicodeString AText, const UnicodeString ASubText)
{
  return (AnsiUpperCase(AText).Pos(AnsiUpperCase(ASubText)) > 0);
}
//---------------------------------------------------------------------------
bool StartsStr(const UnicodeString ASubText, const UnicodeString AText)
{
  return (ASubText.Length() <= AText.Length()) && (AText.SubString(1, ASubText.Length()) == ASubText);
}
//---------------------------------------------------------------------------
bool StartsText(const UnicodeString ASubText, const UnicodeString AText)
{
  return (ASubText.Length() <= AText.Length()) && AnsiSameText(AText.SubString(1, ASubText.Length()), ASubText);
}
//---------------------------------------------------------------------------
bool EndsStr(const UnicodeString ASubText, const UnicodeString AText)
{
  int Start = AText.Length() - ASubText.Length() + 1;
  return (Start >= 1) && (AText.SubString(Start, ASubText.Length()) == ASubText);
}
//---------------------------------------------------------------------------
bool EndsText(const UnicodeString ASubText, const UnicodeString AText)
{
  int Start = AText.Length() - ASubText.Length() + 1;
  return (Start >= 1) && AnsiSameText(AText.SubString(Start, ASubText.Length()), ASubText);
}
//---------------------------------------------------------------------------
bool AnsiContainsStr(const UnicodeString AText, const UnicodeString ASubText) { return ContainsStr(AText, ASubText); }
bool AnsiContainsText(const UnicodeString AText, const UnicodeString ASubText) { return ContainsText(AText, ASubText); }
bool AnsiStartsStr(const UnicodeString ASubText, const UnicodeString AText) { return StartsStr(ASubText, AText); }
bool AnsiStartsText(const UnicodeString ASubText, const UnicodeString AText) { return StartsText(ASubText, AText); }
bool AnsiEndsStr(const UnicodeString ASubText, const UnicodeString AText) { return EndsStr(ASubText, AText); }
bool AnsiEndsText(const UnicodeString ASubText, const UnicodeString AText) { return EndsText(ASubText, AText); }
//---------------------------------------------------------------------------
UnicodeString ReplaceStr(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText)
{
  return StringReplace(AText, AFromText, AToText, TReplaceFlags() << rfReplaceAll);
}
//---------------------------------------------------------------------------
UnicodeString ReplaceText(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText)
{
  return StringReplace(AText, AFromText, AToText, TReplaceFlags() << rfReplaceAll << rfIgnoreCase);
}
//---------------------------------------------------------------------------
UnicodeString AnsiReplaceStr(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText)
{
  return ReplaceStr(AText, AFromText, AToText);
}
//---------------------------------------------------------------------------
UnicodeString AnsiReplaceText(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText)
{
  return ReplaceText(AText, AFromText, AToText);
}
//---------------------------------------------------------------------------
UnicodeString LeftStr(const UnicodeString AText, const int ACount)
{
  return AText.SubString(1, ACount);
}
//---------------------------------------------------------------------------
UnicodeString RightStr(const UnicodeString AText, const int ACount)
{
  return AText.SubString(AText.Length() + 1 - ACount, ACount);
}
//---------------------------------------------------------------------------
UnicodeString MidStr(const UnicodeString AText, const int AStart, const int ACount)
{
  return AText.SubString(AStart, ACount);
}
//---------------------------------------------------------------------------
UnicodeString AnsiLeftStr(const UnicodeString AText, const int ACount) { return LeftStr(AText, ACount); }
UnicodeString AnsiRightStr(const UnicodeString AText, const int ACount) { return RightStr(AText, ACount); }
UnicodeString AnsiMidStr(const UnicodeString AText, const int AStart, const int ACount) { return MidStr(AText, AStart, ACount); }
//---------------------------------------------------------------------------
int PosEx(const UnicodeString SubStr, const UnicodeString S, int Offset)
{
  return System::Pos(SubStr, S, Offset);
}
//---------------------------------------------------------------------------
int IndexStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High)
{
  for (int Index = 0; Index <= AValues_High; Index++)
  {
    if (AValues[Index] == AText)
    {
      return Index;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
int IndexText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High)
{
  for (int Index = 0; Index <= AValues_High; Index++)
  {
    if (AnsiSameText(AValues[Index], AText))
    {
      return Index;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
bool MatchStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High)
{
  return (IndexStr(AText, AValues, AValues_High) >= 0);
}
//---------------------------------------------------------------------------
bool MatchText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High)
{
  return (IndexText(AText, AValues, AValues_High) >= 0);
}
//---------------------------------------------------------------------------
int AnsiIndexStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High) { return IndexStr(AText, AValues, AValues_High); }
int AnsiIndexText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High) { return IndexText(AText, AValues, AValues_High); }
bool AnsiMatchStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High) { return MatchStr(AText, AValues, AValues_High); }
bool AnsiMatchText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High) { return MatchText(AText, AValues, AValues_High); }
//---------------------------------------------------------------------------
UnicodeString DupeString(const UnicodeString AText, int ACount)
{
  std::wstring Result;
  for (int Index = 0; Index < ACount; Index++)
  {
    Result += AText.Std();
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
UnicodeString ReverseString(const UnicodeString AText)
{
  return UnicodeString(std::wstring(AText.Std().rbegin(), AText.Std().rend()));
}
//---------------------------------------------------------------------------
UnicodeString StuffString(const UnicodeString AText, int AStart, int ALength, const UnicodeString ASubText)
{
  return AText.SubString(1, AStart - 1) + ASubText + AText.SubString(AStart + ALength, MaxInt);
}
//---------------------------------------------------------------------------
UnicodeString IfThen(bool AValue, const UnicodeString ATrue, const UnicodeString AFalse)
{
  return AValue ? ATrue : AFalse;
}
//---------------------------------------------------------------------------
TStringDynArray SplitString(const UnicodeString S, const UnicodeString Delimiters)
{
  TStringDynArray Result;
  if (S.IsEmpty())
  {
    return Result;
  }
  std::vector<UnicodeString> Parts;
  int Start = 1;
  for (int I = 1; I <= S.Length(); I++)
  {
    if (S.IsDelimiter(Delimiters, I))
    {
      Parts.push_back(S.SubString(Start, I - Start));
      Start = I + 1;
    }
  }
  Parts.push_back(S.SubString(Start, MaxInt));
  Result.Length = static_cast<int>(Parts.size());
  for (size_t Index = 0; Index < Parts.size(); Index++)
  {
    Result[static_cast<int>(Index)] = Parts[Index];
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Strutils
//---------------------------------------------------------------------------
namespace Math
{
//---------------------------------------------------------------------------
int Floor(const Extended X) { return static_cast<int>(std::floor(X)); }
int Ceil(const Extended X) { return static_cast<int>(std::ceil(X)); }
__int64 Floor64(const Extended X) { return static_cast<__int64>(std::floor(X)); }
__int64 Ceil64(const Extended X) { return static_cast<__int64>(std::ceil(X)); }
Extended Power(const Extended Base, const Extended Exponent) { return std::pow(Base, Exponent); }
Extended Log10(const Extended X) { return std::log10(X); }
Extended Log2(const Extended X) { return std::log2(X); }
Extended LogN(const Extended Base, const Extended X) { return std::log(X) / std::log(Base); }
bool IsNan(const Extended AValue) { return std::isnan(AValue); }
bool IsInfinite(const Extended AValue) { return std::isinf(AValue); }
//---------------------------------------------------------------------------
Extended IntPower(const Extended Base, int Exponent)
{
  Extended Result = 1;
  Extended Factor = Base;
  unsigned int N = (Exponent < 0) ? (0u - static_cast<unsigned int>(Exponent)) : static_cast<unsigned int>(Exponent);
  while (N != 0)
  {
    if ((N & 1) != 0)
    {
      Result *= Factor;
    }
    Factor *= Factor;
    N >>= 1;
  }
  return (Exponent < 0) ? (1 / Result) : Result;
}
//---------------------------------------------------------------------------
int Sign(const Extended AValue)
{
  return (AValue > 0) ? 1 : ((AValue < 0) ? -1 : 0);
}
//---------------------------------------------------------------------------
namespace
{
// Delphi's default epsilons for Extended comparisons
const Extended ExtendedResolution = 1E-19L * 1000;

Extended Epsilon(Extended A, Extended B, Extended Value)
{
  if (Value == 0)
  {
    Value = std::fmax(std::fmin(std::fabs(A), std::fabs(B)) * ExtendedResolution, ExtendedResolution);
  }
  return Value;
}
} // namespace
//---------------------------------------------------------------------------
bool IsZero(const Extended A, Extended Epsilon)
{
  if (Epsilon == 0)
  {
    Epsilon = ExtendedResolution;
  }
  return (std::fabs(A) <= Epsilon);
}
//---------------------------------------------------------------------------
bool SameValue(const Extended A, const Extended B, Extended AEpsilon)
{
  return (std::fabs(A - B) <= Epsilon(A, B, AEpsilon));
}
//---------------------------------------------------------------------------
TValueRelationship CompareValue(const Extended A, const Extended B, Extended AEpsilon)
{
  if (SameValue(A, B, AEpsilon))
  {
    return EqualsValue;
  }
  return (A < B) ? LessThanValue : GreaterThanValue;
}
//---------------------------------------------------------------------------
TValueRelationship CompareValue(const __int64 A, const __int64 B)
{
  return (A == B) ? EqualsValue : ((A < B) ? LessThanValue : GreaterThanValue);
}
//---------------------------------------------------------------------------
TValueRelationship CompareValue(const int A, const int B)
{
  return (A == B) ? EqualsValue : ((A < B) ? LessThanValue : GreaterThanValue);
}
//---------------------------------------------------------------------------
double RoundTo(const double AValue, int ADigit)
{
  double Factor = static_cast<double>(IntPower(10, ADigit));
  return std::nearbyint(AValue / Factor) * Factor;
}
//---------------------------------------------------------------------------
double SimpleRoundTo(const double AValue, int ADigit)
{
  double Factor = static_cast<double>(IntPower(10, ADigit));
  return (AValue < 0) ? (std::trunc(AValue / Factor - 0.5) * Factor) : (std::trunc(AValue / Factor + 0.5) * Factor);
}
//---------------------------------------------------------------------------
void DivMod(int Dividend, Word Divisor, Word & Result, Word & Remainder)
{
  Result = static_cast<Word>(Dividend / Divisor);
  Remainder = static_cast<Word>(Dividend % Divisor);
}
//---------------------------------------------------------------------------
int IfThen(bool AValue, const int ATrue, const int AFalse) { return AValue ? ATrue : AFalse; }
__int64 IfThen(bool AValue, const __int64 ATrue, const __int64 AFalse) { return AValue ? ATrue : AFalse; }
double IfThen(bool AValue, const double ATrue, const double AFalse) { return AValue ? ATrue : AFalse; }
//---------------------------------------------------------------------------
int MinIntValue(const int * Data, const int Data_High)
{
  int Result = Data[0];
  for (int Index = 1; Index <= Data_High; Index++)
  {
    Result = std::min(Result, Data[Index]);
  }
  return Result;
}
//---------------------------------------------------------------------------
int MaxIntValue(const int * Data, const int Data_High)
{
  int Result = Data[0];
  for (int Index = 1; Index <= Data_High; Index++)
  {
    Result = std::max(Result, Data[Index]);
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Math
} // namespace System
