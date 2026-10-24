// Registry functions of the Windows API on an empty registry that cannot be
// written, like the RTL's TRegistry (rtl/Registry.h): keys do not open,
// values do not exist.
//---------------------------------------------------------------------------
#ifndef WinApiRegistryH
#define WinApiRegistryH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>
//---------------------------------------------------------------------------
#define REG_NONE 0
#define REG_SZ 1
#define REG_EXPAND_SZ 2
#define REG_BINARY 3
#define REG_DWORD 4
#define REG_MULTI_SZ 7
#define REG_QWORD 11
#define REG_OPTION_NON_VOLATILE 0x00000000
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
LONG RegOpenKeyEx(HKEY Key, LPCWSTR SubKey, DWORD Options, REGSAM Desired, PHKEY Result);
LONG RegCreateKeyEx(
  HKEY Key, LPCWSTR SubKey, DWORD Reserved, LPWSTR Class, DWORD Options, REGSAM Desired,
  LPSECURITY_ATTRIBUTES SecurityAttributes, PHKEY Result, LPDWORD Disposition);
LONG RegCloseKey(HKEY Key);
LONG RegQueryValueEx(HKEY Key, LPCWSTR ValueName, LPDWORD Reserved, LPDWORD Type, LPBYTE Data, LPDWORD DataSize);
LONG RegSetValueEx(HKEY Key, LPCWSTR ValueName, DWORD Reserved, DWORD Type, const BYTE * Data, DWORD DataSize);
LONG RegDeleteValue(HKEY Key, LPCWSTR ValueName);
LONG RegDeleteKey(HKEY Key, LPCWSTR SubKey);
LONG RegEnumKeyEx(
  HKEY Key, DWORD Index, LPWSTR Name, LPDWORD NameSize, LPDWORD Reserved, LPWSTR Class, LPDWORD ClassSize,
  PFILETIME LastWriteTime);
WINAPI_EXTERN_C_END
// The core passes unsigned long, which is 32-bit on Windows only
LONG RegQueryValueEx(
  HKEY Key, LPCWSTR ValueName, unsigned long * Reserved, unsigned long * Type, LPBYTE Data, unsigned long * DataSize);
//---------------------------------------------------------------------------
#endif
