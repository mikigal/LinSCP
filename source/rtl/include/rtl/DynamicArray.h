// System::DynamicArray, Delphi's dynamic array (as in C++Builder's sysdyn.h).
// Like in Delphi, copies share the elements; setting Length makes the array unique.
//---------------------------------------------------------------------------
#ifndef RtlDynamicArrayH
#define RtlDynamicArrayH
//---------------------------------------------------------------------------
#include <rtl/UnicodeString.h>

#include <initializer_list>
#include <memory>
#include <vector>
//---------------------------------------------------------------------------
namespace rtl
{
[[noreturn]] void ThrowRangeError(int Index, int Length);
}
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
template <class T>
class DynamicArray
{
public:
  typedef T * iterator;
  typedef const T * const_iterator;

  DynamicArray() = default;
  DynamicArray(const DynamicArray & Src) = default;
  DynamicArray(std::initializer_list<T> Values) :
    FData(std::make_shared<std::vector<T>>(Values))
  {
  }
  DynamicArray(const T * Values, int Count) :
    FData(std::make_shared<std::vector<T>>(Values, Values + ((Count > 0) ? Count : 0)))
  {
  }
  DynamicArray & operator =(const DynamicArray & Rhs) = default;

  // Identity, as in C++Builder: true when both refer to the same elements
  bool operator ==(const DynamicArray & Rhs) const { return (FData == Rhs.FData); }
  bool operator !=(const DynamicArray & Rhs) const { return (FData != Rhs.FData); }

  T & operator [](int Index)
  {
    CheckIndex(Index);
    return (*FData)[Index];
  }
  const T & operator [](int Index) const
  {
    CheckIndex(Index);
    return (*FData)[Index];
  }

  int GetLength() const { return FData ? static_cast<int>(FData->size()) : 0; }
  void SetLength(int NewLength)
  {
    if (NewLength <= 0)
    {
      FData.reset();
    }
    else
    {
      std::shared_ptr<std::vector<T>> NewData =
        FData ? std::make_shared<std::vector<T>>(*FData) : std::make_shared<std::vector<T>>();
      NewData->resize(NewLength);
      FData = NewData;
    }
  }
  int GetHigh() const { return GetLength() - 1; }
  int GetLow() const { return 0; }

  RTL_PROPERTY_GET(Length, GetLength) RTL_PROPERTY_PUT(Length, SetLength, int)
  __declspec(property(get = __property_get_Length, put = __property_set_Length)) int Length;
  RTL_PROPERTY_GET(High, GetHigh)
  __declspec(property(get = __property_get_High)) int High;
  RTL_PROPERTY_GET(Low, GetLow)
  __declspec(property(get = __property_get_Low)) int Low;

  DynamicArray Copy() const { return CopyRange(0, GetLength()); }
  DynamicArray CopyRange(int StartIndex, int Count) const
  {
    DynamicArray Result;
    int Len = GetLength();
    if (StartIndex < 0)
    {
      StartIndex = 0;
    }
    if (Count > Len - StartIndex)
    {
      Count = Len - StartIndex;
    }
    if (Count > 0)
    {
      Result.FData = std::make_shared<std::vector<T>>(
        FData->begin() + StartIndex, FData->begin() + StartIndex + Count);
    }
    return Result;
  }

  T * data() { return FData ? FData->data() : nullptr; }
  const T * data() const { return FData ? FData->data() : nullptr; }
  iterator begin() { return data(); }
  iterator end() { return data() + GetLength(); }
  const_iterator begin() const { return data(); }
  const_iterator end() const { return data() + GetLength(); }

private:
  void CheckIndex(int Index) const
  {
    if ((Index < 0) || (Index >= GetLength()))
    {
      rtl::ThrowRangeError(Index, GetLength());
    }
  }

  std::shared_ptr<std::vector<T>> FData;
};
//---------------------------------------------------------------------------
typedef DynamicArray<Byte> TBytes;
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#endif
