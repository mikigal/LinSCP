// UnicodeString and AnsiStringT implementation
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
#include <rtl/Printf.h>

#include <ostream>
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
void ThrowRangeError(int Index, int Length)
{
  throw ERangeError(Format(L"Range check error (index %d, length %d)", ARRAYOFCONST((Index, Length))));
}
//---------------------------------------------------------------------------
std::string AnsiTrim(const std::string & Str, bool Left, bool Right)
{
  size_t Start = 0;
  size_t End = Str.size();
  if (Left)
  {
    while ((Start < End) && (static_cast<unsigned char>(Str[Start]) <= ' '))
    {
      Start++;
    }
  }
  if (Right)
  {
    while ((End > Start) && (static_cast<unsigned char>(Str[End - 1]) <= ' '))
    {
      End--;
    }
  }
  return Str.substr(Start, End - Start);
}
//---------------------------------------------------------------------------
std::string AnsiChangeCase(const std::string & Str, unsigned int CodePage, bool Upper)
{
  std::wstring Wide = DecodeCodePage(CodePage, Str.data(), Str.size());
  for (wchar_t & Ch : Wide)
  {
    Ch = Upper ? UpperChar(Ch) : LowerChar(Ch);
  }
  return EncodeCodePage(CodePage, Wide.data(), Wide.size());
}
//---------------------------------------------------------------------------
std::string AnsiFormat(const std::string & Format, const System::TVarRec * Args, int Size)
{
  System::UnicodeString Result =
    System::Sysutils::Format(System::UnicodeString(DecodeUtf8(Format.data(), Format.size())), Args, Size);
  return EncodeUtf8(Result.c_str(), Result.Length());
}
//---------------------------------------------------------------------------
std::string AnsiNumber(long long Value)
{
  return std::to_string(Value);
}
//---------------------------------------------------------------------------
std::string AnsiNumber(unsigned long long Value)
{
  return std::to_string(Value);
}
//---------------------------------------------------------------------------
std::string AnsiNumber(long double Value)
{
  System::UnicodeString Result = FloatToStr(Value);
  return EncodeUtf8(Result.c_str(), Result.Length());
}
//---------------------------------------------------------------------------
int AnsiToInt(const std::string & Str)
{
  return StrToInt(System::UnicodeString(DecodeUtf8(Str.data(), Str.size())));
}
//---------------------------------------------------------------------------
int AnsiToIntDef(const std::string & Str, int Default)
{
  return StrToIntDef(System::UnicodeString(DecodeUtf8(Str.data(), Str.size())), Default);
}
//---------------------------------------------------------------------------
double AnsiToDouble(const std::string & Str)
{
  return static_cast<double>(StrToFloat(System::UnicodeString(DecodeUtf8(Str.data(), Str.size()))));
}
//---------------------------------------------------------------------------
} // namespace rtl
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(const wchar_t * Src) :
  FData((Src != nullptr) ? Src : L"")
{
}
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(const wchar_t * Src, int Len)
{
  if ((Src != nullptr) && (Len > 0))
  {
    FData.assign(Src, Len);
  }
}
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(const char * Src)
{
  if (Src != nullptr)
  {
    FData = rtl::DecodeCodePage(rtl::AnsiCodePage, Src, strlen(Src));
  }
}
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(const char * Src, int Len)
{
  if ((Src != nullptr) && (Len > 0))
  {
    FData = rtl::DecodeCodePage(rtl::AnsiCodePage, Src, Len);
  }
}
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(char Src) :
  FData(rtl::DecodeCodePage(rtl::AnsiCodePage, &Src, 1))
{
}
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(wchar_t Src) :
  FData(1, Src)
{
}
//---------------------------------------------------------------------------
UnicodeString::UnicodeString(short Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(unsigned short Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(int Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(unsigned int Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(long Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(unsigned long Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(long long Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(unsigned long long Src) : FData(std::to_wstring(Src)) {}
UnicodeString::UnicodeString(float Src) : UnicodeString(FloatToStr(Src)) {}
UnicodeString::UnicodeString(double Src) : UnicodeString(FloatToStr(Src)) {}
UnicodeString::UnicodeString(long double Src) : UnicodeString(FloatToStr(Src)) {}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::operator +=(const UnicodeString & Rhs)
{
  FData += Rhs.FData;
  return *this;
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::operator +=(const wchar_t * Rhs)
{
  if (Rhs != nullptr)
  {
    FData += Rhs;
  }
  return *this;
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::operator +=(wchar_t Rhs)
{
  FData += Rhs;
  return *this;
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::operator +(const UnicodeString & Rhs) const
{
  return UnicodeString(FData + Rhs.FData);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::operator +(const wchar_t * Rhs) const
{
  UnicodeString Result(*this);
  Result += Rhs;
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString operator +(const wchar_t * Lhs, const UnicodeString & Rhs)
{
  UnicodeString Result(Lhs);
  Result += Rhs;
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString operator +(const char * Lhs, const UnicodeString & Rhs)
{
  UnicodeString Result(Lhs);
  Result += Rhs;
  return Result;
}
//---------------------------------------------------------------------------
bool UnicodeString::operator ==(const wchar_t * Rhs) const
{
  return (FData == ((Rhs != nullptr) ? Rhs : L""));
}
//---------------------------------------------------------------------------
wchar_t & UnicodeString::operator [](int Index)
{
  if ((Index < 1) || (Index > Length()))
  {
    rtl::ThrowRangeError(Index, Length());
  }
  return FData[Index - 1];
}
//---------------------------------------------------------------------------
wchar_t UnicodeString::operator [](int Index) const
{
  if ((Index < 1) || (Index > Length()))
  {
    rtl::ThrowRangeError(Index, Length());
  }
  return FData[Index - 1];
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::SetLength(int NewLength)
{
  FData.resize((NewLength > 0) ? NewLength : 0);
  return *this;
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::Insert(const UnicodeString & Str, int Index)
{
  if (Index < 1)
  {
    Index = 1;
  }
  else if (Index > Length() + 1)
  {
    Index = Length() + 1;
  }
  FData.insert(Index - 1, Str.FData);
  return *this;
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::Delete(int Index, int Count)
{
  int Len = Length();
  if ((Index >= 1) && (Index <= Len) && (Count > 0))
  {
    if (Count > Len - Index + 1)
    {
      Count = Len - Index + 1;
    }
    FData.erase(Index - 1, Count);
  }
  return *this;
}
//---------------------------------------------------------------------------
int UnicodeString::Pos(const UnicodeString & SubStr) const
{
  if (SubStr.IsEmpty())
  {
    return 0;
  }
  size_t P = FData.find(SubStr.FData);
  return (P == std::wstring::npos) ? 0 : static_cast<int>(P + 1);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::SubString(int Index, int Count) const
{
  long long Len = Length();
  long long Start = (Index < 1) ? 1 : Index;
  long long N = Count;
  if ((Start > Len) || (N < 1))
  {
    return UnicodeString();
  }
  if (N > Len - Start + 1)
  {
    N = Len - Start + 1;
  }
  return UnicodeString(FData.substr(Start - 1, N));
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::Trim() const
{
  return Sysutils::Trim(*this);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::TrimLeft() const
{
  return Sysutils::TrimLeft(*this);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::TrimRight() const
{
  return Sysutils::TrimRight(*this);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::LowerCase() const
{
  return Sysutils::AnsiLowerCase(*this);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::UpperCase() const
{
  return Sysutils::AnsiUpperCase(*this);
}
//---------------------------------------------------------------------------
bool UnicodeString::IsDelimiter(const UnicodeString & Delimiters, int Index) const
{
  return Sysutils::IsDelimiter(Delimiters, *this, Index);
}
//---------------------------------------------------------------------------
int UnicodeString::LastDelimiter(const UnicodeString & Delimiters) const
{
  return Sysutils::LastDelimiter(Delimiters, *this);
}
//---------------------------------------------------------------------------
bool UnicodeString::IsPathDelimiter(int Index) const
{
  return Sysutils::IsPathDelimiter(*this, Index);
}
//---------------------------------------------------------------------------
bool UnicodeString::IsLeadSurrogate(int Index) const
{
  return (Index >= 1) && (Index <= Length()) && (FData[Index - 1] >= 0xD800) && (FData[Index - 1] <= 0xDBFF);
}
//---------------------------------------------------------------------------
bool UnicodeString::IsTrailSurrogate(int Index) const
{
  return (Index >= 1) && (Index <= Length()) && (FData[Index - 1] >= 0xDC00) && (FData[Index - 1] <= 0xDFFF);
}
//---------------------------------------------------------------------------
int UnicodeString::Compare(const UnicodeString & Rhs) const
{
  return Sysutils::AnsiCompareStr(*this, Rhs);
}
//---------------------------------------------------------------------------
int UnicodeString::CompareIC(const UnicodeString & Rhs) const
{
  return Sysutils::AnsiCompareText(*this, Rhs);
}
//---------------------------------------------------------------------------
int UnicodeString::ToInt() const
{
  return Sysutils::StrToInt(*this);
}
//---------------------------------------------------------------------------
int UnicodeString::ToIntDef(int Default) const
{
  return Sysutils::StrToIntDef(*this, Default);
}
//---------------------------------------------------------------------------
double UnicodeString::ToDouble() const
{
  return static_cast<double>(Sysutils::StrToFloat(*this));
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::sprintf(const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  FData = rtl::PrintfV(Format, Args);
  va_end(Args);
  return *this;
}
//---------------------------------------------------------------------------
int UnicodeString::printf(const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  FData = rtl::PrintfV(Format, Args);
  va_end(Args);
  return Length();
}
//---------------------------------------------------------------------------
int UnicodeString::vprintf(const wchar_t * Format, va_list Args)
{
  FData = rtl::PrintfV(Format, Args);
  return Length();
}
//---------------------------------------------------------------------------
UnicodeString & UnicodeString::cat_sprintf(const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  FData += rtl::PrintfV(Format, Args);
  va_end(Args);
  return *this;
}
//---------------------------------------------------------------------------
int UnicodeString::cat_printf(const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  std::wstring Added = rtl::PrintfV(Format, Args);
  va_end(Args);
  FData += Added;
  return static_cast<int>(Added.size());
}
//---------------------------------------------------------------------------
int UnicodeString::cat_vprintf(const wchar_t * Format, va_list Args)
{
  std::wstring Added = rtl::PrintfV(Format, Args);
  FData += Added;
  return static_cast<int>(Added.size());
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::StringOfChar(wchar_t Ch, int Count)
{
  return UnicodeString(std::wstring((Count > 0) ? Count : 0, Ch));
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::LoadStr(int Ident)
{
  return Sysutils::LoadStr(Ident);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::FmtLoadStr(int Ident, const TVarRec * Args, int Size)
{
  return Sysutils::FmtLoadStr(Ident, Args, Size);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::Format(const UnicodeString & Format, const TVarRec * Args, int Size)
{
  return Sysutils::Format(Format, Args, Size);
}
//---------------------------------------------------------------------------
UnicodeString UnicodeString::IntToHex(int Value, int Digits)
{
  return Sysutils::IntToHex(Value, Digits);
}
//---------------------------------------------------------------------------
std::ostream & operator <<(std::ostream & Stream, const UnicodeString & Str)
{
  std::string Utf8 = rtl::EncodeUtf8(Str.c_str(), Str.Length());
  return Stream << Utf8;
}
//---------------------------------------------------------------------------
std::wostream & operator <<(std::wostream & Stream, const UnicodeString & Str)
{
  return Stream << Str.Std();
}
//---------------------------------------------------------------------------
void PrintTo(const UnicodeString & Str, std::ostream * Stream)
{
  *Stream << "L\"" << Str << "\"";
}
//---------------------------------------------------------------------------
// AnsiStringBase
//---------------------------------------------------------------------------
int AnsiStringBase::DoPos(const char * SubStr, size_t SubLength) const
{
  if (SubLength == 0)
  {
    return 0;
  }
  size_t P = FData.find(SubStr, 0, SubLength);
  return (P == std::string::npos) ? 0 : static_cast<int>(P + 1);
}
//---------------------------------------------------------------------------
void AnsiStringBase::DoInsert(const char * Str, size_t StrLength, int Index)
{
  if (Index < 1)
  {
    Index = 1;
  }
  else if (Index > Length() + 1)
  {
    Index = Length() + 1;
  }
  FData.insert(Index - 1, Str, StrLength);
}
//---------------------------------------------------------------------------
void AnsiStringBase::DoDelete(int Index, int Count)
{
  int Len = Length();
  if ((Index >= 1) && (Index <= Len) && (Count > 0))
  {
    if (Count > Len - Index + 1)
    {
      Count = Len - Index + 1;
    }
    FData.erase(Index - 1, Count);
  }
}
//---------------------------------------------------------------------------
std::string AnsiStringBase::DoSubString(int Index, int Count) const
{
  long long Len = Length();
  long long Start = (Index < 1) ? 1 : Index;
  long long N = Count;
  if ((Start > Len) || (N < 1))
  {
    return std::string();
  }
  if (N > Len - Start + 1)
  {
    N = Len - Start + 1;
  }
  return FData.substr(Start - 1, N);
}
//---------------------------------------------------------------------------
bool AnsiStringBase::DoIsDelimiter(const std::string & Delimiters, int Index) const
{
  return (Index >= 1) && (Index <= Length()) && (Delimiters.find(FData[Index - 1]) != std::string::npos);
}
//---------------------------------------------------------------------------
int AnsiStringBase::DoLastDelimiter(const std::string & Delimiters) const
{
  for (int Index = Length(); Index >= 1; Index--)
  {
    char Ch = FData[Index - 1];
    if ((Ch != '\0') && (Delimiters.find(Ch) != std::string::npos))
    {
      return Index;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
std::ostream & operator <<(std::ostream & Stream, const AnsiStringBase & Str)
{
  return Stream << Str.Std();
}
//---------------------------------------------------------------------------
void PrintTo(const AnsiStringBase & Str, std::ostream * Stream)
{
  *Stream << "\"" << Str.Std() << "\"";
}
//---------------------------------------------------------------------------
} // namespace System
