// Registry functions of the Windows API on an empty registry, see
// winapi/Registry.h.
//---------------------------------------------------------------------------
#include <winapi/Registry.h>
//---------------------------------------------------------------------------
LONG RegOpenKeyEx(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY Result)
{
  *Result = NULL;
  return ERROR_FILE_NOT_FOUND;
}
//---------------------------------------------------------------------------
LONG RegCreateKeyEx(HKEY, LPCWSTR, DWORD, LPWSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, PHKEY Result, LPDWORD)
{
  *Result = NULL;
  return ERROR_ACCESS_DENIED;
}
//---------------------------------------------------------------------------
LONG RegCloseKey(HKEY)
{
  return ERROR_SUCCESS;
}
//---------------------------------------------------------------------------
LONG RegQueryValueEx(HKEY, LPCWSTR, unsigned long *, unsigned long *, LPBYTE, unsigned long *)
{
  return ERROR_FILE_NOT_FOUND;
}
//---------------------------------------------------------------------------
LONG RegQueryValueEx(HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD)
{
  return ERROR_FILE_NOT_FOUND;
}
//---------------------------------------------------------------------------
LONG RegSetValueEx(HKEY, LPCWSTR, DWORD, DWORD, const BYTE *, DWORD)
{
  return ERROR_ACCESS_DENIED;
}
//---------------------------------------------------------------------------
LONG RegDeleteValue(HKEY, LPCWSTR)
{
  return ERROR_FILE_NOT_FOUND;
}
//---------------------------------------------------------------------------
LONG RegDeleteKey(HKEY, LPCWSTR)
{
  return ERROR_FILE_NOT_FOUND;
}
//---------------------------------------------------------------------------
LONG RegEnumKeyEx(HKEY, DWORD, LPWSTR, LPDWORD, LPDWORD, LPWSTR, LPDWORD, PFILETIME)
{
  return ERROR_NO_MORE_ITEMS;
}
//---------------------------------------------------------------------------
