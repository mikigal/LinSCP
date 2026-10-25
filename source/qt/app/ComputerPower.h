// Suspending and shutting down the computer once the operations are done
// (upstream's SuspendWindows and ShutDownWindows of windows/Tools.cpp),
// through systemd-logind (org.freedesktop.login1) on the system bus, which
// asks for an authorization when the user needs one.
//---------------------------------------------------------------------------
#ifndef ComputerPowerH
#define ComputerPowerH
//---------------------------------------------------------------------------
#include <QtTools.h>
//---------------------------------------------------------------------------
// The computer can be suspended or shut down (possibly after an
// authorization)
bool CanSuspendComputer();
bool CanShutDownComputer();
// Throw when refused
void SuspendComputer();
void ShutDownComputer();
//---------------------------------------------------------------------------
#endif
