// The non-visual part of Vcl.Menus used by the core.
//---------------------------------------------------------------------------
#ifndef RtlMenusH
#define RtlMenusH
//---------------------------------------------------------------------------
#include <rtl/System.h>
//---------------------------------------------------------------------------
namespace Vcl
{
namespace Menus
{
// Removes the '&' hotkey markers of a caption ("&&" is a literal '&')
System::UnicodeString StripHotkey(const System::UnicodeString Text);
} // namespace Menus
} // namespace Vcl
//---------------------------------------------------------------------------
using namespace Vcl::Menus;
//---------------------------------------------------------------------------
#endif
