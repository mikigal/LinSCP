// The toolbars of the main window and of its panels (upstream's toolbars of
// forms/ScpCommander.dfm and ScpExplorer.dfm, their options of
// forms/NonVisual.cpp): created for the interface and arranged by
// upstream's layout (ToolbarsLayout2), the menus that show and hide them.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <TextsWin.h>

#include "DirPanel.h"
#include "HistoryComboBox.h"
#include "ScpCommander.h"
#include "Toolbars.h"

#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QStatusBar>

#include <algorithm>
#include <functional>
#include <memory>
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateSideActions()
{
  // The commands on the files of a panel, of its toolbar (upstream's Local*
  // and Remote* actions): the panel becomes the current one; no keys (the
  // current panel's commands have them)
  for (TOperationSide Side : { osLocal, osRemote })
  {
    bool Local = (Side == osLocal);
    QString Prefix = Local ? QStringLiteral("Local") : QStringLiteral("Remote");
    auto SideAction = [this, Side, Prefix](const char * Name, const QString & Caption, const QString & Hint,
      const QString & IconName, std::function<void()> Execute)
    {
      QAction * Action = new QAction(Caption, this);
      Action->setObjectName(Prefix + QString::fromLatin1(Name));
      if (!IconName.isEmpty())
      {
        Action->setIcon(QIcon::fromTheme(IconName));
      }
      if (!Hint.isEmpty())
      {
        Action->setToolTip(Hint);
      }
      connect(Action, &QAction::triggered, this, [this, Side, Execute]()
      {
        ExecuteHandler([this, Side, Execute]()
        {
          ActivatePanel(Side);
          Execute();
        });
      });
      return Action;
    };
    QString CopyCaption = Local ? tr("Up&load...") : tr("Down&load...");
    QString CopyHint =
      Local ? tr("Upload selected local file(s) to remote directory") : tr("Download selected remote file(s) to local directory");
    QString CopyIcon = Local ? QStringLiteral("cloud-upload") : QStringLiteral("download");
    SideAction("CopyAction", CopyCaption, CopyHint, CopyIcon, [this]() { ExecuteCopyOperationCommand(asAuto); });
    SideAction("CopyNonQueueAction", CopyCaption, CopyHint, CopyIcon, [this]() { ExecuteCopyOperationCommand(asOff); });
    SideAction("CopyQueueAction", Local ? tr("Upload in &Background...") : tr("Download in &Background..."),
      Local ? tr("Upload selected local file(s) to remote directory in background") :
        tr("Download selected remote file(s) to local directory in background"),
      QStringLiteral("view-list-details"), [this]() { ExecuteCopyOperationCommand(asOn); });
    SideAction("MoveAction", Local ? tr("Upload and Dele&te...") : tr("Download and Dele&te..."),
      Local ? tr("Upload selected local file(s) to remote directory and delete original") :
        tr("Download selected remote file(s) to local directory and delete original"),
      QString(), [this]() { CurrentMoveActionExecute(); });
    SideAction("EditAction2", tr("&Edit"), tr("Edit selected file(s)"), QStringLiteral("document-edit"),
      [this]() { CurrentEditActionExecute(); });
    SideAction("DeleteAction2", tr("&Delete"), tr("Delete selected file(s)"), QStringLiteral("edit-delete"),
      [this]() { DeleteFiles(false); });
    SideAction("RenameAction2", tr("&Rename"), tr("Rename selected file"), QStringLiteral("edit-rename"),
      [this]() { CurrentRenameActionExecute(); });
    SideAction("PropertiesAction2", tr("&Properties"), tr("Display/change properties of selected file(s)"),
      QStringLiteral("document-properties"), [this]() { CurrentPropertiesActionExecute(); });
    SideAction("CalculateDirectorySizesAction", tr("&Calculate Directory Sizes"), QString(), QString(),
      [this]() { CalculateDirectorySizes(); });
    SideAction("NewFileAction", tr("&File..."), tr("Create new file and open it in editor"),
      QStringLiteral("document-new"), [this]() { NewFileActionExecute(); });
    SideAction("CreateDirAction3", tr("&Directory..."), tr("Create new directory"), QStringLiteral("folder-new"),
      [this]() { CurrentCreateDirActionExecute(); });
    SideAction("AddEditLinkAction3", tr("&Link..."), QString(), QStringLiteral("insert-link"),
      [this]() { AddEditLink(false); });
    SideAction("SelectAction2", tr("Sele&ct Files..."), tr("Select files by mask"), QStringLiteral("list-add"),
      [this]() { SelectByMask(true); });
    SideAction("UnselectAction2", tr("&Unselect Files..."), tr("Unselect files by mask"), QStringLiteral("list-remove"),
      [this]() { SelectByMask(false); });
    SideAction("SelectAllAction2", tr("Select &All"), tr("Select all files"), QStringLiteral("edit-select-all"),
      [this]() { SelectAllActionExecute(); });
  }
  FileColorsPreferencesAction = CreateAction("FileColorsPreferencesAction", tr("File &Colors..."),
    QStringLiteral("color-management"), {}, &TScpCommanderForm::FileColorsPreferencesActionExecute);
  FileColorsPreferencesAction->setToolTip(tr("Configure file color rules"));
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ActivatePanel(TOperationSide Side)
{
  TDirPanel * APanel = Panel(Side);
  SetCurrentPanel(APanel);
  APanel->GetDirView()->setFocus();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateSideActions()
{
  // upstream's EnabledLocalSelectedOperation and its remote counterpart
  bool Busy = IsBusy();
  TTerminal * Terminal = GetTerminal();
  bool Connected = (Terminal != NULL) && Terminal->Active;
  for (TDirPanel * APanel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    bool Local = (APanel == FLocalPanel);
    QString Prefix = Local ? QStringLiteral("Local") : QStringLiteral("Remote");
    auto Action = [this, Prefix](const char * Name)
    {
      return findChild<QAction *>(Prefix + QString::fromLatin1(Name), Qt::FindDirectChildrenOnly);
    };
    bool Ready = !Busy && APanel->IsReady() && (Local || Connected);
    bool Selected = Ready && !APanel->GetSelectedRows().empty();
    bool CanTransfer = Selected && Connected && OtherPanel(APanel)->IsReady();
    for (const char * Name : { "CopyAction", "CopyNonQueueAction", "CopyQueueAction", "MoveAction" })
    {
      Action(Name)->setEnabled(CanTransfer);
    }
    Action("CopyQueueAction")->setEnabled(CanTransfer && Terminal->IsCapable[fcBackgroundTransfers]);
    for (const char * Name : { "EditAction2", "DeleteAction2", "RenameAction2", "PropertiesAction2", "CalculateDirectorySizesAction" })
    {
      Action(Name)->setEnabled(Selected);
    }
    Action("NewFileAction")->setEnabled(Ready);
    Action("CreateDirAction3")->setEnabled(Ready);
    Action("AddEditLinkAction3")->setEnabled(
      Ready && (Local || (Terminal->ResolvingSymlinks && Terminal->IsCapable[fcSymbolicLink])));
    bool AnyFiles = Ready && (APanel->GetFilesCount() > 0);
    for (const char * Name : { "SelectAction2", "UnselectAction2", "SelectAllAction2" })
    {
      Action(Name)->setEnabled(AnyFiles);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteCopyOperationCommand(TAutoSwitch Queue)
{
  // upstream's ExecuteCopyOperationCommand of a panel's toolbar: in the
  // background or not, as the command says
  TTerminal * Terminal = GetTerminal();
  TDirPanel * Source = FCurrentPanel;
  if (IsBusy() || (Terminal == NULL) || !Source->IsReady() || !OtherPanel(Source)->IsReady())
  {
    return;
  }
  std::unique_ptr<TStrings> FileList(CreateFileList(Source));
  if (FileList->Count > 0)
  {
    TTransferOperationParam Param;
    Param.Queue = Queue;
    ExecuteCopyMoveFileOperation(foCopy, Source->GetSide(), FileList.get(), Param);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::FileColorsPreferencesActionExecute()
{
  DoPreferencesDialog(pmFileColors);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateToolbarActions()
{
  LockToolbarsAction = CreateAction("LockToolbarsAction", tr("&Lock Toolbars"), QString(),
    {}, &TScpCommanderForm::LockToolbarsActionExecute);
  LockToolbarsAction->setToolTip(tr("Prevents moving and (un)docking of all toolbars"));
  LockToolbarsAction->setCheckable(true);
  LockToolbarsAction->setChecked(AppConfiguration->LockToolbars);
  SelectiveToolbarTextAction = CreateAction("SelectiveToolbarTextAction", tr("&Show Selective Text Labels"), QString(),
    {}, &TScpCommanderForm::SelectiveToolbarTextActionExecute);
  SelectiveToolbarTextAction->setToolTip(tr("Show text labels for selected important commands on toolbars"));
  SelectiveToolbarTextAction->setCheckable(true);
  SelectiveToolbarTextAction->setChecked(AppConfiguration->SelectiveToolbarText);
  FToolbarIconSizeMenu = new QMenu(tr("T&oolbar Icons Size"), this);
  FToolbarIconSizeMenu->setObjectName(QStringLiteral("ToolbarIconSizeMenu"));
  FToolbarIconSizeMenu->setToolTip(tr("Change toolbar icons size"));
  QActionGroup * IconSizeGroup = new QActionGroup(this);
  struct { const char * Name; QString Caption; QString Hint; int Larger; } Sizes[] =
  {
    { "ToolbarIconSizeNormalAction", tr("&Normal"), tr("Show normal toolbar icons"), 0 },
    { "ToolbarIconSizeLargeAction", tr("&Large"), tr("Show large toolbar icons"), 1 },
    { "ToolbarIconSizeVeryLargeAction", tr("&Very Large"), tr("Show very large toolbar icons"), 2 },
  };
  for (auto & Size : Sizes)
  {
    QAction * Action = new QAction(Size.Caption, IconSizeGroup);
    Action->setObjectName(QString::fromLatin1(Size.Name));
    Action->setToolTip(Size.Hint);
    Action->setCheckable(true);
    Action->setChecked(AppConfiguration->LargerToolbar == Size.Larger);
    addAction(Action);
    int Larger = Size.Larger;
    connect(Action, &QAction::triggered, this, [this, Larger]()
    {
      AppConfiguration->LargerToolbar = Larger;
      ApplyToolbarsOptions();
    });
    FToolbarIconSizeMenu->addAction(Action);
  }

  CommandLinePanelAction = CreateAction("CommandLinePanelAction", tr("Comma&nd Line"), QString(),
    { QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_M) }, &TScpCommanderForm::CommandLinePanelActionExecute);
  CommandLinePanelAction->setToolTip(tr("Hide/show command line"));
  CommandLinePanelAction->setCheckable(true);
  StatusBarAction = CreateAction("StatusBarAction", tr("Status &Bar"), QString(),
    {}, &TScpCommanderForm::StatusBarActionExecute);
  StatusBarAction->setToolTip(tr("Hide/show status bar"));
  StatusBarAction->setCheckable(true);
  StatusBarAction->setChecked(true);

  QueueToggleShowAction = CreateAction("QueueToggleShowAction", tr("&Queue"), QStringLiteral("view-list-details"),
    {}, &TScpCommanderForm::ToggleQueueVisibility);
  QueueToggleShowAction->setToolTip(tr("Show/hide queue list"));
  QueueToggleShowAction->setCheckable(true);
  QueueToolbarAction = CreateAction("QueueToolbarAction", tr("&Toolbar"), QString(),
    {}, &TScpCommanderForm::QueueToolbarActionExecute);
  QueueToolbarAction->setToolTip(tr("Hide/show queue list toolbar (on queue list panel)"));
  QueueToolbarAction->setCheckable(true);
  QueueToolbarAction->setChecked(AppConfiguration->QueueViewToolBar);

  // The sort of the current panel (upstream's CurrentSort*)
  CurrentSortAscendingAction = new QAction(QIcon::fromTheme(QStringLiteral("view-sort-ascending")), tr("&Ascending"), this);
  CurrentSortAscendingAction->setObjectName(QStringLiteral("CurrentSortAscendingAction"));
  CurrentSortAscendingAction->setToolTip(tr("Ascending"));
  CurrentSortAscendingAction->setCheckable(true);
  addAction(CurrentSortAscendingAction);
  connect(CurrentSortAscendingAction, &QAction::triggered, this, [this](bool Checked)
  {
    FCurrentPanel->SetSortAscending(Checked);
  });
  struct { TDirViewColumn Column; const char * Name; QString Caption; QString Hint; QString Icon; } Sorts[] =
  {
    { dvName, "CurrentSortByNameAction", tr("By &Name"), tr("Sort current panel by name"),
      QStringLiteral("sort-name") },
    { dvExt, "CurrentSortByExtAction", tr("By &Extension"), tr("Sort current panel by file name extension"),
      QStringLiteral("sort-name") },
    { dvSize, "CurrentSortBySizeAction", tr("By &Size"), tr("Sort current panel by file size"),
      QStringLiteral("view-sort") },
    { dvType, "CurrentSortByTypeAction2", tr("By &Type"), tr("Sort current panel by file type"),
      QStringLiteral("view-sort") },
    { dvChanged, "CurrentSortByChangedAction2", tr("By Date &modified"), tr("Sort current panel by last modification time"),
      QStringLiteral("view-calendar-day") },
    { dvRights, "CurrentSortByRightsAction", tr("By &Permissions/Attributes"), QString(),
      QStringLiteral("object-locked") },
    { dvOwner, "CurrentSortByOwnerAction", tr("By &Owner"), QString(), QStringLiteral("user-identity") },
    { dvGroup, "CurrentSortByGroupAction", tr("By &Group"), QString(), QStringLiteral("system-users") },
  };
  for (auto & Sort : Sorts)
  {
    QAction * Action = new QAction(QIcon::fromTheme(Sort.Icon), Sort.Caption, this);
    Action->setObjectName(QString::fromLatin1(Sort.Name));
    if (!Sort.Hint.isEmpty())
    {
      Action->setToolTip(Sort.Hint);
    }
    Action->setCheckable(true);
    addAction(Action);
    TDirViewColumn Column = Sort.Column;
    connect(Action, &QAction::triggered, this, [this, Column]()
    {
      FCurrentPanel->SortBy(Column);
      UpdateSortActions();
    });
    FCurrentSortActions.emplace_back(Column, Action);
  }
  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    connect(Panel, &TDirPanel::SortChanged, this, [this]() { UpdateSortActions(); });
  }

  // The panels' toolbars of the window (their files and their selection)
  // and the menus that show and hide them
  CreatePanelToolbars(FLocalPanel);
  CreatePanelToolbars(FRemotePanel);
}
//---------------------------------------------------------------------------
TToolbar * TScpCommanderForm::NewToolbar(const char * Name, const QString & Caption, QWidget * Parent)
{
  TToolbar * Toolbar = new TToolbar(QString::fromLatin1(Name), Caption, Parent);
  // upstream's popup menu of the toolbars
  Toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(Toolbar, &QWidget::customContextMenuRequested, this, [this, Toolbar](const QPoint & Pos)
  {
    ToolbarPopup(Toolbar, Toolbar->mapToGlobal(Pos));
  });
  return Toolbar;
}
//---------------------------------------------------------------------------
QAction * TScpCommanderForm::NewBandAction(TToolbar * Toolbar, const char * Name, const QString & Caption, const QString & Hint)
{
  // Shows and hides the toolbar (upstream's *BandAction)
  QAction * Action = new QAction(Caption, Toolbar);
  Action->setObjectName(QString::fromLatin1(Name));
  Action->setToolTip(Hint);
  Action->setCheckable(true);
  Action->setChecked(true);
  connect(Action, &QAction::triggered, this, [this, Toolbar](bool Checked)
  {
    SetToolbarVisible(Toolbar, Checked);
  });
  Toolbar->SetBandAction(Action);
  return Action;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SetToolbarVisible(TToolbar * Toolbar, bool Visible)
{
  Toolbar->setVisible(Visible);
  if (Toolbar->GetBandAction() != nullptr)
  {
    Toolbar->GetBandAction()->setChecked(Visible);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreatePanelToolbars(TDirPanel * Panel)
{
  bool Local = (Panel == FLocalPanel);
  QString Prefix = Local ? QStringLiteral("Local") : QStringLiteral("Remote");
  auto Action = [this](const QString & Name)
  {
    return findChild<QAction *>(Name, Qt::FindDirectChildrenOnly);
  };

  // The files (upstream's LocalFileToolbar and RemoteFileToolbar)
  TToolbar * FileToolbar = NewToolbar(Local ? "LocalFileToolbar" : "RemoteFileToolbar",
    Local ? tr("Local Files") : tr("Remote Files"), Panel);
  QMenu * CopyMenu = new QMenu(FileToolbar);
  CopyMenu->setObjectName(Prefix + QStringLiteral("CopyMenu"));
  CopyMenu->addAction(Action(Prefix + QStringLiteral("CopyNonQueueAction")));
  CopyMenu->addAction(Action(Prefix + QStringLiteral("CopyQueueAction")));
  CopyMenu->addSeparator();
  CopyMenu->addAction(Action(Prefix + QStringLiteral("MoveAction")));
  FileToolbar->AddMenuButton(Action(Prefix + QStringLiteral("CopyAction")), CopyMenu, tmmSplit, true);
  FileToolbar->AddSeparatorItem();
  QMenu * EditMenu = new QMenu(FileToolbar);
  EditMenu->setObjectName(Prefix + QStringLiteral("EditMenu"));
  TOperationSide Side = Panel->GetSide();
  connect(EditMenu, &QMenu::aboutToShow, this, [this, EditMenu, Side]()
  {
    EditMenu->clear();
    ExecuteHandler([this, EditMenu, Side]() { CreateEditWithMenu(EditMenu, Side); });
  });
  FileToolbar->AddMenuButton(Action(Prefix + QStringLiteral("EditAction2")), EditMenu, tmmSplit, true);
  FileToolbar->AddButton(Action(Prefix + QStringLiteral("DeleteAction2")));
  FileToolbar->AddButton(Action(Prefix + QStringLiteral("RenameAction2")));
  QMenu * PropertiesMenu = new QMenu(FileToolbar);
  PropertiesMenu->setObjectName(Prefix + QStringLiteral("PropertiesMenu"));
  PropertiesMenu->addAction(Action(Prefix + QStringLiteral("PropertiesAction2")));
  PropertiesMenu->setDefaultAction(Action(Prefix + QStringLiteral("PropertiesAction2")));
  PropertiesMenu->addSeparator();
  PropertiesMenu->addAction(Action(Prefix + QStringLiteral("CalculateDirectorySizesAction")));
  FileToolbar->AddMenuButton(Action(Prefix + QStringLiteral("PropertiesAction2")), PropertiesMenu, tmmSplit, true);
  FileToolbar->AddSeparatorItem();
  QMenu * NewMenu = new QMenu(tr("&New"), FileToolbar);
  NewMenu->setObjectName(Prefix + QStringLiteral("NewMenu"));
  NewMenu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
  NewMenu->addAction(Action(Prefix + QStringLiteral("NewFileAction")));
  NewMenu->addAction(Action(Prefix + QStringLiteral("CreateDirAction3")));
  NewMenu->addAction(Action(Prefix + QStringLiteral("AddEditLinkAction3")));
  FileToolbar->AddSubmenu(Prefix + QStringLiteral("NewSubmenuItem"), NewMenu, true);

  // The selection (upstream's LocalSelectionToolbar and RemoteSelectionToolbar)
  TToolbar * SelectionToolbar = NewToolbar(Local ? "LocalSelectionToolbar" : "RemoteSelectionToolbar",
    Local ? tr("Local Selection") : tr("Remote Selection"), Panel);
  SelectionToolbar->AddButton(Action(Prefix + QStringLiteral("SelectAction2")));
  SelectionToolbar->AddButton(Action(Prefix + QStringLiteral("UnselectAction2")));
  SelectionToolbar->AddButton(Action(Prefix + QStringLiteral("SelectAllAction2")));

  // The files can be found from the remote panel
  TToolbar * Navigation = Panel->GetNavigationToolbar();
  if (!Local)
  {
    Navigation->InsertSeparatorItem(Panel->GetTreeAction());
    Navigation->InsertButton(Panel->GetTreeAction(), RemoteFindFilesAction2, true);
  }
  // The colors of the files, in the menu of the filter button
  Panel->GetFilterMenu()->addAction(FileColorsPreferencesAction);

  TToolbarDock * Dock = Panel->GetTopDock();
  Dock->AddToolbar(FileToolbar);
  Dock->AddToolbar(SelectionToolbar);
  for (TToolbar * Toolbar : { Panel->GetPathToolbar(), Panel->GetHistoryToolbar(), Navigation })
  {
    Toolbar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(Toolbar, &QWidget::customContextMenuRequested, this, [this, Toolbar](const QPoint & Pos)
    {
      ToolbarPopup(Toolbar, Toolbar->mapToGlobal(Pos));
    });
  }

  // upstream's Left Panel and Right Panel menus (the toolbars, the tree,
  // the status bar)
  QMenu * Menu = new QMenu(this);
  Menu->setObjectName(Local ? QStringLiteral("CommanderLocalPanelMenu") : QStringLiteral("CommanderRemotePanelMenu"));
  Menu->addAction(NewBandAction(Panel->GetHistoryToolbar(),
    Local ? "CommanderLocalHistoryBandAction2" : "CommanderRemoteHistoryBandAction2",
    tr("&History Buttons"), tr("Hide/show history toolbar")));
  Menu->addAction(NewBandAction(Navigation,
    Local ? "CommanderLocalNavigationBandAction2" : "CommanderRemoteNavigationBandAction2",
    tr("&Navigation Buttons"), tr("Hide/show navigation toolbar")));
  Menu->addAction(NewBandAction(FileToolbar,
    Local ? "CommanderLocalFileBandAction2" : "CommanderRemoteFileBandAction2",
    tr("&File Buttons"), tr("Hide/show file toolbar")));
  Menu->addAction(NewBandAction(SelectionToolbar,
    Local ? "CommanderLocalSelectionBandAction2" : "CommanderRemoteSelectionBandAction2",
    tr("Se&lection Buttons"), tr("Hide/show selection toolbar")));
  Menu->addSeparator();
  Menu->addAction(Panel->GetTreeAction());
  Menu->addSeparator();
  Menu->addAction(Panel->GetStatusBarAction());
  (Local ? FLocalPanelMenu : FRemotePanelMenu) = Menu;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdatePanelMenus()
{
  // upstream swaps the captions of the menus with the panels
  bool Swapped = AppConfiguration->SwappedPanels;
  FLocalPanelMenu->setTitle(Swapped ? tr("&Right Panel") : tr("&Left Panel"));
  FLocalPanelMenu->setToolTip(Swapped ? tr("Change right panel layout") : tr("Change left panel layout"));
  FRemotePanelMenu->setTitle(Swapped ? tr("&Left Panel") : tr("&Right Panel"));
  FRemotePanelMenu->setToolTip(Swapped ? tr("Change left panel layout") : tr("Change right panel layout"));
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DestroyToolbars()
{
  for (TToolbar * Toolbar : FToolbars)
  {
    removeToolBar(Toolbar);
    // The Explorer's address bar goes back to its panel
    if (Toolbar == FRemotePanel->GetPathToolbar())
    {
      delete Toolbar->GetBandAction();
      Toolbar->SetBandAction(nullptr);
      Toolbar->setObjectName(QStringLiteral("RemotePathToolbar"));
      Toolbar->setMovable(false);
      Toolbar->setFloatable(false);
      Toolbar->setParent(FRemotePanel->GetTopDock());
      FRemotePanel->GetTopDock()->SetUndocked(Toolbar, false);
      continue;
    }
    // The transfer list is created again
    if (Toolbar->isAncestorOf(FTransferList))
    {
      FTransferList = nullptr;
    }
    delete Toolbar;
  }
  FToolbars.clear();
  FToolbarBands.clear();
  FHotkeysToolbar = nullptr;
  FCustomCommandsToolbar = nullptr;
  FCommandLineCombo = nullptr;
  FCommandLineToolbar = nullptr;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateTransferToolbar()
{
  TToolbar * Toolbar = NewToolbar("TransferToolbar", tr("Transfer Settings"), this);
  QLabel * TransferLabel = new QLabel(tr("Transfer Settings"), Toolbar);
  TransferLabel->setObjectName(QStringLiteral("TransferSettingsLabelItem"));
  TransferLabel->setContentsMargins(4, 0, 4, 0);
  Toolbar->AddControl(TransferLabel);
  FTransferList = new QComboBox(Toolbar);
  FTransferList->setObjectName(QStringLiteral("TransferList"));
  FTransferList->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  FTransferList->setMinimumContentsLength(16);
  TransferLabel->setBuddy(FTransferList);
  Toolbar->AddControl(FTransferList);
  connect(FTransferList, &QComboBox::currentIndexChanged, this, [this]() { TransferListChange(); });
  FToolbarBands.push_back(
    NewBandAction(Toolbar, IsExplorer() ? "ExplorerTransferBandAction" : "CommanderTransferBandAction",
      tr("&Transfer Settings"), tr("Hide/show transfer settings toolbar")));
  FToolbars.push_back(Toolbar);
  UpdateTransferList();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateSessionToolbar()
{
  TToolbar * Toolbar = NewToolbar("SessionToolbar2", tr("Sessions and Tabs"), this);
  Toolbar->AddButton(NewSessionAction, true);
  Toolbar->AddButton(SaveCurrentSessionAction2);
  Toolbar->AddSeparatorItem();
  Toolbar->AddButton(DuplicateTabAction);
  Toolbar->AddButton(CloseTabAction);
  Toolbar->AddSeparatorItem();
  Toolbar->AddSubmenu(QStringLiteral("SavedSessionsAction2"), FSavedSessionsMenu, true);
  FToolbarBands.push_back(
    NewBandAction(Toolbar, IsExplorer() ? "ExplorerSessionBandAction2" : "CommanderSessionBandAction2",
      tr("Sessio&ns and Tabs Buttons"), tr("Hide/show sessions and tabs toolbar")));
  FToolbars.push_back(Toolbar);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateSortToolbar()
{
  TToolbar * Toolbar = NewToolbar("SortToolbar", tr("Sort"), this);
  Toolbar->AddButton(CurrentSortAscendingAction);
  Toolbar->AddSeparatorItem();
  for (auto & Sort : FCurrentSortActions)
  {
    Toolbar->AddButton(Sort.second);
  }
  FToolbarBands.push_back(
    NewBandAction(Toolbar, IsExplorer() ? "ExplorerSortBandAction" : "CommanderSortBandAction",
      tr("So&rt Buttons"), tr("Hide/show sort toolbar")));
  FToolbars.push_back(Toolbar);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateCustomCommandsToolbar()
{
  FCustomCommandsToolbar = NewToolbar("CustomCommandsToolbar", tr("Custom Commands"), this);
  FToolbarBands.push_back(
    NewBandAction(FCustomCommandsToolbar,
      IsExplorer() ? "ExplorerCustomCommandsBandAction" : "CommanderCustomCommandsBandAction",
      tr("Custom Co&mmand Buttons"), tr("Hide/show custom commands toolbar")));
  FToolbars.push_back(FCustomCommandsToolbar);
  UpdateCustomCommandsToolbar();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateCustomCommandsToolbar()
{
  // upstream's UpdateCustomCommandsToolbar: the items of the custom
  // commands menu (the commands, ad hoc, the last one, customize)
  if (FCustomCommandsToolbar != nullptr)
  {
    TToolbar * Toolbar = FCustomCommandsToolbar;
    Toolbar->ClearItems();
    delete FCustomCommandsToolbarMenu;
    FCustomCommandsToolbarMenu = new QMenu(Toolbar);
    CreateCustomCommandsMenu(FCustomCommandsToolbarMenu);
    for (QAction * Action : FCustomCommandsToolbarMenu->actions())
    {
      if (Action->isSeparator())
      {
        Toolbar->AddSeparatorItem();
      }
      else
      {
        Toolbar->AddButton(Action, true);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateCustomCommandsToolbarState()
{
  if ((FCustomCommandsToolbar != nullptr) && !FCustomCommandsToolbar->isHidden() && (FCustomCommandsToolbarMenu != nullptr))
  {
    for (QAction * Action : FCustomCommandsToolbarMenu->actions())
    {
      if (Action->data().isValid())
      {
        const TCustomCommandType * Command = AppConfiguration->CustomCommandList->Find(ToUnicodeString(Action->data().toString()));
        Action->setEnabled(!IsBusy() && (Command != NULL) && (CustomCommandState(*Command, ccltAll) > 0));
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateHotkeysToolbar()
{
  // The keys of the commands, at the bottom (upstream's Toolbar2Toolbar)
  TToolbar * Hotkeys = NewToolbar("Toolbar2Toolbar", tr("Hot Keys"), this);
  FHotkeysToolbar = Hotkeys;
  for (QAction * Action :
         { CurrentRenameAction, CurrentEditAction, CurrentCopyAction, CurrentMoveAction, CurrentCreateDirAction, CurrentDeleteAction,
           CurrentPropertiesAction, CloseApplicationAction })
  {
    QAction * Proxy = new QAction(Action->icon(), QString(), Hotkeys);
    Proxy->setObjectName(Action->objectName() + QStringLiteral("Hotkey"));
    auto UpdateProxy = [Action, Proxy]()
    {
      QString Caption = Action->text().remove(QLatin1Char('&')).remove(QStringLiteral("..."));
      Proxy->setText(Action->shortcut().toString(QKeySequence::NativeText) + QLatin1Char(' ') + Caption);
      Proxy->setEnabled(Action->isEnabled());
    };
    UpdateProxy();
    connect(Proxy, &QAction::triggered, Action, &QAction::trigger);
    // The state, the keys change with the preferences
    connect(Action, &QAction::changed, Proxy, UpdateProxy);
    Hotkeys->AddButton(Proxy, true);
  }
  QAction * Band = NewBandAction(Hotkeys, "ToolBar2Action", tr("&Hot Keys Toolbar"), tr("Hide/show bottom hot keys toolbar"));
  FToolbarBands.push_back(Band);
  FToolbars.push_back(Hotkeys);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateCommanderToolbars()
{
  TToolbar * Commands = NewToolbar("CommandsToolbar", tr("Commands"), this);
  Commands->AddButton(CompareDirectoriesAction2);
  Commands->AddButton(SynchronizeAction);
  Commands->AddButton(FullSynchronizeAction2, true);
  Commands->AddSeparatorItem();
  Commands->AddButton(ConsoleAction);
  Commands->AddButton(PuttyAction);
  Commands->AddSeparatorItem();
  Commands->AddButton(SynchronizeBrowsingAction2);
  FToolbarBands.push_back(
    NewBandAction(Commands, "CommanderCommandsBandAction", tr("&Commands Buttons"), tr("Hide/show commands toolbar")));
  FToolbars.push_back(Commands);

  CreateSessionToolbar();

  TToolbar * Preferences = NewToolbar("PreferencesToolbar", tr("Preferences"), this);
  Preferences->AddButton(PreferencesAction);
  Preferences->AddSeparatorItem();
  Preferences->AddMenuButton(QueueToggleShowAction, FQueueShowMenu, tmmSplit, true);
  FToolbarBands.push_back(
    NewBandAction(Preferences, "CommanderPreferencesBandAction", tr("&Preferences Buttons"), tr("Hide/show preferences toolbar")));
  FToolbars.push_back(Preferences);

  CreateSortToolbar();
  CreateTransferToolbar();
  CreateCustomCommandsToolbar();
  CreateHotkeysToolbar();
  CreateCommandLineToolbar();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateCommandLineToolbar()
{
  // upstream's command line (CommandLineToolbar): commands for the shell
  // of the session, shown in the console
  TToolbar * Toolbar = NewToolbar("CommandLineToolbar", tr("Command Line"), this);
  QLabel * PromptLabel = new QLabel(Toolbar);
  PromptLabel->setObjectName(QStringLiteral("CommandLinePromptLabel"));
  PromptLabel->setContentsMargins(4, 0, 4, 0);
  PromptLabel->setText(ToQString(LoadStr(COMMAND_LINE_LABEL) + L" $"));
  Toolbar->AddControl(PromptLabel);
  FCommandLineCombo = new THistoryComboBox(Toolbar);
  FCommandLineCombo->setObjectName(QStringLiteral("CommandLineCombo"));
  FCommandLineCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  FCommandLineCombo->SetHistoryKey(L"Commands");
  PromptLabel->setBuddy(FCommandLineCombo);
  Toolbar->AddControl(FCommandLineCombo);
  connect(FCommandLineCombo->lineEdit(), &QLineEdit::returnPressed, this, [this]()
  {
    ExecuteHandler([this]() { ExecuteCommandLine(); });
  });
  // The action of the window (in the Options menu) shows and hides it
  Toolbar->SetBandAction(CommandLinePanelAction);
  FCommandLineToolbar = Toolbar;
  FToolbars.push_back(Toolbar);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteCommandLine()
{
  // upstream's ExecuteCommandLine
  TTerminal * Terminal = GetTerminal();
  UnicodeString Command = FCommandLineCombo->GetText();
  if (!IsBusy() && !Command.IsEmpty() && CanConsole() &&
      Terminal->AllowedAnyCommand(Command) && EnsureCommandSessionFallback(fcAnyCommand))
  {
    FCommandLineCombo->SaveToHistory();
    FCommandLineCombo->SetText(UnicodeString());
    OpenConsole(Command);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateExplorerToolbars()
{
  auto PanelAction = [this](const char * Name)
  {
    return FRemotePanel->findChild<QAction *>(QString::fromLatin1(Name));
  };
  auto WindowAction = [this](const char * Name)
  {
    return findChild<QAction *>(QString::fromLatin1(Name), Qt::FindDirectChildrenOnly);
  };
  auto Menu = [this](const char * Name)
  {
    return findChild<QMenu *>(QString::fromLatin1(Name));
  };

  // The address bar (upstream's AddressToolbar) is the remote panel's path
  // toolbar, in the window while the Explorer is shown
  TToolbar * Address = FRemotePanel->GetPathToolbar();
  FRemotePanel->GetTopDock()->SetUndocked(Address, true);
  Address->setObjectName(QStringLiteral("AddressToolbar"));
  FToolbarBands.push_back(
    NewBandAction(Address, "ExplorerAddressBandAction", tr("&Address"), tr("Hide/show address toolbar")));
  FToolbars.push_back(Address);

  // The Explorer's standard buttons (upstream's ButtonsToolbar)
  TToolbar * Buttons = NewToolbar("ButtonsToolbar", tr("Commands"), this);
  Buttons->AddMenuButton(PanelAction("RemoteBackAction"), Menu("RemoteBackMenu"), tmmSplit);
  Buttons->AddMenuButton(PanelAction("RemoteForwardAction"), Menu("RemoteForwardMenu"), tmmSplit);
  Buttons->AddSeparatorItem();
  Buttons->AddButton(PanelAction("RemoteParentDirAction"));
  Buttons->AddButton(PanelAction("RemoteRootDirAction"));
  Buttons->AddButton(PanelAction("RemoteHomeDirAction"));
  Buttons->AddButton(PanelAction("RemoteRefreshAction"));
  Buttons->AddSeparatorItem();
  Buttons->AddButton(RemoteFindFilesAction2, true);
  Buttons->AddSeparatorItem();
  Buttons->AddMenuButton(WindowAction("RemoteCopyAction"), Menu("RemoteCopyMenu"), tmmSplit, true);
  Buttons->AddSeparatorItem();
  Buttons->AddMenuButton(WindowAction("RemoteEditAction2"), Menu("RemoteEditMenu"), tmmSplit, true);
  Buttons->AddButton(CurrentOpenAction);
  Buttons->AddButton(WindowAction("RemoteDeleteAction2"));
  Buttons->AddMenuButton(WindowAction("RemotePropertiesAction2"), Menu("RemotePropertiesMenu"), tmmSplit, true);
  Buttons->AddButton(WindowAction("RemoteRenameAction2"));
  Buttons->AddSeparatorItem();
  Buttons->AddSubmenu(QStringLiteral("NewSubmenuItem"), Menu("RemoteNewMenu"), true);
  Buttons->AddButton(ConsoleAction);
  Buttons->AddButton(PuttyAction);
  Buttons->AddSeparatorItem();
  Buttons->AddButton(SynchronizeAction);
  Buttons->AddButton(FullSynchronizeAction2, true);
  FToolbarBands.push_back(
    NewBandAction(Buttons, "ExplorerToolbarBandAction", tr("&Standard Buttons"), tr("Hide/show standard toolbar")));
  FToolbars.push_back(Buttons);

  TToolbar * Selection = NewToolbar("SelectionToolbar", tr("Selection"), this);
  Selection->AddButton(SelectAction);
  Selection->AddButton(UnselectAction);
  Selection->AddSeparatorItem();
  Selection->AddButton(SelectAllAction);
  Selection->AddButton(InvertSelectionAction);
  Selection->AddButton(ClearSelectionAction);
  Selection->AddButton(RestoreSelectionAction);
  FToolbarBands.push_back(
    NewBandAction(Selection, "ExplorerSelectionBandAction", tr("Se&lection Buttons"), tr("Hide/show selection toolbar")));
  FToolbars.push_back(Selection);

  CreateSessionToolbar();

  TToolbar * Preferences = NewToolbar("PreferencesToolbar", tr("Preferences"), this);
  Preferences->AddButton(PreferencesAction);
  Preferences->AddSeparatorItem();
  Preferences->AddMenuButton(QueueToggleShowAction, FQueueShowMenu, tmmSplit, true);
  Preferences->AddButton(FRemotePanel->GetTreeAction());
  FToolbarBands.push_back(
    NewBandAction(Preferences, "ExplorerPreferencesBandAction", tr("&Preferences Buttons"), tr("Hide/show preferences toolbar")));
  FToolbars.push_back(Preferences);

  CreateSortToolbar();
  CreateTransferToolbar();
  CreateCustomCommandsToolbar();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateToolbars()
{
  DestroyToolbars();
  if (IsExplorer())
  {
    CreateExplorerToolbars();
  }
  else
  {
    CreateCommanderToolbars();
  }
  RestoreToolbarsLayout();
  ApplyToolbarsOptions();
}
//---------------------------------------------------------------------------
std::vector<TToolbar *> TScpCommanderForm::AllToolbars() const
{
  std::vector<TToolbar *> Result = FToolbars;
  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    for (TToolbar * Toolbar : Panel->GetTopDock()->GetToolbars())
    {
      Result.push_back(Toolbar);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
static Qt::ToolBarArea DockArea(const QString & Dock)
{
  if (Dock == QStringLiteral("BottomDock"))
  {
    return Qt::BottomToolBarArea;
  }
  else if (Dock == QStringLiteral("LeftDock"))
  {
    return Qt::LeftToolBarArea;
  }
  else if (Dock == QStringLiteral("RightDock"))
  {
    return Qt::RightToolBarArea;
  }
  return Qt::TopToolBarArea;
}
//---------------------------------------------------------------------------
static QString DockName(Qt::ToolBarArea Area)
{
  switch (Area)
  {
    case Qt::BottomToolBarArea:
      return QStringLiteral("BottomDock");
    case Qt::LeftToolBarArea:
      return QStringLiteral("LeftDock");
    case Qt::RightToolBarArea:
      return QStringLiteral("RightDock");
    default:
      return QStringLiteral("TopDock");
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RestoreToolbarsLayout()
{
  TToolbarsLayout Layout(IsExplorer() ? AppConfiguration->ExplorerToolbarsLayout : AppConfiguration->CommanderToolbarsLayout);

  // The toolbars of the window in the rows of their docks, in the order of
  // their positions (Qt keeps no gaps between them)
  struct TPlaced
  {
    TToolbar * Toolbar;
    Qt::ToolBarArea Area;
    TToolbarPosition Position;
  };
  std::vector<TPlaced> Placed;
  int NextRow = 1000;
  for (TToolbar * Toolbar : FToolbars)
  {
    TPlaced Item;
    Item.Toolbar = Toolbar;
    if (!Layout.Get(Toolbar->GetKey(), Item.Position))
    {
      Item.Position.Dock = QStringLiteral("TopDock");
      Item.Position.Row = NextRow++;
    }
    Item.Area = DockArea(Item.Position.Dock);
    Placed.push_back(Item);
  }
  std::stable_sort(Placed.begin(), Placed.end(), [](const TPlaced & A, const TPlaced & B)
  {
    return
      (A.Area < B.Area) ||
      ((A.Area == B.Area) &&
       ((A.Position.Row < B.Position.Row) || ((A.Position.Row == B.Position.Row) && (A.Position.Pos < B.Position.Pos))));
  });
  for (size_t Index = 0; Index < Placed.size(); Index++)
  {
    const TPlaced & Item = Placed[Index];
    removeToolBar(Item.Toolbar);
    if ((Index > 0) && (Placed[Index - 1].Area == Item.Area) && (Placed[Index - 1].Position.Row != Item.Position.Row))
    {
      addToolBarBreak(Item.Area);
    }
    addToolBar(Item.Area, Item.Toolbar);
    SetToolbarVisible(Item.Toolbar, Item.Position.Visible);
  }

  // The toolbars of the panels
  for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
  {
    TToolbarDock * Dock = Panel->GetTopDock();
    for (TToolbar * Toolbar : Dock->GetToolbars())
    {
      TToolbarPosition Position;
      if (Layout.Get(Toolbar->GetKey(), Position))
      {
        Dock->SetPosition(Toolbar, Position.Row, Position.Pos);
      }
      // In the Explorer, the window's toolbars have the remote panel's
      // commands
      SetToolbarVisible(Toolbar, Position.Visible && !(IsExplorer() && (Panel == FRemotePanel)));
    }
    Dock->Arrange();
  }

  LoadToolbarsButtons(AllToolbars(),
    IsExplorer() ? AppConfiguration->ExplorerToolbarsButtons : AppConfiguration->CommanderToolbarsButtons);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::StoreToolbarsLayout()
{
  UnicodeString LayoutStr = IsExplorer() ? AppConfiguration->ExplorerToolbarsLayout : AppConfiguration->CommanderToolbarsLayout;
  TToolbarsLayout Layout(LayoutStr);
  // The rows of the visible toolbars by their places in the window; the
  // hidden ones keep theirs
  for (TToolbar * Toolbar : FToolbars)
  {
    TToolbarPosition Position;
    Layout.Get(Toolbar->GetKey(), Position);
    Position.Visible = !Toolbar->isHidden();
    if (Position.Visible)
    {
      Qt::ToolBarArea Area = toolBarArea(Toolbar);
      bool Vertical = (Area == Qt::LeftToolBarArea) || (Area == Qt::RightToolBarArea);
      std::vector<int> Lines;
      for (TToolbar * Other : FToolbars)
      {
        if (!Other->isHidden() && (toolBarArea(Other) == Area))
        {
          Lines.push_back(Vertical ? Other->x() : Other->y());
        }
      }
      std::sort(Lines.begin(), Lines.end());
      Lines.erase(std::unique(Lines.begin(), Lines.end()), Lines.end());
      int Line = static_cast<int>(std::lower_bound(Lines.begin(), Lines.end(), Vertical ? Toolbar->x() : Toolbar->y()) - Lines.begin());
      Position.Dock = DockName(Area);
      // The menu is the first row of upstream's top dock
      Position.Row = Line + ((Area == Qt::TopToolBarArea) ? 1 : 0);
      Position.Pos = Vertical ? Toolbar->y() : Toolbar->x();
    }
    Layout.Set(Toolbar->GetKey(), Position);
  }
  if (!IsExplorer())
  {
    for (TDirPanel * Panel : { static_cast<TDirPanel *>(FLocalPanel), static_cast<TDirPanel *>(FRemotePanel) })
    {
      TToolbarDock * Dock = Panel->GetTopDock();
      for (TToolbar * Toolbar : Dock->GetToolbars())
      {
        TToolbarPosition Position;
        Position.Visible = !Toolbar->isHidden();
        Position.Dock = Dock->objectName();
        Dock->GetPosition(Toolbar, Position.Row, Position.Pos);
        Layout.Set(Toolbar->GetKey(), Position);
      }
    }
  }
  UnicodeString Buttons = SaveToolbarsButtons(AllToolbars());
  if (IsExplorer())
  {
    AppConfiguration->ExplorerToolbarsLayout = Layout.ToString();
    AppConfiguration->ExplorerToolbarsButtons = Buttons;
  }
  else
  {
    AppConfiguration->CommanderToolbarsLayout = Layout.ToString();
    AppConfiguration->CommanderToolbarsButtons = Buttons;
    AppConfiguration->CommanderStatusBar = !statusBar()->isHidden();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ApplyToolbarsOptions()
{
  for (TToolbar * Toolbar : AllToolbars())
  {
    Toolbar->SetSelectiveText(AppConfiguration->SelectiveToolbarText);
    Toolbar->SetLargerToolbar(AppConfiguration->LargerToolbar);
  }
  QueueToolbar->SetLargerToolbar(AppConfiguration->LargerToolbar);
  for (TToolbar * Toolbar : FToolbars)
  {
    Toolbar->setMovable(!AppConfiguration->LockToolbars);
  }
  LockToolbarsAction->setChecked(AppConfiguration->LockToolbars);
  SelectiveToolbarTextAction->setChecked(AppConfiguration->SelectiveToolbarText);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LockToolbarsActionExecute()
{
  AppConfiguration->LockToolbars = LockToolbarsAction->isChecked();
  ApplyToolbarsOptions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SelectiveToolbarTextActionExecute()
{
  AppConfiguration->SelectiveToolbarText = SelectiveToolbarTextAction->isChecked();
  ApplyToolbarsOptions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CommandLinePanelActionExecute()
{
  if (FCommandLineToolbar != nullptr)
  {
    SetToolbarVisible(FCommandLineToolbar, CommandLinePanelAction->isChecked());
    if (CommandLinePanelAction->isChecked())
    {
      FCommandLineCombo->setFocus();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::StatusBarActionExecute()
{
  statusBar()->setVisible(StatusBarAction->isChecked());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::QueueToolbarActionExecute()
{
  AppConfiguration->QueueViewToolBar = QueueToolbarAction->isChecked();
  QueueToolbar->setVisible(AppConfiguration->QueueViewToolBar);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ToggleQueueVisibility()
{
  // upstream's ToggleQueueVisibility
  TQueueViewShow Show = AppConfiguration->QueueViewShow;
  switch (Show)
  {
    case qvShow:
      if ((FQueueStatus != NULL) && (FQueueStatus->Count > 0))
      {
        Show = qvHide;
      }
      else
      {
        Show = AppConfiguration->QueueViewLastHideShow;
      }
      break;

    case qvHideWhenEmpty:
      if (!QueuePanel->isHidden())
      {
        Show = qvHide;
      }
      else
      {
        AppConfiguration->QueueViewLastHideShow = Show;
        Show = qvShow;
      }
      break;

    case qvHide:
      AppConfiguration->QueueViewLastHideShow = Show;
      Show = qvShow;
      break;
  }
  QueueViewShowChange(Show);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateSortActions()
{
  CurrentSortAscendingAction->setChecked(FCurrentPanel->IsSortAscending());
  for (auto & Sort : FCurrentSortActions)
  {
    Sort.second->setEnabled(FCurrentPanel->HasColumn(Sort.first));
    Sort.second->setChecked(FCurrentPanel->IsSortedBy(Sort.first));
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::FillToolbarsMenu(QMenu * Menu)
{
  // upstream's Options > Toolbars (the Explorer's View > Toolbars)
  for (QAction * Band : FToolbarBands)
  {
    if (Band->objectName() == QStringLiteral("ToolBar2Action"))
    {
      Menu->addSeparator();
    }
    Menu->addAction(Band);
  }
  Menu->addSeparator();
  Menu->addAction(LockToolbarsAction);
  Menu->addAction(SelectiveToolbarTextAction);
  Menu->addMenu(FToolbarIconSizeMenu);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ToolbarPopup(TToolbar * Toolbar, const QPoint & GlobalPos)
{
  // upstream's CommanderBarPopup (ExplorerBarPopup)
  QMenu Menu(this);
  Menu.setObjectName(IsExplorer() ? QStringLiteral("ExplorerBarPopup") : QStringLiteral("CommanderBarPopup"));
  for (QAction * Band : FToolbarBands)
  {
    if (Band->objectName() != QStringLiteral("ToolBar2Action"))
    {
      Menu.addAction(Band);
    }
  }
  Menu.addSeparator();
  Menu.addAction(LockToolbarsAction);
  Menu.addAction(SelectiveToolbarTextAction);
  Menu.addMenu(FToolbarIconSizeMenu);
  if ((Toolbar != nullptr) && !Toolbar->GetItemNames().isEmpty() && (Toolbar != QueueToolbar))
  {
    QMenu * CustomizeMenu = Menu.addMenu(tr("&Customize Toolbar"));
    CustomizeMenu->setObjectName(QStringLiteral("CustomizeToolbarMenu"));
    CustomizeMenu->setToolTip(tr("Show/hide toolbar buttons"));
    Toolbar->FillCustomizeMenu(CustomizeMenu);
  }
  Menu.addSeparator();
  Menu.addAction(SessionsTabsAction2);
  if (!IsExplorer())
  {
    Menu.addAction(CommandLinePanelAction);
    for (QAction * Band : FToolbarBands)
    {
      if (Band->objectName() == QStringLiteral("ToolBar2Action"))
      {
        Menu.addAction(Band);
      }
    }
  }
  Menu.addAction(StatusBarAction);
  if (!IsExplorer())
  {
    Menu.addSeparator();
    Menu.addMenu(FLocalPanelMenu);
    Menu.addMenu(FRemotePanelMenu);
  }
  Menu.exec(GlobalPos);
}
//---------------------------------------------------------------------------
