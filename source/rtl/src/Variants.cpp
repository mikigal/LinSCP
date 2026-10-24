// System::Variant
//---------------------------------------------------------------------------
#include <rtl/Variants.h>
#include <rtl/SysUtils.h>

#include <cmath>
#include <limits>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
namespace
{
UnicodeString VarTypeName(TVarType Type)
{
  if ((Type & varArray) != 0)
  {
    return L"Array " + VarTypeName(Type & varTypeMask);
  }
  switch (Type)
  {
    case varEmpty: return L"Empty";
    case varNull: return L"Null";
    case varSmallint: return L"Smallint";
    case varInteger: return L"Integer";
    case varSingle: return L"Single";
    case varDouble: return L"Double";
    case varCurrency: return L"Currency";
    case varDate: return L"Date";
    case varBoolean: return L"Boolean";
    case varVariant: return L"Variant";
    case varShortInt: return L"ShortInt";
    case varByte: return L"Byte";
    case varWord: return L"Word";
    case varLongWord: return L"Cardinal";
    case varInt64: return L"Int64";
    case varUInt64: return L"UInt64";
    case varUString: return L"UnicodeString";
    default: return IntToHex(static_cast<int>(Type), 4);
  }
}

[[noreturn]] void ConversionError(TVarType From, TVarType To)
{
  throw EVariantError(
    L"Could not convert variant of type (" + VarTypeName(From) + L") into type (" + VarTypeName(To) + L")");
}

[[noreturn]] void OverflowError(TVarType From, TVarType To)
{
  throw EVariantError(
    L"Overflow while converting variant of type (" + VarTypeName(From) + L") into type (" + VarTypeName(To) + L")");
}

Variant ZeroOfType(TVarType Type)
{
  switch (Type)
  {
    case varEmpty:
    case varVariant: return Variant();
    case varUString: return Variant(UnicodeString());
    default: return VarAsType(Variant(0), Type);
  }
}
} // namespace
//---------------------------------------------------------------------------
Variant::Variant(bool Src) : FType(varBoolean), FInt(Src ? -1 : 0) {}
Variant::Variant(short Src) : FType(varSmallint), FInt(Src) {}
Variant::Variant(unsigned short Src) : FType(varWord), FInt(Src) {}
Variant::Variant(int Src) : FType(varInteger), FInt(Src) {}
Variant::Variant(unsigned int Src) : FType(varLongWord), FInt(Src) {}
Variant::Variant(long Src) : FType(varInt64), FInt(Src) {}
Variant::Variant(unsigned long Src) : FType(varUInt64), FInt(static_cast<long long>(Src)) {}
Variant::Variant(long long Src) : FType(varInt64), FInt(Src) {}
Variant::Variant(unsigned long long Src) : FType(varUInt64), FInt(static_cast<long long>(Src)) {}
Variant::Variant(float Src) : FType(varSingle), FDouble(Src) {}
Variant::Variant(double Src) : FType(varDouble), FDouble(Src) {}
Variant::Variant(long double Src) : FType(varDouble), FDouble(static_cast<double>(Src)) {}
Variant::Variant(const TDateTime & Src) : FType(varDate), FDouble(Src.Val) {}
Variant::Variant(const UnicodeString & Src) : FType(varUString), FString(Src) {}
Variant::Variant(const wchar_t * Src) : FType(varUString), FString(Src) {}
Variant::Variant(const char * Src) : FType(varUString), FString(Src) {}
//---------------------------------------------------------------------------
Variant::Variant(const int * Bounds, const int BoundsHigh, TVarType VarType)
{
  if (BoundsHigh != 1)
  {
    throw EVariantError(L"Only one-dimensional variant arrays are supported");
  }
  if (Bounds[1] < Bounds[0] - 1)
  {
    throw EVariantError(L"Invalid variant array bounds");
  }
  FType = varArray | (VarType & varTypeMask);
  FArray = std::make_shared<TArray>();
  FArray->LowBound = Bounds[0];
  FArray->ElementType = VarType & varTypeMask;
  FArray->Elements.assign(Bounds[1] - Bounds[0] + 1, ZeroOfType(FArray->ElementType));
}
//---------------------------------------------------------------------------
bool Variant::IsOrdinal() const
{
  switch (FType)
  {
    case varSmallint:
    case varInteger:
    case varBoolean:
    case varShortInt:
    case varByte:
    case varWord:
    case varLongWord:
    case varInt64:
    case varUInt64:
      return true;
  }
  return false;
}
//---------------------------------------------------------------------------
bool Variant::IsFloat() const
{
  return (FType == varSingle) || (FType == varDouble) || (FType == varCurrency) || (FType == varDate);
}
//---------------------------------------------------------------------------
long long Variant::AsInt64() const
{
  if (FType == varEmpty)
  {
    return 0;
  }
  if (IsOrdinal())
  {
    return FInt;
  }
  if (IsFloat())
  {
    // Delphi rounds (half to even) when converting floats to integers
    double Rounded = std::nearbyint(FDouble);
    if (!(Rounded >= -9223372036854775808.0) || !(Rounded < 9223372036854775808.0))
    {
      OverflowError(FType, varInt64);
    }
    return static_cast<long long>(Rounded);
  }
  if (FType == varUString)
  {
    __int64 Result;
    if (TryStrToInt64(FString, Result))
    {
      return Result;
    }
    Extended Float;
    if (TryStrToFloat(FString, Float))
    {
      return Variant(static_cast<double>(Float)).AsInt64();
    }
  }
  ConversionError(FType, varInt64);
}
//---------------------------------------------------------------------------
double Variant::AsDouble() const
{
  if (FType == varEmpty)
  {
    return 0;
  }
  if (FType == varUInt64)
  {
    return static_cast<double>(static_cast<unsigned long long>(FInt));
  }
  if (IsOrdinal())
  {
    return static_cast<double>(FInt);
  }
  if (IsFloat())
  {
    return FDouble;
  }
  if (FType == varUString)
  {
    Extended Result;
    if (TryStrToFloat(FString, Result))
    {
      return static_cast<double>(Result);
    }
  }
  ConversionError(FType, varDouble);
}
//---------------------------------------------------------------------------
UnicodeString Variant::AsString() const
{
  switch (FType)
  {
    case varEmpty: return UnicodeString();
    case varUString: return FString;
    case varBoolean: return BoolToStr(FInt != 0, true);
    case varUInt64: return UIntToStr(static_cast<unsigned long long>(FInt));
    case varDate: return DateTimeToStr(TDateTime(FDouble));
  }
  if (IsOrdinal())
  {
    return IntToStr(FInt);
  }
  if (IsFloat())
  {
    return FloatToStr(FDouble);
  }
  ConversionError(FType, varUString);
}
//---------------------------------------------------------------------------
Variant::operator bool() const
{
  if (FType == varEmpty)
  {
    return false;
  }
  if (IsOrdinal())
  {
    return (FInt != 0);
  }
  if (IsFloat())
  {
    return (FDouble != 0);
  }
  if (FType == varUString)
  {
    bool Result;
    if (TryStrToBool(FString, Result))
    {
      return Result;
    }
  }
  ConversionError(FType, varBoolean);
}
//---------------------------------------------------------------------------
namespace
{
enum TOperation { opAdd, opSubtract, opMultiply, opDivide };

Variant Calculate(const Variant & Lhs, const Variant & Rhs, TOperation Operation)
{
  if (Lhs.IsNull() || Rhs.IsNull())
  {
    return Null();
  }
  if ((Operation == opAdd) && (Lhs.Type() == varUString) && (Rhs.Type() == varUString))
  {
    return Variant(Lhs.AsString() + Rhs.AsString());
  }
  bool Integral =
    (Operation != opDivide) &&
    (VarIsOrdinal(Lhs) || Lhs.IsEmpty()) && (VarIsOrdinal(Rhs) || Rhs.IsEmpty());
  if (Integral)
  {
    long long A = Lhs.AsInt64();
    long long B = Rhs.AsInt64();
    switch (Operation)
    {
      case opAdd: return Variant(A + B);
      case opSubtract: return Variant(A - B);
      default: return Variant(A * B);
    }
  }
  double A = Lhs.AsDouble();
  double B = Rhs.AsDouble();
  double Result;
  switch (Operation)
  {
    case opAdd: Result = A + B; break;
    case opSubtract: Result = A - B; break;
    case opMultiply: Result = A * B; break;
    default:
      if (B == 0)
      {
        throw EZeroDivide(L"Floating point division by zero");
      }
      Result = A / B;
      break;
  }
  bool DateResult =
    ((Operation == opAdd) || (Operation == opSubtract)) &&
    ((Lhs.Type() == varDate) != (Rhs.Type() == varDate));
  return DateResult ? Variant(TDateTime(Result)) : Variant(Result);
}
} // namespace
//---------------------------------------------------------------------------
Variant Variant::operator +(const Variant & Rhs) const { return Calculate(*this, Rhs, opAdd); }
Variant Variant::operator -(const Variant & Rhs) const { return Calculate(*this, Rhs, opSubtract); }
Variant Variant::operator *(const Variant & Rhs) const { return Calculate(*this, Rhs, opMultiply); }
Variant Variant::operator /(const Variant & Rhs) const { return Calculate(*this, Rhs, opDivide); }
//---------------------------------------------------------------------------
bool Variant::operator ==(const Variant & Rhs) const
{
  if (IsNull() || Rhs.IsNull())
  {
    return IsNull() && Rhs.IsNull();
  }
  if (IsEmpty() && Rhs.IsEmpty())
  {
    return true;
  }
  if ((FType == varUString) && (Rhs.FType == varUString))
  {
    return (FString == Rhs.FString);
  }
  if ((FType == varUString) || (Rhs.FType == varUString))
  {
    const Variant & Str = (FType == varUString) ? *this : Rhs;
    const Variant & Other = (FType == varUString) ? Rhs : *this;
    Extended Number;
    if (!TryStrToFloat(Str.FString, Number))
    {
      return (Str.FString == Other.AsString());
    }
    return (static_cast<double>(Number) == Other.AsDouble());
  }
  if (IsArray() || Rhs.IsArray())
  {
    return false;
  }
  if ((IsOrdinal() || IsEmpty()) && (Rhs.IsOrdinal() || Rhs.IsEmpty()))
  {
    return (AsInt64() == Rhs.AsInt64());
  }
  return (AsDouble() == Rhs.AsDouble());
}
//---------------------------------------------------------------------------
int Variant::ElementIndex(int I) const
{
  if (!IsArray())
  {
    throw EVariantError(L"Variant is not an array");
  }
  int Index = I - FArray->LowBound;
  if ((Index < 0) || (Index >= static_cast<int>(FArray->Elements.size())))
  {
    throw EVariantError(L"Variant or safe array index out of bounds");
  }
  return Index;
}
//---------------------------------------------------------------------------
Variant Variant::GetElement(const int I1) const
{
  return FArray->Elements[ElementIndex(I1)];
}
//---------------------------------------------------------------------------
void Variant::PutElement(const Variant & Data, const int I1)
{
  int Index = ElementIndex(I1);
  if (FArray.use_count() > 1)
  {
    FArray = std::make_shared<TArray>(*FArray);
  }
  FArray->Elements[Index] =
    (FArray->ElementType == varVariant) ? Data : VarAsType(Data, FArray->ElementType);
}
//---------------------------------------------------------------------------
int Variant::ArrayLowBound(const int Dim) const
{
  if (!IsArray() || (Dim != 1))
  {
    throw EVariantError(L"Variant array dimension out of range");
  }
  return FArray->LowBound;
}
//---------------------------------------------------------------------------
int Variant::ArrayHighBound(const int Dim) const
{
  if (!IsArray() || (Dim != 1))
  {
    throw EVariantError(L"Variant array dimension out of range");
  }
  return FArray->LowBound + static_cast<int>(FArray->Elements.size()) - 1;
}
//---------------------------------------------------------------------------
void Variant::ArrayRedim(int HighBound)
{
  if (!IsArray() || (HighBound < FArray->LowBound - 1))
  {
    throw EVariantError(L"Invalid variant array bounds");
  }
  if (FArray.use_count() > 1)
  {
    FArray = std::make_shared<TArray>(*FArray);
  }
  FArray->Elements.resize(HighBound - FArray->LowBound + 1, ZeroOfType(FArray->ElementType));
}
//---------------------------------------------------------------------------
Variant Null()
{
  return Variant(Variant::TNullTag());
}
//---------------------------------------------------------------------------
Variant Unassigned()
{
  return Variant();
}
//---------------------------------------------------------------------------
namespace Variants
{
//---------------------------------------------------------------------------
TVarType VarType(const Variant & V)
{
  return V.Type();
}
//---------------------------------------------------------------------------
bool VarIsNull(const Variant & V)
{
  return V.IsNull();
}
//---------------------------------------------------------------------------
bool VarIsEmpty(const Variant & V)
{
  return V.IsEmpty();
}
//---------------------------------------------------------------------------
bool VarIsClear(const Variant & V)
{
  return V.IsEmpty();
}
//---------------------------------------------------------------------------
bool VarIsOrdinal(const Variant & V)
{
  switch (V.Type())
  {
    case varSmallint:
    case varInteger:
    case varBoolean:
    case varShortInt:
    case varByte:
    case varWord:
    case varLongWord:
    case varInt64:
    case varUInt64:
      return true;
  }
  return false;
}
//---------------------------------------------------------------------------
bool VarIsFloat(const Variant & V)
{
  return (V.Type() == varSingle) || (V.Type() == varDouble) || (V.Type() == varCurrency) || (V.Type() == varDate);
}
//---------------------------------------------------------------------------
bool VarIsNumeric(const Variant & V)
{
  return VarIsOrdinal(V) || VarIsFloat(V);
}
//---------------------------------------------------------------------------
bool VarIsStr(const Variant & V)
{
  return (V.Type() == varUString) || (V.Type() == varString) || (V.Type() == varOleStr);
}
//---------------------------------------------------------------------------
bool VarIsArray(const Variant & V)
{
  return V.IsArray();
}
//---------------------------------------------------------------------------
UnicodeString VarToStr(const Variant & V)
{
  return VarToStrDef(V, UnicodeString());
}
//---------------------------------------------------------------------------
UnicodeString VarToStrDef(const Variant & V, const UnicodeString ADefault)
{
  return (V.IsNull() || V.IsEmpty()) ? ADefault : V.AsString();
}
//---------------------------------------------------------------------------
Variant VarAsType(const Variant & V, TVarType AVarType)
{
  if (V.Type() == AVarType)
  {
    return V;
  }
  switch (AVarType)
  {
    case varEmpty: return Variant();
    case varNull: return Null();
    case varBoolean: return Variant(static_cast<bool>(V));
    case varSmallint: return Variant(static_cast<short>(V.AsInt64()));
    case varShortInt:
    case varByte:
    case varInteger: return Variant(static_cast<int>(V.AsInt64()));
    case varWord: return Variant(static_cast<unsigned short>(V.AsInt64()));
    case varLongWord:
      {
        long long Value = V.AsInt64();
        if ((Value < 0) || (Value > std::numeric_limits<unsigned int>::max()))
        {
          OverflowError(V.Type(), AVarType);
        }
        return Variant(static_cast<unsigned int>(Value));
      }
    case varInt64: return Variant(V.AsInt64());
    case varUInt64: return Variant(static_cast<unsigned long long>(V.AsInt64()));
    case varSingle: return Variant(static_cast<float>(V.AsDouble()));
    case varDouble:
    case varCurrency: return Variant(V.AsDouble());
    case varDate: return Variant(TDateTime(V.AsDouble()));
    case varUString:
    case varString:
    case varOleStr: return Variant(V.AsString());
    case varVariant: return V;
  }
  ConversionError(V.Type(), AVarType);
}
//---------------------------------------------------------------------------
Variant VarArrayCreate(const int * Bounds, const int Bounds_High, TVarType AVarType)
{
  return Variant(Bounds, Bounds_High, AVarType);
}
//---------------------------------------------------------------------------
int VarArrayLowBound(const Variant & A, int Dim)
{
  return A.ArrayLowBound(Dim);
}
//---------------------------------------------------------------------------
int VarArrayHighBound(const Variant & A, int Dim)
{
  return A.ArrayHighBound(Dim);
}
//---------------------------------------------------------------------------
void VarArrayRedim(Variant & A, int HighBound)
{
  A.ArrayRedim(HighBound);
}
//---------------------------------------------------------------------------
} // namespace Variants
} // namespace System
