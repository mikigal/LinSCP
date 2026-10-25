// The synchronization of directories of the main window (upstream has it in
// forms/CustomScpExplorer.cpp and forms/ScpCommander.cpp): the comparison,
// the checklist and the synchronization, directly or in the queue.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <TerminalManager.h>

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
  UnicodeString Directory1 = FLocalPanel->GetPath();
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
