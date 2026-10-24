// Kernel objects of the Windows API on POSIX, see winapi/Handles.h.
//
// All waitable objects share one mutex and one condition variable; this keeps
// WaitForMultipleObjects simple, and the core waits on a handful of objects
// only. Waits for events with sockets selected into them (WSAEventSelect)
// poll the sockets, in short slices, so that SetEvent is noticed as well.
//---------------------------------------------------------------------------
#include <winapi/Handles.h>

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
const HANDLE CurrentProcessHandle = reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(-1));
const HANDLE CurrentThreadHandle = reinterpret_cast<HANDLE>(static_cast<LONG_PTR>(-2));
//---------------------------------------------------------------------------
std::mutex & RegistryMutex()
{
  static std::mutex Mutex;
  return Mutex;
}
//---------------------------------------------------------------------------
std::unordered_map<HANDLE, std::shared_ptr<winapi::TKernelObject>> & Registry()
{
  static std::unordered_map<HANDLE, std::shared_ptr<winapi::TKernelObject>> Objects;
  return Objects;
}
//---------------------------------------------------------------------------
// Guards the state of all waitable objects
std::mutex & WaitMutex()
{
  static std::mutex Mutex;
  return Mutex;
}
//---------------------------------------------------------------------------
std::condition_variable & WaitCondition()
{
  static std::condition_variable Condition;
  return Condition;
}
//---------------------------------------------------------------------------
class TWaitable : public winapi::TKernelObject
{
public:
  // Called with WaitMutex locked
  virtual bool IsSignaled() const = 0;
  // Called with WaitMutex locked, after a wait was satisfied by the object
  virtual void Acquired() {}
};
//---------------------------------------------------------------------------
class TEvent : public TWaitable
{
public:
  TEvent(bool ManualReset, bool Signaled) :
    FManualReset(ManualReset),
    FSignaled(Signaled)
  {
  }

  void Set(bool Signaled)
  {
    {
      std::lock_guard<std::mutex> Lock(WaitMutex());
      FSignaled = Signaled;
    }
    if (Signaled)
    {
      WaitCondition().notify_all();
    }
  }

  // With WaitMutex locked
  void SetLocked(bool Signaled)
  {
    FSignaled = Signaled;
  }

  bool IsSignaled() const override { return FSignaled; }

  void Acquired() override
  {
    if (!FManualReset)
    {
      FSignaled = false;
    }
  }

private:
  bool FManualReset;
  bool FSignaled;
};
//---------------------------------------------------------------------------
class TThread : public TWaitable
{
public:
  TThread(LPTHREAD_START_ROUTINE StartAddress, LPVOID Parameter) :
    FStartAddress(StartAddress),
    FParameter(Parameter)
  {
  }

  // Starts the system thread, which waits for Resume when Suspended
  DWORD Start(const std::shared_ptr<TThread> & Self, bool Suspended)
  {
    FSuspended = Suspended;
    std::thread Thread(&TThread::Run, Self);
    Thread.detach();
    std::unique_lock<std::mutex> Lock(WaitMutex());
    WaitCondition().wait(Lock, [this]() { return (FThreadId != 0); });
    return FThreadId;
  }

  DWORD Resume()
  {
    DWORD Result;
    {
      std::lock_guard<std::mutex> Lock(WaitMutex());
      Result = FSuspended ? 1 : 0;
      FSuspended = false;
    }
    WaitCondition().notify_all();
    return Result;
  }

  bool IsSignaled() const override { return FFinished; }

  DWORD ExitCode() const
  {
    std::lock_guard<std::mutex> Lock(WaitMutex());
    return FFinished ? FExitCode : STILL_ACTIVE;
  }

private:
  static void Run(std::shared_ptr<TThread> Self)
  {
    {
      std::unique_lock<std::mutex> Lock(WaitMutex());
      Self->FThreadId = GetCurrentThreadId();
      WaitCondition().notify_all();
      WaitCondition().wait(Lock, [&Self]() { return !Self->FSuspended; });
    }
    DWORD ExitCode = Self->FStartAddress(Self->FParameter);
    {
      std::lock_guard<std::mutex> Lock(WaitMutex());
      Self->FExitCode = ExitCode;
      Self->FFinished = true;
    }
    WaitCondition().notify_all();
  }

