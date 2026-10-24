// The System.SyncObjs unit: TCriticalSection and TEvent.
//---------------------------------------------------------------------------
#ifndef RtlSyncObjsH
#define RtlSyncObjsH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <condition_variable>
#include <mutex>
//---------------------------------------------------------------------------
namespace System
{
namespace Syncobjs
{
//---------------------------------------------------------------------------
enum TWaitResult { wrSignaled, wrTimeout, wrAbandoned, wrError, wrIOCompletion };
//---------------------------------------------------------------------------
class TSynchroObject : public TObject
{
public:
  virtual void Acquire() {}
  virtual void Release() {}
  virtual TWaitResult WaitFor(unsigned int Timeout = INFINITE);
};
//---------------------------------------------------------------------------
// Recursive, like a Windows critical section
class TCriticalSection : public TSynchroObject
{
public:
  TCriticalSection() {}
  TCriticalSection(const TCriticalSection &) = delete;
  TCriticalSection & operator =(const TCriticalSection &) = delete;

  virtual void Acquire() override { FMutex.lock(); }
  virtual void Release() override { FMutex.unlock(); }
  bool TryEnter() { return FMutex.try_lock(); }
  void Enter() { Acquire(); }
  void Leave() { Release(); }

private:
  std::recursive_mutex FMutex;
};
//---------------------------------------------------------------------------
class TEvent : public TSynchroObject
{
public:
  TEvent(void * EventAttributes, bool ManualReset, bool InitialState, const UnicodeString Name, bool UseCOMWait = false);
  explicit TEvent(bool UseCOMWait = false);

  virtual TWaitResult WaitFor(unsigned int Timeout = INFINITE) override;
  void SetEvent();
  void ResetEvent();

private:
  std::mutex FMutex;
  std::condition_variable FCondition;
  bool FManualReset;
  bool FSignaled;
};
//---------------------------------------------------------------------------
class TSimpleEvent : public TEvent
{
public:
  explicit TSimpleEvent(bool UseCOMWait = false) : TEvent(UseCOMWait) {}
};
//---------------------------------------------------------------------------
} // namespace Syncobjs
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Syncobjs;
//---------------------------------------------------------------------------
#endif
