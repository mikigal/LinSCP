// System::TObject and minimal class references (TClass, __classid).
//---------------------------------------------------------------------------
#ifndef RtlObjectH
#define RtlObjectH
//---------------------------------------------------------------------------
#include <rtl/UnicodeString.h>

#include <cstring>
#include <new>
#include <typeinfo>
//---------------------------------------------------------------------------
namespace System
{
class TObject;
//---------------------------------------------------------------------------
// Class reference. Delphi metaclasses carry full RTTI; the core only needs the
// class name and "is this object an instance of the class" checks.
class TMetaClass
{
public:
  typedef bool (*TIsInstanceFunc)(const TObject * Object);

  TMetaClass(const std::type_info & TypeInfo, TIsInstanceFunc IsInstanceFunc) :
    FTypeInfo(TypeInfo),
    FIsInstanceFunc(IsInstanceFunc)
  {
  }

  UnicodeString ClassName() const;
  UnicodeString QualifiedClassName() const;
  const std::type_info & TypeInfo() const { return FTypeInfo; }
  // False for class references obtained from TObject::ClassType()
  bool CanTestInstances() const { return (FIsInstanceFunc != nullptr); }
  bool IsInstance(const TObject * Object) const;

  bool operator ==(const TMetaClass & Rhs) const { return FTypeInfo == Rhs.FTypeInfo; }
  bool operator !=(const TMetaClass & Rhs) const { return !(*this == Rhs); }

private:
  const std::type_info & FTypeInfo;
  TIsInstanceFunc FIsInstanceFunc;
};
typedef TMetaClass * TClass;
//---------------------------------------------------------------------------
class TObject
{
public:
  TObject() {}
  virtual ~TObject() {}

  // Delphi objects start zero-filled (TObject.InitInstance), and the core
  // relies on it: fields that a constructor does not initialize are zero,
  // NULL or false. Delphi-style classes are always allocated on the heap.
  static void * operator new(std::size_t Size)
  {
    void * Result = ::operator new(Size);
    std::memset(Result, 0, Size);
    return Result;
  }
  static void operator delete(void * Pointer) { ::operator delete(Pointer); }

  UnicodeString ClassName() const;
  UnicodeString QualifiedClassName() const;
  TClass ClassType() const;
  bool InheritsFrom(TClass AClass) const;
  virtual UnicodeString ToString();
  virtual bool Equals(TObject * Obj) { return (Obj == this); }
  void Free() { delete this; }
};
//---------------------------------------------------------------------------
namespace rtl_detail
{
UnicodeString ClassNameOf(const std::type_info & TypeInfo, bool Qualified);
TClass ClassOfTypeInfo(const std::type_info & TypeInfo);
}
//---------------------------------------------------------------------------
template <class T>
TClass ClassIdOf()
{
  static TMetaClass Meta(typeid(T), [](const TObject * Object)
    {
      return (dynamic_cast<const T *>(Object) != nullptr);
    });
  return &Meta;
}
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#define __classid(T) (::System::ClassIdOf<T>())
//---------------------------------------------------------------------------
#endif
