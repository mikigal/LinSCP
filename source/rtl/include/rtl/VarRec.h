// System::TVarRec and open arrays (Delphi's "array of const"), as used by
// Format() through the ARRAYOFCONST macro.
//
// ARRAYOFCONST((A, B)) expands to the two arguments C++Builder passes for an
// open array: a pointer to the values and the index of the last one. Unlike
// C++Builder's macro, the count is computed in an unevaluated context, so the
// argument expressions are evaluated only once.
//---------------------------------------------------------------------------
#ifndef RtlVarRecH
#define RtlVarRecH
//---------------------------------------------------------------------------
#include <rtl/AnsiString.h>
#include <rtl/Object.h>

#include <type_traits>
//---------------------------------------------------------------------------
namespace System
{
class Variant;
//---------------------------------------------------------------------------
const int vtInteger = 0;
const int vtBoolean = 1;
const int vtChar = 2;
const int vtExtended = 3;
const int vtString = 4;
const int vtPointer = 5;
const int vtPChar = 6;
const int vtObject = 7;
const int vtClass = 8;
const int vtWideChar = 9;
const int vtPWideChar = 10;
const int vtAnsiString = 11;
const int vtCurrency = 12;
const int vtVariant = 13;
const int vtInterface = 14;
const int vtWideString = 15;
const int vtInt64 = 16;
const int vtUnicodeString = 17;
// Not in Delphi: unsigned 64-bit values that do not fit Int64
const int vtUInt64 = 100;
//---------------------------------------------------------------------------
class TVarRec
{
public:
  TVarRec() : VInt64(0), VType(vtInteger) {}
  TVarRec(bool Src) : VInt64(0), VType(vtBoolean) { VBoolean = Src; }
  TVarRec(char Src) : VInt64(0), VType(vtChar) { VChar = Src; }
  TVarRec(signed char Src) : VInt64(0), VType(vtInteger) { VInteger = Src; }
  TVarRec(unsigned char Src) : VInt64(0), VType(vtInteger) { VInteger = Src; }
  TVarRec(wchar_t Src) : VInt64(0), VType(vtWideChar) { VWideChar = Src; }
  TVarRec(short Src) : VInt64(0), VType(vtInteger) { VInteger = Src; }
  TVarRec(unsigned short Src) : VInt64(0), VType(vtInteger) { VInteger = Src; }
  TVarRec(int Src) : VInt64(0), VType(vtInteger) { VInteger = Src; }
  TVarRec(unsigned int Src) : VInt64(Src), VType(vtInt64) {}
  TVarRec(long Src) : VInt64(Src), VType(vtInt64) {}
  TVarRec(unsigned long Src) : TVarRec(static_cast<unsigned long long>(Src)) {}
  TVarRec(long long Src) : VInt64(Src), VType(vtInt64) {}
  TVarRec(unsigned long long Src) :
    VInt64(static_cast<long long>(Src)),
    VType((Src > static_cast<unsigned long long>(INT64_MAX)) ? vtUInt64 : vtInt64)
  {
  }
  TVarRec(float Src) : VExtended(Src), VType(vtExtended) {}
  TVarRec(double Src) : VExtended(Src), VType(vtExtended) {}
  TVarRec(long double Src) : VExtended(Src), VType(vtExtended) {}
  TVarRec(const char * Src) : VInt64(0), VType(vtPChar) { VPChar = Src; }
  TVarRec(char * Src) : TVarRec(static_cast<const char *>(Src)) {}
  TVarRec(const wchar_t * Src) : VInt64(0), VType(vtPWideChar) { VPWideChar = Src; }
  TVarRec(wchar_t * Src) : TVarRec(static_cast<const wchar_t *>(Src)) {}
  TVarRec(const void * Src) : VInt64(0), VType(vtPointer) { VPointer = Src; }
  TVarRec(TObject * Src) : VInt64(0), VType(vtObject) { VObject = Src; }
  TVarRec(TClass Src) : VInt64(0), VType(vtClass) { VClass = Src; }
  TVarRec(const UnicodeString & Src) : VInt64(0), VType(vtUnicodeString) { VUnicodeString = &Src; }
  template <unsigned short CP>
  TVarRec(const AnsiStringT<CP> & Src) : VInt64(0), VType(vtAnsiString), VCodePage(CP) { VAnsiString = &Src; }
  TVarRec(const Variant & Src) : VInt64(0), VType(vtVariant) { VVariant = &Src; }
  // Any other class type would be converted to a temporary that dies before
  // Format() reads it; refuse such arguments at compile time.
  template <typename T, typename = std::enable_if_t<std::is_class_v<T>>>
  TVarRec(const T & Src) = delete;

  union
  {
    int VInteger;
    bool VBoolean;
    char VChar;
    wchar_t VWideChar;
    long long VInt64;
    long double VExtended;
    const void * VPointer;
    const char * VPChar;
    const wchar_t * VPWideChar;
    TObject * VObject;
    TClass VClass;
    const UnicodeString * VUnicodeString;
    const AnsiStringBase * VAnsiString;
    const Variant * VVariant;
  };
  int VType;
  unsigned short VCodePage = 0;
};
//---------------------------------------------------------------------------
const int MaxOpenArrayCount = 32;
//---------------------------------------------------------------------------
template <typename T>
class OpenArray
{
public:
  OpenArray() = default;
  template <typename... Args>
  OpenArray(const Args &... Values) :
    FCount(static_cast<int>(sizeof...(Args))),
    FValues{T(Values)...}
  {
    static_assert(sizeof...(Args) <= MaxOpenArrayCount, "too many open array values");
  }

  operator const T * () const { return FValues; }
  operator T * () { return FValues; }
  int GetHigh() const { return FCount - 1; }
  int GetCount() const { return FCount; }

private:
  int FCount = 0;
  T FValues[MaxOpenArrayCount] = {};
};
//---------------------------------------------------------------------------
template <typename T>
class OpenArrayCount
{
public:
  template <typename... Args>
  OpenArrayCount(const Args &...) : FCount(static_cast<int>(sizeof...(Args))) {}
  int GetHigh() const { return FCount - 1; }

private:
  int FCount;
};
//---------------------------------------------------------------------------
// Only used in decltype(), never evaluated
template <typename... Args>
std::integral_constant<int, static_cast<int>(sizeof...(Args)) - 1> OpenArrayHigh(const Args &...);
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#define ARRAYOFCONST(values) \
  ::System::OpenArray<::System::TVarRec> values, decltype(::System::OpenArrayHigh values)::value
#define OPENARRAY(type, values) \
  ::System::OpenArray<type> values, decltype(::System::OpenArrayHigh values)::value
#define EXISTINGARRAY(a) (a), static_cast<int>((sizeof(a) / sizeof(a[0])) - 1)
//---------------------------------------------------------------------------
#endif
