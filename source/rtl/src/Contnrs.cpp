// System.Contnrs and System.SyncObjs
//---------------------------------------------------------------------------
#include <rtl/Contnrs.h>
#include <rtl/SyncObjs.h>

#include <chrono>
//---------------------------------------------------------------------------
namespace System
{
namespace Contnrs
{
//---------------------------------------------------------------------------
int TObjectList::FindInstanceOf(TClass AClass, bool AExact, int AStartAt)
{
  for (int I = AStartAt; I < Count; I++)
  {
    TObject * Item = GetItem(I);
    if (Item == nullptr)
    {
      continue;
    }
    if (AExact ? (*Item->ClassType() == *AClass) : Item->InheritsFrom(AClass))
    {
      return I;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TObjectList::Notify(void * Ptr, TListNotification Action)
{
  if ((Action == lnDeleted) && FOwnsObjects)
  {
    delete static_cast<TObject *>(Ptr);
  }
  TList::Notify(Ptr, Action);
}
//---------------------------------------------------------------------------
} // namespace Contnrs
//---------------------------------------------------------------------------
namespace Syncobjs
{
//---------------------------------------------------------------------------
TWaitResult TSynchroObject::WaitFor(unsigned int)
{
  return wrError;
}
//---------------------------------------------------------------------------
TEvent::TEvent(void *, bool ManualReset, bool InitialState, const UnicodeString, bool) :
  FManualReset(ManualReset),
  FSignaled(InitialState)
{
}
//---------------------------------------------------------------------------
TEvent::TEvent(bool) :
  FManualReset(true),
  FSignaled(false)
{
}
//---------------------------------------------------------------------------
TWaitResult TEvent::WaitFor(unsigned int Timeout)
{
  std::unique_lock<std::mutex> Lock(FMutex);
  auto Signaled = [this]() { return FSignaled; };
  if (Timeout == INFINITE)
  {
    FCondition.wait(Lock, Signaled);
  }
  else if (!FCondition.wait_for(Lock, std::chrono::milliseconds(Timeout), Signaled))
  {
    return wrTimeout;
  }
  if (!FManualReset)
  {
    FSignaled = false;
  }
  return wrSignaled;
}
//---------------------------------------------------------------------------
void TEvent::SetEvent()
{
  {
    std::lock_guard<std::mutex> Lock(FMutex);
    FSignaled = true;
  }
  if (FManualReset)
  {
    FCondition.notify_all();
  }
  else
  {
    FCondition.notify_one();
  }
}
//---------------------------------------------------------------------------
void TEvent::ResetEvent()
{
  std::lock_guard<std::mutex> Lock(FMutex);
  FSignaled = false;
}
//---------------------------------------------------------------------------
} // namespace Syncobjs
} // namespace System
