// Windows error codes, see rtl/WinError.h
//---------------------------------------------------------------------------
#include <rtl/WinError.h>

#include <cerrno>
//---------------------------------------------------------------------------
namespace
{
thread_local DWORD LastError = 0;

struct TErrorMapping
{
  int Errno;
  DWORD WinError;
};

// The first entry for an errno wins when mapping to Windows, the first entry
// for a Windows code when mapping back (ErrorMappings before
// WinSockErrorMappings).
const TErrorMapping ErrorMappings[] =
{
  { ENOENT, ERROR_FILE_NOT_FOUND },
  { ENOTDIR, ERROR_PATH_NOT_FOUND },
  { EACCES, ERROR_ACCESS_DENIED },
  { EPERM, ERROR_ACCESS_DENIED },
  { EISDIR, ERROR_ACCESS_DENIED },
  { EEXIST, ERROR_ALREADY_EXISTS },
  { EEXIST, ERROR_FILE_EXISTS },
  { ENOTEMPTY, ERROR_DIR_NOT_EMPTY },
  { ENOSPC, ERROR_DISK_FULL },
  { EDQUOT, ERROR_DISK_FULL },
  { EFBIG, ERROR_DISK_FULL },
  { EROFS, ERROR_WRITE_PROTECT },
  { EBADF, ERROR_INVALID_HANDLE },
  { ENOMEM, ERROR_NOT_ENOUGH_MEMORY },
  { EINVAL, ERROR_INVALID_PARAMETER },
  { ENAMETOOLONG, ERROR_FILENAME_EXCED_RANGE },
  { EXDEV, ERROR_NOT_SAME_DEVICE },
  { EBUSY, ERROR_BUSY },
  { EMFILE, ERROR_TOO_MANY_OPEN_FILES },
  { ENFILE, ERROR_TOO_MANY_OPEN_FILES },
  { ELOOP, ERROR_CANT_RESOLVE_FILENAME },
  { EPIPE, ERROR_BROKEN_PIPE },
  { ETIMEDOUT, ERROR_TIMEOUT },
  { ENOTSUP, ERROR_NOT_SUPPORTED },
  { ECANCELED, ERROR_OPERATION_ABORTED },
  { ENODEV, ERROR_NOT_READY },
  { ENXIO, ERROR_NOT_READY },
  { EIO, ERROR_GEN_FAILURE },
  { ETXTBSY, ERROR_SHARING_VIOLATION },
  { EWOULDBLOCK, ERROR_LOCK_VIOLATION },
};

// WinSock errors, after the ones above; the errno values mapped above map
// to WinSock errors only with ErrnoToWinSockError
const TErrorMapping WinSockErrorMappings[] =
{
  { EINTR, WSAEINTR },
  { EBADF, WSAEBADF },
  { EACCES, WSAEACCES },
  { EFAULT, WSAEFAULT },
  { EINVAL, WSAEINVAL },
  { EMFILE, WSAEMFILE },
  { EWOULDBLOCK, WSAEWOULDBLOCK },
  { EINPROGRESS, WSAEINPROGRESS },
  { EALREADY, WSAEALREADY },
  { ENOTSOCK, WSAENOTSOCK },
  { EDESTADDRREQ, WSAEDESTADDRREQ },
  { EMSGSIZE, WSAEMSGSIZE },
  { EPROTOTYPE, WSAEPROTOTYPE },
  { ENOPROTOOPT, WSAENOPROTOOPT },
  { EPROTONOSUPPORT, WSAEPROTONOSUPPORT },
  { ESOCKTNOSUPPORT, WSAESOCKTNOSUPPORT },
  { EOPNOTSUPP, WSAEOPNOTSUPP },
  { EPFNOSUPPORT, WSAEPFNOSUPPORT },
  { EAFNOSUPPORT, WSAEAFNOSUPPORT },
  { EADDRINUSE, WSAEADDRINUSE },
  { EADDRNOTAVAIL, WSAEADDRNOTAVAIL },
  { ENETDOWN, WSAENETDOWN },
  { ENETUNREACH, WSAENETUNREACH },
  { ENETRESET, WSAENETRESET },
  { ECONNABORTED, WSAECONNABORTED },
  { ECONNRESET, WSAECONNRESET },
  { ENOBUFS, WSAENOBUFS },
  { EISCONN, WSAEISCONN },
  { ENOTCONN, WSAENOTCONN },
  { ESHUTDOWN, WSAESHUTDOWN },
  { ETOOMANYREFS, WSAETOOMANYREFS },
  { ETIMEDOUT, WSAETIMEDOUT },
  { ECONNREFUSED, WSAECONNREFUSED },
  { ELOOP, WSAELOOP },
  { ENAMETOOLONG, WSAENAMETOOLONG },
  { EHOSTDOWN, WSAEHOSTDOWN },
  { EHOSTUNREACH, WSAEHOSTUNREACH },
  { ENOTEMPTY, WSAENOTEMPTY },
  { EUSERS, WSAEUSERS },
  { EDQUOT, WSAEDQUOT },
  { ESTALE, WSAESTALE },
  { EREMOTE, WSAEREMOTE },
};
} // namespace
//---------------------------------------------------------------------------
DWORD GetLastError()
{
  return LastError;
}
//---------------------------------------------------------------------------
void SetLastError(DWORD ErrorCode)
{
  LastError = ErrorCode;
}
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
DWORD ErrnoToWinError(int Errno)
{
  if (Errno == 0)
  {
    return ERROR_SUCCESS;
  }
  for (const TErrorMapping & Mapping : ErrorMappings)
  {
    if (Mapping.Errno == Errno)
    {
      return Mapping.WinError;
    }
  }
  return ErrnoToWinSockError(Errno);
}
//---------------------------------------------------------------------------
DWORD ErrnoToWinSockError(int Errno)
{
  if (Errno == 0)
  {
    return ERROR_SUCCESS;
  }
  for (const TErrorMapping & Mapping : WinSockErrorMappings)
  {
    if (Mapping.Errno == Errno)
    {
      return Mapping.WinError;
    }
  }
  return ErrnoFlag | static_cast<DWORD>(Errno);
}
//---------------------------------------------------------------------------
int WinErrorToErrno(DWORD ErrorCode)
{
  if ((ErrorCode & ErrnoFlag) != 0)
  {
    return static_cast<int>(ErrorCode & ~ErrnoFlag);
  }
  for (const TErrorMapping & Mapping : ErrorMappings)
  {
    if (Mapping.WinError == ErrorCode)
    {
      return Mapping.Errno;
    }
  }
  for (const TErrorMapping & Mapping : WinSockErrorMappings)
  {
    if (Mapping.WinError == ErrorCode)
    {
      return Mapping.Errno;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
DWORD SetLastErrorFromErrno()
{
  DWORD Result = ErrnoToWinError(errno);
  SetLastError(Result);
  return Result;
}
//---------------------------------------------------------------------------
} // namespace rtl
