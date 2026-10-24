// C++Builder's umbrella header. Only the non-visual RTL part exists here.
#pragma once
#include <rtl/System.h>
#include <rtl/Types.h>
#include <rtl/SysUtils.h>
#include <rtl/Classes.h>
#include <rtl/Variants.h>
#include <rtl/Menus.h>
// As C++Builder's, it includes the Windows API: the platform layer's
// (source/linux), when it is on the include path
#if __has_include(<winapi/Windows.h>)
#include <winapi/Windows.h>
#endif
