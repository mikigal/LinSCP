// The main function of openscp, the graphical front end: initializes the
// core and runs the application, as upstream's WinMain.cpp does.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <ProgParams.h>

#include "OpenScpApplication.h"
#include "ScpCommander.h"
#ifdef OPENSCP_AUTOMATION
#include "Automation.h"
#endif

#include <csignal>
//---------------------------------------------------------------------------
static int Execute()
{
#ifdef OPENSCP_AUTOMATION
  StartAutomation();
#endif
  int Result;
  {
    TScpCommanderForm ScpCommander;
    ScpCommander.show();
    Result = QApplication::exec();
  }
#ifdef OPENSCP_AUTOMATION
  StopAutomation();
#endif
  return Result;
}
//---------------------------------------------------------------------------
int main(int argc, char ** argv)
{
  // Writes to closed sockets fail with EPIPE instead of ending the process
  signal(SIGPIPE, SIG_IGN);

  TOpenScpApplication Application(argc, argv);

  int Result = 1;
  try
  {
    TProgramParams * Params = TProgramParams::Instance();
    ApplicationLog = new TApplicationLog();
    UnicodeString AppLogPath;
    if (Params->FindSwitch(L"applog", AppLogPath))
    {
      ApplicationLog->Enable(AppLogPath);
    }
    AppLog(L"Starting...");

    CoreInitialize();
    try
    {
      Configuration->UpdateStaticUsage();
      ApplyCommandLineConfiguration(Params);
      Result = Execute();
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
    }
    CoreFinalize();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }

  delete ApplicationLog;
  ApplicationLog = NULL;
  return Result;
}
//---------------------------------------------------------------------------
