// The COM functions of the Windows API, see winapi/Com.h.
//---------------------------------------------------------------------------
#include <winapi/Com.h>

#include <System.hpp>
#include <SysUtils.hpp>
#include <System.Win.ComObj.hpp>

#include <cstdlib>
#include <cwchar>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class TShellItem : public TCppInterfacedObject<IShellItem>
{
public:
  explicit TShellItem(const UnicodeString & Path) : FPath(Path) {}

  HRESULT STDMETHODCALLTYPE GetDisplayName(SIGDN Name, LPWSTR * DisplayName) override
  {
    UnicodeString Result = (Name == SIGDN_NORMALDISPLAY) ? ExtractFileName(FPath) : FPath;
    *DisplayName = static_cast<LPWSTR>(CoTaskMemAlloc((Result.Length() + 1) * sizeof(WCHAR)));
    if (*DisplayName == nullptr)
    {
      return E_OUTOFMEMORY;
    }
    wmemcpy(*DisplayName, Result.c_str(), Result.Length() + 1);
    return S_OK;
  }

private:
  UnicodeString FPath;
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
HRESULT CoInitialize(LPVOID)
{
  return S_OK;
}
//---------------------------------------------------------------------------
HRESULT CoInitializeEx(LPVOID, DWORD)
{
  return S_OK;
}
//---------------------------------------------------------------------------
void CoUninitialize()
{
}
//---------------------------------------------------------------------------
LPVOID CoTaskMemAlloc(SIZE_T Size)
{
  return malloc(Size);
}
//---------------------------------------------------------------------------
void CoTaskMemFree(LPVOID Memory)
{
  free(Memory);
}
//---------------------------------------------------------------------------
HRESULT CoCreateInstance(REFCLSID, IUnknown *, DWORD, REFIID, LPVOID * Object)
{
  *Object = nullptr;
  return REGDB_E_CLASSNOTREG;
}
//---------------------------------------------------------------------------
HRESULT SHCreateItemFromParsingName(LPCWSTR Path, IBindCtx *, REFIID Riid, void ** Object)
{
  TShellItem * Item = new TShellItem(Path);
  HRESULT Result = Item->QueryInterface(Riid, Object);
  // The creation reference
  Item->Release();
  return Result;
}
//---------------------------------------------------------------------------
namespace System
{
namespace Win
{
namespace Comobj
{
//---------------------------------------------------------------------------
EOleSysError::EOleSysError(const UnicodeString & Message, HRESULT ErrorCode, int HelpContext) :
  EOleError(Message),
  FErrorCode(ErrorCode)
{
  this->HelpContext = HelpContext;
}
//---------------------------------------------------------------------------
void OleCheck(HRESULT Result)
{
  if (FAILED(Result))
  {
    throw EOleSysError(L"OLE error " + IntToHex(static_cast<int>(Result), 8), Result, 0);
  }
}
//---------------------------------------------------------------------------
} // namespace Comobj
} // namespace Win
} // namespace System
//---------------------------------------------------------------------------
