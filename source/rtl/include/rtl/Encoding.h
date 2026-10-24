// SysUtils: TEncoding.
//
// TEncoding::ANSI and TEncoding::Default are UTF-8 without a preamble (the
// Linux ANSI code page), TEncoding::UTF8 is UTF-8 with the BOM preamble.
//---------------------------------------------------------------------------
#ifndef RtlEncodingH
#define RtlEncodingH
//---------------------------------------------------------------------------
#include <rtl/System.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
class TEncoding : public TObject
{
public:
  static TEncoding * const ANSI;
  static TEncoding * const ASCII;
  static TEncoding * const BigEndianUnicode;
  static TEncoding * const Default;
  static TEncoding * const Unicode;
  static TEncoding * const UTF7;
  static TEncoding * const UTF8;

  // Detects a BOM; returns the length of the preamble found
  static int GetBufferEncoding(const TBytes & Buffer, TEncoding *& AEncoding);
  static int GetBufferEncoding(const TBytes & Buffer, TEncoding *& AEncoding, TEncoding * ADefaultEncoding);
  // A new encoding object, unless it is one of the standard ones
  static TEncoding * GetEncoding(int CodePage);
  static bool IsStandardEncoding(TEncoding * AEncoding);
  static void FreeEncoding(TEncoding * AEncoding);
  static TBytes Convert(TEncoding * Source, TEncoding * Destination, const TBytes & Bytes);

  TEncoding(unsigned int ACodePage, bool APreamble);

  TBytes GetBytes(const UnicodeString & S);
  TBytes GetBytes(const wchar_t * Chars, int CharCount);
  int GetByteCount(const UnicodeString & S);
  UnicodeString GetString(const TBytes & Bytes);
  UnicodeString GetString(const TBytes & Bytes, int ByteIndex, int ByteCount);
  UnicodeString GetString(const void * Bytes, int ByteCount);
  int GetCharCount(const TBytes & Bytes);
  TBytes GetPreamble();
  bool IsSingleByte();
  TEncoding * Clone();

  unsigned int GetCodePage() const { return FCodePage; }
  UnicodeString GetEncodingName();
  __declspec(property(get = GetCodePage)) unsigned int CodePage;
  __declspec(property(get = GetEncodingName)) UnicodeString EncodingName;

  // Used by the implementation
  std::string EncodeToBytes(const wchar_t * Chars, size_t Count);
  std::wstring DecodeFromBytes(const char * Bytes, size_t Count);

private:
  unsigned int FCodePage;
  bool FPreamble;
};
//---------------------------------------------------------------------------
class TMBCSEncoding : public TEncoding
{
public:
  TMBCSEncoding() : TEncoding(0, false) {}
  explicit TMBCSEncoding(int CodePage) : TEncoding(CodePage, false) {}
};
//---------------------------------------------------------------------------
class TUTF8Encoding : public TEncoding
{
public:
  TUTF8Encoding() : TEncoding(65001, true) {}
  explicit TUTF8Encoding(bool UseBOM) : TEncoding(65001, UseBOM) {}
};
//---------------------------------------------------------------------------
class TUnicodeEncoding : public TEncoding
{
public:
  TUnicodeEncoding() : TEncoding(1200, true) {}
};
//---------------------------------------------------------------------------
class TBigEndianUnicodeEncoding : public TEncoding
{
public:
  TBigEndianUnicodeEncoding() : TEncoding(1201, true) {}
};
//---------------------------------------------------------------------------
TBytes BytesOf(const UnicodeString & Val);
TBytes BytesOf(const RawByteString & Val);
TBytes BytesOf(const void * Val, int Len);
UnicodeString StringOf(const TBytes & Bytes);
TBytes WideBytesOf(const UnicodeString & Value);
UnicodeString WideStringOf(const TBytes & Value);
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
//---------------------------------------------------------------------------
#endif
