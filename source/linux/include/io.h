// The low-level I/O of the Windows C runtime. File handles are file
// descriptors (see winapi/Handles.h), so a C runtime descriptor and the
// handle of a file are the same descriptor.
#pragma once
#include <winapi/Windows.h>

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#define _O_RDONLY O_RDONLY
#define _O_WRONLY O_WRONLY
#define _O_RDWR O_RDWR
#define _O_APPEND O_APPEND
#define _O_BINARY 0
#define _O_TEXT 0

inline int _open_osfhandle(intptr_t Handle, int)
{
  return winapi::HandleToFd(reinterpret_cast<HANDLE>(Handle));
}

inline intptr_t _get_osfhandle(int Fd)
{
  return Fd;
}

inline int _close(int Fd)
{
  return close(Fd);
}

inline int _read(int Fd, void * Buffer, unsigned int Count)
{
  return static_cast<int>(read(Fd, Buffer, Count));
}

inline int _write(int Fd, const void * Buffer, unsigned int Count)
{
  return static_cast<int>(write(Fd, Buffer, Count));
}

inline long long _lseeki64(int Fd, long long Offset, int Origin)
{
  return lseek(Fd, static_cast<off_t>(Offset), Origin);
}
