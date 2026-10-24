// Process information functions of the Windows API (tlhelp32.h, psapi.h)
// on /proc.
//---------------------------------------------------------------------------
#ifndef WinApiProcessH
#define WinApiProcessH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>
//---------------------------------------------------------------------------
#define TH32CS_SNAPPROCESS 0x00000002
struct PROCESSENTRY32
{
  DWORD dwSize;
  DWORD cntUsage;
  DWORD th32ProcessID;
  ULONG_PTR th32DefaultHeapID;
  DWORD th32ModuleID;
  DWORD cntThreads;
  DWORD th32ParentProcessID;
  LONG pcPriClassBase;
  DWORD dwFlags;
  // The command name (/proc/<pid>/comm)
  WCHAR szExeFile[MAX_PATH];
};
typedef PROCESSENTRY32 * LPPROCESSENTRY32;
// TH32CS_SNAPPROCESS only
WINAPI_EXTERN_C_BEGIN
HANDLE CreateToolhelp32Snapshot(DWORD Flags, DWORD ProcessId);
BOOL Process32First(HANDLE Snapshot, LPPROCESSENTRY32 Entry);
BOOL Process32Next(HANDLE Snapshot, LPPROCESSENTRY32 Entry);
//---------------------------------------------------------------------------
#define PROCESS_QUERY_INFORMATION 0x0400
#define PROCESS_QUERY_LIMITED_INFORMATION 0x1000
#define PROCESS_VM_READ 0x0010
// Succeeds for existing processes; the handle refers to the process id
HANDLE OpenProcess(DWORD DesiredAccess, BOOL InheritHandle, DWORD ProcessId);
// Modules of a process: the executable, then the mapped shared libraries
BOOL EnumProcessModules(HANDLE Process, HMODULE * Modules, DWORD Size, LPDWORD Needed);
// A NULL module is the executable
DWORD GetModuleFileNameEx(HANDLE Process, HMODULE Module, LPWSTR FileName, DWORD Size);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
#endif
