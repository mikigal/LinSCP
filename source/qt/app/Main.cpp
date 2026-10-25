// The main function of openscp, the graphical front end: initializes the
// core and runs the application, as upstream's WinMain.cpp does.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <ProgParams.h>

#include <AppConfiguration.h>
#include <AppInterface.h>

#include "OpenScpApplication.h"
#include "ScpCommander.h"
#include "SingleInstance.h"
#include "TerminalManager.h"
#ifdef OPENSCP_AUTOMATION
#include "Automation.h"
#endif

#include <QTimer>

#include <csignal>
//---------------------------------------------------------------------------
static int Execute()
{
  TProgramParams * Params = TProgramParams::Instance();
  // A session (without an operation) goes to an instance that runs
  // already, if any takes it
  bool CommandLineOperation =
    Params->FindSwitch(UPLOAD_SWITCH) || Params->FindSwitch(UPLOAD_IF_ANY_SWITCH);
  bool TrySendToAnotherInstance =
    (Params->ParamCount > 0) &&
    !CommandLineOperation &&
    AppConfiguration->ExternalSessionInExistingInstance &&
    !Params->FindSwitch(NEWINSTANCE_SWICH) &&
    // With /rawconfig before session url, parsing commandline does not work correctly,
    // when opening session in the other instance.
    !Params->FindSwitch(RAW_CONFIG_SWITCH);
  if (TrySendToAnotherInstance && SendToAnotherInstance(QCoreApplication::arguments().mid(1)))
  {
    Configuration->Usage->Inc(L"SendToAnotherInstance");
    return 0;
  }

#ifdef OPENSCP_AUTOMATION
  StartAutomation();
#endif
  TTerminalManager * Manager = TTerminalManager::Instance();
#ifdef OPENSCP_AUTOMATION
  SetAutomationBusyCheck([Manager]()
  {
    return Manager->IsBusy() || ((Manager->ScpExplorer != NULL) && Manager->ScpExplorer->IsBusy());
  });
#endif
  int Result;
  {
    TScpCommanderForm ScpCommander;
    Manager->ScpExplorer = &ScpCommander;
    TInstanceService InstanceService(nullptr);
    InstanceService.OnCommandLine = [&ScpCommander](const QStringList & Arguments, const QString & ActivationToken)
    {
      return ScpCommander.CommandLineFromAnotherInstance(Arguments, ActivationToken);
    };
    InstanceService.Register();
    ScpCommander.show();
    // The session of the command line, else the site manager
    QTimer::singleShot(0, &ScpCommander, [&ScpCommander, Params]() { ScpCommander.ExecuteCommandLine(Params); });
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
