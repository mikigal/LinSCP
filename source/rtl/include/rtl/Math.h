// The System.Math unit.
//
// Min, Max, InRange and EnsureRange are templates instead of Delphi's overload
// sets: Linux has more integer types of the same width (long vs long long), and
// fixed overloads would make calls with them ambiguous. Mixing signed and
// unsigned types is rejected, as with Delphi's overloads.
//---------------------------------------------------------------------------
#ifndef RtlMathH
#define RtlMathH
//---------------------------------------------------------------------------
#include <rtl/System.h>
#include <rtl/Types.h>

#include <cmath>
#include <limits>
#include <type_traits>
//---------------------------------------------------------------------------
namespace System
{
namespace Math
{
//---------------------------------------------------------------------------
namespace rtl_detail
{
template <typename A, typename B>
using TMixedArithmetic = std::enable_if_t<
  std::is_arithmetic_v<A> && std::is_arithmetic_v<B> && !std::is_same_v<A, B> &&
  (std::is_floating_point_v<A> || std::is_floating_point_v<B> || (std::is_signed_v<A> == std::is_signed_v<B>)),
  std::common_type_t<A, B>>;
}
//---------------------------------------------------------------------------
template <typename T>
constexpr T Min(const T A, const T B) { return (B < A) ? B : A; }
template <typename T>
constexpr T Max(const T A, const T B) { return (A < B) ? B : A; }
template <typename A, typename B>
constexpr rtl_detail::TMixedArithmetic<A, B> Min(const A X, const B Y)
{
  typedef std::common_type_t<A, B> T;
  return Min<T>(static_cast<T>(X), static_cast<T>(Y));
}
template <typename A, typename B>
constexpr rtl_detail::TMixedArithmetic<A, B> Max(const A X, const B Y)
{
  typedef std::common_type_t<A, B> T;
  return Max<T>(static_cast<T>(X), static_cast<T>(Y));
}
template <typename T>
constexpr bool InRange(const T AValue, const T AMin, const T AMax)
{
  return (AValue >= AMin) && (AValue <= AMax);
}
template <typename T>
constexpr T EnsureRange(const T AValue, const T AMin, const T AMax)
{
  return (AValue < AMin) ? AMin : ((AValue > AMax) ? AMax : AValue);
}
//---------------------------------------------------------------------------
const double NaN = std::numeric_limits<double>::quiet_NaN();
const double Infinity = std::numeric_limits<double>::infinity();
const double NegInfinity = -std::numeric_limits<double>::infinity();

int Floor(const Extended X);
int Ceil(const Extended X);
__int64 Floor64(const Extended X);
__int64 Ceil64(const Extended X);
Extended Power(const Extended Base, const Extended Exponent);
Extended IntPower(const Extended Base, int Exponent);
Extended Log10(const Extended X);
Extended Log2(const Extended X);
Extended LogN(const Extended Base, const Extended X);
int Sign(const Extended AValue);
bool IsZero(const Extended A, Extended Epsilon = 0);
bool SameValue(const Extended A, const Extended B, Extended Epsilon = 0);
bool IsNan(const Extended AValue);
bool IsInfinite(const Extended AValue);
TValueRelationship CompareValue(const Extended A, const Extended B, Extended Epsilon = 0);
TValueRelationship CompareValue(const __int64 A, const __int64 B);
TValueRelationship CompareValue(const int A, const int B);
// Rounds to the power of ten ADigit, half to even (banker's rounding)
double RoundTo(const double AValue, int ADigit);
// Rounds to the power of ten ADigit, half away from zero
double SimpleRoundTo(const double AValue, int ADigit = -2);
void DivMod(int Dividend, Word Divisor, Word & Result, Word & Remainder);
int IfThen(bool AValue, const int ATrue, const int AFalse = 0);
__int64 IfThen(bool AValue, const __int64 ATrue, const __int64 AFalse = 0);
double IfThen(bool AValue, const double ATrue, const double AFalse = 0);
int MinIntValue(const int * Data, const int Data_High);
int MaxIntValue(const int * Data, const int Data_High);
//---------------------------------------------------------------------------
} // namespace Math
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Math;
//---------------------------------------------------------------------------
#endif
