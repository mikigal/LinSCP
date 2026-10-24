// System::UnicodeString with the C++Builder API (1-based indexes, Delphi semantics).
//
// Unlike Delphi's reference-counted copy-on-write string this is a value type
// over std::wstring; the difference is not observable through the API. Methods
// and operators mirror C++Builder's ustring.h, so that overload resolution in the
// ported code behaves the same.
//---------------------------------------------------------------------------
#ifndef RtlUnicodeStringH
#define RtlUnicodeStringH
//---------------------------------------------------------------------------
#include <rtl/Defs.h>
#include <rtl/Conversion.h>

#include <cstdarg>
#include <functional>
#include <iosfwd>
#include <string>
//---------------------------------------------------------------------------
namespace System
{
class TVarRec;
template <unsigned short CodePage> class AnsiStringT;
//---------------------------------------------------------------------------
class UnicodeString
{
public:
  typedef wchar_t * iterator;
  typedef const wchar_t * const_iterator;

  UnicodeString() noexcept = default;
  UnicodeString(const UnicodeString & Src) = default;
  UnicodeString(UnicodeString && Src) noexcept = default;
  UnicodeString(const wchar_t * Src);
  UnicodeString(const wchar_t * Src, int Len);
  UnicodeString(const char * Src);
  UnicodeString(const char * Src, int Len);
  UnicodeString(char Src);
  UnicodeString(wchar_t Src);
  UnicodeString(short Src);
  UnicodeString(unsigned short Src);
  UnicodeString(int Src);
  UnicodeString(unsigned int Src);
  UnicodeString(long Src);
  UnicodeString(unsigned long Src);
  UnicodeString(long long Src);
  UnicodeString(unsigned long long Src);
  UnicodeString(float Src);
  UnicodeString(double Src);
  UnicodeString(long double Src);
  template <unsigned short CodePage>
  UnicodeString(const AnsiStringT<CodePage> & Src);
  explicit UnicodeString(const std::wstring & Src) : FData(Src) {}
  explicit UnicodeString(std::wstring && Src) noexcept : FData(std::move(Src)) {}
  ~UnicodeString() = default;

  UnicodeString & operator =(const UnicodeString & Rhs) = default;
  UnicodeString & operator =(UnicodeString && Rhs) noexcept = default;
  UnicodeString & operator +=(const UnicodeString & Rhs);
  UnicodeString & operator +=(const wchar_t * Rhs);
  UnicodeString & operator +=(wchar_t Rhs);

  UnicodeString operator +(const UnicodeString & Rhs) const;
  UnicodeString operator +(const wchar_t * Rhs) const;
  friend UnicodeString operator +(const wchar_t * Lhs, const UnicodeString & Rhs);
  friend UnicodeString operator +(const char * Lhs, const UnicodeString & Rhs);

  // Ordinal comparison, as C++Builder's operators
  bool operator ==(const UnicodeString & Rhs) const { return FData == Rhs.FData; }
  bool operator !=(const UnicodeString & Rhs) const { return FData != Rhs.FData; }
  bool operator <(const UnicodeString & Rhs) const { return FData < Rhs.FData; }
  bool operator >(const UnicodeString & Rhs) const { return FData > Rhs.FData; }
  bool operator <=(const UnicodeString & Rhs) const { return FData <= Rhs.FData; }
  bool operator >=(const UnicodeString & Rhs) const { return FData >= Rhs.FData; }
  bool operator ==(const wchar_t * Rhs) const;
  bool operator !=(const wchar_t * Rhs) const { return !operator ==(Rhs); }
  friend bool operator ==(const wchar_t * Lhs, const UnicodeString & Rhs) { return Rhs == Lhs; }
  friend bool operator !=(const wchar_t * Lhs, const UnicodeString & Rhs) { return !(Rhs == Lhs); }

  // 1-based; out of range raises ERangeError
  wchar_t & operator [](int Index);
  wchar_t operator [](int Index) const;

