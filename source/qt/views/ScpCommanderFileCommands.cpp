// The commands of the files of the main window: the links (upstream's
// AddEditLink), the locks, the sizes of the directories and the popup menus
// of the panels (upstream's RemoteFilePopup, LocalFilePopup and their
// DirViewPopup).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <FileSystems.h>
#include <LocalFiles.h>
#include <RemoteFiles.h>
#include <Terminal.h>
#include <TerminalManager.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QAction>
#include <QMenu>
#include <QTreeView>
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateFileCommandActions()
{
  NewLinkAction = CreateAction("NewLinkAction", tr("&Link..."), QStringLiteral("insert-link"),
    {}, &TScpCommanderForm::NewLinkActionExecute);
  NewLinkAction->setToolTip(tr("Create new link/shortcut"));
  CurrentAddEditLinkAction = CreateAction("CurrentAddEditLinkAction", tr("Edit &Link..."), QStringLiteral("insert-link"),
    {}, &TScpCommanderForm::CurrentAddEditLinkActionExecute);
  CurrentAddEditLinkAction->setToolTip(tr("Add new link/shortcut or edit selected link/shortcut"));
  LockAction = CreateAction("LockAction", tr("&Lock"), QStringLiteral("object-locked"),
    {}, &TScpCommanderForm::LockActionExecute);
  LockAction->setToolTip(tr("Lock selected file(s)"));
  UnlockAction = CreateAction("UnlockAction", tr("&Unlock"), QStringLiteral("object-unlocked"),
    {}, &TScpCommanderForm::UnlockActionExecute);
  UnlockAction->setToolTip(tr("Unlock selected file(s)"));
  CalculateDirectorySizesAction = CreateAction("CalculateDirectorySizesAction", tr("&Calculate Directory Sizes"), QString(),
    { QKeySequence(Qt::ALT | Qt::SHIFT | Qt::Key_Return) }, &TScpCommanderForm::CalculateDirectorySizes);
  CalculateDirectorySizesAction->setToolTip(tr("Calculate sizes of the selected directories and display them in the file panel"));

  FLockingMenu = new QMenu(tr("&Locking"), this);
  FLockingMenu->setObjectName(QStringLiteral("LockingMenu"));
  FLockingMenu->setToolTip(tr("Manage file locks"));
  FLockingMenu->addAction(LockAction);
  FLockingMenu->addAction(UnlockAction);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateFileCommandActions()
{
  bool Busy = IsBusy();
  bool CurrentReady = FCurrentPanel->IsReady();
  bool HasSelection = CurrentReady && !FCurrentPanel->GetSelectedRows().empty();
  bool CanLink = !Busy && CurrentReady && CanAddEditLink();
  NewLinkAction->setEnabled(CanLink);
  CurrentAddEditLinkAction->setEnabled(CanLink);
  TTerminal * Terminal = GetTerminal();
  bool CanLock =
    !Busy && HasSelection && (FCurrentPanel == FRemotePanel) && (Terminal != NULL) && Terminal->Active &&
    Terminal->IsCapable[fcLocking];
  LockAction->setEnabled(CanLock);
  UnlockAction->setEnabled(CanLock);
  FLockingMenu->setEnabled(CanLock);
  CalculateDirectorySizesAction->setEnabled(!Busy && HasSelection);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanAddEditLink()
{
  // Symbolic links of the local file system (upstream's shortcuts)
  TTerminal * Terminal = GetTerminal();
  return
    (FCurrentPanel == FLocalPanel) ||
    ((FManagedSession != NULL) && TTerminalManager::Instance()->IsAvailableTerminal(FManagedSession) &&
     Terminal->ResolvingSymlinks && Terminal->IsCapable[fcSymbolicLink]);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::LinkFocused()
{
  QModelIndex Current = FCurrentPanel->GetDirView()->currentIndex();
  bool Result = FCurrentPanel->IsReady() && Current.isValid();
  if (Result)
  {
    const TDirViewItem & Item = FCurrentPanel->GetModel()->Item(Current.row());
    Result =
      !Item.IsParentDirectory && Item.IsSymLink &&
      ((FCurrentPanel == FLocalPanel) || GetTerminal()->SessionData->ResolveSymlinks);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::AddEditLink(bool Add)
{
  TDirPanel * Panel = FCurrentPanel;
  if (IsBusy() || !Panel->IsReady() || !CanAddEditLink())
  {
    return;
  }
  bool Local = (Panel == FLocalPanel);
  TTerminal * Terminal = GetTerminal();
  bool Edit = false;
  UnicodeString FileName;
  UnicodeString PointTo;
  bool SymbolicLink = true;
  // A copy, the panel can be reloaded meanwhile
  std::unique_ptr<TRemoteFile> File;

  QModelIndex Current = Panel->GetDirView()->currentIndex();
  if (Current.isValid())
  {
    const TDirViewItem & Item = Panel->GetModel()->Item(Current.row());
    Edit = !Add && LinkFocused();
    if (Edit)
    {
      FileName = Item.FileName;
      if (Local)
      {
        PointTo = ToUnicodeString(Item.LinkTarget);
      }
      else
      {
        File.reset(Item.File->Duplicate());
        PointTo = File->LinkTo;
      }
    }
    else if (!Item.IsParentDirectory)
    {
      PointTo = Item.FileName;
    }
  }

  // Local hard links too (upstream's local links are shortcuts)
  bool AllowHardLink = Local || Terminal->IsCapable[fcHardLink];
  if (DoSymlinkDialog(FileName, PointTo, SymbolicLink, Edit, AllowHardLink))
  {
    TAutoNestingCounter BusyCounter(FBusy);
    if (Local)
    {
      Configuration->Usage->Inc(L"LocalShortcutsCreated");
      UnicodeString Path = FileName;
      if (Path[1] != L'/')
      {
        Path = IncludeTrailingBackslash(Panel->GetPath()) + Path;
      }
      CreateLocalLink(Path, PointTo, SymbolicLink, Edit);
      Panel->Reload();
      if (ExcludeTrailingBackslash(ExtractFilePath(Path)) == ExcludeTrailingBackslash(Panel->GetPath()))
      {
        Panel->FocusFile(ExtractFileName(Path));
      }
    }
    else
    {
      Configuration->Usage->Inc(L"RemoteLinksCreated");
      if (Edit)
      {
        DebugAssert(File->FileName == FileName);
        int Params = dfNoRecursive;
        Terminal->ExceptionOnFail = true;
        ::rtl::TFinally Finally([&]() { Terminal->ExceptionOnFail = false; });
        Terminal->DeleteFile(L"", File.get(), &Params);
      }
      Terminal->CreateLink(FileName, PointTo, SymbolicLink);
      Panel->FocusFile(UnixExtractFileName(UnixExcludeTrailingBackslash(FileName)));
    }
  }
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LockFiles(bool Lock)
{
  TTerminal * Terminal = GetTerminal();
  if (IsBusy() || (FCurrentPanel != FRemotePanel) || !FRemotePanel->IsReady() || !Terminal->IsCapable[fcLocking])
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(FRemotePanel));
  if (FileList->Count > 0)
  {
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    FRemotePanel->SaveSelectedNames();
    if (Lock)
    {
      Terminal->LockFiles(FileList.get());
    }
    else
    {
      Terminal->UnlockFiles(FileList.get());
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CalculateDirectorySizes()
{
  TDirPanel * Panel = FCurrentPanel;
  if (IsBusy() || !Panel->IsReady())
  {
    return;
  }
  // The selected directories
  std::vector<UnicodeString> Names;
  std::unique_ptr<TStringList> FileList(new TStringList());
  for (int Row : Panel->GetSelectedRows())
  {
    const TDirViewItem & Item = Panel->GetModel()->Item(Row);
    if (Item.IsDirectory)
    {
      Names.push_back(Item.FileName);
      if (Panel == FLocalPanel)
      {
        FileList->Add(IncludeTrailingBackslash(Panel->GetPath()) + Item.FileName);
      }
      else
      {
        FileList->AddObject(Item.File->FullFileName, Item.File);
      }
    }
  }
  if (Names.empty())
  {
    return;
  }

  ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
  TAutoNestingCounter BusyCounter(FBusy);
  TCalculatedSizes CalculatedSizes;
  if (Panel == FLocalPanel)
  {
    TInstantOperationVisualizer Visualizer;
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      std::unique_ptr<TStringList> OneFile(new TStringList());
      OneFile->Add(FileList->Strings[Index]);
      __int64 Size = 0;
      TCalculateSizeStats Stats;
      CalculateLocalFilesSize(OneFile.get(), Size, Stats);
      CalculatedSizes.push_back(Size);
    }
  }
  else
  {
    // The remote directory may be reloaded during the operation
    std::unique_ptr<TStrings> RemoteFileList(TRemoteFileList::CloneStrings(FileList.get()));
    TCalculateSizeStats Stats;
    Stats.CalculatedSizes = &CalculatedSizes;
    TCalculateSizeParams Params;
    Params.Stats = &Stats;
    Params.UseCache = true;
    __int64 Size = 0;
    GetTerminal()->CalculateFilesSize(RemoteFileList.get(), Size, Params);
  }
  // The sizes of the directories done (all unless cancelled)
  for (size_t Index = 0; (Index < CalculatedSizes.size()) && (Index < Names.size()); Index++)
  {
    Panel->SetCalculatedSize(Names[Index], CalculatedSizes[Index]);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PanelContextPopup(TDirPanel * Panel, bool OnFile, const QPoint & GlobalPos)
{
  if (IsBusy())
  {
    return;
  }
  Panel->GetDirView()->setFocus();
  SetCurrentPanel(Panel);
  UpdateActions();

  bool Remote = (Panel == FRemotePanel);
  QMenu Menu(this);
  if (OnFile)
  {
    Menu.setObjectName(Remote ? QStringLiteral("RemoteFilePopup") : QStringLiteral("LocalFilePopup"));
    // upstream's CurrentAddEditLinkContextAction
    if (LinkFocused())
    {
      Menu.addAction(CurrentAddEditLinkAction);
    }
    Menu.addAction(CurrentOpenAction);
    Menu.addAction(CurrentEditAction);
    Menu.addAction(CurrentCopyAction);
    Menu.addAction(CurrentMoveAction);
    if (Remote)
    {
      Menu.addAction(RemoteCopyToAction);
      Menu.addAction(RemoteMoveToAction);
    }
    Menu.addAction(CurrentDeleteAction);
    Menu.addAction(CurrentRenameAction);
    Menu.addSeparator();
    Menu.addAction(CurrentCopyToClipboardAction2);
    Menu.addSeparator();
    QMenu * CustomCommandsMenu = Menu.addMenu(tr("&Custom Commands"));
    CustomCommandsMenu->setObjectName(Remote ? QStringLiteral("RemoteFilePopupCustomCommandsMenu") : QStringLiteral("LocalFilePopupCustomCommandsMenu"));
    CreateCustomCommandsMenu(CustomCommandsMenu);
    CustomCommandsMenu->setEnabled(!CustomCommandsMenu->isEmpty());
    QMenu * FileNamesMenu = Menu.addMenu(tr("&File Names"));
    FileNamesMenu->addAction(FileListToClipboardAction);
    FileNamesMenu->addAction(FullFileListToClipboardAction);
    if (Remote)
    {
      FileNamesMenu->addAction(FileGenerateUrlAction2);
    }
    Menu.addSeparator();
    Menu.addAction(CurrentPropertiesAction);
  }
  else
  {
    Menu.setObjectName(Remote ? QStringLiteral("RemoteDirViewPopup") : QStringLiteral("LocalDirViewPopup"));
    Panel->FillDirViewPopup(&Menu, Remote ? RemotePathToClipboardAction2 : LocalPathToClipboardAction2);
    Menu.addSeparator();
    QMenu * NewMenu = Menu.addMenu(tr("&New"));
    NewMenu->addAction(NewFileAction);
    NewMenu->addAction(CurrentCreateDirAction);
    NewMenu->addAction(NewLinkAction);
    Menu.addAction(PasteAction3);
    if (Remote)
    {
      QMenu * CustomCommandsMenu = Menu.addMenu(tr("&Custom Commands"));
      CustomCommandsMenu->setObjectName(QStringLiteral("RemoteDirViewPopupCustomCommandsMenu"));
      CreateCustomCommandsMenu(CustomCommandsMenu);
      CustomCommandsMenu->setEnabled(!CustomCommandsMenu->isEmpty());
    }
  }
  Menu.exec(GlobalPos);
}
//---------------------------------------------------------------------------
