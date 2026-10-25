// System information, environment, locale and module functions of the
// Windows API on Linux.
//---------------------------------------------------------------------------
#ifndef WinApiSystemH
#define WinApiSystemH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <cstdio>
#include <type_traits>
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
// The temporary directory ($TMPDIR or /tmp) with a trailing slash
DWORD GetTempPath(DWORD BufferLength, LPWSTR Buffer);
// Expands %NAME% references to environment variables (Windows syntax);
// unknown variables are kept as they are
DWORD ExpandEnvironmentStrings(LPCWSTR Source, LPWSTR Destination, DWORD Size);
//---------------------------------------------------------------------------
// The locale is always en-US (1033); the RTL formats locale-independently
#define LOCALE_USER_DEFAULT 0x0400
#define LOCALE_SYSTEM_DEFAULT 0x0800
LCID GetUserDefaultLCID();
LCID GetSystemDefaultLCID();
LANGID GetUserDefaultUILanguage();
//---------------------------------------------------------------------------
#define MAX_LEADBYTES 12
#define MAX_DEFAULTCHAR 2
struct CPINFOEX
{
  UINT MaxCharSize;
  BYTE DefaultChar[MAX_DEFAULTCHAR];
  BYTE LeadByte[MAX_LEADBYTES];
  WCHAR UnicodeDefaultChar;
  UINT CodePage;
  WCHAR CodePageName[MAX_PATH];
};
typedef CPINFOEX * LPCPINFOEX;
// CP_ACP is UTF-8
BOOL GetCPInfoEx(UINT CodePage, DWORD Flags, LPCPINFOEX CPInfoEx);
UINT GetACP();
UINT GetOEMCP();
//---------------------------------------------------------------------------
int lstrcmp(LPCWSTR String1, LPCWSTR String2);
int lstrcmpi(LPCWSTR String1, LPCWSTR String2);
int lstrlen(LPCWSTR String);
FILE * _wfopen(const wchar_t * FileName, const wchar_t * Mode);
wchar_t * _wcsdup(const wchar_t * String);
//---------------------------------------------------------------------------
// Modules. There are no DLLs: LoadLibrary fails, GetModuleHandle(NULL) and
// GetModuleHandle(kernel32) return pseudo modules, and GetProcAddress finds
// the emulated functions of kernel32 that the core loads dynamically.
#define kernel32 L"kernel32.dll"
typedef int (WINAPI * FARPROC)();
HMODULE LoadLibrary(LPCWSTR FileName);
HMODULE LoadLibraryEx(LPCWSTR FileName, HANDLE File, DWORD Flags);
BOOL FreeLibrary(HMODULE Module);
HMODULE GetModuleHandle(LPCWSTR ModuleName);
FARPROC GetProcAddress(HMODULE Module, LPCSTR ProcName);
// The executable (/proc/self/exe) for the main module
DWORD GetModuleFileName(HMODULE Module, LPWSTR FileName, DWORD Size);
// Strings of the RTL resource string tables (LoadStr)
int LoadString(HINSTANCE Instance, UINT Id, LPWSTR Buffer, int BufferMax);
//---------------------------------------------------------------------------
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
// For memory allocated by the emulated functions (malloc)
HGLOBAL GlobalFree(HGLOBAL Memory);
HLOCAL LocalFree(HLOCAL Memory);
//---------------------------------------------------------------------------
#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_FREE 0x00010000
#define MEM_PRIVATE 0x00020000
#define MEM_MAPPED 0x00040000
#define MEM_IMAGE 0x01000000
struct MEMORY_BASIC_INFORMATION
{
  PVOID BaseAddress;
  PVOID AllocationBase;
  DWORD AllocationProtect;
  SIZE_T RegionSize;
  DWORD State;
  DWORD Protect;
  DWORD Type;
};
typedef MEMORY_BASIC_INFORMATION * PMEMORY_BASIC_INFORMATION;
// The mapping (/proc/self/maps) containing the address, or the gap before
// the next mapping (MEM_FREE); 0 beyond the last mapping
SIZE_T VirtualQuery(LPCVOID Address, PMEMORY_BASIC_INFORMATION Buffer, SIZE_T Length);
//---------------------------------------------------------------------------
#define UNLEN 256
enum EXTENDED_NAME_FORMAT
{
  NameUnknown = 0,
  NameFullyQualifiedDN = 1,
  NameSamCompatible = 2,
  NameDisplay = 3,
  NameUniqueId = 6,
  NameCanonical = 7,
  NameUserPrincipal = 8,
};
// The login name, for all formats
BOOLEAN GetUserNameEx(EXTENDED_NAME_FORMAT NameFormat, LPWSTR NameBuffer, PULONG Size);
WINAPI_EXTERN_C_END
// The core passes unsigned long, which is 32-bit on Windows only
BOOLEAN GetUserNameEx(EXTENDED_NAME_FORMAT NameFormat, LPWSTR NameBuffer, unsigned long * Size);
WINAPI_EXTERN_C_BEGIN
BOOL GetUserName(LPWSTR Buffer, LPDWORD Size);
//---------------------------------------------------------------------------
// There is no user interface: MessageBox writes the caption and the text to
// stderr and returns IDOK
#define MB_OK 0x00000000L
#define MB_ICONHAND 0x00000010L
#define MB_ICONQUESTION 0x00000020L
#define MB_ICONEXCLAMATION 0x00000030L
#define MB_ICONASTERISK 0x00000040L
#define MB_ICONERROR MB_ICONHAND
#define MB_ICONWARNING MB_ICONEXCLAMATION
#define MB_ICONINFORMATION MB_ICONASTERISK
#define IDOK 1
#define IDCANCEL 2
int MessageBox(HWND Window, LPCWSTR Text, LPCWSTR Caption, UINT Type);
// Memory cannot be probed (Windows deprecated the function too): FALSE
// unless the pointer is NULL
BOOL IsBadWritePtr(LPVOID Pointer, UINT_PTR Size);
BOOL GetComputerName(LPWSTR Buffer, LPDWORD Size);
//---------------------------------------------------------------------------
// Whether a 32-bit process runs on a 64-bit system: whether the system is
// 64-bit (as the core asks it only when built for 32 bits, see _WIN64)
BOOL IsWow64Process(HANDLE Process, PBOOL Wow64Process);
//---------------------------------------------------------------------------
// The product type of Windows; always PRODUCT_UNDEFINED
#define PRODUCT_UNDEFINED 0x00000000
BOOL GetProductInfo(DWORD OSMajorVersion, DWORD OSMinorVersion, DWORD SpMajorVersion, DWORD SpMinorVersion,
  PDWORD ReturnedProductType);
//---------------------------------------------------------------------------
// Version information of the executable (as in its Windows version
// resource): the version of the upstream WinSCP core, LinSCP's names.
// Version information of other files is not available.
DWORD GetFileVersionInfoSize(LPCWSTR FileName, LPDWORD Handle);
WINAPI_EXTERN_C_END
// The core passes unsigned long, which is 32-bit on Windows only
template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned long>>>
DWORD GetFileVersionInfoSize(LPCWSTR FileName, T * Handle)
{
  DWORD Value = 0;
  DWORD Result = GetFileVersionInfoSize(FileName, &Value);
  *Handle = Value;
  return Result;
}
WINAPI_EXTERN_C_BEGIN
BOOL GetFileVersionInfo(LPCWSTR FileName, DWORD Handle, DWORD Length, LPVOID Data);
// Supports "\", "\VarFileInfo\Translation" and "\StringFileInfo\<lang><cp>\<name>"
BOOL VerQueryValue(LPCVOID Block, LPCWSTR SubBlock, LPVOID * Buffer, PUINT Length);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
#endif