  int Length() const { return static_cast<int>(FData.size()); }
  bool IsEmpty() const { return FData.empty(); }
  // Never NULL; writable up to Length() (C++Builder returns a non-const pointer)
  wchar_t * c_str() const { return FData.data(); }
  wchar_t * w_str() const { return FData.data(); }
  wchar_t * t_str() const { return FData.data(); }
  const void * data() const { return FData.data(); }

  UnicodeString & SetLength(int NewLength);
  UnicodeString & Unique() { return *this; }
  UnicodeString & Insert(const UnicodeString & Str, int Index);
  UnicodeString & Delete(int Index, int Count);
  UnicodeString & swap(UnicodeString & Other) { FData.swap(Other.FData); return *this; }

  int Pos(const UnicodeString & SubStr) const;
  int AnsiPos(const UnicodeString & SubStr) const { return Pos(SubStr); }
  // Delphi's Copy() semantics: no range errors
  UnicodeString SubString(int Index, int Count = MaxInt) const;

  UnicodeString Trim() const;
  UnicodeString TrimLeft() const;
  UnicodeString TrimRight() const;
  UnicodeString LowerCase() const;
  UnicodeString UpperCase() const;

  bool IsDelimiter(const UnicodeString & Delimiters, int Index) const;
  int LastDelimiter(const UnicodeString & Delimiters) const;
  bool IsPathDelimiter(int Index) const;
  bool IsLeadSurrogate(int Index) const;
  bool IsTrailSurrogate(int Index) const;

  int Compare(const UnicodeString & Rhs) const;
  int CompareIC(const UnicodeString & Rhs) const;
  int AnsiCompare(const UnicodeString & Rhs) const { return Compare(Rhs); }
  int AnsiCompareIC(const UnicodeString & Rhs) const { return CompareIC(Rhs); }

  int ToInt() const;
  int ToIntDef(int Default) const;
  double ToDouble() const;

  // printf family with Windows/C++Builder wide conventions: %s is a wide string,
  // %hs a narrow one, %Ld/%I64d a 64-bit integer
  UnicodeString & sprintf(const wchar_t * Format, ...);
  int printf(const wchar_t * Format, ...);
  int vprintf(const wchar_t * Format, va_list Args);
  UnicodeString & cat_sprintf(const wchar_t * Format, ...);
  int cat_printf(const wchar_t * Format, ...);
  int cat_vprintf(const wchar_t * Format, va_list Args);

  static UnicodeString StringOfChar(wchar_t Ch, int Count);
  static UnicodeString LoadStr(int Ident);
  static UnicodeString FmtLoadStr(int Ident, const TVarRec * Args, int Size);
  static UnicodeString Format(const UnicodeString & Format, const TVarRec * Args, int Size);
  static UnicodeString IntToHex(int Value, int Digits);

  int ElementSize() const { return sizeof(wchar_t); }
  unsigned short CodePage() const { return 1200; }

  iterator begin() { return FData.data(); }
  iterator end() { return FData.data() + FData.size(); }
  const_iterator begin() const { return FData.data(); }
  const_iterator end() const { return FData.data() + FData.size(); }

  // Access to the underlying storage for the RTL implementation and tests
  const std::wstring & Std() const { return FData; }
  std::wstring & Std() { return FData; }

private:
  mutable std::wstring FData;
};
//---------------------------------------------------------------------------
std::ostream & operator <<(std::ostream & Stream, const UnicodeString & Str);
std::wostream & operator <<(std::wostream & Stream, const UnicodeString & Str);
// For GoogleTest failure messages
void PrintTo(const UnicodeString & Str, std::ostream * Stream);
//---------------------------------------------------------------------------
typedef UnicodeString String;
typedef UnicodeString TFileName;
//---------------------------------------------------------------------------
// System.Pos: the 1-based index of SubStr in S from Offset, 0 if none
int Pos(const UnicodeString & SubStr, const UnicodeString & S, int Offset = 1);
} // namespace System
//---------------------------------------------------------------------------
namespace std
{
template <>
struct hash<System::UnicodeString>
{
  size_t operator()(const System::UnicodeString & Str) const noexcept
  {
    return std::hash<std::wstring>()(Str.Std());
  }
};
} // namespace std
//---------------------------------------------------------------------------
#endif
