// Functions of the Microsoft C runtime, see winapi/Crt.h.
//---------------------------------------------------------------------------
#include <winapi/Crt.h>

#include <rtl/Conversion.h>
#include <rtl/Printf.h>

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
template <typename TChar, typename TFold>
int CompareNoCase(const TChar * String1, const TChar * String2, size_t Count, TFold Fold)
{
  for (size_t Index = 0; Index < Count; Index++)
  {
    int Char1 = Fold(String1[Index]);
    int Char2 = Fold(String2[Index]);
    if ((Char1 != Char2) || (Char1 == 0))
    {
      return Char1 - Char2;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
int FoldWide(wchar_t Char)
{
  return static_cast<int>(rtl::LowerChar(Char));
}
//---------------------------------------------------------------------------
int FoldNarrow(char Char)
{
  return tolower(static_cast<unsigned char>(Char));
}
//---------------------------------------------------------------------------
// Copies the formatted string, with the result of _snprintf
template <typename TChar>
int CopyFormatted(TChar * Buffer, size_t Count, const std::basic_string<TChar> & Result)
{
  if (Result.size() < Count)
  {
    std::char_traits<TChar>::copy(Buffer, Result.c_str(), Result.size() + 1);
    return static_cast<int>(Result.size());
  }
  else
  {
    std::char_traits<TChar>::copy(Buffer, Result.c_str(), Count);
    return (Result.size() == Count) ? static_cast<int>(Count) : -1;
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
int _wcsicmp(const wchar_t * String1, const wchar_t * String2)
{
  return CompareNoCase(String1, String2, static_cast<size_t>(-1), FoldWide);
}
//---------------------------------------------------------------------------
int _wcsnicmp(const wchar_t * String1, const wchar_t * String2, size_t Count)
{
  return CompareNoCase(String1, String2, Count, FoldWide);
}
//---------------------------------------------------------------------------
int _stricmp(const char * String1, const char * String2)
{
  return CompareNoCase(String1, String2, static_cast<size_t>(-1), FoldNarrow);
}
//---------------------------------------------------------------------------
int _strnicmp(const char * String1, const char * String2, size_t Count)
{
  return CompareNoCase(String1, String2, Count, FoldNarrow);
}
//---------------------------------------------------------------------------
int strnicmp(const char * String1, const char * String2, size_t Count)
{
  return _strnicmp(String1, String2, Count);
}
//---------------------------------------------------------------------------
wchar_t * _wcsinc(const wchar_t * Current)
{
  return const_cast<wchar_t *>(Current + 1);
}
//---------------------------------------------------------------------------
int _wtoi(const wchar_t * String)
{
  return static_cast<int>(wcstol(String, NULL, 10));
}
//---------------------------------------------------------------------------
long long _wtoi64(const wchar_t * String)
{
  return wcstoll(String, NULL, 10);
}
//---------------------------------------------------------------------------
long long _atoi64(const char * String)
{
  return strtoll(String, NULL, 10);
}
//---------------------------------------------------------------------------
int _snprintf(char * Buffer, size_t Count, const char * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  std::string Result = rtl::AnsiFormatV(Format, Args);
  va_end(Args);
  return CopyFormatted(Buffer, Count, Result);
}
//---------------------------------------------------------------------------
int _snwprintf(wchar_t * Buffer, size_t Count, const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  std::wstring Result = rtl::PrintfV(Format, Args);
  va_end(Args);
  return CopyFormatted(Buffer, Count, Result);
}
//---------------------------------------------------------------------------
int snwprintf(wchar_t * Buffer, size_t Count, const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  std::wstring Result = rtl::PrintfV(Format, Args);
  va_end(Args);
  return CopyFormatted(Buffer, Count, Result);
}
//---------------------------------------------------------------------------
char * strlwr(char * String)
{
  for (char * Char = String; *Char != '\0'; Char++)
  {
    *Char = static_cast<char>(FoldNarrow(*Char));
  }
  return String;
}
//---------------------------------------------------------------------------
char * _strlwr(char * String)
{
  return strlwr(String);
}
//---------------------------------------------------------------------------
int swprintf(wchar_t * Buffer, const wchar_t * Format, ...)
{
  va_list Args;
  va_start(Args, Format);
  std::wstring Result = rtl::PrintfV(Format, Args);
  va_end(Args);
  std::char_traits<wchar_t>::copy(Buffer, Result.c_str(), Result.size() + 1);
  return static_cast<int>(Result.size());
}
//---------------------------------------------------------------------------
