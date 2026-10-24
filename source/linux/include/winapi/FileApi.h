// File functions of the Windows API on POSIX file descriptors.
//
// Attributes follow the RTL (FileGetAttr): FILE_ATTRIBUTE_READONLY means not
// writable by the process, FILE_ATTRIBUTE_HIDDEN a name starting with a dot,
// FILE_ATTRIBUTE_REPARSE_POINT a symbolic link (whose other attributes are
// those of its target). Sharing modes are ignored.
//---------------------------------------------------------------------------
#ifndef WinApiFileApiH
#define WinApiFileApiH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <type_traits>
//---------------------------------------------------------------------------
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define FILE_SHARE_DELETE 0x00000004
//---------------------------------------------------------------------------
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5
//---------------------------------------------------------------------------
#define FILE_FLAG_WRITE_THROUGH 0x80000000
#define FILE_FLAG_OVERLAPPED 0x40000000
#define FILE_FLAG_NO_BUFFERING 0x20000000
#define FILE_FLAG_RANDOM_ACCESS 0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE 0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000
#define FILE_FLAG_POSIX_SEMANTICS 0x01000000
#define FILE_FLAG_OPEN_REPARSE_POINT 0x00200000
//---------------------------------------------------------------------------
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define INVALID_FILE_SIZE (static_cast<DWORD>(0xFFFFFFFF))
#define INVALID_SET_FILE_POINTER (static_cast<DWORD>(-1))
//---------------------------------------------------------------------------
#define IO_REPARSE_TAG_MOUNT_POINT 0xA0000003L
#define IO_REPARSE_TAG_SYMLINK 0xA000000CL
#define FSCTL_GET_REPARSE_POINT 0x000900A8
#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE (16 * 1024)
//---------------------------------------------------------------------------
struct OVERLAPPED;
typedef OVERLAPPED * LPOVERLAPPED;
//---------------------------------------------------------------------------
// Opens directories too (with FILE_FLAG_BACKUP_SEMANTICS, as on Windows),
// read-only, so that their times can be read and set. With
// FILE_FLAG_OPEN_REPARSE_POINT, a symbolic link is opened itself (for
// DeviceIoControl). FILE_FLAG_DELETE_ON_CLOSE unlinks the file right away.
HANDLE CreateFile(
  LPCWSTR FileName, DWORD DesiredAccess, DWORD ShareMode, LPSECURITY_ATTRIBUTES SecurityAttributes,
  DWORD CreationDisposition, DWORD FlagsAndAttributes, HANDLE TemplateFile);
BOOL ReadFile(HANDLE File, LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED Overlapped);
BOOL WriteFile(
  HANDLE File, LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED Overlapped);
BOOL FlushFileBuffers(HANDLE File);
BOOL SetEndOfFile(HANDLE File);
DWORD SetFilePointer(HANDLE File, LONG DistanceToMove, PLONG DistanceToMoveHigh, DWORD MoveMethod);
BOOL SetFilePointerEx(HANDLE File, LARGE_INTEGER DistanceToMove, PLARGE_INTEGER NewFilePointer, DWORD MoveMethod);
DWORD GetFileSize(HANDLE File, LPDWORD FileSizeHigh);
// The core passes unsigned long, which is 32-bit on Windows only (a template,
// so that NULL selects the overload above)
template <typename T, typename = std::enable_if_t<std::is_same_v<T, unsigned long>>>
DWORD GetFileSize(HANDLE File, T * FileSizeHigh)
{
  DWORD High = 0;
  DWORD Result = GetFileSize(File, &High);
  *FileSizeHigh = High;
  return Result;
}
BOOL GetFileSizeEx(HANDLE File, PLARGE_INTEGER FileSize);
// The creation time is the birth time where the file system records it,
// otherwise the modification time; setting it is not supported and ignored.
BOOL GetFileTime(HANDLE File, LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime);
BOOL SetFileTime(
  HANDLE File, const FILETIME * CreationTime, const FILETIME * LastAccessTime, const FILETIME * LastWriteTime);
//---------------------------------------------------------------------------
DWORD GetFileAttributes(LPCWSTR FileName);
// Supports FILE_ATTRIBUTE_READONLY only
BOOL SetFileAttributes(LPCWSTR FileName, DWORD FileAttributes);
//---------------------------------------------------------------------------
// Supports FSCTL_GET_REPARSE_POINT on a symbolic link opened with
// FILE_FLAG_OPEN_REPARSE_POINT: returns a REPARSE_DATA_BUFFER with
// IO_REPARSE_TAG_SYMLINK whose substitute and print names are the target.
BOOL DeviceIoControl(
  HANDLE Device, DWORD IoControlCode, LPVOID InBuffer, DWORD InBufferSize, LPVOID OutBuffer, DWORD OutBufferSize,
  LPDWORD BytesReturned, LPOVERLAPPED Overlapped);
//---------------------------------------------------------------------------
#endif
