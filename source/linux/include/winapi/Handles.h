// Kernel objects of the Windows API on POSIX: handles, events, threads,
// waits, process and thread identity, timing.
//
// A file handle (CreateFile, the RTL's FileOpen) is the file descriptor cast
// to HANDLE, so it converts to the RTL's THandle and back. Other kernel
// objects (events, threads, process snapshots) are registered objects; a
// HANDLE that is not registered is a file descriptor.
//---------------------------------------------------------------------------
#ifndef WinApiHandlesH
#define WinApiHandlesH
//---------------------------------------------------------------------------
#include <rtl/WinTypes.h>
#include <rtl/WinError.h>

#include <memory>
//---------------------------------------------------------------------------
#define WINAPI
#define CALLBACK
#define APIENTRY
// The emulated API has C linkage, as on Windows, so that C code (PuTTY) can
// call it too
#ifdef __cplusplus
#define WINAPI_EXTERN_C_BEGIN extern "C" {
#define WINAPI_EXTERN_C_END }
#else
#define WINAPI_EXTERN_C_BEGIN
#define WINAPI_EXTERN_C_END
#endif
//---------------------------------------------------------------------------
#define WAIT_OBJECT_0 0x00000000L
#define WAIT_ABANDONED 0x00000080L
#define WAIT_IO_COMPLETION 0x000000C0L
#define WAIT_FAILED (static_cast<DWORD>(0xFFFFFFFF))
#define MAXIMUM_WAIT_OBJECTS 64
#define STILL_ACTIVE 259
//---------------------------------------------------------------------------
#define CREATE_SUSPENDED 0x00000004
#define THREAD_PRIORITY_LOWEST -2
#define THREAD_PRIORITY_BELOW_NORMAL -1
#define THREAD_PRIORITY_NORMAL 0
#define THREAD_PRIORITY_ABOVE_NORMAL 1
#define THREAD_PRIORITY_HIGHEST 2
//---------------------------------------------------------------------------
struct SECURITY_ATTRIBUTES
{
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
};
typedef SECURITY_ATTRIBUTES * PSECURITY_ATTRIBUTES;
typedef SECURITY_ATTRIBUTES * LPSECURITY_ATTRIBUTES;
typedef DWORD (WINAPI * LPTHREAD_START_ROUTINE)(LPVOID Parameter);
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
BOOL CloseHandle(HANDLE Object);
//---------------------------------------------------------------------------
// Events (the name is not supported, must be NULL)
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES Attributes, BOOL ManualReset, BOOL InitialState, LPCWSTR Name);
BOOL SetEvent(HANDLE Event);
BOOL ResetEvent(HANDLE Event);
//---------------------------------------------------------------------------
// Waits for events and threads. Alertable waits behave as non-alertable ones.
DWORD WaitForSingleObject(HANDLE Handle, DWORD Milliseconds);
DWORD WaitForMultipleObjects(DWORD Count, const HANDLE * Handles, BOOL WaitAll, DWORD Milliseconds);
//---------------------------------------------------------------------------
// Threads. The thread priority is advisory and ignored.
HANDLE CreateThread(
  LPSECURITY_ATTRIBUTES Attributes, SIZE_T StackSize, LPTHREAD_START_ROUTINE StartAddress, LPVOID Parameter,
  DWORD CreationFlags, LPDWORD ThreadId);
// Returns the previous suspend count; only the initial suspension
// (CREATE_SUSPENDED) is supported
DWORD ResumeThread(HANDLE Thread);
BOOL SetThreadPriority(HANDLE Thread, int Priority);
BOOL GetExitCodeThread(HANDLE Thread, LPDWORD ExitCode);
//---------------------------------------------------------------------------
DWORD GetCurrentThreadId();
DWORD GetCurrentProcessId();
// Pseudo handles, valid for the calling thread or process only
HANDLE GetCurrentThread();
HANDLE GetCurrentProcess();
//---------------------------------------------------------------------------
void Sleep(DWORD Milliseconds);
DWORD SleepEx(DWORD Milliseconds, BOOL Alertable);
// Milliseconds of a monotonic clock; GetTickCount wraps around after 49.7 days
DWORD GetTickCount();
ULONGLONG GetTickCount64();
//---------------------------------------------------------------------------
// Recursive mutex
struct CRITICAL_SECTION
{
  void * Mutex;
};
typedef CRITICAL_SECTION * LPCRITICAL_SECTION;
typedef CRITICAL_SECTION * PCRITICAL_SECTION;
void InitializeCriticalSection(LPCRITICAL_SECTION CriticalSection);
void DeleteCriticalSection(LPCRITICAL_SECTION CriticalSection);
void EnterCriticalSection(LPCRITICAL_SECTION CriticalSection);
void LeaveCriticalSection(LPCRITICAL_SECTION CriticalSection);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
namespace winapi
{
// Base of the objects behind non-file handles
class TKernelObject
{
public:
  virtual ~TKernelObject() {}
};
// Registers the object; its handle stays valid until CloseHandle
HANDLE RegisterObject(std::shared_ptr<TKernelObject> Object);
std::shared_ptr<TKernelObject> FindKernelObject(HANDLE Handle);
// Empty if the handle is not a registered object of the type
template <class T>
std::shared_ptr<T> FindObject(HANDLE Handle)
{
  return std::dynamic_pointer_cast<T>(FindKernelObject(Handle));
}
// The file descriptor of a file handle, -1 for other handles
int HandleToFd(HANDLE Handle);
// A file handle of a descriptor; descriptors 0 to 2 are moved, so that a
// file handle is never NULL (fails with INVALID_HANDLE_VALUE)
HANDLE FdToHandle(int Fd);
}
//---------------------------------------------------------------------------
#endif
