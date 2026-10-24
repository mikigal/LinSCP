// System.Classes: synchronization of threads with the main thread
//---------------------------------------------------------------------------
#include <rtl/Classes.h>

#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
//---------------------------------------------------------------------------
namespace System
{
namespace Classes
{
//---------------------------------------------------------------------------
TNotifyEvent WakeMainThread;
//---------------------------------------------------------------------------
namespace
{
struct TSyncRequest
{
  TThreadMethod Method;
  bool Wait = false;
  bool Done = false;
  std::exception_ptr Exception;
};
//---------------------------------------------------------------------------
struct TSyncQueue
{
  std::mutex Mutex;
  std::condition_variable Added;
  std::condition_variable Finished;
  std::deque<std::shared_ptr<TSyncRequest>> Requests;
  std::thread::id MainThread = std::this_thread::get_id();
};
//---------------------------------------------------------------------------
// Constructed by the main thread, before the program's static objects
TSyncQueue SyncQueue __attribute__((init_priority(200)));
//---------------------------------------------------------------------------
void AddRequest(const std::shared_ptr<TSyncRequest> & Request)
{
  {
    std::lock_guard<std::mutex> Lock(SyncQueue.Mutex);
    SyncQueue.Requests.push_back(Request);
  }
  SyncQueue.Added.notify_all();
  if (WakeMainThread != nullptr)
  {
    WakeMainThread(nullptr);
  }
}
} // namespace
//---------------------------------------------------------------------------
void TThread::Synchronize(TThread *, TThreadMethod Method)
{
  if (std::this_thread::get_id() == SyncQueue.MainThread)
  {
    Method();
  }
  else
  {
    std::shared_ptr<TSyncRequest> Request = std::make_shared<TSyncRequest>();
    Request->Method = Method;
    Request->Wait = true;
    AddRequest(Request);
    std::unique_lock<std::mutex> Lock(SyncQueue.Mutex);
    SyncQueue.Finished.wait(Lock, [&]() { return Request->Done; });
    if (Request->Exception)
    {
      std::rethrow_exception(Request->Exception);
    }
  }
}
//---------------------------------------------------------------------------
void TThread::Queue(TThread *, TThreadMethod Method)
{
  if (std::this_thread::get_id() == SyncQueue.MainThread)
  {
    Method();
  }
  else
  {
    std::shared_ptr<TSyncRequest> Request = std::make_shared<TSyncRequest>();
    Request->Method = Method;
    AddRequest(Request);
  }
}
//---------------------------------------------------------------------------
bool CheckSynchronize(int Timeout)
{
  if (std::this_thread::get_id() != SyncQueue.MainThread)
  {
    throw Exception(L"CheckSynchronize called from a thread other than the main one");
  }

  std::deque<std::shared_ptr<TSyncRequest>> Requests;
  {
    std::unique_lock<std::mutex> Lock(SyncQueue.Mutex);
    if (Timeout > 0)
    {
      SyncQueue.Added.wait_for(
        Lock, std::chrono::milliseconds(Timeout), []() { return !SyncQueue.Requests.empty(); });
    }
    Requests.swap(SyncQueue.Requests);
  }

  bool Result = !Requests.empty();
  while (!Requests.empty())
  {
    std::shared_ptr<TSyncRequest> Request = Requests.front();
    Requests.pop_front();
    try
    {
      Request->Method();
    }
    catch (...)
    {
      if (!Request->Wait)
      {
        // Nobody else to raise it to; the remaining requests stay queued
        std::lock_guard<std::mutex> Lock(SyncQueue.Mutex);
        SyncQueue.Requests.insert(SyncQueue.Requests.begin(), Requests.begin(), Requests.end());
        throw;
      }
      Request->Exception = std::current_exception();
    }
    if (Request->Wait)
    {
      {
        std::lock_guard<std::mutex> Lock(SyncQueue.Mutex);
        Request->Done = true;
      }
      SyncQueue.Finished.notify_all();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace Classes
} // namespace System
//---------------------------------------------------------------------------
