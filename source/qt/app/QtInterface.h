// Declarations of the Qt front end shared by its windows (upstream has them
// in WinInterface.h).
//---------------------------------------------------------------------------
#ifndef QtInterfaceH
#define QtInterfaceH
//---------------------------------------------------------------------------
#include <AppPCH.h>
//---------------------------------------------------------------------------
class TTerminal;
//---------------------------------------------------------------------------
void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E);
//---------------------------------------------------------------------------
#endif
