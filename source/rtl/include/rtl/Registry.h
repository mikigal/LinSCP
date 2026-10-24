// System.Win.Registry. The registry does not exist on Linux; the classes are
// only declared so that the core's declarations compile (the configuration
// is always stored in an INI file, see docs/port/PLAN.md, section 3.3).
//---------------------------------------------------------------------------
#ifndef RtlRegistryH
#define RtlRegistryH
//---------------------------------------------------------------------------
#include <rtl/Classes.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Win
{
namespace Registry
{
class TRegistry;
class TRegIniFile;
class TRegistryIniFile;
RTL_DECLARE_EXCEPTION(ERegistryException, Exception)
} // namespace Registry
} // namespace Win
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Win::Registry;
//---------------------------------------------------------------------------
#endif
