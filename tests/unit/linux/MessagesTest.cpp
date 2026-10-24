// Window messages of the platform layer: thread queues, windows, timers,
// and the network events of WSAAsyncSelect on real loopback connections.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>
#include <winapi/WinSock.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <thread>
#include <vector>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
std::vector<MSG> Dispatched;
//---------------------------------------------------------------------------
LRESULT CALLBACK RecordingWndProc(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
  MSG Msg = {};
  Msg.hwnd = Window;
  Msg.message = Message;
  Msg.wParam = wParam;
  Msg.lParam = lParam;
  Dispatched.push_back(Msg);
  return 0;
}
//---------------------------------------------------------------------------
HWND CreateTestWindow()
{
  WNDCLASSEX Class = {};
  Class.cbSize = sizeof(Class);
  Class.lpfnWndProc = RecordingWndProc;
  Class.lpszClassName = L"OpenSCP Test Window";
  // (Fails when registered by a previous test)
  RegisterClassEx(&Class);
  return CreateWindow(L"OpenSCP Test Window", L"", 0, 0, 0, 0, 0, 0, 0, NULL, 0);
}
//---------------------------------------------------------------------------
// Retrieves the next message matching the filter within the timeout
bool WaitForMessage(MSG & Msg, HWND Window, UINT Min, UINT Max, DWORD Timeout = 5000)
{
  DWORD Start = GetTickCount();
  do
  {
    if (PeekMessage(&Msg, Window, Min, Max, PM_REMOVE))
    {
      return true;
    }
    Sleep(1);
  }
  while (GetTickCount() - Start < Timeout);
  return false;
}
//---------------------------------------------------------------------------
// Waits for the network event of a socket; other messages are dropped
bool WaitForEvent(HWND Window, UINT Message, SOCKET Socket, long Event, int & Error, DWORD Timeout = 5000)
{
  MSG Msg;
  DWORD Start = GetTickCount();
  while (WaitForMessage(Msg, Window, Message, Message, Timeout - std::min(Timeout, GetTickCount() - Start)))
  {
    if ((Msg.wParam == Socket) && (WSAGETSELECTEVENT(Msg.lParam) == Event))
    {
      Error = WSAGETSELECTERROR(Msg.lParam);
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
// The network events posted for the message within the time
std::vector<long> CollectEvents(HWND Window, UINT Message, DWORD Time)
{
  std::vector<long> Result;
  MSG Msg;
  DWORD Start = GetTickCount();
  while (GetTickCount() - Start < Time)
  {
    if (PeekMessage(&Msg, Window, Message, Message, PM_REMOVE))
    {
      Result.push_back(WSAGETSELECTEVENT(Msg.lParam));
    }
    else
    {
      Sleep(1);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class MessagesTest : public testing::Test
{
protected:
  void SetUp() override
  {
    Dispatched.clear();
    FWindow = CreateTestWindow();
    ASSERT_NE(FWindow, (HWND)NULL);
  }

  void TearDown() override
  {
    for (SOCKET Socket : FSockets)
    {
      closesocket(Socket);
    }
    DestroyWindow(FWindow);
    // Drop what is left in the queue
    MSG Msg;
    while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
    {
    }
  }

  SOCKET Socket()
  {
    SOCKET Result = socket(AF_INET, SOCK_STREAM, 0);
    FSockets.push_back(Result);
    return Result;
  }

  // A listening socket on a free loopback port; its address in Address
  SOCKET Listen(sockaddr_in & Address)
  {
    SOCKET Result = Socket();
    memset(&Address, 0, sizeof(Address));
    Address.sin_family = AF_INET;
    Address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    EXPECT_EQ(bind(Result, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)), 0);
    int Length = sizeof(Address);
    EXPECT_EQ(getsockname(Result, reinterpret_cast<sockaddr *>(&Address), &Length), 0);
    EXPECT_EQ(listen(Result, 5), 0);
    return Result;
  }

  // A connected pair: Client selected with ClientEvents (WM_USER + 2),
  // Server accepted and selected with ServerEvents (WM_USER + 3)
  void Connect(SOCKET & Client, SOCKET & Server, long ClientEvents, long ServerEvents)
  {
    sockaddr_in Address;
    SOCKET Listening = Listen(Address);
    ASSERT_EQ(WSAAsyncSelect(Listening, FWindow, WM_USER + 1, FD_ACCEPT), 0);
    Client = Socket();
    ASSERT_EQ(WSAAsyncSelect(Client, FWindow, WM_USER + 2, ClientEvents | FD_CONNECT), 0);
    int Result = connect(Client, reinterpret_cast<sockaddr *>(&Address), sizeof(Address));
    ASSERT_TRUE((Result == 0) || (WSAGetLastError() == WSAEWOULDBLOCK));
    int Error = -1;
    ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 1, Listening, FD_ACCEPT, Error));
    EXPECT_EQ(Error, 0);
    Server = accept(Listening, NULL, NULL);
    ASSERT_NE(Server, INVALID_SOCKET);
    FSockets.push_back(Server);
    ASSERT_EQ(WSAAsyncSelect(Server, FWindow, WM_USER + 3, ServerEvents), 0);
    ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_CONNECT, Error));
    EXPECT_EQ(Error, 0);
  }

  HWND FWindow;
  std::vector<SOCKET> FSockets;
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST_F(MessagesTest, ThreadMessages)
{
  EXPECT_TRUE(PostThreadMessage(GetCurrentThreadId(), WM_USER + 5, 1, 2));
  EXPECT_TRUE(PostMessage(NULL, WM_USER + 6, 3, 4));
  MSG Msg;
  // Filtered by the range; PM_NOREMOVE leaves the message in the queue
  EXPECT_FALSE(PeekMessage(&Msg, NULL, WM_USER + 7, WM_USER + 9, PM_NOREMOVE));
  ASSERT_TRUE(PeekMessage(&Msg, NULL, WM_USER + 6, WM_USER + 6, PM_NOREMOVE));
  EXPECT_EQ(Msg.wParam, 3u);
  EXPECT_EQ(Msg.hwnd, (HWND)NULL);
  // In order
  ASSERT_TRUE(GetMessage(&Msg, NULL, 0, 0));
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_USER + 5));
  EXPECT_EQ(Msg.lParam, 2);
  ASSERT_TRUE(GetMessage(&Msg, NULL, 0, 0));
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_USER + 6));
  EXPECT_FALSE(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE));
  // WM_QUIT ends GetMessage
  PostQuitMessage(7);
  EXPECT_FALSE(GetMessage(&Msg, NULL, 0, 0));
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_QUIT));
  EXPECT_EQ(Msg.wParam, 7u);
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, MessageLoopOfThread)
{
  std::atomic<DWORD> ThreadId(0);
  std::atomic<int> Sum(0);
  std::thread Thread([&]()
    {
      MSG Msg;
      // Creates the queue
      PeekMessage(&Msg, NULL, 0, 0, PM_NOREMOVE);
      ThreadId = GetCurrentThreadId();
      while (GetMessage(&Msg, NULL, 0, 0))
      {
        Sum += static_cast<int>(Msg.wParam);
      }
    });
  while (ThreadId == 0)
  {
    Sleep(1);
  }
  for (int Index = 1; Index <= 10; Index++)
  {
    EXPECT_TRUE(PostThreadMessage(ThreadId, WM_USER, Index, 0));
  }
  EXPECT_TRUE(PostThreadMessage(ThreadId, WM_QUIT, 0, 0));
  Thread.join();
  EXPECT_EQ(Sum, 55);
  // The queue ended with the thread
  EXPECT_FALSE(PostThreadMessage(ThreadId, WM_USER, 0, 0));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_INVALID_THREAD_ID));
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, ThreadWithoutQueue)
{
  std::atomic<DWORD> ThreadId(0);
  std::atomic<bool> Done(false);
  std::thread Thread([&]()
    {
      ThreadId = GetCurrentThreadId();
      while (!Done)
      {
        Sleep(1);
      }
    });
  while (ThreadId == 0)
  {
    Sleep(1);
  }
  EXPECT_FALSE(PostThreadMessage(ThreadId, WM_USER, 0, 0));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_INVALID_THREAD_ID));
  Done = true;
  Thread.join();
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, Windows)
{
  EXPECT_TRUE(IsWindow(FWindow));
  EXPECT_EQ(GetWindowThreadProcessId(FWindow, NULL), GetCurrentThreadId());
  EXPECT_EQ(SetWindowLongPtr(FWindow, GWLP_USERDATA, 1234), 0);
  EXPECT_EQ(GetWindowLongPtr(FWindow, GWLP_USERDATA), 1234);
  EXPECT_EQ(CreateWindow(L"No Such Class", L"", 0, 0, 0, 0, 0, 0, 0, NULL, 0), (HWND)NULL);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_CANNOT_FIND_WND_CLASS));

  EXPECT_TRUE(PostMessage(FWindow, WM_USER + 1, 10, 20));
  EXPECT_TRUE(PostThreadMessage(GetCurrentThreadId(), WM_USER + 2, 0, 0));
  MSG Msg;
  // Thread messages only
  ASSERT_TRUE(PeekMessage(&Msg, reinterpret_cast<HWND>(-1), 0, 0, PM_REMOVE));
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_USER + 2));
  // Messages of the window only
  ASSERT_TRUE(PeekMessage(&Msg, FWindow, 0, 0, PM_REMOVE));
  EXPECT_EQ(Msg.hwnd, FWindow);
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_USER + 1));
  DispatchMessage(&Msg);
  ASSERT_EQ(Dispatched.size(), 1u);
  EXPECT_EQ(Dispatched[0].message, static_cast<UINT>(WM_USER + 1));
  EXPECT_EQ(Dispatched[0].wParam, 10u);
  EXPECT_EQ(Dispatched[0].lParam, 20);

  // Destroying the window removes its messages
  HWND Window = CreateTestWindow();
  EXPECT_NE(Window, FWindow);
  EXPECT_TRUE(PostMessage(Window, WM_USER, 0, 0));
  EXPECT_TRUE(DestroyWindow(Window));
  EXPECT_FALSE(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE));
  EXPECT_FALSE(IsWindow(Window));
  EXPECT_FALSE(PostMessage(Window, WM_USER, 0, 0));
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_INVALID_WINDOW_HANDLE));
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, RegisterWindowMessage)
{
  UINT Message = RegisterWindowMessage(L"OpenSCP test message");
  EXPECT_GE(Message, 0xC000u);
  EXPECT_LE(Message, 0xFFFFu);
  EXPECT_EQ(RegisterWindowMessage(L"OpenSCP test message"), Message);
  EXPECT_NE(RegisterWindowMessage(L"OpenSCP other message"), Message);
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, ThreadTimer)
{
  UINT_PTR Timer = SetTimer(NULL, 0, 30, NULL);
  ASSERT_NE(Timer, 0u);
  DWORD Start = GetTickCount();
  MSG Msg;
  ASSERT_TRUE(GetMessage(&Msg, NULL, 0, 0));
  EXPECT_GE(GetTickCount() - Start, 25u);
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_TIMER));
  EXPECT_EQ(Msg.hwnd, (HWND)NULL);
  EXPECT_EQ(Msg.wParam, Timer);
  // Posted messages first
  Sleep(40);
  PostMessage(NULL, WM_USER, 0, 0);
  ASSERT_TRUE(GetMessage(&Msg, NULL, 0, 0));
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_USER));
  ASSERT_TRUE(GetMessage(&Msg, NULL, 0, 0));
  EXPECT_EQ(Msg.message, static_cast<UINT>(WM_TIMER));
  // One WM_TIMER at a time
  EXPECT_FALSE(PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE));
  EXPECT_TRUE(KillTimer(NULL, Timer));
  EXPECT_FALSE(WaitForMessage(Msg, NULL, 0, 0, 100));
  EXPECT_FALSE(KillTimer(NULL, Timer));
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, WindowTimer)
{
  EXPECT_EQ(SetTimer(FWindow, 5, 10, NULL), 5u);
  MSG Msg;
  ASSERT_TRUE(WaitForMessage(Msg, FWindow, WM_TIMER, WM_TIMER));
  EXPECT_EQ(Msg.hwnd, FWindow);
  EXPECT_EQ(Msg.wParam, 5u);
  DispatchMessage(&Msg);
  ASSERT_EQ(Dispatched.size(), 1u);
  EXPECT_EQ(Dispatched[0].message, static_cast<UINT>(WM_TIMER));
  // Destroying the window kills its timers
  DestroyWindow(FWindow);
  EXPECT_FALSE(KillTimer(FWindow, 5));
  FWindow = CreateTestWindow();
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, ConnectAndExchange)
{
  SOCKET Client, Server;
  Connect(Client, Server, FD_READ | FD_WRITE | FD_CLOSE, FD_READ | FD_WRITE | FD_CLOSE);
  int Error = -1;
  // Both can send now
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_WRITE, Error));
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 3, Server, FD_WRITE, Error));

  ASSERT_EQ(send(Server, "hello", 5, 0), 5);
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_READ, Error));
  EXPECT_EQ(Error, 0);
  // Not again until recv is called
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
  char Buffer[16];
  EXPECT_EQ(recv(Client, Buffer, 2, 0), 2);
  // Data is left
  EXPECT_EQ(CollectEvents(FWindow, WM_USER + 2, 50), std::vector<long>({ FD_READ }));
  EXPECT_EQ(recv(Client, Buffer, static_cast<int>(sizeof(Buffer)), 0), 3);
  EXPECT_EQ(memcmp(Buffer, "llo", 3), 0);
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
  // No data now
  EXPECT_EQ(recv(Client, Buffer, static_cast<int>(sizeof(Buffer)), 0), SOCKET_ERROR);
  EXPECT_EQ(WSAGetLastError(), WSAEWOULDBLOCK);

  // The other direction
  ASSERT_EQ(send(Client, "abc", 3, 0), 3);
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 3, Server, FD_READ, Error));
  EXPECT_EQ(recv(Server, Buffer, static_cast<int>(sizeof(Buffer)), 0), 3);

  // Closing: FD_CLOSE once
  closesocket(Server);
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_CLOSE, Error));
  EXPECT_EQ(Error, 0);
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
  EXPECT_EQ(recv(Client, Buffer, static_cast<int>(sizeof(Buffer)), 0), 0);
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, CloseAfterData)
{
  SOCKET Client, Server;
  Connect(Client, Server, FD_READ | FD_CLOSE, FD_READ | FD_CLOSE);
  ASSERT_EQ(send(Server, "last words", 10, 0), 10);
  closesocket(Server);
  int Error = -1;
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_READ, Error));
  // FD_CLOSE only after the data is read
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
  char Buffer[4];
  EXPECT_EQ(recv(Client, Buffer, static_cast<int>(sizeof(Buffer)), 0), 4);
  EXPECT_EQ(CollectEvents(FWindow, WM_USER + 2, 50), std::vector<long>({ FD_READ }));
  char Rest[16];
  EXPECT_EQ(recv(Client, Rest, static_cast<int>(sizeof(Rest)), 0), 6);
  EXPECT_EQ(CollectEvents(FWindow, WM_USER + 2, 50), std::vector<long>({ FD_CLOSE }));
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, AbortiveClose)
{
  SOCKET Client, Server;
  Connect(Client, Server, FD_READ | FD_CLOSE, FD_READ | FD_CLOSE);
  // A reset (SO_LINGER 0)
  linger Linger = { 1, 0 };
  ASSERT_EQ(setsockopt(Server, SOL_SOCKET, SO_LINGER, reinterpret_cast<const char *>(&Linger), sizeof(Linger)), 0);
  closesocket(Server);
  int Error = -1;
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_CLOSE, Error));
  EXPECT_EQ(Error, WSAECONNRESET);
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, WriteReenabledByBlockedSend)
{
  SOCKET Client, Server;
  Connect(Client, Server, FD_WRITE, FD_READ);
  int Error;
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_WRITE, Error));
  int BufferSize = 4096;
  setsockopt(Client, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char *>(&BufferSize), sizeof(BufferSize));
  setsockopt(Server, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char *>(&BufferSize), sizeof(BufferSize));
  // Fill the buffers
  std::vector<char> Data(65536, 'x');
  int Sent;
  int Total = 0;
  while ((Sent = send(Client, Data.data(), static_cast<int>(Data.size()), 0)) > 0)
  {
    Total += Sent;
  }
  ASSERT_EQ(WSAGetLastError(), WSAEWOULDBLOCK);
  EXPECT_GT(Total, 0);
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
  // Space when the peer reads
  std::vector<char> Buffer(65536);
  int Received = 0;
  DWORD Start = GetTickCount();
  bool Writable = false;
  while (!Writable && (GetTickCount() - Start < 5000))
  {
    int Count = recv(Server, Buffer.data(), static_cast<int>(Buffer.size()), 0);
    if (Count > 0)
    {
      Received += Count;
    }
    std::vector<long> Events = CollectEvents(FWindow, WM_USER + 2, 10);
    Writable = !Events.empty();
    if (Writable)
    {
      EXPECT_EQ(Events, std::vector<long>({ FD_WRITE }));
    }
  }
  EXPECT_TRUE(Writable);
  EXPECT_GT(Received, 0);
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, ConnectionRefused)
{
  // A port that was free a moment ago
  sockaddr_in Address;
  SOCKET Listening = Listen(Address);
  closesocket(Listening);
  FSockets.erase(std::find(FSockets.begin(), FSockets.end(), Listening));
  SOCKET Client = Socket();
  ASSERT_EQ(WSAAsyncSelect(Client, FWindow, WM_USER + 2, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE), 0);
  if (connect(Client, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)) != 0)
  {
    if (WSAGetLastError() == WSAEWOULDBLOCK)
    {
      int Error = -1;
      ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 2, Client, FD_CONNECT, Error));
      EXPECT_EQ(Error, WSAECONNREFUSED);
    }
    else
    {
      EXPECT_EQ(WSAGetLastError(), WSAECONNREFUSED);
    }
  }
  else
  {
    ADD_FAILURE() << "connected to a closed port";
  }
  // Nothing else follows
  EXPECT_TRUE(CollectEvents(FWindow, WM_USER + 2, 50).empty());
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, AcceptedSocketInheritsSelection)
{
  sockaddr_in Address;
  SOCKET Listening = Listen(Address);
  ASSERT_EQ(WSAAsyncSelect(Listening, FWindow, WM_USER + 1, FD_ACCEPT | FD_WRITE), 0);
  SOCKET Client = Socket();
  ASSERT_EQ(connect(Client, reinterpret_cast<sockaddr *>(&Address), sizeof(Address)), 0);
  int Error;
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 1, Listening, FD_ACCEPT, Error));
  int Length = sizeof(Address);
  SOCKET Server = accept(Listening, reinterpret_cast<sockaddr *>(&Address), &Length);
  ASSERT_NE(Server, INVALID_SOCKET);
  FSockets.push_back(Server);
  EXPECT_EQ(Length, static_cast<int>(sizeof(sockaddr_in)));
  // Non-blocking, with the events of the listening socket
  char Byte;
  EXPECT_EQ(recv(Server, &Byte, 1, 0), SOCKET_ERROR);
  EXPECT_EQ(WSAGetLastError(), WSAEWOULDBLOCK);
  ASSERT_TRUE(WaitForEvent(FWindow, WM_USER + 1, Server, FD_WRITE, Error));
  // No other pending connection
  EXPECT_EQ(accept(Listening, NULL, NULL), INVALID_SOCKET);
  EXPECT_EQ(WSAGetLastError(), WSAEWOULDBLOCK);
}
//---------------------------------------------------------------------------
TEST_F(MessagesTest, AsyncGetHostByName)
{
  std::vector<char> Buffer(MAXGETHOSTSTRUCT);
  HANDLE Request = WSAAsyncGetHostByName(FWindow, WM_USER + 4, "127.0.0.1", Buffer.data(), MAXGETHOSTSTRUCT);
  ASSERT_NE(Request, (HANDLE)NULL);
  MSG Msg;
  ASSERT_TRUE(WaitForMessage(Msg, FWindow, WM_USER + 4, WM_USER + 4));
  EXPECT_EQ(Msg.wParam, reinterpret_cast<WPARAM>(Request));
  EXPECT_EQ(WSAGETASYNCERROR(Msg.lParam), 0);
  hostent * Host = reinterpret_cast<hostent *>(Buffer.data());
  EXPECT_EQ(Host->h_addrtype, AF_INET);
  ASSERT_NE(Host->h_addr_list[0], nullptr);
  EXPECT_EQ(reinterpret_cast<in_addr *>(Host->h_addr_list[0])->s_addr, htonl(INADDR_LOOPBACK));

  // A cancelled request posts nothing
  Request = WSAAsyncGetHostByName(FWindow, WM_USER + 4, "127.0.0.1", Buffer.data(), MAXGETHOSTSTRUCT);
  EXPECT_EQ(WSACancelAsyncRequest(Request), 0);
  EXPECT_FALSE(PeekMessage(&Msg, FWindow, WM_USER + 4, WM_USER + 4, PM_REMOVE));
  EXPECT_EQ(WSACancelAsyncRequest(Request), SOCKET_ERROR);

  // A too small buffer
  Request = WSAAsyncGetHostByName(FWindow, WM_USER + 4, "127.0.0.1", Buffer.data(), 16);
  ASSERT_TRUE(WaitForMessage(Msg, FWindow, WM_USER + 4, WM_USER + 4));
  EXPECT_EQ(WSAGETASYNCERROR(Msg.lParam), WSAENOBUFS);
  EXPECT_GT(WSAGETASYNCBUFLEN(Msg.lParam), 16);
}
//---------------------------------------------------------------------------
TEST(WinSockTest, LastError)
{
  WSASetLastError(WSAEWOULDBLOCK);
  EXPECT_EQ(WSAGetLastError(), WSAEWOULDBLOCK);
  // Codes without an errno equivalent survive
  WSASetLastError(WSAEINPROGRESS);
  EXPECT_EQ(WSAGetLastError(), WSAEINPROGRESS);
  WSASetLastError(WSANOTINITIALISED);
  EXPECT_EQ(WSAGetLastError(), WSANOTINITIALISED);
  WSASetLastError(WSAECONNABORTED);
  EXPECT_EQ(WSAGetLastError(), WSAECONNABORTED);
  WSASetLastError(0);
  EXPECT_EQ(WSAGetLastError(), 0);

  WSADATA Data;
  EXPECT_EQ(WSAStartup(MAKEWORD(1, 1), &Data), 0);
  EXPECT_EQ(LOBYTE(Data.wVersion), 1);
  EXPECT_EQ(HIBYTE(Data.wVersion), 1);
  EXPECT_EQ(WSAStartup(MAKEWORD(3, 0), &Data), WSAVERNOTSUPPORTED);
  EXPECT_EQ(WSACleanup(), 0);
}
//---------------------------------------------------------------------------
