//---------------------------------------------------------------------------
#ifndef NamedObjsH
#define NamedObjsH

#include <System.hpp>
#include <Contnrs.hpp>
//---------------------------------------------------------------------------
class TNamedObjectList;
class TNamedObject : public TPersistent
{
public:
  UnicodeString & __property_get_Name() const { return ::rtl::Mutable(this)->FName; } void __property_set_Name(UnicodeString Value) { SetName(Value); } __declspec(property(get = __property_get_Name, put = __property_set_Name)) UnicodeString Name;
  bool & __property_get_Hidden() const { return ::rtl::Mutable(this)->FHidden; } __declspec(property(get = __property_get_Hidden)) bool Hidden;
  __fastcall TNamedObject() {}
  bool __fastcall IsSameName(const UnicodeString & Name);
  virtual int __fastcall Compare(TNamedObject * Other);
  __fastcall TNamedObject(UnicodeString aName);
  void __fastcall MakeUniqueIn(TNamedObjectList * List);
private:
  UnicodeString FName;
  bool FHidden;

  void __fastcall SetName(UnicodeString value);
};
//---------------------------------------------------------------------------
class TNamedObjectList : public TObjectList
{
private:
  int __fastcall GetCount();
  int __fastcall GetCountIncludingHidden();
  virtual void __fastcall Notify(void *Ptr, TListNotification Action);
protected:
  int FHiddenCount;
  bool FControlledAdd;
  void __fastcall Recount();
public:
  static const UnicodeString HiddenPrefix;

  bool AutoSort;

  __fastcall TNamedObjectList();
  void __fastcall AlphaSort();
  int __fastcall Add(TObject * AObject);
  virtual TNamedObject * __fastcall AtObject(Integer Index);
  TNamedObject * __fastcall FindByName(const UnicodeString & Name);
  int __property_get_Count() const { return ::rtl::Mutable(this)->GetCount(); } __declspec(property(get = __property_get_Count)) int Count;
  int __property_get_CountIncludingHidden() const { return ::rtl::Mutable(this)->GetCountIncludingHidden(); } __declspec(property(get = __property_get_CountIncludingHidden)) int CountIncludingHidden;
};
//---------------------------------------------------------------------------
int __fastcall NamedObjectSortProc(void * Item1, void * Item2);
//---------------------------------------------------------------------------
#endif
