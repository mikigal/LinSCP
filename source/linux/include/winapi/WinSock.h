// WinSock on POSIX sockets: the types and constants the core uses, and the
// functions with no POSIX equivalent.
//
// A SOCKET is the socket descriptor (as UINT_PTR, which the core declares
// itself). WSAEventSelect associates sockets with an event: waiting for the
// event (WaitForSingleObject, WaitForMultipleObjects) polls the sockets too,
// and the event becomes signaled when one of them is ready for the selected
// network events. WSAEnumNetworkEvents reports the readiness of a socket at
// the time of the call (level-triggered, unlike on Windows), so FD_WRITE is
// not reported: the core's sessions wait for writability through PuTTY's
// uxsel instead.
//---------------------------------------------------------------------------
#ifndef WinApiWinSockH
#define WinApiWinSockH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
//---------------------------------------------------------------------------
typedef UINT_PTR SOCKET;
typedef HANDLE WSAEVENT;
typedef struct hostent HOSTENT;
typedef struct hostent * LPHOSTENT;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in * LPSOCKADDR_IN;
typedef struct sockaddr_in6 SOCKADDR_IN6;
typedef struct in6_addr IN6_ADDR;
//---------------------------------------------------------------------------
#define INVALID_SOCKET (static_cast<SOCKET>(~0))
#define SOCKET_ERROR (-1)
//---------------------------------------------------------------------------
#define FD_READ_BIT 0
#define FD_READ (1 << FD_READ_BIT)
#define FD_WRITE_BIT 1
#define FD_WRITE (1 << FD_WRITE_BIT)
#define FD_OOB_BIT 2
#define FD_OOB (1 << FD_OOB_BIT)
#define FD_ACCEPT_BIT 3
#define FD_ACCEPT (1 << FD_ACCEPT_BIT)
#define FD_CONNECT_BIT 4
#define FD_CONNECT (1 << FD_CONNECT_BIT)
#define FD_CLOSE_BIT 5
#define FD_CLOSE (1 << FD_CLOSE_BIT)
#define FD_MAX_EVENTS 10
#define FD_ALL_EVENTS ((1 << FD_MAX_EVENTS) - 1)
//---------------------------------------------------------------------------
#define WSAMAKESELECTREPLY(Event, Error) (static_cast<LONG>((static_cast<DWORD>(Event) & 0xFFFF) | (static_cast<DWORD>(Error) << 16)))
#define WSAGETSELECTEVENT(Param) (static_cast<WORD>(static_cast<DWORD_PTR>(Param) & 0xFFFF))
#define WSAGETSELECTERROR(Param) (static_cast<WORD>((static_cast<DWORD_PTR>(Param) >> 16) & 0xFFFF))
//---------------------------------------------------------------------------
// The WinSock error codes the core compares with
#define WSABASEERR 10000
#define WSAEWOULDBLOCK (WSABASEERR + 35)
#define WSAEINPROGRESS (WSABASEERR + 36)
#define WSAEOPNOTSUPP (WSABASEERR + 45)
#define WSAECONNABORTED (WSABASEERR + 53)
#define WSAECONNRESET (WSABASEERR + 54)
#define WSAETIMEDOUT (WSABASEERR + 60)
#define WSAECONNREFUSED (WSABASEERR + 61)
//---------------------------------------------------------------------------
#define SIO_IDEAL_SEND_BACKLOG_QUERY 0x4004747B
//---------------------------------------------------------------------------
struct _WSANETWORKEVENTS
{
  long lNetworkEvents;
  int iErrorCode[FD_MAX_EVENTS];
};
typedef struct _WSANETWORKEVENTS WSANETWORKEVENTS;
typedef struct _WSANETWORKEVENTS * LPWSANETWORKEVENTS;
struct WSAOVERLAPPED;
typedef WSAOVERLAPPED * LPWSAOVERLAPPED;
typedef void (*LPWSAOVERLAPPED_COMPLETION_ROUTINE)(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
// The error of the last socket call (errno), as a WinSock error code
int WSAGetLastError();
int closesocket(SOCKET Socket);
// Network events 0 remove the association
int WSAEventSelect(SOCKET Socket, WSAEVENT Event, long NetworkEvents);
// FD_READ, FD_OOB, FD_CLOSE (with the socket's error) or FD_ACCEPT, as the
// socket is ready now; the event is not used
int WSAEnumNetworkEvents(SOCKET Socket, WSAEVENT Event, LPWSANETWORKEVENTS NetworkEvents);
// No control codes are supported (WSAEOPNOTSUPP)
int WSAIoctl(
  SOCKET Socket, DWORD IoControlCode, LPVOID InBuffer, DWORD InBufferSize, LPVOID OutBuffer, DWORD OutBufferSize,
  LPDWORD BytesReturned, LPWSAOVERLAPPED Overlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
#endif