  LPTHREAD_START_ROUTINE FStartAddress;
  LPVOID FParameter;
  DWORD FThreadId = 0;
  bool FSuspended = false;
  bool FFinished = false;
  DWORD FExitCode = 0;
};
//---------------------------------------------------------------------------
// Sockets selected into events: descriptor -> (event, network events)
struct TSocketSelection
{
  std::weak_ptr<TEvent> Event;
  long NetworkEvents;
};
std::unordered_map<int, TSocketSelection> & SocketSelections()
{
  static std::unordered_map<int, TSocketSelection> Selections;
  return Selections;
}
//---------------------------------------------------------------------------
short PollEvents(long NetworkEvents)
{
  short Result = 0;
  // FD_READ, FD_ACCEPT, FD_CLOSE
  if ((NetworkEvents & (0x01 | 0x08 | 0x20)) != 0)
  {
    Result |= POLLIN;
  }
  // FD_WRITE, FD_CONNECT
  if ((NetworkEvents & (0x02 | 0x10)) != 0)
  {
    Result |= POLLOUT;
  }
  // FD_OOB
  if ((NetworkEvents & 0x04) != 0)
  {
    Result |= POLLPRI;
  }
  return Result;
}
//---------------------------------------------------------------------------
std::vector<std::shared_ptr<TWaitable>> FindWaitables(DWORD Count, const HANDLE * Handles)
{
  std::vector<std::shared_ptr<TWaitable>> Result;
  for (DWORD Index = 0; Index < Count; Index++)
  {
    std::shared_ptr<TWaitable> Waitable = winapi::FindObject<TWaitable>(Handles[Index]);
    if (!Waitable)
    {
      Result.clear();
      break;
    }
    Result.push_back(Waitable);
  }
  return Result;
}
//---------------------------------------------------------------------------
// Called with WaitMutex locked; WAIT_TIMEOUT if the wait is not satisfied
DWORD TryAcquire(const std::vector<std::shared_ptr<TWaitable>> & Waitables, bool WaitAll)
{
  if (WaitAll)
  {
    for (auto & Waitable : Waitables)
    {
      if (!Waitable->IsSignaled())
      {
        return WAIT_TIMEOUT;
      }
    }
    for (auto & Waitable : Waitables)
    {
      Waitable->Acquired();
    }
    return WAIT_OBJECT_0;
  }
  for (size_t Index = 0; Index < Waitables.size(); Index++)
  {
    if (Waitables[Index]->IsSignaled())
    {
      Waitables[Index]->Acquired();
      return WAIT_OBJECT_0 + static_cast<DWORD>(Index);
    }
  }
  return WAIT_TIMEOUT;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
namespace winapi
{
//---------------------------------------------------------------------------
HANDLE RegisterObject(std::shared_ptr<TKernelObject> Object)
{
  HANDLE Result = Object.get();
  std::lock_guard<std::mutex> Lock(RegistryMutex());
  Registry()[Result] = std::move(Object);
  return Result;
}
//---------------------------------------------------------------------------
std::shared_ptr<TKernelObject> FindKernelObject(HANDLE Handle)
{
  std::lock_guard<std::mutex> Lock(RegistryMutex());
  auto I = Registry().find(Handle);
  return (I != Registry().end()) ? I->second : std::shared_ptr<TKernelObject>();
}
//---------------------------------------------------------------------------
int HandleToFd(HANDLE Handle)
{
  intptr_t Value = reinterpret_cast<intptr_t>(Handle);
  if ((Value <= 0) || (Value > INT32_MAX) || FindKernelObject(Handle))
  {
    return -1;
  }
  return static_cast<int>(Value);
}
//---------------------------------------------------------------------------
bool SelectSocket(int Fd, HANDLE Event, long NetworkEvents)
{
  std::shared_ptr<TEvent> Object;
  if (NetworkEvents != 0)
  {
    Object = FindObject<TEvent>(Event);
    if (!Object)
    {
      return false;
    }
  }
  std::lock_guard<std::mutex> Lock(WaitMutex());
  // A socket is selected into one event at a time
  SocketSelections().erase(Fd);
  if (NetworkEvents != 0)
  {
    SocketSelections()[Fd] = { Object, NetworkEvents };
  }
  return true;
}
//---------------------------------------------------------------------------
HANDLE FdToHandle(int Fd)
{
  if (Fd < 0)
  {
    return INVALID_HANDLE_VALUE;
  }
  if (Fd <= STDERR_FILENO)
  {
    int Moved = fcntl(Fd, F_DUPFD_CLOEXEC, STDERR_FILENO + 1);
    int Error = errno;
    close(Fd);
    if (Moved < 0)
    {
      SetLastError(rtl::ErrnoToWinError(Error));
      return INVALID_HANDLE_VALUE;
    }
    Fd = Moved;
  }
  return reinterpret_cast<HANDLE>(static_cast<intptr_t>(Fd));
}
//---------------------------------------------------------------------------
} // namespace winapi
//---------------------------------------------------------------------------
BOOL CloseHandle(HANDLE Object)
{
  if ((Object == CurrentProcessHandle) || (Object == CurrentThreadHandle))
  {
    return TRUE;
  }
  {
    std::lock_guard<std::mutex> Lock(RegistryMutex());
    if (Registry().erase(Object) > 0)
    {
      return TRUE;
    }
  }
  int Fd = winapi::HandleToFd(Object);
  if (Fd < 0)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (close(Fd) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return FALSE;
  }
  return TRUE;
}
//---------------------------------------------------------------------------
HANDLE CreateEvent(LPSECURITY_ATTRIBUTES, BOOL ManualReset, BOOL InitialState, LPCWSTR Name)
{
  if (Name != NULL)
  {
    SetLastError(ERROR_NOT_SUPPORTED);
    return NULL;
  }
  return winapi::RegisterObject(std::make_shared<TEvent>(ManualReset, InitialState));
}
//---------------------------------------------------------------------------
static BOOL SetEventState(HANDLE Event, bool Signaled)
{
  std::shared_ptr<TEvent> Object = winapi::FindObject<TEvent>(Event);
  if (!Object)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  Object->Set(Signaled);
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL SetEvent(HANDLE Event)
{
  return SetEventState(Event, true);
}
//---------------------------------------------------------------------------
BOOL ResetEvent(HANDLE Event)
{
  return SetEventState(Event, false);
}
//---------------------------------------------------------------------------
DWORD WaitForSingleObject(HANDLE Handle, DWORD Milliseconds)
{
  return WaitForMultipleObjects(1, &Handle, TRUE, Milliseconds);
}
//---------------------------------------------------------------------------
DWORD WaitForMultipleObjects(DWORD Count, const HANDLE * Handles, BOOL WaitAll, DWORD Milliseconds)
{
  if ((Count == 0) || (Count > MAXIMUM_WAIT_OBJECTS))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return WAIT_FAILED;
  }
  std::vector<std::shared_ptr<TWaitable>> Waitables = FindWaitables(Count, Handles);
  if (Waitables.empty())
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return WAIT_FAILED;
  }
  auto Deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(Milliseconds);
  std::unique_lock<std::mutex> Lock(WaitMutex());
  while (true)
  {
    DWORD Result = TryAcquire(Waitables, WaitAll);
    if (Result != WAIT_TIMEOUT)
    {
      return Result;
    }
    // The sockets selected into the events being waited for
    std::vector<pollfd> Fds;
    std::vector<std::shared_ptr<TEvent>> FdEvents;
    for (auto & Selection : SocketSelections())
    {
      std::shared_ptr<TEvent> Event = Selection.second.Event.lock();
      for (auto & Waitable : Waitables)
      {
        if (Event && (Event == Waitable))
        {
          Fds.push_back({ Selection.first, PollEvents(Selection.second.NetworkEvents), 0 });
          FdEvents.push_back(Event);
        }
      }
    }
    if (!Fds.empty())
    {
      int Slice = 10;
      if (Milliseconds != INFINITE)
      {
        auto Left = std::chrono::duration_cast<std::chrono::milliseconds>(Deadline - std::chrono::steady_clock::now());
        Slice = std::max(0, std::min(Slice, static_cast<int>(Left.count())));
      }
      Lock.unlock();
      int Ready = poll(Fds.data(), Fds.size(), Slice);
      Lock.lock();
      for (size_t Index = 0; (Ready > 0) && (Index < Fds.size()); Index++)
      {
        if (Fds[Index].revents != 0)
        {
          // Signaled as with WSAEventSelect; a wait acquires it
          FdEvents[Index]->SetLocked(true);
        }
      }
      if ((Ready <= 0) && (Milliseconds != INFINITE) && (std::chrono::steady_clock::now() >= Deadline))
      {
        return TryAcquire(Waitables, WaitAll);
      }
    }
    else if (Milliseconds == INFINITE)
    {
      WaitCondition().wait(Lock);
    }
    else if (WaitCondition().wait_until(Lock, Deadline) == std::cv_status::timeout)
    {
      return TryAcquire(Waitables, WaitAll);
    }
  }
}
//---------------------------------------------------------------------------
HANDLE CreateThread(
  LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE StartAddress, LPVOID Parameter,
  DWORD CreationFlags, LPDWORD ThreadId)
{
  std::shared_ptr<TThread> Thread = std::make_shared<TThread>(StartAddress, Parameter);
  HANDLE Result = winapi::RegisterObject(Thread);
  DWORD Id = Thread->Start(Thread, (CreationFlags & CREATE_SUSPENDED) != 0);
  if (ThreadId != NULL)
  {
    *ThreadId = Id;
  }
  return Result;
}
//---------------------------------------------------------------------------
DWORD ResumeThread(HANDLE Thread)
{
  std::shared_ptr<TThread> Object = winapi::FindObject<TThread>(Thread);
  if (!Object)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return static_cast<DWORD>(-1);
  }
  return Object->Resume();
}
//---------------------------------------------------------------------------
BOOL SetThreadPriority(HANDLE, int)
{
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL GetExitCodeThread(HANDLE Thread, LPDWORD ExitCode)
{
  std::shared_ptr<TThread> Object = winapi::FindObject<TThread>(Thread);
  if (!Object)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  *ExitCode = Object->ExitCode();
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetCurrentThreadId()
{
  return static_cast<DWORD>(syscall(SYS_gettid));
}
//---------------------------------------------------------------------------
DWORD GetCurrentProcessId()
{
  return static_cast<DWORD>(getpid());
}
//---------------------------------------------------------------------------
HANDLE GetCurrentThread()
{
  return CurrentThreadHandle;
}
//---------------------------------------------------------------------------
HANDLE GetCurrentProcess()
{
  return CurrentProcessHandle;
}
//---------------------------------------------------------------------------
void Sleep(DWORD Milliseconds)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(Milliseconds));
}
//---------------------------------------------------------------------------
DWORD SleepEx(DWORD Milliseconds, BOOL)
{
  Sleep(Milliseconds);
  return 0;
}
//---------------------------------------------------------------------------
ULONGLONG GetTickCount64()
{
  struct timespec Now;
  clock_gettime(CLOCK_MONOTONIC, &Now);
  return static_cast<ULONGLONG>(Now.tv_sec) * 1000 + static_cast<ULONGLONG>(Now.tv_nsec / 1000000);
}
//---------------------------------------------------------------------------
DWORD GetTickCount()
{
  return static_cast<DWORD>(GetTickCount64());
}
//---------------------------------------------------------------------------
BOOL QueryPerformanceFrequency(LARGE_INTEGER * Frequency)
{
  Frequency->QuadPart = 1000000000LL;
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL QueryPerformanceCounter(LARGE_INTEGER * PerformanceCount)
{
  struct timespec Now;
  clock_gettime(CLOCK_MONOTONIC, &Now);
  PerformanceCount->QuadPart = static_cast<LONGLONG>(Now.tv_sec) * 1000000000LL + Now.tv_nsec;
  return TRUE;
}
//---------------------------------------------------------------------------
void InitializeCriticalSection(LPCRITICAL_SECTION CriticalSection)
{
  CriticalSection->Mutex = new std::recursive_mutex();
}
//---------------------------------------------------------------------------
void DeleteCriticalSection(LPCRITICAL_SECTION CriticalSection)
{
  delete static_cast<std::recursive_mutex *>(CriticalSection->Mutex);
  CriticalSection->Mutex = NULL;
}
//---------------------------------------------------------------------------
void EnterCriticalSection(LPCRITICAL_SECTION CriticalSection)
{
  static_cast<std::recursive_mutex *>(CriticalSection->Mutex)->lock();
}
//---------------------------------------------------------------------------
void LeaveCriticalSection(LPCRITICAL_SECTION CriticalSection)
{
  static_cast<std::recursive_mutex *>(CriticalSection->Mutex)->unlock();
}
//---------------------------------------------------------------------------
