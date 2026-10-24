// C++Builder's COM unit (System.Win.ComObj): the exceptions and OleCheck.
// The COM declarations themselves are in winapi/Com.h.
//---------------------------------------------------------------------------
#ifndef SystemWinComObjHpp
#define SystemWinComObjHpp
//---------------------------------------------------------------------------
#include <winapi/Com.h>
#include <SysUtils.hpp>
//---------------------------------------------------------------------------
namespace System
{
namespace Win
{
namespace Comobj
{
//---------------------------------------------------------------------------
class EOleError : public Exception
{
public:
  explicit EOleError(const UnicodeString & Message) : Exception(Message) {}
};
//---------------------------------------------------------------------------
class EOleSysError : public EOleError
{
public:
  EOleSysError(const UnicodeString & Message, HRESULT ErrorCode, int HelpContext);
  HRESULT __property_get_ErrorCode() const { return FErrorCode; }
  __declspec(property(get = __property_get_ErrorCode)) HRESULT ErrorCode;

private:
  HRESULT FErrorCode;
};
//---------------------------------------------------------------------------
// Throws EOleSysError for a failure code
void OleCheck(HRESULT Result);
//---------------------------------------------------------------------------
} // namespace Comobj
} // namespace Win
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Win::Comobj;
//---------------------------------------------------------------------------
#endif
