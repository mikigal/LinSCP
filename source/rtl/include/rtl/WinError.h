// Windows error codes and the thread's "last error".
//
// The core compares OS error codes with Windows constants (ERROR_FILE_NOT_FOUND,
// ERROR_NO_MORE_FILES, ...), so errors reported by the RTL and the platform
// layer are Windows codes. errno values without a Windows equivalent are
// encoded reversibly as (ErrnoFlag | errno).
//---------------------------------------------------------------------------
#ifndef RtlWinErrorH
#define RtlWinErrorH
//---------------------------------------------------------------------------
#include <rtl/WinTypes.h>
//---------------------------------------------------------------------------
#define ERROR_INVALID_FUNCTION 1L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_PATH_NOT_FOUND 3L
#define ERROR_TOO_MANY_OPEN_FILES 4L
#define ERROR_ACCESS_DENIED 5L
#define ERROR_INVALID_HANDLE 6L
#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_INVALID_DATA 13L
#define ERROR_OUTOFMEMORY 14L
#define ERROR_INVALID_DRIVE 15L
#define ERROR_CURRENT_DIRECTORY 16L
#define ERROR_NOT_SAME_DEVICE 17L
#define ERROR_NO_MORE_FILES 18L
#define ERROR_WRITE_PROTECT 19L
#define ERROR_NOT_READY 21L
#define ERROR_GEN_FAILURE 31L
#define ERROR_SHARING_VIOLATION 32L
#define ERROR_LOCK_VIOLATION 33L
#define ERROR_HANDLE_EOF 38L
#define ERROR_HANDLE_DISK_FULL 39L
#define ERROR_NOT_SUPPORTED 50L
#define ERROR_FILE_EXISTS 80L
#define ERROR_CANNOT_MAKE 82L
#define ERROR_INVALID_PARAMETER 87L
#define ERROR_BROKEN_PIPE 109L
#define ERROR_DISK_FULL 112L
#define ERROR_INSUFFICIENT_BUFFER 122L
#define ERROR_INVALID_NAME 123L
#define ERROR_DIR_NOT_EMPTY 145L
#define ERROR_BUSY 170L
#define ERROR_ALREADY_EXISTS 183L
#define ERROR_FILENAME_EXCED_RANGE 206L
#define ERROR_MORE_DATA 234L
#define WAIT_TIMEOUT 258L
#define ERROR_NO_MORE_ITEMS 259L
#define ERROR_DIRECTORY 267L
#define ERROR_NOT_OWNER 288L
#define ERROR_OPERATION_ABORTED 995L
#define ERROR_IO_PENDING 997L
#define ERROR_NOACCESS 998L
#define ERROR_CANCELLED 1223L
#define ERROR_PRIVILEGE_NOT_HELD 1314L
#define ERROR_TIMEOUT 1460L
#define ERROR_CANT_RESOLVE_FILENAME 1921L
#define ERROR_NOT_A_REPARSE_POINT 4390L
//---------------------------------------------------------------------------
DWORD GetLastError();
void SetLastError(DWORD ErrorCode);
//---------------------------------------------------------------------------
namespace rtl
{
const DWORD ErrnoFlag = 0x20000000;
DWORD ErrnoToWinError(int Errno);
int WinErrorToErrno(DWORD ErrorCode);
// Sets the last error from errno and returns it
DWORD SetLastErrorFromErrno();
} // namespace rtl
//---------------------------------------------------------------------------
#endif
