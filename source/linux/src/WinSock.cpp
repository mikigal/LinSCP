// WinSock on POSIX sockets, see winapi/WinSock.h.
//---------------------------------------------------------------------------
#include <winapi/WinSock.h>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
int ErrnoToWsaError(int Error)
{
  switch (Error)
  {
    case 0: return 0;
    case EWOULDBLOCK: return WSAEWOULDBLOCK;
    // A non-blocking connect on Windows fails with WSAEWOULDBLOCK
    case EINPROGRESS: return WSAEWOULDBLOCK;
    case EOPNOTSUPP: return WSAEOPNOTSUPP;
    case ECONNABORTED: return WSAECONNABORTED;
    case ECONNRESET: return WSAECONNRESET;
    case ETIMEDOUT: return WSAETIMEDOUT;
    case ECONNREFUSED: return WSAECONNREFUSED;
    default: return static_cast<int>(rtl::ErrnoToWinError(Error));
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
int WSAGetLastError()
{
  return ErrnoToWsaError(errno);
}
//---------------------------------------------------------------------------
int closesocket(SOCKET Socket)
{
  int Fd = static_cast<int>(Socket);
  winapi::SelectSocket(Fd, NULL, 0);
  return close(Fd);
}
//---------------------------------------------------------------------------
int WSAEventSelect(SOCKET Socket, WSAEVENT Event, long NetworkEvents)
{
  int Fd = static_cast<int>(Socket);
  if (!winapi::SelectSocket(Fd, Event, NetworkEvents))
  {
    errno = EINVAL;
    return SOCKET_ERROR;
  }
  if (NetworkEvents != 0)
  {
    // As on Windows, the socket becomes non-blocking
    int Flags = fcntl(Fd, F_GETFL);
    if ((Flags < 0) || (fcntl(Fd, F_SETFL, Flags | O_NONBLOCK) < 0))
    {
      return SOCKET_ERROR;
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
    return SOCKET_ERROR;
  }
  if ((Poll.revents & POLLNVAL) != 0)
  {
    errno = EBADF;
    return SOCKET_ERROR;
  }
  if ((Poll.revents & POLLPRI) != 0)
  {
    NetworkEvents->lNetworkEvents |= FD_OOB;
  }
  if ((Poll.revents & (POLLERR | POLLHUP)) != 0)
  {
    int Error = 0;
    socklen_t ErrorLen = sizeof(Error);
    getsockopt(Fd, SOL_SOCKET, SO_ERROR, &Error, &ErrorLen);
    NetworkEvents->lNetworkEvents |= FD_CLOSE;
    NetworkEvents->iErrorCode[FD_CLOSE_BIT] = ErrnoToWsaError(Error);
  }
  else if ((Poll.revents & POLLIN) != 0)
  {
    int Listening = 0;
    socklen_t ListeningLen = sizeof(Listening);
    char Byte;
    if ((getsockopt(Fd, SOL_SOCKET, SO_ACCEPTCONN, &Listening, &ListeningLen) == 0) && Listening)
    {
      NetworkEvents->lNetworkEvents |= FD_ACCEPT;
    }
    else if (recv(Fd, &Byte, 1, MSG_PEEK | MSG_DONTWAIT) == 0)
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
  errno = EOPNOTSUPP;
  return SOCKET_ERROR;
}
//---------------------------------------------------------------------------
