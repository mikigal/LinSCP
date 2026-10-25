// The custom commands of the main window (upstream has them in
// forms/CustomScpExplorer.cpp and forms/NonVisual.cpp): their menu and
// shortcuts, the ad hoc and the last command, and their execution: the
// remote ones in the shell of the session, the local ones on the local
// files, or on the remote files downloaded to a temporary directory (and
// uploaded back once changed).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <CustomCommands.h>
#include <ShortCuts.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ProgressForm.h"
#include "ScpCommander.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMenu>
#include <QTreeView>

#include <map>
//---------------------------------------------------------------------------
bool TScpCommanderForm::CustomCommandRemoteAllowed()
{
  // remote custom commands can be executed only if the server supports shell commands
  // or have secondary shell
  TTerminal * Terminal = GetTerminal();
  return
    (Terminal != NULL) && Terminal->Active &&
    (Terminal->IsCapable[fcSecondaryShell] || Terminal->IsCapable[fcShellAnyCommand]);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::AnyFileSelected(TDirPanel * APanel)
{
  return APanel->IsReady() && !APanel->GetSelectedRows().empty();
}
//---------------------------------------------------------------------------
int TScpCommanderForm::CustomCommandState(const TCustomCommandType & Command, TCustomCommandListType ListType)
{
  // -1 = hidden, 0 = disabled, 1 = enabled
  int Result;

  TFileCustomCommand RemoteCustomCommand;
  TLocalCustomCommand LocalCustomCommand;
  TFileCustomCommand * NonInteractiveCustomCommand =
    FLAGCLEAR(Command.Params, ccLocal) ? &RemoteCustomCommand : &LocalCustomCommand;
  TInteractiveCustomCommand InteractiveCustomCommand(NonInteractiveCustomCommand);
  UnicodeString Cmd = InteractiveCustomCommand.Complete(Command.Command, false);
  TTerminal * Terminal = GetTerminal();
  bool HasTerminal = (Terminal != NULL) && Terminal->Active;

  if (FLAGCLEAR(Command.Params, ccLocal))
  {
    int AllowedState = CustomCommandRemoteAllowed() ? 1 : 0;
    // custom command that does not operate with files can be executed anytime ...
    if (!NonInteractiveCustomCommand->IsFileCommand(Cmd))
    {
      // ... but do not show such command in remote file menu
      if ((ListType == ccltAll) || (ListType == ccltNonFile))
      {
        Result = AllowedState;
      }
      else
      {
        Result = -1;
      }
    }
    else
    {
      if ((ListType == ccltAll) || ((ListType == ccltFile) && (FCurrentPanel == FRemotePanel)))
      {
        Result = AnyFileSelected(FCurrentPanel) ? AllowedState : 0;
      }
      else
      {
        Result = -1;
      }
    }
  }
  else
  {
    // custom command that does not operate with files can be executed anytime
    if (!NonInteractiveCustomCommand->IsFileCommand(Cmd))
    {
      Result = ((ListType == ccltAll) || (ListType == ccltNonFile)) ? 1 : -1;
    }
    else if (LocalCustomCommand.HasLocalFileName(Cmd))
    {
      if ((ListType == ccltAll) || (ListType == ccltFile))
      {
        // special case is "diff"-style command that can be executed over any side,
        // if we have both sides
        Result = (HasTerminal && AnyFileSelected(FLocalPanel) && AnyFileSelected(FRemotePanel)) ? 1 : 0;
      }
      else if (ListType == ccltBoth)
      {
        Result = 1;
      }
      else
      {
        Result = -1;
      }
    }
    else
    {
      if ((ListType == ccltAll) ||
          ((ListType == ccltFile) && ((FCurrentPanel == FRemotePanel) || FLAGCLEAR(Command.Params, ccRemoteFiles))))
      {
        Result = AnyFileSelected(FCurrentPanel) ? 1 : 0;
        // The remote files are downloaded
        if ((Result > 0) && (FCurrentPanel == FRemotePanel) && !HasTerminal)
        {
          Result = 0;
        }
      }
      else
      {
        Result = -1;
      }
    }

    if ((Result > 0) &&
        LocalCustomCommand.IsSessionCommand(Cmd) &&
        !HasTerminal)
    {
      Result = 0;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalCaptureLog(const UnicodeString & AddedLine, TCaptureOutputType OutputType)
{
  DebugAssert(FCapturedLog != nullptr);
  if ((OutputType == cotOutput) || (OutputType == cotError))
  {
    FCapturedLog->Add(AddedLine);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RemoteCustomCommand(
  TStrings * FileList, const TCustomCommandType & ACommand,
  const TCustomCommandData & Data, const UnicodeString & CommandCommand)
{
  if (EnsureCommandSessionFallback(fcShellAnyCommand))
  {
    TTerminal * Terminal = GetTerminal();
    TRemoteCustomCommand RemoteCustomCommand(Data, Terminal->CurrentDirectory);
    TWinInteractiveCustomCommand InteractiveCustomCommand(&RemoteCustomCommand, ACommand.Name);

    UnicodeString Command = InteractiveCustomCommand.Complete(CommandCommand, false);

    Configuration->Usage->Inc(L"RemoteCustomCommandRuns2");

    bool Capture =
      FLAGSET(ACommand.Params, ccShowResults) ||
      FLAGSET(ACommand.Params, ccCopyResults) ||
      FLAGSET(ACommand.Params, ccShowResultsInMsgBox);
    TCaptureOutputEvent OutputEvent = NULL;

    DebugAssert(FCapturedLog == nullptr);
    ::rtl::TFinally Finally([&]() { FCapturedLog.reset(); });
    if (Capture)
    {
      FCapturedLog.reset(new TStringList());
      OutputEvent = ::rtl::MethodPtr(this, &TScpCommanderForm::TerminalCaptureLog);
    }

    if (!RemoteCustomCommand.IsFileCommand(Command))
    {
      Terminal->AnyCommand(RemoteCustomCommand.Complete(Command, true), OutputEvent);
    }
    else
    {
      Terminal->CustomCommandOnFiles(Command, ACommand.Params, FileList, OutputEvent);
    }

    if ((FCapturedLog != nullptr) && (FCapturedLog->Count > 0))
    {
      if (FLAGSET(ACommand.Params, ccCopyResults))
      {
        QApplication::clipboard()->setText(ToQString(FCapturedLog->Text));
      }

      if (FLAGSET(ACommand.Params, ccShowResults))
      {
        DoConsoleDialog(Terminal, L"", FCapturedLog.get());
      }
      else if (FLAGSET(ACommand.Params, ccShowResultsInMsgBox))
      {
        MessageDialog(FCapturedLog->Text, qtInformation, qaOK);
      }
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TScpCommanderForm::DefaultDownloadTargetDirectory()
{
  return FLocalPanel->GetPath();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LocalCustomCommandPure(
  TStrings * FileList, const TCustomCommandType & ACommand, const UnicodeString & Command, TStrings * ALocalFileList,
  const TCustomCommandData & Data, bool LocalFileCommand, bool FileListCommand, UnicodeString * POutput)
{
  TTerminal * Terminal = GetTerminal();
  std::unique_ptr<TStrings> LocalFileListOwned;
  TStrings * LocalFileList = NULL;
  std::unique_ptr<TStrings> RemoteFileListOwned;
  TStrings * RemoteFileList = NULL;
  std::unique_ptr<TStrings> RemoteFileListFullOwned;
  TStrings * RemoteFileListFull = NULL;

  if (LocalFileCommand)
  {
    if (ALocalFileList == NULL)
    {
      DebugAssert(AnyFileSelected(FLocalPanel));
      LocalFileListOwned.reset(CreateFileList(FLocalPanel));
      LocalFileList = LocalFileListOwned.get();
    }
    else
    {
      LocalFileList = ALocalFileList;
    }

    if (FileListCommand)
    {
      if (LocalFileList->Count != 1)
      {
        throw Exception(LoadStr(CUSTOM_COMMAND_SELECTED_UNMATCH1));
      }
    }
    else
    {
      if ((LocalFileList->Count != 1) &&
          (FileList->Count != 1) &&
          (LocalFileList->Count != FileList->Count))
      {
        throw Exception(LoadStr(CUSTOM_COMMAND_SELECTED_UNMATCH));
      }
    }
  }

  UnicodeString RootTempDir;
  UnicodeString TempDir;

  bool RemoteFiles = FLAGSET(ACommand.Params, ccRemoteFiles);
  if (!RemoteFiles)
  {
    TemporarilyDownloadFiles(FileList, false, RootTempDir, TempDir, true);
  }

  ::rtl::TFinally TempFinally([&]()
  {
    FAutoOperation = false;
    if (!RootTempDir.IsEmpty() && DebugAlwaysTrue(!RemoteFiles))
    {
      RecursiveDeleteFile(ExcludeTrailingBackslash(RootTempDir));
    }
  });

  TDateTimes RemoteFileTimes;

  if (RemoteFiles)
  {
    RemoteFileList = FileList;
  }
  else
  {
    RemoteFileListOwned.reset(new TStringList());
    RemoteFileList = RemoteFileListOwned.get();

    TMakeLocalFileListParams MakeFileListParam;
    MakeFileListParam.FileList = RemoteFileList;
    MakeFileListParam.FileTimes = &RemoteFileTimes;
    MakeFileListParam.IncludeDirs = FLAGSET(ACommand.Params, ccApplyToDirectories);
    MakeFileListParam.Recursive =
      FLAGSET(ACommand.Params, ccRecursive) && !FileListCommand;

    ProcessLocalDirectory(TempDir, ::rtl::MethodPtr(Terminal, &TTerminal::MakeLocalFileList), &MakeFileListParam);

    if (!MakeFileListParam.Recursive)
    {
      RemoteFileTimes.clear();

      RemoteFileListFullOwned.reset(new TStringList());
      RemoteFileListFull = RemoteFileListFullOwned.get();

      MakeFileListParam.FileList = RemoteFileListFull;
      MakeFileListParam.Recursive = true;
      MakeFileListParam.IncludeDirs = false;

      ProcessLocalDirectory(TempDir, ::rtl::MethodPtr(Terminal, &TTerminal::MakeLocalFileList), &MakeFileListParam);
    }
    else
    {
      RemoteFileListFull = RemoteFileList;
    }
  }

  bool NonBlocking = FileListCommand && RemoteFiles && !POutput;

  TFileOperationProgressType Progress(
    ::rtl::MethodPtr(this, &TScpCommanderForm::OperationProgress),
    ::rtl::MethodPtr(this, &TScpCommanderForm::OperationFinished));

  if (!NonBlocking)
  {
    int Count;
    if (FileListCommand)
    {
      Count = 1;
    }
    else if (LocalFileCommand)
    {
      Count = std::max(LocalFileList->Count, RemoteFileList->Count);
    }
    else
    {
      Count = RemoteFileList->Count;
    }
    Progress.Start(foCustomCommand, osRemote, Count);
  }

  {
    ::rtl::TFinally ProgressFinally([&]()
    {
      if (!NonBlocking)
      {
        Progress.Stop();
      }
    });
    if (FileListCommand)
    {
      UnicodeString LocalFile;
      // MakeFileList does not delimit filenames
      UnicodeString AFileList = MakeFileList(RemoteFileList);

      if (LocalFileCommand)
      {
        DebugAssert(LocalFileList->Count == 1);
        LocalFile = LocalFileList->Strings[0];
      }

      TLocalCustomCommand CustomCommand(Data,
        Terminal->CurrentDirectory, DefaultDownloadTargetDirectory(), L"", LocalFile, AFileList);
      UnicodeString ShellCommand = CustomCommand.Complete(Command, true);

      if (NonBlocking)
      {
        DebugAssert(!POutput);
        ExecuteShellChecked(ShellCommand);
      }
      else
      {
        ExecuteProcessCheckedAndWait(ShellCommand, POutput);
      }
    }
    else if (LocalFileCommand)
    {
      auto ExecuteOne = [&](const UnicodeString & FileName, const UnicodeString & LocalFileName,
        const UnicodeString & FileNameForProgress)
      {
        Progress.SetFile(FileNameForProgress);
        TLocalCustomCommand CustomCommand(
          Data, Terminal->CurrentDirectory, DefaultDownloadTargetDirectory(),
          FileName, LocalFileName, L"");
        ExecuteProcessCheckedAndWait(CustomCommand.Complete(Command, true), POutput);
        TOnceDoneOperation OnceDoneOperation;
        // Do not unselect anything, as it won't work with two panels anyway
        Progress.Finish(UnicodeString(), true, OnceDoneOperation);
      };

      if (LocalFileList->Count == 1)
      {
        UnicodeString LocalFile = LocalFileList->Strings[0];
        for (int Index = 0; Index < RemoteFileList->Count; Index++)
        {
          UnicodeString FileName = RemoteFileList->Strings[Index];
          ExecuteOne(FileName, LocalFile, RemoteFiles ? FileName : ExtractFileName(FileName));
        }
      }
      else if (RemoteFileList->Count == 1)
      {
        UnicodeString FileName = RemoteFileList->Strings[0];
        for (int Index = 0; Index < LocalFileList->Count; Index++)
        {
          UnicodeString LocalFileName = LocalFileList->Strings[Index];
          ExecuteOne(FileName, LocalFileName, ExtractFileName(LocalFileName));
        }
      }
      else
      {
        if (LocalFileList->Count != RemoteFileList->Count)
        {
          throw Exception(LoadStr(CUSTOM_COMMAND_PAIRS_DOWNLOAD_FAILED));
        }

        for (int Index = 0; Index < LocalFileList->Count; Index++)
        {
          UnicodeString LocalFileName = LocalFileList->Strings[Index];
          ExecuteOne(RemoteFileList->Strings[Index], LocalFileName, ExtractFileName(LocalFileName));
        }
      }
    }
    else
    {
      for (int Index = 0; Index < RemoteFileList->Count; Index++)
      {
        UnicodeString FileName = RemoteFileList->Strings[Index];
        UnicodeString FileNameForProgress = RemoteFiles ? FileName : ExtractFileName(FileName);
        Progress.SetFile(FileNameForProgress);
        TLocalCustomCommand CustomCommand(Data,
          Terminal->CurrentDirectory, DefaultDownloadTargetDirectory(),
          FileName, L"", L"");
        ExecuteProcessCheckedAndWait(CustomCommand.Complete(Command, true), POutput);
        TOnceDoneOperation OnceDoneOperation;
        Progress.Finish(FileNameForProgress, true, OnceDoneOperation);
      }
    }
  }

  DebugAssert(!FAutoOperation);

  if (!RemoteFiles)
  {
    // The changed files go back
    TempDir = IncludeTrailingBackslash(TempDir);
    std::map<UnicodeString, UnicodeString> TempNameMap;
    for (int Index = 0; Index < RemoteFileListFull->Count; Index++)
    {
      UnicodeString FileName = RemoteFileListFull->Strings[Index];
      TSearchRecSmart SearchRec;
      if (DebugAlwaysTrue(SameText(TempDir, FileName.SubString(1, TempDir.Length()))) &&
          FileSearchRec(FileName, SearchRec) &&
          // Skip directories, process only nested files.
          !SearchRec.IsDirectory() &&
          (SearchRec.GetLastWriteTime() != RemoteFileTimes[Index]))
      {
        TCopyParamType CopyParam = TemporaryFileCopyParam(false);
        if (TempNameMap.empty()) // optimization (create map only once it is really needed)
        {
          for (int Index2 = 0; Index2 < FileList->Count; Index2++)
          {
            // Particularly with SCP the FileList contains relative paths,
            // and we may end up with empty RemoteDir below.
            UnicodeString RemotePath = Terminal->AbsolutePath(FileList->Strings[Index2], true);
            UnicodeString TempName = Terminal->ChangeFileName(&CopyParam, UnixExtractFileName(RemotePath), osRemote, false);
            TempNameMap.insert(std::make_pair(TempName, RemotePath));
          }
        }

        UnicodeString RelPath = MidStr(FileName, TempDir.Length() + 1);
        UnicodeString TempName = CutToChar(RelPath, L'/', false);
        auto TempNameI = TempNameMap.find(TempName);
        if (DebugAlwaysTrue(TempNameI != TempNameMap.end()))
        {
          CopyParam.FileMask = L"";

          FAutoOperation = true;
          std::unique_ptr<TStrings> TemporaryFilesList(new TStringList());
          TemporaryFilesList->Add(FileName);

          UnicodeString RemoteDir = UnixExtractFileDir(UnixCombinePathsSmart(TempNameI->second, ToUnixPath(RelPath)));

          Terminal->CopyToRemote(TemporaryFilesList.get(), RemoteDir, &CopyParam, cpTemporary, NULL);
          FAutoOperation = false;
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LocalCustomCommandWithLocalFiles(
  const TCustomCommandType & ACommand, const UnicodeString & Command, const TCustomCommandData & Data,
  bool FileListCommand, UnicodeString * POutput)
{
  std::unique_ptr<TStrings> SelectedFileList(CreateFileList(FLocalPanel));

  std::unique_ptr<TStrings> LocalFileList(new TStringList());

  for (int Index = 0; Index < SelectedFileList->Count; Index++)
  {
    UnicodeString FileName = SelectedFileList->Strings[Index];
    if (DirectoryExists(ApiPath(FileName)))
    {
      if (FLAGSET(ACommand.Params, ccApplyToDirectories))
      {
        LocalFileList->Add(FileName);
      }

      if (FLAGSET(ACommand.Params, ccRecursive))
      {
        TMakeLocalFileListParams MakeFileListParam;
        MakeFileListParam.FileList = LocalFileList.get();
        MakeFileListParam.FileTimes = NULL;
        MakeFileListParam.IncludeDirs = FLAGSET(ACommand.Params, ccApplyToDirectories);
        MakeFileListParam.Recursive = true;

        // OpenSCP: upstream uses the method of the terminal, which needs no session
        TTerminal * Terminal = GetTerminal();
        if (Terminal != NULL)
        {
          ProcessLocalDirectory(FileName, ::rtl::MethodPtr(Terminal, &TTerminal::MakeLocalFileList), &MakeFileListParam);
        }
      }
    }
    else
    {
      LocalFileList->Add(FileName);
    }
  }

  UnicodeString RemotePath;
  TTerminal * Terminal = GetTerminal();
  if (Terminal != NULL)
  {
    RemotePath = Terminal->CurrentDirectory;
  }

  if (FileListCommand)
  {
    UnicodeString FileList = MakeFileList(LocalFileList.get());
    TLocalCustomCommand CustomCommand(
      Data, RemotePath, DefaultDownloadTargetDirectory(),
      L"", L"", FileList);
    ExecuteProcessChecked(CustomCommand.Complete(Command, true), POutput);
  }
  else
  {
    TFileOperationProgressType Progress(
      ::rtl::MethodPtr(this, &TScpCommanderForm::OperationProgress),
      ::rtl::MethodPtr(this, &TScpCommanderForm::OperationFinished));

    Progress.Start(foCustomCommand, osLocal, LocalFileList->Count);

    ::rtl::TFinally Finally([&]() { Progress.Stop(); });
    for (int Index = 0; Index < LocalFileList->Count; Index++)
    {
      UnicodeString FileName = LocalFileList->Strings[Index];
      Progress.SetFile(FileName);
      TLocalCustomCommand CustomCommand(
        Data, RemotePath, DefaultDownloadTargetDirectory(),
        FileName, L"", L"");
      ExecuteProcessCheckedAndWait(CustomCommand.Complete(Command, true), POutput);
      TOnceDoneOperation OnceDoneOperation;
      Progress.Finish(FileName, true, OnceDoneOperation);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LocalCustomCommand(TStrings * FileList,
  const TCustomCommandType & ACommand, TStrings * ALocalFileList,
  const TCustomCommandData & Data, const UnicodeString & CommandCommand)
{
  TTerminal * Terminal = GetTerminal();
  UnicodeString RemotePath = (Terminal != NULL) ? Terminal->CurrentDirectory : UnicodeString();
  TLocalCustomCommand LocalCustomCommand(Data, RemotePath, DefaultDownloadTargetDirectory());
  TWinInteractiveCustomCommand InteractiveCustomCommand(&LocalCustomCommand, ACommand.Name);

  UnicodeString Command = InteractiveCustomCommand.Complete(CommandCommand, false);

  bool FileListCommand = LocalCustomCommand.IsFileListCommand(Command);
  bool LocalFileCommand = LocalCustomCommand.HasLocalFileName(Command);

  Configuration->Usage->Inc(L"LocalCustomCommandRuns2");

  std::unique_ptr<UnicodeString> POutput;
  if (FLAGSET(ACommand.Params, ccShowResultsInMsgBox) ||
      FLAGSET(ACommand.Params, ccCopyResults))
  {
    POutput.reset(new UnicodeString());
  }

  if (!LocalCustomCommand.IsFileCommand(Command))
  {
    ExecuteProcessChecked(LocalCustomCommand.Complete(Command, true), POutput.get());
  }
  // remote files?
  else if ((FCurrentPanel == FRemotePanel) || LocalFileCommand)
  {
    LocalCustomCommandPure(FileList, ACommand, Command, ALocalFileList, Data, LocalFileCommand, FileListCommand, POutput.get());
  }
  // local files
  else
  {
    LocalCustomCommandWithLocalFiles(ACommand, Command, Data, FileListCommand, POutput.get());
  }

  if (POutput != nullptr)
  {
    // If the output is single-line, we do not want the trailing line break
    if (EndsStr(L"\n", *POutput) && (POutput->Pos(L"\n") == POutput->Length()))
    {
      POutput->SetLength(POutput->Length() - 1);
    }
    // Copy even empty output to clipboard
    if (FLAGSET(ACommand.Params, ccCopyResults))
    {
      QApplication::clipboard()->setText(ToQString(*POutput));
    }
    // But do not show an empty message box.
    // This way the ShowResultsInMsgBox can be used to suppress a console window of commands with no output
    if (FLAGSET(ACommand.Params, ccShowResultsInMsgBox) &&
        !POutput->IsEmpty())
    {
      MessageDialog(*POutput, qtInformation, qaOK);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CustomCommand(TStrings * FileList,
  const TCustomCommandType & ACommand, TStrings * ALocalFileList)
{
  TCustomCommandData Data;
  TTerminal * Terminal = GetTerminal();
  if ((Terminal != NULL) && Terminal->Active)
  {
    Data = TCustomCommandData(Terminal);
  }

  // Upstream also expands the options of the extensions here
  UnicodeString CommandCommand = ACommand.Command;

  if (FLAGCLEAR(ACommand.Params, ccLocal))
  {
    RemoteCustomCommand(FileList, ACommand, Data, CommandCommand);
  }
  else
  {
    LocalCustomCommand(FileList, ACommand, ALocalFileList, Data, CommandCommand);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteCustomCommand(const TCustomCommandType & Command)
{
  // upstream's ExecuteFileOperation(foCustomCommand): the files of the remote
  // panel (where there is a session), relative to its directory
  if (IsBusy())
  {
    return;
  }
  std::unique_ptr<TStrings> FileList;
  if (FRemotePanel->IsReady())
  {
    FileList.reset(CreateFileList(FRemotePanel, false));
  }
  else
  {
    FileList.reset(new TStringList());
  }

  ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
  TAutoNestingCounter BusyCounter(FBusy);
  UpdateActions();
  ::rtl::TFinally Finally([&]()
  {
    DestroyProgressForm();
    FLocalPanel->Reload();
  });
  CustomCommand(FileList.get(), Command, NULL);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::AdHocCustomCommand()
{
  if (IsBusy())
  {
    return;
  }
  bool RemoteAllowed = CustomCommandRemoteAllowed();
  TCustomCommandType Command;
  // make sure we use local custom command when remote are not supported
  if (RemoteAllowed || FLAGSET(FLastCustomCommand.Params, ccLocal))
  {
    Command = FLastCustomCommand;
  }
  else
  {
    Command.Params = Command.Params | ccLocal;
  }
  Command.Name = LoadStr(CUSTOM_COMMAND_AD_HOC_NAME);
  bool LocalSide = (FCurrentPanel == FLocalPanel);
  int Options =
    FLAGMASK((!RemoteAllowed || LocalSide), ccoDisableRemote) |
    FLAGMASK(LocalSide, ccoDisableRemoteFiles);
  if (DoCustomCommandDialog(Command, AppConfiguration->CustomCommandList, ccmAdHoc, Options,
        [this](const TCustomCommandType & ACommand)
        {
          if (CustomCommandState(ACommand, ccltAll) <= 0)
          {
            throw Exception(FMTLOAD(CUSTOM_COMMAND_IMPOSSIBLE2, (ACommand.Command)));
          }
        },
        NULL))
  {
    FLastCustomCommand = Command;
    ExecuteCustomCommand(Command);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LastCustomCommand()
{
  if (FLastCustomCommand.Command.IsEmpty() || IsBusy())
  {
    return;
  }

  int State = CustomCommandState(FLastCustomCommand, ccltAll);
  if (State <= 0)
  {
    throw Exception(FMTLOAD(CUSTOM_COMMAND_IMPOSSIBLE2, (FLastCustomCommand.Command)));
  }

  TCustomCommandType Command = FLastCustomCommand;
  ExecuteCustomCommand(Command);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CustomCommandsCustomize()
{
  if (DoCustomCommandsDialog())
  {
    UpdateCustomCommandShortCuts();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateCustomCommandsMenu(QMenu * Menu)
{
  // upstream's CreateCustomCommandsMenu, for the commands menu (ccltAll)
  TCustomCommandList * List = AppConfiguration->CustomCommandList;
  for (int Index = 0; Index < List->Count; Index++)
  {
    const TCustomCommandType * Command = List->Commands[Index];
    int State = CustomCommandState(*Command, ccltAll);
    if (State >= 0)
    {
      QAction * Item = Menu->addAction(ToQString(Command->Name));
      Item->setObjectName(QStringLiteral("CustomCommand%1").arg(Index));
      Item->setEnabled(!IsBusy() && (State > 0));
      Item->setShortcut(ShortCutToKeySequence(Command->ShortCut));
      // Shown only, the window handles the shortcuts itself
      Item->setShortcutContext(Qt::WidgetShortcut);
      Item->setToolTip(ToQString(FMTLOAD(CUSTOM_COMMAND_HINT_LONG, (StripHotkey(Command->Name), Command->Command))));
      UnicodeString Name = Command->Name;
      connect(Item, &QAction::triggered, this, [this, Name]()
      {
        ExecuteHandler([this, Name]()
        {
          const TCustomCommandType * ACommand = AppConfiguration->CustomCommandList->Find(Name);
          if (ACommand != NULL)
          {
            TCustomCommandType CommandCopy = *ACommand;
            ExecuteCustomCommand(CommandCopy);
          }
        });
      });
    }
  }
  Menu->setToolTipsVisible(true);

  Menu->addAction(CustomCommandsEnterAction);
  CustomCommandsEnterAction->setEnabled(!IsBusy());

  // upstream's CustomCommandsLastUpdate
  bool Defined = !FLastCustomCommand.Command.IsEmpty();
  CustomCommandsLastAction->setVisible(Defined);
  if (Defined)
  {
    UnicodeString TitleCommand = FLastCustomCommand.Command;
    int MaxTitleCommandLen = 20;
    if (TitleCommand.Length() > MaxTitleCommandLen)
    {
      TitleCommand = TitleCommand.SubString(1, MaxTitleCommandLen - 3) + Ellipsis;
    }
    CustomCommandsLastAction->setText(ToQString(FMTLOAD(CUSTOM_COMMAND_LAST, (EscapeHotkey(TitleCommand)))));
    CustomCommandsLastAction->setToolTip(ToQString(FLastCustomCommand.Command));
    CustomCommandsLastAction->setEnabled(!IsBusy() && (CustomCommandState(FLastCustomCommand, ccltAll) > 0));
  }
  Menu->addAction(CustomCommandsLastAction);

  Menu->addSeparator();
  Menu->addAction(CustomCommandsCustomizeAction);
  CustomCommandsCustomizeAction->setEnabled(!IsBusy());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateCustomCommandShortCuts()
{
  // upstream's CheckCustomCommandShortCut in KeyDown
  qDeleteAll(FCustomCommandShortCutActions);
  FCustomCommandShortCutActions.clear();
  TCustomCommandList * List = AppConfiguration->CustomCommandList;
  for (int Index = 0; Index < List->Count; Index++)
  {
    const TCustomCommandType * Command = List->Commands[Index];
    QKeySequence Sequence = ShortCutToKeySequence(Command->ShortCut);
    if (IsCustomShortCut(Command->ShortCut) && !Sequence.isEmpty())
    {
      QAction * Action = new QAction(this);
      Action->setObjectName(QStringLiteral("CustomCommandShortCutAction"));
      Action->setShortcut(Sequence);
      addAction(Action);
      UnicodeString Name = Command->Name;
      connect(Action, &QAction::triggered, this, [this, Name]()
      {
        ExecuteHandler([this, Name]()
        {
          const TCustomCommandType * ACommand = AppConfiguration->CustomCommandList->Find(Name);
          if ((ACommand != NULL) && !IsBusy() && (CustomCommandState(*ACommand, ccltAll) > 0))
          {
            TCustomCommandType CommandCopy = *ACommand;
            ExecuteCustomCommand(CommandCopy);
          }
        });
      });
      FCustomCommandShortCutActions.push_back(Action);
    }
  }
}
//---------------------------------------------------------------------------
