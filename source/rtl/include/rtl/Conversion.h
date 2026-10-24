// Character set conversions at the boundaries of UnicodeString (PLAN.md 3.2).
//
// UTF-8 decoding is lossless: every byte that is not part of a valid UTF-8
// sequence is mapped to the lone surrogate U+DC80..U+DCFF (like Python's
// "surrogateescape"), and encoding maps those back to the original bytes. File
// names that are not valid UTF-8 therefore survive a round trip unchanged.
//---------------------------------------------------------------------------
#ifndef RtlConversionH
#define RtlConversionH
//---------------------------------------------------------------------------
#include <string>
//---------------------------------------------------------------------------
namespace rtl
{
// Code page of AnsiString (CP_ACP). The Linux "ANSI" code page is UTF-8.
const unsigned int AnsiCodePage = 65001;
const unsigned int RawByteCodePage = 0xFFFF;

std::wstring DecodeUtf8(const char * Data, size_t Length);
std::string EncodeUtf8(const wchar_t * Data, size_t Length);
// UTF-16 (little endian, in host order as 16-bit units)
std::wstring DecodeUtf16(const char16_t * Data, size_t Length);
std::u16string EncodeUtf16(const wchar_t * Data, size_t Length);
// Any code page. 0 (CP_ACP), 0xFFFF (raw) and 65001 are UTF-8, others use iconv.
// Characters that cannot be represented are replaced by '?' (like Windows).
std::wstring DecodeCodePage(unsigned int CodePage, const char * Data, size_t Length);
std::string EncodeCodePage(unsigned int CodePage, const wchar_t * Data, size_t Length);
bool IsUtf8CodePage(unsigned int CodePage);
// iconv name of a Windows code page number, empty if unknown
std::string CodePageName(unsigned int CodePage);

// Unicode simple case mapping, independent of the process locale.
wchar_t UpperChar(wchar_t Ch);
wchar_t LowerChar(wchar_t Ch);
} // namespace rtl
//---------------------------------------------------------------------------
#endif
