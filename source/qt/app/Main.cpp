// The main function of openscp, the graphical front end: initializes the
// core and runs the application, as upstream's WinMain.cpp does.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <ProgParams.h>

#include "OpenScpApplication.h"
#include "ScpCommander.h"
#include "TerminalManager.h"
#ifdef OPENSCP_AUTOMATION
#include "Automation.h"
#endif

#include <QTimer>

#include <csignal>
//---------------------------------------------------------------------------
static int Execute()
{
#ifdef OPENSCP_AUTOMATION
  StartAutomation();
#endif
  TTerminalManager * Manager = TTerminalManager::Instance();
#ifdef OPENSCP_AUTOMATION
  SetAutomationBusyCheck([Manager]() { return Manager->IsBusy(); });
#endif
  int Result;
  {
    TScpCommanderForm ScpCommander;
    Manager->ScpExplorer = &ScpCommander;
    ScpCommander.show();
    // The site manager on startup
    QTimer::singleShot(0, &ScpCommander, [&ScpCommander]() { ScpCommander.NewSession(); });
    Result = QApplication::exec();
    Manager->ScpExplorer = NULL;
  }
  TTerminalManager::DestroyInstance();
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
