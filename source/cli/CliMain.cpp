// The main function of openscp-cli (the counterpart of upstream's
// winscp.com): initializes the core and runs the console in the mode the
// switches select, as upstream's WinMain.cpp (Execute) does.
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <AppConfiguration.h>
#include <ProgParams.h>
#include <Translations.h>

#include "CliInterface.h"
//---------------------------------------------------------------------------
static TConsoleMode ConsoleMode(TProgramParams * Params)
{
  TConsoleMode Mode;
  if (Params->FindSwitch(L"help") || Params->FindSwitch(L"h") || Params->FindSwitch(L"?"))
  {
    Mode = cmHelp;
  }
  else if (Params->FindSwitch(L"batchsettings"))
  {
    Mode = cmBatchSettings;
  }
  else if (Params->FindSwitch(KEYGEN_SWITCH))
  {
    Mode = cmKeyGen;
  }
  else if (Params->FindSwitch(COPYID_SWITCH))
  {
    Mode = cmCopyId;
  }
  else if (Params->FindSwitch(FINGERPRINTSCAN_SWITCH))
  {
    Mode = cmFingerprintScan;
  }
  else if (Params->FindSwitch(INFO_SWITCH))
  {
    Mode = cmInfo;
  }
  else
  {
    // As winscp.com, which always runs the console
    Mode = cmScripting;
  }
  return Mode;
}
//---------------------------------------------------------------------------
int main(int /*argc*/, char ** /*argv*/)
{
  // Before any thread is started
  InitializeConsoleSignals();

  int Result = RESULT_ANY_ERROR;
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
      // The resource strings in the language of the configuration
      LoadTranslation(AppConfiguration->Language);
      Result = Console(ConsoleMode(Params));
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
