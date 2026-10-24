// Functions of the Microsoft C runtime that glibc does not have, for the
// upstream code that calls them (the FileZilla engine, libs/mfc). Case
// insensitive comparisons fold the case as the RTL does (rtl::LowerChar, the
// C.UTF-8 locale); narrow ones fold ASCII only.
//---------------------------------------------------------------------------
#ifndef WinApiCrtH
#define WinApiCrtH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <cstdarg>
#include <cstddef>
#include <cwctype>
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
int _wcsicmp(const wchar_t * String1, const wchar_t * String2);
int _wcsnicmp(const wchar_t * String1, const wchar_t * String2, size_t Count);
int _stricmp(const char * String1, const char * String2);
int _strnicmp(const char * String1, const char * String2, size_t Count);
int strnicmp(const char * String1, const char * String2, size_t Count);
// The next character (wide strings have no multi-unit characters here)
wchar_t * _wcsinc(const wchar_t * Current);
// Decimal integers; 0 if there is no number, as atoi
int _wtoi(const wchar_t * String);
long long _wtoi64(const wchar_t * String);
long long _atoi64(const char * String);
// Unlike snprintf, the result is not terminated when it fills the buffer,
// and -1 is returned when it does not fit
int _snprintf(char * Buffer, size_t Count, const char * Format, ...) __attribute__((format(printf, 3, 4)));
// With the Windows meaning of %s in wide formats (a wide string), as the
// RTL's sprintf; -1 when the result does not fit (not terminated then)
int _snwprintf(wchar_t * Buffer, size_t Count, const wchar_t * Format, ...);
int snwprintf(wchar_t * Buffer, size_t Count, const wchar_t * Format, ...);
// Lowercases ASCII letters in place
char * strlwr(char * String);
char * _strlwr(char * String);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
// The legacy swprintf of the Microsoft C runtime, without the buffer size
// (an overload of the ISO one), with the Windows meaning of %s
int swprintf(wchar_t * Buffer, const wchar_t * Format, ...);
//---------------------------------------------------------------------------
// tchar.h, for UNICODE
#define _istalpha iswalpha
#define _istdigit iswdigit
#define _istspace iswspace
//---------------------------------------------------------------------------
#endif
