// System::AnsiStringT<CodePage> and its typedefs AnsiString, UTF8String and
// RawByteString, with the C++Builder API.
//
// The ANSI code page is UTF-8 on Linux, so AnsiString, UTF8String and
// RawByteString all hold UTF-8 and convert to each other by copying bytes.
// Other code pages convert through UnicodeString.
//---------------------------------------------------------------------------
#ifndef RtlAnsiStringH
#define RtlAnsiStringH
//---------------------------------------------------------------------------
#include <rtl/UnicodeString.h>

#include <cstring>
#include <string>
//---------------------------------------------------------------------------
namespace rtl
{
// Shared implementation of AnsiStringT, on the bytes
std::string AnsiTrim(const std::string & Str, bool Left, bool Right);
std::string AnsiChangeCase(const std::string & Str, unsigned int CodePage, bool Upper);
std::string AnsiFormatV(const char * Format, va_list Args);
std::string AnsiFormat(const std::string & Format, const System::TVarRec * Args, int Size);
std::string AnsiNumber(long long Value);
std::string AnsiNumber(unsigned long long Value);
std::string AnsiNumber(long double Value);
int AnsiToInt(const std::string & Str);
int AnsiToIntDef(const std::string & Str, int Default);
double AnsiToDouble(const std::string & Str);
[[noreturn]] void ThrowRangeError(int Index, int Length);
} // namespace rtl
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
class AnsiStringBase
{
public:
  int Length() const { return static_cast<int>(FData.size()); }
  bool IsEmpty() const { return FData.empty(); }
  char * c_str() const { return FData.data(); }
  // NULL for an empty string, as in C++Builder (whose empty strings have no
  // data); upstream code relies on it
  const void * data() const { return FData.empty() ? nullptr : FData.data(); }
  int ElementSize() const { return 1; }

  const std::string & Std() const { return FData; }
  std::string & Std() { return FData; }

protected:
  AnsiStringBase() = default;
  explicit AnsiStringBase(const std::string & Data) : FData(Data) {}
  explicit AnsiStringBase(std::string && Data) noexcept : FData(std::move(Data)) {}

  int DoPos(const char * SubStr, size_t SubLength) const;
  void DoInsert(const char * Str, size_t StrLength, int Index);
  void DoDelete(int Index, int Count);
  std::string DoSubString(int Index, int Count) const;
  bool DoIsDelimiter(const std::string & Delimiters, int Index) const;
  int DoLastDelimiter(const std::string & Delimiters) const;

  mutable std::string FData;
};
//---------------------------------------------------------------------------
template <unsigned short CP>
class AnsiStringT : public AnsiStringBase
{
public:
  typedef char * iterator;
  typedef const char * const_iterator;

