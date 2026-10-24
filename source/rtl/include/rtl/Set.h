// System::Set, Delphi's set type (as in C++Builder's sysset.h).
//---------------------------------------------------------------------------
#ifndef RtlSetH
#define RtlSetH
//---------------------------------------------------------------------------
#include <bitset>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
template <class T, unsigned char MinEl, unsigned char MaxEl>
class Set
{
public:
  Set() = default;
  Set(const Set & Src) = default;
  Set & operator =(const Set & Rhs) = default;

  Set & operator +=(const Set & Rhs) { FBits |= Rhs.FBits; return *this; }
  Set & operator -=(const Set & Rhs) { FBits &= ~Rhs.FBits; return *this; }
  Set & operator *=(const Set & Rhs) { FBits &= Rhs.FBits; return *this; }
  Set operator +(const Set & Rhs) const { Set Result(*this); Result += Rhs; return Result; }
  Set operator -(const Set & Rhs) const { Set Result(*this); Result -= Rhs; return Result; }
  Set operator *(const Set & Rhs) const { Set Result(*this); Result *= Rhs; return Result; }

  Set & operator <<(const T El)
  {
    if (InRange(El))
    {
      FBits.set(Bit(El));
    }
    return *this;
  }
  Set & operator >>(const T El)
  {
    if (InRange(El))
    {
      FBits.reset(Bit(El));
    }
    return *this;
  }

  bool Contains(const T El) const { return InRange(El) && FBits.test(Bit(El)); }
  bool operator ==(const Set & Rhs) const { return (FBits == Rhs.FBits); }
  bool operator !=(const Set & Rhs) const { return (FBits != Rhs.FBits); }
  Set & Clear() { FBits.reset(); return *this; }
  bool Empty() const { return FBits.none(); }

  // Delphi's in-memory layout: bit (El - MinEl) of the lowest bytes
  unsigned long long ToInt() const { return FBits.to_ullong(); }
  static Set FromInt(unsigned long long Value)
  {
    Set Result;
    Result.FBits = std::bitset<Size>(Value);
    return Result;
  }

private:
  static const unsigned int Size = static_cast<unsigned int>(MaxEl) - MinEl + 1;

  static bool InRange(const T El)
  {
    return (static_cast<int>(El) >= MinEl) && (static_cast<int>(El) <= MaxEl);
  }
  static size_t Bit(const T El)
  {
    return static_cast<size_t>(static_cast<int>(El) - MinEl);
  }

  std::bitset<Size> FBits;
};
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#endif
