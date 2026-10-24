// Reference-counted interfaces: IInterface (COM's IUnknown), TInterfacedObject,
// DelphiInterface (C++Builder's _di_ types) and interface_cast.
//---------------------------------------------------------------------------
#ifndef RtlInterfaceH
#define RtlInterfaceH
//---------------------------------------------------------------------------
#include <rtl/Object.h>
#include <rtl/WinTypes.h>

#include <atomic>
#include <cstring>
#include <utility>
//---------------------------------------------------------------------------
inline bool operator ==(const GUID & Guid1, const GUID & Guid2)
{
  return
    (Guid1.Data1 == Guid2.Data1) && (Guid1.Data2 == Guid2.Data2) && (Guid1.Data3 == Guid2.Data3) &&
    (std::memcmp(Guid1.Data4, Guid2.Data4, sizeof(Guid1.Data4)) == 0);
}
inline bool operator !=(const GUID & Guid1, const GUID & Guid2)
{
  return !(Guid1 == Guid2);
}
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
// Delphi's IInterface is COM's IUnknown. Interfaces carry their identifier
// as the static member Iid (see IID_PPV_ARGS in winapi/Com.h).
class IInterface
{
public:
  static constexpr GUID Iid = { 0x00000000, 0x0000, 0x0000, { 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };
  virtual HRESULT QueryInterface(REFIID Riid, void ** Object) = 0;
  virtual ULONG AddRef() = 0;
  virtual ULONG Release() = 0;

protected:
  virtual ~IInterface() {}
};
typedef IInterface IUnknown;
//---------------------------------------------------------------------------
// QueryInterface of an object that implements IUnknown only
inline HRESULT QueryUnknown(IInterface * Self, REFIID Riid, void ** Object)
{
  if (Riid == IInterface::Iid)
  {
    *Object = Self;
    Self->AddRef();
    return S_OK;
  }
  *Object = nullptr;
  return static_cast<HRESULT>(0x80004002); // E_NOINTERFACE
}
//---------------------------------------------------------------------------
// Interfaces derive virtually from IInterface, so that an object implementing
// several of them has one reference count. QueryInterface knows IUnknown only,
// interface_cast casts with dynamic_cast.
class TInterfacedObject : public TObject, public virtual IInterface
{
public:
  virtual HRESULT QueryInterface(REFIID Riid, void ** Object) override { return QueryUnknown(this, Riid, Object); }
  virtual ULONG AddRef() override { return ++FRefCount; }
  virtual ULONG Release() override
  {
    ULONG Result = --FRefCount;
    if (Result == 0)
    {
      delete this;
    }
    return Result;
  }
  int GetRefCount() const { return FRefCount; }
  RTL_PROPERTY_GET(RefCount, GetRefCount)
  __declspec(property(get = __property_get_RefCount)) int RefCount;

private:
  std::atomic<ULONG> FRefCount{0};
};
//---------------------------------------------------------------------------
// Smart pointer to an interface (C++Builder's _di_ types)
template <class T>
class DelphiInterface
{
public:
  DelphiInterface() noexcept = default;
  DelphiInterface(T * Intf) : FIntf(Intf) { if (FIntf != nullptr) FIntf->AddRef(); }
  DelphiInterface(const DelphiInterface & Src) : DelphiInterface(Src.FIntf) {}
  DelphiInterface(DelphiInterface && Src) noexcept : FIntf(Src.FIntf) { Src.FIntf = nullptr; }
  ~DelphiInterface() { Reset(); }

  DelphiInterface & operator =(const DelphiInterface & Rhs) { DelphiInterface(Rhs).Swap(*this); return *this; }
  DelphiInterface & operator =(DelphiInterface && Rhs) noexcept { DelphiInterface(std::move(Rhs)).Swap(*this); return *this; }
  DelphiInterface & operator =(T * Rhs) { DelphiInterface(Rhs).Swap(*this); return *this; }

  T * operator ->() const { return FIntf; }
  operator T * () const { return FIntf; }
  T & operator *() const { return *FIntf; }
  T * GetInterface() const { return FIntf; }
  bool operator !() const { return (FIntf == nullptr); }

private:
  void Reset()
  {
    if (FIntf != nullptr)
    {
      T * Intf = FIntf;
      FIntf = nullptr;
      Intf->Release();
    }
  }
  void Swap(DelphiInterface & Other) noexcept { std::swap(FIntf, Other.FIntf); }

  T * FIntf = nullptr;
};
//---------------------------------------------------------------------------
template <class I, class T>
DelphiInterface<I> interface_cast(T * Object)
{
  return DelphiInterface<I>(dynamic_cast<I *>(Object));
}
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
#endif
