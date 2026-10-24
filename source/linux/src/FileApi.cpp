// File functions of the Windows API, see winapi/FileApi.h.
//---------------------------------------------------------------------------
#include <winapi/FileApi.h>
#include <winapi/TimeApi.h>

#include <System.hpp>
#include <SysUtils.hpp>

#include <climits>
#include <cstddef>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
int FailWithErrno()
{
  rtl::SetLastErrorFromErrno();
  return -1;
}
//---------------------------------------------------------------------------
// The descriptor of a file handle, or -1 with ERROR_INVALID_HANDLE
int FileFd(HANDLE File)
{
  int Fd = winapi::HandleToFd(File);
  if (Fd < 0)
  {
    SetLastError(ERROR_INVALID_HANDLE);
  }
  return Fd;
}
//---------------------------------------------------------------------------
// Windows symbolic link reparse data; the core declares the same structure
struct TSymbolicLinkReparseBuffer
{
  ULONG ReparseTag;
  USHORT ReparseDataLength;
  USHORT Reserved;
  USHORT SubstituteNameOffset;
  USHORT SubstituteNameLength;
  USHORT PrintNameOffset;
  USHORT PrintNameLength;
  ULONG Flags;
  WCHAR PathBuffer[1];
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
HANDLE CreateFile(
  LPCWSTR FileName, DWORD DesiredAccess, DWORD, LPSECURITY_ATTRIBUTES,
  DWORD CreationDisposition, DWORD FlagsAndAttributes, HANDLE)
{
  std::string Path = rtl::FileNameToBytes(FileName);
  bool Read = (DesiredAccess & (GENERIC_READ | GENERIC_ALL)) != 0;
  bool Write = (DesiredAccess & (GENERIC_WRITE | GENERIC_ALL)) != 0;
  bool BackupSemantics = (FlagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) != 0;
  int Flags = O_CLOEXEC;
  if ((FlagsAndAttributes & FILE_FLAG_OPEN_REPARSE_POINT) != 0)
  {
    Flags |= O_PATH | O_NOFOLLOW;
  }
  else
  {
    struct stat Info;
    bool Directory = BackupSemantics && (stat(Path.c_str(), &Info) == 0) && S_ISDIR(Info.st_mode);
    if (Directory)
    {
      // Directories open read-only, enough to get and set their times
      Flags |= O_RDONLY | O_DIRECTORY;
    }
    else if (Read && Write)
    {
      Flags |= O_RDWR;
    }
    else if (Write)
    {
      Flags |= O_WRONLY;
    }
    else
    {
      Flags |= O_RDONLY;
    }
  }
  switch (CreationDisposition)
  {
    case CREATE_NEW:
      Flags |= O_CREAT | O_EXCL;
      break;
    case CREATE_ALWAYS:
      Flags |= O_CREAT | O_TRUNC;
      break;
    case OPEN_EXISTING:
      break;
    case OPEN_ALWAYS:
      Flags |= O_CREAT;
      break;
    case TRUNCATE_EXISTING:
      Flags |= O_TRUNC;
      break;
    default:
      SetLastError(ERROR_INVALID_PARAMETER);
      return INVALID_HANDLE_VALUE;
  }
  mode_t Mode = ((FlagsAndAttributes & FILE_ATTRIBUTE_READONLY) != 0) ? 0444 : 0666;
  bool Existed = ((Flags & O_CREAT) != 0) && (access(Path.c_str(), F_OK) == 0);
  int Fd = open(Path.c_str(), Flags, Mode);
  if (Fd < 0)
  {
    // CREATE_NEW of an existing file fails with ERROR_FILE_EXISTS
    SetLastError((errno == EEXIST) ? ERROR_FILE_EXISTS : rtl::ErrnoToWinError(errno));
    return INVALID_HANDLE_VALUE;
  }
  struct stat Info;
  if (((Flags & O_PATH) == 0) && !BackupSemantics && (fstat(Fd, &Info) == 0) && S_ISDIR(Info.st_mode))
  {
    // Windows opens directories with FILE_FLAG_BACKUP_SEMANTICS only
    close(Fd);
    SetLastError(ERROR_ACCESS_DENIED);
    return INVALID_HANDLE_VALUE;
  }
  if ((FlagsAndAttributes & FILE_FLAG_DELETE_ON_CLOSE) != 0)
  {
    unlink(Path.c_str());
  }
  HANDLE Result = winapi::FdToHandle(Fd);
  if (Result != INVALID_HANDLE_VALUE)
  {
    // As on Windows, a successful CREATE_ALWAYS or OPEN_ALWAYS tells whether the file existed
    SetLastError(Existed ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS);
  }
  return Result;
}
//---------------------------------------------------------------------------
BOOL ReadFile(HANDLE File, LPVOID Buffer, DWORD NumberOfBytesToRead, LPDWORD NumberOfBytesRead, LPOVERLAPPED)
{
  int Fd = FileFd(File);
  if (NumberOfBytesRead != NULL)
  {
    *NumberOfBytesRead = 0;
  }
  if (Fd < 0)
  {
    return FALSE;
  }
  DWORD Total = 0;
  while (Total < NumberOfBytesToRead)
  {
    ssize_t Count = read(Fd, static_cast<char *>(Buffer) + Total, NumberOfBytesToRead - Total);
    if (Count < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      FailWithErrno();
      return FALSE;
    }
    if (Count == 0)
    {
      break;
    }
    Total += static_cast<DWORD>(Count);
  }
  if (NumberOfBytesRead != NULL)
  {
    *NumberOfBytesRead = Total;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL WriteFile(
  HANDLE File, LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LPOVERLAPPED)
{
  int Fd = FileFd(File);
  if (NumberOfBytesWritten != NULL)
  {
    *NumberOfBytesWritten = 0;
  }
  if (Fd < 0)
  {
    return FALSE;
  }
  DWORD Total = 0;
  while (Total < NumberOfBytesToWrite)
  {
    ssize_t Count = write(Fd, static_cast<const char *>(Buffer) + Total, NumberOfBytesToWrite - Total);
    if (Count < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      FailWithErrno();
      return FALSE;
    }
    Total += static_cast<DWORD>(Count);
    if (NumberOfBytesWritten != NULL)
    {
      *NumberOfBytesWritten = Total;
    }
  }
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL FlushFileBuffers(HANDLE File)
{
  int Fd = FileFd(File);
  if ((Fd < 0) || (fsync(Fd) != 0))
  {
    if (Fd >= 0)
    {
      FailWithErrno();
    }
    return FALSE;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL SetEndOfFile(HANDLE File)
{
  int Fd = FileFd(File);
  if (Fd < 0)
  {
    return FALSE;
  }
  off_t Position = lseek(Fd, 0, SEEK_CUR);
  if ((Position < 0) || (ftruncate(Fd, Position) != 0))
  {
    FailWithErrno();
    return FALSE;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
static bool Seek(HANDLE File, long long Distance, DWORD MoveMethod, long long & NewPosition)
{
  int Fd = FileFd(File);
  if (Fd < 0)
  {
    return false;
  }
  int Whence;
  switch (MoveMethod)
  {
    case FILE_BEGIN: Whence = SEEK_SET; break;
    case FILE_CURRENT: Whence = SEEK_CUR; break;
    case FILE_END: Whence = SEEK_END; break;
    default:
      SetLastError(ERROR_INVALID_PARAMETER);
      return false;
  }
  off_t Result = lseek(Fd, static_cast<off_t>(Distance), Whence);
  if (Result < 0)
  {
    FailWithErrno();
    return false;
  }
  NewPosition = Result;
  return true;
}
//---------------------------------------------------------------------------
DWORD SetFilePointer(HANDLE File, LONG DistanceToMove, PLONG DistanceToMoveHigh, DWORD MoveMethod)
{
  long long Distance =
    (DistanceToMoveHigh != NULL) ?
      static_cast<long long>((static_cast<unsigned long long>(*DistanceToMoveHigh) << 32) | static_cast<DWORD>(DistanceToMove)) :
      DistanceToMove;
  long long NewPosition;
  if (!Seek(File, Distance, MoveMethod, NewPosition))
  {
    return INVALID_SET_FILE_POINTER;
  }
  if (DistanceToMoveHigh != NULL)
  {
    *DistanceToMoveHigh = static_cast<LONG>(NewPosition >> 32);
  }
  SetLastError(NO_ERROR);
  return static_cast<DWORD>(NewPosition & 0xFFFFFFFF);
}
//---------------------------------------------------------------------------
BOOL SetFilePointerEx(HANDLE File, LARGE_INTEGER DistanceToMove, PLARGE_INTEGER NewFilePointer, DWORD MoveMethod)
{
  long long NewPosition;
  if (!Seek(File, DistanceToMove.QuadPart, MoveMethod, NewPosition))
  {
    return FALSE;
  }
  if (NewFilePointer != NULL)
  {
    NewFilePointer->QuadPart = NewPosition;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL GetFileSizeEx(HANDLE File, PLARGE_INTEGER FileSize)
{
  int Fd = FileFd(File);
  struct stat Info;
  if (Fd < 0)
  {
    return FALSE;
  }
  if (fstat(Fd, &Info) != 0)
  {
    FailWithErrno();
    return FALSE;
  }
  FileSize->QuadPart = Info.st_size;
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetFileSize(HANDLE File, LPDWORD FileSizeHigh)
{
  LARGE_INTEGER Size;
  if (!GetFileSizeEx(File, &Size))
  {
    return INVALID_FILE_SIZE;
  }
  if (FileSizeHigh != NULL)
  {
    *FileSizeHigh = static_cast<DWORD>(Size.QuadPart >> 32);
  }
  // The size may be INVALID_FILE_SIZE, callers check the last error then
  SetLastError(NO_ERROR);
  return static_cast<DWORD>(Size.QuadPart & 0xFFFFFFFF);
}
//---------------------------------------------------------------------------
BOOL GetFileTime(HANDLE File, LPFILETIME CreationTime, LPFILETIME LastAccessTime, LPFILETIME LastWriteTime)
{
  int Fd = FileFd(File);
  if (Fd < 0)
  {
    return FALSE;
  }
  struct statx Info;
  if (statx(Fd, "", AT_EMPTY_PATH, STATX_BASIC_STATS | STATX_BTIME, &Info) != 0)
  {
    FailWithErrno();
    return FALSE;
  }
  auto ToFileTime = [](const struct statx_timestamp & Time)
  {
    struct timespec Spec;
    Spec.tv_sec = Time.tv_sec;
    Spec.tv_nsec = Time.tv_nsec;
    return winapi::TimespecToFileTime(Spec);
  };
  if (CreationTime != NULL)
  {
    *CreationTime = ToFileTime(((Info.stx_mask & STATX_BTIME) != 0) ? Info.stx_btime : Info.stx_mtime);
  }
  if (LastAccessTime != NULL)
  {
    *LastAccessTime = ToFileTime(Info.stx_atime);
  }
  if (LastWriteTime != NULL)
  {
    *LastWriteTime = ToFileTime(Info.stx_mtime);
  }
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL SetFileTime(HANDLE File, const FILETIME *, const FILETIME * LastAccessTime, const FILETIME * LastWriteTime)
{
  int Fd = FileFd(File);
  if (Fd < 0)
  {
    return FALSE;
  }
  struct timespec Times[2];
  Times[0].tv_sec = 0;
  Times[0].tv_nsec = UTIME_OMIT;
  Times[1] = Times[0];
  if (LastAccessTime != NULL)
  {
    Times[0] = winapi::FileTimeToTimespec(*LastAccessTime);
  }
  if (LastWriteTime != NULL)
  {
    Times[1] = winapi::FileTimeToTimespec(*LastWriteTime);
  }
  if (futimens(Fd, Times) != 0)
  {
    FailWithErrno();
    return FALSE;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetFileAttributes(LPCWSTR FileName)
{
  int Attr = FileGetAttr(FileName);
  return (Attr < 0) ? INVALID_FILE_ATTRIBUTES : static_cast<DWORD>(Attr);
}
//---------------------------------------------------------------------------
BOOL SetFileAttributes(LPCWSTR FileName, DWORD FileAttributes)
{
  return FileSetAttr(FileName, static_cast<int>(FileAttributes & FILE_ATTRIBUTE_READONLY)) == 0;
}
//---------------------------------------------------------------------------
BOOL DeviceIoControl(
  HANDLE Device, DWORD IoControlCode, LPVOID, DWORD, LPVOID OutBuffer, DWORD OutBufferSize,
  LPDWORD BytesReturned, LPOVERLAPPED)
{
  int Fd = FileFd(Device);
  if (Fd < 0)
  {
    return FALSE;
  }
  if (IoControlCode != FSCTL_GET_REPARSE_POINT)
  {
    SetLastError(ERROR_NOT_SUPPORTED);
    return FALSE;
  }
  char Target[PATH_MAX];
  ssize_t Length = readlinkat(Fd, "", Target, sizeof(Target));
  if (Length < 0)
  {
    SetLastError(((errno == EINVAL) || (errno == ENOENT)) ? ERROR_NOT_A_REPARSE_POINT : rtl::ErrnoToWinError(errno));
    return FALSE;
  }
  UnicodeString Name = rtl::BytesToFileName(std::string(Target, Length).c_str());
  size_t NameSize = Name.Length() * sizeof(WCHAR);
  size_t Size = offsetof(TSymbolicLinkReparseBuffer, PathBuffer) + 2 * NameSize;
  if ((Size > OutBufferSize) || (2 * NameSize > 0xFFFF))
  {
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
  }
  TSymbolicLinkReparseBuffer * Buffer = static_cast<TSymbolicLinkReparseBuffer *>(OutBuffer);
  Buffer->ReparseTag = IO_REPARSE_TAG_SYMLINK;
  Buffer->ReparseDataLength = static_cast<USHORT>(Size - offsetof(TSymbolicLinkReparseBuffer, SubstituteNameOffset));
  Buffer->Reserved = 0;
  Buffer->SubstituteNameOffset = 0;
  Buffer->SubstituteNameLength = static_cast<USHORT>(NameSize);
  Buffer->PrintNameOffset = static_cast<USHORT>(NameSize);
  Buffer->PrintNameLength = static_cast<USHORT>(NameSize);
  Buffer->Flags = (Name.IsEmpty() || (Name[1] != L'/')) ? 1 /* SYMLINK_FLAG_RELATIVE */ : 0;
  memcpy(Buffer->PathBuffer, Name.c_str(), NameSize);
  memcpy(reinterpret_cast<char *>(Buffer->PathBuffer) + NameSize, Name.c_str(), NameSize);
  if (BytesReturned != NULL)
  {
    *BytesReturned = static_cast<DWORD>(Size);
  }
  return TRUE;
}
//---------------------------------------------------------------------------
// A search of FindFirstFile
class TFindObject : public winapi::TKernelObject
{
public:
  ~TFindObject() override
  {
    FindClose(SearchRec);
  }

  TSearchRec SearchRec;
};
//---------------------------------------------------------------------------
HANDLE FindFirstFile(LPCWSTR FileName, LPWIN32_FIND_DATAW FindFileData)
{
  std::shared_ptr<TFindObject> Object = std::make_shared<TFindObject>();
  // (FindFirst and FindNext set the last error)
  if (FindFirst(FileName, faAnyFile, Object->SearchRec) != 0)
  {
    return INVALID_HANDLE_VALUE;
  }
  *FindFileData = Object->SearchRec.FindData;
  return winapi::RegisterObject(Object);
}
//---------------------------------------------------------------------------
BOOL FindNextFile(HANDLE FindFile, LPWIN32_FIND_DATAW FindFileData)
{
  std::shared_ptr<TFindObject> Object = winapi::FindObject<TFindObject>(FindFile);
  if (!Object)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (FindNext(Object->SearchRec) != 0)
  {
    return FALSE;
  }
  *FindFileData = Object->SearchRec.FindData;
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL FindClose(HANDLE FindFile)
{
  if (!winapi::FindObject<TFindObject>(FindFile))
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  return CloseHandle(FindFile);
}
//---------------------------------------------------------------------------
BOOL CreateDirectory(LPCWSTR PathName, LPSECURITY_ATTRIBUTES)
{
  if (mkdir(rtl::FileNameToBytes(PathName).c_str(), 0777) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return FALSE;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
