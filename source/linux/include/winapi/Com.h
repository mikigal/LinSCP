// The COM declarations the core uses: the shell file operation interfaces
// (IFileOperation, IShellItem) and C++Builder's TCppInterfacedObject.
// IUnknown is the RTL's IInterface (rtl/Interface.h).
//
// There is no COM runtime: CoCreateInstance knows no classes yet
// (REGDB_E_CLASSNOTREG), so the core's local-to-local synchronization through
// IFileOperation is not available.
//---------------------------------------------------------------------------
#ifndef WinApiComH
#define WinApiComH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>
#include <rtl/Interface.h>

#include <atomic>
#include <type_traits>
//---------------------------------------------------------------------------
#define STDMETHODCALLTYPE
#define STDAPI HRESULT
#define E_NOINTERFACE (static_cast<HRESULT>(0x80004002))
#define E_POINTER (static_cast<HRESULT>(0x80004003))
#define E_OUTOFMEMORY (static_cast<HRESULT>(0x8007000E))
#define E_INVALIDARG (static_cast<HRESULT>(0x80070057))
#define REGDB_E_CLASSNOTREG (static_cast<HRESULT>(0x80040154))
#define CLSCTX_INPROC_SERVER 0x1
#define CLSCTX_LOCAL_SERVER 0x4
#define CLSCTX_ALL 0x17
#define COINIT_APARTMENTTHREADED 0x2
#define COINIT_MULTITHREADED 0x0
//---------------------------------------------------------------------------
using System::IUnknown;
//---------------------------------------------------------------------------
// Interfaces carry their identifier as the static member Iid
#define IID_PPV_ARGS(Pointer) \
  std::remove_reference_t<decltype(**(Pointer))>::Iid, reinterpret_cast<void **>(Pointer)
