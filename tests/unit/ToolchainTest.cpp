// Verifies the compiler features the port relies on (docs/port/PLAN.md, section 3.2).
// If any of these fail, the property conversion strategy for the core does not work
// with the current toolchain.

#include <gtest/gtest.h>

#include <map>
#include <string>

namespace
{

class TPropertyHolder
{
public:
  int GetCount() const { return FCount; }
  void SetCount(int Value) { FCount = Value; FSetterCalls++; }

  std::wstring GetName(int Index) const
  {
    auto I = FNames.find(Index);
    return (I != FNames.end()) ? I->second : std::wstring();
  }
  void SetName(int Index, const std::wstring & Value) { FNames[Index] = Value; }

  // Field-backed read, as produced by bcb2clang for `read = FField`
  int ReadOnlyField() const { return FReadOnly; }

  __declspec(property(get = GetCount, put = SetCount)) int Count;
  __declspec(property(get = GetName, put = SetName)) std::wstring Names[];
  __declspec(property(get = ReadOnlyField)) int ReadOnly;

  int FSetterCalls = 0;

private:
  int FCount = 0;
  int FReadOnly = 42;
  std::map<int, std::wstring> FNames;
};

} // namespace

TEST(Toolchain, DeclspecPropertyReadWrite)
{
  TPropertyHolder Holder;
  Holder.Count = 5;
  EXPECT_EQ(Holder.Count, 5);
  Holder.Count += 2;
  EXPECT_EQ(Holder.Count, 7);
  EXPECT_EQ(Holder.FSetterCalls, 2);
}

TEST(Toolchain, DeclspecPropertyThroughPointer)
{
  TPropertyHolder Holder;
  TPropertyHolder * Pointer = &Holder;
  Pointer->Count = 3;
  EXPECT_EQ(Pointer->Count, 3);
  EXPECT_EQ(Pointer->ReadOnly, 42);
}

TEST(Toolchain, DeclspecIndexedProperty)
{
  TPropertyHolder Holder;
  Holder.Names[1] = L"first";
  Holder.Names[7] = L"seventh";
  EXPECT_EQ(Holder.Names[1], L"first");
  EXPECT_EQ(Holder.Names[7], L"seventh");
  EXPECT_EQ(Holder.Names[2], L"");
}

TEST(Toolchain, WideCharIsUtf32)
{
  // UnicodeString is built on wchar_t; on Linux that is UTF-32 (plan section 3.2).
  static_assert(sizeof(wchar_t) == 4);
  const wchar_t * NonBmp = L"\U0001F600";
  EXPECT_EQ(std::wstring(NonBmp).size(), 1u);
}
