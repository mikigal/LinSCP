// SysUtils: TEncoding
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
namespace
{
// Constructed before the RTL's and the core's other static objects
TEncoding AnsiEncoding __attribute__((init_priority(150)))(0, false);
TEncoding AsciiEncoding __attribute__((init_priority(150)))(20127, false);
TEncoding BigEndianUnicodeEncoding __attribute__((init_priority(150)))(1201, true);
TEncoding UnicodeEncoding __attribute__((init_priority(150)))(1200, true);
TEncoding Utf7Encoding __attribute__((init_priority(150)))(65000, false);
TEncoding Utf8Encoding __attribute__((init_priority(150)))(65001, true);

TBytes ToBytes(const std::string & Data)
{
  TBytes Result;
  Result.Length = static_cast<int>(Data.size());
  if (!Data.empty())
  {
    memcpy(Result.data(), Data.data(), Data.size());
  }
  return Result;
}

bool HasPreamble(const TBytes & Buffer, const std::string & Preamble)
{
  return
    !Preamble.empty() && (Buffer.Length >= static_cast<int>(Preamble.size())) &&
    (memcmp(Buffer.data(), Preamble.data(), Preamble.size()) == 0);
}

std::string PreambleOf(unsigned int CodePage)
{
  switch (CodePage)
  {
    case 65001: return "\xEF\xBB\xBF";
    case 1200: return "\xFF\xFE";
    case 1201: return "\xFE\xFF";
    default: return std::string();
  }
}
} // namespace
//---------------------------------------------------------------------------
TEncoding * const TEncoding::ANSI = &AnsiEncoding;
TEncoding * const TEncoding::ASCII = &AsciiEncoding;
TEncoding * const TEncoding::BigEndianUnicode = &BigEndianUnicodeEncoding;
TEncoding * const TEncoding::Default = &AnsiEncoding;
TEncoding * const TEncoding::Unicode = &UnicodeEncoding;
TEncoding * const TEncoding::UTF7 = &Utf7Encoding;
TEncoding * const TEncoding::UTF8 = &Utf8Encoding;
//---------------------------------------------------------------------------
TEncoding::TEncoding(unsigned int ACodePage, bool APreamble) :
  FCodePage(ACodePage),
  FPreamble(APreamble)
{
}
//---------------------------------------------------------------------------
std::string TEncoding::EncodeToBytes(const wchar_t * Chars, size_t Count)
{
  switch (FCodePage)
  {
    case 1200:
    case 1201:
      {
        std::u16string Utf16 = rtl::EncodeUtf16(Chars, Count);
        std::string Result(Utf16.size() * 2, '\0');
        bool BigEndian = (FCodePage == 1201);
        for (size_t Index = 0; Index < Utf16.size(); Index++)
        {
          char Low = static_cast<char>(Utf16[Index] & 0xFF);
          char High = static_cast<char>(Utf16[Index] >> 8);
          Result[Index * 2] = BigEndian ? High : Low;
          Result[Index * 2 + 1] = BigEndian ? Low : High;
        }
        return Result;
      }
    default:
      return rtl::EncodeCodePage(FCodePage, Chars, Count);
  }
}
//---------------------------------------------------------------------------
std::wstring TEncoding::DecodeFromBytes(const char * Bytes, size_t Count)
{
  switch (FCodePage)
  {
    case 1200:
    case 1201:
      {
        std::u16string Utf16(Count / 2, u'\0');
        bool BigEndian = (FCodePage == 1201);
        for (size_t Index = 0; Index < Utf16.size(); Index++)
        {
          unsigned char First = static_cast<unsigned char>(Bytes[Index * 2]);
          unsigned char Second = static_cast<unsigned char>(Bytes[Index * 2 + 1]);
          Utf16[Index] = static_cast<char16_t>(BigEndian ? ((First << 8) | Second) : ((Second << 8) | First));
        }
        return rtl::DecodeUtf16(Utf16.data(), Utf16.size());
      }
    default:
      return rtl::DecodeCodePage(FCodePage, Bytes, Count);
  }
}
//---------------------------------------------------------------------------
TBytes TEncoding::GetBytes(const UnicodeString & S)
{
  return ToBytes(EncodeToBytes(S.c_str(), S.Length()));
}
//---------------------------------------------------------------------------
TBytes TEncoding::GetBytes(const wchar_t * Chars, int CharCount)
{
  return ToBytes(EncodeToBytes(Chars, (CharCount > 0) ? CharCount : 0));
}
//---------------------------------------------------------------------------
int TEncoding::GetByteCount(const UnicodeString & S)
{
  return static_cast<int>(EncodeToBytes(S.c_str(), S.Length()).size());
}
//---------------------------------------------------------------------------
UnicodeString TEncoding::GetString(const TBytes & Bytes)
{
  return GetString(Bytes, 0, Bytes.Length);
}
//---------------------------------------------------------------------------
UnicodeString TEncoding::GetString(const TBytes & Bytes, int ByteIndex, int ByteCount)
{
  if ((ByteIndex < 0) || (ByteCount < 0) || (ByteIndex + ByteCount > Bytes.Length))
  {
    throw EEncodingError(L"Invalid byte range");
  }
  return GetString(Bytes.data() + ByteIndex, ByteCount);
}
//---------------------------------------------------------------------------
UnicodeString TEncoding::GetString(const void * Bytes, int ByteCount)
{
  if (ByteCount <= 0)
  {
    return UnicodeString();
  }
  return UnicodeString(DecodeFromBytes(static_cast<const char *>(Bytes), ByteCount));
}
//---------------------------------------------------------------------------
int TEncoding::GetCharCount(const TBytes & Bytes)
{
  return GetString(Bytes).Length();
}
//---------------------------------------------------------------------------
TBytes TEncoding::GetPreamble()
{
  return FPreamble ? ToBytes(PreambleOf(FCodePage)) : TBytes();
}
//---------------------------------------------------------------------------
bool TEncoding::IsSingleByte()
{
  return (FCodePage != 0) && (FCodePage != 65001) && (FCodePage != 65000) && (FCodePage != 1200) && (FCodePage != 1201) &&
    (FCodePage != 932) && (FCodePage != 936) && (FCodePage != 949) && (FCodePage != 950) && (FCodePage != 54936);
}
//---------------------------------------------------------------------------
TEncoding * TEncoding::Clone()
{
  return new TEncoding(FCodePage, FPreamble);
}
//---------------------------------------------------------------------------
UnicodeString TEncoding::GetEncodingName()
{
  std::string Name = rtl::CodePageName((FCodePage == 0) ? rtl::AnsiCodePage : FCodePage);
  return Format(L"%d  (%s)", ARRAYOFCONST((static_cast<int>(FCodePage), UnicodeString(Name.c_str()))));
}
//---------------------------------------------------------------------------
int TEncoding::GetBufferEncoding(const TBytes & Buffer, TEncoding *& AEncoding)
{
  return GetBufferEncoding(Buffer, AEncoding, Default);
}
//---------------------------------------------------------------------------
int TEncoding::GetBufferEncoding(const TBytes & Buffer, TEncoding *& AEncoding, TEncoding * ADefaultEncoding)
{
  if (AEncoding == nullptr)
  {
    TEncoding * Candidates[] = { UTF8, Unicode, BigEndianUnicode };
    for (TEncoding * Candidate : Candidates)
    {
      std::string Preamble = PreambleOf(Candidate->FCodePage);
      if (HasPreamble(Buffer, Preamble))
      {
        AEncoding = Candidate;
        return static_cast<int>(Preamble.size());
      }
    }
    AEncoding = ADefaultEncoding;
    return 0;
  }
  std::string Preamble = AEncoding->FPreamble ? PreambleOf(AEncoding->FCodePage) : std::string();
  return HasPreamble(Buffer, Preamble) ? static_cast<int>(Preamble.size()) : 0;
}
//---------------------------------------------------------------------------
TEncoding * TEncoding::GetEncoding(int CodePage)
{
  switch (CodePage)
  {
    case 1200: return new TUnicodeEncoding();
    case 1201: return new TBigEndianUnicodeEncoding();
    case 65001: return new TUTF8Encoding();
    default: return new TMBCSEncoding(CodePage);
  }
}
//---------------------------------------------------------------------------
bool TEncoding::IsStandardEncoding(TEncoding * AEncoding)
{
  return
    (AEncoding == ANSI) || (AEncoding == ASCII) || (AEncoding == BigEndianUnicode) || (AEncoding == Unicode) ||
    (AEncoding == UTF7) || (AEncoding == UTF8);
}
//---------------------------------------------------------------------------
void TEncoding::FreeEncoding(TEncoding * AEncoding)
{
  if (!IsStandardEncoding(AEncoding))
  {
    delete AEncoding;
  }
}
//---------------------------------------------------------------------------
TBytes TEncoding::Convert(TEncoding * Source, TEncoding * Destination, const TBytes & Bytes)
{
  return Destination->GetBytes(Source->GetString(Bytes));
}
//---------------------------------------------------------------------------
TBytes BytesOf(const UnicodeString & Val)
{
  return TEncoding::Default->GetBytes(Val);
}
//---------------------------------------------------------------------------
TBytes BytesOf(const RawByteString & Val)
{
  return ToBytes(Val.Std());
}
//---------------------------------------------------------------------------
TBytes BytesOf(const void * Val, int Len)
{
  return ToBytes(std::string(static_cast<const char *>(Val), (Len > 0) ? Len : 0));
}
//---------------------------------------------------------------------------
UnicodeString StringOf(const TBytes & Bytes)
{
  return TEncoding::Default->GetString(Bytes);
}
//---------------------------------------------------------------------------
TBytes WideBytesOf(const UnicodeString & Value)
{
  return TEncoding::Unicode->GetBytes(Value);
}
//---------------------------------------------------------------------------
UnicodeString WideStringOf(const TBytes & Value)
{
  return TEncoding::Unicode->GetString(Value);
}
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