  AnsiStringT() = default;
  AnsiStringT(const AnsiStringT & Src) = default;
  AnsiStringT(AnsiStringT && Src) noexcept = default;
  template <unsigned short SrcCP>
  AnsiStringT(const AnsiStringT<SrcCP> & Src)
  {
    if ((CP == SrcCP) || (CP == rtl::RawByteCodePage) || (SrcCP == rtl::RawByteCodePage) ||
        (rtl::IsUtf8CodePage(CP) && rtl::IsUtf8CodePage(SrcCP)))
    {
      FData = Src.Std();
    }
    else
    {
      std::wstring Wide = rtl::DecodeCodePage(SrcCP, Src.c_str(), Src.Length());
      FData = rtl::EncodeCodePage(CP, Wide.data(), Wide.size());
    }
  }
  AnsiStringT(const char * Src) : AnsiStringBase(std::string((Src != nullptr) ? Src : "")) {}
  AnsiStringT(const char * Src, int Len) :
    AnsiStringBase(((Src != nullptr) && (Len > 0)) ? std::string(Src, Len) : std::string()) {}
  AnsiStringT(const wchar_t * Src) : AnsiStringT(UnicodeString(Src)) {}
  AnsiStringT(const wchar_t * Src, int Len) : AnsiStringT(UnicodeString(Src, Len)) {}
  AnsiStringT(const UnicodeString & Src) :
    AnsiStringBase(rtl::EncodeCodePage(CP, Src.c_str(), Src.Length())) {}
  AnsiStringT(char Src) : AnsiStringBase(std::string(1, Src)) {}
  AnsiStringT(wchar_t Src) : AnsiStringT(UnicodeString(Src)) {}
  AnsiStringT(short Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<long long>(Src))) {}
  AnsiStringT(unsigned short Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<long long>(Src))) {}
  AnsiStringT(int Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<long long>(Src))) {}
  AnsiStringT(unsigned int Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<unsigned long long>(Src))) {}
  AnsiStringT(long Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<long long>(Src))) {}
  AnsiStringT(unsigned long Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<unsigned long long>(Src))) {}
  AnsiStringT(long long Src) : AnsiStringBase(rtl::AnsiNumber(Src)) {}
  AnsiStringT(unsigned long long Src) : AnsiStringBase(rtl::AnsiNumber(Src)) {}
  AnsiStringT(double Src) : AnsiStringBase(rtl::AnsiNumber(static_cast<long double>(Src))) {}
  AnsiStringT(long double Src) : AnsiStringBase(rtl::AnsiNumber(Src)) {}
  explicit AnsiStringT(const std::string & Src) : AnsiStringBase(Src) {}
  explicit AnsiStringT(std::string && Src) noexcept : AnsiStringBase(std::move(Src)) {}
  ~AnsiStringT() = default;

  AnsiStringT & operator =(const AnsiStringT & Rhs) = default;
  AnsiStringT & operator =(AnsiStringT && Rhs) noexcept = default;
  AnsiStringT & operator +=(const AnsiStringT & Rhs) { FData += Rhs.FData; return *this; }
  AnsiStringT & operator +=(const char * Rhs) { if (Rhs != nullptr) FData += Rhs; return *this; }
  AnsiStringT & operator +=(char Rhs) { FData += Rhs; return *this; }

  AnsiStringT operator +(const AnsiStringT & Rhs) const { return AnsiStringT(FData + Rhs.FData); }
  AnsiStringT operator +(const char * Rhs) const { AnsiStringT Result(*this); Result += Rhs; return Result; }
  friend AnsiStringT operator +(const char * Lhs, const AnsiStringT & Rhs)
  {
    return AnsiStringT(std::string((Lhs != nullptr) ? Lhs : "") + Rhs.FData);
  }

  bool operator ==(const AnsiStringT & Rhs) const { return FData == Rhs.FData; }
  bool operator !=(const AnsiStringT & Rhs) const { return FData != Rhs.FData; }
  bool operator <(const AnsiStringT & Rhs) const { return FData < Rhs.FData; }
  bool operator >(const AnsiStringT & Rhs) const { return FData > Rhs.FData; }
  bool operator <=(const AnsiStringT & Rhs) const { return FData <= Rhs.FData; }
  bool operator >=(const AnsiStringT & Rhs) const { return FData >= Rhs.FData; }
  bool operator ==(const char * Rhs) const { return FData == ((Rhs != nullptr) ? Rhs : ""); }
  bool operator !=(const char * Rhs) const { return !operator ==(Rhs); }
  friend bool operator ==(const char * Lhs, const AnsiStringT & Rhs) { return Rhs == Lhs; }
  friend bool operator !=(const char * Lhs, const AnsiStringT & Rhs) { return !(Rhs == Lhs); }

  char & operator [](int Index)
  {
    if ((Index < 1) || (Index > Length()))
    {
      rtl::ThrowRangeError(Index, Length());
    }
    return FData[Index - 1];
  }
  char operator [](int Index) const
  {
    if ((Index < 1) || (Index > Length()))
    {
      rtl::ThrowRangeError(Index, Length());
    }
    return FData[Index - 1];
  }

  AnsiStringT & SetLength(int NewLength) { FData.resize((NewLength > 0) ? NewLength : 0); return *this; }
  AnsiStringT & Unique() { return *this; }
  AnsiStringT & Insert(const AnsiStringT & Str, int Index) { DoInsert(Str.FData.data(), Str.FData.size(), Index); return *this; }
  AnsiStringT & Delete(int Index, int Count) { DoDelete(Index, Count); return *this; }
  AnsiStringT & swap(AnsiStringT & Other) { FData.swap(Other.FData); return *this; }

  int Pos(const AnsiStringT & SubStr) const { return DoPos(SubStr.FData.data(), SubStr.FData.size()); }
  int AnsiPos(const AnsiStringT & SubStr) const { return Pos(SubStr); }
  AnsiStringT SubString(int Index, int Count = MaxInt) const { return AnsiStringT(DoSubString(Index, Count)); }

  AnsiStringT Trim() const { return AnsiStringT(rtl::AnsiTrim(FData, true, true)); }
  AnsiStringT TrimLeft() const { return AnsiStringT(rtl::AnsiTrim(FData, true, false)); }
  AnsiStringT TrimRight() const { return AnsiStringT(rtl::AnsiTrim(FData, false, true)); }
  AnsiStringT LowerCase() const { return AnsiStringT(rtl::AnsiChangeCase(FData, CP, false)); }
  AnsiStringT UpperCase() const { return AnsiStringT(rtl::AnsiChangeCase(FData, CP, true)); }

  bool IsDelimiter(const AnsiStringT & Delimiters, int Index) const { return DoIsDelimiter(Delimiters.FData, Index); }
  int LastDelimiter(const AnsiStringT & Delimiters) const { return DoLastDelimiter(Delimiters.FData); }
  bool IsPathDelimiter(int Index) const { return DoIsDelimiter("/", Index); }

  int ToInt() const { return rtl::AnsiToInt(FData); }
  int ToIntDef(int Default) const { return rtl::AnsiToIntDef(FData, Default); }
  double ToDouble() const { return rtl::AnsiToDouble(FData); }

  AnsiStringT & sprintf(const char * Format, ...)
  {
    va_list Args;
    va_start(Args, Format);
    FData = rtl::AnsiFormatV(Format, Args);
    va_end(Args);
    return *this;
  }
  int printf(const char * Format, ...)
  {
    va_list Args;
    va_start(Args, Format);
    FData = rtl::AnsiFormatV(Format, Args);
    va_end(Args);
    return Length();
  }
  int vprintf(const char * Format, va_list Args)
  {
    FData = rtl::AnsiFormatV(Format, Args);
    return Length();
  }
  AnsiStringT & cat_sprintf(const char * Format, ...)
  {
    va_list Args;
    va_start(Args, Format);
    FData += rtl::AnsiFormatV(Format, Args);
    va_end(Args);
    return *this;
  }
  int cat_printf(const char * Format, ...)
  {
    va_list Args;
    va_start(Args, Format);
    std::string Added = rtl::AnsiFormatV(Format, Args);
    va_end(Args);
    FData += Added;
    return static_cast<int>(Added.size());
  }

  static AnsiStringT StringOfChar(char Ch, int Count) { return AnsiStringT(std::string((Count > 0) ? Count : 0, Ch)); }
  static AnsiStringT LoadStr(int Ident) { return AnsiStringT(UnicodeString::LoadStr(Ident)); }
  static AnsiStringT FmtLoadStr(int Ident, const TVarRec * Args, int Size)
  {
    return AnsiStringT(UnicodeString::FmtLoadStr(Ident, Args, Size));
  }
  static AnsiStringT Format(const AnsiStringT & Format, const TVarRec * Args, int Size)
  {
    return AnsiStringT(rtl::AnsiFormat(Format.FData, Args, Size));
  }

  unsigned short CodePage() const
  {
    return static_cast<unsigned short>((CP == 0) ? rtl::AnsiCodePage : CP);
  }

  iterator begin() { return FData.data(); }
  iterator end() { return FData.data() + FData.size(); }
  const_iterator begin() const { return FData.data(); }
  const_iterator end() const { return FData.data() + FData.size(); }
};
//---------------------------------------------------------------------------
typedef AnsiStringT<0> AnsiString;
typedef AnsiStringT<65001> UTF8String;
typedef AnsiStringT<0xFFFF> RawByteString;
//---------------------------------------------------------------------------
template <unsigned short CodePage>
UnicodeString::UnicodeString(const AnsiStringT<CodePage> & Src) :
  FData(rtl::DecodeCodePage(CodePage, Src.c_str(), Src.Length()))
{
}
//---------------------------------------------------------------------------
std::ostream & operator <<(std::ostream & Stream, const AnsiStringBase & Str);
void PrintTo(const AnsiStringBase & Str, std::ostream * Stream);
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#endif
