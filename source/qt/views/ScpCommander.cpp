// The main window with the Commander interface.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <EditorManager.h>
#include <ExportedFiles.h>
#include <FileSystems.h>
#include <LocalFiles.h>
#include <TaskbarProgress.h>
#include <TerminalManager.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "HistoryComboBox.h"
#include "PathLabel.h"
#include "ProgressForm.h"
#include "ScpCommander.h"
#include "Toolbars.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QKeyEvent>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QProcess>
#include <QSplitter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTabBar>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QTreeWidget>
//---------------------------------------------------------------------------
TScpCommanderForm::TScpCommanderForm(QWidget * Parent) :
  QMainWindow(Parent)
{
  setObjectName(QStringLiteral("ScpCommanderForm"));
  // upstream's 1090 pixels, for the wider fonts of the desktops
  resize(1200, 720);
  FProgressForm = nullptr;
  FSynchronizeProgressForm = nullptr;
  FManagedSession = NULL;
  FPendingTempSpaceWarn = false;
  FUpdatingSessionTabs = false;
  FNewSessionNesting = 0;
  FFirstTerminal = true;
  FBusy = 0;
  FUpdatingTransferList = 0;
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
  FQueueFileListSplitter = nullptr;
  QueueFileList = nullptr;
  FDragSourcePanel = NULL;
  FDragging = false;
  FDragExtracted = false;
  FClipboardTerminal = NULL;
  FNotifications.reset(new TNotifications(nullptr));
  FTaskbarProgress.reset(new TTaskbarProgress());
  FTrayIcon = nullptr;
  FTransferList = nullptr;
  FHotkeysToolbar = nullptr;
  FCustomCommandsToolbar = nullptr;
  FCustomCommandsToolbarMenu = nullptr;
  FCommandLineToolbar = nullptr;
  FCommandLineCombo = nullptr;
  CreateEditorManager();

  // The tabs of the sessions, the file panels below them, the queue below
  // them
  QWidget * Central = new QWidget(this);
  Central->setObjectName(QStringLiteral("CentralWidget"));
  QVBoxLayout * CentralLayout = new QVBoxLayout(Central);
  CentralLayout->setContentsMargins(0, 0, 0, 0);
  CentralLayout->setSpacing(0);
  CreateSessionsPageControl();
  CentralLayout->addWidget(SessionsPageControl);
  FQueueSplitter = new QSplitter(Qt::Vertical, Central);
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
  connect(FQueueSplitter, &QSplitter::splitterMoved, this, [this]()
  {
    AppConfiguration->QueueViewHeight = QueuePanel->height();
  });
  QueuePanel->setVisible(false);
  CentralLayout->addWidget(FQueueSplitter, 1);
  setCentralWidget(Central);
  FCurrentPanel = FLocalPanel;
  FSynchronisingBrowse = false;

  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    connect(Panel, &TDirPanel::Entered, this, [this](TDirPanel * APanel) { SetCurrentPanel(APanel); });
    connect(Panel, &TDirPanel::DirectoryChanged, this, [this](TDirPanel * APanel)
    {
      TransferPresetAutoSelect();
      ExecuteHandler([this, APanel]() { SynchronizeBrowsing(APanel); });
    });
    Panel->OnHomeDirectory = [this](TDirPanel * APanel) { return SynchronizedHomeDirectory(APanel); };
    connect(Panel, &TDirPanel::OpenDirectoryRequested, this, [this](TDirPanel * APanel) { OpenDirectory(APanel->GetSide()); });
    connect(Panel, &TDirPanel::FileActivated, this, [this](TDirPanel * APanel) { FileActivated(APanel); });
    connect(Panel, &TDirPanel::AddBookmarkRequested, this, [this](TDirPanel * APanel) { AddBookmark(APanel->GetSide()); });
    connect(Panel, &TDirPanel::BookmarksMenuRequested, this,
      [this](TDirPanel * APanel, QMenu * Menu) { CreateOpenDirMenu(APanel, Menu); });
    connect(Panel, &TDirPanel::ToggleHiddenFilesRequested, this, [this]() { ShowHiddenFilesAction->trigger(); });
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
    connect(Panel, &TDirPanel::ContextPopupRequested, this, [this](TDirPanel * APanel, bool OnFile, const QPoint & Pos)
    {
      ExecuteHandler([this, APanel, OnFile, Pos]() { PanelContextPopup(APanel, OnFile, Pos); });
    });
    QTreeView * DirView = Panel->GetDirView();
    DirView->installEventFilter(this);
    connect(DirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() { UpdateActions(); });
    connect(DirView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this]() { UpdateActions(); });
    connect(DirView->model(), &QAbstractItemModel::modelReset, this, [this]() { UpdateActions(); });
  }
  connect(FRemotePanel, &TRemoteDirPanel::NewSessionRequested, this, [this]() { ExecuteHandler([this]() { NewSession(); }); });
  connect(FRemotePanel, &TRemoteDirPanel::ReconnectRequested, this,
    [this]() { ExecuteHandler([this]() { ReconnectSessionActionExecute(); }); });
  connect(FRemotePanel, &TRemoteDirPanel::FilesLoaded, this, [this]()
  {
    if (FManagedSession != NULL)
    {
      FManagedSession->DirectoryLoaded = Now();
    }
  });
  // The path in the title of the window
  connect(FRemotePanel, &TDirPanel::DirectoryChanged, this, [this]() { UpdateCaption(); });

  NewSessionAction = CreateAction("NewSessionAction", tr("&New Tab"), QStringLiteral("network-connect"),
    { QKeySequence(Qt::CTRL | Qt::Key_N) }, &TScpCommanderForm::NewSessionActionExecute);
  NewSessionAction->setToolTip(tr("Open new tab"));
  CloseTabAction = CreateAction("CloseTabAction", tr("&Close Tab"), QStringLiteral("tab-close"),
    { QKeySequence(Qt::CTRL | Qt::Key_W), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D) },
    &TScpCommanderForm::CloseTabActionExecute);
  CloseTabAction->setToolTip(tr("Close the current tab"));
  DuplicateTabAction = CreateAction("DuplicateTabAction", tr("Du&plicate Tab"), QStringLiteral("tab-duplicate"),
    {}, &TScpCommanderForm::DuplicateTabActionExecute);
  DuplicateTabAction->setToolTip(tr("Open new tab with the same folder"));
  RenameTabAction = CreateAction("RenameTabAction", tr("&Rename Tab"), QString(),
    {}, &TScpCommanderForm::RenameTabActionExecute);
  // Upstream cycles the tabs by Ctrl+Tab in its key handler
  NextTabAction = CreateAction("NextTabAction", tr("&Next Tab"), QString(),
    { QKeySequence(Qt::CTRL | Qt::Key_Tab) }, &TScpCommanderForm::NextTabActionExecute);
  PreviousTabAction = CreateAction("PreviousTabAction", tr("&Previous Tab"), QString(),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Backtab) },
    &TScpCommanderForm::PreviousTabActionExecute);
  DisconnectSessionAction = CreateAction("DisconnectSessionAction", tr("&Disconnect Session"), QStringLiteral("network-disconnect"),
    {}, &TScpCommanderForm::DisconnectSessionActionExecute);
  DisconnectSessionAction->setToolTip(tr("Disconnect the current session, but keep the tab opened"));
  ReconnectSessionAction = CreateAction("ReconnectSessionAction", tr("&Reconnect Session"), QStringLiteral("view-refresh"),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_R) }, &TScpCommanderForm::ReconnectSessionActionExecute);
  ReconnectSessionAction->setToolTip(tr("Reconnect the current disconnected session"));
  SaveCurrentSessionAction2 = CreateAction("SaveCurrentSessionAction2", tr("&Save Session as Site..."),
    QStringLiteral("document-save"), {}, &TScpCommanderForm::SaveCurrentSessionActionExecute);
  SaveCurrentSessionAction2->setToolTip(tr("Save current session as site"));
  ColorMenuAction2 = CreateAction("ColorMenuAction2", tr("C&olor"), QStringLiteral("color-management"),
    {}, &TScpCommanderForm::ColorMenuActionExecute);
  ColorMenuAction2->setToolTip(tr("Change color of the current tab"));
  FileSystemInfoAction = CreateAction("FileSystemInfoAction", tr("Server/Protocol &Information"),
    QStringLiteral("documentinfo"), {}, &TScpCommanderForm::FileSystemInfoActionExecute);
  FileSystemInfoAction->setToolTip(tr("Display server/protocol information"));
  SessionGenerateUrlAction2 = CreateAction("SessionGenerateUrlAction2", tr("Generate Session &URL/Code..."), QString(),
    {}, &TScpCommanderForm::SessionGenerateUrlActionExecute);
  SessionGenerateUrlAction2->setToolTip(tr("Generate URL or code for current session"));
  FileGenerateUrlAction2 = CreateAction("FileGenerateUrlAction2", tr("Generate File &URL..."), QString(),
    {}, &TScpCommanderForm::FileGenerateUrlActionExecute);
  FileGenerateUrlAction2->setToolTip(tr("Generate URL's of selected file(s)"));
  ChangePasswordAction = CreateAction("ChangePasswordAction", tr("&Change Password..."), QString(),
    {}, &TScpCommanderForm::ChangePasswordActionExecute);
  ChangePasswordAction->setToolTip(tr("Change account password"));
  PrivateKeyUploadAction = CreateAction("PrivateKeyUploadAction", tr("&Install Public Key into Server..."), QString(),
    {}, &TScpCommanderForm::PrivateKeyUploadActionExecute);
  PrivateKeyUploadAction->setToolTip(tr("Install public key for authentication into the server"));
  ClearCachesAction = CreateAction("ClearCachesAction", tr("Clea&r Caches"), QString(),
    {}, &TScpCommanderForm::ClearCachesActionExecute);
  ClearCachesAction->setToolTip(tr("Clear directory listing and directory changes caches"));
  SaveWorkspaceAction = CreateAction("SaveWorkspaceAction", tr("Save Wor&kspace..."), QStringLiteral("document-save-all"),
    {}, &TScpCommanderForm::SaveWorkspaceActionExecute);
  SaveWorkspaceAction->setToolTip(tr("Save workspace"));
  SiteManagerAction = CreateAction("SiteManagerAction", tr("Site &Manager..."), QString(),
    {}, &TScpCommanderForm::NewSessionActionExecute);
  SiteManagerAction->setToolTip(tr("Opens site manager"));
  SessionsTabsAction2 = CreateAction("SessionsTabsAction2", tr("T&abs"), QString(),
    {}, &TScpCommanderForm::SessionsTabsActionExecute);
  SessionsTabsAction2->setToolTip(tr("Hide/show tabs"));
  SessionsTabsAction2->setCheckable(true);
  SessionsTabsAction2->setChecked(AppConfiguration->SessionsTabs);
  PreferencesAction = CreateAction("PreferencesAction", tr("&Preferences..."), QStringLiteral("configure"),
    { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_P) }, &TScpCommanderForm::PreferencesActionExecute);
  PreferencesAction->setToolTip(tr("Show/change user preferences"));
  TableOfContentsAction = CreateAction("TableOfContentsAction", tr("&Contents"), QStringLiteral("help-contents"),
    { QKeySequence(Qt::Key_F1) }, &TScpCommanderForm::TableOfContentsActionExecute);
  TableOfContentsAction->setToolTip(tr("Opens the documentation in the web browser"));
  HomepageAction = CreateAction("HomepageAction", tr("Product &Homepage"), QStringLiteral("internet-web-browser"),
    {}, &TScpCommanderForm::HomepageActionExecute);
  HomepageAction->setToolTip(tr("Opens web browser and points it to project homepage"));
  ForumPageAction = CreateAction("ForumPageAction", tr("&Support Forum"), QString(),
    {}, &TScpCommanderForm::ForumPageActionExecute);
  ForumPageAction->setToolTip(tr("Opens web browser and points it to support forum page"));
  AboutAction = CreateAction("AboutAction", tr("&About..."), QStringLiteral("help-about"),
    {}, &TScpCommanderForm::AboutActionExecute);
  AboutAction->setToolTip(tr("Show About box"));
  SessionsPageControl->setVisible(AppConfiguration->SessionsTabs);
  for (int Index = 0; Index < 10; Index++)
  {
    QAction * Action = new QAction(this);
    Action->setObjectName(QStringLiteral("OpenedTabAction%1").arg(Index + 1));
    Action->setShortcut(QKeySequence(Qt::ALT | static_cast<Qt::Key>((Index < 9) ? (Qt::Key_1 + Index) : Qt::Key_0)));
    addAction(Action);
    connect(Action, &QAction::triggered, this, [this, Index]() { ExecuteHandler([this, Index]() { OpenedTabActionExecute(Index); }); });
    FOpenedTabActions.push_back(Action);
  }
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
  CurrentEditWithAction = CreateAction("CurrentEditWithAction", tr("Edit &With..."), QString(),
    {}, &TScpCommanderForm::CurrentEditWithActionExecute);
  CurrentEditWithAction->setToolTip(tr("Edit selected file(s) using editor of your choice"));
  EditorListCustomizeAction = CreateAction("EditorListCustomizeAction", tr("&Configure..."), QStringLiteral("configure"),
    {}, &TScpCommanderForm::EditorListCustomizeActionExecute);
  EditorListCustomizeAction->setToolTip(tr("Customize editors"));
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
  SynchronizeBrowsingAction2 = CreateAction("SynchronizeBrowsingAction2", tr("Synchronize &Browsing"),
    QStringLiteral("view-split-left-right"), { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_B) },
    &TScpCommanderForm::SynchronizeBrowsingActionExecute);
  SynchronizeBrowsingAction2->setToolTip(tr("Synchronize browsing between both panels"));
  SynchronizeBrowsingAction2->setCheckable(true);
  SynchronizeAction = CreateAction("SynchronizeAction", tr("&Keep Remote Directory up to Date..."),
    QStringLiteral("view-refresh"), { QKeySequence(Qt::CTRL | Qt::Key_U) }, &TScpCommanderForm::SynchronizeActionExecute);
  ConsoleAction = CreateAction("ConsoleAction", tr("Open &Terminal"), QStringLiteral("utilities-terminal"),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T) }, &TScpCommanderForm::ConsoleActionExecute);
  // Upstream's "Open in PuTTY": PuTTY or OpenSSH in a terminal emulator
  PuttyAction = CreateAction("PuttyAction", tr("Open in SSH &Client"), QStringLiteral("utilities-terminal"),
    { QKeySequence(Qt::CTRL | Qt::Key_P) }, &TScpCommanderForm::PuttyActionExecute);
  PuttyAction->setToolTip(tr("Open the session in an SSH client (PuTTY, or ssh in a terminal emulator)"));
  CustomCommandsEnterAction = CreateAction("CustomCommandsEnterAction", tr("&Enter..."), QString(),
    {}, &TScpCommanderForm::AdHocCustomCommand);
  CustomCommandsLastAction = CreateAction("CustomCommandsLastAction", QStringLiteral("CustomCommandsLastAction"), QString(),
    {}, &TScpCommanderForm::LastCustomCommand);
  CustomCommandsLastAction->setVisible(false);
  CustomCommandsCustomizeAction = CreateAction("CustomCommandsCustomizeAction", tr("&Customize..."), QString(),
    {}, &TScpCommanderForm::CustomCommandsCustomize);
  CurrentCopyToClipboardAction2 = CreateAction("CurrentCopyToClipboardAction2", tr("&Copy to Clipboard"),
    QStringLiteral("edit-copy"), { QKeySequence(Qt::CTRL | Qt::Key_C) }, &TScpCommanderForm::CurrentCopyToClipboardActionExecute);
  PasteAction3 = CreateAction("PasteAction3", tr("&Paste from Clipboard"), QStringLiteral("edit-paste"),
    { QKeySequence(Qt::CTRL | Qt::Key_V) }, &TScpCommanderForm::PasteActionExecute);
  FileListToClipboardAction = CreateAction("FileListToClipboardAction", tr("Copy to &Clipboard"), QString(),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C) }, &TScpCommanderForm::FileListToClipboardActionExecute);
  FullFileListToClipboardAction = CreateAction("FullFileListToClipboardAction", tr("Copy to Clipboard (Include &Paths)"),
    QString(), { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_C) }, &TScpCommanderForm::FullFileListToClipboardActionExecute);
  LocalPathToClipboardAction2 = CreateAction("LocalPathToClipboardAction2", tr("Copy &Path to Clipboard"), QString(),
    { QKeySequence(Qt::CTRL | Qt::Key_BracketLeft) }, &TScpCommanderForm::LocalPathToClipboardActionExecute);
  RemotePathToClipboardAction2 = CreateAction("RemotePathToClipboardAction2", tr("Copy &Path to Clipboard"), QString(),
    { QKeySequence(Qt::CTRL | Qt::Key_BracketRight) }, &TScpCommanderForm::RemotePathToClipboardActionExecute);
  ShowHiddenFilesAction = CreateAction("ShowHiddenFilesAction", tr("Show/Hide &Hidden Files"), QString(),
    { QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_H) }, &TScpCommanderForm::ShowHiddenFilesActionExecute);
  ShowHiddenFilesAction->setCheckable(true);
  ShowHiddenFilesAction->setChecked(AppConfiguration->ShowHiddenFiles);
  CreateMarkActions();
  CreateFileCommandActions();
  CreateSideActions();
  CreateToolbarActions();

  // The toolbars of the window come with the interface (ApplyInterface)
  CreateMenus();
  UpdateBookmarkShortCuts();
  UpdateCustomCommandShortCuts();

  // upstream's status bar: the notes, the protocol, the security and the
  // time of the session; a double-click shows the server information
  FSessionStatusLabel = new QLabel(statusBar());
  FSessionStatusLabel->setObjectName(QStringLiteral("SessionStatusLabel"));
  FSecurityStatusLabel = new QLabel(statusBar());
  FSecurityStatusLabel->setObjectName(QStringLiteral("SecurityStatusLabel"));
  FDurationStatusLabel = new QLabel(statusBar());
  FDurationStatusLabel->setObjectName(QStringLiteral("DurationStatusLabel"));
  for (QLabel * Label : { FSessionStatusLabel, FSecurityStatusLabel, FDurationStatusLabel })
  {
    Label->setAlignment(Qt::AlignCenter);
    Label->setContentsMargins(6, 0, 6, 0);
    Label->installEventFilter(this);
    statusBar()->addPermanentWidget(Label);
  }
  int SecurityIconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
  FSecurityStatusLabel->setPixmap(QIcon::fromTheme(QStringLiteral("object-locked")).pixmap(SecurityIconSize));
  FSecurityStatusLabel->setVisible(false);
  FDurationStatusLabel->setVisible(false);
  FDurationStatusLabel->setMinimumWidth(fontMetrics().horizontalAdvance(QStringLiteral("00:00:00")) + 16);

  FLocalPanel->ApplyConfiguration();
  FRemotePanel->ApplyConfiguration();
  SessionsPageControl->setElideMode(AppConfiguration->SessionTabCaptionTruncation ? Qt::ElideRight : Qt::ElideNone);
  FInterface = AppConfiguration->Interface;
  RestoreWindowParams();
  ApplyInterface();
  FLocalPanel->ChangeDirectory(ToUnicodeString(QDir::homePath()));

  SessionChanged();
  FLocalPanel->GetDirView()->setFocus();

  connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() { ClipboardChanged(); });

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
  // However the application ends (upstream's StoreParams)
  ExecuteHandler([this]() { StorePanelsParams(); });
  // The sessions outlive the window
  FRemotePanel->SetTerminal(NULL);
  // The remote files of the clipboard cannot be downloaded any more
  ClipboardClear();
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

    if (AppConfiguration->RefreshRemotePanel)
    {
      if ((FManagedSession != NULL) && (FManagedSession->Status == ssOpened) && FRemotePanel->IsReady() &&
          (Now() - FManagedSession->DirectoryLoaded > AppConfiguration->RefreshRemotePanelInterval))
      {
        FRemotePanel->Reload();
      }
    }
  }

  if (FQueueStatusInvalidated)
  {
    UpdateQueueStatus(false);
  }

  RefreshQueueItems();
  UpdateDurationStatus();
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
  connect(Action, &QAction::triggered, this, [this, Method]() { ExecuteHandler([this, Method]() { (this->*Method)(); }); });
  return Action;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateMenus()
{
  // The menus of both interfaces, arranged in the menu bar by the
  // interface (ArrangeMenus)
  menuBar()->setObjectName(QStringLiteral("MenuToolbar"));
  auto NewMenu = [this](const QString & Title, const char * Name)
  {
    QMenu * Menu = new QMenu(Title, this);
    Menu->setObjectName(QString::fromLatin1(Name));
    return Menu;
  };

  FLocalMenu = NewMenu(tr("&Local"), "LocalMenuButton");
  FLocalPanel->FillMenu(FLocalMenu, LocalPathToClipboardAction2);

  FMarkMenu = CreateMarkMenu();

  QMenu * NewFilesMenu = NewMenu(tr("&New"), "NewMenu");
  NewFilesMenu->addAction(NewFileAction);
  NewFilesMenu->addAction(CurrentCreateDirAction);
  NewFilesMenu->addAction(NewLinkAction);
  QMenu * EditWithMenu = NewMenu(tr("Edi&t With"), "CurrentEditWithMenu");
  connect(EditWithMenu, &QMenu::aboutToShow, this, [this, EditWithMenu]()
  {
    EditWithMenu->clear();
    ExecuteHandler([this, EditWithMenu]() { CreateEditWithMenu(EditWithMenu, osCurrent); });
  });
  QMenu * FileNamesMenu = NewMenu(tr("&File Names"), "FileNamesMenu");
  FileNamesMenu->addAction(FileListToClipboardAction);
  FileNamesMenu->addAction(FullFileListToClipboardAction);
  FileNamesMenu->addAction(FileGenerateUrlAction2);
  QMenu * CustomCommandsMenu = NewMenu(tr("&Custom Commands"), "CustomCommandsMenu");
  connect(CustomCommandsMenu, &QMenu::aboutToShow, this, [this, CustomCommandsMenu]()
  {
    CustomCommandsMenu->clear();
    CreateCustomCommandsMenu(CustomCommandsMenu);
  });
  QMenu * QueueMenu = CreateQueueMenu();

  FFilesMenu = NewMenu(tr("&Files"), "FilesMenu");
  FFilesMenu->addMenu(NewFilesMenu);
  FFilesMenu->addSeparator();
  FFilesMenu->addAction(CurrentOpenAction);
  FFilesMenu->addAction(CurrentEditAction);
  FFilesMenu->addMenu(EditWithMenu);
  FFilesMenu->addAction(CurrentAddEditLinkAction);
  FFilesMenu->addSeparator();
  FFilesMenu->addAction(CurrentCopyAction);
  FFilesMenu->addAction(CurrentMoveAction);
  FFilesMenu->addAction(RemoteCopyToAction);
  FFilesMenu->addAction(RemoteMoveToAction);
  FFilesMenu->addAction(CurrentRenameAction);
  FFilesMenu->addAction(CurrentDeleteAction);
  FFilesMenu->addAction(CurrentDeleteAlternativeAction);
  FFilesMenu->addSeparator();
  FFilesMenu->addAction(CurrentCopyToClipboardAction2);
  FFilesMenu->addAction(PasteAction3);
  FFilesMenu->addSeparator();
  FFilesMenu->addMenu(FileNamesMenu);
  FFilesMenu->addMenu(FLockingMenu);
  FFilesMenu->addSeparator();
  FFilesMenu->addAction(CurrentPropertiesAction);
  FFilesMenu->addAction(CalculateDirectorySizesAction);

  FCommandsMenu = NewMenu(tr("&Commands"), "CommandsMenu");
  FCommandsMenu->addAction(CurrentCreateDirAction);
  FCommandsMenu->addAction(RemoteFindFilesAction2);
  FCommandsMenu->addAction(CompareDirectoriesAction2);
  FCommandsMenu->addAction(FullSynchronizeAction2);
  FCommandsMenu->addAction(SynchronizeBrowsingAction2);
  FCommandsMenu->addAction(SynchronizeAction);
  FCommandsMenu->addSeparator();
  FCommandsMenu->addMenu(CustomCommandsMenu);
  FCommandsMenu->addAction(ConsoleAction);
  FCommandsMenu->addAction(PuttyAction);
  FCommandsMenu->addSeparator();
  FCommandsMenu->addMenu(QueueMenu);
  FCommandsMenu->addSeparator();
  FCommandsMenu->addAction(ClearCachesAction);
  FCommandsMenu->addSeparator();
  FCommandsMenu->addAction(CloseApplicationAction);

  FTabsMenu = NewMenu(tr("&Tabs"), "TabsMenu");
  FTabsMenu->addAction(NewSessionAction);
  FTabsMenu->addAction(CloseTabAction);
  FTabsMenu->addAction(DuplicateTabAction);
  FTabsMenu->addAction(RenameTabAction);
  FTabsMenu->addSeparator();
  FTabsMenu->addAction(ColorMenuAction2);
  FTabsMenu->addSeparator();
  FTabsMenu->addAction(DisconnectSessionAction);
  FTabsMenu->addAction(ReconnectSessionAction);
  FTabsMenu->addAction(SaveCurrentSessionAction2);
  FTabsMenu->addSeparator();
  FTabsMenu->addAction(FileSystemInfoAction);
  FTabsMenu->addAction(SessionGenerateUrlAction2);
  FTabsMenu->addAction(ChangePasswordAction);
  FTabsMenu->addAction(PrivateKeyUploadAction);
  FTabsMenu->addSeparator();
  QMenu * OpenedTabsMenu = FTabsMenu->addMenu(tr("&Opened Tabs"));
  OpenedTabsMenu->setObjectName(QStringLiteral("OpenedTabsMenu"));
  connect(OpenedTabsMenu, &QMenu::aboutToShow, this, [this, OpenedTabsMenu]()
  {
    OpenedTabsMenu->clear();
    CreateOpenedTabsMenu(OpenedTabsMenu);
  });
  QMenu * WorkspacesMenu = FTabsMenu->addMenu(tr("&Workspaces"));
  WorkspacesMenu->setObjectName(QStringLiteral("WorkspacesMenu"));
  connect(WorkspacesMenu, &QMenu::aboutToShow, this, [this, WorkspacesMenu]()
  {
    WorkspacesMenu->clear();
    ExecuteHandler([this, WorkspacesMenu]() { CreateWorkspacesMenu(WorkspacesMenu); });
  });
  FTabsMenu->addAction(SaveWorkspaceAction);
  FTabsMenu->addSeparator();
  QMenu * SavedSessionsMenu = FTabsMenu->addMenu(tr("Si&tes"));
  SavedSessionsMenu->setObjectName(QStringLiteral("SavedSessionsMenu"));
  SavedSessionsMenu->setIcon(QIcon::fromTheme(QStringLiteral("network-server")));
  SavedSessionsMenu->setToolTip(tr("Open site"));
  FSavedSessionsMenu = SavedSessionsMenu;
  connect(SavedSessionsMenu, &QMenu::aboutToShow, this, [this, SavedSessionsMenu]()
  {
    ExecuteHandler([this, SavedSessionsMenu]()
    {
      StoredSessions->Reload();
      CreateSessionListMenuLevel(SavedSessionsMenu, 0, 0);
    });
  });

  QMenu * QueueViewMenu = NewMenu(tr("&Queue"), "QueueViewMenu");
  QueueViewMenu->addActions(FQueueShowGroup->actions());
  QueueViewMenu->addSeparator();
  QueueViewMenu->addAction(QueueToolbarAction);
  QueueViewMenu->addAction(QueueFileListAction);
  // The menu of the queue button (upstream's QueueToggleShowAction)
  FQueueShowMenu = NewMenu(QString(), "QueueShowMenu");
  FQueueShowMenu->addActions(FQueueShowGroup->actions());
  FQueueShowMenu->addSeparator();
  FQueueShowMenu->addAction(QueueFileListAction);

  // upstream's Toolbars menu (of the interface's toolbars)
  FToolbarsMenu = NewMenu(tr("&Toolbars"), "ToolbarsMenu");
  connect(FToolbarsMenu, &QMenu::aboutToShow, this, [this]()
  {
    FToolbarsMenu->clear();
    FillToolbarsMenu(FToolbarsMenu);
  });

  FOptionsMenu = NewMenu(tr("&Options"), "OptionsMenu");
  FOptionsMenu->addMenu(FToolbarsMenu);
  FOptionsMenu->addMenu(FLocalPanelMenu);
  FOptionsMenu->addMenu(FRemotePanelMenu);
  UpdatePanelMenus();
  FOptionsMenu->addSeparator();
  FOptionsMenu->addAction(SessionsTabsAction2);
  FOptionsMenu->addAction(CommandLinePanelAction);
  FOptionsMenu->addAction(StatusBarAction);
  FOptionsMenu->addMenu(QueueViewMenu);
  FOptionsMenu->addSeparator();
  FOptionsMenu->addAction(ShowHiddenFilesAction);
  FOptionsMenu->addAction(PreferencesAction);

  FHelpMenu = NewMenu(tr("&Help"), "HelpMenu");
  FHelpMenu->addAction(TableOfContentsAction);
  FHelpMenu->addSeparator();
  FHelpMenu->addAction(HomepageAction);
  FHelpMenu->addAction(ForumPageAction);
  FHelpMenu->addSeparator();
  FHelpMenu->addAction(AboutAction);

  FRemoteMenu = NewMenu(tr("&Remote"), "RemoteMenuButton");
  FRemotePanel->FillMenu(FRemoteMenu, RemotePathToClipboardAction2);

  // The Explorer's (upstream's ScpExplorer.dfm)
  FExplorerFilesMenu = NewMenu(tr("&Files"), "ExplorerFilesMenu");
  FExplorerFilesMenu->addMenu(NewFilesMenu);
  FExplorerFilesMenu->addSeparator();
  FExplorerFilesMenu->addAction(CurrentOpenAction);
  FExplorerFilesMenu->addAction(CurrentEditAction);
  FExplorerFilesMenu->addMenu(EditWithMenu);
  FExplorerFilesMenu->addAction(CurrentAddEditLinkAction);
  FExplorerFilesMenu->addAction(CurrentDeleteAction);
  FExplorerFilesMenu->addAction(CurrentDeleteAlternativeAction);
  FExplorerFilesMenu->addAction(CurrentRenameAction);
  FExplorerFilesMenu->addAction(CurrentPropertiesAction);
  FExplorerFilesMenu->addAction(CalculateDirectorySizesAction);
  FExplorerFilesMenu->addSeparator();
  FExplorerFilesMenu->addAction(CurrentCopyAction);
  FExplorerFilesMenu->addAction(CurrentMoveAction);
  FExplorerFilesMenu->addAction(RemoteCopyToAction);
  FExplorerFilesMenu->addAction(RemoteMoveToAction);
  FExplorerFilesMenu->addSeparator();
  FExplorerFilesMenu->addAction(CurrentCopyToClipboardAction2);
  FExplorerFilesMenu->addAction(PasteAction3);
  FExplorerFilesMenu->addSeparator();
  FExplorerFilesMenu->addMenu(FileNamesMenu);
  FExplorerFilesMenu->addMenu(FLockingMenu);
  FExplorerFilesMenu->addSeparator();
  FExplorerFilesMenu->addAction(CloseTabAction);
  FExplorerFilesMenu->addAction(CloseApplicationAction);

  FExplorerCommandsMenu = NewMenu(tr("&Commands"), "ExplorerCommandsMenu");
  FExplorerCommandsMenu->addAction(SynchronizeAction);
  FExplorerCommandsMenu->addAction(FullSynchronizeAction2);
  FExplorerCommandsMenu->addAction(RemoteFindFilesAction2);
  FExplorerCommandsMenu->addSeparator();
  FExplorerCommandsMenu->addMenu(QueueMenu);
  FExplorerCommandsMenu->addSeparator();
  FExplorerCommandsMenu->addMenu(CustomCommandsMenu);
  FExplorerCommandsMenu->addAction(RemotePathToClipboardAction2);
  FExplorerCommandsMenu->addSeparator();
  FExplorerCommandsMenu->addAction(ConsoleAction);
  FExplorerCommandsMenu->addAction(PuttyAction);
  FExplorerCommandsMenu->addSeparator();
  FExplorerCommandsMenu->addAction(ClearCachesAction);

  FViewMenu = NewMenu(tr("&View"), "ViewMenu");
  FViewMenu->addMenu(FToolbarsMenu);
  FViewMenu->addAction(SessionsTabsAction2);
  FViewMenu->addAction(StatusBarAction);
  FViewMenu->addMenu(QueueViewMenu);
  FViewMenu->addAction(FRemotePanel->GetTreeAction());
  FViewMenu->addSeparator();
  FRemotePanel->FillMenu(FViewMenu, nullptr);
  FViewMenu->addAction(ShowHiddenFilesAction);
  FViewMenu->addSeparator();
  FViewMenu->addAction(PreferencesAction);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ArrangeMenus()
{
  QMenuBar * Menu = menuBar();
  Menu->clear();
  std::vector<QMenu *> Menus;
  if (IsExplorer())
  {
    Menus = { FExplorerFilesMenu, FExplorerCommandsMenu, FMarkMenu, FTabsMenu, FViewMenu, FHelpMenu };
  }
  else
  {
    Menus = { FLocalMenu, FMarkMenu, FFilesMenu, FCommandsMenu, FTabsMenu, FOptionsMenu, FRemoteMenu, FHelpMenu };
  }
  for (QMenu * AMenu : Menus)
  {
    Menu->addMenu(AMenu);
  }
}
//---------------------------------------------------------------------------
TTerminal * TScpCommanderForm::GetTerminal() const
{
  return FManagedSession;
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
bool TScpCommanderForm::EventFilterPanels(QObject * Object, QEvent * Event)
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
      if (Other->IsReady() && !Other->isHidden())
      {
        Other->GetDirView()->setFocus();
      }
      return true;
    }
    // The shortcuts of the numeric keypad: Qt matches shortcuts without
    // the keypad modifier on xkb (as the keys of the main keyboard)
    if (KeyEvent->modifiers().testFlag(Qt::KeypadModifier))
    {
      QKeyCombination Combination(KeyEvent->modifiers(), static_cast<Qt::Key>(KeyEvent->key()));
      for (QAction * Action : { SelectAction, UnselectAction, InvertSelectionAction, SelectSameExtAction, UnselectSameExtAction })
      {
        if (!Action->shortcut().isEmpty() && (Action->shortcut()[0] == Combination))
        {
          if (Action->isEnabled())
          {
            Action->trigger();
          }
          return true;
        }
      }
    }
  }
  return false;
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
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  bool HasSession = (FManagedSession != NULL);
  CloseTabAction->setEnabled(!Busy && HasSession);
  DuplicateTabAction->setEnabled(!Busy && HasSession);
  RenameTabAction->setEnabled(!Busy && HasSession);
  bool MultipleTabs = (Manager != NULL) && (Manager->Count > 1);
  NextTabAction->setEnabled(!Busy && MultipleTabs);
  PreviousTabAction->setEnabled(!Busy && MultipleTabs);
  DisconnectSessionAction->setEnabled(!Busy && Connected);
  ReconnectSessionAction->setEnabled(!Busy && HasSession && !Connected);
  ReconnectSessionAction->setVisible(HasSession && !Connected);
  SaveCurrentSessionAction2->setEnabled(!Busy && HasSession);
  ColorMenuAction2->setEnabled(!Busy && HasSession);
  FileSystemInfoAction->setEnabled(!Busy && Connected);
  SessionGenerateUrlAction2->setEnabled(!Busy && HasSession);
  FileGenerateUrlAction2->setEnabled(!Busy && HasSelection && Connected && (FCurrentPanel == FRemotePanel));
  ChangePasswordAction->setEnabled(!Busy && CanChangePassword());
  PrivateKeyUploadAction->setEnabled(!Busy && CanPrivateKeyUpload());
  ClearCachesAction->setEnabled(!Busy && Connected && !Terminal->AreCachesEmpty);
  SaveWorkspaceAction->setEnabled(!Busy && HasSession);
  SiteManagerAction->setEnabled(!Busy);
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
  CurrentCopyToClipboardAction2->setEnabled(!Busy && HasSelection && ((FCurrentPanel == FLocalPanel) || Connected));
  PasteAction3->setEnabled(CanPasteFromClipboard());
  FileListToClipboardAction->setEnabled(HasSelection);
  FullFileListToClipboardAction->setEnabled(HasSelection);
  LocalPathToClipboardAction2->setEnabled(FLocalPanel->IsReady());
  RemotePathToClipboardAction2->setEnabled(FRemotePanel->IsReady());
  RemoteFindFilesAction2->setEnabled(!Busy && Connected && FRemotePanel->IsReady());
  FullSynchronizeAction2->setEnabled(!Busy && Connected && FRemotePanel->IsReady());
  SynchronizeAction->setEnabled(!Busy && Connected && FRemotePanel->IsReady());
  SynchronizeBrowsingAction2->setEnabled(!Busy && Connected && !IsExplorer());
  ConsoleAction->setEnabled(!Busy && CanConsole());
  PuttyAction->setEnabled(!Busy && (Manager != NULL) && Manager->CanOpenInPutty());
  UpdateMarkActions();
  UpdateFileCommandActions();
  UpdateSideActions();
  UpdateSortActions();
  if (FCommandLineCombo != nullptr)
  {
    FCommandLineCombo->setEnabled(!Busy && CanConsole());
  }
  CustomCommandsEnterAction->setEnabled(!Busy);
  CustomCommandsCustomizeAction->setEnabled(!Busy);
  UpdateCustomCommandsToolbarState();
  CurrentEditWithAction->setEnabled(CanEdit);
  QueueToggleShowAction->setChecked(!QueuePanel->isHidden());
  QueueToolbarAction->setEnabled(!QueuePanel->isHidden());
  QueueFileListAction->setEnabled(!QueuePanel->isHidden());
  QueueFileListAction->setChecked(!QueueFileList->isHidden());
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
void TScpCommanderForm::PuttyActionExecute()
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  if (!IsBusy() && Manager->CanOpenInPutty())
  {
    Manager->OpenInPutty();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PreferencesActionExecute()
{
  if (!IsBusy())
  {
    // The rules of new presets from the current session
    std::unique_ptr<TCopyParamRuleData> CopyParamRuleData;
    TTerminal * Terminal = GetTerminal();
    if ((Terminal != NULL) && Terminal->Active && FRemotePanel->IsReady())
    {
      CopyParamRuleData.reset(new TCopyParamRuleData());
      CopyParamRuleData->HostName = Terminal->SessionData->HostNameExpanded;
      CopyParamRuleData->UserName = Terminal->SessionData->UserNameExpanded;
      CopyParamRuleData->RemoteDirectory = FRemotePanel->GetPath();
      CopyParamRuleData->LocalDirectory = FLocalPanel->GetPath();
    }
    TPreferencesDialogData PreferencesData;
    PreferencesData.CopyParamRuleData = CopyParamRuleData.get();
    DoPreferencesDialog(pmDefault, &PreferencesData);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ConfigurationChanged()
{
  UpdateTransferList();
  UpdateBookmarkShortCuts();
  UpdateCustomCommandShortCuts();
  // Not from the handler of a button of the toolbar
  QTimer::singleShot(0, this, [this]() { UpdateCustomCommandsToolbar(); });
  UpdatePanelMenus();
  // The preferences
  ShowHiddenFilesAction->setChecked(AppConfiguration->ShowHiddenFiles);
  FLocalPanel->ApplyConfiguration();
  FRemotePanel->ApplyConfiguration();
  SessionsPageControl->setElideMode(AppConfiguration->SessionTabCaptionTruncation ? Qt::ElideRight : Qt::ElideNone);
  UpdateQueueView();
  UpdateCaption();
  if (AppConfiguration->Interface != FInterface)
  {
    // The layout of the interface is kept for the next time
    StorePanelsParams();
    FInterface = AppConfiguration->Interface;
    ApplyInterface();
  }
  else
  {
    UpdatePanelsLayout();
    UpdateKeyboardShortcuts();
  }
  FLocalPanel->SetTreeOnLeft(AppConfiguration->TreeOnLeft);
  FRemotePanel->SetTreeOnLeft(IsExplorer() || AppConfiguration->TreeOnLeft);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ApplyInterface()
{
  bool Explorer = IsExplorer();
  ArrangeMenus();
  CreateToolbars();
  CommandLinePanelAction->setVisible(!Explorer);
  CommandLinePanelAction->setChecked((FCommandLineToolbar != nullptr) && !FCommandLineToolbar->isHidden());
  // The Explorer has one status bar (upstream's), with the remote panel's
  if (Explorer)
  {
    statusBar()->insertWidget(0, FRemotePanel->GetStatusPanel(), 1);
    FRemotePanel->GetStatusPanel()->show();
  }
  else if (FRemotePanel->GetStatusPanel()->parentWidget() != FRemotePanel)
  {
    statusBar()->removeWidget(FRemotePanel->GetStatusPanel());
    FRemotePanel->AttachStatusPanel();
  }
  statusBar()->setVisible(Explorer ? AppConfiguration->ExplorerPanel.StatusBar : AppConfiguration->CommanderStatusBar);
  StatusBarAction->setChecked(!statusBar()->isHidden());
  FLocalPanel->setVisible(!Explorer);
  if (Explorer)
  {
    SetCurrentPanel(FRemotePanel);
    // Without the local panel
    SynchronizeBrowsingAction2->setChecked(false);
  }
  SessionsTabsAction2->setChecked(Explorer ? AppConfiguration->ExplorerSessionsTabs : AppConfiguration->SessionsTabs);
  SessionsPageControl->setVisible(SessionsTabsAction2->isChecked());
  // upstream's RemoteCopyAction of the Explorer
  CurrentCopyAction->setText(Explorer ? tr("&Download...") : tr("&Copy..."));
  CurrentMoveAction->setText(Explorer ? tr("Download and D&elete...") : tr("&Move..."));
  FRemotePanel->SetAddressBar(Explorer, AppConfiguration->ExplorerShowFullAddress);
  // The Explorer has the address bar only
  FRemotePanel->GetPathLabel()->setVisible(!Explorer);
  UpdatePanelsLayout();
  UpdateKeyboardShortcuts();
  RestorePanelsParams();
  UpdateActions();
}
//---------------------------------------------------------------------------
UnicodeString TScpCommanderForm::LocalDirectory()
{
  UnicodeString Result;
  if (IsExplorer())
  {
    Result = AppConfiguration->ExplorerLastLocalTargetDirectory;
    if (Result.IsEmpty() || !DirectoryExists(Result))
    {
      Result = ToUnicodeString(QDir::homePath());
    }
  }
  else
  {
    Result = FLocalPanel->GetPath();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RestorePanelsParams()
{
  // upstream's RestoreParams of the panels
  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    TPanelConfiguration PanelConfiguration = PanelParams(Panel);
    // The tree of the Explorer is left of the files
    bool TreeOnLeft = AppConfiguration->TreeOnLeft || (IsExplorer() && (Panel == FRemotePanel));
    Panel->SetDirViewParams(PanelConfiguration.DirViewParams);
    // The Explorer's is the window's
    Panel->SetStatusBarVisible(PanelConfiguration.StatusBar || (IsExplorer() && (Panel == FRemotePanel)));
    Panel->SetTreeOnLeft(TreeOnLeft);
    Panel->SetTreeVisible(PanelConfiguration.DriveView);
    Panel->SetTreeSize(TreeOnLeft ? PanelConfiguration.DriveViewWidth : PanelConfiguration.DriveViewHeight);
  }
}
//---------------------------------------------------------------------------
TPanelConfiguration TScpCommanderForm::PanelParams(TDirPanel * Panel)
{
  return
    (Panel == FLocalPanel) ? AppConfiguration->LocalPanel :
      (IsExplorer() ? AppConfiguration->ExplorerPanel : AppConfiguration->RemotePanel);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::StorePanelsParams()
{
  Configuration->BeginUpdate();
  ::rtl::TFinally Finally([&]() { Configuration->EndUpdate(); });
  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    TPanelConfiguration PanelConfiguration = PanelParams(Panel);
    bool TreeOnLeft = AppConfiguration->TreeOnLeft || (IsExplorer() && (Panel == FRemotePanel));
    PanelConfiguration.DirViewParams = Panel->GetDirViewParams();
    PanelConfiguration.StatusBar =
      (IsExplorer() && (Panel == FRemotePanel)) ? !statusBar()->isHidden() : Panel->GetStatusBarVisible();
    PanelConfiguration.DriveView = Panel->GetTreeVisible();
    if (Panel->GetTreeVisible())
    {
      (TreeOnLeft ? PanelConfiguration.DriveViewWidth : PanelConfiguration.DriveViewHeight) = Panel->GetTreeSize();
    }
    if (Panel == FLocalPanel)
    {
      AppConfiguration->LocalPanel = PanelConfiguration;
    }
    else if (IsExplorer())
    {
      AppConfiguration->ExplorerPanel = PanelConfiguration;
    }
    else
    {
      AppConfiguration->RemotePanel = PanelConfiguration;
    }
  }
  if (IsExplorer())
  {
    AppConfiguration->ExplorerSessionsTabs = SessionsTabsAction2->isChecked();
  }
  StoreToolbarsLayout();
  StoreWindowParams();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RestoreWindowParams()
{
  // upstream's RestoreFormSize: the size and the maximized state (the
  // compositor places the window)
  UnicodeString Params = IsExplorer() ? AppConfiguration->ExplorerWindowParams : AppConfiguration->CommanderWindowParams;
  if (!Params.IsEmpty())
  {
    CutToChar(Params, L';', true);
    CutToChar(Params, L';', true);
    int Width = StrToIntDef(CutToChar(Params, L';', true), 0);
    int Height = StrToIntDef(CutToChar(Params, L';', true), 0);
    int State = StrToIntDef(CutToChar(Params, L';', true), 0);
    if ((Width > 0) && (Height > 0))
    {
      resize(Width, Height);
    }
    // wsMaximized
    if (State == 2)
    {
      setWindowState(windowState() | Qt::WindowMaximized);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::StoreWindowParams()
{
  bool Maximized = isMaximized();
  QSize Size = (Maximized && normalGeometry().isValid()) ? normalGeometry().size() : size();
  UnicodeString Params = FORMAT(L"-1;-1;%d;%d;%d;96", (Size.width(), Size.height(), (Maximized ? 2 : 0)));
  if (IsExplorer())
  {
    AppConfiguration->ExplorerWindowParams = Params;
  }
  else
  {
    AppConfiguration->CommanderWindowParams = Params;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdatePanelsLayout()
{
  // The remote panel on the left (SwappedPanels)
  TDirPanel * Left = AppConfiguration->SwappedPanels ? static_cast<TDirPanel *>(FRemotePanel) : static_cast<TDirPanel *>(FLocalPanel);
  if (FPanelSplitter->indexOf(Left) != 0)
  {
    QList<int> Sizes = FPanelSplitter->sizes();
    FPanelSplitter->insertWidget(0, Left);
    std::reverse(Sizes.begin(), Sizes.end());
    FPanelSplitter->setSizes(Sizes);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateKeyboardShortcuts()
{
  typedef QList<QKeySequence> TKeys;
  QKeySequence RefreshShortCut;
  if (IsExplorer())
  {
    // upstream's ExplorerShortcuts
    CurrentCreateDirAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::Key_D) });
    CurrentEditAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::Key_E) });
    CurrentEditInternalAction->setShortcuts(TKeys());
    CurrentCopyAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::Key_T) });
    CurrentMoveAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::Key_M) });
    CurrentDeleteAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_Delete) });
    CurrentDeleteAlternativeAction->setShortcuts(TKeys{ QKeySequence(Qt::SHIFT | Qt::Key_Delete) });
    CurrentPropertiesAction->setShortcuts(TKeys{ QKeySequence(Qt::ALT | Qt::Key_Return) });
    RemoteMoveToAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_M) });
    RemoteCopyToAction->setShortcuts(TKeys());
    RemotePathToClipboardAction2->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P) });
    NewFileAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_E) });
    RemoteFindFilesAction2->setShortcuts(TKeys{ QKeySequence(Qt::Key_F3) });
    CloseApplicationAction->setShortcuts(TKeys{ QKeySequence(Qt::ALT | Qt::Key_F4) });
    SelectOneAction->setShortcuts(TKeys());
    RefreshShortCut = QKeySequence(Qt::Key_F5);
    CloseTabAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::Key_W), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D) });
  }
  else
  {
    // upstream's CommanderShortcuts, with the Explorer's keys for copying,
    // refreshing and finding files (ExplorerKeyboardShortcuts)
    bool Explorer = AppConfiguration->ExplorerKeyboardShortcuts;
    CurrentCreateDirAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_F7) });
    CurrentEditAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_F4) });
    CurrentEditInternalAction->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F4) });
    CurrentCopyAction->setShortcuts(TKeys{ Explorer ? QKeySequence(Qt::CTRL | Qt::Key_K) : QKeySequence(Qt::Key_F5) });
    CurrentMoveAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_F6) });
    CurrentDeleteAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_F8), QKeySequence(Qt::Key_Delete) });
    CurrentDeleteAlternativeAction->setShortcuts(
      TKeys{ QKeySequence(Qt::SHIFT | Qt::Key_F8), QKeySequence(Qt::SHIFT | Qt::Key_Delete) });
    CurrentPropertiesAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_F9) });
    RemoteMoveToAction->setShortcuts(TKeys{ QKeySequence(Qt::SHIFT | Qt::Key_F6) });
    RemoteCopyToAction->setShortcuts(TKeys{ QKeySequence(Qt::SHIFT | Qt::Key_F5) });
    RemotePathToClipboardAction2->setShortcuts(TKeys{ QKeySequence(Qt::CTRL | Qt::Key_BracketRight) });
    NewFileAction->setShortcuts(TKeys{ QKeySequence(Qt::SHIFT | Qt::Key_F4) });
    RemoteFindFilesAction2->setShortcuts(TKeys{ Explorer ? QKeySequence(Qt::Key_F3) : QKeySequence(Qt::ALT | Qt::Key_F7) });
    CloseApplicationAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_F10), QKeySequence(Qt::CTRL | Qt::Key_Q) });
    SelectOneAction->setShortcuts(TKeys{ QKeySequence(Qt::Key_Insert) });
    RefreshShortCut = Explorer ? QKeySequence(Qt::Key_F5) : QKeySequence(Qt::CTRL | Qt::Key_R);
    TKeys CloseTabShortCuts = { QKeySequence(Qt::CTRL | Qt::Key_W), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D) };
    if (Explorer)
    {
      CloseTabShortCuts.append(QKeySequence(Qt::CTRL | Qt::Key_F4));
    }
    CloseTabAction->setShortcuts(CloseTabShortCuts);
  }
  FLocalPanel->SetRefreshShortCut(RefreshShortCut);
  FRemotePanel->SetRefreshShortCut(RefreshShortCut);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateTransferList()
{
  if (FTransferList == nullptr)
  {
    return;
  }
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
  if ((FUpdatingTransferList == 0) && (FTransferList != nullptr))
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
  // upstream's TTerminalManager::UpdateAppTitle
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  UnicodeString SessionName = (Manager != NULL) ? Manager->GetActiveSessionAppTitle() : UnicodeString();
  UnicodeString Title = FormatMainFormCaption(UnicodeString(), SessionName);
  UnicodeString Path = PathForCaption();
  if (!Path.IsEmpty())
  {
    Title = Path + TitleSeparator + Title;
  }
  setWindowTitle(ToQString(Title));
  UpdateTrayIcon();
}
//---------------------------------------------------------------------------
UnicodeString TScpCommanderForm::PathForCaption()
{
  UnicodeString Result;
  TTerminal * Terminal = GetTerminal();
  if ((Terminal != NULL) && FRemotePanel->IsReady())
  {
    switch (AppConfiguration->PathInCaption)
    {
      case picShort:
        Result = ExtractShortName(Terminal->CurrentDirectory, true);
        break;

      case picFull:
        Result = Terminal->CurrentDirectory;
        break;

      default:
        break;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OperationComplete(const TDateTime & StartTime)
{
  if (GUIConfiguration->BeepOnFinish &&
      (Now() - StartTime > GUIConfiguration->BeepOnFinishAfter))
  {
    // OpenSCP: the sound of the freedesktop sound theme, or of a file
    UnicodeString BeepSound = GUIConfiguration->BeepSound;
    QString Player = QStandardPaths::findExecutable(QStringLiteral("canberra-gtk-play"));
    if (Player.isEmpty())
    {
      QApplication::beep();
    }
    else if (!ExtractFileExt(BeepSound).IsEmpty())
    {
      QProcess::startDetached(Player, { QStringLiteral("-f"), ToQString(BeepSound) });
    }
    else
    {
      QProcess::startDetached(Player, { QStringLiteral("-i"), QStringLiteral("complete"), QStringLiteral("-d"), ToQString(AppName) });
    }
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TScpCommanderForm::CloseApplicationActionExecute()
{
  close();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::closeEvent(QCloseEvent * Event)
{
  // The operation in progress is to be cancelled first
  bool CanClose = false;
  ExecuteHandler([&]()
  {
    CanClose = !IsBusy() && CanCloseSessions() && CloseEditors();
    if (CanClose)
    {
      SessionsClosing();
    }
  });
  if (!CanClose)
  {
    Event->ignore();
  }
  else
  {
    QMainWindow::closeEvent(Event);
  }
}
//---------------------------------------------------------------------------
TStrings * TScpCommanderForm::CreateFileList(TDirPanel * Panel, bool FullPath, int ARow)
{
  std::unique_ptr<TStringList> Result(new TStringList());
  std::vector<int> Rows = (ARow >= 0) ? std::vector<int>{ ARow } : Panel->GetSelectedRows();
  for (int Row : Rows)
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
      if (AppConfiguration->DDTransferConfirmation == asAuto)
      {
        Notify(NULL, LoadStr(DD_TRANSFER_CONFIRM_OFF2), qtInformation, false, [this](const QString & ActivationToken)
        {
          AppConfiguration->DDTransferConfirmation = asOn;
          ActivateFromOtherApplication(ActivationToken);
        });
      }
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
    // Particularly when called for another application (a drop, a paste)
    FlashOnBackground();
    Result = DoCopyDialog(Direction == tdToRemote, Type == ttMove,
      FileList, TargetDirectory, &CopyParam, Options, CopyParamAttrs, &OutputOptions);

    if (Result)
    {
      HandleDoNotShowCopyDialogAgain(DragDrop, FLAGSET(OutputOptions, cooDoNotShowAgain));
      // upstream's TScpExplorerForm::CopyParamDialogAfter
      if (IsExplorer() && (Direction == tdToLocal) && !DragDrop)
      {
        AppConfiguration->ExplorerLastLocalTargetDirectory = TargetDirectory;
      }
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
      Source->SaveSelectedNames();
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
  // The Explorer downloads to its last local directory
  UnicodeString TargetDirectory =
    !Param.TargetDirectory.IsEmpty() ? Param.TargetDirectory :
      ((IsExplorer() && !ToRemote) ? LocalDirectory() : Target->GetPath());
  TGUICopyParamType CopyParam = GUIConfiguration->CurrentCopyParam;
  if (Param.Queue != asAuto)
  {
    CopyParam.Queue = (Param.Queue == asOn);
  }
  bool Confirm = !Param.NoConfirmation && (!Param.DragDrop || (AppConfiguration->DDTransferConfirmation != asOff));
  bool Result = CopyParamDialog(Direction, Type, FileList, TargetDirectory, CopyParam, Confirm, Param.DragDrop);
  if (Result)
  {
    Panel(Side)->SaveSelectedNames();
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
  Panel(Side)->SaveSelectedNames();
  // Delete moves local files to the trash, Shift+Delete deletes them
  // (by default); the remote ones go to the recycle bin of the site
  bool Recycle;
  if (Side == osLocal)
  {
    Recycle = (AppConfiguration->DeleteToRecycleBin != Alternative);
  }
  else
  {
    TTerminal * Terminal = GetTerminal();
    Recycle =
      (Terminal->SessionData->DeleteToRecycleBin != Alternative) &&
      !Terminal->SessionData->RecycleBinPath.IsEmpty() &&
      !Terminal->IsRecycledFile(FileList->Strings[0]);
  }

  if (Recycle ? AppConfiguration->ConfirmRecycling : AppConfiguration->ConfirmDeleting)
  {
    UnicodeString Message;
    if (FileList->Count == 1)
    {
      UnicodeString FileName =
        (Side == osLocal) ? ExtractFileName(FileList->Strings[0]) : UnixExtractFileName(FileList->Strings[0]);
      Message = FMTLOAD((Recycle ? CONFIRM_RECYCLE_FILE : CONFIRM_DELETE_FILE), (FileName));
    }
    else
    {
      Message = FMTLOAD((Recycle ? CONFIRM_RECYCLE_FILES : CONFIRM_DELETE_FILES), (FileList->Count));
    }
    TMessageParams Params(mpNeverAskAgainCheck);
    Params.ImageName = L"Delete file";
    unsigned int Answer = MessageDialog(MainInstructions(Message), qtConfirmation, qaOK | qaCancel, &Params);
    if (Answer == qaNeverAskAgain)
    {
      if (Recycle)
      {
        AppConfiguration->ConfirmRecycling = false;
      }
      else
      {
        AppConfiguration->ConfirmDeleting = false;
      }
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
      bool ToRecycleBin = Recycle;
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
    Panel->SaveSelectedNames();
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
  // The progress of the queue, if any
  SetQueueProgress();
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
  // Download to temporary local directory
  if (FPendingTempSpaceWarn && ProgressData.InProgress && ProgressData.TotalSizeSet)
  {
    bool Continue = true;
    FPendingTempSpaceWarn = false;
    DoWarnLackOfTempSpace(ProgressData.Directory, ProgressData.TotalSize, Continue);
    if (!Continue)
    {
      Abort();
    }
  }

  // The operation runs and the progress is not shown yet
  if (ProgressData.InProgress && (FProgressForm == nullptr))
  {
    FProgressForm = new TProgressForm(this, ProgressData.IsTransfer() && (ProgressData.Count > 1));
    FProgressForm->SetOnceDoneOperation(ProgressData.InitialOnceDoneOperation);
    FProgressForm->ShowAsModal();
  }
  // The operation finished (or was cancelled)
  // The progress of a synchronization stays over the operations of its items
  else if (!ProgressData.InProgress && (FProgressForm != nullptr) && (FProgressForm->GetSynchronizeProgress() == nullptr))
  {
    DestroyProgressForm();

    if ((ProgressData.Cancel == csContinue) &&
        !FAutoOperation)
    {
      OperationComplete(ProgressData.StartTime);
    }
  }

  if ((FProgressForm != nullptr) && ProgressData.InProgress)
  {
    SetTaskbarProgress(&ProgressData);
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
  TOnceDoneOperation & OnceDoneOperation)
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

  if (FProgressForm != nullptr)
  {
    OnceDoneOperation = FProgressForm->GetOnceDoneOperation();
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
