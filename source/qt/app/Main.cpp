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

#include <QDesktopServices>
#include <QTimer>

#include <csignal>
//---------------------------------------------------------------------------
// The temporary directories left by previous runs (upstream's
// TemporaryDirectoryCleanup of windows/Setup.cpp)
static void TemporaryDirectoryCleanup()
{
  std::unique_ptr<TStrings> Folders(AppConfiguration->FindTemporaryFolders());
  if (Folders.get() != NULL)
  {
    bool Continue = true;
    if (AppConfiguration->ConfirmTemporaryDirectoryCleanup)
    {
      Configuration->Usage->Inc(L"TemporaryDirectoryCleanupConfirmations");

      TQueryButtonAlias Aliases[1];
      Aliases[0].Button = qaRetry;
      Aliases[0].Alias = LoadStr(OPEN_BUTTON);
      TMessageParams Params(mpNeverAskAgainCheck);
      Params.Aliases = Aliases;
      Params.AliasesCount = std::size(Aliases);

      unsigned int Answer = MoreMessageDialog(
        FMTLOAD(CLEANTEMP_CONFIRM2, (Folders->Count)), Folders.get(),
        qtConfirmation, qaYes | qaNo | qaRetry, &Params);

      if (Answer == qaNeverAskAgain)
      {
        AppConfiguration->ConfirmTemporaryDirectoryCleanup = false;
        Answer = qaYes;
      }
      else if (Answer == qaRetry)
      {
        for (int Index = 0; Index < Folders->Count; Index++)
        {
          QDesktopServices::openUrl(LocalPathToUrl(Folders->Strings[Index]));
        }
      }
      Continue = (Answer == qaYes);
    }

    if (Continue)
    {
      try
      {
        AppConfiguration->CleanupTemporaryFolders(Folders.get());
      }
      catch (Exception & E)
      {
        ShowExtendedException(&E);
      }
    }
  }
}
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
    // Not the directories of another instance
    if (InstanceService.IsOnlyInstance() && AppConfiguration->TemporaryDirectoryCleanup)
    {
      TemporaryDirectoryCleanup();
    }
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
