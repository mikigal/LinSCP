// The main window with the Commander interface.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <FileSystems.h>
#include <TerminalManager.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ProgressForm.h"
#include "ScpCommander.h"

#include <QAction>
#include <QCloseEvent>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QTreeView>
//---------------------------------------------------------------------------
// Upstream's DeleteToRecycleBin preference (on by default): Delete moves
// local files to the trash, Shift+Delete deletes them
static const bool DeleteToRecycleBin = true;
//---------------------------------------------------------------------------
TScpCommanderForm::TScpCommanderForm(QWidget * Parent) :
  QMainWindow(Parent)
{
  setObjectName(QStringLiteral("ScpCommanderForm"));
  resize(1100, 720);
  FProgressForm = nullptr;
  FBusy = 0;
  FLocalDirectoryTerminal = NULL;

  FPanelSplitter = new QSplitter(Qt::Horizontal, this);
  FPanelSplitter->setObjectName(QStringLiteral("PanelSplitter"));
  FLocalPanel = new TLocalDirPanel(FPanelSplitter);
  FRemotePanel = new TRemoteDirPanel(FPanelSplitter);
  FPanelSplitter->addWidget(FLocalPanel);
  FPanelSplitter->addWidget(FRemotePanel);
  setCentralWidget(FPanelSplitter);
  FCurrentPanel = FLocalPanel;

  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    connect(Panel, &TDirPanel::Entered, this, [this](TDirPanel * APanel) { SetCurrentPanel(APanel); });
    Panel->OnRename = [this](TDirPanel * APanel, const UnicodeString & FileName, const UnicodeString & NewName)
    {
      RenameFile(APanel, FileName, NewName);
    };
    QTreeView * DirView = Panel->GetDirView();
    DirView->installEventFilter(this);
    connect(DirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() { UpdateActions(); });
    connect(DirView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this]() { UpdateActions(); });
    connect(DirView->model(), &QAbstractItemModel::modelReset, this, [this]() { UpdateActions(); });
  }
  connect(FRemotePanel, &TRemoteDirPanel::NewSessionRequested, this, [this]() { NewSession(); });

  NewSessionAction = CreateAction("NewSessionAction", tr("&New Session..."), QStringLiteral("network-connect"),
    { QKeySequence(Qt::CTRL | Qt::Key_N) }, &TScpCommanderForm::NewSessionActionExecute);
  DisconnectSessionAction = CreateAction("DisconnectSessionAction", tr("&Disconnect Session"), QStringLiteral("network-disconnect"),
    {}, &TScpCommanderForm::DisconnectSessionActionExecute);
  CloseApplicationAction = CreateAction("CloseApplicationAction", tr("&Quit"), QStringLiteral("application-exit"),
    { QKeySequence(Qt::Key_F10), QKeySequence(Qt::CTRL | Qt::Key_Q) }, &TScpCommanderForm::CloseApplicationActionExecute);
  CurrentCopyAction = CreateAction("CurrentCopyAction", tr("&Copy..."), QStringLiteral("edit-copy"),
    { QKeySequence(Qt::Key_F5) }, &TScpCommanderForm::CurrentCopyActionExecute);
  CurrentRenameAction = CreateAction("CurrentRenameAction", tr("&Rename"), QStringLiteral("edit-rename"),
    { QKeySequence(Qt::Key_F2) }, &TScpCommanderForm::CurrentRenameActionExecute);
  CurrentDeleteAction = CreateAction("CurrentDeleteAction", tr("&Delete"), QStringLiteral("edit-delete"),
    { QKeySequence(Qt::Key_F8), QKeySequence(Qt::Key_Delete) }, &TScpCommanderForm::CurrentDeleteActionExecute);
  CurrentDeleteAlternativeAction =
    CreateAction("CurrentDeleteAlternativeAction", tr("Delete &Permanently"), QStringLiteral("edit-delete-shred"),
      { QKeySequence(Qt::SHIFT | Qt::Key_F8), QKeySequence(Qt::SHIFT | Qt::Key_Delete) },
      &TScpCommanderForm::CurrentDeleteAlternativeActionExecute);
  CurrentCreateDirAction = CreateAction("CurrentCreateDirAction", tr("&Create Directory..."), QStringLiteral("folder-new"),
    { QKeySequence(Qt::Key_F7) }, &TScpCommanderForm::CurrentCreateDirActionExecute);
  ShowHiddenFilesAction = CreateAction("ShowHiddenFilesAction", tr("Show/Hide &Hidden Files"), QString(),
    { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_H) }, &TScpCommanderForm::ShowHiddenFilesActionExecute);
  ShowHiddenFilesAction->setCheckable(true);
  ShowHiddenFilesAction->setChecked(AppConfiguration->ShowHiddenFiles);

  CreateMenus();
  CreateToolbars();

  FSessionStatusLabel = new QLabel(statusBar());
  FSessionStatusLabel->setObjectName(QStringLiteral("SessionStatusLabel"));
  statusBar()->addPermanentWidget(FSessionStatusLabel);

  FLocalPanel->SetShowHiddenFiles(AppConfiguration->ShowHiddenFiles);
  FRemotePanel->SetShowHiddenFiles(AppConfiguration->ShowHiddenFiles);
  FLocalPanel->ChangeDirectory(ToUnicodeString(QDir::homePath()));

  TerminalChanged();
  FLocalPanel->GetDirView()->setFocus();
}
//---------------------------------------------------------------------------
TScpCommanderForm::~TScpCommanderForm()
{
  DestroyProgressForm();
}
//---------------------------------------------------------------------------
QAction * TScpCommanderForm::CreateAction(const char * Name, const QString & Caption, const QString & IconName,
  const QList<QKeySequence> & ShortCuts, void (TScpCommanderForm::*Method)())
{
  QAction * Action = new QAction(Caption, this);
  Action->setObjectName(QString::fromLatin1(Name));
  if (!IconName.isEmpty())
  {
    Action->setIcon(QIcon::fromTheme(IconName));
  }
  Action->setShortcuts(ShortCuts);
  addAction(Action);
  connect(Action, &QAction::triggered, this, [this, Method]() { (this->*Method)(); });
  return Action;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateMenus()
{
  QMenuBar * Menu = menuBar();
  Menu->setObjectName(QStringLiteral("MenuToolbar"));

  QMenu * LocalMenu = Menu->addMenu(tr("&Local"));
  LocalMenu->setObjectName(QStringLiteral("LocalMenuButton"));
  LocalMenu->addActions(FLocalPanel->actions());

  QMenu * FilesMenu = Menu->addMenu(tr("&Files"));
  FilesMenu->setObjectName(QStringLiteral("FilesMenu"));
  FilesMenu->addAction(CurrentCopyAction);
  FilesMenu->addAction(CurrentRenameAction);
  FilesMenu->addAction(CurrentDeleteAction);
  FilesMenu->addAction(CurrentDeleteAlternativeAction);

  QMenu * CommandsMenu = Menu->addMenu(tr("&Commands"));
  CommandsMenu->setObjectName(QStringLiteral("CommandsMenu"));
  CommandsMenu->addAction(CurrentCreateDirAction);
  CommandsMenu->addSeparator();
  CommandsMenu->addAction(CloseApplicationAction);

  QMenu * SessionMenu = Menu->addMenu(tr("&Session"));
  SessionMenu->setObjectName(QStringLiteral("SessionMenu"));
  SessionMenu->addAction(NewSessionAction);
  SessionMenu->addAction(DisconnectSessionAction);

  QMenu * OptionsMenu = Menu->addMenu(tr("&Options"));
  OptionsMenu->setObjectName(QStringLiteral("OptionsMenu"));
  OptionsMenu->addAction(ShowHiddenFilesAction);

  QMenu * RemoteMenu = Menu->addMenu(tr("&Remote"));
  RemoteMenu->setObjectName(QStringLiteral("RemoteMenuButton"));
  RemoteMenu->addActions(FRemotePanel->actions());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateToolbars()
{
  QToolBar * Toolbar = addToolBar(tr("Commands"));
  Toolbar->setObjectName(QStringLiteral("CommandsToolbar"));
  Toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  Toolbar->addAction(NewSessionAction);
  Toolbar->addAction(DisconnectSessionAction);
  Toolbar->addSeparator();
  Toolbar->addAction(CurrentCopyAction);
  Toolbar->addAction(CurrentRenameAction);
  Toolbar->addAction(CurrentDeleteAction);
  Toolbar->addAction(CurrentCreateDirAction);

  // The keys of the commands, at the bottom, as in upstream's Commander
  QToolBar * Hotkeys = new QToolBar(tr("Hotkeys"), this);
  Hotkeys->setObjectName(QStringLiteral("HotkeysToolbar"));
  Hotkeys->setToolButtonStyle(Qt::ToolButtonTextOnly);
  addToolBar(Qt::BottomToolBarArea, Hotkeys);
  for (QAction * Action : { CurrentRenameAction, CurrentCopyAction, CurrentCreateDirAction, CurrentDeleteAction, CloseApplicationAction })
  {
    QString Caption = Action->text().remove(QLatin1Char('&')).remove(QStringLiteral("..."));
    QAction * Proxy = Hotkeys->addAction(Action->shortcut().toString(QKeySequence::NativeText) + QLatin1Char(' ') + Caption);
    Proxy->setObjectName(Action->objectName() + QStringLiteral("Hotkey"));
    connect(Proxy, &QAction::triggered, Action, &QAction::trigger);
    connect(Action, &QAction::enabledChanged, Proxy, &QAction::setEnabled);
  }
}
//---------------------------------------------------------------------------
TTerminal * TScpCommanderForm::GetTerminal() const
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  return (Manager != NULL) ? Manager->ActiveTerminal : NULL;
}
//---------------------------------------------------------------------------
TDirPanel * TScpCommanderForm::OtherPanel(TDirPanel * Panel) const
{
  return (Panel == FLocalPanel) ? static_cast<TDirPanel *>(FRemotePanel) : static_cast<TDirPanel *>(FLocalPanel);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SetCurrentPanel(TDirPanel * Panel)
{
  if (FCurrentPanel != Panel)
  {
    FCurrentPanel = Panel;
    UpdateActions();
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::eventFilter(QObject * Object, QEvent * Event)
{
  // Tab switches the panels
  if ((Event->type() == QEvent::KeyPress) &&
      ((Object == FLocalPanel->GetDirView()) || (Object == FRemotePanel->GetDirView())))
  {
    QKeyEvent * KeyEvent = static_cast<QKeyEvent *>(Event);
    if ((KeyEvent->key() == Qt::Key_Tab) && (KeyEvent->modifiers() == Qt::NoModifier))
    {
      TDirPanel * Other =
        (Object == FLocalPanel->GetDirView()) ? static_cast<TDirPanel *>(FRemotePanel) : static_cast<TDirPanel *>(FLocalPanel);
      if (Other->IsReady())
      {
        Other->GetDirView()->setFocus();
      }
      return true;
    }
  }
  return QMainWindow::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateActions()
{
  bool Busy = IsBusy();
  TTerminal * Terminal = GetTerminal();
  bool Connected = (Terminal != NULL) && Terminal->Active;
  bool CurrentReady = FCurrentPanel->IsReady();
  bool HasSelection = CurrentReady && !FCurrentPanel->GetSelectedRows().empty();

  NewSessionAction->setEnabled(!Busy);
  DisconnectSessionAction->setEnabled(!Busy && (Terminal != NULL));
  CurrentCopyAction->setEnabled(!Busy && HasSelection && Connected && OtherPanel(FCurrentPanel)->IsReady());
  CurrentRenameAction->setEnabled(!Busy && HasSelection);
  CurrentDeleteAction->setEnabled(!Busy && HasSelection);
  CurrentDeleteAlternativeAction->setEnabled(!Busy && HasSelection);
  CurrentCreateDirAction->setEnabled(!Busy && CurrentReady);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateCaption()
{
  TTerminal * Terminal = GetTerminal();
  UnicodeString SessionName = ((Terminal != NULL) && Terminal->Active) ? Terminal->SessionData->SessionName : UnicodeString();
  setWindowTitle(ToQString(FormatMainFormCaption(UnicodeString(), SessionName)));
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalChanged()
{
  TTerminal * Terminal = GetTerminal();
  FRemotePanel->SetTerminal(Terminal);
  UnicodeString Status;
  UnicodeString Hint;
  if ((Terminal != NULL) && Terminal->Active)
  {
    // As upstream's status bar: the protocol of the file system (SFTP-3),
    // else of the session; the encryption in the hint
    const TSessionInfo & SessionInfo = Terminal->GetSessionInfo();
    const TFileSystemInfo & FileSystemInfo = Terminal->GetFileSystemInfo();
    Status = FileSystemInfo.ProtocolName.IsEmpty() ? SessionInfo.ProtocolName : FileSystemInfo.ProtocolName;
    Hint = SessionInfo.SecurityProtocolName;
    // As upstream: the local directory of the site, once it opens
    if (FLocalDirectoryTerminal != Terminal)
    {
      FLocalDirectoryTerminal = Terminal;
      UnicodeString LocalDirectory = Terminal->SessionData->LocalDirectoryExpanded;
      if (!LocalDirectory.IsEmpty())
      {
        try
        {
          FLocalPanel->ChangeDirectory(LocalDirectory);
        }
        catch (Exception & E)
        {
          ShowExtendedException(&E);
        }
      }
    }
  }
  else if (Terminal == NULL)
  {
    FLocalDirectoryTerminal = NULL;
  }
  FSessionStatusLabel->setText(ToQString(Status));
  FSessionStatusLabel->setToolTip(ToQString(Hint));
  if (!FRemotePanel->IsReady() && (FCurrentPanel == FRemotePanel))
  {
    FLocalPanel->GetDirView()->setFocus();
    SetCurrentPanel(FLocalPanel);
  }
  UpdateCaption();
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::NewSession()
{
  if (!IsBusy())
  {
    std::unique_ptr<TObjectList> DataList(new TObjectList());
    bool Connected = false;
    // As upstream on startup: the site manager again, when the session does
    // not open
    while (!Connected && DoLoginDialog(DataList.get(), this, false))
    {
      TTerminalManager * Manager = TTerminalManager::Instance();
      Manager->NewSession(static_cast<TSessionData *>(DataList->Items[0]));
      Connected = Manager->ConnectActiveTerminal();
    }
    if (Connected)
    {
      FRemotePanel->GetDirView()->setFocus();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DisconnectSessionActionExecute()
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  if (!IsBusy() && (Manager->ActiveTerminal != NULL))
  {
    Manager->FreeActiveTerminal();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CloseApplicationActionExecute()
{
  close();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::closeEvent(QCloseEvent * Event)
{
  // The operation in progress is to be cancelled first
  if (IsBusy())
  {
    Event->ignore();
  }
  else
  {
    QMainWindow::closeEvent(Event);
  }
}
//---------------------------------------------------------------------------
TStrings * TScpCommanderForm::CreateFileList(TDirPanel * Panel)
{
  std::unique_ptr<TStringList> Result(new TStringList());
  for (int Row : Panel->GetSelectedRows())
  {
    const TDirViewItem & Item = Panel->GetModel()->Item(Row);
    if (Panel == FLocalPanel)
    {
      Result->Add(IncludeTrailingBackslash(Panel->GetPath()) + Item.FileName);
    }
    else
    {
      Result->AddObject(Item.File->FullFileName, Item.File);
    }
  }
  if (Panel == FRemotePanel)
  {
    // The remote directory may be reloaded during the operation, freeing
    // the files of the panel
    return TRemoteFileList::CloneStrings(Result.get());
  }
  return Result.release();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentCopyActionExecute()
{
  TTerminal * Terminal = GetTerminal();
  TDirPanel * Source = FCurrentPanel;
  TDirPanel * Target = OtherPanel(Source);
  if (IsBusy() || (Terminal == NULL) || !Source->IsReady() || !Target->IsReady())
  {
    return;
  }

  std::unique_ptr<TStrings> FileList(CreateFileList(Source));
  if (FileList->Count == 0)
  {
    return;
  }

  bool ToRemote = (Source == FLocalPanel);
  UnicodeString TargetDirectory = Target->GetPath();
  if (DoCopyDialog(ToRemote, FileList.get(), TargetDirectory))
  {
    TGUICopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]()
    {
      DestroyProgressForm();
      if (!ToRemote)
      {
        FLocalPanel->Reload();
      }
    });
    if (ToRemote)
    {
      UnicodeString TargetDir = UnixIncludeTrailingBackslash(TargetDirectory);
      CopyParam.IncludeFileMask.SetRoots(FileList.get(), TargetDir);
      Terminal->CopyToRemote(FileList.get(), TargetDir, &CopyParam, 0, NULL);
    }
    else
    {
      UnicodeString TargetDir = IncludeTrailingBackslash(TargetDirectory);
      CopyParam.IncludeFileMask.SetRoots(TargetDir, FileList.get());
      Terminal->CopyToLocal(FileList.get(), TargetDir, &CopyParam, 0, NULL);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DeleteFiles(bool Alternative)
{
  TDirPanel * Panel = FCurrentPanel;
  if (IsBusy() || !Panel->IsReady())
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(Panel));
  if (FileList->Count == 0)
  {
    return;
  }

  if (AppConfiguration->ConfirmDeleting)
  {
    UnicodeString Message;
    if (FileList->Count == 1)
    {
      UnicodeString FileName =
        (Panel == FLocalPanel) ? ExtractFileName(FileList->Strings[0]) : UnixExtractFileName(FileList->Strings[0]);
      Message = FMTLOAD(CONFIRM_DELETE_FILE, (FileName));
    }
    else
    {
      Message = FMTLOAD(CONFIRM_DELETE_FILES, (FileList->Count));
    }
    TMessageParams Params(mpNeverAskAgainCheck);
    unsigned int Answer = MessageDialog(MainInstructions(Message), qtConfirmation, qaOK | qaCancel, &Params);
    if (Answer == qaNeverAskAgain)
    {
      AppConfiguration->ConfirmDeleting = false;
    }
    else if (Answer != qaOK)
    {
      return;
    }
  }

  ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
  {
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]()
    {
      DestroyProgressForm();
      if (Panel == FLocalPanel)
      {
        FLocalPanel->Reload();
      }
    });
    if (Panel == FLocalPanel)
    {
      bool ToRecycleBin = (DeleteToRecycleBin != Alternative);
      for (int Index = 0; Index < FileList->Count; Index++)
      {
        RecursiveDeleteFileChecked(FileList->Strings[Index], ToRecycleBin);
      }
    }
    else
    {
      GetTerminal()->DeleteFiles(FileList.get(), FLAGMASK(Alternative, dfAlternative));
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentRenameActionExecute()
{
  if (!IsBusy())
  {
    FCurrentPanel->StartRename();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RenameFile(TDirPanel * Panel, const UnicodeString & FileName, const UnicodeString & NewName)
{
  if (IsBusy() || !Panel->IsReady())
  {
    return;
  }
  {
    TAutoNestingCounter BusyCounter(FBusy);
    if (Panel == FLocalPanel)
    {
      UnicodeString Directory = IncludeTrailingBackslash(Panel->GetPath());
      if (!::RenameFile(Directory + FileName, Directory + NewName))
      {
        throw EOSExtException(FMTLOAD(RENAME_FILE_ERROR, (FileName, NewName)));
      }
      Panel->Reload();
    }
    else
    {
      int Row = Panel->GetModel()->FindFile(FileName);
      if (Row >= 0)
      {
        // A copy, as the directory is reloaded after renaming
        std::unique_ptr<TRemoteFile> File(FRemotePanel->GetFile(Row)->Duplicate(true));
        GetTerminal()->RenameFile(File.get(), NewName);
      }
    }
  }
  Panel->FocusFile(NewName);
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CurrentCreateDirActionExecute()
{
  TDirPanel * Panel = FCurrentPanel;
  if (IsBusy() || !Panel->IsReady())
  {
    return;
  }
  UnicodeString Name = LoadStr(NEW_FOLDER);
  if (DoCreateDirectoryDialog(Name))
  {
    {
      TAutoNestingCounter BusyCounter(FBusy);
      if (Panel == FLocalPanel)
      {
        UnicodeString Path = IncludeTrailingBackslash(Panel->GetPath()) + Name;
        if (!ForceDirectories(ApiPath(Path)))
        {
          throw EOSExtException(FMTLOAD(CREATE_DIR_ERROR, (Path)));
        }
        Panel->Reload();
      }
      else
      {
        // The attributes of new directories (the dialog does not set them yet)
        TRemoteProperties Properties = GUIConfiguration->NewDirectoryProperties;
        GetTerminal()->CreateDirectory(UnixCombinePaths(Panel->GetPath(), Name), &Properties);
      }
    }
    Panel->FocusFile(UnixExtractFileName(UnixExcludeTrailingBackslash(Name)));
    UpdateActions();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ShowHiddenFilesActionExecute()
{
  bool Show = ShowHiddenFilesAction->isChecked();
  AppConfiguration->ShowHiddenFiles = Show;
  FLocalPanel->SetShowHiddenFiles(Show);
  FRemotePanel->SetShowHiddenFiles(Show);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DestroyProgressForm()
{
  delete FProgressForm;
  FProgressForm = nullptr;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OperationProgress(TFileOperationProgressType & ProgressData)
{
  // The operation runs and the progress is not shown yet
  if (ProgressData.InProgress && (FProgressForm == nullptr))
  {
    FProgressForm = new TProgressForm(this, ProgressData.IsTransfer() && (ProgressData.Count > 1));
    FProgressForm->ShowAsModal();
  }
  // The operation finished (or was cancelled)
  else if (!ProgressData.InProgress && (FProgressForm != nullptr))
  {
    DestroyProgressForm();
  }

  if ((FProgressForm != nullptr) && ProgressData.InProgress)
  {
    FProgressForm->SetProgressData(ProgressData);
    // The form is freed when the operation finishes while the events are
    // processed
    if (FProgressForm != nullptr)
    {
      TCancelStatus Cancel = FProgressForm->GetCancel();
      if (Cancel > csContinue)
      {
        ProgressData.SetCancelAtLeast(Cancel);
        if (Cancel == csCancelFile)
        {
          FProgressForm->ClearCancel();
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OperationFinished(TFileOperation Operation, TOperationSide Side,
  bool Temp, const UnicodeString & FileName, bool Success, bool /*NotCancelled*/,
  TOnceDoneOperation & /*OnceDoneOperation*/)
{
  // The transferred files are unselected in the source panel, as upstream
  if (Success && !Temp && ((Operation == foCopy) || (Operation == foMove)))
  {
    TDirPanel * Panel = (Side == osLocal) ? static_cast<TDirPanel *>(FLocalPanel) : static_cast<TDirPanel *>(FRemotePanel);
    UnicodeString Name = (Side == osLocal) ? ExtractFileName(FileName) : UnixExtractFileName(FileName);
    int Row = Panel->GetModel()->FindFile(Name);
    if (Row >= 0)
    {
      QItemSelectionModel * Selection = Panel->GetDirView()->selectionModel();
      Selection->select(Panel->GetModel()->index(Row, 0), QItemSelectionModel::Deselect | QItemSelectionModel::Rows);
    }
  }
}
//---------------------------------------------------------------------------
