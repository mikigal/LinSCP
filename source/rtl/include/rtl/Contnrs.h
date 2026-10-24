// The System.Contnrs unit: TObjectList.
//---------------------------------------------------------------------------
#ifndef RtlContnrsH
#define RtlContnrsH
//---------------------------------------------------------------------------
#include <rtl/Classes.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Contnrs
{
//---------------------------------------------------------------------------
class TObjectList : public TList
{
public:
  TObjectList() : FOwnsObjects(true) {}
  explicit TObjectList(bool AOwnsObjects) : FOwnsObjects(AOwnsObjects) {}
  // Delphi's TList destructor clears through the virtual Notify; in C++ the
  // derived part is gone by then, so the owned objects are freed here
  virtual ~TObjectList() { Clear(); }

  int Add(TObject * AObject) { return TList::Add(AObject); }
  TObject * Extract(TObject * Item) { return static_cast<TObject *>(TList::Extract(Item)); }
  int Remove(TObject * AObject) { return TList::Remove(AObject); }
  int IndexOf(TObject * AObject) { return TList::IndexOf(AObject); }
  int FindInstanceOf(TClass AClass, bool AExact = true, int AStartAt = 0);
  void Insert(int Index, TObject * AObject) { TList::Insert(Index, AObject); }
  TObject * First() { return static_cast<TObject *>(TList::First()); }
  TObject * Last() { return static_cast<TObject *>(TList::Last()); }

  TObject * GetItem(int Index) { return static_cast<TObject *>(Get(Index)); }
  void SetItem(int Index, TObject * AObject) { Put(Index, AObject); }
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  __declspec(property(get = GetItem, put = SetItem)) TObject * Items[];
  __declspec(property(get = GetOwnsObjects, put = SetOwnsObjects)) bool OwnsObjects;
  TObject * operator [](int Index) { return GetItem(Index); }

protected:
  virtual void Notify(void * Ptr, TListNotification Action) override;

private:
  bool FOwnsObjects;
};
//---------------------------------------------------------------------------
} // namespace Contnrs
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Contnrs;
//---------------------------------------------------------------------------
#endif
