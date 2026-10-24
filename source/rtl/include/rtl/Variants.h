// System::Variant (C++Builder's sysvari.h) and the Variants unit, limited to
// what the core needs: scalars, strings, dates and one-dimensional arrays.
//---------------------------------------------------------------------------
#ifndef RtlVariantsH
#define RtlVariantsH
//---------------------------------------------------------------------------
#include <rtl/System.h>

#include <memory>
#include <vector>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
typedef Word TVarType;

const TVarType varEmpty = 0x0000;
const TVarType varNull = 0x0001;
const TVarType varSmallint = 0x0002;
const TVarType varInteger = 0x0003;
const TVarType varSingle = 0x0004;
const TVarType varDouble = 0x0005;
const TVarType varCurrency = 0x0006;
const TVarType varDate = 0x0007;
const TVarType varOleStr = 0x0008;
const TVarType varError = 0x000A;
const TVarType varBoolean = 0x000B;
const TVarType varVariant = 0x000C;
const TVarType varShortInt = 0x0010;
const TVarType varByte = 0x0011;
const TVarType varWord = 0x0012;
const TVarType varLongWord = 0x0013;
const TVarType varInt64 = 0x0014;
const TVarType varUInt64 = 0x0015;
const TVarType varString = 0x0100;
const TVarType varUString = 0x0102;
const TVarType varTypeMask = 0x0FFF;
const TVarType varArray = 0x2000;
const TVarType varByRef = 0x4000;
//---------------------------------------------------------------------------
class Variant
{
public:
  Variant() = default;
  Variant(const Variant & Src) = default;
  Variant & operator =(const Variant & Rhs) = default;
  Variant(bool Src);
  Variant(short Src);
  Variant(unsigned short Src);
  Variant(int Src);
  Variant(unsigned int Src);
  Variant(long Src);
  Variant(unsigned long Src);
  Variant(long long Src);
  Variant(unsigned long long Src);
  Variant(float Src);
  Variant(double Src);
  Variant(long double Src);
  Variant(const TDateTime & Src);
  Variant(const UnicodeString & Src);
  Variant(const wchar_t * Src);
  Variant(const char * Src);
  template <unsigned short CP>
  Variant(const AnsiStringT<CP> & Src) : Variant(UnicodeString(Src)) {}
  // Array of the given element type with bounds [Bounds[0], Bounds[1]]
  Variant(const int * Bounds, const int BoundsHigh, TVarType VarType);

  TVarType Type() const { return FType; }
  bool IsNull() const { return (FType == varNull); }
  bool IsEmpty() const { return (FType == varEmpty); }
  bool IsArray() const { return (FType & varArray) != 0; }

  operator bool() const;
  operator short() const { return static_cast<short>(AsInt64()); }
  operator unsigned short() const { return static_cast<unsigned short>(AsInt64()); }
  operator int() const { return static_cast<int>(AsInt64()); }
  operator unsigned int() const { return static_cast<unsigned int>(AsInt64()); }
  operator long() const { return static_cast<long>(AsInt64()); }
  operator unsigned long() const { return static_cast<unsigned long>(AsInt64()); }
  operator long long() const { return AsInt64(); }
  operator unsigned long long() const { return static_cast<unsigned long long>(AsInt64()); }
  operator float() const { return static_cast<float>(AsDouble()); }
  operator double() const { return AsDouble(); }
  operator long double() const { return AsDouble(); }
  operator TDateTime() const { return TDateTime(AsDouble()); }
  operator UnicodeString() const { return AsString(); }

  // Delphi semantics: integers stay integers, mixing with floats gives a double,
  // strings are converted to numbers
  Variant operator +(const Variant & Rhs) const;
  Variant operator -(const Variant & Rhs) const;
  Variant operator *(const Variant & Rhs) const;
  Variant operator /(const Variant & Rhs) const;
  friend Variant operator +(int Lhs, const Variant & Rhs) { return Variant(Lhs) + Rhs; }
  friend Variant operator -(int Lhs, const Variant & Rhs) { return Variant(Lhs) - Rhs; }
  friend Variant operator *(int Lhs, const Variant & Rhs) { return Variant(Lhs) * Rhs; }
  friend Variant operator /(int Lhs, const Variant & Rhs) { return Variant(Lhs) / Rhs; }
  friend Variant operator *(double Lhs, const Variant & Rhs) { return Variant(Lhs) * Rhs; }
  bool operator ==(const Variant & Rhs) const;
  bool operator !=(const Variant & Rhs) const { return !(*this == Rhs); }

  // Arrays
  Variant GetElement(const int I1) const;
  void PutElement(const Variant & Data, const int I1);
  int ArrayDimCount() const { return IsArray() ? 1 : 0; }
  int ArrayLowBound(const int Dim = 1) const;
  int ArrayHighBound(const int Dim = 1) const;
  void ArrayRedim(int HighBound);

  long long AsInt64() const;
  double AsDouble() const;
  UnicodeString AsString() const;

private:
  friend Variant Null();
  struct TNullTag {};
  explicit Variant(TNullTag) : FType(varNull) {}

  struct TArray
  {
    int LowBound;
    TVarType ElementType;
    std::vector<Variant> Elements;
  };

  bool IsOrdinal() const;
  bool IsFloat() const;
  int ElementIndex(int I) const;

  TVarType FType = varEmpty;
  long long FInt = 0;
  double FDouble = 0;
  UnicodeString FString;
  std::shared_ptr<TArray> FArray;
};
typedef Variant OleVariant;
//---------------------------------------------------------------------------
Variant Null();
Variant Unassigned();
//---------------------------------------------------------------------------
namespace Variants
{
TVarType VarType(const Variant & V);
bool VarIsNull(const Variant & V);
bool VarIsEmpty(const Variant & V);
bool VarIsClear(const Variant & V);
bool VarIsOrdinal(const Variant & V);
bool VarIsFloat(const Variant & V);
bool VarIsNumeric(const Variant & V);
bool VarIsStr(const Variant & V);
bool VarIsArray(const Variant & V);
UnicodeString VarToStr(const Variant & V);
UnicodeString VarToStrDef(const Variant & V, const UnicodeString ADefault);
Variant VarAsType(const Variant & V, TVarType AVarType);
Variant VarArrayCreate(const int * Bounds, const int Bounds_High, TVarType AVarType);
int VarArrayLowBound(const Variant & A, int Dim);
int VarArrayHighBound(const Variant & A, int Dim);
void VarArrayRedim(Variant & A, int HighBound);
} // namespace Variants
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Variants;
//---------------------------------------------------------------------------
#endif
