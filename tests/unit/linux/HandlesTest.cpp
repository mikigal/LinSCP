// Kernel objects of the platform layer: events, threads, waits, timing.
//---------------------------------------------------------------------------
#include <winapi/Windows.h>

#include <gtest/gtest.h>

#include <atomic>
//---------------------------------------------------------------------------
TEST(HandlesTest, AutoResetEvent)
{
  HANDLE Event = CreateEvent(NULL, FALSE, FALSE, NULL);
  ASSERT_NE(Event, (HANDLE)NULL);
  EXPECT_EQ(WaitForSingleObject(Event, 0), static_cast<DWORD>(WAIT_TIMEOUT));
  EXPECT_TRUE(SetEvent(Event));
  EXPECT_EQ(WaitForSingleObject(Event, 0), static_cast<DWORD>(WAIT_OBJECT_0));
  // The wait reset it
  EXPECT_EQ(WaitForSingleObject(Event, 10), static_cast<DWORD>(WAIT_TIMEOUT));
  EXPECT_TRUE(CloseHandle(Event));
}
//---------------------------------------------------------------------------
TEST(HandlesTest, ManualResetEvent)
{
  HANDLE Event = CreateEvent(NULL, TRUE, TRUE, NULL);
  EXPECT_EQ(WaitForSingleObject(Event, 0), static_cast<DWORD>(WAIT_OBJECT_0));
  EXPECT_EQ(WaitForSingleObject(Event, 0), static_cast<DWORD>(WAIT_OBJECT_0));
  EXPECT_TRUE(ResetEvent(Event));
  EXPECT_EQ(WaitForSingleObject(Event, 0), static_cast<DWORD>(WAIT_TIMEOUT));
  CloseHandle(Event);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, WaitForMultipleObjects)
{
  HANDLE Events[2] = { CreateEvent(NULL, FALSE, FALSE, NULL), CreateEvent(NULL, FALSE, FALSE, NULL) };
  SetEvent(Events[1]);
  EXPECT_EQ(WaitForMultipleObjects(2, Events, FALSE, 0), static_cast<DWORD>(WAIT_OBJECT_0 + 1));
  SetEvent(Events[0]);
  EXPECT_EQ(WaitForMultipleObjects(2, Events, TRUE, 0), static_cast<DWORD>(WAIT_TIMEOUT));
  // A failed wait for all does not consume the signaled object
  SetEvent(Events[1]);
  EXPECT_EQ(WaitForMultipleObjects(2, Events, TRUE, 0), static_cast<DWORD>(WAIT_OBJECT_0));
  EXPECT_EQ(WaitForMultipleObjects(2, Events, FALSE, 0), static_cast<DWORD>(WAIT_TIMEOUT));
  CloseHandle(Events[0]);
  CloseHandle(Events[1]);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, WaitTimeout)
{
  HANDLE Event = CreateEvent(NULL, FALSE, FALSE, NULL);
  DWORD Start = GetTickCount();
  EXPECT_EQ(WaitForSingleObject(Event, 100), static_cast<DWORD>(WAIT_TIMEOUT));
  DWORD Elapsed = GetTickCount() - Start;
  EXPECT_GE(Elapsed, 95u);
  EXPECT_LT(Elapsed, 2000u);
  CloseHandle(Event);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, InvalidHandles)
{
  HANDLE Event = CreateEvent(NULL, FALSE, FALSE, NULL);
  CloseHandle(Event);
  EXPECT_EQ(WaitForSingleObject(Event, 0), WAIT_FAILED);
  EXPECT_EQ(GetLastError(), static_cast<DWORD>(ERROR_INVALID_HANDLE));
  EXPECT_FALSE(SetEvent(Event));
  EXPECT_FALSE(CloseHandle(NULL));
  EXPECT_FALSE(CreateEvent(NULL, FALSE, FALSE, L"Named"));
  // Pseudo handles close successfully
  EXPECT_TRUE(CloseHandle(GetCurrentProcess()));
}
//---------------------------------------------------------------------------
namespace
{
std::atomic<int> ThreadRuns(0);
DWORD WINAPI ThreadProc(LPVOID Parameter)
{
  ThreadRuns++;
  HANDLE Event = static_cast<HANDLE>(Parameter);
  WaitForSingleObject(Event, INFINITE);
  return 42;
}
}
//---------------------------------------------------------------------------
TEST(HandlesTest, SuspendedThread)
{
  ThreadRuns = 0;
  HANDLE Event = CreateEvent(NULL, TRUE, FALSE, NULL);
  DWORD ThreadId = 0;
  HANDLE Thread = CreateThread(NULL, 0, ThreadProc, Event, CREATE_SUSPENDED, &ThreadId);
  ASSERT_NE(Thread, (HANDLE)NULL);
  EXPECT_NE(ThreadId, 0u);
  EXPECT_NE(ThreadId, GetCurrentThreadId());
  Sleep(50);
  EXPECT_EQ(ThreadRuns.load(), 0);
  DWORD ExitCode = 0;
  EXPECT_TRUE(GetExitCodeThread(Thread, &ExitCode));
  EXPECT_EQ(ExitCode, static_cast<DWORD>(STILL_ACTIVE));
  EXPECT_TRUE(SetThreadPriority(Thread, THREAD_PRIORITY_BELOW_NORMAL));

  EXPECT_EQ(ResumeThread(Thread), 1u);
  EXPECT_EQ(ResumeThread(Thread), 0u);
  EXPECT_EQ(WaitForSingleObject(Thread, 50), static_cast<DWORD>(WAIT_TIMEOUT));
  SetEvent(Event);
  EXPECT_EQ(WaitForSingleObject(Thread, 5000), static_cast<DWORD>(WAIT_OBJECT_0));
  // A finished thread stays signaled
  EXPECT_EQ(WaitForSingleObject(Thread, 0), static_cast<DWORD>(WAIT_OBJECT_0));
  EXPECT_EQ(ThreadRuns.load(), 1);
  EXPECT_TRUE(GetExitCodeThread(Thread, &ExitCode));
  EXPECT_EQ(ExitCode, 42u);
  CloseHandle(Thread);
  CloseHandle(Event);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, ThreadOutlivesItsHandle)
{
  HANDLE Event = CreateEvent(NULL, TRUE, FALSE, NULL);
  HANDLE Thread = CreateThread(NULL, 0, ThreadProc, Event, 0, NULL);
  EXPECT_TRUE(CloseHandle(Thread));
  SetEvent(Event);
  // Give the thread time to finish while the event exists
  Sleep(50);
  CloseHandle(Event);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, Identity)
{
  EXPECT_EQ(GetCurrentProcessId(), static_cast<DWORD>(getpid()));
  EXPECT_EQ(GetCurrentThreadId(), static_cast<DWORD>(gettid()));
}
//---------------------------------------------------------------------------
TEST(HandlesTest, TickCount)
{
  ULONGLONG Start = GetTickCount64();
  Sleep(20);
  ULONGLONG End = GetTickCount64();
  EXPECT_GE(End - Start, 19u);
  EXPECT_LE(End - Start, 2000u);
  EXPECT_EQ(SleepEx(1, TRUE), 0u);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, CriticalSectionIsRecursive)
{
  CRITICAL_SECTION Section;
  InitializeCriticalSection(&Section);
  EnterCriticalSection(&Section);
  EnterCriticalSection(&Section);
  LeaveCriticalSection(&Section);
  LeaveCriticalSection(&Section);
  DeleteCriticalSection(&Section);
}
//---------------------------------------------------------------------------
TEST(HandlesTest, FileDescriptors)
{
  int Fd = dup(STDERR_FILENO);
  HANDLE Handle = winapi::FdToHandle(Fd);
  EXPECT_EQ(winapi::HandleToFd(Handle), Fd);
  EXPECT_TRUE(CloseHandle(Handle));
  EXPECT_FALSE(CloseHandle(Handle));
  // Descriptors 0 to 2 are moved, so that a file handle is never NULL
  int Saved = dup(STDIN_FILENO);
  close(STDIN_FILENO);
  int Zero = dup(STDERR_FILENO);
  ASSERT_EQ(Zero, 0);
  HANDLE Moved = winapi::FdToHandle(Zero);
  EXPECT_NE(Moved, (HANDLE)NULL);
  EXPECT_GT(winapi::HandleToFd(Moved), STDERR_FILENO);
  CloseHandle(Moved);
  dup2(Saved, STDIN_FILENO);
  close(Saved);
}
//---------------------------------------------------------------------------
