// WinSock on POSIX sockets, see winapi/WinSock.h. WSAAsyncSelect and
// WSAAsyncGetHostByName are in Messages.cpp, with the message queues.
//---------------------------------------------------------------------------
#include <winapi/WinSock.h>
#include <winapi/Messages.h>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
//---------------------------------------------------------------------------
namespace winapi
{
//---------------------------------------------------------------------------
int ErrnoToWsaError(int Error)
{
  switch (Error)
  {
    // A non-blocking connect on Windows fails with WSAEWOULDBLOCK
    case EINPROGRESS: return WSAEWOULDBLOCK;
    case EPIPE: return WSAECONNRESET;
    default: return static_cast<int>(rtl::ErrnoToWinSockError(Error));
  }
}
//---------------------------------------------------------------------------
int FailWithErrno(int Error)
{
  WSASetLastError(ErrnoToWsaError(Error));
  return SOCKET_ERROR;
}
//---------------------------------------------------------------------------
} // namespace winapi
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The result of a POSIX socket call; the error of a failed one
int Result(int Value)
{
  return (Value < 0) ? winapi::FailWithErrno(errno) : Value;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
int WSAStartup(WORD VersionRequested, LPWSADATA WSAData)
{
  if (WSAData == NULL)
  {
    return WSAEFAULT;
  }
  BYTE Major = LOBYTE(VersionRequested);
  BYTE Minor = HIBYTE(VersionRequested);
  if ((Major < 1) || (Major > 2) || ((Major == 2) && (Minor > 2)))
  {
    return WSAVERNOTSUPPORTED;
  }
  memset(WSAData, 0, sizeof(*WSAData));
  WSAData->wVersion = VersionRequested;
  WSAData->wHighVersion = MAKEWORD(2, 2);
  strcpy(WSAData->szDescription, "OpenSCP WinSock");
  strcpy(WSAData->szSystemStatus, "Running");
  return 0;
}
//---------------------------------------------------------------------------
int WSACleanup()
{
  return 0;
}
//---------------------------------------------------------------------------
int WSAGetLastError()
{
  return static_cast<int>(GetLastError());
}
//---------------------------------------------------------------------------
void WSASetLastError(int Error)
{
  SetLastError(static_cast<DWORD>(Error));
}
//---------------------------------------------------------------------------
int closesocket(SOCKET Socket)
{
  int Fd = static_cast<int>(Socket);
  winapi::SelectSocket(Fd, NULL, 0);
  winapi::SocketClosed(Fd);
  return Result(close(Fd));
}
//---------------------------------------------------------------------------
int WSAEventSelect(SOCKET Socket, WSAEVENT Event, long NetworkEvents)
{
  int Fd = static_cast<int>(Socket);
  if (!winapi::SelectSocket(Fd, Event, NetworkEvents))
  {
    return winapi::FailWithErrno(EINVAL);
  }
  if (NetworkEvents != 0)
  {
    // As on Windows, the socket becomes non-blocking
    int Flags = fcntl(Fd, F_GETFL);
    if ((Flags < 0) || (fcntl(Fd, F_SETFL, Flags | O_NONBLOCK) < 0))
    {
      return winapi::FailWithErrno(errno);
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
int WSAEnumNetworkEvents(SOCKET Socket, WSAEVENT, LPWSANETWORKEVENTS NetworkEvents)
{
  int Fd = static_cast<int>(Socket);
  memset(NetworkEvents, 0, sizeof(*NetworkEvents));
  pollfd Poll = { Fd, POLLIN | POLLPRI, 0 };
  if (poll(&Poll, 1, 0) < 0)
  {
    return winapi::FailWithErrno(errno);
  }
  if ((Poll.revents & POLLNVAL) != 0)
  {
    return winapi::FailWithErrno(ENOTSOCK);
  }
  if ((Poll.revents & POLLPRI) != 0)
  {
    NetworkEvents->lNetworkEvents |= FD_OOB;
  }
  if ((Poll.revents & (POLLERR | POLLHUP)) != 0)
  {
    int Error = 0;
    socklen_t ErrorLen = sizeof(Error);
    ::getsockopt(Fd, SOL_SOCKET, SO_ERROR, &Error, &ErrorLen);
    NetworkEvents->lNetworkEvents |= FD_CLOSE;
    NetworkEvents->iErrorCode[FD_CLOSE_BIT] = (Error != 0) ? winapi::ErrnoToWsaError(Error) : 0;
  }
  else if ((Poll.revents & POLLIN) != 0)
  {
    int Listening = 0;
    socklen_t ListeningLen = sizeof(Listening);
    char Byte;
    if ((::getsockopt(Fd, SOL_SOCKET, SO_ACCEPTCONN, &Listening, &ListeningLen) == 0) && Listening)
    {
      NetworkEvents->lNetworkEvents |= FD_ACCEPT;
    }
    else if (::recv(Fd, static_cast<void *>(&Byte), 1, MSG_PEEK | MSG_DONTWAIT) == 0)
    {
      NetworkEvents->lNetworkEvents |= FD_CLOSE;
    }
    else
    {
      NetworkEvents->lNetworkEvents |= FD_READ;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
int WSAIoctl(
  SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD, LPDWORD, LPWSAOVERLAPPED, LPWSAOVERLAPPED_COMPLETION_ROUTINE)
{
  return winapi::FailWithErrno(EOPNOTSUPP);
}
//---------------------------------------------------------------------------
// The socket functions with Windows signatures; the POSIX ones are called
// with arguments that match them exactly
//---------------------------------------------------------------------------
int recv(SOCKET Socket, char * Buffer, int Length, int Flags)
{
  int Fd = static_cast<int>(Socket);
  int Received = static_cast<int>(::recv(Fd, static_cast<void *>(Buffer), static_cast<size_t>(Length), Flags));
  winapi::SocketCalled(Fd, winapi::scRecv, Received, errno);
  return Result(Received);
}
//---------------------------------------------------------------------------
int send(SOCKET Socket, const char * Buffer, int Length, int Flags)
{
  int Fd = static_cast<int>(Socket);
  int Sent = static_cast<int>(
    ::send(Fd, static_cast<const void *>(Buffer), static_cast<size_t>(Length), Flags | MSG_NOSIGNAL));
  winapi::SocketCalled(Fd, winapi::scSend, Sent, errno, Length);
  return Result(Sent);
}
//---------------------------------------------------------------------------
int connect(SOCKET Socket, const sockaddr * Name, int NameLength)
{
  int Fd = static_cast<int>(Socket);
  int Connected = ::connect(Fd, Name, static_cast<socklen_t>(NameLength));
  winapi::SocketCalled(Fd, winapi::scConnect, Connected, errno);
  return Result(Connected);
}
//---------------------------------------------------------------------------
SOCKET accept(SOCKET Socket, sockaddr * Address, int * AddressLength)
{
  int Fd = static_cast<int>(Socket);
  socklen_t Length = (AddressLength != NULL) ? static_cast<socklen_t>(*AddressLength) : 0;
  // As on Windows, the accepted socket is non-blocking as the listening one
  int ListeningFlags = fcntl(Fd, F_GETFL);
  int Flags = SOCK_CLOEXEC | (((ListeningFlags >= 0) && ((ListeningFlags & O_NONBLOCK) != 0)) ? SOCK_NONBLOCK : 0);
  int Accepted = ::accept4(Fd, Address, (AddressLength != NULL) ? &Length : NULL, Flags);
  if ((Accepted >= 0) && (AddressLength != NULL))
  {
    *AddressLength = static_cast<int>(Length);
  }
  winapi::SocketCalled(Fd, winapi::scAccept, Accepted, errno);
  return (Result(Accepted) >= 0) ? static_cast<SOCKET>(Accepted) : INVALID_SOCKET;
}
//---------------------------------------------------------------------------
int bind(SOCKET Socket, const sockaddr * Name, int NameLength)
{
  return Result(::bind(static_cast<int>(Socket), Name, static_cast<socklen_t>(NameLength)));
}
//---------------------------------------------------------------------------
int listen(SOCKET Socket, int Backlog)
{
  int Fd = static_cast<int>(Socket);
  int Listening = ::listen(Fd, Backlog);
  winapi::SocketCalled(Fd, winapi::scListen, Listening, errno);
  return Result(Listening);
}
//---------------------------------------------------------------------------
int shutdown(SOCKET Socket, int How)
{
  return Result(::shutdown(static_cast<int>(Socket), How));
}
//---------------------------------------------------------------------------
int getpeername(SOCKET Socket, sockaddr * Name, int * NameLength)
{
  socklen_t Length = static_cast<socklen_t>(*NameLength);
  int Value = ::getpeername(static_cast<int>(Socket), Name, &Length);
  *NameLength = static_cast<int>(Length);
  return Result(Value);
}
//---------------------------------------------------------------------------
int getsockname(SOCKET Socket, sockaddr * Name, int * NameLength)
{
  socklen_t Length = static_cast<socklen_t>(*NameLength);
  int Value = ::getsockname(static_cast<int>(Socket), Name, &Length);
  *NameLength = static_cast<int>(Length);
  return Result(Value);
}
//---------------------------------------------------------------------------
int getsockopt(SOCKET Socket, int Level, int OptionName, char * OptionValue, int * OptionLength)
{
  socklen_t Length = static_cast<socklen_t>(*OptionLength);
  int Value = ::getsockopt(static_cast<int>(Socket), Level, OptionName, static_cast<void *>(OptionValue), &Length);
  *OptionLength = static_cast<int>(Length);
  return Result(Value);
}
//---------------------------------------------------------------------------
int setsockopt(SOCKET Socket, int Level, int OptionName, const char * OptionValue, int OptionLength)
{
  return Result(::setsockopt(
    static_cast<int>(Socket), Level, OptionName, static_cast<const void *>(OptionValue),
    static_cast<socklen_t>(OptionLength)));
}
//---------------------------------------------------------------------------
int ioctlsocket(SOCKET Socket, long Command, DWORD * Argument)
{
  int Fd = static_cast<int>(Socket);
  if (Command == static_cast<long>(FIONREAD))
  {
    int Available = 0;
    if (ioctl(Fd, FIONREAD, &Available) < 0)
    {
      return winapi::FailWithErrno(errno);
    }
    *Argument = static_cast<DWORD>(Available);
    return 0;
  }
  else if (Command == static_cast<long>(FIONBIO))
  {
    int Flags = fcntl(Fd, F_GETFL);
    if (Flags < 0)
    {
      return winapi::FailWithErrno(errno);
    }
    Flags = (*Argument != 0) ? (Flags | O_NONBLOCK) : (Flags & ~O_NONBLOCK);
    return Result(fcntl(Fd, F_SETFL, Flags));
  }
  else
  {
    return winapi::FailWithErrno(EINVAL);
  }
}
//---------------------------------------------------------------------------
