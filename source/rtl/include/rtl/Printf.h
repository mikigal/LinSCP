// printf-style formatting with the conventions of the Windows C runtime, as used
// by UnicodeString::sprintf and AnsiString::sprintf.
//
// In a wide format, %s and %c take wide arguments and %hs/%S narrow ones; in a
// narrow format it is the other way round. %Ld, %I64d and %qd take 64-bit
// integers, %Id size_t. Numbers are always formatted in the "C" locale.
//---------------------------------------------------------------------------
#ifndef RtlPrintfH
#define RtlPrintfH
//---------------------------------------------------------------------------
#include <cstdarg>
#include <string>
//---------------------------------------------------------------------------
namespace rtl
{
std::wstring PrintfV(const wchar_t * Format, va_list Args);
std::string AnsiFormatV(const char * Format, va_list Args);
// Runs F with the "C" locale active in this thread
template <typename F> auto WithCLocale(F Function) -> decltype(Function());
} // namespace rtl
//---------------------------------------------------------------------------
#include <locale.h>
//---------------------------------------------------------------------------
namespace rtl
{
locale_t CLocale();

template <typename F>
auto WithCLocale(F Function) -> decltype(Function())
{
  struct TRestore
  {
    locale_t Previous;
    ~TRestore() { uselocale(Previous); }
  } Restore{uselocale(CLocale())};
  return Function();
}
} // namespace rtl
//---------------------------------------------------------------------------
#endif
