// Window messages of the Windows API on Linux, see winapi/Messages.h, and
// the asynchronous WinSock functions that post them (WSAAsyncSelect,
// WSAAsyncGetHostByName, see winapi/WinSock.h).
//
// All state (queues, classes, windows, timers, selected sockets) is guarded
// by one mutex, which is never held while window or timer procedures run.
// Each queue has an eventfd that posting a message writes to, so that the
// poll of the queue's thread wakes up.
//
// The network events of a socket selected with WSAAsyncSelect are found by
// polling it in the thread of its window, for the events that are enabled:
// an event that was posted is disabled until the socket function that
// re-enables it is called (SocketCalled), as on Windows. The end of the
// connection (POLLRDHUP) is reported as FD_CLOSE once the data that arrived
// before it was read; as the readiness for reading is level-triggered after
// it, the socket is not polled for reading any more, and recv posts the
// events that follow.
//---------------------------------------------------------------------------
#include <winapi/Messages.h>
#include <winapi/WinSock.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
typedef std::chrono::steady_clock TClock;
const HWND ThreadMessagesOnly = reinterpret_cast<HWND>(static_cast<LONG_PTR>(-1));
// Windows does not let timers be shorter
const UINT MinimumTimerElapse = 10;
//---------------------------------------------------------------------------
struct TQueuedMessage
{
  MSG Msg;
  // The request of a WSAAsyncGetHostByName reply (for WSACancelAsyncRequest)
  HANDLE AsyncRequest;
};
//---------------------------------------------------------------------------
struct TQueue
{
  DWORD ThreadId;
  int WakeFd;
  std::deque<TQueuedMessage> Messages;
};
//---------------------------------------------------------------------------
struct TWindow
{
  WNDPROC WndProc;
  LONG_PTR UserData;
  DWORD ThreadId;
};
//---------------------------------------------------------------------------
struct TTimer
{
  HWND Window;
  UINT_PTR Id;
  DWORD ThreadId;
  TIMERPROC TimerFunc;
  TClock::duration Interval;
  TClock::time_point Due;
};
//---------------------------------------------------------------------------
enum TSocketState { ssUnconnected, ssConnecting, ssConnected, ssListening, ssFailed };
//---------------------------------------------------------------------------
// A socket used with the WinSock functions that track it; its association
// with a window (WSAAsyncSelect) if Window is not NULL
struct TSocket
{
  HWND Window = NULL;
  UINT Message = 0;
  long Events = 0;
  TSocketState State = ssUnconnected;
  // The events that may be posted
  bool ReadEnabled = true;
  bool WriteEnabled = true;
  bool AcceptEnabled = true;
  // The peer closed the connection; FD_CLOSE was posted
  bool PeerClosed = false;
  bool CloseReported = false;
};
//---------------------------------------------------------------------------
struct TState
{
  std::mutex Mutex;
  std::unordered_map<DWORD, std::shared_ptr<TQueue>> Queues;
  std::map<std::wstring, WNDPROC> Classes;
  ATOM LastClass = 0xC000;
  std::unordered_map<HWND, TWindow> Windows;
  uintptr_t LastWindow = 0x10000;
  std::vector<TTimer> Timers;
  UINT_PTR LastThreadTimer = 0;
  std::map<std::wstring, UINT> RegisteredMessages;
  std::unordered_map<int, TSocket> Sockets;
  uintptr_t LastAsyncRequest = 0;
};
//---------------------------------------------------------------------------
// Never destroyed: threads may post messages while the process exits
TState & State()
{
  static TState * Result = new TState();
  return *Result;
}
//---------------------------------------------------------------------------
typedef std::lock_guard<std::mutex> TLock;
//---------------------------------------------------------------------------
void Wake(const TQueue & Queue)
{
  uint64_t One = 1;
  ssize_t Written = write(Queue.WakeFd, &One, sizeof(One));
  (void)Written;
}
//---------------------------------------------------------------------------
void DrainWake(const TQueue & Queue)
{
  uint64_t Value;
  ssize_t Read = read(Queue.WakeFd, &Value, sizeof(Value));
  (void)Read;
}
//---------------------------------------------------------------------------
// With the mutex locked
TQueue * FindQueue(DWORD ThreadId)
{
  auto I = State().Queues.find(ThreadId);
  return (I != State().Queues.end()) ? I->second.get() : NULL;
}
//---------------------------------------------------------------------------
// With the mutex locked
TWindow * FindWindow(HWND Window)
{
  auto I = State().Windows.find(Window);
  return (I != State().Windows.end()) ? &I->second : NULL;
}
//---------------------------------------------------------------------------
// With the mutex locked
void PostLocked(TQueue & Queue, HWND Window, UINT Message, WPARAM wParam, LPARAM lParam, HANDLE AsyncRequest = NULL)
{
  TQueuedMessage Entry;
  Entry.Msg.hwnd = Window;
  Entry.Msg.message = Message;
  Entry.Msg.wParam = wParam;
  Entry.Msg.lParam = lParam;
  Entry.Msg.time = GetTickCount();
  Entry.Msg.pt = { 0, 0 };
  Entry.AsyncRequest = AsyncRequest;
  Queue.Messages.push_back(Entry);
  Wake(Queue);
}
//---------------------------------------------------------------------------
// With the mutex locked: removes the windows and timers of the thread
void ReleaseThreadLocked(DWORD ThreadId)
{
  TState & S = State();
  for (auto I = S.Windows.begin(); I != S.Windows.end();)
  {
    if (I->second.ThreadId == ThreadId)
    {
      for (auto & Socket : S.Sockets)
      {
        if (Socket.second.Window == I->first)
        {
          Socket.second.Window = NULL;
          Socket.second.Events = 0;
        }
      }
      I = S.Windows.erase(I);
    }
    else
    {
      ++I;
    }
  }
  S.Timers.erase(
    std::remove_if(S.Timers.begin(), S.Timers.end(), [ThreadId](const TTimer & Timer) { return Timer.ThreadId == ThreadId; }),
    S.Timers.end());
  S.Queues.erase(ThreadId);
}
//---------------------------------------------------------------------------
// The queue of the calling thread, released when the thread ends
class TThreadQueue
{
public:
  ~TThreadQueue()
  {
    if (Queue)
    {
      {
        TLock Lock(State().Mutex);
        ReleaseThreadLocked(Queue->ThreadId);
      }
      close(Queue->WakeFd);
    }
  }

