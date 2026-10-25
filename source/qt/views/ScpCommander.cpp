// The main window with the Commander interface.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <EditorManager.h>
#include <FileSystems.h>
#include <LocalFiles.h>
#include <TerminalManager.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "ProgressForm.h"
#include "ScpCommander.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QTreeWidget>
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
  FSynchronizeProgressForm = nullptr;
  FBusy = 0;
  FUpdatingTransferList = 0;
  FLocalDirectoryTerminal = NULL;
  FRefreshLocalDirectory = false;
  FRefreshRemoteDirectory = false;
  FFileFindTerminal = NULL;
  FAutoOperation = false;
  FSynchronizeController = NULL;
  FSynchronizeTerminal = NULL;
  FQueue = NULL;
  FQueueStatus = NULL;
  FQueueStatusInvalidated = false;
  FQueueItemInvalidated = false;
  FQueueStatusUpdating = false;
  FPendingQueueActionItem = NULL;
  FDragSourcePanel = NULL;
  FDragging = false;
  FDragExtracted = false;
  CreateEditorManager();

  // The file panels, the queue below them
  FQueueSplitter = new QSplitter(Qt::Vertical, this);
  FQueueSplitter->setObjectName(QStringLiteral("QueueSplitter"));
  FQueueSplitter->setChildrenCollapsible(false);
  FPanelSplitter = new QSplitter(Qt::Horizontal, FQueueSplitter);
  FPanelSplitter->setObjectName(QStringLiteral("PanelSplitter"));
  FLocalPanel = new TLocalDirPanel(FPanelSplitter);
  FRemotePanel = new TRemoteDirPanel(FPanelSplitter);
  FPanelSplitter->addWidget(FLocalPanel);
  FPanelSplitter->addWidget(FRemotePanel);
  FQueueSplitter->addWidget(FPanelSplitter);
  CreateQueueActions();
  CreateQueuePanel();
  FQueueSplitter->addWidget(QueuePanel);
  FQueueSplitter->setStretchFactor(0, 1);
  QueuePanel->setVisible(false);
  setCentralWidget(FQueueSplitter);
  FCurrentPanel = FLocalPanel;

  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    connect(Panel, &TDirPanel::Entered, this, [this](TDirPanel * APanel) { SetCurrentPanel(APanel); });
    connect(Panel, &TDirPanel::DirectoryChanged, this, [this]() { TransferPresetAutoSelect(); });
    connect(Panel, &TDirPanel::OpenDirectoryRequested, this, [this](TDirPanel * APanel) { OpenDirectory(APanel->GetSide()); });
    connect(Panel, &TDirPanel::FileActivated, this, [this](TDirPanel * APanel) { FileActivated(APanel); });
    connect(Panel, &TDirPanel::AddBookmarkRequested, this, [this](TDirPanel * APanel) { AddBookmark(APanel->GetSide()); });
    connect(Panel, &TDirPanel::BookmarksMenuRequested, this,
      [this](TDirPanel * APanel, QMenu * Menu) { CreateOpenDirMenu(APanel, Menu); });
    Panel->OnRename = [this](TDirPanel * APanel, const UnicodeString & FileName, const UnicodeString & NewName)
    {
      RenameFile(APanel, FileName, NewName);
    };
    Panel->OnStartDrag = [this](TDirPanel * APanel) { PanelStartDrag(APanel); };
    Panel->OnDragOver = [this](TDirPanel * APanel, const TDropParams & Params) { return PanelDragOver(APanel, Params); };
    Panel->OnDrop = [this](TDirPanel * APanel, const TDropParams & Params, Qt::DropAction Action)
    {
      PanelDrop(APanel, Params, Action);
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
  CurrentMoveAction = CreateAction("CurrentMoveAction", tr("&Move..."), QStringLiteral("edit-cut"),
    { QKeySequence(Qt::Key_F6) }, &TScpCommanderForm::CurrentMoveActionExecute);
  RemoteCopyToAction = CreateAction("RemoteCopyToAction", tr("&Duplicate..."), QStringLiteral("edit-copy"),
    { QKeySequence(Qt::SHIFT | Qt::Key_F5) }, &TScpCommanderForm::RemoteCopyToActionExecute);
  RemoteMoveToAction = CreateAction("RemoteMoveToAction", tr("Mo&ve To..."), QStringLiteral("go-jump"),
    { QKeySequence(Qt::SHIFT | Qt::Key_F6) }, &TScpCommanderForm::RemoteMoveToActionExecute);
  CurrentEditAction = CreateAction("CurrentEditAction", tr("&Edit"), QStringLiteral("document-edit"),
    { QKeySequence(Qt::Key_F4) }, &TScpCommanderForm::CurrentEditActionExecute);
  CurrentEditInternalAction = CreateAction("CurrentEditInternalAction", tr("&Internal editor"), QString(),
    { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F4) }, &TScpCommanderForm::CurrentEditInternalActionExecute);
  CurrentOpenAction = CreateAction("CurrentOpenAction", tr("&Open"), QStringLiteral("document-open"),
    {}, &TScpCommanderForm::CurrentOpenActionExecute);
  NewFileAction = CreateAction("NewFileAction", tr("&File..."), QStringLiteral("document-new"),
    { QKeySequence(Qt::SHIFT | Qt::Key_F4) }, &TScpCommanderForm::NewFileActionExecute);
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
  CurrentPropertiesAction = CreateAction("CurrentPropertiesAction", tr("&Properties"), QStringLiteral("document-properties"),
    { QKeySequence(Qt::Key_F9) }, &TScpCommanderForm::CurrentPropertiesActionExecute);
  RemoteFindFilesAction2 = CreateAction("RemoteFindFilesAction2", tr("&Find Files..."), QStringLiteral("edit-find"),
    { QKeySequence(Qt::ALT | Qt::Key_F7) }, &TScpCommanderForm::RemoteFindFilesActionExecute);
  FullSynchronizeAction2 = CreateAction("FullSynchronizeAction2", tr("&Synchronize..."), QStringLiteral("folder-sync"),
    { QKeySequence(Qt::CTRL | Qt::Key_S) }, &TScpCommanderForm::FullSynchronizeActionExecute);
  SynchronizeAction = CreateAction("SynchronizeAction", tr("&Keep Remote Directory up to Date..."),
    QStringLiteral("view-refresh"), { QKeySequence(Qt::CTRL | Qt::Key_U) }, &TScpCommanderForm::SynchronizeActionExecute);
  ConsoleAction = CreateAction("ConsoleAction", tr("Open &Terminal"), QStringLiteral("utilities-terminal"),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T) }, &TScpCommanderForm::ConsoleActionExecute);
  CustomCommandsEnterAction = CreateAction("CustomCommandsEnterAction", tr("&Enter..."), QString(),
    {}, &TScpCommanderForm::AdHocCustomCommand);
  CustomCommandsLastAction = CreateAction("CustomCommandsLastAction", QStringLiteral("CustomCommandsLastAction"), QString(),
    {}, &TScpCommanderForm::LastCustomCommand);
  CustomCommandsLastAction->setVisible(false);
  CustomCommandsCustomizeAction = CreateAction("CustomCommandsCustomizeAction", tr("&Customize..."), QString(),
    {}, &TScpCommanderForm::CustomCommandsCustomize);
  ShowHiddenFilesAction = CreateAction("ShowHiddenFilesAction", tr("Show/Hide &Hidden Files"), QString(),
    { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_H) }, &TScpCommanderForm::ShowHiddenFilesActionExecute);
  ShowHiddenFilesAction->setCheckable(true);
  ShowHiddenFilesAction->setChecked(AppConfiguration->ShowHiddenFiles);

  CreateMenus();
  CreateToolbars();
  UpdateBookmarkShortCuts();
  UpdateCustomCommandShortCuts();

  FSessionStatusLabel = new QLabel(statusBar());
  FSessionStatusLabel->setObjectName(QStringLiteral("SessionStatusLabel"));
  statusBar()->addPermanentWidget(FSessionStatusLabel);

  FLocalPanel->SetShowHiddenFiles(AppConfiguration->ShowHiddenFiles);
  FRemotePanel->SetShowHiddenFiles(AppConfiguration->ShowHiddenFiles);
  FLocalPanel->ChangeDirectory(ToUnicodeString(QDir::homePath()));

  TerminalChanged();
  FLocalPanel->GetDirView()->setFocus();

  // Upstream's SessionIdleTimer
  FIdleTimer = new QTimer(this);
  FIdleTimer->setObjectName(QStringLiteral("SessionIdleTimer"));
  FIdleTimer->setInterval(250);
  connect(FIdleTimer, &QTimer::timeout, this, [this]() { Idle(); });
  FIdleTimer->start();
}
//---------------------------------------------------------------------------
TScpCommanderForm::~TScpCommanderForm()
{
  DoDelayedDeletion();
  ForceCloseEditors();
  HideFileFindDialog();
  DestroyProgressForm();
  SetQueue(NULL);
  delete FQueueStatus;
  FQueueStatus = NULL;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::Idle()
{
  // Not reentered from the nested event loops of what it shows
  static bool Executing = false;
  if (Executing)
  {
    return;
  }
  TAutoFlag ExecutingFlag(Executing);

  try
  {
    FEditorManager->Check();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
  }

  // make sure that Idle is called before update queue, as it may invoke QueueEvent
  // that needs to know if queue view is visible (and it may be closed after queue update)
  TTerminalManager::Instance()->Idle();

  if (!IsBusy() && (QApplication::activeModalWidget() == nullptr))
  {
    if (FRefreshRemoteDirectory)
    {
      TTerminal * Terminal = GetTerminal();
      if ((Terminal != NULL) && (Terminal->Status == ssOpened))
      {
        Terminal->RefreshDirectory();
      }
      FRefreshRemoteDirectory = false;
    }
    if (FRefreshLocalDirectory)
    {
      FLocalPanel->Reload();
      FRefreshLocalDirectory = false;
    }
  }

  if (FQueueStatusInvalidated)
  {
    UpdateQueueStatus(false);
  }

  RefreshQueueItems();
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
  QMenu * NewMenu = FilesMenu->addMenu(tr("&New"));
  NewMenu->setObjectName(QStringLiteral("NewMenu"));
  NewMenu->addAction(NewFileAction);
  FilesMenu->addSeparator();
  FilesMenu->addAction(CurrentOpenAction);
  FilesMenu->addAction(CurrentEditAction);
  QMenu * EditWithMenu = FilesMenu->addMenu(tr("Edi&t With"));
  EditWithMenu->setObjectName(QStringLiteral("CurrentEditWithMenu"));
  EditWithMenu->addAction(CurrentEditInternalAction);
  FilesMenu->addSeparator();
  FilesMenu->addAction(CurrentCopyAction);
  FilesMenu->addAction(CurrentMoveAction);
  FilesMenu->addAction(RemoteCopyToAction);
  FilesMenu->addAction(RemoteMoveToAction);
  FilesMenu->addAction(CurrentRenameAction);
  FilesMenu->addAction(CurrentDeleteAction);
  FilesMenu->addAction(CurrentDeleteAlternativeAction);
  FilesMenu->addSeparator();
  FilesMenu->addAction(CurrentPropertiesAction);

  QMenu * CommandsMenu = Menu->addMenu(tr("&Commands"));
  CommandsMenu->setObjectName(QStringLiteral("CommandsMenu"));
  CommandsMenu->addAction(CurrentCreateDirAction);
  CommandsMenu->addAction(RemoteFindFilesAction2);
  CommandsMenu->addAction(FullSynchronizeAction2);
  CommandsMenu->addAction(SynchronizeAction);
  CommandsMenu->addSeparator();
  QMenu * CustomCommandsMenu = CommandsMenu->addMenu(tr("&Custom Commands"));
  CustomCommandsMenu->setObjectName(QStringLiteral("CustomCommandsMenu"));
  connect(CustomCommandsMenu, &QMenu::aboutToShow, this, [this, CustomCommandsMenu]()
  {
    CustomCommandsMenu->clear();
    CreateCustomCommandsMenu(CustomCommandsMenu);
  });
  CommandsMenu->addAction(ConsoleAction);
  CommandsMenu->addSeparator();
  CommandsMenu->addMenu(CreateQueueMenu());
  CommandsMenu->addSeparator();
  CommandsMenu->addAction(CloseApplicationAction);

  QMenu * SessionMenu = Menu->addMenu(tr("&Session"));
  SessionMenu->setObjectName(QStringLiteral("SessionMenu"));
  SessionMenu->addAction(NewSessionAction);
  SessionMenu->addAction(DisconnectSessionAction);

  QMenu * OptionsMenu = Menu->addMenu(tr("&Options"));
  OptionsMenu->setObjectName(QStringLiteral("OptionsMenu"));
  OptionsMenu->addAction(ShowHiddenFilesAction);
  QMenu * QueueViewMenu = OptionsMenu->addMenu(tr("&Queue"));
  QueueViewMenu->setObjectName(QStringLiteral("QueueViewMenu"));
  QueueViewMenu->addActions(FQueueShowGroup->actions());

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
  Toolbar->addAction(CurrentMoveAction);
  Toolbar->addAction(CurrentRenameAction);
  Toolbar->addAction(CurrentDeleteAction);
  Toolbar->addAction(CurrentCreateDirAction);
  Toolbar->addAction(CurrentPropertiesAction);

  // The transfer settings presets (upstream's TransferDropDown), in a row
  // of their own
  addToolBarBreak();
  QToolBar * TransferToolbar = addToolBar(tr("Transfer Settings"));
  TransferToolbar->setObjectName(QStringLiteral("TransferToolbar"));
  QLabel * TransferLabel = new QLabel(tr("Transfer Settings"), TransferToolbar);
  TransferLabel->setObjectName(QStringLiteral("TransferLabel"));
  TransferLabel->setContentsMargins(4, 0, 4, 0);
  TransferToolbar->addWidget(TransferLabel);
  FTransferList = new QComboBox(TransferToolbar);
  FTransferList->setObjectName(QStringLiteral("TransferList"));
  FTransferList->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  TransferLabel->setBuddy(FTransferList);
  TransferToolbar->addWidget(FTransferList);
  connect(FTransferList, &QComboBox::currentIndexChanged, this, [this]() { TransferListChange(); });
  UpdateTransferList();

  // The keys of the commands, at the bottom, as in upstream's Commander
  QToolBar * Hotkeys = new QToolBar(tr("Hotkeys"), this);
  Hotkeys->setObjectName(QStringLiteral("HotkeysToolbar"));
  Hotkeys->setToolButtonStyle(Qt::ToolButtonTextOnly);
  addToolBar(Qt::BottomToolBarArea, Hotkeys);
  for (QAction * Action :
         { CurrentRenameAction, CurrentEditAction, CurrentCopyAction, CurrentMoveAction, CurrentCreateDirAction, CurrentDeleteAction,
           CurrentPropertiesAction, CloseApplicationAction })
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
  bool CanTransfer = !Busy && HasSelection && Connected && OtherPanel(FCurrentPanel)->IsReady();
  CurrentCopyAction->setEnabled(CanTransfer);
  CurrentMoveAction->setEnabled(CanTransfer);
  bool RemoteSelection = !Busy && HasSelection && Connected && (FCurrentPanel == FRemotePanel);
  RemoteCopyToAction->setEnabled(RemoteSelection);
  RemoteMoveToAction->setEnabled(RemoteSelection);
  CurrentRenameAction->setEnabled(!Busy && HasSelection);
  bool CanEdit = !Busy && HasSelection && ((FCurrentPanel == FLocalPanel) || Connected);
  CurrentEditAction->setEnabled(CanEdit);
  CurrentEditInternalAction->setEnabled(CanEdit);
  CurrentOpenAction->setEnabled(CanEdit);
  NewFileAction->setEnabled(!Busy && CurrentReady);
  CurrentDeleteAction->setEnabled(!Busy && HasSelection);
  CurrentDeleteAlternativeAction->setEnabled(!Busy && HasSelection);
  CurrentCreateDirAction->setEnabled(!Busy && CurrentReady);
  CurrentPropertiesAction->setEnabled(!Busy && HasSelection);
  RemoteFindFilesAction2->setEnabled(!Busy && Connected && FRemotePanel->IsReady());
  FullSynchronizeAction2->setEnabled(!Busy && Connected && FRemotePanel->IsReady());
  SynchronizeAction->setEnabled(!Busy && Connected && FRemotePanel->IsReady());
  ConsoleAction->setEnabled(!Busy && CanConsole());
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanConsole()
{
  TTerminal * Terminal = GetTerminal();
  return
    (Terminal != NULL) && Terminal->Active &&
    (Terminal->IsCapable[fcAnyCommand] || Terminal->IsCapable[fcSecondaryShell]);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OpenConsole(const UnicodeString & Command)
{
  if (!IsBusy() && CanConsole() && EnsureCommandSessionFallback(fcAnyCommand))
  {
    DoConsoleDialog(GetTerminal(), Command);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ConfigurationChanged()
{
  UpdateTransferList();
  UpdateBookmarkShortCuts();
  UpdateCustomCommandShortCuts();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateTransferList()
{
  TAutoNestingCounter UpdatingCounter(FUpdatingTransferList);
  FTransferList->clear();
  FTransferList->addItem(ToQString(StripHotkey(LoadStr(COPY_PARAM_DEFAULT))));
  const TCopyParamList * CopyParamList = GUIConfiguration->CopyParamList;
  for (int Index = 0; Index < CopyParamList->Count; Index++)
  {
    FTransferList->addItem(ToQString(StripHotkey(CopyParamList->Names[Index])));
  }
  FTransferList->setCurrentIndex(GUIConfiguration->CopyParamIndex + 1);
  TTerminal * Terminal = GetTerminal();
  int CopyParamAttrs = ((Terminal != NULL) && Terminal->Active) ? Terminal->UsableCopyParamAttrs(0).General : 0;
  FTransferList->setToolTip(
    ToQString(FORMAT(L"%s:\n%s",
      (StripHotkey(ToUnicodeString(FTransferList->currentText())),
       GUIConfiguration->CurrentCopyParam.GetInfoStr(L"; ", CopyParamAttrs)))));
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TransferListChange()
{
  if (FUpdatingTransferList == 0)
  {
    int Index = FTransferList->currentIndex();
    UnicodeString Name;
    if (Index > 0)
    {
      Name = GUIConfiguration->CopyParamList->Names[Index - 1];
    }
    if (FCopyParamAutoSelected.IsEmpty())
    {
      // if previous preset was not autoselected, make new preset the "default"
      FCopyParamDefault = Name;
    }
    GUIConfiguration->CopyParamCurrent = Name;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TransferPresetAutoSelect()
{
  // Terminal can be null when we are changing local directory implicitly,
  // because it has been deleted, while no session is connected
  // (Login dialog is open)
  TTerminal * Terminal = GetTerminal();
  if ((Terminal == NULL) || !Terminal->Active || !FRemotePanel->IsReady())
  {
    return;
  }

  TCopyParamRuleData Data;
  Data.HostName = Terminal->SessionData->HostNameExpanded;
  Data.UserName = Terminal->SessionData->UserNameExpanded;
  Data.RemoteDirectory = FRemotePanel->GetPath();
  Data.LocalDirectory = FLocalPanel->GetPath();

  int CopyParamIndex = GUIConfiguration->CopyParamList->Find(Data);
  UnicodeString CopyParamCurrent = GUIConfiguration->CopyParamCurrent;

  if (CopyParamIndex < 0)
  {
    // there is no preset that matches autoselection
    // set preset that we consider "default"
    FCopyParamAutoSelected = L""; // forget last autoselected preset
    GUIConfiguration->CopyParamCurrent = FCopyParamDefault;
  }
  else
  {
    // there is preset matching autoselection
    UnicodeString CopyParamName = GUIConfiguration->CopyParamList->Names[CopyParamIndex];
    if (CopyParamName == FCopyParamAutoSelected)
    {
      // autoselected the same preset as the last time
      // make no change (i.e. preserve custom user preset, if any)
    }
    else
    {
      // autoselected the different preset then the last time (or there
      // was default preset set)
      FCopyParamAutoSelected = CopyParamName; // remember autoselection
      GUIConfiguration->CopyParamCurrent = CopyParamName;
    }
  }

  if ((GUIConfiguration->CopyParamCurrent != CopyParamCurrent) && AppConfiguration->CopyParamAutoSelectNotice)
  {
    if (CopyParamIndex >= 0)
    {
      Configuration->Usage->Inc(L"CopyParamAutoSelects");
    }

    int Fmt =
      (CopyParamIndex < 0) ?
        (GUIConfiguration->CopyParamIndex < 0 ? COPY_PARAM_DEFAULT_NORM : COPY_PARAM_DEFAULT_CUSTOM) :
        COPY_PARAM_AUTOSELECTED;
    UnicodeString Message = MainInstructions(FMTLOAD(Fmt, (StripHotkey(GUIConfiguration->CopyParamCurrent))));

    int CopyParamAttrs = Terminal->UsableCopyParamAttrs(0).General;
    UnicodeString Info = GUIConfiguration->CurrentCopyParam.GetInfoStr(L"\n", CopyParamAttrs);
    if (CopyParamIndex >= 0)
    {
      DebugAssert(GUIConfiguration->CopyParamList->Rules[CopyParamIndex] != NULL);
      Info = FORMAT(L"%s\n \n%s", (Info,
        FMTLOAD(COPY_PARAM_RULE,
          (GUIConfiguration->CopyParamList->Rules[CopyParamIndex]->GetInfoStr(L"\n")))));
    }
    Message += L"\n\n" + Info;

    // Upstream also offers the preferences of the presets (not ported yet)
    TMessageParams Params(mpNeverAskAgainCheck);
    unsigned int Answer = MoreMessageDialog(Message, NULL, qtInformation, qaOK, &Params);
    if (Answer == qaNeverAskAgain)
    {
      AppConfiguration->CopyParamAutoSelectNotice = false;
    }
  }
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
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  SetQueue((Manager != NULL) ? Manager->ActiveQueue : NULL);
  // The file find of a session that is gone
  if ((FFileFindTerminal != NULL) && (FFileFindTerminal != Terminal))
  {
    FFileFindTerminal = NULL;
    HideFileFindDialog();
  }
  EditedFilesTerminalChanged(Terminal);
  // Keeping the remote directory of a session that is gone up to date
  if ((FOnSynchronizeAbort != nullptr) && (FSynchronizeTerminal != Terminal))
  {
    FOnSynchronizeAbort();
  }
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
  UpdateTransferList();
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
  if (!IsBusy() && (Manager->ActiveTerminal != NULL) && CanCloseQueue())
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
  if (IsBusy() || !CanCloseQueue() || !CloseEditors())
  {
    Event->ignore();
  }
  else
  {
    QMainWindow::closeEvent(Event);
  }
}
//---------------------------------------------------------------------------
TStrings * TScpCommanderForm::CreateFileList(TDirPanel * Panel, bool FullPath)
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
      Result->AddObject(FullPath ? Item.File->FullFileName : Item.File->FileName, Item.File);
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
void TScpCommanderForm::HandleDoNotShowCopyDialogAgain(bool DragDrop, bool DoNotShowAgain)
{
  if (DoNotShowAgain)
  {
    if (DragDrop)
    {
      AppConfiguration->DDTransferConfirmation = asOff;
    }
    else
    {
      AppConfiguration->ConfirmTransferring = false;
    }
  }
  // User explicitly unchecked "do not show again",
  // so show him the dialog the next time
  else if (DragDrop && (AppConfiguration->DDTransferConfirmation == asAuto))
  {
    AppConfiguration->DDTransferConfirmation = asOn;
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CopyParamDialog(TTransferDirection Direction, TTransferType Type,
  TStrings * FileList, UnicodeString & TargetDirectory, TGUICopyParamType & CopyParam, bool Confirm,
  bool DragDrop)
{
  bool Result = true;
  TTerminal * Terminal = GetTerminal();

  // these parameters are known in advance
  int Params =
    FLAGMASK(Type == ttMove, cpDelete);
  if (Result && Confirm && AppConfiguration->ConfirmTransferring)
  {
    bool DisableNewerOnly =
      (!Terminal->IsCapable[fcNewerOnlyUpload] && (Direction == tdToRemote));
    int Options =
      FLAGMASK(!Terminal->IsCapable[fcBackgroundTransfers], coDisableQueue) |
      coDoNotShowAgain;
    TUsableCopyParamAttrs UsableCopyParamAttrs = Terminal->UsableCopyParamAttrs(Params);
    int CopyParamAttrs = (Direction == tdToRemote ?
      UsableCopyParamAttrs.Upload : UsableCopyParamAttrs.Download) |
      FLAGMASK(DisableNewerOnly, cpaNoNewerOnly);
    int OutputOptions =
      FLAGMASK(DragDrop && (AppConfiguration->DDTransferConfirmation == asAuto), cooDoNotShowAgain);
    Result = DoCopyDialog(Direction == tdToRemote, Type == ttMove,
      FileList, TargetDirectory, &CopyParam, Options, CopyParamAttrs, &OutputOptions);

    if (Result)
    {
      HandleDoNotShowCopyDialogAgain(DragDrop, FLAGSET(OutputOptions, cooDoNotShowAgain));
    }
  }

  if (Result && CopyParam.Queue && Terminal->IsCapable[fcBackgroundTransfers])
  {
    Configuration->Usage->Inc(L"TransfersOnBackground");

    // these parameter are known only after transfer dialog
    Params |=
      (CopyParam.QueueNoConfirmation ? cpNoConfirmation : 0);

    AddQueueItem(Direction, FileList, TargetDirectory, CopyParam, Params);
    Result = false;

    // upstream's ClearTransferSourceSelection
    TOperationSide Side = ((Direction == tdToRemote) ? osLocal : osRemote);
    if (FOnFileOperationFinished)
    {
      FOnFileOperationFinished(Side, UnicodeString(), true, true);
    }
    else
    {
      TDirPanel * Source = (Side == osLocal) ? static_cast<TDirPanel *>(FLocalPanel) : static_cast<TDirPanel *>(FRemotePanel);
      Source->GetDirView()->selectionModel()->clearSelection();
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteCopyMoveFileOperation(TFileOperation Operation)
{
  TTerminal * Terminal = GetTerminal();
  TDirPanel * Source = FCurrentPanel;
  TDirPanel * Target = OtherPanel(Source);
  if (IsBusy() || (Terminal == NULL) || !Source->IsReady() || !Target->IsReady())
  {
    return;
  }

  std::unique_ptr<TStrings> FileList(CreateFileList(Source));
  if (FileList->Count > 0)
  {
    ExecuteCopyMoveFileOperation(Operation, Source->GetSide(), FileList.get());
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::ExecuteCopyMoveFileOperation(TFileOperation Operation, TOperationSide Side, TStrings * FileList,
  const TTransferOperationParam & Param)
{
  TTerminal * Terminal = GetTerminal();
  bool ToRemote = (Side == osLocal);
  TDirPanel * Target = ToRemote ? static_cast<TDirPanel *>(FRemotePanel) : static_cast<TDirPanel *>(FLocalPanel);
  TTransferDirection Direction = (ToRemote ? tdToRemote : tdToLocal);
  TTransferType Type = (Operation == foCopy ? ttCopy : ttMove);
  UnicodeString TargetDirectory = Param.TargetDirectory.IsEmpty() ? Target->GetPath() : Param.TargetDirectory;
  TGUICopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
  bool Confirm = !Param.DragDrop || (AppConfiguration->DDTransferConfirmation != asOff);
  bool Result = CopyParamDialog(Direction, Type, FileList, TargetDirectory, CopyParam, Confirm, Param.DragDrop);
  if (Result)
  {
    int Params = FLAGMASK(Operation == foMove, cpDelete);
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]()
    {
      DestroyProgressForm();
      // The sources of an upload that moved, the target of a download
      FLocalPanel->Reload();
    });
    if (ToRemote)
    {
      UnicodeString TargetDir = UnixIncludeTrailingBackslash(TargetDirectory);
      CopyParam.IncludeFileMask.SetRoots(FileList, TargetDir);
      Terminal->CopyToRemote(FileList, TargetDir, &CopyParam, Params, NULL);
    }
    else
    {
      UnicodeString TargetDir = IncludeTrailingBackslash(TargetDirectory);
      CopyParam.IncludeFileMask.SetRoots(TargetDir, FileList);
      Terminal->CopyToLocal(FileList, TargetDir, &CopyParam, Params, NULL);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::DoDirectoryExists(void * Session, const UnicodeString & Directory)
{
  TTerminal * ATerminal = (Session == NULL) ? GetTerminal() : reinterpret_cast<TTerminal *>(Session);
  TAutoNestingCounter BusyCounter(FBusy);
  return (ATerminal != NULL) && ATerminal->Active && ATerminal->DirectoryExists(Directory);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::NeedSecondarySessionForRemoteCopy(TStrings * FileList)
{
  TTerminal * Terminal = GetTerminal();
  bool CopyDirsOnSecondarySession = Terminal->IsCapable[fcSecondaryShell];
  return
    !Terminal->IsCapable[fcRemoteCopy] ||
    (CopyDirsOnSecondarySession && TRemoteFileList::AnyDirectory(FileList));
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::RemoteTransferDialog(TStrings * FileList, UnicodeString & Target, UnicodeString & FileMask,
  bool & DirectCopy, bool Move)
{
  TTerminal * Terminal = GetTerminal();
  // A single session: it is the target
  void * Session = Terminal;
  if (FileList->Count == 1)
  {
    FileMask = DelimitFileNameMask(UnixExtractFileName(FileList->Strings[0]));
  }
  else
  {
    FileMask = AnyMask;
  }
  DirectCopy = Terminal->IsCapable[fcRemoteCopy] || Terminal->IsCapable[fcSecondaryShell];
  bool Multi = (FileList->Count > 1);
  TDirectoryExistsEvent DirectoryExistsEvent =
    [this](void * ASession, const UnicodeString & Directory) { return DoDirectoryExists(ASession, Directory); };

  bool Result;
  if (Move)
  {
    Result = DoRemoteMoveDialog(Multi, Target, FileMask, DirectoryExistsEvent);
  }
  else
  {
    std::unique_ptr<TStrings> Sessions(new TStringList);
    std::unique_ptr<TStrings> Directories(new TStringList);
    Sessions->AddObject(Terminal->SessionData->SessionName, reinterpret_cast<TObject *>(Terminal));
    Directories->Add(FRemotePanel->GetPath());

    TDirectRemoteCopy AllowDirectCopy;
    if (Terminal->CommandSessionOpened || !NeedSecondarySessionForRemoteCopy(FileList))
    {
      DebugAssert(DirectCopy);
      AllowDirectCopy = drcAllow;
    }
    else if (Terminal->IsCapable[fcSecondaryShell])
    {
      DebugAssert(DirectCopy);
      AllowDirectCopy = Terminal->IsCapable[fcRemoteCopy] ? drcConfirmCommandSessionDirs : drcConfirmCommandSession;
    }
    else
    {
      DebugAssert(!DirectCopy);
      AllowDirectCopy = drcDisallow;
    }
    Result = DoRemoteCopyDialog(
      Sessions.get(), Directories.get(), AllowDirectCopy, Multi, Session, Target, FileMask, DirectCopy, Terminal,
      DirectoryExistsEvent, false);
  }
  return Result;
}
//---------------------------------------------------------------------------
TCopyParamType TScpCommanderForm::TemporaryFileCopyParam(bool ForceText)
{
  TCopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
  CopyParam.FileNameCase = ncNoChange;
  CopyParam.PreserveRights = false;
  CopyParam.PreserveReadOnly = false;
  CopyParam.ReplaceInvalidChars = true;
  CopyParam.IncludeFileMask = TFileMasks();
  CopyParam.NewerOnly = false;
  CopyParam.FileMask = L"";
  if (ForceText)
  {
    CopyParam.TransferMode = tmAscii;
  }
  return CopyParam;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TemporarilyDownloadFiles(TStrings * FileList, bool ForceText, UnicodeString & RootTempDir,
  UnicodeString & TempDir, bool AutoOperation)
{
  TTerminal * Terminal = GetTerminal();
  TCopyParamType CopyParam = TemporaryFileCopyParam(ForceText);

  // A new temporary directory, unless given
  if (TempDir.IsEmpty())
  {
    RootTempDir = IncludeTrailingBackslash(AppConfiguration->TemporaryDir());
    TempDir = RootTempDir;
    if (!ForceDirectories(ApiPath(TempDir)))
    {
      throw EOSExtException(FMTLOAD(CREATE_TEMP_DIR_ERROR, (TempDir)));
    }
  }

  DebugAssert(!FAutoOperation);
  FAutoOperation = AutoOperation;
  Terminal->ExceptionOnFail = true;
  ::rtl::TFinally Finally([&]()
  {
    FAutoOperation = false;
    Terminal->ExceptionOnFail = false;
  });
  try
  {
    // turn off confirmations, as for MDI editors we may possibly download
    // the same file over
    Terminal->CopyToLocal(FileList, TempDir, &CopyParam, cpNoConfirmation | cpTemporary, NULL);
  }
  catch (...)
  {
    if (!RootTempDir.IsEmpty())
    {
      RecursiveDeleteFile(ExcludeTrailingBackslash(RootTempDir));
    }
    throw;
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::RemoteTransferFiles(TStrings * FileList, bool Move,
  const UnicodeString & TargetDirectory, bool NoConfirmation)
{
  TTerminal * Terminal = GetTerminal();
  UnicodeString Target =
    UnixIncludeTrailingBackslash(TargetDirectory.IsEmpty() ? FRemotePanel->GetPath() : TargetDirectory);
  UnicodeString FileMask = AnyMask;
  bool DirectCopy = Terminal->IsCapable[fcRemoteCopy] || Terminal->IsCapable[fcSecondaryShell];
  bool Result = NoConfirmation || RemoteTransferDialog(FileList, Target, FileMask, DirectCopy, Move);
  if (Result)
  {
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
    if (!Move && !DirectCopy)
    {
      Configuration->Usage->Inc("RemoteCopyTemp");

      UnicodeString RootTempDir;
      UnicodeString TempDir;

      TemporarilyDownloadFiles(FileList, false, RootTempDir, TempDir);

      ::rtl::TFinally TempFinally([&]()
      {
        if (!RootTempDir.IsEmpty())
        {
          RecursiveDeleteFile(ExcludeTrailingBackslash(RootTempDir));
        }
      });
      std::unique_ptr<TStrings> TemporaryFilesList(new TStringList());
      TMakeLocalFileListParams MakeFileListParam;
      MakeFileListParam.FileList = TemporaryFilesList.get();
      MakeFileListParam.FileTimes = NULL;
      MakeFileListParam.IncludeDirs = true;
      MakeFileListParam.Recursive = false;

      ProcessLocalDirectory(TempDir, ::rtl::MethodPtr(Terminal, &TTerminal::MakeLocalFileList), &MakeFileListParam);

      if (TemporaryFilesList->Count > 0)
      {
        TGUICopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
        CopyParam.FileMask = FileMask;

        Terminal->CopyToRemote(TemporaryFilesList.get(), Target, &CopyParam, cpTemporary, NULL);
      }
    }
    else if (Move)
    {
      Configuration->Usage->Inc("RemoteMove");

      Terminal->MoveFiles(FileList, Target, FileMask, false);
    }
    else
    {
      Configuration->Usage->Inc("RemoteCopyDirect");

      DebugAssert(DirectCopy);
      if (!NeedSecondarySessionForRemoteCopy(FileList) ||
          Terminal->CommandSessionOpened ||
          TTerminalManager::Instance()->ConnectTerminal(Terminal->CommandSession))
      {
        Terminal->CopyFiles(FileList, Target, FileMask, false);
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteRemoteTransfer(bool Move)
{
  if (IsBusy() || (FCurrentPanel != FRemotePanel) || !FRemotePanel->IsReady())
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(FRemotePanel));
  if (FileList->Count > 0)
  {
    RemoteTransferFiles(FileList.get(), Move);
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
  if (FileList->Count > 0)
  {
    DeleteFiles(Panel->GetSide(), FileList.get(), Alternative);
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::DeleteFiles(TOperationSide Side, TStrings * FileList, bool Alternative)
{
  if (AppConfiguration->ConfirmDeleting)
  {
    UnicodeString Message;
    if (FileList->Count == 1)
    {
      UnicodeString FileName =
        (Side == osLocal) ? ExtractFileName(FileList->Strings[0]) : UnixExtractFileName(FileList->Strings[0]);
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
      return false;
    }
  }

  ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
  {
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ::rtl::TFinally Finally([&]()
    {
      DestroyProgressForm();
      if (Side == osLocal)
      {
        FLocalPanel->Reload();
      }
    });
    if (Side == osLocal)
    {
      bool ToRecycleBin = (DeleteToRecycleBin != Alternative);
      for (int Index = 0; Index < FileList->Count; Index++)
      {
        RecursiveDeleteFileChecked(FileList->Strings[Index], ToRecycleBin);
      }
    }
    else
    {
      GetTerminal()->DeleteFiles(FileList, FLAGMASK(Alternative, dfAlternative));
    }
  }
  return true;
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
void TScpCommanderForm::CurrentPropertiesActionExecute()
{
  TDirPanel * Panel = FCurrentPanel;
  if (IsBusy() || !Panel->IsReady())
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(Panel));
  if (FileList->Count > 0)
  {
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    SetProperties(Panel, FileList.get());
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RemoteFindFilesActionExecute()
{
  TTerminal * Terminal = GetTerminal();
  if (!IsBusy() && (Terminal != NULL) && FRemotePanel->IsReady())
  {
    FFileFindTerminal = Terminal;
    ShowFileFindDialog(
      Terminal, FRemotePanel->GetPath(),
      [this](TTerminal * ATerminal, UnicodeString Directory, const TFileMasks & FileMask,
        TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
      {
        DoFindFiles(ATerminal, Directory, FileMask, OnFileFound, OnFindingFile);
      },
      [this](TTerminal * ATerminal, const UnicodeString & Path) { DoFocusRemotePath(ATerminal, Path); },
      [this](TTerminal * ATerminal, TStrings * FileList, TFileOperationFinishedEvent OnFileOperationFinished)
      {
        if (CanOperateOnFoundFiles(ATerminal))
        {
          TValueRestorer<TFileOperationFinishedEvent> OnFileOperationFinishedRestorer(
            FOnFileOperationFinished, OnFileOperationFinished);
          DeleteFiles(osRemote, FileList, false);
        }
      },
      [this](TTerminal * ATerminal, TStrings * FileList, TFileOperationFinishedEvent OnFileOperationFinished)
      {
        if (CanOperateOnFoundFiles(ATerminal))
        {
          TValueRestorer<TFileOperationFinishedEvent> OnFileOperationFinishedRestorer(
            FOnFileOperationFinished, OnFileOperationFinished);
          ExecuteCopyMoveFileOperation(foCopy, osRemote, FileList);
        }
      },
      [this](TTerminal * ATerminal, TStrings * FileList, TFileOperationFinishedEvent OnFileOperationFinished)
      {
        DoEditFoundFiles(ATerminal, FileList, OnFileOperationFinished);
      });
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoFindFiles(TTerminal * ATerminal, UnicodeString Directory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)
{
  if (!IsBusy() && (ATerminal == GetTerminal()))
  {
    Configuration->Usage->Inc(L"FileFinds");
    ::rtl::TFinally UpdateFinally([&]() { UpdateActions(); });
    TAutoNestingCounter BusyCounter(FBusy);
    UpdateActions();
    ATerminal->FilesFind(Directory, FileMask, OnFileFound, OnFindingFile);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoFocusRemotePath(TTerminal * ATerminal, const UnicodeString & Path)
{
  if (!IsBusy() && (ATerminal == GetTerminal()) && FRemotePanel->IsReady())
  {
    activateWindow();
    FRemotePanel->ChangeDirectory(UnixExtractFilePath(Path));
    UnicodeString FileName = UnixExtractFileName(Path);
    // If not found, it can because we have loaded directory from cache and the file is new, try again after reload.
    if (FRemotePanel->GetModel()->FindFile(FileName) < 0)
    {
      FRemotePanel->Reload();
    }
    FRemotePanel->FocusFile(FileName);
    FRemotePanel->GetDirView()->setFocus();
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanOperateOnFoundFiles(TTerminal * ATerminal)
{
  return !IsBusy() && (ATerminal == GetTerminal());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CalculateSize(
  TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats, bool & Close)
{
  TTerminal * Terminal = GetTerminal();
  // terminal can be already closed (e.g. dropped connection)
  if (Terminal != NULL)
  {
    TAutoNestingCounter BusyCounter(FBusy);
    try
    {
      TCalculateSizeParams Params;
      Params.Stats = &Stats;
      Terminal->CalculateFilesSize(FileList, Size, Params);
    }
    catch (...)
    {
      if (!Terminal->Active)
      {
        Close = true;
      }
      throw;
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CalculateChecksum(const UnicodeString & Alg,
  TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum, bool & Close)
{
  TTerminal * Terminal = GetTerminal();
  // terminal can be already closed (e.g. dropped connection)
  if ((Terminal != NULL) && EnsureCommandSessionFallback(fcCalculatingChecksum))
  {
    Configuration->Usage->Inc(L"ChecksumCalculated");

    TAutoNestingCounter BusyCounter(FBusy);
    try
    {
      Terminal->CalculateFilesChecksum(Alg, FileList, OnCalculatedChecksum);
    }
    catch (...)
    {
      if (!Terminal->Active)
      {
        Close = true;
      }
      throw;
    }
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanCalculateChecksum()
{
  TTerminal * Terminal = GetTerminal();
  return
    Terminal->IsCapable[fcCalculatingChecksum] ||
    (Terminal->IsCapable[fcSecondaryShell] && !Terminal->IsEncryptingFiles());
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::EnsureCommandSessionFallback(TFSCapability Capability)
{
  TTerminal * Terminal = GetTerminal();
  bool Result = Terminal->IsCapable[Capability] ||
    Terminal->CommandSessionOpened;

  if (!Result)
  {
    // We might get here for example when "checksum" synchronization is requested from command-line
    // on server that supports neither checksum nor shell
    if (!Terminal->IsCapable[fcSecondaryShell])
    {
      NotSupported();
    }
    else if (!GUIConfiguration->ConfirmCommandSession)
    {
      Result = true;
    }
    else
    {
      TMessageParams Params(mpNeverAskAgainCheck);
      const TFileSystemInfo & FileSystemInfo = Terminal->GetFileSystemInfo();
      unsigned int Answer = MessageDialog(FMTLOAD(PERFORM_ON_COMMAND_SESSION2,
        (FileSystemInfo.ProtocolName, FileSystemInfo.ProtocolName)), qtConfirmation,
        qaOK | qaCancel, &Params);
      if (Answer == qaNeverAskAgain)
      {
        GUIConfiguration->ConfirmCommandSession = false;
        Result = true;
      }
      else if (Answer == qaOK)
      {
        Result = true;
      }
    }

    if (Result)
    {
      DebugAssert(!Terminal->CommandSessionOpened);
      TAutoNestingCounter BusyCounter(FBusy);
      Result = TTerminalManager::Instance()->ConnectTerminal(Terminal->CommandSession);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::SetProperties(TDirPanel * Panel, TStrings * FileList)
{
  bool Result;
  if (Panel == FRemotePanel)
  {
    TTerminal * Terminal = GetTerminal();
    std::unique_ptr<TRemoteTokenList> GroupList;
    std::unique_ptr<TRemoteTokenList> UserList;

    TRemoteProperties CurrentProperties;

    {
      TAutoNestingCounter BusyCounter(FBusy);
      Terminal->LoadFilesProperties(FileList);
    }

    bool CapableGroupChanging = Terminal->IsCapable[fcGroupChanging];
    bool CapableOwnerChanging = Terminal->IsCapable[fcOwnerChanging];
    if (CapableGroupChanging || CapableOwnerChanging)
    {
      if (CapableGroupChanging)
      {
        GroupList.reset(Terminal->Groups->Duplicate());
      }
      if (CapableOwnerChanging)
      {
        UserList.reset(Terminal->Users->Duplicate());
      }
      TRemoteDirectory * Files = Terminal->Files;
      int Count = Files->Count;
      if (Count > 100)
      {
        Count = 100;
      }
      for (int Index = 0; Index < Count; Index++)
      {
        TRemoteFile * File = Files->Files[Index];
        if (CapableGroupChanging)
        {
          GroupList->AddUnique(File->Group);
        }
        if (CapableOwnerChanging)
        {
          UserList->AddUnique(File->Owner);
        }
      }

      // if we haven't collected tokens for all files in current directory,
      // make sure we collect them at least for all selected files.
      // (note that so far the files in FileList has to be from current direcotry)
      if (Count < Files->Count)
      {
        for (int Index = 0; Index < FileList->Count; Index++)
        {
          TRemoteFile * File = static_cast<TRemoteFile *>(FileList->Objects[Index]);
          if (CapableGroupChanging)
          {
            GroupList->AddUnique(File->Group);
          }
          if (CapableOwnerChanging)
          {
            UserList->AddUnique(File->Owner);
          }
        }
      }
    }

    CurrentProperties = TRemoteProperties::CommonProperties(FileList);

    bool CapableAclChanging = Terminal->IsCapable[fcAclChangingFiles];
    if (CapableAclChanging && TRemoteFileList::AnyDirectory(FileList))
    {
      CapableAclChanging = false;
      CurrentProperties.Valid = CurrentProperties.Valid >> vpRights;
    }

    int Flags = 0;
    if (Terminal->IsCapable[fcModeChanging]) Flags |= cpMode;
    if (CapableAclChanging) Flags |= cpAcl;
    if (CapableOwnerChanging) Flags |= cpOwner;
    if (CapableGroupChanging) Flags |= cpGroup;

    TCalculateChecksumEvent CalculateChecksumEvent;
    if (CanCalculateChecksum())
    {
      CalculateChecksumEvent =
        [this](const UnicodeString & Alg, TStrings * AFileList, TCalculatedChecksumEvent OnCalculatedChecksum, bool & Close)
        {
          CalculateChecksum(Alg, AFileList, OnCalculatedChecksum, Close);
        };
    }

    std::unique_ptr<TStrings> ChecksumAlgs(new TStringList());
    Terminal->GetSupportedChecksumAlgs(ChecksumAlgs.get());

    int Options =
      FLAGMASK(Terminal->IsCapable[fcGroupOwnerChangingByID], poUserGroupByID) |
      FLAGMASK(Terminal->IsCapable[fcTags], poTags);

    TRemoteProperties NewProperties = CurrentProperties;
    Result =
      DoPropertiesDialog(FileList, FRemotePanel->GetPath(),
        GroupList.get(), UserList.get(), ChecksumAlgs.get(), &NewProperties, Flags, Options,
        [this](TStrings * AFileList, __int64 & Size, TCalculateSizeStats & Stats, bool & Close)
        {
          CalculateSize(AFileList, Size, Stats, Close);
        },
        CalculateChecksumEvent);
    if (Result)
    {
      NewProperties = TRemoteProperties::ChangedProperties(CurrentProperties, NewProperties);
      TAutoNestingCounter BusyCounter(FBusy);
      UpdateActions();
      ::rtl::TFinally Finally([&]() { DestroyProgressForm(); });
      Terminal->ChangeFilesProperties(FileList, &NewProperties);
    }
  }
  else
  {
    // Upstream shows the properties dialog of the Windows shell; the
    // dialog of the remote files shows the local ones, their permissions
    // can be changed
    std::unique_ptr<TStrings> LocalFileList(CreateLocalFileList(FileList));
    TRemoteProperties CurrentProperties = TRemoteProperties::CommonProperties(LocalFileList.get());
    TRemoteProperties NewProperties = CurrentProperties;
    std::unique_ptr<TStrings> ChecksumAlgs(new TStringList());
    Result =
      DoPropertiesDialog(LocalFileList.get(), ExcludeTrailingBackslash(FLocalPanel->GetPath()),
        NULL, NULL, ChecksumAlgs.get(), &NewProperties, cpMode, 0,
        [](TStrings * AFileList, __int64 & Size, TCalculateSizeStats & Stats, bool & /*Close*/)
        {
          CalculateLocalFilesSize(AFileList, Size, Stats);
        },
        nullptr);
    if (Result)
    {
      NewProperties = TRemoteProperties::ChangedProperties(CurrentProperties, NewProperties);
      ::rtl::TFinally Finally([&]() { FLocalPanel->Reload(); });
      ChangeLocalFilesProperties(LocalFileList.get(), NewProperties);
    }
  }
  return Result;
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
void TScpCommanderForm::CreateProgressForm(TSynchronizeProgress * SynchronizeProgress)
{
  DebugAssert(FProgressForm == nullptr);
  FProgressForm = new TProgressForm(this, true, SynchronizeProgress);
  FProgressForm->ShowAsModal();
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
  // The progress of a synchronization stays over the operations of its items
  else if (!ProgressData.InProgress && (FProgressForm != nullptr) && (FProgressForm->GetSynchronizeProgress() == nullptr))
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
  bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
  TOnceDoneOperation & /*OnceDoneOperation*/)
{
  // The operations on the files of the file find dialog report to it
  if (FOnFileOperationFinished)
  {
    if ((Operation != foCalculateSize) && (Operation != foGetProperties) && (Operation != foCalculateChecksum))
    {
      FOnFileOperationFinished(Side, FileName, Success, NotCancelled);
    }
  }
  // The transferred files are unselected in the source panel, as upstream
  else if (Success && !Temp && !FAutoOperation && ((Operation == foCopy) || (Operation == foMove)))
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

  if (Success && (FSynchronizeController != NULL))
  {
    if (Operation == foCopy)
    {
      DebugAssert(Side == osLocal);
      FSynchronizeController->LogOperation(soUpload, FileName);
    }
    else if (Operation == foDelete)
    {
      DebugAssert(Side == osRemote);
      FSynchronizeController->LogOperation(soDelete, FileName);
    }
  }
}
//---------------------------------------------------------------------------
unsigned int TScpCommanderForm::MoreMessageDialog(const UnicodeString & Message, TStrings * MoreMessages,
  TQueryType Type, unsigned int Answers, const TMessageParams * Params)
{
  if ((FOnFeedSynchronizeError != nullptr) &&
      (Params != NULL) && FLAGSET(Params->Params, mpAllowContinueOnError))
  {
    FOnFeedSynchronizeError(Message, MoreMessages, Type);
    return ContinueAnswer(Answers);
  }
  else
  {
    return ::MoreMessageDialog(Message, MoreMessages, Type, Answers, Params);
  }
}
//---------------------------------------------------------------------------
