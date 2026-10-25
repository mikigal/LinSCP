// The sessions and the operations of the command line (upstream's
// Execute, GetLoginData, Upload and Download of windows/WinMain.cpp and
// CommandLineFromAnotherInstance of forms/CustomScpExplorer.cpp): a site,
// a URL or a folder of sites to open, files to upload, a file of the URL
// to download.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <AppInterface.h>
#include <Option.h>
#include <TerminalManager.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QApplication>
#include <QTimer>
//---------------------------------------------------------------------------
static UnicodeString GetFolderOrWorkspaceName(const UnicodeString & SessionName, TOptions * Options, bool PreferSite)
{
  UnicodeString FolderOrWorkspaceName = DecodeUrlChars(SessionName);
  UnicodeString Result;
  if ((Options == nullptr) || !Options->FindSwitch(WORKSPACE_SWITCH))
  {
    PreferSite = true;
  }
  if ((!PreferSite || (StoredSessions->FindByName(FolderOrWorkspaceName) == nullptr)) &&
      StoredSessions->IsFolderOrWorkspace(FolderOrWorkspaceName))
  {
    Result = FolderOrWorkspaceName;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::OpenCommandLineSession(
  const UnicodeString & SessionName, TOptions * Options, bool NeedTerminal, UnicodeString & DownloadFile)
{
  std::unique_ptr<TObjectList> DataList(new TObjectList());
  bool DefaultsOnly = false;

  UnicodeString FolderOrWorkspaceName = GetFolderOrWorkspaceName(SessionName, Options, NeedTerminal);
  if (!FolderOrWorkspaceName.IsEmpty())
  {
    StoredSessions->GetFolderOrWorkspace(FolderOrWorkspaceName, DataList.get());
  }
  else
  {
    int ParsedInfo;
    int Flags = pufAllowStoredSiteWithProtocol | FLAGMASK(!CheckSafe(Options), pufUnsafe);
    TSessionData * SessionData = StoredSessions->ParseUrl(SessionName, Options, ParsedInfo, &DownloadFile, NULL, Flags);
    DefaultsOnly = FLAGSET(ParsedInfo, piDefaultsOnly);
    DataList->Add(SessionData);

    if (SessionData->SaveOnly)
    {
      Configuration->Usage->Inc(L"CommandLineSessionSave");
      TSessionData * SavedSession = DoSaveSession(SessionData, NULL, true, NULL);
      if (SavedSession == NULL)
      {
        return false;
      }
      AppConfiguration->LastStoredSession = SavedSession->Name;
      DataList->Clear();
    }
  }

  // The site manager when the session is not complete
  if ((DataList->Count == 0) ||
      !static_cast<TSessionData *>(DataList->Items[0])->CanOpen ||
      DefaultsOnly)
  {
    if (!DoLoginDialog(DataList.get(), this, NeedTerminal))
    {
      return false;
    }
  }

  TTerminalManager * Manager = TTerminalManager::Instance();
  TManagedTerminal * Session = Manager->NewSessions(DataList.get());
  Manager->ActiveSession = Session;
  bool Result = Manager->IsAvailableTerminal(Session) && (Manager->ActiveSession == Session);
  if (Result)
  {
    FRemotePanel->GetDirView()->setFocus();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteCommandLine(TOptions * Params)
{
  enum { pcNone, pcUpload } ParamCommand = pcNone;
  std::unique_ptr<TStrings> CommandParams(new TStringList());
  int UseDefaults = -1;
  UnicodeString Value;
  if (Params->FindSwitch(DEFAULTS_SWITCH, Value) && CheckSafe(Params))
  {
    UseDefaults = StrToIntDef(Value, 0);
  }
  if (Params->FindSwitch(UPLOAD_SWITCH, CommandParams.get()))
  {
    ParamCommand = pcUpload;
    if (CommandParams->Count == 0)
    {
      throw Exception(LoadStr(NO_UPLOAD_LIST_ERROR));
    }
  }
  if (Params->FindSwitch(UPLOAD_IF_ANY_SWITCH, CommandParams.get()) && (CommandParams->Count > 0))
  {
    ParamCommand = pcUpload;
  }

  UnicodeString SessionName = (Params->ParamCount > 0) ? Params->Param[1] : UnicodeString();
  bool CommandLineOperation = (ParamCommand != pcNone);
  if (SessionName.IsEmpty() && !CommandLineOperation)
  {
    // The site manager on startup
    NeedSession(true);
  }
  else
  {
    Configuration->Usage->Inc(CommandLineOperation ? L"CommandLineOperation" : L"CommandLineSession2");
    UnicodeString DownloadFile;
    if (OpenCommandLineSession(SessionName, Params, CommandLineOperation, DownloadFile))
    {
      if (ParamCommand == pcUpload)
      {
        CommandLineUpload(CommandParams.get(), UseDefaults);
      }
      else if (!DownloadFile.IsEmpty())
      {
        CommandLineDownload(DownloadFile, UseDefaults);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CommandLineUpload(TStrings * FileList, int UseDefaults)
{
  TTerminal * Terminal = GetTerminal();
  UnicodeString TargetDirectory = UnixIncludeTrailingBackslash(Terminal->CurrentDirectory);
  TGUICopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
  int Options = coDisableQueue;
  int CopyParamAttrs = Terminal->UsableCopyParamAttrs(0).Upload;
  if ((UseDefaults == 0) ||
      DoCopyDialog(true, false, FileList, TargetDirectory, &CopyParam, Options, CopyParamAttrs, NULL))
  {
    // Setting parameter overrides only now, otherwise the dialog would present the parametes as non-default
    // (the session is closed once done)
    CopyParam.OnceDoneOperation = odoDisconnect;
    CopyParam.IncludeFileMask.SetRoots(FileList, TargetDirectory);

    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
    Terminal->CopyToRemote(FileList, TargetDirectory, &CopyParam, 0, NULL);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CommandLineDownload(const UnicodeString & FileName, int UseDefaults)
{
  TTerminal * Terminal = GetTerminal();
  UnicodeString FullFileName = UnixIncludeTrailingBackslash(Terminal->CurrentDirectory) + FileName;
  TRemoteFile * File;
  {
    Terminal->ExceptionOnFail = true;
    ::rtl::TFinally Finally([&]() { Terminal->ExceptionOnFail = false; });
    File = Terminal->ReadFile(FullFileName);
  }
  std::unique_ptr<TStringList> FileList(new TStringList());
  FileList->OwnsObjects = true;
  File->FullFileName = FullFileName;
  FileList->AddObject(FullFileName, File);

  UnicodeString LocalDirectory = Terminal->SessionData->LocalDirectoryExpanded;
  if (LocalDirectory.IsEmpty())
  {
    LocalDirectory = FLocalPanel->GetPath();
  }
  UnicodeString TargetDirectory = IncludeTrailingBackslash(LocalDirectory);
  TGUICopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;
  int Options = coDisableQueue;
  int CopyParamAttrs = Terminal->UsableCopyParamAttrs(0).Download;
  if ((UseDefaults == 0) ||
      DoCopyDialog(false, false, FileList.get(), TargetDirectory, &CopyParam, Options, CopyParamAttrs, NULL))
  {
    // The session is closed once done
    CopyParam.OnceDoneOperation = odoDisconnect;
    CopyParam.IncludeFileMask.SetRoots(TargetDirectory, FileList.get());

    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]()
    {
      DestroyProgressForm();
      FLocalPanel->Reload();
    });
    Terminal->CopyToLocal(FileList.get(), TargetDirectory, &CopyParam, 0, NULL);
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanCommandLineFromAnotherInstance()
{
  // The session opens in a new tab; not while a modal window (the site
  // manager) shows
  return
    isVisible() && !IsBusy() &&
    (QApplication::activeModalWidget() == nullptr);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CommandLineFromAnotherInstance(const QStringList & Arguments, const QString & ActivationToken)
{
  bool Result = CanCommandLineFromAnotherInstance() && !Arguments.isEmpty();
  if (Result)
  {
    AppLog(L"Processing command-line from another instance");
    // The other instance waits for the reply
    QTimer::singleShot(0, this, [this, Arguments, ActivationToken]()
    {
      // This action is initiated from another process, so it's likely that
      // our window is not visible, and user won't see what is going on
      // (the token of the other instance's launcher allows activating it)
      ActivateFromOtherApplication(ActivationToken);
      if (CanCommandLineFromAnotherInstance())
      {
        TOptions Params;
        for (const QString & Argument : Arguments)
        {
          Params.Add(ToUnicodeString(Argument));
        }
        // Reload sessions as we may be asked to open a session just stored
        // by another instance
        StoredSessions->Reload();
        UnicodeString DownloadFile;
        if (Params.ParamCount > 0)
        {
          OpenCommandLineSession(Params.Param[1], &Params, false, DownloadFile);
        }
      }
    });
  }
  return Result;
}
//---------------------------------------------------------------------------
