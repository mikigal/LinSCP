// The parts of the core's front end interface (core/Interface.h) specific
// to openscp-cli, which has no GUI.
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include "CliInterface.h"

#include <cstdio>
//---------------------------------------------------------------------------
void __fastcall ShowExtendedException(Exception * E)
{
  // Errors outside of the console runner (e.g. while loading or saving the
  // configuration)
  UnicodeString Message;
  if ((E != NULL) && ExceptionFullMessage(E, Message))
  {
    Message = RemoveEmptyLines(UnformatMessage(Message));
    fprintf(stderr, "%s\n", UTF8String(Message).c_str());
  }
}
//---------------------------------------------------------------------------
void * __fastcall BusyStart()
{
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall BusyEnd(void * /*Token*/)
{
}
//---------------------------------------------------------------------------
void __fastcall SetNoGUI()
{
}
//---------------------------------------------------------------------------
bool __fastcall ProcessGUI(bool /*Force*/)
{
  return false;
}
//---------------------------------------------------------------------------
void SystemRequired()
{
}
//---------------------------------------------------------------------------
void __fastcall CopyToClipboard(UnicodeString /*Text*/)
{
  // No clipboard in a terminal
}
//---------------------------------------------------------------------------
bool __fastcall TextFromClipboard(UnicodeString & /*Text*/, bool /*Trim*/)
{
  return false;
}
//---------------------------------------------------------------------------
