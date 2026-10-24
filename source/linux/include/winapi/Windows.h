// The Windows API emulated on Linux, for the upstream code (core) that calls
// it; see docs/port/PLAN.md section 3.3. Types, constants and error codes
// come from the RTL shim (rtl/WinTypes.h, rtl/WinError.h).
//---------------------------------------------------------------------------
#ifndef WinApiWindowsH
#define WinApiWindowsH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>
#include <winapi/FileApi.h>
#include <winapi/TimeApi.h>
#include <winapi/System.h>
#include <winapi/Shell.h>
#include <winapi/Process.h>
#include <winapi/Com.h>
#include <winapi/Crypt.h>
#include <winapi/Registry.h>
//---------------------------------------------------------------------------
#endif
