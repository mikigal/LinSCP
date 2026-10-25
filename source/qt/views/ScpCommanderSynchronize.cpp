// The synchronization of directories of the main window (upstream has it in
// forms/CustomScpExplorer.cpp and forms/ScpCommander.cpp): the comparison,
// the checklist and the synchronization, directly or in the queue; the
// synchronized browsing.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <TerminalManager.h>
#include <TextsWin.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ProgressForm.h"
#include "ScpCommander.h"
#include "SynchronizeProgressForm.h"

#include <QTreeView>
//---------------------------------------------------------------------------
void TScpCommanderForm::FullSynchronizeActionExecute()
{
  if (IsBusy() || !FRemotePanel->IsReady())
  {
    return;
  }
  UnicodeString Directory1 = LocalDirectory();
  UnicodeString Directory2 = FRemotePanel->GetPath();
  bool SaveMode = !(GUIConfiguration->SynchronizeModeAuto < 0);
  TSynchronizeMode Mode =
    (SaveMode ? static_cast<TSynchronizeMode>(GUIConfiguration->SynchronizeModeAuto) :
      ((FCurrentPanel == FLocalPanel) ? smRemote : smLocal));
  int Params = GUIConfiguration->SynchronizeParams;
  if (DoFullSynchronizeDirectories(Directory1, Directory2, Mode, Params, SaveMode) >= 0)
  {
    if (SaveMode)
    {
      GUIConfiguration->SynchronizeModeAuto = Mode;
    }
    if (IsExplorer())
    {
      AppConfiguration->ExplorerLastLocalTargetDirectory = Directory1;
    }
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::SynchronizeAllowSelectedOnly()
{
  return
    !FRemotePanel->GetDirView()->selectionModel()->selectedRows().isEmpty() ||
    !FLocalPanel->GetDirView()->selectionModel()->selectedRows().isEmpty();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::GetSynchronizeOptions(int Params, TSynchronizeOptions & Options)
{
  if (FLAGSET(Params, TTerminal::spSelectedOnly) && SynchronizeAllowSelectedOnly())
  {
    Options.Filter = new TStringList();
    Options.Filter->CaseSensitive = false;
    Options.Filter->Duplicates = Types::dupAccept;

    for (TDirPanel * Panel : { static_cast<TDirPanel *>(FRemotePanel), static_cast<TDirPanel *>(FLocalPanel) })
    {
      for (const QModelIndex & Index : Panel->GetDirView()->selectionModel()->selectedRows())
      {
        const TDirViewItem & Item = Panel->GetModel()->Item(Index.row());
        if (!Item.IsParentDirectory)
        {
          Options.Filter->Add(Item.FileName);
        }
      }
    }
    Options.Filter->Sort();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalSynchronizeDirectory(
  const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
  bool & Continue, bool Collect, const TSynchronizeOptions * Options)
{
  if (Collect)
  {
    DebugAssert(FSynchronizeProgressForm != nullptr);
    if (!FSynchronizeProgressForm->GetStarted())
    {
      FSynchronizeProgressForm->Start();
    }
    FSynchronizeProgressForm->SetData(LocalDirectory, RemoteDirectory, Options->Files, Continue);
  }
  else
  {
    DebugAssert(FSynchronizeProgressForm == nullptr);
    DebugAssert(FProgressForm != nullptr);
  }
}
//---------------------------------------------------------------------------
int TScpCommanderForm::DoFullSynchronizeDirectories(
  UnicodeString & Directory1, UnicodeString & Directory2,
  TSynchronizeMode & Mode, int Params, bool & SaveMode)
{
  int Result;
  TTerminal * Terminal = GetTerminal();

  bool SaveSettings = false;
  int Options =
    FLAGMASK(!Terminal->IsCapable[fcTimestampChanging], fsoDisableTimestamp) |
    FLAGMASK(!CanCalculateChecksum(), fsoDisableByChecksum) |
    FLAGMASK(SynchronizeAllowSelectedOnly(), fsoAllowSelectedOnly);
  TCopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
  TUsableCopyParamAttrs CopyParamAttrs = Terminal->UsableCopyParamAttrs(0);
  bool Continue =
    DoFullSynchronizeDialog(
      Mode, Params, Directory1, Directory2, &CopyParam, SaveSettings, SaveMode, Options, CopyParamAttrs) &&
    (FLAGCLEAR(Params, TTerminal::spByChecksum) || EnsureCommandSessionFallback(fcCalculatingChecksum));
  if (Continue)
  {
    Configuration->Usage->Inc(L"Synchronizations");
    CopyParam.IncludeFileMask.SetRoots(Directory1, Directory2);

    TSynchronizeOptions SynchronizeOptions;
    GetSynchronizeOptions(Params, SynchronizeOptions);

    if (SaveSettings)
    {
      GUIConfiguration->SynchronizeParams = Params;
    }
    else
    {
      SaveMode = false;
    }

    TDateTime StartTime = Now();

    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    std::unique_ptr<TSynchronizeChecklist> Checklist;
    {
      TAutoNestingCounter BusyCounter(FBusy);
      UpdateActions();
      ::rtl::TFinally Finally([&]()
      {
        delete FSynchronizeProgressForm;
        FSynchronizeProgressForm = nullptr;
      });
      UnicodeString SessionKey = Terminal->SessionData->SessionKey;
      std::unique_ptr<TStrings> DataList(Configuration->LoadDirectoryStatisticsCache(SessionKey, Directory2, CopyParam));

      int Files = -1;
      if (DataList->Count >= 1)
      {
        Files = StrToIntDef(DataList->Strings[0], Files);
      }
      else
      {
        DataList->Add(UnicodeString());
      }

      FSynchronizeProgressForm = new TSynchronizeProgressForm(this, Files, false);
      FSynchronizeProgressForm->Start();

      Checklist.reset(
        Terminal->SynchronizeCollect(
          Directory1, Directory2,
          static_cast<TTerminal::TSynchronizeMode>(Mode),
          &CopyParam, Params | TTerminal::spNoConfirmation,
          ::rtl::MethodPtr(this, &TScpCommanderForm::TerminalSynchronizeDirectory),
          &SynchronizeOptions));

      if (Terminal->SessionData->CacheDirectories)
      {
        DataList->Strings[0] = IntToStr(SynchronizeOptions.Files);
        Configuration->SaveDirectoryStatisticsCache(SessionKey, Directory2, CopyParam, DataList.get());
      }
    }

    TSynchronizeParams SynchronizeParams;
    SynchronizeParams.Mode = Mode;
    SynchronizeParams.CopyParam = &CopyParam;
    SynchronizeParams.Params = Params;
    SynchronizeParams.Checklist = Checklist.get();
    SynchronizeParams.CollectElapsed = (Now() - StartTime);
    SynchronizeParams.StartTime = &StartTime;
    SynchronizeParams.OnProcessedItem = NULL;
    Result = Checklist->Count;
    if (Checklist->Count > 0)
    {
      if (FLAGSET(Params, TTerminal::spPreviewChanges))
      {
        bool CanQueue = FLAGCLEAR(Params, TTerminal::spTimestamp);
        TQueueSynchronizeEvent OnQueueSynchronize;
        if (CanQueue)
        {
          OnQueueSynchronize = [this](void * Token) { DoQueueSynchronize(Token); };
        }

        if (!DoSynchronizeChecklistDialog(
              Checklist.get(), Mode, Params, Directory1, Directory2,
              [this](void * Token, TProcessedSynchronizationChecklistItem OnProcessedItem,
                TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems)
              {
                TSynchronizeParams & AParams = *static_cast<TSynchronizeParams *>(Token);
                *AParams.StartTime = Now();
                FullSynchronize(AParams, OnProcessedItem, OnUpdatedSynchronizationChecklistItems);
              },
              OnQueueSynchronize,
              [this](TSynchronizeChecklist * AChecklist, const TSynchronizeChecklist::TItemList & Items, void * Token)
              {
                DoSynchronizeChecklistCalculateSize(AChecklist, Items, Token);
              },
              &SynchronizeParams))
        {
          Result = -1;
        }
      }
      else
      {
        FullSynchronize(SynchronizeParams, NULL, NULL);
      }
    }
    else
    {
      UnicodeString Message = MainInstructions(LoadStr(COMPARE_NO_DIFFERENCES));
      MessageDialog(Message, qtInformation, qaOK);
    }
  }
  else
  {
    Result = -1;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::FullSynchronize(
  TSynchronizeParams & Params, TProcessedSynchronizationChecklistItem OnProcessedItem,
  TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems)
{
  TTerminal * Terminal = GetTerminal();
  TFileOperationStatistics Statistics;
  TDateTime Start = Now();

  {
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]()
    {
      Params.OnProcessedItem = NULL;
      FLocalPanel->Reload();
    });
    Params.OnProcessedItem = OnProcessedItem;

    TSynchronizeProgress SynchronizeProgress(Params.Checklist);
    CreateProgressForm(&SynchronizeProgress);
    ::rtl::TFinally ProgressFinally([&]() { DestroyProgressForm(); });
    Terminal->SynchronizeApply(
      Params.Checklist, Params.CopyParam, Params.Params | TTerminal::spNoConfirmation,
      ::rtl::MethodPtr(this, &TScpCommanderForm::TerminalSynchronizeDirectory),
      ::rtl::MethodPtr(this, &TScpCommanderForm::SynchronizeProcessedItem),
      OnUpdatedSynchronizationChecklistItems, &Params, &Statistics);
  }

  if (AppConfiguration->SynchronizeSummary)
  {
    UnicodeString Message = MainInstructions(LoadStr(SYNCHRONIZE_COMPLETE)) + L"\n";

    // The statistics should be 0 anyway in this case
    if (FLAGCLEAR(Params.Params, TTerminal::spTimestamp))
    {
      Message += L"\n";
      if (Statistics.FilesUploaded > 0)
      {
        Message += FORMAT(LoadStrPart(SYNCHRONIZE_SUMMARY, 1), (FormatNumber(Statistics.FilesUploaded), FormatBytes(Statistics.TotalUploaded))) + L"\n";
      }
      if (Statistics.FilesDownloaded > 0)
      {
        Message += FORMAT(LoadStrPart(SYNCHRONIZE_SUMMARY, 2), (FormatNumber(Statistics.FilesDownloaded), FormatBytes(Statistics.TotalDownloaded))) + L"\n";
      }
      if (Statistics.FilesDeletedLocal > 0)
      {
        Message += FORMAT(LoadStrPart(SYNCHRONIZE_SUMMARY, 3), (FormatNumber(Statistics.FilesDeletedLocal))) + L"\n";
      }
      if (Statistics.FilesDeletedRemote > 0)
      {
        Message += FORMAT(LoadStrPart(SYNCHRONIZE_SUMMARY, 4), (FormatNumber(Statistics.FilesDeletedRemote))) + L"\n";
      }
    }

    TDateTime Elapsed = (Now() - Start);
    Message +=
      L"\n" +
      FORMAT(LoadStrPart(SYNCHRONIZE_SUMMARY, 5), (FormatDateTimeSpan(Params.CollectElapsed))) + L"\n" +
      FORMAT(LoadStrPart(SYNCHRONIZE_SUMMARY, 6), (FormatDateTimeSpan(Elapsed)));
    TMessageParams MessageParams(mpNeverAskAgainCheck);
    unsigned int Result = MessageDialog(Message, qtInformation, qaOK, &MessageParams);
    if (Result == qaNeverAskAgain)
    {
      AppConfiguration->SynchronizeSummary = false;
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeProcessedItem(void * Token, const TSynchronizeChecklist::TItem * ChecklistItem)
{
  if ((FProgressForm != nullptr) && DebugAlwaysTrue(FProgressForm->GetSynchronizeProgress() != NULL))
  {
    FProgressForm->GetSynchronizeProgress()->ItemProcessed(ChecklistItem);
  }

  // Not set in keep-up-to-date mode
  if (Token != NULL)
  {
    TSynchronizeParams & Params = *static_cast<TSynchronizeParams *>(Token);
    if (Params.OnProcessedItem != NULL)
    {
      Params.OnProcessedItem(NULL, ChecklistItem);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoQueueSynchronize(void * Token)
{
  TTerminal * Terminal = GetTerminal();
  TSynchronizeParams & Params = *static_cast<TSynchronizeParams *>(Token);

  int CopyParams = TTerminal::GetSynchronizeCopyParams(Params.Params);
  TCopyParamType SyncCopyParam = TTerminal::GetSynchronizeCopyParam(Params.CopyParam, Params.Params);

  bool Parallel = GUIConfiguration->CurrentCopyParam.QueueParallel;

  int Index = 0;
  const TSynchronizeChecklist::TItem * ChecklistItem;
  while (Params.Checklist->GetNextChecked(Index, ChecklistItem))
  {
    TQueueItem * QueueItem = Terminal->SynchronizeToQueue(ChecklistItem, &SyncCopyParam, CopyParams, Parallel);
    FQueue->AddItem(QueueItem);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoSynchronizeChecklistCalculateSize(
  TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items, void * Token)
{
  TTerminal * Terminal = GetTerminal();
  // terminal can be already closed (e.g. dropped connection)
  if ((Terminal != NULL) && Terminal->Active)
  {
    TSynchronizeParams & Params = *static_cast<TSynchronizeParams *>(Token);
    TAutoNestingCounter BusyCounter(FBusy);
    ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
    Terminal->SynchronizeChecklistCalculateSize(Checklist, Items, Params.CopyParam, Params.Params);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeActionExecute()
{
  if (IsBusy() || !FRemotePanel->IsReady())
  {
    return;
  }
  UnicodeString ALocalDirectory = LocalDirectory();
  UnicodeString RemoteDirectory = FRemotePanel->GetPath();
  if (DoSynchronizeDirectories(ALocalDirectory, RemoteDirectory) && IsExplorer())
  {
    AppConfiguration->ExplorerLastLocalTargetDirectory = ALocalDirectory;
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::DoSynchronizeDirectories(UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory)
{
  TTerminal * Terminal = GetTerminal();
  TSynchronizeParamType Params;
  Params.LocalDirectory = LocalDirectory;
  Params.RemoteDirectory = RemoteDirectory;
  int UnusedParams =
    (GUIConfiguration->SynchronizeParams &
      (TTerminal::spPreviewChanges | TTerminal::spTimestamp | TTerminal::spNotByTime | TTerminal::spBySize |
       TTerminal::spCaseSensitive | TTerminal::spMirror | TTerminal::spByChecksum));
  Params.Params = GUIConfiguration->SynchronizeParams & ~UnusedParams;
  Params.Options = GUIConfiguration->SynchronizeOptions;
  bool SaveSettings = false;
  TSynchronizeController Controller(
    ::rtl::MethodPtr(this, &TScpCommanderForm::DoSynchronize),
    ::rtl::MethodPtr(this, &TScpCommanderForm::DoSynchronizeInvalid),
    ::rtl::MethodPtr(this, &TScpCommanderForm::DoSynchronizeTooManyDirectories));
  DebugAssert(FSynchronizeController == NULL);
  FSynchronizeController = &Controller;
  bool Result;
  ::rtl::TFinally Finally([&]()
  {
    FSynchronizeController = NULL;
    DebugAssert(FOnFeedSynchronizeError == nullptr);
    FOnFeedSynchronizeError = nullptr;
    DebugAssert(FOnSynchronizeAbort == nullptr);
    FOnSynchronizeAbort = nullptr;
    FSynchronizeTerminal = NULL;
  });
  TCopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
  int CopyParamAttrs = Terminal->UsableCopyParamAttrs(0).Upload;
  int Options =
    FLAGMASK(SynchronizeAllowSelectedOnly(), soAllowSelectedOnly);
  DebugAssert(FOnFeedSynchronizeError == nullptr);
  DebugAssert(FOnSynchronizeAbort == nullptr);
  DebugAssert(FSynchronizeTerminal == NULL);
  FSynchronizeTerminal = Terminal;
  Result = DoSynchronizeDialog(Params, &CopyParam,
    ::rtl::MethodPtr(&Controller, &TSynchronizeController::StartStop),
    SaveSettings, Options, CopyParamAttrs,
    [this](int AParams, TSynchronizeOptions & AOptions) { GetSynchronizeOptions(AParams, AOptions); },
    [this](const UnicodeString & Message) { LogSynchronizeEvent(GetTerminal(), Message); },
    FOnFeedSynchronizeError, FOnSynchronizeAbort);
  if (Result)
  {
    if (SaveSettings)
    {
      GUIConfiguration->SynchronizeParams = Params.Params | UnusedParams;
      GUIConfiguration->SynchronizeOptions = Params.Options;
    }
    else
    {
      if (FLAGSET(GUIConfiguration->SynchronizeOptions, soSynchronizeAsk) &&
          FLAGCLEAR(Params.Options, soSynchronizeAsk) &&
          FLAGSET(Params.Options, soSynchronize))
      {
        GUIConfiguration->SynchronizeOptions =
          (GUIConfiguration->SynchronizeOptions & ~soSynchronizeAsk) |
          soSynchronize;
      }
    }
    LocalDirectory = Params.LocalDirectory;
    RemoteDirectory = Params.RemoteDirectory;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoSynchronize(
  TSynchronizeController * /*Sender*/, const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
  const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options, bool Full)
{
  // Keep-up-to-date mode
  TTerminal * Terminal = GetTerminal();
  if ((Terminal != NULL) && (Terminal == FSynchronizeTerminal) && (Terminal->Status == ssOpened))
  {
    try
    {
      int PParams = Params.Params;
      if (!Full)
      {
        PParams |= TTerminal::spNoRecurse | TTerminal::spUseCache | TTerminal::spDelayProgress;
        if (FLAGSET(Params.Options, soRecurse))
        {
          PParams |= TTerminal::spSubDirs;
        }
      }
      else
      {
        // if keepuptodate is non-recursive,
        // full sync before has to be non-recursive as well
        if (FLAGCLEAR(Params.Options, soRecurse))
        {
          PParams |= TTerminal::spNoRecurse;
        }
      }
      Synchronize(LocalDirectory, RemoteDirectory, smRemote, CopyParam,
        PParams, Checklist, Options);
    }
    catch (Exception & E)
    {
      if (!Terminal->Active)
      {
        ShowExtendedExceptionEx(Terminal, &E);
        // Do not abort the "keep up to date" when the session was succesfully reconnected.
        // (LinSCP: the session may be freed meanwhile, unlike upstream's
        // kept in its tab)
        if ((GetTerminal() != Terminal) || !Terminal->Active)
        {
          throw;
        }
      }
      else
      {
        if (FLAGSET(Params.Options, soContinueOnError) &&
            DebugAlwaysTrue(FOnFeedSynchronizeError != nullptr))
        {
          // noop, already logged in MoreMessageDialog
        }
        else
        {
          // We get mostly EAbort here, so this is noop.
          // Exception is an error when listing directories while looking for differences.
          ShowExtendedExceptionEx(Terminal, &E);
          throw;
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, const UnicodeString Directory,
  const UnicodeString ErrorStr)
{
  if (!Directory.IsEmpty())
  {
    SimpleErrorDialog(FMTLOAD(WATCH_ERROR_DIRECTORY, (Directory)), ErrorStr);
  }
  else
  {
    SimpleErrorDialog(LoadStr(WATCH_ERROR_GENERAL), ErrorStr);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoSynchronizeTooManyDirectories(
  TSynchronizeController * /*Sender*/, int & MaxDirectories)
{
  if (MaxDirectories < GUIConfiguration->MaxWatchDirectories)
  {
    MaxDirectories = GUIConfiguration->MaxWatchDirectories;
  }
  else
  {
    TMessageParams Params(mpNeverAskAgainCheck);
    unsigned int Result = MessageDialog(
      FMTLOAD(TOO_MANY_WATCH_DIRECTORIES, (MaxDirectories, MaxDirectories)),
      qtConfirmation, qaYes | qaNo, &Params);

    if ((Result == qaYes) || (Result == qaNeverAskAgain))
    {
      MaxDirectories *= 2;
      if (Result == qaNeverAskAgain)
      {
        GUIConfiguration->MaxWatchDirectories = MaxDirectories;
      }
    }
    else
    {
      Abort();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::Synchronize(const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, TSynchronizeMode Mode,
  const TCopyParamType & CopyParam, int Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * Options)
{
  // Keep-up-to-date mode
  TTerminal * Terminal = GetTerminal();
  DebugAssert(!FAutoOperation);
  ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
  TAutoNestingCounter BusyCounter(FBusy);
  UpdateActions();
  FAutoOperation = true;

  TSynchronizeChecklist * AChecklist = NULL;
  ::rtl::TFinally Finally([&]()
  {
    if (Checklist == NULL)
    {
      delete AChecklist;
    }
    else
    {
      *Checklist = AChecklist;
    }

    FAutoOperation = false;
    delete FSynchronizeProgressForm;
    FSynchronizeProgressForm = nullptr;
    FLocalPanel->Reload();
  });
  FSynchronizeProgressForm = new TSynchronizeProgressForm(this, -1, false);
  if (FLAGCLEAR(Params, TTerminal::spDelayProgress))
  {
    FSynchronizeProgressForm->Start();
  }

  AChecklist = Terminal->SynchronizeCollect(LocalDirectory, RemoteDirectory,
    static_cast<TTerminal::TSynchronizeMode>(Mode),
    &CopyParam, Params | TTerminal::spNoConfirmation,
    ::rtl::MethodPtr(this, &TScpCommanderForm::TerminalSynchronizeDirectory), Options);

  delete FSynchronizeProgressForm;
  FSynchronizeProgressForm = nullptr;

  if (AChecklist->CheckedCount > 0)
  {
    TSynchronizeProgress SynchronizeProgress(AChecklist);
    CreateProgressForm(&SynchronizeProgress);
    ::rtl::TFinally ProgressFinally([&]() { DestroyProgressForm(); });
    Terminal->SynchronizeApply(
      AChecklist, &CopyParam, Params | TTerminal::spNoConfirmation,
      ::rtl::MethodPtr(this, &TScpCommanderForm::TerminalSynchronizeDirectory),
      ::rtl::MethodPtr(this, &TScpCommanderForm::SynchronizeProcessedItem), NULL, NULL, NULL);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeBrowsingActionExecute()
{
  // upstream's SynchronizeBrowsingChanged
  if (SynchronizeBrowsingAction2->isChecked())
  {
    Configuration->Usage->Inc(L"SynchronizeBrowsingEnabled");
  }
  PostNote(FORMAT(LoadStrPart(SYNC_DIR_BROWSE_TOGGLE, 1),
    (LoadStrPart(SYNC_DIR_BROWSE_TOGGLE, (SynchronizeBrowsingAction2->isChecked() ? 2 : 3)))));
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeBrowsingLocal(UnicodeString PrevPath, UnicodeString & NewPath, bool Create)
{
  TTerminal * Terminal = GetTerminal();
  Terminal->ExceptionOnFail = true;
  ::rtl::TFinally Finally([&]() { Terminal->ExceptionOnFail = false; });
  std::unique_ptr<TStrings> Paths(new TStringList());
  UnicodeString LocalPath = FLocalPanel->GetPath();
  Paths->Add(IncludeTrailingBackslash(PrevPath));
  Paths->Add(IncludeTrailingBackslash(LocalPath));
  UnicodeString CommonPath;
  if (ExtractCommonPath(Paths.get(), CommonPath))
  {
    PrevPath = IncludeTrailingBackslash(PrevPath);
    CommonPath = IncludeTrailingBackslash(CommonPath);
    // With the trailing slash, as upstream's path of the remote panel
    NewPath = UnixIncludeTrailingBackslash(FRemotePanel->GetPath());
    while (!SamePaths(PrevPath, CommonPath))
    {
      if (NewPath == UnixExcludeTrailingBackslash(NewPath))
      {
        Abort();
      }
      NewPath = UnixExtractFilePath(UnixExcludeTrailingBackslash(NewPath));
      PrevPath = ExtractFilePath(ExcludeTrailingBackslash(PrevPath));
    }

    NewPath = UnixIncludeTrailingBackslash(NewPath) +
      LocalPath.SubString(PrevPath.Length() + 1, LocalPath.Length() - PrevPath.Length());
  }
  else
  {
    Abort();
  }

  if (Create)
  {
    TRemoteProperties Properties = GUIConfiguration->NewDirectoryProperties;
    Terminal->CreateDirectory(UnixExcludeTrailingBackslash(NewPath), &Properties);
  }

  FRemotePanel->ChangeDirectory(NewPath);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeBrowsingRemote(UnicodeString PrevPath, UnicodeString & NewPath, bool Create)
{
  std::unique_ptr<TStrings> Paths(new TStringList());
  UnicodeString RemotePath = FRemotePanel->GetPath();
  Paths->Add(UnixIncludeTrailingBackslash(PrevPath));
  Paths->Add(UnixIncludeTrailingBackslash(RemotePath));
  UnicodeString CommonPath;

  if (UnixExtractCommonPath(Paths.get(), CommonPath))
  {
    PrevPath = UnixIncludeTrailingBackslash(PrevPath);
    CommonPath = UnixIncludeTrailingBackslash(CommonPath);

    UnicodeString NewLocalPath;
    NewPath = ExcludeTrailingBackslash(FLocalPanel->GetPath());
    while (!UnixSamePath(PrevPath, CommonPath))
    {
      NewLocalPath = ExcludeTrailingBackslash(ExtractFileDir(NewPath));
      if (NewLocalPath == NewPath)
      {
        Abort();
      }
      NewPath = NewLocalPath;
      PrevPath = UnixExtractFilePath(UnixExcludeTrailingBackslash(PrevPath));
    }

    // The remote path is a local one on Linux (upstream converts the slashes)
    NewPath = IncludeTrailingBackslash(NewPath) +
      RemotePath.SubString(PrevPath.Length() + 1, RemotePath.Length() - PrevPath.Length());
  }
  else
  {
    Abort();
  }

  if (Create && !ForceDirectories(ApiPath(ExcludeTrailingBackslash(NewPath))))
  {
    throw EOSExtException(FMTLOAD(CREATE_DIR_ERROR, (NewPath)));
  }

  FLocalPanel->ChangeDirectory(NewPath);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeBrowsing(TDirPanel * Panel, UnicodeString PrevPath, UnicodeString & NewPath, bool Create)
{
  if (Panel == FLocalPanel)
  {
    SynchronizeBrowsingLocal(PrevPath, NewPath, Create);
  }
  else
  {
    SynchronizeBrowsingRemote(PrevPath, NewPath, Create);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SynchronizeBrowsing(TDirPanel * Panel)
{
  int Index = (Panel == FLocalPanel) ? 1 : 0;
  UnicodeString PrevPath = FPrevPath[Index];
  FPrevPath[Index] = Panel->GetPath();

  if (!FSynchronisingBrowse && SynchronizeBrowsingAction2->isChecked() && !IsExplorer() &&
      FRemotePanel->IsReady() && !PrevPath.IsEmpty() && (PrevPath != Panel->GetPath()))
  {
    TAutoFlag SynchronisingBrowseFlag(FSynchronisingBrowse);

    try
    {
      UnicodeString NewPath;

      bool Error = false;
      std::unique_ptr<TStrings> ErrorMoreMessages;

      try
      {
        SynchronizeBrowsing(Panel, PrevPath, NewPath, false);
      }
      // EAbort means that we do not know how to synchronize browsing
      // there's no fallback scenario for that
      catch (EAbort &)
      {
        throw;
      }
      catch (Exception & E)
      {
        Error = true;
        ErrorMoreMessages.reset(ExceptionToMoreMessages(&E));
      }

      if (Error)
      {
        if (::MoreMessageDialog(FMTLOAD(SYNC_DIR_BROWSE_CREATE2, (NewPath)),
              ErrorMoreMessages.get(), qtConfirmation, qaYes | qaNo) == qaYes)
        {
          try
          {
            SynchronizeBrowsing(Panel, PrevPath, NewPath, true);
          }
          catch (Exception & E)
          {
            GetTerminal()->ShowExtendedException(&E);
            throw;
          }
        }
        else
        {
          SynchronizeBrowsingAction2->setChecked(false);
        }
      }
    }
    catch (Exception &)
    {
      SynchronizeBrowsingAction2->setChecked(false);
      MessageDialog(LoadStr(SYNC_DIR_BROWSE_ERROR), qtInformation, qaOK);
    }
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::SynchronizedHomeDirectory(TDirPanel * Panel)
{
  bool Result = SynchronizeBrowsingAction2->isChecked() && !IsExplorer() && FRemotePanel->IsReady();
  if (Result)
  {
    TAutoFlag SynchronisingBrowseFlag(FSynchronisingBrowse);
    Panel->HomeDirectory();
    OtherPanel(Panel)->HomeDirectory();
  }
  return Result;
}
//---------------------------------------------------------------------------
