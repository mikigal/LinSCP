// The System.Types unit: TDuplicates, geometry types, dynamic array types.
//---------------------------------------------------------------------------
#ifndef RtlTypesH
#define RtlTypesH
//---------------------------------------------------------------------------
#include <rtl/System.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Types
{
//---------------------------------------------------------------------------
enum TDuplicates { dupIgnore, dupAccept, dupError };

typedef signed char TValueRelationship;
const TValueRelationship LessThanValue = -1;
const TValueRelationship EqualsValue = 0;
const TValueRelationship GreaterThanValue = 1;

struct TPoint
{
  int X = 0;
  int Y = 0;
  TPoint() = default;
  TPoint(int AX, int AY) : X(AX), Y(AY) {}
  bool operator ==(const TPoint & Rhs) const { return (X == Rhs.X) && (Y == Rhs.Y); }
  bool operator !=(const TPoint & Rhs) const { return !(*this == Rhs); }
};

struct TSize
{
  int cx = 0;
  int cy = 0;
  TSize() = default;
  TSize(int ACx, int ACy) : cx(ACx), cy(ACy) {}
  int GetWidth() const { return cx; }
  int GetHeight() const { return cy; }
  __declspec(property(get = GetWidth)) int Width;
  __declspec(property(get = GetHeight)) int Height;
  bool operator ==(const TSize & Rhs) const { return (cx == Rhs.cx) && (cy == Rhs.cy); }
  bool operator !=(const TSize & Rhs) const { return !(*this == Rhs); }
};

struct TRect
{
  int Left = 0;
  int Top = 0;
  int Right = 0;
  int Bottom = 0;
  TRect() = default;
  TRect(int ALeft, int ATop, int ARight, int ABottom) : Left(ALeft), Top(ATop), Right(ARight), Bottom(ABottom) {}
  int Width() const { return Right - Left; }
  int Height() const { return Bottom - Top; }
  bool operator ==(const TRect & Rhs) const
  {
    return (Left == Rhs.Left) && (Top == Rhs.Top) && (Right == Rhs.Right) && (Bottom == Rhs.Bottom);
  }
  bool operator !=(const TRect & Rhs) const { return !(*this == Rhs); }
};

inline TPoint Point(int AX, int AY) { return TPoint(AX, AY); }
inline TRect Rect(int ALeft, int ATop, int ARight, int ABottom) { return TRect(ALeft, ATop, ARight, ABottom); }
inline TRect Bounds(int ALeft, int ATop, int AWidth, int AHeight) { return TRect(ALeft, ATop, ALeft + AWidth, ATop + AHeight); }

typedef DynamicArray<UnicodeString> TStringDynArray;
typedef DynamicArray<int> TIntegerDynArray;
typedef DynamicArray<Byte> TByteDynArray;
//---------------------------------------------------------------------------
} // namespace Types
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Types;
//---------------------------------------------------------------------------
#endif
