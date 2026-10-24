// Basic Windows API types and macros used by the core's declarations.
// Only types, structures and constants live here; emulation of Windows API
// functions belongs to the platform layer (source/linux).
//---------------------------------------------------------------------------
#ifndef RtlWinTypesH
#define RtlWinTypesH
//---------------------------------------------------------------------------
#include <cstdint>
//---------------------------------------------------------------------------
typedef int BOOL;
typedef unsigned char BYTE;
typedef unsigned char UCHAR;
typedef unsigned char BOOLEAN;
typedef unsigned short WORD;
typedef unsigned short USHORT;
typedef short SHORT;
// Fixed 32-bit, as on Windows (where long is 32-bit).
typedef unsigned int DWORD;
typedef unsigned int ULONG;
typedef int LONG;
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int UINT32;
typedef int INT32;
typedef long long LONGLONG;
typedef unsigned long long ULONGLONG;
typedef unsigned long long DWORD64;
typedef unsigned long long UINT64;
typedef long long INT64;
typedef intptr_t INT_PTR;
typedef intptr_t LONG_PTR;
typedef uintptr_t UINT_PTR;
typedef uintptr_t ULONG_PTR;
typedef uintptr_t DWORD_PTR;
typedef ULONG_PTR SIZE_T;
typedef LONG_PTR SSIZE_T;
typedef char CHAR;
typedef wchar_t WCHAR;
typedef WCHAR TCHAR;
typedef void VOID;
typedef void * PVOID;
typedef void * LPVOID;
typedef const void * LPCVOID;
typedef void * HANDLE;
typedef HANDLE * PHANDLE;
typedef CHAR * LPSTR;
typedef CHAR * PSTR;
typedef const CHAR * LPCSTR;
typedef const CHAR * PCSTR;
typedef WCHAR * LPWSTR;
typedef WCHAR * PWSTR;
typedef const WCHAR * LPCWSTR;
typedef const WCHAR * PCWSTR;
typedef LPWSTR LPTSTR;
typedef LPCWSTR LPCTSTR;
typedef BYTE * PBYTE;
typedef BYTE * LPBYTE;
typedef WORD * PWORD;
typedef WORD * LPWORD;
typedef DWORD * PDWORD;
typedef DWORD * LPDWORD;
typedef LONG * PLONG;
typedef LONG * LPLONG;
typedef BOOL * PBOOL;
typedef BOOL * LPBOOL;
typedef ULONG * PULONG;
typedef LONG HRESULT;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef DWORD LCID;
typedef WORD LANGID;
typedef DWORD ACCESS_MASK;
typedef ACCESS_MASK REGSAM;
typedef struct HKEY__ * HKEY;
typedef HKEY * PHKEY;
typedef struct HWND__ * HWND;
typedef struct HINSTANCE__ * HINSTANCE;
typedef HINSTANCE HMODULE;
typedef struct HICON__ * HICON;
typedef struct HDC__ * HDC;
//---------------------------------------------------------------------------
struct FILETIME
{
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
};
typedef FILETIME * PFILETIME;
typedef FILETIME * LPFILETIME;
typedef FILETIME _FILETIME;
//---------------------------------------------------------------------------
struct SYSTEMTIME
{
  WORD wYear;
  WORD wMonth;
  WORD wDayOfWeek;
  WORD wDay;
  WORD wHour;
  WORD wMinute;
  WORD wSecond;
  WORD wMilliseconds;
};
typedef SYSTEMTIME * PSYSTEMTIME;
typedef SYSTEMTIME * LPSYSTEMTIME;
typedef SYSTEMTIME _SYSTEMTIME;
//---------------------------------------------------------------------------
struct TIME_ZONE_INFORMATION
{
  LONG Bias;
  WCHAR StandardName[32];
  SYSTEMTIME StandardDate;
  LONG StandardBias;
  WCHAR DaylightName[32];
  SYSTEMTIME DaylightDate;
  LONG DaylightBias;
};
typedef TIME_ZONE_INFORMATION * PTIME_ZONE_INFORMATION;
typedef TIME_ZONE_INFORMATION * LPTIME_ZONE_INFORMATION;
typedef TIME_ZONE_INFORMATION _TIME_ZONE_INFORMATION;
//---------------------------------------------------------------------------
struct GUID
{
  DWORD Data1;
  WORD Data2;
  WORD Data3;
  BYTE Data4[8];
};
typedef GUID CLSID;
typedef GUID IID;
typedef const GUID & REFGUID;
typedef const GUID & REFCLSID;
typedef const GUID & REFIID;
//---------------------------------------------------------------------------
union LARGE_INTEGER
{
  struct
  {
    DWORD LowPart;
    LONG HighPart;
  };
  struct
  {
    DWORD LowPart;
    LONG HighPart;
  } u;
  LONGLONG QuadPart;
};
typedef LARGE_INTEGER * PLARGE_INTEGER;
//---------------------------------------------------------------------------
union ULARGE_INTEGER
{
  struct
  {
    DWORD LowPart;
    DWORD HighPart;
  };
  struct
  {
    DWORD LowPart;
    DWORD HighPart;
  } u;
  ULONGLONG QuadPart;
};
typedef ULARGE_INTEGER * PULARGE_INTEGER;
//---------------------------------------------------------------------------
// Windows MAX_PATH is 260; buffers sized with it receive POSIX paths here.
#define MAX_PATH 4096
//---------------------------------------------------------------------------
struct WIN32_FIND_DATAW
{
  DWORD dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD nFileSizeHigh;
  DWORD nFileSizeLow;
  DWORD dwReserved0;
  DWORD dwReserved1;
  WCHAR cFileName[MAX_PATH];
  WCHAR cAlternateFileName[14];
};
typedef WIN32_FIND_DATAW WIN32_FIND_DATA;
typedef WIN32_FIND_DATAW TWin32FindData;
typedef WIN32_FIND_DATAW * PWIN32_FIND_DATAW;
typedef WIN32_FIND_DATAW * LPWIN32_FIND_DATAW;
//---------------------------------------------------------------------------
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define INVALID_HANDLE_VALUE (reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(-1)))
//---------------------------------------------------------------------------
#define S_OK (static_cast<HRESULT>(0))
#define S_FALSE (static_cast<HRESULT>(1))
#define E_FAIL (static_cast<HRESULT>(0x80004005))
#define E_NOTIMPL (static_cast<HRESULT>(0x80004001))
#define SUCCEEDED(hr) ((static_cast<HRESULT>(hr)) >= 0)
#define FAILED(hr) ((static_cast<HRESULT>(hr)) < 0)
//---------------------------------------------------------------------------
#define ERROR_SUCCESS 0L
//---------------------------------------------------------------------------
#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020
#define FILE_ATTRIBUTE_DEVICE 0x00000040
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE 0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800
#define FILE_ATTRIBUTE_OFFLINE 0x00001000
#define FILE_ATTRIBUTE_ENCRYPTED 0x00004000
#define INVALID_FILE_ATTRIBUTES (static_cast<DWORD>(-1))
//---------------------------------------------------------------------------
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_ALL 0x10000000
//---------------------------------------------------------------------------
#define CP_ACP 0
#define CP_OEMCP 1
#define CP_UTF7 65000
#define CP_UTF8 65001
//---------------------------------------------------------------------------
#define LOBYTE(w) (static_cast<BYTE>(static_cast<DWORD_PTR>(w) & 0xff))
#define HIBYTE(w) (static_cast<BYTE>((static_cast<DWORD_PTR>(w) >> 8) & 0xff))
#define LOWORD(l) (static_cast<WORD>(static_cast<DWORD_PTR>(l) & 0xffff))
#define HIWORD(l) (static_cast<WORD>((static_cast<DWORD_PTR>(l) >> 16) & 0xffff))
#define MAKEWORD(a, b) (static_cast<WORD>((static_cast<BYTE>(a)) | ((static_cast<WORD>(static_cast<BYTE>(b))) << 8)))
#define MAKELONG(a, b) (static_cast<LONG>((static_cast<WORD>(a)) | ((static_cast<DWORD>(static_cast<WORD>(b))) << 16)))
//---------------------------------------------------------------------------
#define __TEXT(quote) L##quote
#define TEXT(quote) __TEXT(quote)
//---------------------------------------------------------------------------
#endif
