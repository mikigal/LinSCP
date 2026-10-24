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
#define ERROR_BUFFER_OVERFLOW 111L
#define ERROR_DISK_FULL 112L
#define ERROR_INSUFFICIENT_BUFFER 122L
#define ERROR_INVALID_NAME 123L
#define ERROR_MOD_NOT_FOUND 126L
#define ERROR_PROC_NOT_FOUND 127L
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
#define ERROR_INVALID_WINDOW_HANDLE 1400L
#define ERROR_CANNOT_FIND_WND_CLASS 1407L
#define ERROR_CLASS_ALREADY_EXISTS 1410L
#define ERROR_CLASS_DOES_NOT_EXIST 1411L
#define ERROR_INVALID_INDEX 1413L
#define ERROR_INVALID_THREAD_ID 1444L
#define ERROR_TIMEOUT 1460L
#define ERROR_RESOURCE_TYPE_NOT_FOUND 1813L
#define ERROR_RESOURCE_NAME_NOT_FOUND 1814L
#define ERROR_CANT_RESOLVE_FILENAME 1921L
#define ERROR_NOT_A_REPARSE_POINT 4390L
//---------------------------------------------------------------------------
// WinSock error codes (also Windows error codes); those with an errno
// equivalent map to it and back
#define WSABASEERR 10000
#define WSAEINTR 10004L
#define WSAEBADF 10009L
#define WSAEACCES 10013L
#define WSAEFAULT 10014L
#define WSAEINVAL 10022L
#define WSAEMFILE 10024L
#define WSAEWOULDBLOCK 10035L
#define WSAEINPROGRESS 10036L
#define WSAEALREADY 10037L
#define WSAENOTSOCK 10038L
#define WSAEDESTADDRREQ 10039L
#define WSAEMSGSIZE 10040L
#define WSAEPROTOTYPE 10041L
#define WSAENOPROTOOPT 10042L
#define WSAEPROTONOSUPPORT 10043L
#define WSAESOCKTNOSUPPORT 10044L
#define WSAEOPNOTSUPP 10045L
#define WSAEPFNOSUPPORT 10046L
#define WSAEAFNOSUPPORT 10047L
#define WSAEADDRINUSE 10048L
#define WSAEADDRNOTAVAIL 10049L
#define WSAENETDOWN 10050L
#define WSAENETUNREACH 10051L
#define WSAENETRESET 10052L
#define WSAECONNABORTED 10053L
#define WSAECONNRESET 10054L
#define WSAENOBUFS 10055L
#define WSAEISCONN 10056L
#define WSAENOTCONN 10057L
#define WSAESHUTDOWN 10058L
#define WSAETOOMANYREFS 10059L
#define WSAETIMEDOUT 10060L
#define WSAECONNREFUSED 10061L
#define WSAELOOP 10062L
#define WSAENAMETOOLONG 10063L
#define WSAEHOSTDOWN 10064L
#define WSAEHOSTUNREACH 10065L
#define WSAENOTEMPTY 10066L
#define WSAEPROCLIM 10067L
#define WSAEUSERS 10068L
#define WSAEDQUOT 10069L
#define WSAESTALE 10070L
#define WSAEREMOTE 10071L
#define WSASYSNOTREADY 10091L
#define WSAVERNOTSUPPORTED 10092L
#define WSANOTINITIALISED 10093L
#define WSAEDISCON 10101L
#define WSAHOST_NOT_FOUND 11001L
#define WSATRY_AGAIN 11002L
#define WSANO_RECOVERY 11003L
#define WSANO_DATA 11004L
//---------------------------------------------------------------------------
DWORD GetLastError();
void SetLastError(DWORD ErrorCode);
//---------------------------------------------------------------------------
namespace rtl
{
const DWORD ErrnoFlag = 0x20000000;
DWORD ErrnoToWinError(int Errno);
// The WinSock error code (WSAE*) of an errno value, for socket calls
DWORD ErrnoToWinSockError(int Errno);
int WinErrorToErrno(DWORD ErrorCode);
// Sets the last error from errno and returns it
DWORD SetLastErrorFromErrno();
} // namespace rtl
//---------------------------------------------------------------------------
#endif
