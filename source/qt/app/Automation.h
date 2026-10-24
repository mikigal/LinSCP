// The automation hook of test builds (CMake option OPENSCP_AUTOMATION,
// docs/port/PLAN.md section 6.4): the end-to-end tests drive the GUI
// through a Unix socket with a JSON API (tools/testenv/openscp_gui.py).
//
// The server listens on the path in OPENSCP_AUTOMATION_SOCKET. Each request
// is a line with a JSON object {"id": N, "cmd": NAME, ...}; each reply is a
// line {"id": N, "ok": true, "result": ...} or {"id": N, "ok": false,
// "error": MESSAGE}. Widgets and actions are addressed by object names
// (the component names of upstream's .dfm files): "Window/Child", where
// Window is a top-level widget and Child any descendant of it.
//
// Commands that act (click, trigger, set, activate, ...) reply before they
// act: an action may open a modal window, whose event loop then processes
// the next requests, as it would process the user's input.
//---------------------------------------------------------------------------
#ifndef AutomationH
#define AutomationH
//---------------------------------------------------------------------------
#include <functional>
//---------------------------------------------------------------------------
// Starts the server if OPENSCP_AUTOMATION_SOCKET is set
void StartAutomation();
void StopAutomation();
// "wait-idle" replies once the application is not busy, as this function
// tells (a terminal operation in progress)
void SetAutomationBusyCheck(std::function<bool()> BusyCheck);
//---------------------------------------------------------------------------
#endif