//---------------------------------------------------------------------------
WINAPI_EXTERN_C_BEGIN
HRESULT CoInitialize(LPVOID Reserved);
HRESULT CoInitializeEx(LPVOID Reserved, DWORD CoInit);
void CoUninitialize();
LPVOID CoTaskMemAlloc(SIZE_T Size);
void CoTaskMemFree(LPVOID Memory);
HRESULT CoCreateInstance(REFCLSID Clsid, IUnknown * Outer, DWORD ClsContext, REFIID Riid, LPVOID * Object);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
enum SIGDN
{
  SIGDN_NORMALDISPLAY = 0,
  SIGDN_PARENTRELATIVEPARSING = static_cast<int>(0x80018001),
  SIGDN_DESKTOPABSOLUTEPARSING = static_cast<int>(0x80028000),
  SIGDN_FILESYSPATH = static_cast<int>(0x80058000),
};
//---------------------------------------------------------------------------
struct IBindCtx;
//---------------------------------------------------------------------------
struct IShellItem : public IUnknown
{
  static constexpr IID Iid = { 0x43826d1e, 0xe718, 0x42ee, { 0xbc, 0x55, 0xa1, 0xe2, 0x61, 0xc3, 0x7b, 0xfe } };
  // The name is allocated with CoTaskMemAlloc
  virtual HRESULT STDMETHODCALLTYPE GetDisplayName(SIGDN Name, LPWSTR * DisplayName) = 0;
};
// A file system item
WINAPI_EXTERN_C_BEGIN
HRESULT SHCreateItemFromParsingName(LPCWSTR Path, IBindCtx * BindContext, REFIID Riid, void ** Object);
WINAPI_EXTERN_C_END
//---------------------------------------------------------------------------
struct IFileOperationProgressSink : public IUnknown
{
  static constexpr IID Iid = { 0x04b0f1a7, 0x9490, 0x44bc, { 0x96, 0xe1, 0x42, 0x23, 0xa1, 0x9a, 0x6c, 0x3e } };
  virtual HRESULT STDMETHODCALLTYPE StartOperations() = 0;
  virtual HRESULT STDMETHODCALLTYPE FinishOperations(HRESULT Result) = 0;
  virtual HRESULT STDMETHODCALLTYPE PreRenameItem(DWORD Flags, IShellItem * Item, LPCWSTR NewName) = 0;
  virtual HRESULT STDMETHODCALLTYPE PostRenameItem(
    DWORD Flags, IShellItem * Item, LPCWSTR NewName, HRESULT Rename, IShellItem * NewlyCreated) = 0;
  virtual HRESULT STDMETHODCALLTYPE PreMoveItem(
    DWORD Flags, IShellItem * Item, IShellItem * DestinationFolder, LPCWSTR NewName) = 0;
  virtual HRESULT STDMETHODCALLTYPE PostMoveItem(
    DWORD Flags, IShellItem * Item, IShellItem * DestinationFolder, LPCWSTR NewName, HRESULT Move,
    IShellItem * NewlyCreated) = 0;
  virtual HRESULT STDMETHODCALLTYPE PreCopyItem(
    DWORD Flags, IShellItem * Item, IShellItem * DestinationFolder, LPCWSTR NewName) = 0;
  virtual HRESULT STDMETHODCALLTYPE PostCopyItem(
    DWORD Flags, IShellItem * Item, IShellItem * DestinationFolder, LPCWSTR NewName, HRESULT Copy,
    IShellItem * NewlyCreated) = 0;
  virtual HRESULT STDMETHODCALLTYPE PreDeleteItem(DWORD Flags, IShellItem * Item) = 0;
  virtual HRESULT STDMETHODCALLTYPE PostDeleteItem(
    DWORD Flags, IShellItem * Item, HRESULT Delete, IShellItem * NewlyCreated) = 0;
  virtual HRESULT STDMETHODCALLTYPE PreNewItem(DWORD Flags, IShellItem * DestinationFolder, LPCWSTR NewName) = 0;
  virtual HRESULT STDMETHODCALLTYPE PostNewItem(
    DWORD Flags, IShellItem * DestinationFolder, LPCWSTR NewName, LPCWSTR TemplateName, DWORD FileAttributes,
    HRESULT New, IShellItem * NewItem) = 0;
  virtual HRESULT STDMETHODCALLTYPE UpdateProgress(UINT WorkTotal, UINT WorkSoFar) = 0;
  virtual HRESULT STDMETHODCALLTYPE ResetTimer() = 0;
  virtual HRESULT STDMETHODCALLTYPE PauseTimer() = 0;
  virtual HRESULT STDMETHODCALLTYPE ResumeTimer() = 0;
};
//---------------------------------------------------------------------------
struct IFileOperation : public IUnknown
{
  static constexpr IID Iid = { 0x947aab5f, 0x0a5c, 0x4c13, { 0xb4, 0xd6, 0x4b, 0xf7, 0x83, 0x6f, 0xc9, 0xf8 } };
  virtual HRESULT STDMETHODCALLTYPE Advise(IFileOperationProgressSink * Sink, DWORD * Cookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE Unadvise(DWORD Cookie) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetOperationFlags(DWORD OperationFlags) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetOwnerWindow(HWND Owner) = 0;
  virtual HRESULT STDMETHODCALLTYPE RenameItem(IShellItem * Item, LPCWSTR NewName, IFileOperationProgressSink * Sink) = 0;
  virtual HRESULT STDMETHODCALLTYPE MoveItem(
    IShellItem * Item, IShellItem * DestinationFolder, LPCWSTR NewName, IFileOperationProgressSink * Sink) = 0;
  virtual HRESULT STDMETHODCALLTYPE CopyItem(
    IShellItem * Item, IShellItem * DestinationFolder, LPCWSTR CopyName, IFileOperationProgressSink * Sink) = 0;
  virtual HRESULT STDMETHODCALLTYPE DeleteItem(IShellItem * Item, IFileOperationProgressSink * Sink) = 0;
  virtual HRESULT STDMETHODCALLTYPE PerformOperations() = 0;
  virtual HRESULT STDMETHODCALLTYPE GetAnyOperationsAborted(BOOL * AnyOperationsAborted) = 0;
};
constexpr CLSID CLSID_FileOperation = { 0x3ad05575, 0x8857, 0x4850, { 0x92, 0x77, 0x11, 0xb8, 0x5b, 0xdb, 0x8e, 0x09 } };
//---------------------------------------------------------------------------
// C++Builder's implementation of IUnknown for C++ classes. The object is
// created with one reference, owned by its creator, who releases it.
template <class T>
class TCppInterfacedObject : public T
{
public:
  virtual ~TCppInterfacedObject() {}

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID Riid, void ** Object) override
  {
    if ((Riid == T::Iid) || (Riid == IUnknown::Iid))
    {
      *Object = static_cast<T *>(this);
      AddRef();
      return S_OK;
    }
    *Object = nullptr;
    return E_NOINTERFACE;
  }

  ULONG STDMETHODCALLTYPE AddRef() override
  {
    return ++FRefCount;
  }

  ULONG STDMETHODCALLTYPE Release() override
  {
    ULONG Result = --FRefCount;
    if (Result == 0)
    {
      delete this;
    }
    return Result;
  }

private:
  std::atomic<ULONG> FRefCount{1};
};
//---------------------------------------------------------------------------
#endif
