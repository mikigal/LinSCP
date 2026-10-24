// Window messages of the Windows API on Linux, for the upstream code that
// runs a Windows message loop (the FileZilla engine's thread): message
// queues of threads, message-only windows, timers, and the network events
// of WSAAsyncSelect (winapi/WinSock.h).
//
// A thread gets its message queue with its first call of a function of
// this header (as on Windows, PostThreadMessage fails for a thread without
// one); the queue and the windows of the thread end with the thread. Windows
// are message-only: they have a window procedure and user data, nothing
// else. GetMessage waits for posted messages, timers and the network events
// of the sockets selected into the thread's windows at the same time, with
// poll. WM_TIMER is generated when a timer is due and no posted message is
// waiting, one at a time per timer, as on Windows.
//---------------------------------------------------------------------------
#ifndef WinApiMessagesH
#define WinApiMessagesH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>
//---------------------------------------------------------------------------
#define WM_NULL 0x0000
#define WM_DESTROY 0x0002
#define WM_QUIT 0x0012
#define WM_TIMER 0x0113
#define WM_USER 0x0400
#define WM_APP 0x8000
//---------------------------------------------------------------------------
#define PM_NOREMOVE 0x0000
#define PM_REMOVE 0x0001
//---------------------------------------------------------------------------
#define GWLP_WNDPROC (-4)
#define GWLP_USERDATA (-21)
#define HWND_MESSAGE (reinterpret_cast<HWND>(static_cast<LONG_PTR>(-3)))
//---------------------------------------------------------------------------
typedef struct HMENU__ * HMENU;
typedef struct HCURSOR__ * HCURSOR;
typedef struct HBRUSH__ * HBRUSH;
typedef WORD ATOM;
//---------------------------------------------------------------------------
struct POINT
{
  LONG x;
  LONG y;
};
typedef POINT * PPOINT;
typedef POINT * LPPOINT;
//---------------------------------------------------------------------------
struct MSG
{
  HWND hwnd;
  UINT message;
  WPARAM wParam;
  LPARAM lParam;
  DWORD time;
  POINT pt;
};
typedef MSG * PMSG;
typedef MSG * LPMSG;
//---------------------------------------------------------------------------
typedef LRESULT (CALLBACK * WNDPROC)(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
typedef void (CALLBACK * TIMERPROC)(HWND Window, UINT Message, UINT_PTR IdEvent, DWORD Time);
//---------------------------------------------------------------------------
struct WNDCLASSEXW
{
  UINT cbSize;
  UINT style;
  WNDPROC lpfnWndProc;
  int cbClsExtra;
  int cbWndExtra;
  HINSTANCE hInstance;
  HICON hIcon;
  HCURSOR hCursor;
  HBRUSH hbrBackground;
  LPCWSTR lpszMenuName;
  LPCWSTR lpszClassName;
  HICON hIconSm;
};
typedef WNDCLASSEXW WNDCLASSEX;
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
// Classes are process-wide and identified by their names only
ATOM RegisterClassEx(const WNDCLASSEX * WndClass);
BOOL UnregisterClass(LPCWSTR ClassName, HINSTANCE Instance);
// A message-only window of the class, owned by the calling thread; the
// position, size, parent and menu are ignored, no messages are sent
HWND CreateWindowEx(
  DWORD ExStyle, LPCWSTR ClassName, LPCWSTR WindowName, DWORD Style, int X, int Y, int Width, int Height,
  HWND Parent, HMENU Menu, HINSTANCE Instance, LPVOID Param);
HWND CreateWindow(
  LPCWSTR ClassName, LPCWSTR WindowName, DWORD Style, int X, int Y, int Width, int Height,
  HWND Parent, HMENU Menu, HINSTANCE Instance, LPVOID Param);
// Also removes the window's timers and its messages waiting in the queue,
// and ends the WSAAsyncSelect associations with it
BOOL DestroyWindow(HWND Window);
BOOL IsWindow(HWND Window);
// GWLP_USERDATA and GWLP_WNDPROC
LONG_PTR SetWindowLongPtr(HWND Window, int Index, LONG_PTR NewLong);
LONG_PTR GetWindowLongPtr(HWND Window, int Index);
LRESULT DefWindowProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
DWORD GetWindowThreadProcessId(HWND Window, LPDWORD ProcessId);
//---------------------------------------------------------------------------
// A message identifier (0xC000 to 0xFFFF) unique to the string
UINT RegisterWindowMessage(LPCWSTR String);
// A NULL window posts to the calling thread
BOOL PostMessage(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam);
BOOL PostThreadMessage(DWORD ThreadId, UINT Message, WPARAM wParam, LPARAM lParam);
void PostQuitMessage(int ExitCode);
// The window filter: NULL for the messages of the thread and of all its
// windows, -1 for the messages of the thread only; the range 0, 0 for all
// messages. GetMessage returns FALSE for WM_QUIT, -1 on an error.
BOOL GetMessage(LPMSG Msg, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax);
BOOL PeekMessage(LPMSG Msg, HWND Window, UINT MsgFilterMin, UINT MsgFilterMax, UINT RemoveMsg);
// Does nothing (there is no keyboard input)
BOOL TranslateMessage(const MSG * Msg);
// Calls the window procedure, or the timer procedure of WM_TIMER
LRESULT DispatchMessage(const MSG * Msg);
//---------------------------------------------------------------------------
// Without a window, a timer of the calling thread: the identifier is a new
// one, unless the identifier of an existing timer of the thread is given
UINT_PTR SetTimer(HWND Window, UINT_PTR IdEvent, UINT Elapse, TIMERPROC TimerFunc);
BOOL KillTimer(HWND Window, UINT_PTR IdEvent);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
namespace winapi
{
// The socket functions (WinSock.cpp) report their calls of selected
// sockets, which re-enable network events (see WSAAsyncSelect): Result is
// the call's result (the accepted socket of accept), Error its errno, Length
// the length passed to send.
enum TSocketCall { scRecv, scSend, scConnect, scAccept, scListen };
void SocketCalled(int Fd, TSocketCall Call, int Result, int Error, int Length = 0);
void SocketClosed(int Fd);
}
//---------------------------------------------------------------------------
#endif
