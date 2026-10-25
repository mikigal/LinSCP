// The desktop shortcuts of sites and workspaces (upstream's
// CreateDesktopSessionShortCut of windows/Tools.cpp): desktop entry files
// in the desktop directory that open them.
//---------------------------------------------------------------------------
#ifndef DesktopShortcutH
#define DesktopShortcutH
//---------------------------------------------------------------------------
#include <QtInterface.h>
//---------------------------------------------------------------------------
// The shortcut opens the site, the site folder or the workspace of the name
void CreateDesktopSessionShortCut(const UnicodeString & SessionName);
//---------------------------------------------------------------------------
#endif
