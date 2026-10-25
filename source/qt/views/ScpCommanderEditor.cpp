// The editing and opening of files from the main window (upstream has it in
// forms/CustomScpExplorer.cpp): the internal editor, external editors and
// the applications of the desktop; the remote files are edited in their
// temporary copies, which are uploaded once saved.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <CoreMain.h>
#include <EditorManager.h>
#include <EditorPreferences.h>
#include <SessionData.h>
#include <TerminalManager.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ScpCommander.h"

#include <QDesktopServices>
#include <QMenu>
#include <QProcess>
#include <QTreeView>
#include <QUrl>

#include <algorithm>
#include <functional>
#include <memory>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class TEditorUploadQueueItem : public TUploadQueueItem
{
public:
  TEditorUploadQueueItem(
      TTerminal * Terminal, TStrings * FilesToCopy, const UnicodeString & TargetDir,
      const TCopyParamType * CopyParam, int Params) :
    TUploadQueueItem(Terminal, FilesToCopy, TargetDir, CopyParam, Params, false)
  {
  }

protected:
  virtual void __fastcall DoTransferExecute(TTerminal * Terminal, TParallelOperation * ParallelOperation) override
  {
    TUploadQueueItem::DoTransferExecute(Terminal, ParallelOperation);
    TTerminalManager * Manager = TTerminalManager::Instance(false);
    if ((Manager != NULL) && (Manager->ScpExplorer != NULL))
    {
      Manager->ScpExplorer->EditedFileUploaded(Terminal, CompleteEvent);
    }
  }
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateEditorManager()
{
  FEditorManager.reset(new TEditorManager());
  FEditorManager->OnFileChange =
    [this](const UnicodeString & FileName, const TDateTime & Timestamp, TEditedFileData * Data,
      HANDLE UploadCompleteEvent, bool & Retry)
    {
      ExecutedFileChanged(FileName, Timestamp, Data, UploadCompleteEvent, Retry);
    };
  FEditorManager->OnFileReload =
    [this](const UnicodeString & FileName, TEditedFileData * Data) { ExecutedFileReload(FileName, Data); };
  FEditorManager->OnFileEarlyClosed =
    [this](const TEditedFileData * Data, bool & KeepOpen) { ExecutedFileEarlyClosed(Data, KeepOpen); };
  FEditorManager->OnFileUploadComplete =
    [](void * Token) { EditorFormFileUploadComplete(static_cast<QWidget *>(Token)); };
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::RemoteExecuteForceText(TExecuteFileBy ExecuteFileBy, const TEditorData * ExternalEditor)
{
  DebugAssert((ExecuteFileBy == efExternalEditor) ==
    ((ExternalEditor != NULL) && (ExternalEditor->Editor == edExternal)));
  DebugAssert(ExecuteFileBy != efDefaultEditor);

  return
    ((ExecuteFileBy == efInternalEditor)) ||
    ((ExecuteFileBy == efExternalEditor) && ExternalEditor->ExternalEditorText);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TemporaryDirectoryForRemoteFiles(
  const UnicodeString & RemoteDirectory, const TCopyParamType & CopyParam,
  UnicodeString & Result, UnicodeString & RootDirectory)
{
  if (!AppConfiguration->TemporaryDirectoryDeterministic)
  {
    RootDirectory = IncludeTrailingBackslash(AppConfiguration->TemporaryDir());
    Result = RootDirectory;
  }
  else
  {
    RootDirectory = L"";
    Result = AppConfiguration->ExpandedTemporaryDirectory();
    Result = IncludeTrailingBackslash(Result);
  }

  if (AppConfiguration->TemporaryDirectoryAppendSession)
  {
    Result = IncludeTrailingBackslash(Result + MakeValidFileName(GetTerminal()->SessionData->SessionName));
  }

  if (AppConfiguration->TemporaryDirectoryAppendPath)
  {
    // LinSCP: upstream's FromUnixPath and ValidLocalPath work with
    // Windows paths
    UnicodeString ARemoteDirectory = RemoteDirectory;
    UnicodeString LocalPath;
    while (!ARemoteDirectory.IsEmpty())
    {
      UnicodeString Name = CutToChar(ARemoteDirectory, L'/', false);
      if (!Name.IsEmpty())
      {
        AddToList(LocalPath, CopyParam.ValidLocalFileName(Name), L"/");
      }
    }
    Result = IncludeTrailingBackslash(Result + LocalPath);
  }

  if (!ForceDirectories(ApiPath(Result)))
  {
    throw EOSExtException(FMTLOAD(CREATE_TEMP_DIR_ERROR, (Result)));
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteFileNormalize(
  TExecuteFileBy & ExecuteFileBy, const TEditorData *& ExternalEditor,
  const UnicodeString & FileName, bool Local, const TFileMasks::TParams & MaskParams)
{
  if (ExecuteFileBy == efDefaultEditor)
  {
    // Upstream offers an editor detected in the system (EditorAutoConfig)
    const TEditorPreferences * Editor =
      AppConfiguration->DefaultEditorForFile(FileName, Local, MaskParams);
    if ((Editor == NULL) || (Editor->Data->Editor == edInternal))
    {
      ExecuteFileBy = efInternalEditor;
      ExternalEditor = NULL;
    }
    else if (Editor->Data->Editor == edOpen)
    {
      ExecuteFileBy = efShell;
      ExternalEditor = NULL;
    }
    else
    {
      ExecuteFileBy = efExternalEditor;
      ExternalEditor = Editor->Data;
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CustomExecuteFile(TOperationSide Side,
  TExecuteFileBy ExecuteFileBy, UnicodeString FileName, UnicodeString OriginalFileName,
  const TEditorData * ExternalEditor, UnicodeString LocalRootDirectory,
  UnicodeString RemoteDirectory, bool NewFile, const TDateTime & SourceTimestamp)
{
  DebugAssert((ExecuteFileBy == efExternalEditor) ==
    ((ExternalEditor != NULL) && (ExternalEditor->Editor == edExternal)));
  DebugAssert(ExecuteFileBy != efDefaultEditor);

  TTerminal * Terminal = GetTerminal();
  bool Remote = (Side == osRemote);
  std::unique_ptr<TEditedFileData> Data(new TEditedFileData);
  if (Remote)
  {
    Data->Terminal = Terminal;
    Data->SessionData = Terminal->SessionData->Clone();
    Data->ForceText = RemoteExecuteForceText(ExecuteFileBy, ExternalEditor);
    Data->RemoteDirectory = RemoteDirectory;
    Data->SessionName = Terminal->SessionData->SessionName;
    Data->LocalRootDirectory = LocalRootDirectory;
    Data->OriginalFileName = OriginalFileName;
    Data->Command = L""; // will be changed later for external editor
    // Empty SourceTimestamp indicates that checking was disabled when the file was opened
    if (AppConfiguration->EditorCheckNotModified)
    {
      Data->SourceTimestamp = SourceTimestamp;
    }
  }

  auto SaveAll = [this](QWidget *) { SaveAllInternalEditors(); };
  auto AnyModified = [this](QWidget *, bool & Modified) { AnyInternalEditorModified(Modified); };

  if (ExecuteFileBy == efInternalEditor)
  {
    if (Remote)
    {
      UnicodeString RemotePath = UnixCombinePaths(RemoteDirectory, OriginalFileName);
      UnicodeString Caption = RemotePath + TitleSeparator + Terminal->SessionData->SessionName;
      QWidget * Editor;
      try
      {
        TEditorManager * Manager = FEditorManager.get();
        Editor =
          ShowEditorForm(
            FileName, this,
            [Manager](QWidget * Form) { Manager->FileChanged(Form); },
            [Manager](QWidget * Form) { Manager->FileReload(Form); },
            [Manager](QWidget * Form, bool Forced) { Manager->FileClosed(Form, Forced); },
            SaveAll, AnyModified, Caption, NewFile);
      }
      catch (...)
      {
        if (!LocalRootDirectory.IsEmpty())
        {
          RecursiveDeleteFile(ExcludeTrailingBackslash(LocalRootDirectory));
        }
        throw;
      }

      FEditorManager->AddFileInternal(FileName, Data.release(), Editor);
      AppLogFmt(L"Opened internal editor with \"%s\"", (FileName));
    }
    else
    {
      QWidget * Editor =
        ShowEditorForm(FileName, this, nullptr, nullptr,
          [this](QWidget * Form, bool) { LocalEditorClosed(Form); },
          SaveAll, AnyModified, L"", NewFile);
      FLocalEditors.push_back(Editor);
    }
  }
  else
  {
    qint64 Process = 0;

    if (ExecuteFileBy == efExternalEditor)
    {
      UnicodeString Program, Params, Dir;
      Data->Command = ExternalEditor->ExternalEditor;
      ReformatFileNameCommand(Data->Command);
      SplitCommand(Data->Command, Program, Params, Dir);
      Params = ExpandFileNameCommand(Params, FileName);
      Program = ExpandEnvironmentVariables(Program);
      // LinSCP: run by /bin/sh, as the local custom commands
      UnicodeString Command = FormatCommand(Program, Params);
      AppLogFmt(L"Executing editor: %s", (Command));
      // Detached, so that the editor survives us, as on Windows
      if (!QProcess::startDetached(
            QStringLiteral("/bin/sh"), { QStringLiteral("-c"), ToQString(Command) }, QString(), &Process))
      {
        throw EOSExtException(FMTLOAD(EDITOR_ERROR, (Program)));
      }
      if (!Remote || !ExternalEditor->SDIExternalEditor)
      {
        // no need for the process
        Process = 0;
      }
    }
    else
    {
      // The application of the desktop (upstream's shell execution); its
      // process is not known
      if (!QDesktopServices::openUrl(QUrl::fromLocalFile(ToQString(FileName))))
      {
        throw EOSExtException(FMTLOAD(EXECUTE_FILE_ERROR, (FileName)));
      }
      if (Remote && AppConfiguration->Editor.SDIShellEditor)
      {
        throw ExtException(LoadStr(OPEN_FILE_NO_PROCESS2), L"");
      }
    }

    if (Remote)
    {
      FEditorManager->AddFileExternal(FileName, Data.release(), Process);
      AppLogFmt(L"Opened external editor with remote file \"%s\".", (FileName));
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteFile(TOperationSide Side,
  TExecuteFileBy ExecuteFileBy, const TEditorData * ExternalEditor,
  UnicodeString FullFileName, TRemoteFile * File, const TFileMasks::TParams & MaskParams)
{
  TTerminal * Terminal = GetTerminal();
  bool Remote = (Side == osRemote);
  UnicodeString OriginalFileName;
  UnicodeString LocalRootDirectory;
  UnicodeString RemoteDirectory;
  ExecuteFileNormalize(ExecuteFileBy, ExternalEditor, FullFileName, !Remote, MaskParams);

  UnicodeString LocalFileName;
  bool Handled = false;
  if (Remote)
  {
    // We need to trim VMS version here, so that we use name without version
    // when uploading back to create a new version of the file
    OriginalFileName = Terminal->GetBaseFileName(UnixExtractFileName(FullFileName));
    RemoteDirectory = UnixExtractFilePath(FullFileName);
    void * Token = NULL;
    UnicodeString LocalDirectory;
    if (!FEditorManager->CanAddFile(RemoteDirectory, OriginalFileName,
           Terminal->SessionData->SessionName, Token, LocalRootDirectory,
           LocalDirectory))
    {
      if (Token != NULL)
      {
        QWidget * Form = static_cast<QWidget *>(Token);
        if (Form->isMinimized())
        {
          Form->showNormal();
        }
        Form->raise();
        Form->activateWindow();
        Handled = true;
      }
      else
      {
        throw Exception(FMTLOAD(ALREADY_EDITED_EXTERNALLY_OR_UPLOADED, (OriginalFileName)));
      }
    }

    if (!Handled)
    {
      std::unique_ptr<TStrings> FileList1(new TStringList());
      FileList1->AddObject(FullFileName, File);
      bool ForceText = RemoteExecuteForceText(ExecuteFileBy, ExternalEditor);
      TCopyParamType CopyParam = TemporaryFileCopyParam(ForceText);
      if (LocalDirectory.IsEmpty())
      {
        TemporaryDirectoryForRemoteFiles(RemoteDirectory, CopyParam, LocalDirectory, LocalRootDirectory);
      }
      TemporarilyDownloadFiles(FileList1.get(), ForceText, LocalRootDirectory, LocalDirectory, true);
      LocalFileName = LocalDirectory + Terminal->ChangeFileName(&CopyParam, UnixExtractFileName(FullFileName), osRemote, false);
    }
  }
  else
  {
    LocalFileName = FullFileName;
    OriginalFileName = ExtractFileName(FullFileName);
  }

  if (!Handled)
  {
    CustomExecuteFile(Side, ExecuteFileBy, LocalFileName, OriginalFileName,
      ExternalEditor, LocalRootDirectory, RemoteDirectory, false, MaskParams.Modification);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteFile(TOperationSide Side, TExecuteFileBy ExecuteFileBy,
  const TEditorData * ExternalEditor, bool AllSelected)
{
  TDirPanel * APanel = Panel(Side);
  if (IsBusy() || !APanel->IsReady())
  {
    return;
  }

  TTerminal * Terminal = GetTerminal();
  std::vector<int> Rows = APanel->GetSelectedRows();
  if (!AllSelected)
  {
    int Current = APanel->GetDirView()->currentIndex().row();
    Rows.clear();
    if (Current >= 0)
    {
      Rows.push_back(Current);
    }
  }
  for (int Row : Rows)
  {
    const TDirViewItem & Item = APanel->GetModel()->Item(Row);
    if (!Item.IsDirectory && !Item.IsParentDirectory)
    {
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Item.Size;
      MaskParams.Modification = Item.Modification;
      if (Side == osRemote)
      {
        UnicodeString FullFileName = UnixCombinePaths(APanel->GetPath(), Item.FileName);
        // A fresh copy of the file, the panel may be reloaded meanwhile
        std::unique_ptr<TRemoteFile> File(Terminal->TryReadFile(FullFileName));
        if (File == nullptr)
        {
          File.reset(Item.File->Duplicate(true));
        }
        File->FullFileName = FullFileName;
        MaskParams.Size = File->Size;
        MaskParams.Modification = File->Modification;
        ExecuteFile(Side, ExecuteFileBy, ExternalEditor, FullFileName, File.get(), MaskParams);
      }
      else
      {
        ExecuteFile(Side, ExecuteFileBy, ExternalEditor,
          IncludeTrailingBackslash(APanel->GetPath()) + Item.FileName, NULL, MaskParams);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::EditNew(TOperationSide Side)
{
  TDirPanel * APanel = Panel(Side);
  if (IsBusy() || !APanel->IsReady())
  {
    return;
  }

  TTerminal * Terminal = GetTerminal();
  UnicodeString Name;
  int Current = APanel->GetDirView()->currentIndex().row();
  if ((Current >= 0) && !APanel->GetModel()->Item(Current).IsDirectory &&
      !APanel->GetModel()->Item(Current).IsParentDirectory)
  {
    Name = APanel->GetModel()->Item(Current).FileName;
  }
  else
  {
    Name = LoadStr(NEW_FILE);
  }
  UnicodeString Names = Name;
  std::unique_ptr<TStrings> History(CloneStrings(AppConfiguration->History[L"EditFile"]));
  if (InputDialog(LoadStr(EDIT_FILE_CAPTION), LoadStr(EDIT_FILE_PROMPT), Names, History.get(), true))
  {
    while (!Names.IsEmpty())
    {
      Name = CutToChar(Names, FileMasksDelimiters[1], false);
      AppConfiguration->History[L"EditFile"] = History.get();
      UnicodeString TargetFileName;
      UnicodeString LocalFileName;
      UnicodeString RootTempDir;
      UnicodeString TempDir;
      UnicodeString RemoteDirectory;
      bool ExistingFile = false;
      if (Side == osRemote)
      {
        Name = AbsolutePath(Terminal->CurrentDirectory, Name);

        std::unique_ptr<TRemoteFile> File(Terminal->TryReadFile(Name));
        if ((File != nullptr) && !File->IsDirectory)
        {
          File->FullFileName = Name;
          TFileMasks::TParams MaskParams;
          MaskParams.Size = File->Size;
          MaskParams.Modification = File->Modification;
          ExecuteFile(osRemote, efDefaultEditor, NULL, Name, File.get(), MaskParams);
          ExistingFile = true;
        }
        File.reset();

        if (!ExistingFile)
        {
          RemoteDirectory = UnixExtractFilePath(Name);
          TemporaryDirectoryForRemoteFiles(
            RemoteDirectory, GUIConfiguration->CurrentCopyParam, TempDir, RootTempDir);

          TargetFileName = UnixExtractFileName(Name);
          TCopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
          LocalFileName = TempDir +
            // We probably do not want to trim the VMS version here
            Terminal->ChangeFileName(&CopyParam, TargetFileName, osRemote, false);
        }
      }
      else
      {
        if (ExtractFilePath(Name).IsEmpty())
        {
          LocalFileName = IncludeTrailingBackslash(APanel->GetPath()) + Name;
        }
        else
        {
          LocalFileName = ExpandFileName(Name);
        }

        TargetFileName = ExtractFileName(Name);
      }

      if (!ExistingFile)
      {
        bool NewFile = !FileExists(ApiPath(LocalFileName));
        if (NewFile)
        {
          THandle File = FileCreate(ApiPath(LocalFileName));
          if (File == reinterpret_cast<THandle>(INVALID_HANDLE_VALUE))
          {
            if (!RootTempDir.IsEmpty())
            {
              RecursiveDeleteFile(ExcludeTrailingBackslash(RootTempDir));
            }
            throw Exception(FMTLOAD(CREATE_FILE_ERROR, (LocalFileName)));
          }
          else
          {
            FileClose(File);
          }
        }

        TExecuteFileBy ExecuteFileBy = efDefaultEditor;
        const TEditorData * ExternalEditor = NULL;
        TFileMasks::TParams MaskParams; // size not known
        ExecuteFileNormalize(ExecuteFileBy, ExternalEditor, TargetFileName,
          (Side == osLocal), MaskParams);

        CustomExecuteFile(Side, ExecuteFileBy, LocalFileName, TargetFileName,
          ExternalEditor, RootTempDir, RemoteDirectory, NewFile, TDateTime());
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::EditedFileUploaded(TTerminal * ATerminal, HANDLE UploadCompleteEvent)
{
  // Called by the thread of the queue
  if (AppConfiguration->EditorCheckNotModified) // optimization
  {
    UnicodeString RemoteFilePath;
    {
      std::lock_guard<std::recursive_mutex> Guard(FEditorManager->Section);
      TEditedFileData * Data = FEditorManager->FindByUploadCompleteEvent(UploadCompleteEvent);
      if ((Data != NULL) && (Data->SourceTimestamp != TDateTime()))
      {
        RemoteFilePath = UnixCombinePaths(Data->RemoteDirectory, Data->OriginalFileName);
      }
    }
    if (!RemoteFilePath.IsEmpty())
    {
      std::unique_ptr<TRemoteFile> File(ATerminal->TryReadFile(RemoteFilePath));
      if (File != nullptr)
      {
        std::lock_guard<std::recursive_mutex> Guard(FEditorManager->Section);
        TEditedFileData * Data = FEditorManager->FindByUploadCompleteEvent(UploadCompleteEvent);
        if (Data != NULL)
        {
          Data->SourceTimestamp = File->Modification;
          AppLogFmt(L"Uploaded edited remote file timestamp: %s", (StandardTimestamp(Data->SourceTimestamp)));
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecutedFileChanged(
  const UnicodeString & FileName, const TDateTime & Timestamp, TEditedFileData * Data,
  HANDLE UploadCompleteEvent, bool & Retry)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  TManagedTerminal * DataTerminal = dynamic_cast<TManagedTerminal *>(Data->Terminal);

  bool IsInavailable = !Manager->IsAvailableTerminal(Data->Terminal);
  if (Manager->IsReconnectingTerminal(Data->Terminal))
  {
    AppLog(L"Session is being reconnected, will retry the upload later");
    Retry = true;
  }
  else if (IsInavailable &&
           GUIConfiguration->SessionReopenAutoInactive &&
           (DataTerminal != NULL) &&
           (DataTerminal->NextInactiveReconnect != TDateTime()))
  {
    if (DataTerminal->LastInactiveSave != Timestamp)
    {
      AppLog(L"Inactive session reconnect is pending, rescheduling it for as soon as possible to retry the upload");
      DataTerminal->NextInactiveReconnect = Now();
      DataTerminal->LastInactiveSave = Timestamp;
    }
    else
    {
      AppLog(L"Inactive session reconnect is pending, will retry the upload later");
    }
    Retry = true;
  }
  else if (IsInavailable)
  {
    if (!IsBusy())
    {
      UnicodeString FileNameOnly = ExtractFileName(FileName);
      if (Data->Terminal == NULL)
      {
        TManagedTerminal * SameSiteTerminal = Manager->FindActiveTerminalForSite(Data->SessionData);
        if (SameSiteTerminal != NULL)
        {
          UnicodeString Message =
            FMTLOAD(EDIT_SESSION_REATTACH,
              (FileNameOnly, Data->SessionName, FileNameOnly));
          if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) == qaOK)
          {
            Data->Terminal = SameSiteTerminal;
            Data->SessionName = SameSiteTerminal->SessionData->SessionName;
            // We might also overwrite session data
          }
          else
          {
            Abort();
          }
        }
      }
      // Foreground session should reconnect itself (except for those disconnected explicitly by the user)
      else if (DebugAlwaysTrue(DataTerminal != NULL) &&
               ((DataTerminal != FManagedSession) || DataTerminal->Disconnected))
      {
        UnicodeString Message =
          MainInstructions(
            FMTLOAD(EDIT_SESSION_RECONNECT, (Data->SessionName, FileNameOnly)));
        TMessageParams Params(FLAGMASK(!GUIConfiguration->SessionReopenAutoInactive, mpNeverAskAgainCheck));
        Params.NeverAskAgainTitle = LoadStr(ALWAYS_RECONNECT);
        unsigned int Answer = MessageDialog(Message, qtConfirmation, qaOK | qaCancel, &Params);
        if (Answer == qaNeverAskAgain)
        {
          GUIConfiguration->SessionReopenAutoInactive = true;
          Answer = qaOK;
        }

        if (Answer == qaOK)
        {
          // LinSCP: upstream tests whether the session shown is the
          // disconnected one
          if (DataTerminal->Disconnected)
          {
            Manager->ActiveSession = DataTerminal;
            ReconnectSessionActionExecute();
            // Was it reconnected? (if not, Data->Terminal should be nulled now)
            if (!Manager->IsAvailableTerminal(Data->Terminal) || (FManagedSession != Data->Terminal))
            {
              Abort();
            }
          }
          else
          {
            Manager->SetActiveTerminalWithAutoReconnect(DataTerminal);
          }
        }
        else
        {
          Abort();
        }
      }
    }

    if (!Manager->IsAvailableTerminal(Data->Terminal))
    {
      // Prevent this when not idle (IsBusy())?
      throw Exception(FMTLOAD(EDIT_SESSION_CLOSED2,
        (ExtractFileName(FileName), Data->SessionName)));
    }
  }

  if (!Retry && AppConfiguration->EditorCheckNotModified && (Data->SourceTimestamp != TDateTime()))
  {
    if (IsBusy())
    {
      Retry = true;
    }
    else
    {
      UnicodeString RemoteFilePath = UnixCombinePaths(Data->RemoteDirectory, Data->OriginalFileName);
      std::unique_ptr<TRemoteFile> File(Data->Terminal->TryReadFile(RemoteFilePath));
      if (File != nullptr)
      {
        AppLogFmt(L"Edited remote file timestamp: %s, Original timestamp: %s",
          (StandardTimestamp(File->Modification), StandardTimestamp(Data->SourceTimestamp)));

        if (File->Modification != Data->SourceTimestamp)
        {
          UnicodeString Message = MainInstructions(LoadStr(EDIT_CHANGED_EXTERNALLY));
          if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) != qaOK)
          {
            Abort();
          }
        }
      }
    }
  }

  if (!Retry)
  {
    std::unique_ptr<TStrings> FileList(new TStringList());
    FileList->Add(FileName);

    // Consider using the same settings (preset) as when the file was downloaded.
    // More over this does not reflect the actual session that will do the upload.
    TCopyParamType CopyParam = TemporaryFileCopyParam(Data->ForceText);
    // so i do not need to worry if masking algorithm works in all cases
    // ("" means "copy file name", no masking is actually done)
    if (ExtractFileName(FileName) == Data->OriginalFileName)
    {
      CopyParam.FileMask = L"";
    }
    else
    {
      CopyParam.FileMask = DelimitFileNameMask(Data->OriginalFileName);
    }

    int Params = cpNoConfirmation | cpTemporary;
    if (Data->Terminal->IsCapable[fcBackgroundTransfers])
    {
      TTerminalQueue * AQueue = Manager->FindQueueForTerminal(Data->Terminal);
      TQueueItem * QueueItem =
        new TEditorUploadQueueItem(Data->Terminal, FileList.get(), Data->RemoteDirectory, &CopyParam, Params);
      QueueItem->CompleteEvent = UploadCompleteEvent;
      AQueue->AddItem(QueueItem);
    }
    else
    {
      if (IsBusy())
      {
        Retry = true;
      }
      else
      {
        ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
        TAutoNestingCounter BusyCounter(FBusy);
        UpdateActions();
        ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
        Data->Terminal->CopyToRemote(FileList.get(), Data->RemoteDirectory, &CopyParam, Params, NULL);
        SetEvent(UploadCompleteEvent);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecutedFileReload(const UnicodeString & FileName, TEditedFileData * Data)
{
  // Sanity check, we should not be busy otherwise user would not be able to click Reload button.
  DebugAssert(!IsBusy());

  if ((Data->Terminal == NULL) || !Data->Terminal->Active || (Data->Terminal != GetTerminal()))
  {
    throw Exception(FMTLOAD(EDIT_SESSION_CLOSED_RELOAD,
      (ExtractFileName(FileName), Data->SessionName)));
  }

  TTerminal * Terminal = Data->Terminal;
  std::unique_ptr<TRemoteFile> File;
  UnicodeString RemoteFileName =
    UnixIncludeTrailingBackslash(Data->RemoteDirectory) + Data->OriginalFileName;
  {
    Terminal->ExceptionOnFail = true;
    ::rtl::TFinally Finally([&]() { Terminal->ExceptionOnFail = false; });
    File.reset(Terminal->ReadFile(RemoteFileName));
    if (!File->HaveFullFileName)
    {
      File->FullFileName = RemoteFileName;
    }
  }
  std::unique_ptr<TStrings> FileList(new TStringList());
  FileList->AddObject(RemoteFileName, File.get());

  UnicodeString RootTempDir = Data->LocalRootDirectory;
  UnicodeString TempDir = ExtractFilePath(FileName);

  ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
  TAutoNestingCounter BusyCounter(FBusy);
  UpdateActions();
  ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
  TemporarilyDownloadFiles(FileList.get(), Data->ForceText, RootTempDir, TempDir, true);

  if (AppConfiguration->EditorCheckNotModified && (Data->SourceTimestamp != TDateTime()))
  {
    Data->SourceTimestamp = File->Modification;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecutedFileEarlyClosed(const TEditedFileData * Data, bool & KeepOpen)
{
  // Command is set for external editors only (not for "shell" open).
  if (!Data->Command.IsEmpty())
  {
    bool AnyFound = false;
    bool AnyMDI = false;
    bool AnyNonMDI = false;
    bool AnyDetect = false;
    std::unique_ptr<TEditorList> EditorList(new TEditorList());
    *EditorList = *AppConfiguration->EditorList;
    for (int Index = 0; Index < EditorList->Count; Index++)
    {
      const TEditorPreferences * Editor = EditorList->Editors[Index];
      if ((Editor->Data->Editor == edExternal) &&
          (Editor->Data->ExternalEditor == Data->Command))
      {
        AnyFound = true;
        if (Editor->Data->SDIExternalEditor)
        {
          AnyNonMDI = true;
          if (Editor->Data->DetectMDIExternalEditor)
          {
            AnyDetect = true;
          }
        }
        else
        {
          AnyMDI = true;
        }
      }
    }

    bool EnableMDI = false;
    bool DisableDetect = false;

    if (AnyMDI)
    {
      KeepOpen = true;
      if (AnyNonMDI)
      {
        // there is at least one instance of the editor with MDI support enabled,
        // and one with disabled, enable it for all instances
        EnableMDI = true;
      }
    }
    else if (AnyFound && !AnyDetect)
    {
      // at least once instance found but all have MDI autodetection disabled
      // => close the file (default action)
    }
    else
    {
      // no instance of the editor has MDI support enabled

      TMessageParams Params;
      if (AnyFound)
      {
        // there is at least one instance of the editor
        Params.Params |= mpNeverAskAgainCheck;
      }
      unsigned int Answer = MessageDialog(FMTLOAD(EDITOR_EARLY_CLOSED2, (Data->OriginalFileName)), qtWarning,
        qaYes | qaNo, &Params);
      switch (Answer)
      {
        case qaNeverAskAgain:
          DisableDetect = true;
          break;

        case qaNo:
          EnableMDI = true;
          KeepOpen = true;
          break;
      }
    }

    if (AnyFound && (EnableMDI || DisableDetect))
    {
      bool Changed = false;
      for (int Index = 0; Index < EditorList->Count; Index++)
      {
        const TEditorPreferences * Editor = EditorList->Editors[Index];
        if ((Editor->Data->Editor == edExternal) &&
            (Editor->Data->ExternalEditor == Data->Command) &&
            ((EnableMDI && Editor->Data->SDIExternalEditor) ||
             (DisableDetect && Editor->Data->DetectMDIExternalEditor)))
        {
          Changed = true;
          TEditorPreferences * UpdatedEditor = new TEditorPreferences(*Editor);
          if (EnableMDI)
          {
            UpdatedEditor->GetData()->SDIExternalEditor = false;
          }
          if (DisableDetect)
          {
            UpdatedEditor->GetData()->DetectMDIExternalEditor = false;
          }
          EditorList->Change(Index, UpdatedEditor);
        }
      }

      if (Changed)
      {
        AppConfiguration->EditorList = EditorList.get();
      }
    }
  }
  else
  {
    // "open" case

    MessageDialog(FMTLOAD(APP_EARLY_CLOSED, (Data->OriginalFileName)), qtWarning, qaOK);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SaveAllInternalEditors()
{
  for (QWidget * Editor : std::vector<QWidget *>(FLocalEditors))
  {
    EditorFormFileSave(Editor);
  }

  FEditorManager->ProcessFiles(
    [](const UnicodeString &, TEditedFileData *, void * Token)
    {
      if (Token != NULL)
      {
        EditorFormFileSave(static_cast<QWidget *>(Token));
      }
    });
}
//---------------------------------------------------------------------------
void TScpCommanderForm::AnyInternalEditorModified(bool & Modified)
{
  for (QWidget * Editor : FLocalEditors)
  {
    if (IsEditorFormModified(Editor))
    {
      Modified = true;
    }
  }

  if (!Modified)
  {
    FEditorManager->ProcessFiles(
      [&Modified](const UnicodeString &, TEditedFileData *, void * Token)
      {
        if ((Token != NULL) && IsEditorFormModified(static_cast<QWidget *>(Token)))
        {
          Modified = true;
        }
      });
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LocalEditorClosed(QWidget * Editor)
{
  auto I = std::find(FLocalEditors.begin(), FLocalEditors.end(), Editor);
  if (DebugAlwaysTrue(I != FLocalEditors.end()))
  {
    FLocalEditors.erase(I);
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CloseEditors()
{
  // upstream's FormCloseQuery
  bool CanClose =
    FEditorManager->CloseInternalEditors([](void * Token) { static_cast<QWidget *>(Token)->close(); }) &&
    FEditorManager->CloseExternalFilesWithoutProcess();

  if (CanClose)
  {
    while (CanClose && !FLocalEditors.empty())
    {
      size_t PrevCount = FLocalEditors.size();
      FLocalEditors.front()->close();
      CanClose = (FLocalEditors.size() < PrevCount);
    }

    if (CanClose)
    {
      CanClose = FEditorManager->Empty(true);
      if (!CanClose)
      {
        activateWindow();
        CanClose = (MessageDialog(LoadStr(PENDING_EDITORS), qtWarning, qaIgnore | qaCancel) == qaIgnore);
      }
    }
  }
  return CanClose;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ForceCloseEditors()
{
  FEditorManager->CloseInternalEditors([](void * Token) { delete static_cast<QWidget *>(Token); });
  while (!FLocalEditors.empty())
  {
    delete FLocalEditors.front();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::EditedFilesTerminalRemoved(TTerminal * ATerminal)
{
  FEditorManager->ProcessFiles(
    [ATerminal](const UnicodeString &, TEditedFileData * Data, void *)
    {
      if (Data->Terminal == ATerminal)
      {
        Data->Terminal = NULL;
      }
    });
}
//---------------------------------------------------------------------------
void TScpCommanderForm::FileActivated(TDirPanel * APanel)
{
  // upstream's DoDirViewExecFile (files only, the panel enters directories)
  TOperationSide Side = APanel->GetSide();
  switch (AppConfiguration->DoubleClickAction)
  {
    case dcaCopy:
      {
        TTerminal * Terminal = GetTerminal();
        if ((Terminal != NULL) && Terminal->Active && OtherPanel(APanel)->IsReady() && !IsBusy())
        {
          // The focused file only
          QModelIndex Current = APanel->GetDirView()->currentIndex();
          if (Current.isValid())
          {
            std::unique_ptr<TStrings> FileList(CreateFileList(APanel, true, Current.row()));
            if (FileList->Count > 0)
            {
              TTransferOperationParam Param;
              Param.NoConfirmation = !AppConfiguration->CopyOnDoubleClickConfirmation;
              ExecuteCopyMoveFileOperation(foCopy, Side, FileList.get(), Param);
            }
          }
        }
      }
      break;

    case dcaOpen:
      ExecuteFile(Side, efShell, NULL, false);
      break;

    default:
      ExecuteFile(Side, efDefaultEditor, NULL, false);
      break;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoEditFoundFiles(TTerminal * ATerminal, TStrings * FileList,
  TFileOperationFinishedEvent OnFileOperationFinished)
{
  if (CanOperateOnFoundFiles(ATerminal))
  {
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      UnicodeString FileName = FileList->Strings[Index];
      TRemoteFile * File = static_cast<TRemoteFile *>(FileList->Objects[Index]);
      // needed for checking filemasks, as there's no directory object
      // associated with the file object
      File->FullFileName = FileName;
      TFileMasks::TParams MaskParams;
      MaskParams.Size = File->Size;
      MaskParams.Modification = File->Modification;
      ExecuteFile(osRemote, efDefaultEditor, NULL, FileName, File, MaskParams);
      OnFileOperationFinished(osRemote, FileName, true, true);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentEditActionExecute()
{
  ExecuteFile(FCurrentPanel->GetSide(), efDefaultEditor, NULL, true);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentEditInternalActionExecute()
{
  ExecuteFile(FCurrentPanel->GetSide(), efInternalEditor, NULL, true);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentOpenActionExecute()
{
  ExecuteFile(FCurrentPanel->GetSide(), efShell, NULL, true);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::NewFileActionExecute()
{
  EditNew(FCurrentPanel->GetSide());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateEditWithMenu(QMenu * Menu, TOperationSide Side)
{
  // upstream's CreateEditorListMenu: the default editor, the internal one,
  // the external ones of the list (each once), any other, the list; of the
  // panel of the toolbar or of the current one
  auto Item = [this, Menu, Side](const QString & Caption, const QString & Hint, std::function<void()> Execute)
  {
    QAction * Action = Menu->addAction(Caption);
    Action->setToolTip(Hint);
    connect(Action, &QAction::triggered, this, [this, Side, Execute]()
    {
      ExecuteHandler([this, Side, Execute]()
      {
        if (Side != osCurrent)
        {
          ActivatePanel(Side);
        }
        Execute();
      });
    });
    return Action;
  };
  bool Enabled = CurrentEditAction->isEnabled();
  if (Side != osCurrent)
  {
    Enabled = findChild<QAction *>(
      (Side == osLocal) ? QStringLiteral("LocalEditAction2") : QStringLiteral("RemoteEditAction2"),
      Qt::FindDirectChildrenOnly)->isEnabled();
  }
  Menu->setToolTipsVisible(true);
  QAction * EditItem = Item(CurrentEditAction->text(), CurrentEditAction->toolTip(), [this]() { CurrentEditActionExecute(); });
  EditItem->setIcon(CurrentEditAction->icon());
  EditItem->setEnabled(Enabled);
  Menu->addSeparator();
  Item(CurrentEditInternalAction->text(), tr("Edit selected file(s) using internal editor"),
    [this]() { CurrentEditInternalActionExecute(); })->setEnabled(Enabled);
  Menu->addSeparator();
  std::vector<UnicodeString> UsedEditors;
  const TEditorList * EditorList = AppConfiguration->EditorList;
  for (int Index = 0; Index < EditorList->Count; Index++)
  {
    const TEditorPreferences * Editor = EditorList->Editors[Index];
    if ((Editor->Data->Editor == edExternal) &&
        (std::find(UsedEditors.begin(), UsedEditors.end(), Editor->Data->ExternalEditor) == UsedEditors.end()))
    {
      UsedEditors.push_back(Editor->Data->ExternalEditor);
      UnicodeString Hint = FMTLOAD(EXTERNAL_EDITOR_HINT, (Editor->Name));
      TEditorData Data = *Editor->Data;
      QString LongHint = ToQString(Hint);
      LongHint = LongHint.contains(QLatin1Char('|')) ? LongHint.section(QLatin1Char('|'), 1) : LongHint;
      Item(ToQString(Editor->Name), LongHint, [this, Data]()
      {
        ExecuteFile(FCurrentPanel->GetSide(), efExternalEditor, &Data, true);
      })->setEnabled(Enabled);
    }
  }
  Item(CurrentEditWithAction->text(), CurrentEditWithAction->toolTip(), [this]() { CurrentEditWithActionExecute(); })
    ->setEnabled(Enabled);
  Menu->addSeparator();
  Menu->addAction(EditorListCustomizeAction);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentEditWithActionExecute()
{
  // upstream's ExecuteCurrentFileWith
  if (!CurrentEditAction->isEnabled())
  {
    return;
  }
  TEditorData ExternalEditor;
  ExternalEditor.Editor = edExternal;
  bool Remember = false;
  if (DoEditorPreferencesDialog(&ExternalEditor, Remember, epmAdHoc, (FCurrentPanel == FRemotePanel)))
  {
    if (Remember)
    {
      std::unique_ptr<TEditorList> EditorList(new TEditorList());
      *EditorList = *AppConfiguration->EditorList;

      bool Found = false;
      int i = 0;
      while (!Found && (i < EditorList->Count))
      {
        const TEditorPreferences * Editor = EditorList->Editors[i];
        if ((Editor->Data->Editor == edExternal) &&
            (Editor->Data->ExternalEditor == ExternalEditor.ExternalEditor))
        {
          Found = true;
        }
        i++;
      }

      if (!Found)
      {
        EditorList->Add(new TEditorPreferences(ExternalEditor));
        AppConfiguration->EditorList = EditorList.get();
      }
    }

    ExecuteFile(FCurrentPanel->GetSide(), efExternalEditor, &ExternalEditor, true);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::EditorListCustomizeActionExecute()
{
  DoPreferencesDialog(pmEditor);
}
//---------------------------------------------------------------------------
