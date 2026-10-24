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
  RTL_PROPERTY_GET(Items, GetItem) RTL_PROPERTY_PUT_INDEXED(Items, SetItem, int, TObject *)
  __declspec(property(get = __property_get_Items, put = __property_set_Items)) TObject * Items[];
  RTL_PROPERTY_GET(OwnsObjects, GetOwnsObjects) RTL_PROPERTY_PUT(OwnsObjects, SetOwnsObjects, bool)
  __declspec(property(get = __property_get_OwnsObjects, put = __property_set_OwnsObjects)) bool OwnsObjects;
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