  std::shared_ptr<TQueue> Queue;
};
thread_local TThreadQueue ThreadQueue;
//---------------------------------------------------------------------------
// Not with the mutex locked
std::shared_ptr<TQueue> CurrentQueue()
{
  if (!ThreadQueue.Queue)
  {
    std::shared_ptr<TQueue> Queue = std::make_shared<TQueue>();
    Queue->ThreadId = GetCurrentThreadId();
    Queue->WakeFd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    TLock Lock(State().Mutex);
    State().Queues[Queue->ThreadId] = Queue;
    ThreadQueue.Queue = Queue;
  }
  return ThreadQueue.Queue;
}
//---------------------------------------------------------------------------
bool Matches(HWND MessageWindow, UINT Message, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax)
{
  if (Window == ThreadMessagesOnly)
  {
    if (MessageWindow != NULL)
    {
      return false;
    }
  }
  else if ((Window != NULL) && (MessageWindow != Window))
  {
    return false;
  }
  return
    ((MsgFilterMin == 0) && (MsgFilterMax == 0)) ||
    ((Message >= MsgFilterMin) && (Message <= MsgFilterMax));
}
//---------------------------------------------------------------------------
// With the mutex locked: the timer of the thread matching the filter that is
// due first
TTimer * FirstTimer(DWORD ThreadId, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax)
{
  TTimer * Result = NULL;
  for (auto & Timer : State().Timers)
  {
    if ((Timer.ThreadId == ThreadId) && Matches(Timer.Window, WM_TIMER, Window, MsgFilterMin, MsgFilterMax) &&
        ((Result == NULL) || (Timer.Due < Result->Due)))
    {
      Result = &Timer;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
// Socket readiness
//---------------------------------------------------------------------------
int SocketError(int Fd)
{
  int Error = 0;
  socklen_t Length = sizeof(Error);
  if (getsockopt(Fd, SOL_SOCKET, SO_ERROR, &Error, &Length) < 0)
  {
    Error = errno;
  }
  return Error;
}
//---------------------------------------------------------------------------
int BytesAvailable(int Fd)
{
  int Available = 0;
  return (ioctl(Fd, FIONREAD, &Available) < 0) ? 0 : Available;
}
//---------------------------------------------------------------------------
// With the mutex locked
bool OwnedBy(const TSocket & Socket, DWORD ThreadId)
{
  TWindow * Window = (Socket.Window != NULL) ? FindWindow(Socket.Window) : NULL;
  return (Window != NULL) && (Window->ThreadId == ThreadId);
}
//---------------------------------------------------------------------------
// With the mutex locked
void PostEvent(int Fd, const TSocket & Socket, long Event, int Error)
{
  if ((Socket.Events & Event) == 0)
  {
    return;
  }
  TWindow * Window = FindWindow(Socket.Window);
  TQueue * Queue = (Window != NULL) ? FindQueue(Window->ThreadId) : NULL;
  if (Queue != NULL)
  {
    PostLocked(
      *Queue, Socket.Window, Socket.Message, static_cast<WPARAM>(Fd),
      WSAMAKESELECTREPLY(Event, (Error != 0) ? winapi::ErrnoToWsaError(Error) : 0));
  }
}
//---------------------------------------------------------------------------
// With the mutex locked: the peer closed the connection; FD_CLOSE once the
// data before it was read
void ReportPeerClosed(int Fd, TSocket & Socket)
{
  Socket.PeerClosed = true;
  if (!Socket.CloseReported)
  {
    if (BytesAvailable(Fd) == 0)
    {
      Socket.CloseReported = true;
      PostEvent(Fd, Socket, FD_CLOSE, 0);
    }
    else if (Socket.ReadEnabled && ((Socket.Events & FD_READ) != 0))
    {
      Socket.ReadEnabled = false;
      PostEvent(Fd, Socket, FD_READ, 0);
    }
  }
}
//---------------------------------------------------------------------------
// With the mutex locked: the poll events for the enabled network events
short PollEvents(const TSocket & Socket)
{
  short Result = 0;
  switch (Socket.State)
  {
    case ssConnecting:
      Result = POLLOUT;
      break;

    case ssListening:
      if (Socket.AcceptEnabled && ((Socket.Events & FD_ACCEPT) != 0))
      {
        Result = POLLIN;
      }
      break;

    case ssConnected:
      if (Socket.CloseReported)
      {
        // Nothing to report any more
      }
      else
      {
        if (Socket.WriteEnabled && ((Socket.Events & FD_WRITE) != 0))
        {
          Result |= POLLOUT;
        }
        // After the end of the connection, recv reports the rest
        if (!Socket.PeerClosed)
        {
          if (Socket.ReadEnabled && ((Socket.Events & FD_READ) != 0))
          {
            Result |= POLLIN;
          }
          if ((Socket.Events & FD_CLOSE) != 0)
          {
            Result |= POLLRDHUP;
          }
        }
      }
      break;

    default:
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
// With the mutex locked: posts the network events of the readiness
void HandleReadiness(int Fd, TSocket & Socket, short Revents)
{
  switch (Socket.State)
  {
    case ssConnecting:
      {
        int Error = SocketError(Fd);
        if ((Error == 0) && ((Revents & POLLOUT) == 0))
        {
          Error = ENOTCONN;
        }
        if (Error != 0)
        {
          Socket.State = ssFailed;
          PostEvent(Fd, Socket, FD_CONNECT, Error);
        }
        else
        {
          Socket.State = ssConnected;
          Socket.ReadEnabled = true;
          Socket.WriteEnabled = true;
          PostEvent(Fd, Socket, FD_CONNECT, 0);
        }
      }
      break;

    case ssListening:
      // (Not listening any more without POLLIN)
      if (Socket.AcceptEnabled && ((Revents & (POLLIN | POLLERR | POLLHUP)) != 0))
      {
        Socket.AcceptEnabled = false;
        if ((Revents & POLLIN) != 0)
        {
          PostEvent(Fd, Socket, FD_ACCEPT, 0);
        }
      }
      break;

    case ssConnected:
      if (Socket.CloseReported)
      {
        break;
      }
      if ((Revents & POLLERR) != 0)
      {
        int Error = SocketError(Fd);
        if (Error != 0)
        {
          // An abortive close: the data is lost
          Socket.PeerClosed = true;
          Socket.CloseReported = true;
          PostEvent(Fd, Socket, FD_CLOSE, Error);
          break;
        }
      }
      if (((Revents & POLLOUT) != 0) && Socket.WriteEnabled)
      {
        Socket.WriteEnabled = false;
        PostEvent(Fd, Socket, FD_WRITE, 0);
      }
      if (Socket.PeerClosed)
      {
        // Polled for writing only; a connection closed in both directions
        // is not writable any more
        if ((Revents & (POLLHUP | POLLERR)) != 0)
        {
          Socket.WriteEnabled = false;
        }
      }
      else
      {
        int Available = BytesAvailable(Fd);
        if (((Revents & POLLIN) != 0) && Socket.ReadEnabled && (Available > 0))
        {
          Socket.ReadEnabled = false;
          PostEvent(Fd, Socket, FD_READ, 0);
        }
        if (((Revents & (POLLRDHUP | POLLHUP)) != 0) || (((Revents & POLLIN) != 0) && (Available == 0)))
        {
          ReportPeerClosed(Fd, Socket);
        }
      }
      break;

    default:
      break;
  }
}
//---------------------------------------------------------------------------
// Polls the sockets of the windows of the queue's thread, and its eventfd
// unless Timeout is 0, and posts their network events
void PollSockets(const std::shared_ptr<TQueue> & Queue, int Timeout)
{
  std::vector<pollfd> Fds;
  {
    TLock Lock(State().Mutex);
    for (auto & Socket : State().Sockets)
    {
      if (OwnedBy(Socket.second, Queue->ThreadId))
      {
        short Events = PollEvents(Socket.second);
        if (Events != 0)
        {
          Fds.push_back({ Socket.first, Events, 0 });
        }
      }
    }
  }
  size_t SocketCount = Fds.size();
  if (Timeout != 0)
  {
    Fds.push_back({ Queue->WakeFd, POLLIN, 0 });
  }
  if (Fds.empty())
  {
    return;
  }
  int SavedErrno = errno;
  int Ready = poll(Fds.data(), Fds.size(), Timeout);
  errno = SavedErrno;
  if (Ready > 0)
  {
    TLock Lock(State().Mutex);
    for (size_t Index = 0; Index < SocketCount; Index++)
    {
      if (Fds[Index].revents != 0)
      {
        auto I = State().Sockets.find(Fds[Index].fd);
        // (Unless the socket was closed or deselected by another thread)
        if ((I != State().Sockets.end()) && OwnedBy(I->second, Queue->ThreadId))
        {
          if ((Fds[Index].revents & POLLNVAL) != 0)
          {
            // Closed without closesocket
            State().Sockets.erase(I);
          }
          else
          {
            HandleReadiness(Fds[Index].fd, I->second, Fds[Index].revents);
          }
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
// GetMessage (Wait) and PeekMessage; 1 if a message was retrieved
int RetrieveMessage(LPMSG Msg, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax, bool Remove, bool Wait)
{
  std::shared_ptr<TQueue> Queue = CurrentQueue();
  while (true)
  {
    DrainWake(*Queue);
    PollSockets(Queue, 0);
    int Timeout = -1;
    {
      TLock Lock(State().Mutex);
      auto & Messages = Queue->Messages;
      auto I = std::find_if(Messages.begin(), Messages.end(), [&](const TQueuedMessage & Entry)
        { return Matches(Entry.Msg.hwnd, Entry.Msg.message, Window, MsgFilterMin, MsgFilterMax); });
      if (I != Messages.end())
      {
        *Msg = I->Msg;
        if (Remove)
        {
          Messages.erase(I);
        }
        return 1;
      }
      TClock::time_point Now = TClock::now();
      TTimer * Timer = FirstTimer(Queue->ThreadId, Window, MsgFilterMin, MsgFilterMax);
      if ((Timer != NULL) && (Timer->Due <= Now))
      {
        Msg->hwnd = Timer->Window;
        Msg->message = WM_TIMER;
        Msg->wParam = Timer->Id;
        Msg->lParam = reinterpret_cast<LPARAM>(Timer->TimerFunc);
        Msg->time = GetTickCount();
        Msg->pt = { 0, 0 };
        if (Remove)
        {
          Timer->Due = Now + Timer->Interval;
        }
        return 1;
      }
      if (!Wait)
      {
        return 0;
      }
      if (Timer != NULL)
      {
        auto Left = std::chrono::duration_cast<std::chrono::milliseconds>(Timer->Due - Now);
        Timeout = static_cast<int>(std::min<long long>(Left.count() + 1, INT32_MAX));
      }
    }
    PollSockets(Queue, Timeout);
  }
}
//---------------------------------------------------------------------------
// Fills the buffer of WSAAsyncGetHostByName with a hostent of the
// addresses; the size needed, 0 if it does not fit
size_t BuildHostEnt(const char * Name, const std::vector<in_addr> & Addresses, char * Buffer, size_t BufferLength)
{
  // hostent, aliases (none), address pointers, addresses, name
  size_t AliasesOffset = sizeof(hostent);
  size_t AddressListOffset = AliasesOffset + sizeof(char *);
  size_t AddressesOffset = AddressListOffset + (Addresses.size() + 1) * sizeof(char *);
  size_t NameOffset = AddressesOffset + Addresses.size() * sizeof(in_addr);
  size_t Needed = NameOffset + strlen(Name) + 1;
  if (Needed > BufferLength)
  {
    return Needed;
  }
  hostent * Host = reinterpret_cast<hostent *>(Buffer);
  char ** Aliases = reinterpret_cast<char **>(Buffer + AliasesOffset);
  char ** AddressList = reinterpret_cast<char **>(Buffer + AddressListOffset);
  Aliases[0] = NULL;
  for (size_t Index = 0; Index < Addresses.size(); Index++)
  {
    char * Address = Buffer + AddressesOffset + Index * sizeof(in_addr);
    memcpy(Address, &Addresses[Index], sizeof(in_addr));
    AddressList[Index] = Address;
  }
  AddressList[Addresses.size()] = NULL;
  strcpy(Buffer + NameOffset, Name);
  Host->h_name = Buffer + NameOffset;
  Host->h_aliases = Aliases;
  Host->h_addrtype = AF_INET;
  Host->h_length = sizeof(in_addr);
  Host->h_addr_list = AddressList;
  return Needed;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
// Windows
//---------------------------------------------------------------------------
ATOM RegisterClassEx(const WNDCLASSEX * WndClass)
{
  if ((WndClass == NULL) || (WndClass->lpszClassName == NULL) || (WndClass->lpfnWndProc == NULL))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  TLock Lock(State().Mutex);
  std::wstring Name(WndClass->lpszClassName);
  if (State().Classes.count(Name) > 0)
  {
    SetLastError(ERROR_CLASS_ALREADY_EXISTS);
    return 0;
  }
  State().Classes[Name] = WndClass->lpfnWndProc;
  return ++State().LastClass;
}
//---------------------------------------------------------------------------
BOOL UnregisterClass(LPCWSTR ClassName, HINSTANCE)
{
  TLock Lock(State().Mutex);
  if (State().Classes.erase(ClassName) == 0)
  {
    SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
    return FALSE;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
HWND CreateWindowEx(DWORD, LPCWSTR ClassName, LPCWSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID)
{
  std::shared_ptr<TQueue> Queue = CurrentQueue();
  TLock Lock(State().Mutex);
  auto Class = State().Classes.find(ClassName);
  if (Class == State().Classes.end())
  {
    SetLastError(ERROR_CANNOT_FIND_WND_CLASS);
    return NULL;
  }
  HWND Result = reinterpret_cast<HWND>(++State().LastWindow);
  State().Windows[Result] = { Class->second, 0, Queue->ThreadId };
  return Result;
}
//---------------------------------------------------------------------------
HWND CreateWindow(
  LPCWSTR ClassName, LPCWSTR WindowName, DWORD Style, int X, int Y, int Width, int Height,
  HWND Parent, HMENU Menu, HINSTANCE Instance, LPVOID Param)
{
  return CreateWindowEx(0, ClassName, WindowName, Style, X, Y, Width, Height, Parent, Menu, Instance, Param);
}
//---------------------------------------------------------------------------
BOOL DestroyWindow(HWND Window)
{
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  if (Object == NULL)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  TQueue * Queue = FindQueue(Object->ThreadId);
  if (Queue != NULL)
  {
    auto & Messages = Queue->Messages;
    Messages.erase(
      std::remove_if(Messages.begin(), Messages.end(), [Window](const TQueuedMessage & Entry) { return Entry.Msg.hwnd == Window; }),
      Messages.end());
  }
  auto & Timers = State().Timers;
  Timers.erase(
    std::remove_if(Timers.begin(), Timers.end(), [Window](const TTimer & Timer) { return Timer.Window == Window; }),
    Timers.end());
  for (auto & Socket : State().Sockets)
  {
    if (Socket.second.Window == Window)
    {
      Socket.second.Window = NULL;
      Socket.second.Events = 0;
    }
  }
  State().Windows.erase(Window);
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL IsWindow(HWND Window)
{
  TLock Lock(State().Mutex);
  return (FindWindow(Window) != NULL) ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
LONG_PTR SetWindowLongPtr(HWND Window, int Index, LONG_PTR NewLong)
{
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  if (Object == NULL)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  LONG_PTR Result;
  switch (Index)
  {
    case GWLP_USERDATA:
      Result = Object->UserData;
      Object->UserData = NewLong;
      break;

    case GWLP_WNDPROC:
      Result = reinterpret_cast<LONG_PTR>(Object->WndProc);
      Object->WndProc = reinterpret_cast<WNDPROC>(NewLong);
      break;

    default:
      SetLastError(ERROR_INVALID_INDEX);
      Result = 0;
      break;
  }
  return Result;
}
//---------------------------------------------------------------------------
LONG_PTR GetWindowLongPtr(HWND Window, int Index)
{
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  if (Object == NULL)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  switch (Index)
  {
    case GWLP_USERDATA:
      return Object->UserData;

    case GWLP_WNDPROC:
      return reinterpret_cast<LONG_PTR>(Object->WndProc);

    default:
      SetLastError(ERROR_INVALID_INDEX);
      return 0;
  }
}
//---------------------------------------------------------------------------
LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM)
{
  return 0;
}
//---------------------------------------------------------------------------
DWORD GetWindowThreadProcessId(HWND Window, LPDWORD ProcessId)
{
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  if (Object == NULL)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  if (ProcessId != NULL)
  {
    *ProcessId = GetCurrentProcessId();
  }
  return Object->ThreadId;
}
//---------------------------------------------------------------------------
// Messages
//---------------------------------------------------------------------------
UINT RegisterWindowMessage(LPCWSTR String)
{
  if ((String == NULL) || (*String == L'\0'))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  TLock Lock(State().Mutex);
  auto & Registered = State().RegisteredMessages;
  auto I = Registered.find(String);
  if (I != Registered.end())
  {
    return I->second;
  }
  UINT Result = 0xC000 + static_cast<UINT>(Registered.size());
  Registered[String] = Result;
  return Result;
}
//---------------------------------------------------------------------------
BOOL PostMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
  if (Window == NULL)
  {
    CurrentQueue();
    return PostThreadMessage(GetCurrentThreadId(), Message, wParam, lParam);
  }
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  TQueue * Queue = (Object != NULL) ? FindQueue(Object->ThreadId) : NULL;
  if (Queue == NULL)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  PostLocked(*Queue, Window, Message, wParam, lParam);
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL PostThreadMessage(DWORD ThreadId, UINT Message, WPARAM wParam, LPARAM lParam)
{
  TLock Lock(State().Mutex);
  TQueue * Queue = FindQueue(ThreadId);
  if (Queue == NULL)
  {
    SetLastError(ERROR_INVALID_THREAD_ID);
    return FALSE;
  }
  PostLocked(*Queue, NULL, Message, wParam, lParam);
  return TRUE;
}
//---------------------------------------------------------------------------
void PostQuitMessage(int ExitCode)
{
  PostMessage(NULL, WM_QUIT, static_cast<WPARAM>(ExitCode), 0);
}
//---------------------------------------------------------------------------
BOOL GetMessage(LPMSG Msg, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax)
{
  if (Msg == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }
  RetrieveMessage(Msg, Window, MsgFilterMin, MsgFilterMax, true, true);
  return (Msg->message == WM_QUIT) ? FALSE : TRUE;
}
//---------------------------------------------------------------------------
BOOL PeekMessage(LPMSG Msg, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax, UINT RemoveMsg)
{
  return RetrieveMessage(Msg, Window, MsgFilterMin, MsgFilterMax, (RemoveMsg & PM_REMOVE) != 0, false) ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
BOOL TranslateMessage(const MSG *)
{
  return FALSE;
}
//---------------------------------------------------------------------------
LRESULT DispatchMessage(const MSG * Msg)
{
  if ((Msg->message == WM_TIMER) && (Msg->lParam != 0))
  {
    reinterpret_cast<TIMERPROC>(Msg->lParam)(Msg->hwnd, WM_TIMER, Msg->wParam, GetTickCount());
    return 0;
  }
  if (Msg->hwnd == NULL)
  {
    return 0;
  }
  WNDPROC WndProc;
  {
    TLock Lock(State().Mutex);
    TWindow * Object = FindWindow(Msg->hwnd);
    if (Object == NULL)
    {
      return 0;
    }
    WndProc = Object->WndProc;
  }
  return WndProc(Msg->hwnd, Msg->message, Msg->wParam, Msg->lParam);
}
//---------------------------------------------------------------------------
// Timers
//---------------------------------------------------------------------------
UINT_PTR SetTimer(HWND Window, UINT_PTR IdEvent, UINT Elapse, TIMERPROC TimerFunc)
{
  std::shared_ptr<TQueue> Queue = CurrentQueue();
  TLock Lock(State().Mutex);
  DWORD ThreadId = Queue->ThreadId;
  if (Window != NULL)
  {
    TWindow * Object = FindWindow(Window);
    if (Object == NULL)
    {
      SetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
    }
    ThreadId = Object->ThreadId;
  }
  auto & Timers = State().Timers;
  auto I = std::find_if(Timers.begin(), Timers.end(), [&](const TTimer & Timer)
    {
      return
        (Timer.Window == Window) && (Timer.Id == IdEvent) &&
        ((Window != NULL) || (Timer.ThreadId == ThreadId));
    });
  TClock::duration Interval = std::chrono::milliseconds(std::max(Elapse, MinimumTimerElapse));
  UINT_PTR Result;
  if (I != Timers.end())
  {
    I->TimerFunc = TimerFunc;
    I->Interval = Interval;
    I->Due = TClock::now() + Interval;
    Result = I->Id;
  }
  else
  {
    UINT_PTR Id = IdEvent;
    if (Window == NULL)
    {
      // A new identifier, unique in the process
      Id = ++State().LastThreadTimer;
    }
    Timers.push_back({ Window, Id, ThreadId, TimerFunc, Interval, TClock::now() + Interval });
    Result = Id;
  }
  TQueue * Owner = FindQueue(ThreadId);
  if (Owner != NULL)
  {
    Wake(*Owner);
  }
  return ((Window != NULL) && (Result == 0)) ? 1 : Result;
}
//---------------------------------------------------------------------------
BOOL KillTimer(HWND Window, UINT_PTR IdEvent)
{
  DWORD ThreadId = GetCurrentThreadId();
  TLock Lock(State().Mutex);
  auto & Timers = State().Timers;
  auto I = std::find_if(Timers.begin(), Timers.end(), [&](const TTimer & Timer)
    {
      return
        (Timer.Window == Window) && (Timer.Id == IdEvent) &&
        ((Window != NULL) || (Timer.ThreadId == ThreadId));
    });
  if (I == Timers.end())
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  Timers.erase(I);
  return TRUE;
}
//---------------------------------------------------------------------------
// WinSock
//---------------------------------------------------------------------------
int WSAAsyncSelect(SOCKET Socket, HWND Window, UINT Message, long Events)
{
  int Fd = static_cast<int>(Socket);
  if (Events != 0)
  {
    int Flags = fcntl(Fd, F_GETFL);
    if ((Flags < 0) || (fcntl(Fd, F_SETFL, Flags | O_NONBLOCK) < 0))
    {
      return SOCKET_ERROR;
    }
    winapi::SelectSocket(Fd, NULL, 0);
  }
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  if ((Events != 0) && (Object == NULL))
  {
    errno = EINVAL;
    return SOCKET_ERROR;
  }
  TSocket & Data = State().Sockets[Fd];
  if (Events == 0)
  {
    Data.Window = NULL;
    Data.Events = 0;
    return 0;
  }
  if (Data.State == ssUnconnected)
  {
    int Listening = 0;
    socklen_t Length = sizeof(Listening);
    sockaddr_storage Address;
    socklen_t AddressLength = sizeof(Address);
    if ((getsockopt(Fd, SOL_SOCKET, SO_ACCEPTCONN, &Listening, &Length) == 0) && Listening)
    {
      Data.State = ssListening;
    }
    else if (::getpeername(Fd, reinterpret_cast<sockaddr *>(&Address), &AddressLength) == 0)
    {
      Data.State = ssConnected;
    }
  }
  Data.Window = Window;
  Data.Message = Message;
  Data.Events = Events;
  // As a new selection, the events that hold already are posted
  Data.ReadEnabled = true;
  Data.WriteEnabled = true;
  Data.AcceptEnabled = true;
  Data.CloseReported = false;
  if ((Data.State == ssConnected) && Data.PeerClosed)
  {
    ReportPeerClosed(Fd, Data);
  }
  TQueue * Queue = FindQueue(Object->ThreadId);
  if (Queue != NULL)
  {
    Wake(*Queue);
  }
  return 0;
}
//---------------------------------------------------------------------------
HANDLE WSAAsyncGetHostByName(HWND Window, UINT Message, const char * Name, char * Buffer, int BufferLength)
{
  addrinfo Hints;
  memset(&Hints, 0, sizeof(Hints));
  Hints.ai_family = AF_INET;
  Hints.ai_socktype = SOCK_STREAM;
  Hints.ai_flags = AI_CANONNAME;
  addrinfo * Info = NULL;
  int Status = getaddrinfo(Name, NULL, &Hints, &Info);
  int Error = 0;
  size_t Needed = 0;
  if (Status != 0)
  {
    Error = (Status == EAI_AGAIN) ? WSATRY_AGAIN : ((Status == EAI_FAIL) ? WSANO_RECOVERY : WSAHOST_NOT_FOUND);
  }
  else
  {
    std::vector<in_addr> Addresses;
    for (addrinfo * Entry = Info; Entry != NULL; Entry = Entry->ai_next)
    {
      in_addr Address = reinterpret_cast<sockaddr_in *>(Entry->ai_addr)->sin_addr;
      if (std::none_of(Addresses.begin(), Addresses.end(), [&](const in_addr & A) { return A.s_addr == Address.s_addr; }))
      {
        Addresses.push_back(Address);
      }
    }
    const char * HostName = ((Info->ai_canonname != NULL) ? Info->ai_canonname : Name);
    Needed = BuildHostEnt(HostName, Addresses, Buffer, static_cast<size_t>(std::max(BufferLength, 0)));
    if (Needed > static_cast<size_t>(std::max(BufferLength, 0)))
    {
      Error = WSAENOBUFS;
    }
    freeaddrinfo(Info);
  }
  TLock Lock(State().Mutex);
  TWindow * Object = FindWindow(Window);
  TQueue * Queue = (Object != NULL) ? FindQueue(Object->ThreadId) : NULL;
  if (Queue == NULL)
  {
    errno = EINVAL;
    return NULL;
  }
  HANDLE Request = reinterpret_cast<HANDLE>(++State().LastAsyncRequest);
  PostLocked(
    *Queue, Window, Message, reinterpret_cast<WPARAM>(Request),
    WSAMAKEASYNCREPLY(std::min<size_t>(Needed, 0xFFFF), Error), Request);
  return Request;
}
//---------------------------------------------------------------------------
int WSACancelAsyncRequest(HANDLE AsyncTaskHandle)
{
  TLock Lock(State().Mutex);
  size_t Removed = 0;
  for (auto & Queue : State().Queues)
  {
    auto & Messages = Queue.second->Messages;
    size_t Count = Messages.size();
    Messages.erase(
      std::remove_if(Messages.begin(), Messages.end(),
        [AsyncTaskHandle](const TQueuedMessage & Entry) { return Entry.AsyncRequest == AsyncTaskHandle; }),
      Messages.end());
    Removed += Count - Messages.size();
  }
  if (Removed == 0)
  {
    errno = EALREADY;
    return SOCKET_ERROR;
  }
  return 0;
}
//---------------------------------------------------------------------------
namespace winapi
{
//---------------------------------------------------------------------------
void SocketCalled(int Fd, TSocketCall Call, int Result, int Error, int Length)
{
  int SavedErrno = errno;
  {
    TLock Lock(State().Mutex);
    auto & Sockets = State().Sockets;
    switch (Call)
    {
      case scConnect:
        if ((Result == 0) || (Error == EINPROGRESS))
        {
          // Even if it connected at once, FD_CONNECT follows
          Sockets[Fd].State = ssConnecting;
        }
        break;

      case scListen:
        if (Result == 0)
        {
          TSocket & Data = Sockets[Fd];
          Data.State = ssListening;
          Data.AcceptEnabled = true;
        }
        break;

      case scAccept:
        {
          TSocket Accepted;
          auto I = Sockets.find(Fd);
          if (I != Sockets.end())
          {
            I->second.AcceptEnabled = true;
            // The accepted socket has the selection of the listening one
            Accepted.Window = I->second.Window;
            Accepted.Message = I->second.Message;
            Accepted.Events = I->second.Events;
          }
          if (Result >= 0)
          {
            Accepted.State = ssConnected;
            Sockets[Result] = Accepted;
          }
        }
        break;

      case scRecv:
        {
          auto I = Sockets.find(Fd);
          if ((I != Sockets.end()) && (I->second.Window != NULL))
          {
            TSocket & Data = I->second;
            Data.ReadEnabled = true;
            // The socket is not polled for reading after the end of the
            // connection: the data left, or the end
            if ((Data.State == ssConnected) && Data.PeerClosed)
            {
              ReportPeerClosed(Fd, Data);
            }
          }
        }
        break;

      case scSend:
        {
          auto I = Sockets.find(Fd);
          // Windows re-enables FD_WRITE after WSAEWOULDBLOCK only; as Linux
          // sends partially when the buffer fills, also after that
          if ((I != Sockets.end()) &&
              (((Result < 0) && ((Error == EWOULDBLOCK) || (Error == EAGAIN))) || ((Result >= 0) && (Result < Length))))
          {
            I->second.WriteEnabled = true;
          }
        }
        break;
    }
    // The thread of the socket's window polls with the new state
    auto I = Sockets.find(Fd);
    TWindow * Window = ((I != Sockets.end()) && (I->second.Window != NULL)) ? FindWindow(I->second.Window) : NULL;
    TQueue * Queue = (Window != NULL) ? FindQueue(Window->ThreadId) : NULL;
    if ((Queue != NULL) && (Queue->ThreadId != GetCurrentThreadId()))
    {
      Wake(*Queue);
    }
  }
  errno = SavedErrno;
}
//---------------------------------------------------------------------------
void SocketClosed(int Fd)
{
  int SavedErrno = errno;
  {
    TLock Lock(State().Mutex);
    State().Sockets.erase(Fd);
  }
  errno = SavedErrno;
}
//---------------------------------------------------------------------------
} // namespace winapi
//---------------------------------------------------------------------------
