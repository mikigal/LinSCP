// WinSock on POSIX sockets: the types and constants the upstream code uses,
// the functions with no POSIX equivalent, and the socket functions with
// their Windows signatures and semantics.
//
// A SOCKET is the socket descriptor (as UINT_PTR, which the core declares
// itself). The error of the last socket call is errno; WSAGetLastError maps
// it to a WinSock error code (rtl/WinError.h), WSASetLastError stores one.
//
// WSAEventSelect associates sockets with an event: waiting for the event
// (WaitForSingleObject, WaitForMultipleObjects) polls the sockets too, and
// the event becomes signaled when one of them is ready for the selected
// network events. WSAEnumNetworkEvents reports the readiness of a socket at
// the time of the call (level-triggered, unlike on Windows), so FD_WRITE is
// not reported: the core's sessions wait for writability through PuTTY's
// uxsel instead.
//
// WSAAsyncSelect posts the network events of a socket to a window (see
// winapi/Messages.h) as Windows does: each event once, until the function
// that re-enables it is called (recv for FD_READ, send failing with
// WSAEWOULDBLOCK for FD_WRITE, accept for FD_ACCEPT); FD_CONNECT once a
// connect completes, FD_CLOSE once the peer closed the connection and its
// data was read. The events are found by the thread of the window, while it
// retrieves messages. For this, the socket functions of the C++ overloads
// below (with the SOCKET type and the Windows signatures, which the upstream
// code calls) must be used for selected sockets.
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
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
//---------------------------------------------------------------------------
typedef UINT_PTR SOCKET;
typedef HANDLE WSAEVENT;
typedef struct hostent HOSTENT;
typedef struct hostent * LPHOSTENT;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr * LPSOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr_in * LPSOCKADDR_IN;
typedef struct sockaddr_in6 SOCKADDR_IN6;
typedef struct in_addr IN_ADDR;
typedef struct in_addr * LPIN_ADDR;
typedef struct in6_addr IN6_ADDR;
// The Windows name of the bytes of an IPv6 address
#define s6_bytes s6_addr
//---------------------------------------------------------------------------
#define INVALID_SOCKET (static_cast<SOCKET>(~0))
#define SOCKET_ERROR (-1)
#define SD_RECEIVE SHUT_RD
#define SD_SEND SHUT_WR
#define SD_BOTH SHUT_RDWR
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
#define WSAMAKEASYNCREPLY(BufferLength, Error) WSAMAKESELECTREPLY(BufferLength, Error)
#define WSAGETASYNCBUFLEN(Param) WSAGETSELECTEVENT(Param)
#define WSAGETASYNCERROR(Param) WSAGETSELECTERROR(Param)
// The buffer size WSAAsyncGetHostByName needs at most
#define MAXGETHOSTSTRUCT 1024
//---------------------------------------------------------------------------
#define SIO_IDEAL_SEND_BACKLOG_QUERY 0x4004747B
//---------------------------------------------------------------------------
#define WSADESCRIPTION_LEN 256
#define WSASYS_STATUS_LEN 128
struct WSADATA
{
  WORD wVersion;
  WORD wHighVersion;
  unsigned short iMaxSockets;
  unsigned short iMaxUdpDg;
  char * lpVendorInfo;
  char szDescription[WSADESCRIPTION_LEN + 1];
  char szSystemStatus[WSASYS_STATUS_LEN + 1];
};
typedef WSADATA * LPWSADATA;
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
// Succeed for versions 1.0 to 2.2, reporting the requested one; nothing to
// initialize
int WSAStartup(WORD VersionRequested, LPWSADATA WSAData);
int WSACleanup();
// The error of the last socket call (errno), as a WinSock error code
int WSAGetLastError();
// Codes without an errno equivalent are kept as they are
void WSASetLastError(int Error);
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
// Posts the network events to the window as Message (wParam the socket,
// lParam WSAMAKESELECTREPLY(event, error)), see above; makes the socket
// non-blocking and ends its WSAEventSelect association. Events 0 end the
// association.
int WSAAsyncSelect(SOCKET Socket, HWND Window, UINT Message, long Events);
// Resolves the IPv4 addresses of the name to a hostent in the buffer and
// posts Message to the window (wParam the returned handle, lParam
// WSAMAKEASYNCREPLY(the buffer length needed, error)). The name is resolved
// before the function returns; only the reply is asynchronous.
HANDLE WSAAsyncGetHostByName(HWND Window, UINT Message, const char * Name, char * Buffer, int BufferLength);
// Removes the reply of the request that was not retrieved yet
int WSACancelAsyncRequest(HANDLE AsyncTaskHandle);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
// The socket functions with Windows signatures (overloads of the POSIX ones
// for the SOCKET type). Lengths are int, buffers char; send does not raise
// SIGPIPE.
int recv(SOCKET Socket, char * Buffer, int Length, int Flags);
int send(SOCKET Socket, const char * Buffer, int Length, int Flags);
int connect(SOCKET Socket, const sockaddr * Name, int NameLength);
SOCKET accept(SOCKET Socket, sockaddr * Address, int * AddressLength);
int listen(SOCKET Socket, int Backlog);
int getpeername(SOCKET Socket, sockaddr * Name, int * NameLength);
int getsockname(SOCKET Socket, sockaddr * Name, int * NameLength);
int getsockopt(SOCKET Socket, int Level, int OptionName, char * OptionValue, int * OptionLength);
// FIONREAD and FIONBIO
int ioctlsocket(SOCKET Socket, long Command, DWORD * Argument);
//---------------------------------------------------------------------------
namespace winapi
{
// The WinSock error code of an errno value (WSAEWOULDBLOCK for a connect in
// progress)
int ErrnoToWsaError(int Error);
}
//---------------------------------------------------------------------------
#endif
