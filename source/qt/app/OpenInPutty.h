// Opens the session in an SSH client (upstream's OpenSessionInPutty of
// windows/GUITools.cpp): the command of PuttyPath, PuTTY or OpenSSH's ssh in
// a terminal emulator. With the patterns of a site (!@, !U, !#, ...) the
// command is the user's; otherwise the parameters of the session are added
// in the syntax of the client, the last word of the command (ssh or PuTTY,
// whose sessions are not stored: as upstream's command line of the UWP
// version of PuTTY).
//---------------------------------------------------------------------------
#ifndef OpenInPuttyH
#define OpenInPuttyH
//---------------------------------------------------------------------------
#include <QtInterface.h>
//---------------------------------------------------------------------------
void OpenSessionInPutty(TSessionData * SessionData);
//---------------------------------------------------------------------------
#endif
