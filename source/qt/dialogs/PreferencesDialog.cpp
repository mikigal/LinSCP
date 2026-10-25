// The preferences (upstream's forms/Preferences.dfm): the pages of the
// navigation tree, loaded from the configuration and saved to it.
//
// Not ported: the pages of upstream's automatic updates and of its Windows
// shell integration, the configuration in the registry, the search of the
// preferences. The Interface, Explorer and Languages pages are added with
// their features.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Bookmarks.h>
#include <CoreMain.h>
#include <CustomCommands.h>
#include <DesktopShortcut.h>
#include <EditorPreferences.h>
#include <PuttyTools.h>
#include <SingleInstance.h>
#include <TerminalManager.h>

#include "CustomDialog.h"
#include "HistoryComboBox.h"
#include "TreePagesDialog.h"

#include <QApplication>
#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDatabase>
#include <QFontDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QToolButton>
#include <QTreeWidget>
//---------------------------------------------------------------------------
const int PrivatePortMin = 49152; // https://en.wikipedia.org/wiki/List_of_TCP_and_UDP_port_numbers#Dynamic,_private_or_ephemeral_ports
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
// The tags of the pages, as upstream's (the nodes of the navigation tree)
enum TPreferencesPage
{
  ppEnvironment = 1, ppLogging = 2, ppInterface = 3, ppPanels = 4, ppCommander = 5, ppExplorer = 6,
  ppEditor = 8, ppIntegration = 9, ppCommands = 10, ppDragDrop = 11, ppBackground = 12, ppStorage = 13,
  ppEndurance = 14, ppTransfer = 16, ppWindow = 17, ppApplications = 18, ppSecurity = 19, ppNetwork = 20,
  ppRemote = 21, ppLocal = 22, ppLanguages = 23, ppEditorInternal = 24, ppFileColors = 25
};
//---------------------------------------------------------------------------
class TPreferencesDialog : public TTreePagesDialog
{
public:
  TPreferencesDialog(QWidget * Parent, TPreferencesMode PreferencesMode);

  bool Execute(TPreferencesDialogData * DialogData);

protected:
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;
  virtual void accept() override;

private:
  TPreferencesMode FPreferencesMode;
  TPreferencesDialogData * FDialogData;
  int FNoUpdate;
  std::unique_ptr<TCustomCommandList> FCustomCommandList;
  std::unique_ptr<TCopyParamList> FCopyParamList;
  std::unique_ptr<TEditorList> FEditorList;
  TCopyParamType FCopyParams;
  TSshHostCA::TList FSshHostCAPlainList;
  QFont FPanelFont;
  QFont FEditorFont;
  TColor FEditorFontColor;
  TColor FEditorBackgroundColor;

  // Environment
  QCheckBox * ConfirmTransferringCheck;
  QCheckBox * DDTransferConfirmationCheck2;
  QCheckBox * ConfirmOverwritingCheck;
  QCheckBox * ConfirmResumeCheck;
  QCheckBox * ConfirmDeletingCheck;
  QCheckBox * ConfirmRecyclingCheck;
  QCheckBox * ConfirmExitOnCompletionCheck;
  QCheckBox * ConfirmCommandSessionCheck;
  QCheckBox * ContinueOnErrorCheck;
  QCheckBox * SynchronizeSummaryCheck;
  QCheckBox * ConfirmClosingSessionCheck2;
  QCheckBox * BeepOnFinishCheck;
  QSpinBox * BeepOnFinishAfterEdit;
  QCheckBox * BalloonNotificationsCheck;
  QCheckBox * FlashTaskbarCheck;
  // Window
  QRadioButton * PathInCaptionFullButton;
  QRadioButton * PathInCaptionShortButton;
  QRadioButton * PathInCaptionNoneButton;
  QCheckBox * MinimizeToTrayCheck;
  QCheckBox * ExternalSessionInExistingInstanceCheck;
  QCheckBox * KeepOpenWhenNoSessionCheck;
  QCheckBox * ShowLoginWhenNoSessionCheck;
  QCheckBox * SessionTabCaptionTruncationCheck;
  QCheckBox * SessionSilentDisconnectCheck;
  QCheckBox * AutoSaveWorkspaceCheck;
  QLabel * AutoWorkspaceLabel;
  QComboBox * AutoWorkspaceCombo;
  QCheckBox * AutoSaveWorkspacePasswordsCheck;
  QCheckBox * WorkspaceConnectAllCheck;
  // Commander
  QComboBox * NortonLikeModeCombo;
  QCheckBox * SwappedPanelsCheck;
  QComboBox * ExplorerKeyboardShortcutsCombo;
  QCheckBox * UseLocationProfilesCheck;
  QCheckBox * CompareByTimeCheck;
  QCheckBox * CompareBySizeCheck;
  // Panels
  QCheckBox * ShowHiddenFilesCheck;
  QCheckBox * DefaultDirIsHomeCheck;
  QCheckBox * PreservePanelStateCheck;
  QCheckBox * RenameWholeNameCheck;
  QComboBox * FormatSizeBytesCombo;
  QCheckBox * NaturalOrderNumericalSortingCheck;
  QCheckBox * AlwaysSortDirectoriesByNameCheck;
  QComboBox * PanelSearchCombo;
  QComboBox * DoubleClickActionCombo;
  QCheckBox * CopyOnDoubleClickConfirmationCheck;
  QCheckBox * PanelFontCheck;
  QLabel * PanelFontLabel;
  QPushButton * PanelFontButton;
  // Remote, Local
  QCheckBox * ShowInaccesibleDirectoriesCheck;
  QCheckBox * AutoReadDirectoryAfterOpCheck;
  QCheckBox * RefreshRemotePanelCheck;
  QSpinBox * RefreshRemotePanelIntervalEdit;
  QCheckBox * PreserveLocalDirectoryCheck;
  QCheckBox * DeleteToRecycleBinCheck;
  // File colors
  TFileColorData::TList FFileColors;
  QTreeWidget * FileColorsView;
  QPushButton * AddFileColorButton;
  QPushButton * EditFileColorButton;
  QPushButton * RemoveFileColorButton;
  QPushButton * UpFileColorButton;
  QPushButton * DownFileColorButton;
  // Editors
  QTreeWidget * EditorListView3;
  QPushButton * AddEditorButton;
  QPushButton * EditEditorButton;
  QPushButton * RemoveEditorButton;
  QPushButton * UpEditorButton;
  QPushButton * DownEditorButton;
  QCheckBox * EditorCheckNotModifiedCheck;
  QCheckBox * EditorWordWrapCheck;
  QSpinBox * EditorTabSizeEdit;
  QComboBox * EditorEncodingCombo;
  QLabel * EditorFontLabel;
  QCheckBox * EditorWarnLargeFileCheck;
  QSpinBox * EditorLargeFileSizeEdit;
  // Transfer
  QTreeWidget * CopyParamListView;
  QLabel * CopyParamLabel;
  QPushButton * AddCopyParamButton;
  QPushButton * EditCopyParamButton;
  QPushButton * DuplicateCopyParamButton;
  QPushButton * RemoveCopyParamButton;
  QPushButton * UpCopyParamButton;
  QPushButton * DownCopyParamButton;
  QCheckBox * CopyParamAutoSelectNoticeCheck;
  // Drag & Drop
  QRadioButton * DDFakeFileEnabledButton;
  QRadioButton * DDFakeFileDisabledButton;
  QCheckBox * DDWarnLackOfTempSpaceCheck;
  // Background
  QSpinBox * QueueTransferLimitEdit;
  QCheckBox * QueueCheck;
  QCheckBox * QueueNoConfirmationCheck;
  QCheckBox * QueueParallelCheck;
  QCheckBox * ParallelTransferCheck;
  QComboBox * ParallelTransferThresholdCombo;
  QCheckBox * EnableQueueByDefaultCheck;
  QCheckBox * QueueAutoPopupCheck;
  QComboBox * QueueKeepDoneItemsForCombo;
  QRadioButton * QueueViewShowButton;
  QRadioButton * QueueViewHideWhenEmptyButton;
  QRadioButton * QueueViewHideButton;
  // Endurance
  QRadioButton * ResumeOnButton;
  QRadioButton * ResumeSmartButton;
  QRadioButton * ResumeOffButton;
  QSpinBox * ResumeThresholdEdit;
  QCheckBox * SessionReopenAutoCheck;
  QSpinBox * SessionReopenAutoEdit;
  QCheckBox * SessionReopenAutoIdleCheck;
  QCheckBox * SessionReopenAutoInactiveCheck;
  QSpinBox * SessionReopenAutoIdleEdit;
  QCheckBox * SessionReopenAutoStallCheck;
  QSpinBox * SessionReopenAutoStallEdit;
  QSpinBox * SessionReopenTimeoutEdit;
  // Network
  QRadioButton * RetrieveExternalIpAddressButton;
  QRadioButton * CustomExternalIpAddressButton;
  QLineEdit * CustomExternalIpAddressEdit;
  QCheckBox * LocalPortNumberCheck;
  QSpinBox * LocalPortNumberMinEdit;
  QSpinBox * LocalPortNumberMaxEdit;
  QCheckBox * TryFtpWhenSshFailsCheck;
  // Security
  QCheckBox * UseMasterPasswordCheck;
  QPushButton * SetMasterPasswordButton;
  QCheckBox * SessionRememberPasswordCheck;
  QTreeWidget * SshHostCAsView;
  QPushButton * AddSshHostCAButton;
  QPushButton * EditSshHostCAButton;
  QPushButton * RemoveSshHostCAButton;
  // Logging
  QCheckBox * EnableLoggingCheck;
  QComboBox * LogProtocolCombo2;
  QLineEdit * LogFileNameEdit3;
  QRadioButton * LogFileAppendButton;
  QRadioButton * LogFileOverwriteButton;
  QCheckBox * LogMaxSizeCheck;
  QComboBox * LogMaxSizeCombo;
  QCheckBox * LogMaxSizeCountCheck;
  QSpinBox * LogMaxSizeCountEdit;
  QCheckBox * LogSensitiveCheck;
  QLabel * LogProtocolHintLabel;
  QCheckBox * EnableActionsLoggingCheck;
  QLineEdit * ActionsLogFileNameEdit;
  // Applications
  THistoryComboBox * PuttyPathEdit;
  QCheckBox * PuttyPasswordCheck2;
  QCheckBox * AutoOpenInPuttyCheck;
  QCheckBox * TelnetForFtpInPuttyCheck;
  // Commands
  QTreeWidget * CustomCommandsView;
  QPushButton * AddCommandButton;
  QPushButton * EditCommandButton;
  QPushButton * RemoveCommandButton;
  QPushButton * UpCommandButton;
  QPushButton * DownCommandButton;
  // Storage
  QRadioButton * DDSystemTemporaryDirectoryButton;
  QRadioButton * DDCustomTemporaryDirectoryButton;
  QLineEdit * DDTemporaryDirectoryEdit;
  QCheckBox * TemporaryDirectoryAppendSessionCheck;
  QCheckBox * TemporaryDirectoryAppendPathCheck;
  QCheckBox * TemporaryDirectoryDeterministicCheck;
  QCheckBox * TemporaryDirectoryCleanupCheck;
  QCheckBox * ConfirmTemporaryDirectoryCleanupCheck;
  QLineEdit * RandomSeedFileEdit;

  void CreateEnvironmentSheet();
  void CreateWindowSheet();
  void CreateCommanderSheet();
  void CreatePanelsSheets();
  void CreateFileColorsSheet();
  void CreateEditorSheets();
  void CreateTransferSheets();
  void CreateNetworkSheet();
  void CreateSecuritySheet();
  void CreateLogSheet();
  void CreateIntegrationSheets();
  void CreateCommandsSheet();
  void CreateStorageSheet();

  void LoadConfiguration();
  void SaveConfiguration();
  virtual void UpdateControls() override;

  void UpdateCustomCommandsView();
  TShortCuts GetShortCuts();
  void AddEditCommand(bool Edit);
  void RemoveCommand();
  void CustomCommandMove(int Source, int Dest);
  const TCopyParamType * GetCopyParam(int Index);
  void UpdateCopyParamListView();
  void CopyParamMove(int Source, int Dest);
  void RemoveCopyParam();
  void AddEditCopyParam(TCopyParamPresetMode Mode);
  void UpdateEditorListView();
  void EditorMove(int Source, int Dest);
  void RemoveEditor();
  void AddEditEditor(bool Edit);
  void UpdateFileColorsView();
  void AddEditFileColor(bool Edit);
  void RemoveFileColor();
  void FileColorMove(int Source, int Dest);
  void UpdateSshHostCAsView();
  void AddEditSshHostCA(bool Edit);
  void RemoveSshHostCA();
  void PanelFontButtonClick();
  void EditorFontButtonClick();
  void EditorColorButtonClick(bool Background, const QPoint & Pos);
  void UseMasterPasswordCheckClick();
  void ChangeMasterPassword(const UnicodeString & Message);
  void MasterPasswordChanged(const UnicodeString & Message, TStrings * RecryptPasswordErrors);
  bool CanSetMasterPassword();
  void DesktopIconButtonClick();
  void RegisterAsUrlHandlersButtonClick();
  void SizeComboExit(QComboBox * Combo);
};
//---------------------------------------------------------------------------
TPreferencesDialog::TPreferencesDialog(QWidget * Parent, TPreferencesMode PreferencesMode) :
  TTreePagesDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("PreferencesDialog"));
  setWindowTitle(tr("Preferences"));
  FPreferencesMode = PreferencesMode;
  FDialogData = NULL;
  FNoUpdate = 0;
  FCustomCommandList.reset(new TCustomCommandList());
  FCopyParamList.reset(new TCopyParamList());
  FEditorList.reset(new TEditorList());
  FEditorFontColor = TColor(0);
  FEditorBackgroundColor = TColor(0);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  Buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("OKButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CloseButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout()->addWidget(Buttons);

  CreateEnvironmentSheet();
  CreatePanelsSheets();
  CreateEditorSheets();
  CreateTransferSheets();
  CreateNetworkSheet();
  CreateSecuritySheet();
  CreateLogSheet();
  CreateIntegrationSheets();
  CreateCommandsSheet();
  CreateStorageSheet();
  NavigationTree->expandAll();

  // The changes of the controls update the others
  ConnectControls();

  resize(fontMetrics().averageCharWidth() * 120, fontMetrics().height() * 38);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateEnvironmentSheet()
{
  QWidget * PreferencesSheet = AddSheet("PreferencesSheet", tr("Environment"), ppEnvironment, nullptr);
  QGroupBox * CommonPreferencesGroup = AddGroup(PreferencesSheet, "CommonPreferencesGroup", tr("Confirmations"));
  QGridLayout * Grid = new QGridLayout();
  delete CommonPreferencesGroup->layout();
  CommonPreferencesGroup->setLayout(Grid);
  struct { QCheckBox ** Check; const char * Name; QString Caption; int Column; } Checks[] =
  {
    { &ConfirmTransferringCheck, "ConfirmTransferringCheck", tr("&Transferring of files"), 0 },
    { &DDTransferConfirmationCheck2, "DDTransferConfirmationCheck2", tr("D&rag && drop operations and paste to other applications"), 0 },
    { &ConfirmOverwritingCheck, "ConfirmOverwritingCheck", tr("&Overwriting of files"), 0 },
    { &ConfirmResumeCheck, "ConfirmResumeCheck", tr("Trans&fer resuming"), 0 },
    { &ConfirmDeletingCheck, "ConfirmDeletingCheck", tr("&Deleting of files (recommended)"), 0 },
    { &ConfirmRecyclingCheck, "ConfirmRecyclingCheck", tr("&Moving files to trash"), 0 },
    { &ConfirmClosingSessionCheck2, "ConfirmClosingSessionCheck2", tr("Closing sessions when exiting appli&cation"), 1 },
    { &ConfirmExitOnCompletionCheck, "ConfirmExitOnCompletionCheck", tr("Exiting application on o&peration completion"), 1 },
    { &ConfirmCommandSessionCheck, "ConfirmCommandSessionCheck", tr("Opening separate &shell session"), 1 },
    { &SynchronizeSummaryCheck, "SynchronizeSummaryCheck", tr("Sync&hronization summary"), 1 },
    { &ContinueOnErrorCheck, "ContinueOnErrorCheck", tr("Continue on &error (advanced users)"), 1 },
  };
  int Rows[2] = { 0, 0 };
  for (auto & Check : Checks)
  {
    *Check.Check = new QCheckBox(Check.Caption, CommonPreferencesGroup);
    (*Check.Check)->setObjectName(QString::fromLatin1(Check.Name));
    Grid->addWidget(*Check.Check, Rows[Check.Column]++, Check.Column);
  }
  // The background transfers have options of their own
  QLabel * BackgroundConfirmationsLink =
    new QLabel(QStringLiteral("<a href=\"#\">%1</a>").arg(tr("Change confirmations of background transfers")), CommonPreferencesGroup);
  BackgroundConfirmationsLink->setObjectName(QStringLiteral("BackgroundConfirmationsLink"));
  connect(BackgroundConfirmationsLink, &QLabel::linkActivated, this, [this]() { SetActivePage(ppBackground); });
  Grid->addWidget(BackgroundConfirmationsLink, std::max(Rows[0], Rows[1]), 0, 1, 2);

  QGroupBox * NotificationsGroup = AddGroup(PreferencesSheet, "NotificationsGroup", tr("Notifications"));
  QHBoxLayout * BeepRow = AddRow(NotificationsGroup);
  BeepOnFinishCheck = new QCheckBox(tr("&Beep when work finishes, if it lasted more than"), NotificationsGroup);
  BeepOnFinishCheck->setObjectName(QStringLiteral("BeepOnFinishCheck"));
  BeepRow->addWidget(BeepOnFinishCheck);
  BeepOnFinishAfterEdit = CreateSpin(NotificationsGroup, "BeepOnFinishAfterEdit", 0, 999, 15);
  BeepOnFinishAfterEdit->setSuffix(tr(" s"));
  BeepRow->addWidget(BeepOnFinishAfterEdit);
  BeepRow->addStretch(1);
  BalloonNotificationsCheck = AddCheck(NotificationsGroup, "BalloonNotificationsCheck", tr("Show desktop &notifications"));
  FlashTaskbarCheck = AddCheck(NotificationsGroup, "FlashTaskbarCheck", tr("&Flash taskbar button when attention is needed"));
  static_cast<QVBoxLayout *>(PreferencesSheet->layout())->addStretch(1);

  CreateWindowSheet();
  CreateCommanderSheet();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateWindowSheet()
{
  QWidget * WindowSheet = AddSheet("WindowSheet", tr("Window"), ppWindow, NodeOf(ppEnvironment));
  QGroupBox * WorkspacesGroup = AddGroup(WindowSheet, "WorkspacesGroup", tr("Workspaces"));
  AutoSaveWorkspaceCheck = AddCheck(WorkspacesGroup, "AutoSaveWorkspaceCheck", tr("&Automatically save workspace on exit"));
  QHBoxLayout * AutoWorkspaceRow = AddRow(WorkspacesGroup);
  AutoWorkspaceLabel = new QLabel(tr("&Default workspace name:"), WorkspacesGroup);
  AutoWorkspaceLabel->setObjectName(QStringLiteral("AutoWorkspaceLabel"));
  AutoWorkspaceRow->addWidget(AutoWorkspaceLabel);
  std::unique_ptr<TStrings> Workspaces(StoredSessions->GetWorkspaces());
  QStringList WorkspaceNames;
  for (int Index = 0; Index < Workspaces->Count; Index++)
  {
    WorkspaceNames.append(ToQString(Workspaces->Strings[Index]));
  }
  AutoWorkspaceCombo = CreateCombo(WorkspacesGroup, "AutoWorkspaceCombo", WorkspaceNames, true);
  AutoWorkspaceLabel->setBuddy(AutoWorkspaceCombo);
  AutoWorkspaceRow->addWidget(AutoWorkspaceCombo, 1);
  AutoSaveWorkspacePasswordsCheck =
    AddCheck(WorkspacesGroup, "AutoSaveWorkspacePasswordsCheck", ToQString(LoadStr(SAVE_WORKSPACE_PASSWORDS)));
  WorkspaceConnectAllCheck =
    AddCheck(WorkspacesGroup, "WorkspaceConnectAllCheck", tr("&Connect all sessions of opened workspace or site folder"));

  QGroupBox * PathInCaptionGroup = AddGroup(WindowSheet, "PathInCaptionGroup", tr("Path in window title"));
  PathInCaptionFullButton = AddRadio(PathInCaptionGroup, "PathInCaptionFullButton", tr("Show &full path"));
  PathInCaptionShortButton = AddRadio(PathInCaptionGroup, "PathInCaptionShortButton", tr("Sho&w short path"));
  PathInCaptionNoneButton = AddRadio(PathInCaptionGroup, "PathInCaptionNoneButton", tr("Do &not show"));

  QGroupBox * WindowMiscellaneousGroup = AddGroup(WindowSheet, "WindowMiscellaneousGroup", tr("Miscellaneous"));
  MinimizeToTrayCheck =
    AddCheck(WindowMiscellaneousGroup, "MinimizeToTrayCheck", tr("&Minimize main window to system tray"));
  ExternalSessionInExistingInstanceCheck =
    AddCheck(WindowMiscellaneousGroup, "ExternalSessionInExistingInstanceCheck", tr("Open new externally initiated sessions in &existing window"));
  ShowLoginWhenNoSessionCheck =
    AddCheck(WindowMiscellaneousGroup, "ShowLoginWhenNoSessionCheck", tr("Show &Login dialog on startup and when the last session is closed"));
  KeepOpenWhenNoSessionCheck =
    AddCheck(WindowMiscellaneousGroup, "KeepOpenWhenNoSessionCheck", tr("&Keep main window open when the last session is closed"));
  SessionTabCaptionTruncationCheck =
    AddCheck(WindowMiscellaneousGroup, "SessionTabCaptionTruncationCheck", tr("&Truncate tab titles when they do not fit to window"));
  SessionSilentDisconnectCheck =
    AddCheck(WindowMiscellaneousGroup, "SessionSilentDisconnectCheck", tr("Do not p&opup error message when idle session is disconnected"));
  static_cast<QVBoxLayout *>(WindowSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateCommanderSheet()
{
  QWidget * CommanderSheet = AddSheet("CommanderSheet", tr("Commander"), ppCommander, NodeOf(ppEnvironment));
  QLabel * Label3 = new QLabel(tr("Preferences on this page apply to Commander interface only."), CommanderSheet);
  CommanderSheet->layout()->addWidget(Label3);
  QGroupBox * PanelsGroup = AddGroup(CommanderSheet, "PanelsGroup", tr("Panels"));
  QHBoxLayout * NortonLikeModeRow = AddRow(PanelsGroup);
  QLabel * Label8 = new QLabel(tr("&Explorer-style selection:"), PanelsGroup);
  NortonLikeModeRow->addWidget(Label8);
  NortonLikeModeCombo =
    CreateCombo(PanelsGroup, "NortonLikeModeCombo", { tr("Never"), tr("Mouse only"), tr("Mouse and Keyboard") });
  Label8->setBuddy(NortonLikeModeCombo);
  NortonLikeModeRow->addWidget(NortonLikeModeCombo);
  NortonLikeModeRow->addStretch(1);
  SwappedPanelsCheck = AddCheck(PanelsGroup, "SwappedPanelsCheck", tr("S&wap panels (local on right, remote on left)"));

  QGroupBox * CommanderMiscGroup = AddGroup(CommanderSheet, "CommanderMiscGroup", tr("Miscellaneous"));
  QHBoxLayout * ShortcutsRow = AddRow(CommanderMiscGroup);
  QLabel * Label10 = new QLabel(tr("&Keyboard shortcuts"), CommanderMiscGroup);
  ShortcutsRow->addWidget(Label10);
  ExplorerKeyboardShortcutsCombo =
    CreateCombo(CommanderMiscGroup, "ExplorerKeyboardShortcutsCombo", { tr("Commander"), tr("Explorer") });
  Label10->setBuddy(ExplorerKeyboardShortcutsCombo);
  ShortcutsRow->addWidget(ExplorerKeyboardShortcutsCombo);
  ShortcutsRow->addStretch(1);
  UseLocationProfilesCheck =
    AddCheck(CommanderMiscGroup, "UseLocationProfilesCheck", tr("&Use Location Profiles instead of Directory Bookmarks"));

  QGroupBox * CompareCriterionsGroup = AddGroup(CommanderSheet, "CompareCriterionsGroup", tr("Compare directory criteria"));
  CompareByTimeCheck = AddCheck(CompareCriterionsGroup, "CompareByTimeCheck", tr("Compare by &time"));
  CompareBySizeCheck = AddCheck(CompareCriterionsGroup, "CompareBySizeCheck", tr("Compare by &size"));
  static_cast<QVBoxLayout *>(CommanderSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreatePanelsSheets()
{
  QWidget * PanelsSheet = AddSheet("PanelsSheet", tr("Panels"), ppPanels, nullptr);
  QGroupBox * PanelsCommonGroup = AddGroup(PanelsSheet, "PanelsCommonGroup", tr("Common"));
  ShowHiddenFilesCheck = AddCheck(PanelsCommonGroup, "ShowHiddenFilesCheck", tr("&Show hidden files (Ctrl+Alt+H)"));
  DefaultDirIsHomeCheck = AddCheck(PanelsCommonGroup, "DefaultDirIsHomeCheck", tr("Default directory is &home directory"));
  PreservePanelStateCheck = AddCheck(PanelsCommonGroup, "PreservePanelStateCheck", tr("&Remember panels' state when switching sessions"));
  RenameWholeNameCheck = AddCheck(PanelsCommonGroup, "RenameWholeNameCheck", tr("Select &whole name when renaming file"));
  NaturalOrderNumericalSortingCheck =
    AddCheck(PanelsCommonGroup, "NaturalOrderNumericalSortingCheck", tr("Use &natural order numerical sorting"));
  AlwaysSortDirectoriesByNameCheck =
    AddCheck(PanelsCommonGroup, "AlwaysSortDirectoriesByNameCheck", tr("Always sort &directories by name"));
  QGridLayout * PanelsGrid = new QGridLayout();
  static_cast<QBoxLayout *>(PanelsCommonGroup->layout())->addLayout(PanelsGrid);
  QLabel * Label1 = new QLabel(tr("Show file si&zes in:"), PanelsCommonGroup);
  FormatSizeBytesCombo = CreateCombo(PanelsCommonGroup, "FormatSizeBytesCombo", { tr("Bytes"), tr("Kilobytes"), tr("Short format") });
  Label1->setBuddy(FormatSizeBytesCombo);
  PanelsGrid->addWidget(Label1, 0, 0);
  PanelsGrid->addWidget(FormatSizeBytesCombo, 0, 1);
  QLabel * Label2 = new QLabel(tr("&Incremental search:"), PanelsCommonGroup);
  PanelSearchCombo =
    CreateCombo(PanelsCommonGroup, "PanelSearchCombo", { tr("Beginning of name only"), tr("Any part of name"), tr("All columns") });
  Label2->setBuddy(PanelSearchCombo);
  PanelsGrid->addWidget(Label2, 1, 0);
  PanelsGrid->addWidget(PanelSearchCombo, 1, 1);
  PanelsGrid->setColumnStretch(2, 1);

  QGroupBox * DoubleClickGroup = AddGroup(PanelsSheet, "DoubleClickGroup", tr("Double-click"));
  QHBoxLayout * DoubleClickRow = AddRow(DoubleClickGroup);
  QLabel * DoubleClickActionLabel = new QLabel(tr("&Operation to perform on double-click:"), DoubleClickGroup);
  DoubleClickActionLabel->setObjectName(QStringLiteral("DoubleClickActionLabel"));
  DoubleClickRow->addWidget(DoubleClickActionLabel);
  DoubleClickActionCombo = CreateCombo(DoubleClickGroup, "DoubleClickActionCombo", { tr("Open"), tr("Copy"), tr("Edit") });
  DoubleClickActionLabel->setBuddy(DoubleClickActionCombo);
  DoubleClickRow->addWidget(DoubleClickActionCombo);
  DoubleClickRow->addStretch(1);
  CopyOnDoubleClickConfirmationCheck =
    AddCheck(DoubleClickGroup, "CopyOnDoubleClickConfirmationCheck", tr("&Confirm copy on double-click operation"));

  QGroupBox * PanelFontGroup = AddGroup(PanelsSheet, "PanelFontGroup", tr("Panel font"));
  QHBoxLayout * PanelFontRow = AddRow(PanelFontGroup);
  PanelFontCheck = new QCheckBox(tr("&Use custom font"), PanelFontGroup);
  PanelFontCheck->setObjectName(QStringLiteral("PanelFontCheck"));
  PanelFontRow->addWidget(PanelFontCheck);
  PanelFontButton = CreateButton(PanelFontGroup, "PanelFontButton", tr("Select fon&t..."));
  PanelFontRow->addWidget(PanelFontButton);
  PanelFontRow->addStretch(1);
  connect(PanelFontButton, &QPushButton::clicked, this, [this]() { PanelFontButtonClick(); });
  PanelFontLabel = new QLabel(PanelFontGroup);
  PanelFontLabel->setObjectName(QStringLiteral("PanelFontLabel"));
  PanelFontLabel->setFrameShape(QFrame::StyledPanel);
  PanelFontGroup->layout()->addWidget(PanelFontLabel);
  static_cast<QVBoxLayout *>(PanelsSheet->layout())->addStretch(1);

  QWidget * PanelRemoteSheet = AddSheet("PanelRemoteSheet", tr("Remote"), ppRemote, NodeOf(ppPanels));
  QGroupBox * PanelsRemoteDirectoryGroup = AddGroup(PanelRemoteSheet, "PanelsRemoteDirectoryGroup", tr("Remote panel"));
  ShowInaccesibleDirectoriesCheck =
    AddCheck(PanelsRemoteDirectoryGroup, "ShowInaccesibleDirectoriesCheck", tr("Show in&accessible directories"));
  AutoReadDirectoryAfterOpCheck =
    AddCheck(PanelsRemoteDirectoryGroup, "AutoReadDirectoryAfterOpCheck", tr("Auto&matically refresh directory after operation"));
  QHBoxLayout * RefreshRow = AddRow(PanelsRemoteDirectoryGroup);
  RefreshRemotePanelCheck = new QCheckBox(tr("Refresh remote panel &every"), PanelsRemoteDirectoryGroup);
  RefreshRemotePanelCheck->setObjectName(QStringLiteral("RefreshRemotePanelCheck"));
  RefreshRow->addWidget(RefreshRemotePanelCheck);
  RefreshRemotePanelIntervalEdit = CreateSpin(PanelsRemoteDirectoryGroup, "RefreshRemotePanelIntervalEdit", 10, 9999, 15);
  RefreshRemotePanelIntervalEdit->setSuffix(tr(" s"));
  RefreshRow->addWidget(RefreshRemotePanelIntervalEdit);
  RefreshRow->addStretch(1);
  static_cast<QVBoxLayout *>(PanelRemoteSheet->layout())->addStretch(1);

  QWidget * PanelLocalSheet = AddSheet("PanelLocalSheet", tr("Local"), ppLocal, NodeOf(ppPanels));
  QGroupBox * LocalPanelGroup = AddGroup(PanelLocalSheet, "LocalPanelGroup", tr("Local panel"));
  PreserveLocalDirectoryCheck =
    AddCheck(LocalPanelGroup, "PreserveLocalDirectoryCheck", tr("Do not &change state when switching sessions"));
  DeleteToRecycleBinCheck = AddCheck(LocalPanelGroup, "DeleteToRecycleBinCheck", tr("&Delete files to trash"));
  static_cast<QVBoxLayout *>(PanelLocalSheet->layout())->addStretch(1);

  CreateFileColorsSheet();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateFileColorsSheet()
{
  QWidget * FileColorsSheet = AddSheet("FileColorsSheet", tr("File colors"), ppFileColors, NodeOf(ppPanels));
  QGroupBox * FileColorsGroup = AddGroup(FileColorsSheet, "FileColorsGroup", tr("File colors"));
  QHBoxLayout * FileColorsRow = AddRow(FileColorsGroup);
  FileColorsView = CreateListView(FileColorsGroup, "FileColorsView", { tr("Mask") });
  FileColorsView->setHeaderHidden(true);
  FileColorsView->setMinimumHeight(fontMetrics().height() * 15);
  AddFileColorButton = CreateButton(FileColorsGroup, "AddFileColorButton", tr("&Add..."));
  EditFileColorButton = CreateButton(FileColorsGroup, "EditFileColorButton", tr("&Edit..."));
  RemoveFileColorButton = CreateButton(FileColorsGroup, "RemoveFileColorButton", tr("&Remove"));
  UpFileColorButton = CreateButton(FileColorsGroup, "UpFileColorButton", tr("&Up"));
  DownFileColorButton = CreateButton(FileColorsGroup, "DownFileColorButton", tr("&Down"));
  AddListButtons(FileColorsGroup, FileColorsRow, FileColorsView,
    { AddFileColorButton, EditFileColorButton, RemoveFileColorButton, UpFileColorButton, DownFileColorButton });
  connect(AddFileColorButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditFileColor(false); }); });
  connect(EditFileColorButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditFileColor(true); }); });
  connect(RemoveFileColorButton, &QPushButton::clicked, this, [this]() { RemoveFileColor(); });
  connect(UpFileColorButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(FileColorsView);
    FileColorMove(Index, Index - 1);
  });
  connect(DownFileColorButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(FileColorsView);
    FileColorMove(Index, Index + 1);
  });
  connect(FileColorsView, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditFileColorButton->isEnabled())
    {
      ExecuteHandler([this]() { AddEditFileColor(true); });
    }
  });
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateEditorSheets()
{
  QWidget * EditorSheet = AddSheet("EditorSheet", tr("Editors"), ppEditor, nullptr);
  QGroupBox * EditorPreferenceGroup = AddGroup(EditorSheet, "EditorPreferenceGroup", tr("Editor preference"));
  QHBoxLayout * EditorsRow = AddRow(EditorPreferenceGroup);
  EditorListView3 = CreateListView(EditorPreferenceGroup, "EditorListView3", { tr("Editor"), tr("Mask"), tr("Text") });
  EditorListView3->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  AddEditorButton = CreateButton(EditorPreferenceGroup, "AddEditorButton", tr("&Add..."));
  EditEditorButton = CreateButton(EditorPreferenceGroup, "EditEditorButton", tr("&Edit..."));
  UpEditorButton = CreateButton(EditorPreferenceGroup, "UpEditorButton", tr("&Up"));
  DownEditorButton = CreateButton(EditorPreferenceGroup, "DownEditorButton", tr("&Down"));
  RemoveEditorButton = CreateButton(EditorPreferenceGroup, "RemoveEditorButton", tr("&Remove"));
  AddListButtons(EditorPreferenceGroup, EditorsRow, EditorListView3,
    { AddEditorButton, EditEditorButton, UpEditorButton, DownEditorButton, RemoveEditorButton });
  connect(AddEditorButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditEditor(false); }); });
  connect(EditEditorButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditEditor(true); }); });
  connect(RemoveEditorButton, &QPushButton::clicked, this, [this]() { RemoveEditor(); });
  connect(UpEditorButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(EditorListView3);
    EditorMove(Index, Index - 1);
  });
  connect(DownEditorButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(EditorListView3);
    EditorMove(Index, Index + 1);
  });
  connect(EditorListView3, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditEditorButton->isEnabled())
    {
      ExecuteHandler([this]() { AddEditEditor(true); });
    }
  });
  QGroupBox * EditingOptionsGroup = AddGroup(EditorSheet, "EditingOptionsGroup", tr("Editing options"));
  EditorCheckNotModifiedCheck =
    AddCheck(EditingOptionsGroup, "EditorCheckNotModifiedCheck", tr("&Check that edited remote file has not changed before saving it"));

  QWidget * EditorInternalSheet = AddSheet("EditorInternalSheet", tr("Internal editor"), ppEditorInternal, NodeOf(ppEditor));
  QGroupBox * InternalEditorGroup = AddGroup(EditorInternalSheet, "InternalEditorGroup", tr("Display"));
  EditorWordWrapCheck = AddCheck(InternalEditorGroup, "EditorWordWrapCheck", tr("&Wrap long lines"));
  QGridLayout * DisplayGrid = new QGridLayout();
  static_cast<QBoxLayout *>(InternalEditorGroup->layout())->addLayout(DisplayGrid);
  QLabel * Label9 = new QLabel(tr("&Tabulator size:"), InternalEditorGroup);
  EditorTabSizeEdit = CreateSpin(InternalEditorGroup, "EditorTabSizeEdit", 1, 99);
  Label9->setBuddy(EditorTabSizeEdit);
  DisplayGrid->addWidget(Label9, 0, 0);
  DisplayGrid->addWidget(EditorTabSizeEdit, 0, 1);
  QLabel * Label11 = new QLabel(tr("Default en&coding:"), InternalEditorGroup);
  // upstream's other encoding is the ANSI code page of Windows
  EditorEncodingCombo = CreateCombo(InternalEditorGroup, "EditorEncodingCombo", { QStringLiteral("ISO-8859-1"), ToQString(LoadStr(UTF8_NAME)) });
  Label11->setBuddy(EditorEncodingCombo);
  DisplayGrid->addWidget(Label11, 1, 0);
  DisplayGrid->addWidget(EditorEncodingCombo, 1, 1);
  DisplayGrid->setColumnStretch(2, 1);

  QGroupBox * FontGroup = AddGroup(EditorInternalSheet, "FontGroup", tr("Font"));
  QHBoxLayout * FontRow = AddRow(FontGroup);
  EditorFontLabel = new QLabel(FontGroup);
  EditorFontLabel->setObjectName(QStringLiteral("EditorFontLabel"));
  EditorFontLabel->setFrameShape(QFrame::StyledPanel);
  EditorFontLabel->setAutoFillBackground(true);
  EditorFontLabel->setMinimumHeight(fontMetrics().height() * 5);
  FontRow->addWidget(EditorFontLabel, 1);
  QVBoxLayout * FontButtons = new QVBoxLayout();
  QPushButton * EditorFontButton = CreateButton(FontGroup, "EditorFontButton", tr("&Select font..."));
  connect(EditorFontButton, &QPushButton::clicked, this, [this]() { EditorFontButtonClick(); });
  QPushButton * EditorFontColorButton = CreateButton(FontGroup, "EditorFontColorButton", tr("&Text color"));
  connect(EditorFontColorButton, &QPushButton::clicked, this, [this, EditorFontColorButton]()
  {
    EditorColorButtonClick(false, EditorFontColorButton->mapToGlobal(QPoint(0, EditorFontColorButton->height())));
  });
  QPushButton * EditorBackgroundColorButton = CreateButton(FontGroup, "EditorBackgroundColorButton", tr("Default &background"));
  connect(EditorBackgroundColorButton, &QPushButton::clicked, this, [this, EditorBackgroundColorButton]()
  {
    EditorColorButtonClick(true, EditorBackgroundColorButton->mapToGlobal(QPoint(0, EditorBackgroundColorButton->height())));
  });
  FontButtons->addWidget(EditorFontButton);
  FontButtons->addWidget(EditorFontColorButton);
  FontButtons->addWidget(EditorBackgroundColorButton);
  FontButtons->addStretch(1);
  FontRow->addLayout(FontButtons);

  QGroupBox * InternalEditorBehaviourGroup = AddGroup(EditorInternalSheet, "InternalEditorBehaviourGroup", tr("Behaviour"));
  QHBoxLayout * LargeFileRow = AddRow(InternalEditorBehaviourGroup);
  EditorWarnLargeFileCheck = new QCheckBox(tr("Warn when &editing files larger than:"), InternalEditorBehaviourGroup);
  EditorWarnLargeFileCheck->setObjectName(QStringLiteral("EditorWarnLargeFileCheck"));
  LargeFileRow->addWidget(EditorWarnLargeFileCheck);
  EditorLargeFileSizeEdit = CreateSpin(InternalEditorBehaviourGroup, "EditorLargeFileSizeEdit", 1, 1048576, 1024);
  EditorLargeFileSizeEdit->setSuffix(tr(" KB"));
  LargeFileRow->addWidget(EditorLargeFileSizeEdit);
  LargeFileRow->addStretch(1);
  static_cast<QVBoxLayout *>(EditorInternalSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateTransferSheets()
{
  QWidget * CopyParamListSheet = AddSheet("CopyParamListSheet", tr("Transfer"), ppTransfer, nullptr);
  QGroupBox * CopyParamListGroup = AddGroup(CopyParamListSheet, "CopyParamListGroup", tr("Transfer settings presets"));
  QHBoxLayout * CopyParamRow = AddRow(CopyParamListGroup);
  CopyParamListView = CreateListView(CopyParamListGroup, "CopyParamListView", { tr("Preset description"), tr("Auto") });
  CopyParamListView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  AddCopyParamButton = CreateButton(CopyParamListGroup, "AddCopyParamButton", tr("&Add..."));
  EditCopyParamButton = CreateButton(CopyParamListGroup, "EditCopyParamButton", tr("&Edit..."));
  DuplicateCopyParamButton = CreateButton(CopyParamListGroup, "DuplicateCopyParamButton", tr("Du&plicate..."));
  RemoveCopyParamButton = CreateButton(CopyParamListGroup, "RemoveCopyParamButton", tr("&Remove"));
  UpCopyParamButton = CreateButton(CopyParamListGroup, "UpCopyParamButton", tr("&Up"));
  DownCopyParamButton = CreateButton(CopyParamListGroup, "DownCopyParamButton", tr("&Down"));
  AddListButtons(CopyParamListGroup, CopyParamRow, CopyParamListView,
    { AddCopyParamButton, EditCopyParamButton, DuplicateCopyParamButton, RemoveCopyParamButton, UpCopyParamButton, DownCopyParamButton });
  connect(AddCopyParamButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditCopyParam(cpmAdd); }); });
  connect(EditCopyParamButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditCopyParam(cpmEdit); }); });
  connect(DuplicateCopyParamButton, &QPushButton::clicked, this,
    [this]() { ExecuteHandler([this]() { AddEditCopyParam(cpmDuplicate); }); });
  connect(RemoveCopyParamButton, &QPushButton::clicked, this, [this]() { RemoveCopyParam(); });
  connect(UpCopyParamButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(CopyParamListView);
    CopyParamMove(Index, Index - 1);
  });
  connect(DownCopyParamButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(CopyParamListView);
    CopyParamMove(Index, Index + 1);
  });
  connect(CopyParamListView, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditCopyParamButton->isEnabled())
    {
      ExecuteHandler([this]() { AddEditCopyParam(cpmEdit); });
    }
  });
  CopyParamLabel = new QLabel(CopyParamListGroup);
  CopyParamLabel->setObjectName(QStringLiteral("CopyParamLabel"));
  CopyParamLabel->setWordWrap(true);
  CopyParamListGroup->layout()->addWidget(CopyParamLabel);
  CopyParamAutoSelectNoticeCheck =
    AddCheck(CopyParamListGroup, "CopyParamAutoSelectNoticeCheck", tr("&Announce when transfer settings preset is autoselected"));

  QWidget * DragDropSheet = AddSheet("DragDropSheet", tr("Drag & Drop"), ppDragDrop, NodeOf(ppTransfer));
  QGroupBox * DragDropDownloadsGroup = AddGroup(DragDropSheet, "DragDropDownloadsGroup", tr("Drag && Drop downloads"));
  DDFakeFileEnabledButton =
    AddRadio(DragDropDownloadsGroup, "DDFakeFileEnabledButton", tr("Determine drop target by the &file manager"));
  QLabel * DDFakeFileEnabledLabel =
    new QLabel(tr("Files are downloaded directly to the directory where they are dropped or pasted, "
                  "which the file manager tells (Dolphin and other KDE applications)."), DragDropDownloadsGroup);
  DDFakeFileEnabledLabel->setObjectName(QStringLiteral("DDFakeFileEnabledLabel"));
  DDFakeFileEnabledLabel->setWordWrap(true);
  DDFakeFileEnabledLabel->setIndent(fontMetrics().averageCharWidth() * 4);
  DragDropDownloadsGroup->layout()->addWidget(DDFakeFileEnabledLabel);
  DDFakeFileDisabledButton =
    AddRadio(DragDropDownloadsGroup, "DDFakeFileDisabledButton", tr("Download files via &temporary folder"));
  QLabel * DDFakeFileDisabledLabel =
    new QLabel(tr("Files are downloaded to a temporary directory once the other application asks for them; "
                  "the application then copies them to their destination."), DragDropDownloadsGroup);
  DDFakeFileDisabledLabel->setObjectName(QStringLiteral("DDFakeFileDisabledLabel"));
  DDFakeFileDisabledLabel->setWordWrap(true);
  DDFakeFileDisabledLabel->setIndent(fontMetrics().averageCharWidth() * 4);
  DragDropDownloadsGroup->layout()->addWidget(DDFakeFileDisabledLabel);
  DDWarnLackOfTempSpaceCheck =
    AddCheck(DragDropDownloadsGroup, "DDWarnLackOfTempSpaceCheck", tr("&Warn when there is not enough free space"));
  static_cast<QVBoxLayout *>(DragDropSheet->layout())->addStretch(1);

  QWidget * QueueSheet = AddSheet("QueueSheet", tr("Background"), ppBackground, NodeOf(ppTransfer));
  QGroupBox * QueueGroup = AddGroup(QueueSheet, "QueueGroup", tr("Background transfers"));
  QHBoxLayout * LimitRow = AddRow(QueueGroup);
  QLabel * Label5 = new QLabel(tr("&Maximal number of transfers at the same time:"), QueueGroup);
  LimitRow->addWidget(Label5);
  QueueTransferLimitEdit = CreateSpin(QueueGroup, "QueueTransferLimitEdit", 1, 9);
  Label5->setBuddy(QueueTransferLimitEdit);
  LimitRow->addWidget(QueueTransferLimitEdit);
  LimitRow->addStretch(1);
  QueueParallelCheck = AddCheck(QueueGroup, "QueueParallelCheck", tr("&Use multiple connections for single transfer"));
  QHBoxLayout * ParallelRow = AddRow(QueueGroup);
  ParallelTransferCheck = new QCheckBox(tr("Use multiple connections for single files abo&ve:"), QueueGroup);
  ParallelTransferCheck->setObjectName(QStringLiteral("ParallelTransferCheck"));
  ParallelRow->addSpacing(fontMetrics().averageCharWidth() * 4);
  ParallelRow->addWidget(ParallelTransferCheck);
  ParallelTransferThresholdCombo =
    CreateCombo(QueueGroup, "ParallelTransferThresholdCombo", { QStringLiteral("1M"), QStringLiteral("10M"), QStringLiteral("100M"), QStringLiteral("1G") }, true);
  ParallelTransferThresholdCombo->installEventFilter(this);
  ParallelRow->addWidget(ParallelTransferThresholdCombo);
  ParallelRow->addWidget(new QLabel(tr("bytes"), QueueGroup));
  ParallelRow->addStretch(1);
  QueueCheck = AddCheck(QueueGroup, "QueueCheck", tr("Transfer in &background by default"));
  QueueNoConfirmationCheck = AddCheck(QueueGroup, "QueueNoConfirmationCheck", tr("&No confirmations for background transfers"));
  EnableQueueByDefaultCheck = AddCheck(QueueGroup, "EnableQueueByDefaultCheck", tr("&Enable queue processing by default"));
  QueueAutoPopupCheck = AddCheck(QueueGroup, "QueueAutoPopupCheck", tr("&Automatically popup prompts of background transfers when idle"));
  QHBoxLayout * KeepDoneRow = AddRow(QueueGroup);
  QLabel * QueueKeepDoneItemsCheck = new QLabel(tr("Display &completed transfers in queue for:"), QueueGroup);
  QueueKeepDoneItemsCheck->setObjectName(QStringLiteral("QueueKeepDoneItemsCheck"));
  KeepDoneRow->addWidget(QueueKeepDoneItemsCheck);
  QueueKeepDoneItemsForCombo =
    CreateCombo(QueueGroup, "QueueKeepDoneItemsForCombo",
      { tr("Never"), tr("15 seconds"), tr("1 minute"), tr("15 minutes"), tr("1 hour"), tr("Forever") });
  QueueKeepDoneItemsCheck->setBuddy(QueueKeepDoneItemsForCombo);
  KeepDoneRow->addWidget(QueueKeepDoneItemsForCombo);
  KeepDoneRow->addStretch(1);
  QGroupBox * QueueViewGroup = AddGroup(QueueSheet, "QueueViewGroup", tr("Queue list"));
  QueueViewShowButton = AddRadio(QueueViewGroup, "QueueViewShowButton", tr("&Show"));
  QueueViewHideWhenEmptyButton = AddRadio(QueueViewGroup, "QueueViewHideWhenEmptyButton", tr("Hide &when empty"));
  QueueViewHideButton = AddRadio(QueueViewGroup, "QueueViewHideButton", tr("&Hide"));
  static_cast<QVBoxLayout *>(QueueSheet->layout())->addStretch(1);

  QWidget * TransferEnduranceSheet = AddSheet("TransferEnduranceSheet", tr("Endurance"), ppEndurance, NodeOf(ppTransfer));
  QGroupBox * ResumeBox = AddGroup(TransferEnduranceSheet, "ResumeBox", tr("Enable transfer resume/transfer to temporary filename for"));
  ResumeOnButton = AddRadio(ResumeBox, "ResumeOnButton", tr("A&ll files"));
  QHBoxLayout * ResumeSmartRow = AddRow(ResumeBox);
  ResumeSmartButton = new QRadioButton(tr("Files abo&ve:"), ResumeBox);
  ResumeSmartButton->setObjectName(QStringLiteral("ResumeSmartButton"));
  ResumeSmartRow->addWidget(ResumeSmartButton);
  ResumeThresholdEdit = CreateSpin(ResumeBox, "ResumeThresholdEdit", 0, 4194304, 10);
  ResumeThresholdEdit->setSuffix(tr(" KB"));
  ResumeSmartRow->addWidget(ResumeThresholdEdit);
  ResumeSmartRow->addStretch(1);
  ResumeOffButton = AddRadio(ResumeBox, "ResumeOffButton", tr("Di&sable"));

  QGroupBox * SessionReopenGroup = AddGroup(TransferEnduranceSheet, "SessionReopenGroup", tr("Automatic reconnect"));
  QGridLayout * ReopenGrid = new QGridLayout();
  static_cast<QBoxLayout *>(SessionReopenGroup->layout())->addLayout(ReopenGrid);
  int Row = 0;
  auto AddReopen = [&](QCheckBox *& Check, const char * CheckName, const QString & Caption, QSpinBox *& Edit, const char * EditName,
    const QString & LabelCaption)
  {
    Check = new QCheckBox(Caption, SessionReopenGroup);
    Check->setObjectName(QString::fromLatin1(CheckName));
    ReopenGrid->addWidget(Check, Row, 0, 1, 3);
    Row++;
    if (Edit == nullptr)
    {
      QLabel * Label = new QLabel(LabelCaption, SessionReopenGroup);
      Edit = CreateSpin(SessionReopenGroup, EditName, 1, 300, 5);
      Edit->setSuffix(tr(" seconds"));
      Label->setBuddy(Edit);
      ReopenGrid->addWidget(Label, Row, 1);
      ReopenGrid->addWidget(Edit, Row, 2);
      Row++;
    }
  };
  SessionReopenAutoEdit = nullptr;
  AddReopen(SessionReopenAutoCheck, "SessionReopenAutoCheck", tr("&Automatically reconnect session, if it breaks during transfer"),
    SessionReopenAutoEdit, "SessionReopenAutoEdit", tr("&Reconnect after:"));
  SessionReopenAutoIdleEdit = nullptr;
  AddReopen(SessionReopenAutoIdleCheck, "SessionReopenAutoIdleCheck", tr("Automatically reconnect session, if it breaks &while idle"),
    SessionReopenAutoIdleEdit, "SessionReopenAutoIdleEdit", tr("Reco&nnect after:"));
  QSpinBox * NoEdit = SessionReopenAutoIdleEdit;
  AddReopen(SessionReopenAutoInactiveCheck, "SessionReopenAutoInactiveCheck",
    tr("Automatically reconnect session, if it breaks while &inactive"), NoEdit, "", QString());
  SessionReopenAutoStallEdit = nullptr;
  AddReopen(SessionReopenAutoStallCheck, "SessionReopenAutoStallCheck", tr("Automatically reconnect session, if it &stalls"),
    SessionReopenAutoStallEdit, "SessionReopenAutoStallEdit", tr("Re&connect after:"));
  QLabel * SessionReopenTimeoutLabel = new QLabel(tr("&Keep reconnecting for:"), SessionReopenGroup);
  SessionReopenTimeoutLabel->setObjectName(QStringLiteral("SessionReopenTimeoutLabel"));
  SessionReopenTimeoutEdit = CreateSpin(SessionReopenGroup, "SessionReopenTimeoutEdit", 0, 86400, 30);
  SessionReopenTimeoutEdit->setSuffix(tr(" seconds"));
  SessionReopenTimeoutEdit->setSpecialValueText(ToQString(LoadStr(PREFERENCES_RECONNECT_TIMEOUT_UNLIMITED)));
  SessionReopenTimeoutLabel->setBuddy(SessionReopenTimeoutEdit);
  ReopenGrid->addWidget(SessionReopenTimeoutLabel, Row, 0, 1, 2);
  ReopenGrid->addWidget(SessionReopenTimeoutEdit, Row, 2);
  ReopenGrid->setColumnMinimumWidth(0, fontMetrics().averageCharWidth() * 4);
  ReopenGrid->setColumnStretch(3, 1);
  static_cast<QVBoxLayout *>(TransferEnduranceSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateNetworkSheet()
{
  QWidget * NetworkSheet = AddSheet("NetworkSheet", tr("Network"), ppNetwork, nullptr);
  QGroupBox * ExternalIpAddressGroupBox2 =
    AddGroup(NetworkSheet, "ExternalIpAddressGroupBox2", tr("Incoming FTP connections (active mode)"));
  RetrieveExternalIpAddressButton =
    AddRadio(ExternalIpAddressGroupBox2, "RetrieveExternalIpAddressButton", tr("Retrieve the external IP address from &operating system"));
  CustomExternalIpAddressButton =
    AddRadio(ExternalIpAddressGroupBox2, "CustomExternalIpAddressButton", tr("Use the &following external IP address:"));
  CustomExternalIpAddressEdit = new QLineEdit(ExternalIpAddressGroupBox2);
  CustomExternalIpAddressEdit->setObjectName(QStringLiteral("CustomExternalIpAddressEdit"));
  QHBoxLayout * IpRow = AddRow(ExternalIpAddressGroupBox2);
  IpRow->addSpacing(fontMetrics().averageCharWidth() * 4);
  IpRow->addWidget(CustomExternalIpAddressEdit, 1);
  QHBoxLayout * PortsRow = AddRow(ExternalIpAddressGroupBox2);
  LocalPortNumberCheck = new QCheckBox(tr("Limit listening &ports to:"), ExternalIpAddressGroupBox2);
  LocalPortNumberCheck->setObjectName(QStringLiteral("LocalPortNumberCheck"));
  PortsRow->addWidget(LocalPortNumberCheck);
  LocalPortNumberMinEdit = CreateSpin(ExternalIpAddressGroupBox2, "LocalPortNumberMinEdit", 1024, 65535);
  PortsRow->addWidget(LocalPortNumberMinEdit);
  PortsRow->addWidget(new QLabel(QStringLiteral("\u2013"), ExternalIpAddressGroupBox2));
  LocalPortNumberMaxEdit = CreateSpin(ExternalIpAddressGroupBox2, "LocalPortNumberMaxEdit", 1024, 65535);
  PortsRow->addWidget(LocalPortNumberMaxEdit);
  PortsRow->addStretch(1);
  // The range stays valid
  connect(LocalPortNumberMinEdit, &QSpinBox::editingFinished, this, [this]()
  {
    if (LocalPortNumberMinEdit->value() > LocalPortNumberMaxEdit->value())
    {
      LocalPortNumberMaxEdit->setValue(LocalPortNumberMinEdit->value());
    }
  });
  connect(LocalPortNumberMaxEdit, &QSpinBox::editingFinished, this, [this]()
  {
    if (LocalPortNumberMinEdit->value() > LocalPortNumberMaxEdit->value())
    {
      LocalPortNumberMinEdit->setValue(LocalPortNumberMaxEdit->value());
    }
  });
  QGroupBox * ConnectionsGroup = AddGroup(NetworkSheet, "ConnectionsGroup", tr("Connections"));
  TryFtpWhenSshFailsCheck =
    AddCheck(ConnectionsGroup, "TryFtpWhenSshFailsCheck", tr("When SFTP connection is &rejected, knock FTP port"));
  static_cast<QVBoxLayout *>(NetworkSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateSecuritySheet()
{
  QWidget * SecuritySheet = AddSheet("SecuritySheet", tr("Security"), ppSecurity, nullptr);
  QGroupBox * MasterPasswordGroup = AddGroup(SecuritySheet, "MasterPasswordGroup", tr("Master password"));
  QHBoxLayout * MasterRow = AddRow(MasterPasswordGroup);
  UseMasterPasswordCheck = new QCheckBox(tr("&Use master password"), MasterPasswordGroup);
  UseMasterPasswordCheck->setObjectName(QStringLiteral("UseMasterPasswordCheck"));
  MasterRow->addWidget(UseMasterPasswordCheck);
  MasterRow->addStretch(1);
  SetMasterPasswordButton = CreateButton(MasterPasswordGroup, "SetMasterPasswordButton", tr("&Change master password..."));
  MasterRow->addWidget(SetMasterPasswordButton);
  // Not by the toggled signal: the dialogs it shows may revert it
  connect(UseMasterPasswordCheck, &QCheckBox::clicked, this, [this]() { ExecuteHandler([this]() { UseMasterPasswordCheckClick(); }); });
  connect(SetMasterPasswordButton, &QPushButton::clicked, this, [this]()
  {
    ExecuteHandler([this]()
    {
      if (CanSetMasterPassword())
      {
        ChangeMasterPassword(LoadStr(MASTER_PASSWORD_CHANGED));
      }
    });
  });

  QGroupBox * AuthenticationGroup = AddGroup(SecuritySheet, "AuthenticationGroup", tr("Authentication"));
  SessionRememberPasswordCheck =
    AddCheck(AuthenticationGroup, "SessionRememberPasswordCheck", tr("Remember &password for duration of session"));

  QGroupBox * SshHostCAsGroup = AddGroup(SecuritySheet, "SshHostCAsGroup", tr("Trusted host certification authorities"));
  QHBoxLayout * CAsRow = AddRow(SshHostCAsGroup);
  SshHostCAsView = CreateListView(SshHostCAsGroup, "SshHostCAsView", { tr("Name"), tr("Hosts") });
  SshHostCAsView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  AddSshHostCAButton = CreateButton(SshHostCAsGroup, "AddSshHostCAButton", tr("&Add..."));
  EditSshHostCAButton = CreateButton(SshHostCAsGroup, "EditSshHostCAButton", tr("&Edit..."));
  RemoveSshHostCAButton = CreateButton(SshHostCAsGroup, "RemoveSshHostCAButton", tr("&Remove"));
  AddListButtons(SshHostCAsGroup, CAsRow, SshHostCAsView, { AddSshHostCAButton, EditSshHostCAButton, RemoveSshHostCAButton });
  connect(AddSshHostCAButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditSshHostCA(false); }); });
  connect(EditSshHostCAButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditSshHostCA(true); }); });
  connect(RemoveSshHostCAButton, &QPushButton::clicked, this, [this]() { RemoveSshHostCA(); });
  connect(SshHostCAsView, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditSshHostCAButton->isEnabled())
    {
      ExecuteHandler([this]() { AddEditSshHostCA(true); });
    }
  });
  static_cast<QVBoxLayout *>(SecuritySheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateLogSheet()
{
  QWidget * LogSheet = AddSheet("LogSheet", tr("Logging"), ppLogging, nullptr);
  QGroupBox * LoggingGroup = AddGroup(LogSheet, "LoggingGroup", tr("Session log"));
  QHBoxLayout * LevelRow = AddRow(LoggingGroup);
  EnableLoggingCheck = new QCheckBox(tr("Enable &session logging on level:"), LoggingGroup);
  EnableLoggingCheck->setObjectName(QStringLiteral("EnableLoggingCheck"));
  LevelRow->addWidget(EnableLoggingCheck);
  LogProtocolCombo2 = CreateCombo(LoggingGroup, "LogProtocolCombo2", { tr("Reduced"), tr("Normal"), tr("Debug 1"), tr("Debug 2") });
  LevelRow->addWidget(LogProtocolCombo2);
  LevelRow->addStretch(1);
  QLabel * LogFileNameLabel = new QLabel(tr("&Log path:"), LoggingGroup);
  LogFileNameLabel->setObjectName(QStringLiteral("LogFileNameLabel"));
  LoggingGroup->layout()->addWidget(LogFileNameLabel);
  QHBoxLayout * LogFileRow = AddRow(LoggingGroup);
  LogFileNameEdit3 = CreatePathEdit(LoggingGroup, LogFileRow, "LogFileNameEdit3", false, tr("Select file for session log."));
  LogFileNameLabel->setBuddy(LogFileNameEdit3);
  QLabel * LogFileNameHintText = new QLabel(tr("<a href=\"#\">patterns</a>"), LoggingGroup);
  LogFileNameHintText->setObjectName(QStringLiteral("LogFileNameHintText"));
  LogFileNameHintText->setToolTip(ToQString(LoadStr(LOG_FILE_HINT3)).replace(QStringLiteral("C:\\"), QStringLiteral("~/")));
  LogFileRow->addWidget(LogFileNameHintText);
  QHBoxLayout * AppendRow = AddRow(LoggingGroup);
  QButtonGroup * AppendGroup = new QButtonGroup(LoggingGroup);
  LogFileAppendButton = new QRadioButton(tr("Appe&nd"), LoggingGroup);
  LogFileAppendButton->setObjectName(QStringLiteral("LogFileAppendButton"));
  LogFileOverwriteButton = new QRadioButton(tr("&Overwrite"), LoggingGroup);
  LogFileOverwriteButton->setObjectName(QStringLiteral("LogFileOverwriteButton"));
  AppendGroup->addButton(LogFileAppendButton);
  AppendGroup->addButton(LogFileOverwriteButton);
  AppendRow->addWidget(LogFileAppendButton);
  AppendRow->addWidget(LogFileOverwriteButton);
  AppendRow->addStretch(1);
  QHBoxLayout * MaxSizeRow = AddRow(LoggingGroup);
  LogMaxSizeCheck = new QCheckBox(tr("&Rotate log files after reaching"), LoggingGroup);
  LogMaxSizeCheck->setObjectName(QStringLiteral("LogMaxSizeCheck"));
  MaxSizeRow->addWidget(LogMaxSizeCheck);
  LogMaxSizeCombo =
    CreateCombo(LoggingGroup, "LogMaxSizeCombo", { QStringLiteral("1M"), QStringLiteral("10M"), QStringLiteral("100M"), QStringLiteral("1G") }, true);
  LogMaxSizeCombo->installEventFilter(this);
  MaxSizeRow->addWidget(LogMaxSizeCombo);
  MaxSizeRow->addStretch(1);
  QHBoxLayout * MaxCountRow = AddRow(LoggingGroup);
  LogMaxSizeCountCheck = new QCheckBox(tr("&Delete old log files, keep"), LoggingGroup);
  LogMaxSizeCountCheck->setObjectName(QStringLiteral("LogMaxSizeCountCheck"));
  MaxCountRow->addSpacing(fontMetrics().averageCharWidth() * 4);
  MaxCountRow->addWidget(LogMaxSizeCountCheck);
  LogMaxSizeCountEdit = CreateSpin(LoggingGroup, "LogMaxSizeCountEdit", 1, 999);
  LogMaxSizeCountEdit->setSuffix(tr(" files"));
  MaxCountRow->addWidget(LogMaxSizeCountEdit);
  MaxCountRow->addStretch(1);
  LogSensitiveCheck = AddCheck(LoggingGroup, "LogSensitiveCheck", tr("Log pass&words and other sensitive information"));
  LogProtocolHintLabel =
    new QLabel(tr("Debug logging levels may cause performance problems and should be used for troubleshooting only."), LogSheet);
  LogProtocolHintLabel->setObjectName(QStringLiteral("LogProtocolHintLabel"));
  LogProtocolHintLabel->setWordWrap(true);
  LogSheet->layout()->addWidget(LogProtocolHintLabel);

  QGroupBox * ActionsLoggingGroup = AddGroup(LogSheet, "ActionsLoggingGroup", tr("XML log"));
  EnableActionsLoggingCheck = AddCheck(ActionsLoggingGroup, "EnableActionsLoggingCheck", tr("Enable &XML logging to file:"));
  QHBoxLayout * ActionsLogRow = AddRow(ActionsLoggingGroup);
  ActionsLogFileNameEdit = CreatePathEdit(ActionsLoggingGroup, ActionsLogRow, "ActionsLogFileNameEdit", false, tr("Select file for XML log."));
  QLabel * ActionsLogFileNameHintText = new QLabel(tr("<a href=\"#\">patterns</a>"), ActionsLoggingGroup);
  ActionsLogFileNameHintText->setObjectName(QStringLiteral("ActionsLogFileNameHintText"));
  ActionsLogFileNameHintText->setToolTip(LogFileNameHintText->toolTip());
  ActionsLogRow->addWidget(ActionsLogFileNameHintText);
  static_cast<QVBoxLayout *>(LogSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateIntegrationSheets()
{
  QWidget * IntegrationSheet = AddSheet("IntegrationSheet", tr("Integration"), ppIntegration, nullptr);
  QGroupBox * ShellIconsGroup = AddGroup(IntegrationSheet, "ShellIconsGroup", tr("Desktop"));
  QPushButton * DesktopIconButton = CreateButton(ShellIconsGroup, "DesktopIconButton", tr("Create a &desktop icon"));
  ShellIconsGroup->layout()->addWidget(DesktopIconButton);
  connect(DesktopIconButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { DesktopIconButtonClick(); }); });
  QPushButton * RegisterAsUrlHandlersButton =
    CreateButton(ShellIconsGroup, "RegisterAsUrlHandlersButton", tr("Make OpenSCP the default handler of &URL addresses"));
  ShellIconsGroup->layout()->addWidget(RegisterAsUrlHandlersButton);
  connect(RegisterAsUrlHandlersButton, &QPushButton::clicked, this,
    [this]() { ExecuteHandler([this]() { RegisterAsUrlHandlersButtonClick(); }); });
  for (QWidget * Button : { static_cast<QWidget *>(DesktopIconButton), static_cast<QWidget *>(RegisterAsUrlHandlersButton) })
  {
    static_cast<QVBoxLayout *>(ShellIconsGroup->layout())->setAlignment(Button, Qt::AlignLeft);
  }
  static_cast<QVBoxLayout *>(IntegrationSheet->layout())->addStretch(1);

  QWidget * IntegrationAppSheet = AddSheet("IntegrationAppSheet", tr("Applications"), ppApplications, NodeOf(ppIntegration));
  QGroupBox * ExternalAppsGroup = AddGroup(IntegrationAppSheet, "ExternalAppsGroup", tr("External applications"));
  QLabel * PuttyPathLabel = new QLabel(tr("SSH &client path:"), ExternalAppsGroup);
  PuttyPathLabel->setObjectName(QStringLiteral("PuttyPathLabel"));
  ExternalAppsGroup->layout()->addWidget(PuttyPathLabel);
  QHBoxLayout * PuttyRow = AddRow(ExternalAppsGroup);
  PuttyPathEdit = new THistoryComboBox(ExternalAppsGroup);
  PuttyPathEdit->setObjectName(QStringLiteral("PuttyPathEdit"));
  PuttyPathEdit->SetHistoryKey(L"PuttyPath");
  PuttyPathLabel->setBuddy(PuttyPathEdit);
  PuttyRow->addWidget(PuttyPathEdit, 1);
  QPushButton * PuttyPathBrowseButton = CreateButton(ExternalAppsGroup, "PuttyPathBrowseButton", tr("B&rowse..."));
  PuttyRow->addWidget(PuttyPathBrowseButton);
  connect(PuttyPathBrowseButton, &QPushButton::clicked, this, [this]()
  {
    ExecuteHandler([this]() { BrowseForExecutable(PuttyPathEdit, LoadStr(PREFERENCES_SELECT_PUTTY2), false); });
  });
  QLabel * PuttyPathHintText = new QLabel(tr("<a href=\"#\">patterns</a>"), ExternalAppsGroup);
  PuttyPathHintText->setObjectName(QStringLiteral("PuttyPathHintText"));
  std::unique_ptr<TStrings> PuttyPathHintStrings(TextToStringList(LoadStr(PUTTY_PATTERNS_HINT2)));
  PuttyPathHintStrings->Insert(7, LoadStr(PATTERNS_HINT_K));
  PuttyPathHintText->setToolTip(ToQString(TrimRight(StringsToText(PuttyPathHintStrings.get()))));
  PuttyRow->addWidget(PuttyPathHintText);
  PuttyPasswordCheck2 =
    AddCheck(ExternalAppsGroup, "PuttyPasswordCheck2", tr("&Remember session password and pass it to the client (SSH)"));
  AutoOpenInPuttyCheck = AddCheck(ExternalAppsGroup, "AutoOpenInPuttyCheck", tr("Automatically &open new sessions in the client"));
  TelnetForFtpInPuttyCheck = AddCheck(ExternalAppsGroup, "TelnetForFtpInPuttyCheck", tr("Open &Telnet sessions in PuTTY for FTP sessions"));
  static_cast<QVBoxLayout *>(IntegrationAppSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateCommandsSheet()
{
  QWidget * CustomCommandsSheet = AddSheet("CustomCommandsSheet", tr("Commands"), ppCommands, nullptr);
  QGroupBox * CustomCommandsGroup = AddGroup(CustomCommandsSheet, "CustomCommandsGroup", tr("Custom commands"));
  QHBoxLayout * CommandsRow = AddRow(CustomCommandsGroup);
  CustomCommandsView = CreateListView(CustomCommandsGroup, "CustomCommandsView", { tr("Description"), tr("Command"), tr("L/R") });
  CustomCommandsView->header()->setStretchLastSection(false);
  CustomCommandsView->header()->setSectionResizeMode(1, QHeaderView::Stretch);
  CustomCommandsView->setMinimumHeight(fontMetrics().height() * 15);
  AddCommandButton = CreateButton(CustomCommandsGroup, "AddCommandButton", tr("&Add..."));
  EditCommandButton = CreateButton(CustomCommandsGroup, "EditCommandButton", tr("&Edit..."));
  RemoveCommandButton = CreateButton(CustomCommandsGroup, "RemoveCommandButton", tr("&Remove"));
  UpCommandButton = CreateButton(CustomCommandsGroup, "UpCommandButton", tr("&Up"));
  DownCommandButton = CreateButton(CustomCommandsGroup, "DownCommandButton", tr("&Down"));
  AddListButtons(CustomCommandsGroup, CommandsRow, CustomCommandsView,
    { AddCommandButton, EditCommandButton, RemoveCommandButton, UpCommandButton, DownCommandButton });
  connect(AddCommandButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditCommand(false); }); });
  connect(EditCommandButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { AddEditCommand(true); }); });
  connect(RemoveCommandButton, &QPushButton::clicked, this, [this]() { RemoveCommand(); });
  connect(UpCommandButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(CustomCommandsView);
    CustomCommandMove(Index, Index - 1);
  });
  connect(DownCommandButton, &QPushButton::clicked, this, [this]()
  {
    int Index = ListIndex(CustomCommandsView);
    CustomCommandMove(Index, Index + 1);
  });
  connect(CustomCommandsView, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditCommandButton->isEnabled())
    {
      ExecuteHandler([this]() { AddEditCommand(true); });
    }
  });
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CreateStorageSheet()
{
  QWidget * StorageSheet = AddSheet("StorageSheet", tr("Storage"), ppStorage, nullptr);
  QGroupBox * StorageGroup = AddGroup(StorageSheet, "StorageGroup", tr("Configuration storage"));
  // OpenSCP: an INI file only
  QLabel * AutomaticIniFileStorageLabel = new QLabel(ToQString(Configuration->IniFileStorageName), StorageGroup);
  AutomaticIniFileStorageLabel->setObjectName(QStringLiteral("AutomaticIniFileStorageLabel"));
  AutomaticIniFileStorageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
  StorageGroup->layout()->addWidget(AutomaticIniFileStorageLabel);

  QGroupBox * TemporaryDirectoryGrouo = AddGroup(StorageSheet, "TemporaryDirectoryGrouo", tr("Temporary directory"));
  QLabel * Label6 = new QLabel(tr("Specify where to temporarily store edited and downloaded files."), TemporaryDirectoryGrouo);
  TemporaryDirectoryGrouo->layout()->addWidget(Label6);
  DDSystemTemporaryDirectoryButton =
    AddRadio(TemporaryDirectoryGrouo, "DDSystemTemporaryDirectoryButton", tr("&Use temporary directory of system"));
  QHBoxLayout * CustomRow = AddRow(TemporaryDirectoryGrouo);
  DDCustomTemporaryDirectoryButton = new QRadioButton(tr("Use this &directory:"), TemporaryDirectoryGrouo);
  DDCustomTemporaryDirectoryButton->setObjectName(QStringLiteral("DDCustomTemporaryDirectoryButton"));
  CustomRow->addWidget(DDCustomTemporaryDirectoryButton);
  DDTemporaryDirectoryEdit =
    CreatePathEdit(TemporaryDirectoryGrouo, CustomRow, "DDTemporaryDirectoryEdit", true, tr("Select directory for temporary files."));
  TemporaryDirectoryAppendSessionCheck =
    AddCheck(TemporaryDirectoryGrouo, "TemporaryDirectoryAppendSessionCheck", tr("Append &session name to temporary path"));
  TemporaryDirectoryAppendPathCheck =
    AddCheck(TemporaryDirectoryGrouo, "TemporaryDirectoryAppendPathCheck", tr("Append remote &path to temporary path"));
  TemporaryDirectoryDeterministicCheck =
    AddCheck(TemporaryDirectoryGrouo, "TemporaryDirectoryDeterministicCheck", tr("Keep temporary copies of remote files in &deterministic paths"));
  TemporaryDirectoryCleanupCheck =
    AddCheck(TemporaryDirectoryGrouo, "TemporaryDirectoryCleanupCheck", tr("&Cleanup obsolete temporary directories on startup"));
  QHBoxLayout * ConfirmCleanupRow = AddRow(TemporaryDirectoryGrouo);
  ConfirmTemporaryDirectoryCleanupCheck = new QCheckBox(tr("&Ask before cleanup"), TemporaryDirectoryGrouo);
  ConfirmTemporaryDirectoryCleanupCheck->setObjectName(QStringLiteral("ConfirmTemporaryDirectoryCleanupCheck"));
  ConfirmCleanupRow->addSpacing(fontMetrics().averageCharWidth() * 4);
  ConfirmCleanupRow->addWidget(ConfirmTemporaryDirectoryCleanupCheck);
  ConfirmCleanupRow->addStretch(1);

  QGroupBox * OtherStorageGroup = AddGroup(StorageSheet, "OtherStorageGroup", tr("Other"));
  QLabel * RandomSeedFileLabel = new QLabel(tr("&Random seed file:"), OtherStorageGroup);
  RandomSeedFileLabel->setObjectName(QStringLiteral("RandomSeedFileLabel"));
  OtherStorageGroup->layout()->addWidget(RandomSeedFileLabel);
  QHBoxLayout * SeedRow = AddRow(OtherStorageGroup);
  RandomSeedFileEdit = CreatePathEdit(OtherStorageGroup, SeedRow, "RandomSeedFileEdit", false, tr("Select file for random seed"));
  RandomSeedFileLabel->setBuddy(RandomSeedFileEdit);
  static_cast<QVBoxLayout *>(StorageSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
bool TPreferencesDialog::Execute(TPreferencesDialogData * DialogData)
{
  FDialogData = DialogData;
  LoadConfiguration();

  int InitialPage;
  switch (FPreferencesMode)
  {
    case pmEditor: InitialPage = ppEditor; break;
    case pmCustomCommands: InitialPage = ppCommands; break;
    case pmQueue: InitialPage = ppBackground; break;
    case pmLogging: InitialPage = ppLogging; break;
    case pmPresets: InitialPage = ppTransfer; break;
    case pmEditors: InitialPage = ppEditor; break;
    case pmEditorInternal: InitialPage = ppEditorInternal; break;
    case pmCommander: InitialPage = ppCommander; break;
    case pmFileColors: InitialPage = ppFileColors; break;
    default: InitialPage = ppEnvironment; break;
  }
  SetActivePage(InitialPage);
  NavigationTree->setFocus();

  bool Result = (exec() == QDialog::Accepted);
  if (Result)
  {
    SaveConfiguration();
    PuttyPathEdit->SaveToHistory();
  }
  return Result;
}
//---------------------------------------------------------------------------
void TPreferencesDialog::LoadConfiguration()
{
  TAutoNestingCounter NoUpdateCounter(FNoUpdate);

  #define BOOLPROP(PROP) PROP ## Check->setChecked(AppConfiguration->PROP)
  BOOLPROP(SwappedPanels);
  BOOLPROP(UseLocationProfiles);
  BOOLPROP(CompareByTime);
  BOOLPROP(CompareBySize);
  // "Explorer-style selection"
  switch (AppConfiguration->NortonLikeMode)
  {
    case nlOff:
      NortonLikeModeCombo->setCurrentIndex(2);
      break;

    case nlKeyboard:
      NortonLikeModeCombo->setCurrentIndex(1);
      break;

    default:
      NortonLikeModeCombo->setCurrentIndex(0);
      break;
  }
  ExplorerKeyboardShortcutsCombo->setCurrentIndex(AppConfiguration->ExplorerKeyboardShortcuts ? 1 : 0);
  TFileColorData::LoadList(AppConfiguration->FileColors, FFileColors);
  UpdateFileColorsView();
  BOOLPROP(DefaultDirIsHome);
  BOOLPROP(PreservePanelState);
  BOOLPROP(DeleteToRecycleBin);
  BOOLPROP(DDWarnLackOfTempSpace);
  BOOLPROP(ShowHiddenFiles);
  BOOLPROP(RenameWholeName);
  BOOLPROP(ShowInaccesibleDirectories);
  BOOLPROP(CopyOnDoubleClickConfirmation);
  BOOLPROP(ConfirmTransferring);
  BOOLPROP(ConfirmDeleting);
  BOOLPROP(ConfirmRecycling);
  BOOLPROP(ConfirmExitOnCompletion);
  BOOLPROP(SynchronizeSummary);
  BOOLPROP(TemporaryDirectoryAppendSession);
  BOOLPROP(TemporaryDirectoryAppendPath);
  BOOLPROP(TemporaryDirectoryDeterministic);
  BOOLPROP(TemporaryDirectoryCleanup);
  BOOLPROP(ConfirmTemporaryDirectoryCleanup);
  BOOLPROP(NaturalOrderNumericalSorting);
  BOOLPROP(AlwaysSortDirectoriesByName);
  ConfirmOverwritingCheck->setChecked(Configuration->ConfirmOverwriting);
  ConfirmResumeCheck->setChecked(Configuration->ConfirmResume);
  ConfirmCommandSessionCheck->setChecked(GUIConfiguration->ConfirmCommandSession);
  ContinueOnErrorCheck->setChecked(GUIConfiguration->ContinueOnError);
  BeepOnFinishCheck->setChecked(GUIConfiguration->BeepOnFinish);

  ConfirmClosingSessionCheck2->setChecked(AppConfiguration->ConfirmClosingSession);

  // allow grayed state only initially,
  // once the off state is confirmed, never allow returning
  // to the undefined state
  DDTransferConfirmationCheck2->setTristate(AppConfiguration->DDTransferConfirmation == asAuto);
  switch (AppConfiguration->DDTransferConfirmation)
  {
    case asOn: DDTransferConfirmationCheck2->setCheckState(Qt::Checked); break;
    case asOff: DDTransferConfirmationCheck2->setCheckState(Qt::Unchecked); break;
    default: DDTransferConfirmationCheck2->setCheckState(Qt::PartiallyChecked); break;
  }

  BeepOnFinishAfterEdit->setValue(int(static_cast<double>(GUIConfiguration->BeepOnFinishAfter) * SecsPerDay));
  BOOLPROP(BalloonNotifications);
  BOOLPROP(FlashTaskbar);

  DDFakeFileEnabledButton->setChecked(AppConfiguration->DDFakeFile);
  DDFakeFileDisabledButton->setChecked(!DDFakeFileEnabledButton->isChecked());

  if (AppConfiguration->DDTemporaryDirectory.IsEmpty())
  {
    DDSystemTemporaryDirectoryButton->setChecked(true);
    DDTemporaryDirectoryEdit->setText(ToQString(SystemTemporaryDirectory()));
  }
  else
  {
    DDCustomTemporaryDirectoryButton->setChecked(true);
    DDTemporaryDirectoryEdit->setText(ToQString(AppConfiguration->DDTemporaryDirectory));
  }

  // Local panel
  BOOLPROP(PreserveLocalDirectory);

  RandomSeedFileEdit->setText(ToQString(Configuration->RandomSeedFile));

  // editor
  TEditorConfiguration Editor = AppConfiguration->Editor;
  EditorWordWrapCheck->setChecked(Editor.WordWrap);
  EditorTabSizeEdit->setValue(Editor.TabSize);
  EditorEncodingCombo->setCurrentIndex((Editor.Encoding == CP_UTF8) ? 1 : 0);
  FEditorFont = EditorFont(Editor);
  FEditorFontColor = Editor.FontColor;
  FEditorBackgroundColor = Editor.BackgroundColor;
  EditorWarnLargeFileCheck->setChecked(Editor.WarnOrLargeFileSize);
  EditorLargeFileSizeEdit->setValue(Editor.LargeFileSize);
  (*FEditorList) = *AppConfiguration->EditorList;
  UpdateEditorListView();
  BOOLPROP(EditorCheckNotModified);

  FCopyParams = GUIConfiguration->DefaultCopyParam;
  ResumeOnButton->setChecked(GUIConfiguration->DefaultCopyParam.ResumeSupport == rsOn);
  ResumeSmartButton->setChecked(GUIConfiguration->DefaultCopyParam.ResumeSupport == rsSmart);
  ResumeOffButton->setChecked(GUIConfiguration->DefaultCopyParam.ResumeSupport == rsOff);
  ResumeThresholdEdit->setValue(static_cast<int>(GUIConfiguration->DefaultCopyParam.ResumeThreshold / 1024));
  SessionReopenAutoCheck->setChecked(Configuration->SessionReopenAuto > 0);
  SessionReopenAutoEdit->setValue(Configuration->SessionReopenAuto > 0 ?
    (Configuration->SessionReopenAuto / MSecsPerSec) : 5);
  SessionReopenAutoIdleCheck->setChecked(
    GUIConfiguration->SessionReopenAutoIdleOn && (GUIConfiguration->SessionReopenAutoIdle > 0));
  SessionReopenAutoInactiveCheck->setChecked(GUIConfiguration->SessionReopenAutoInactive);
  int SessionReopenAutoIdle =
    (GUIConfiguration->SessionReopenAutoIdle > 0 ? GUIConfiguration->SessionReopenAutoIdle : SessionReopenAutoIdleDefault);
  SessionReopenAutoIdleEdit->setValue(SessionReopenAutoIdle / MSecsPerSec);
  SessionReopenAutoStallCheck->setChecked(Configuration->SessionReopenAutoStall > 0);
  SessionReopenAutoStallEdit->setValue(Configuration->SessionReopenAutoStall > 0 ?
    (Configuration->SessionReopenAutoStall / MSecsPerSec) : SecsPerMin);
  SessionReopenTimeoutEdit->setValue(Configuration->SessionReopenTimeout / MSecsPerSec);

  FCustomCommandList->Assign(AppConfiguration->CustomCommandList);
  UpdateCustomCommandsView();

  PuttyPathEdit->SetText(GUIConfiguration->PuttyPath);
  PuttyPasswordCheck2->setChecked(GUIConfiguration->PuttyPassword);
  BOOLPROP(AutoOpenInPutty);
  TelnetForFtpInPuttyCheck->setChecked(GUIConfiguration->TelnetForFtpInPutty);

  // Queue
  QueueTransferLimitEdit->setValue(Configuration->QueueTransfersLimit);
  BOOLPROP(EnableQueueByDefault);
  QueueAutoPopupCheck->setChecked(GUIConfiguration->QueueAutoPopup);
  QueueCheck->setChecked(GUIConfiguration->DefaultCopyParam.Queue);
  QueueParallelCheck->setChecked(GUIConfiguration->DefaultCopyParam.QueueParallel);
  ParallelTransferCheck->setChecked(Configuration->ParallelTransferThreshold > 0);
  __int64 ParallelTransferThreshold = static_cast<__int64>(Configuration->ParallelTransferThreshold) * 1024;
  ParallelTransferThresholdCombo->setEditText(
    ToQString(SizeToStr((ParallelTransferThreshold > 0) ? ParallelTransferThreshold : 100*1024*1024)));
  QueueNoConfirmationCheck->setChecked(GUIConfiguration->DefaultCopyParam.QueueNoConfirmation);
  if (!GUIConfiguration->QueueKeepDoneItems)
  {
    QueueKeepDoneItemsForCombo->setCurrentIndex(0);
  }
  else if (GUIConfiguration->QueueKeepDoneItemsFor < 0)
  {
    QueueKeepDoneItemsForCombo->setCurrentIndex(5);
  }
  else if (GUIConfiguration->QueueKeepDoneItemsFor <= 15)
  {
    QueueKeepDoneItemsForCombo->setCurrentIndex(1);
  }
  else if (GUIConfiguration->QueueKeepDoneItemsFor <= 60)
  {
    QueueKeepDoneItemsForCombo->setCurrentIndex(2);
  }
  else if (GUIConfiguration->QueueKeepDoneItemsFor <= 15 * 60)
  {
    QueueKeepDoneItemsForCombo->setCurrentIndex(3);
  }
  else if (GUIConfiguration->QueueKeepDoneItemsFor <= 60 * 60)
  {
    QueueKeepDoneItemsForCombo->setCurrentIndex(4);
  }
  if (AppConfiguration->QueueViewShow == qvShow)
  {
    QueueViewShowButton->setChecked(true);
  }
  else if (AppConfiguration->QueueViewShow == qvHideWhenEmpty)
  {
    QueueViewHideWhenEmptyButton->setChecked(true);
  }
  else
  {
    QueueViewHideButton->setChecked(true);
  }

  // window
  BOOLPROP(AutoSaveWorkspace);
  AutoWorkspaceCombo->setEditText(
    ToQString(
      DefaultStr(AppConfiguration->AutoWorkspace,
        // It will rarely happen that LastWorkspace is set, while AutoWorkspace not.
        // It can happen only when user saved workspace before opening the Preferences
        // dialog for the first time
        DefaultStr(AppConfiguration->LastWorkspace, LoadStr(NEW_WORKSPACE)))));
  BOOLPROP(AutoSaveWorkspacePasswords);
  BOOLPROP(WorkspaceConnectAll);
  if (AppConfiguration->PathInCaption == picFull)
  {
    PathInCaptionFullButton->setChecked(true);
  }
  else if (AppConfiguration->PathInCaption == picShort)
  {
    PathInCaptionShortButton->setChecked(true);
  }
  else
  {
    PathInCaptionNoneButton->setChecked(true);
  }
  BOOLPROP(MinimizeToTray);
  BOOLPROP(ExternalSessionInExistingInstance);
  BOOLPROP(ShowLoginWhenNoSession);
  BOOLPROP(KeepOpenWhenNoSession);
  SessionSilentDisconnectCheck->setChecked(GUIConfiguration->SessionSilentDisconnect);
  BOOLPROP(SessionTabCaptionTruncation);

  // panels
  DoubleClickActionCombo->setCurrentIndex(AppConfiguration->DoubleClickAction);
  AutoReadDirectoryAfterOpCheck->setChecked(Configuration->AutoReadDirectoryAfterOp);
  BOOLPROP(RefreshRemotePanel);
  RefreshRemotePanelIntervalEdit->setValue(
    int(static_cast<double>(AppConfiguration->RefreshRemotePanelInterval) * SecsPerDay));

  switch (AppConfiguration->FormatSizeBytes)
  {
    case fbNone:
      FormatSizeBytesCombo->setCurrentIndex(0);
      break;
    case fbKilobytes:
      FormatSizeBytesCombo->setCurrentIndex(1);
      break;
    case fbShort:
      FormatSizeBytesCombo->setCurrentIndex(2);
      break;
    default:
      DebugFail();
  }

  switch (AppConfiguration->PanelSearch)
  {
    case isOff:
      PanelSearchCombo->setCurrentIndex(-1);
      break;
    case isNameStartOnly:
      PanelSearchCombo->setCurrentIndex(0);
      break;
    case isName:
      PanelSearchCombo->setCurrentIndex(1);
      break;
    case isAll:
      PanelSearchCombo->setCurrentIndex(2);
      break;
    default:
      DebugFail();
  }

  bool CustomPanelFont = !AppConfiguration->PanelFont.FontName.IsEmpty();
  PanelFontCheck->setChecked(CustomPanelFont);
  // Default to the font of the desktop, when starting customization
  FPanelFont = CustomPanelFont ? PanelFont(AppConfiguration->PanelFont) : QApplication::font();

  // presets
  (*FCopyParamList) = *GUIConfiguration->CopyParamList;
  UpdateCopyParamListView();
  SetListIndex(CopyParamListView, 0);
  BOOLPROP(CopyParamAutoSelectNotice);

  // security
  BOOLPROP(UseMasterPassword);
  SessionRememberPasswordCheck->setChecked(GUIConfiguration->SessionRememberPassword);
  FSshHostCAPlainList = Configuration->SshHostCAList->GetList();
  UpdateSshHostCAsView();

  // network
  RetrieveExternalIpAddressButton->setChecked(Configuration->ExternalIpAddress.IsEmpty());
  CustomExternalIpAddressButton->setChecked(!RetrieveExternalIpAddressButton->isChecked());
  CustomExternalIpAddressEdit->setText(ToQString(Configuration->ExternalIpAddress));
  LocalPortNumberCheck->setChecked(Configuration->HasLocalPortNumberLimits());
  LocalPortNumberMinEdit->setValue(
    (Configuration->LocalPortNumberMin > 0 ? Configuration->LocalPortNumberMin : PrivatePortMin));
  LocalPortNumberMaxEdit->setValue(
    (Configuration->LocalPortNumberMax >= LocalPortNumberMinEdit->value() ?
      Configuration->LocalPortNumberMax : LocalPortNumberMaxEdit->maximum()));
  TryFtpWhenSshFailsCheck->setChecked(Configuration->TryFtpWhenSshFails);

  // logging
  EnableLoggingCheck->setChecked(Configuration->Logging);
  LogProtocolCombo2->setCurrentIndex(Configuration->LogProtocol + BelowNormalLogLevels);
  LogFileNameEdit3->setText(
    ToQString(!Configuration->LogFileName.IsEmpty() ? Configuration->LogFileName : Configuration->DefaultLogFileName));
  if (Configuration->LogFileAppend)
  {
    LogFileAppendButton->setChecked(true);
  }
  else
  {
    LogFileOverwriteButton->setChecked(true);
  }
  LogMaxSizeCheck->setChecked(Configuration->LogMaxSize > 0);
  LogMaxSizeCombo->setEditText(ToQString(SizeToStr((Configuration->LogMaxSize > 0) ? Configuration->LogMaxSize : (1 * 1024 * 1024))));
  LogMaxSizeCountCheck->setChecked(Configuration->LogMaxCount > 0);
  LogMaxSizeCountEdit->setValue((Configuration->LogMaxCount > 0) ? Configuration->LogMaxCount : 5);
  LogSensitiveCheck->setChecked(Configuration->LogSensitive);

  EnableActionsLoggingCheck->setChecked(Configuration->LogActions);
  ActionsLogFileNameEdit->setText(ToQString(Configuration->ActionsLogFileName));

  #undef BOOLPROP

  FNoUpdate--;
  UpdateControls();
  FNoUpdate++;
}
//---------------------------------------------------------------------------
void TPreferencesDialog::SaveConfiguration()
{
  Configuration->BeginUpdate();
  ::rtl::TFinally Finally([&]() { Configuration->EndUpdate(); });

  TGUICopyParamType CopyParam = GUIConfiguration->DefaultCopyParam;

  #define BOOLPROP(PROP) AppConfiguration->PROP = PROP ## Check->isChecked()
  BOOLPROP(SwappedPanels);
  BOOLPROP(UseLocationProfiles);
  BOOLPROP(CompareByTime);
  BOOLPROP(CompareBySize);
  switch (NortonLikeModeCombo->currentIndex())
  {
    case 2:
      AppConfiguration->NortonLikeMode = nlOff;
      break;

    case 1:
      AppConfiguration->NortonLikeMode = nlKeyboard;
      break;

    default:
      AppConfiguration->NortonLikeMode = nlOn;
      break;
  }
  AppConfiguration->ExplorerKeyboardShortcuts = (ExplorerKeyboardShortcutsCombo->currentIndex() != 0);
  AppConfiguration->FileColors = TFileColorData::SaveList(FFileColors);
  BOOLPROP(DefaultDirIsHome);
  BOOLPROP(PreservePanelState);
  BOOLPROP(DeleteToRecycleBin);
  BOOLPROP(DDWarnLackOfTempSpace);
  BOOLPROP(ShowHiddenFiles);
  BOOLPROP(RenameWholeName);
  BOOLPROP(ShowInaccesibleDirectories);
  BOOLPROP(CopyOnDoubleClickConfirmation);
  BOOLPROP(ConfirmTransferring);
  BOOLPROP(ConfirmDeleting);
  BOOLPROP(ConfirmRecycling);
  BOOLPROP(ConfirmExitOnCompletion);
  BOOLPROP(SynchronizeSummary);
  BOOLPROP(TemporaryDirectoryAppendSession);
  BOOLPROP(TemporaryDirectoryAppendPath);
  BOOLPROP(TemporaryDirectoryDeterministic);
  BOOLPROP(TemporaryDirectoryCleanup);
  BOOLPROP(ConfirmTemporaryDirectoryCleanup);
  BOOLPROP(NaturalOrderNumericalSorting);
  BOOLPROP(AlwaysSortDirectoriesByName);
  Configuration->ConfirmOverwriting = ConfirmOverwritingCheck->isChecked();
  Configuration->ConfirmResume = ConfirmResumeCheck->isChecked();
  GUIConfiguration->ConfirmCommandSession = ConfirmCommandSessionCheck->isChecked();
  GUIConfiguration->ContinueOnError = ContinueOnErrorCheck->isChecked();
  GUIConfiguration->BeepOnFinish = BeepOnFinishCheck->isChecked();

  AppConfiguration->ConfirmClosingSession = ConfirmClosingSessionCheck2->isChecked();

  switch (DDTransferConfirmationCheck2->checkState())
  {
    case Qt::Checked: AppConfiguration->DDTransferConfirmation = asOn; break;
    case Qt::Unchecked: AppConfiguration->DDTransferConfirmation = asOff; break;
    default: AppConfiguration->DDTransferConfirmation = asAuto; break;
  }

  GUIConfiguration->BeepOnFinishAfter =
    static_cast<double>(BeepOnFinishAfterEdit->value() / SecsPerDay);
  BOOLPROP(BalloonNotifications);
  BOOLPROP(FlashTaskbar);

  AppConfiguration->DDFakeFile = DDFakeFileEnabledButton->isChecked();

  if (DDSystemTemporaryDirectoryButton->isChecked())
  {
    AppConfiguration->DDTemporaryDirectory = L"";
  }
  else
  {
    AppConfiguration->DDTemporaryDirectory = ToUnicodeString(DDTemporaryDirectoryEdit->text());
  }

  // Local panel
  BOOLPROP(PreserveLocalDirectory);

  Configuration->RandomSeedFile = ToUnicodeString(RandomSeedFileEdit->text());

  // editor
  TEditorConfiguration Editor = AppConfiguration->Editor;
  Editor.WordWrap = EditorWordWrapCheck->isChecked();
  Editor.TabSize = EditorTabSizeEdit->value();
  Editor.Encoding = (EditorEncodingCombo->currentIndex() == 1) ? CP_UTF8 : CP_ACP;
  Editor.FontName = ToUnicodeString(FEditorFont.family());
  Editor.FontSize = FEditorFont.pointSize();
  Editor.FontColor = FEditorFontColor;
  Editor.BackgroundColor = FEditorBackgroundColor;
  Editor.WarnOrLargeFileSize = EditorWarnLargeFileCheck->isChecked();
  Editor.LargeFileSize = EditorLargeFileSizeEdit->value();
  AppConfiguration->Editor = Editor;
  AppConfiguration->EditorList = FEditorList.get();
  BOOLPROP(EditorCheckNotModified);

  // overwrites only TCopyParamType fields
  CopyParam = FCopyParams;
  if (ResumeOnButton->isChecked()) CopyParam.ResumeSupport = rsOn;
  if (ResumeSmartButton->isChecked()) CopyParam.ResumeSupport = rsSmart;
  if (ResumeOffButton->isChecked()) CopyParam.ResumeSupport = rsOff;
  CopyParam.ResumeThreshold = ResumeThresholdEdit->value() * 1024;

  Configuration->SessionReopenAuto =
    (SessionReopenAutoCheck->isChecked() ? (SessionReopenAutoEdit->value() * MSecsPerSec) : 0);
  GUIConfiguration->SessionReopenAutoIdleOn = SessionReopenAutoIdleCheck->isChecked();
  GUIConfiguration->SessionReopenAutoInactive = SessionReopenAutoInactiveCheck->isChecked();
  GUIConfiguration->SessionReopenAutoIdle = (SessionReopenAutoIdleEdit->value() * MSecsPerSec);
  Configuration->SessionReopenAutoStall =
    (SessionReopenAutoStallCheck->isChecked() ? (SessionReopenAutoStallEdit->value() * MSecsPerSec) : 0);
  Configuration->SessionReopenTimeout = (SessionReopenTimeoutEdit->value() * MSecsPerSec);

  AppConfiguration->CustomCommandList = FCustomCommandList.get();

  GUIConfiguration->PuttyPath = PuttyPathEdit->GetText();
  GUIConfiguration->PuttyPassword = PuttyPasswordCheck2->isChecked();
  BOOLPROP(AutoOpenInPutty);
  GUIConfiguration->TelnetForFtpInPutty = TelnetForFtpInPuttyCheck->isChecked();

  // Queue
  Configuration->QueueTransfersLimit = QueueTransferLimitEdit->value();
  BOOLPROP(EnableQueueByDefault);
  GUIConfiguration->QueueAutoPopup = QueueAutoPopupCheck->isChecked();
  CopyParam.Queue = QueueCheck->isChecked();
  CopyParam.QueueParallel = QueueParallelCheck->isChecked();
  __int64 ParallelTransferThreshold = 0; // shut up
  if (ParallelTransferCheck->isChecked() &&
      TryStrToSize(ToUnicodeString(ParallelTransferThresholdCombo->currentText()), ParallelTransferThreshold))
  {
    Configuration->ParallelTransferThreshold =
      static_cast<int>(std::min(ParallelTransferThreshold / 1024, static_cast<__int64>(std::numeric_limits<int>::max())));
  }
  else
  {
    // In future, we might set it to 0, if -1 will actually mean some default threshold
    Configuration->ParallelTransferThreshold = -1;
  }
  CopyParam.QueueNoConfirmation = QueueNoConfirmationCheck->isChecked();
  GUIConfiguration->QueueKeepDoneItems = (QueueKeepDoneItemsForCombo->currentIndex() != 0);
  switch (QueueKeepDoneItemsForCombo->currentIndex())
  {
    case 0:
      GUIConfiguration->QueueKeepDoneItemsFor = 0;
      break;
    case 1:
      GUIConfiguration->QueueKeepDoneItemsFor = 15;
      break;
    case 2:
      GUIConfiguration->QueueKeepDoneItemsFor = 60;
      break;
    case 3:
      GUIConfiguration->QueueKeepDoneItemsFor = 15 * 60;
      break;
    case 4:
      GUIConfiguration->QueueKeepDoneItemsFor = 60 * 60;
      break;
    default:
      GUIConfiguration->QueueKeepDoneItemsFor = -1;
      break;
  }
  if (QueueViewShowButton->isChecked())
  {
    AppConfiguration->QueueViewShow = qvShow;
  }
  else if (QueueViewHideWhenEmptyButton->isChecked())
  {
    AppConfiguration->QueueViewShow = qvHideWhenEmpty;
  }
  else
  {
    AppConfiguration->QueueViewShow = qvHide;
  }

  GUIConfiguration->DefaultCopyParam = CopyParam;

  // window
  UnicodeString AutoWorkspace = ToUnicodeString(AutoWorkspaceCombo->currentText());
  AppConfiguration->AutoSaveWorkspace =
    !AutoWorkspace.IsEmpty() &&
    AutoSaveWorkspaceCheck->isChecked();
  if (!AutoWorkspace.IsEmpty())
  {
    AppConfiguration->AutoWorkspace = AutoWorkspace;
  }
  BOOLPROP(AutoSaveWorkspacePasswords);
  BOOLPROP(WorkspaceConnectAll);
  if (PathInCaptionFullButton->isChecked())
  {
    AppConfiguration->PathInCaption = picFull;
  }
  else if (PathInCaptionShortButton->isChecked())
  {
    AppConfiguration->PathInCaption = picShort;
  }
  else
  {
    AppConfiguration->PathInCaption = picNone;
  }
  BOOLPROP(MinimizeToTray);
  BOOLPROP(ExternalSessionInExistingInstance);
  BOOLPROP(ShowLoginWhenNoSession);
  BOOLPROP(KeepOpenWhenNoSession);
  GUIConfiguration->SessionSilentDisconnect = SessionSilentDisconnectCheck->isChecked();
  BOOLPROP(SessionTabCaptionTruncation);

  // panels
  AppConfiguration->DoubleClickAction = static_cast<TDoubleClickAction>(DoubleClickActionCombo->currentIndex());
  Configuration->AutoReadDirectoryAfterOp = AutoReadDirectoryAfterOpCheck->isChecked();
  BOOLPROP(RefreshRemotePanel);
  AppConfiguration->RefreshRemotePanelInterval =
    static_cast<double>(RefreshRemotePanelIntervalEdit->value() / SecsPerDay);

  switch (FormatSizeBytesCombo->currentIndex())
  {
    case 0:
      AppConfiguration->FormatSizeBytes = fbNone;
      break;
    case 1:
      AppConfiguration->FormatSizeBytes = fbKilobytes;
      break;
    case 2:
      AppConfiguration->FormatSizeBytes = fbShort;
      break;
    default:
      DebugFail();
  }

  switch (PanelSearchCombo->currentIndex())
  {
    case -1:
      AppConfiguration->PanelSearch = isOff;
      break;
    case 0:
      AppConfiguration->PanelSearch = isNameStartOnly;
      break;
    case 1:
      AppConfiguration->PanelSearch = isName;
      break;
    case 2:
      AppConfiguration->PanelSearch = isAll;
      break;
    default:
      DebugFail();
  }

  TFontConfiguration PanelFontConfiguration;
  if (PanelFontCheck->isChecked())
  {
    PanelFontConfiguration.FontName = ToUnicodeString(FPanelFont.family());
    PanelFontConfiguration.FontSize = FPanelFont.pointSize();
    PanelFontConfiguration.FontStyle =
      FLAGMASK(FPanelFont.bold(), fsBold) | FLAGMASK(FPanelFont.italic(), fsItalic) |
      FLAGMASK(FPanelFont.underline(), fsUnderline) | FLAGMASK(FPanelFont.strikeOut(), fsStrikeOut);
  }
  AppConfiguration->PanelFont = PanelFontConfiguration;

  // presets
  GUIConfiguration->CopyParamList = FCopyParamList.get();
  BOOLPROP(CopyParamAutoSelectNotice);

  // network
  Configuration->ExternalIpAddress =
    (CustomExternalIpAddressButton->isChecked() ? ToUnicodeString(CustomExternalIpAddressEdit->text()) : UnicodeString());
  Configuration->LocalPortNumberMin = LocalPortNumberCheck->isChecked() ? LocalPortNumberMinEdit->value() : 0;
  Configuration->LocalPortNumberMax = LocalPortNumberCheck->isChecked() ? LocalPortNumberMaxEdit->value() : 0;
  Configuration->TryFtpWhenSshFails = TryFtpWhenSshFailsCheck->isChecked();

  // security
  GUIConfiguration->SessionRememberPassword = SessionRememberPasswordCheck->isChecked();
  std::unique_ptr<TSshHostCAList> SshHostCAList(new TSshHostCAList(FSshHostCAPlainList));
  Configuration->SshHostCAList = SshHostCAList.get();

  // logging
  Configuration->Logging = EnableLoggingCheck->isChecked() && !LogFileNameEdit3->text().isEmpty();
  Configuration->LogProtocol = LogProtocolCombo2->currentIndex() - BelowNormalLogLevels;
  Configuration->LogFileName = ToUnicodeString(LogFileNameEdit3->text());
  Configuration->LogFileAppend = LogFileAppendButton->isChecked();
  __int64 LogMaxSize = 0; // shut up
  if (LogMaxSizeCheck->isChecked() && TryStrToSize(ToUnicodeString(LogMaxSizeCombo->currentText()), LogMaxSize))
  {
    Configuration->LogMaxSize = LogMaxSize;
  }
  else
  {
    Configuration->LogMaxSize = 0;
  }
  Configuration->LogMaxCount = (LogMaxSizeCountCheck->isChecked() ? LogMaxSizeCountEdit->value() : 0);
  Configuration->LogSensitive = LogSensitiveCheck->isChecked();

  Configuration->LogActions = EnableActionsLoggingCheck->isChecked();
  Configuration->ActionsLogFileName = ToUnicodeString(ActionsLogFileNameEdit->text());
  #undef BOOLPROP
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UpdateControls()
{
  if (FNoUpdate > 0)
  {
    return;
  }

  DDTransferConfirmationCheck2->setEnabled(ConfirmTransferringCheck->isChecked());
  BeepOnFinishAfterEdit->setEnabled(BeepOnFinishCheck->isChecked());

  ResumeThresholdEdit->setEnabled(ResumeSmartButton->isChecked());
  SessionReopenAutoEdit->setEnabled(SessionReopenAutoCheck->isChecked());
  SessionReopenAutoIdleEdit->setEnabled(SessionReopenAutoIdleCheck->isChecked() || SessionReopenAutoInactiveCheck->isChecked());
  SessionReopenAutoStallEdit->setEnabled(SessionReopenAutoStallCheck->isChecked());
  SessionReopenTimeoutEdit->setEnabled(SessionReopenAutoEdit->isEnabled() || SessionReopenAutoStallCheck->isChecked());

  CopyOnDoubleClickConfirmationCheck->setEnabled(
    (DoubleClickActionCombo->currentIndex() == dcaCopy) && ConfirmTransferringCheck->isChecked());

  QFont ActualPanelFont = PanelFontCheck->isChecked() ? FPanelFont : QApplication::font();
  PanelFontLabel->setText(ToQString(FMTLOAD(EDITOR_FONT_FMT, (ToUnicodeString(ActualPanelFont.family()), ActualPanelFont.pointSize()))));
  PanelFontLabel->setFont(ActualPanelFont);
  PanelFontButton->setEnabled(PanelFontCheck->isChecked());

  RefreshRemotePanelIntervalEdit->setEnabled(RefreshRemotePanelCheck->isChecked());
  EditorLargeFileSizeEdit->setEnabled(EditorWarnLargeFileCheck->isChecked());

  // upstream's TabSample
  auto TabSample = [this](const QString & Values)
  {
    QString Result;
    for (int Index = 0; Index < Values.length(); Index++)
    {
      if (Index > 0)
      {
        Result += QLatin1Char(' ');
        if (EditorTabSizeEdit->value() > 2)
        {
          Result += QString(EditorTabSizeEdit->value() - 2, QLatin1Char(' '));
        }
      }
      Result += Values[Index];
    }
    return Result;
  };
  QString EditorFontLabelText =
    ToQString(FMTLOAD(EDITOR_FONT_FMT, (ToUnicodeString(FEditorFont.family()), FEditorFont.pointSize()))) + QStringLiteral("\n\n") +
    TabSample(QStringLiteral("ABCD")) + QLatin1Char('\n') + TabSample(QStringLiteral("1234"));
  EditorFontLabel->setText(EditorFontLabelText);
  EditorFontLabel->setFont(FEditorFont);
  EditorFontLabel->setWordWrap(EditorWordWrapCheck->isChecked());
  QPalette EditorPalette = EditorFontLabel->palette();
  QPalette DefaultPalette = QApplication::palette();
  EditorPalette.setColor(QPalette::Window,
    (FEditorBackgroundColor != TColor(0)) ? TColorToQColor(FEditorBackgroundColor) : DefaultPalette.color(QPalette::Base));
  EditorPalette.setColor(QPalette::WindowText,
    (FEditorFontColor != TColor(0)) ? TColorToQColor(FEditorFontColor) : DefaultPalette.color(QPalette::Text));
  EditorFontLabel->setPalette(EditorPalette);

  int FileColorIndex = ListIndex(FileColorsView);
  bool FileColorSelected = (FileColorIndex >= 0);
  EditFileColorButton->setEnabled(FileColorSelected);
  RemoveFileColorButton->setEnabled(FileColorSelected);
  UpFileColorButton->setEnabled(FileColorSelected && (FileColorIndex > 0));
  DownFileColorButton->setEnabled(FileColorSelected && (FileColorIndex < static_cast<int>(FFileColors.size()) - 1));

  int CommandIndex = ListIndex(CustomCommandsView);
  bool CommandSelected = (CommandIndex >= 0);
  EditCommandButton->setEnabled(CommandSelected);
  RemoveCommandButton->setEnabled(CommandSelected);
  UpCommandButton->setEnabled(CommandSelected && (CommandIndex > 0));
  DownCommandButton->setEnabled(CommandSelected && (CommandIndex < FCustomCommandList->Count - 1));

  int CopyParamIndex = ListIndex(CopyParamListView);
  bool CopyParamSelected = (CopyParamIndex >= 0);
  EditCopyParamButton->setEnabled(CopyParamSelected);
  DuplicateCopyParamButton->setEnabled(CopyParamSelected);
  RemoveCopyParamButton->setEnabled(CopyParamSelected && (CopyParamIndex >= 1));
  UpCopyParamButton->setEnabled(CopyParamSelected && (CopyParamIndex > 1));
  DownCopyParamButton->setEnabled(
    CopyParamSelected &&
    (CopyParamIndex >= 1) &&
    (CopyParamIndex < CopyParamListView->topLevelItemCount() - 1));
  CopyParamAutoSelectNoticeCheck->setEnabled(FCopyParamList->AnyRule);

  UnicodeString InfoStr;
  if (CopyParamSelected)
  {
    const TCopyParamType * SelectedCopyParam = GetCopyParam(CopyParamIndex);
    InfoStr = SelectedCopyParam->GetInfoStr(L"; ", 0);
    if (CopyParamIndex >= 1)
    {
      const TCopyParamRule * Rule = FCopyParamList->Rules[CopyParamIndex - 1];
      if (Rule != NULL)
      {
        InfoStr += L"\n" + FORMAT(ReplaceStr(LoadStr(COPY_PARAM_RULE), L"\n", L" "), (Rule->GetInfoStr(L"; ")));
      }
    }
  }
  CopyParamLabel->setText(ToQString(InfoStr));

  DDWarnLackOfTempSpaceCheck->setEnabled(DDFakeFileDisabledButton->isChecked());
  DDTemporaryDirectoryEdit->setEnabled(DDCustomTemporaryDirectoryButton->isChecked());
  ConfirmTemporaryDirectoryCleanupCheck->setEnabled(TemporaryDirectoryCleanupCheck->isChecked());

  // editors
  int EditorIndex = ListIndex(EditorListView3);
  bool EditorSelected = (EditorIndex >= 0);
  EditEditorButton->setEnabled(EditorSelected);
  RemoveEditorButton->setEnabled(EditorSelected);
  UpEditorButton->setEnabled(EditorSelected && (EditorIndex > 0));
  DownEditorButton->setEnabled(EditorSelected && (EditorIndex < EditorListView3->topLevelItemCount() - 1));

  bool IsSiteCommand = false;
  bool IsPasswordCommand = false;
  try
  {
    TRemoteCustomCommand RemoteCustomCommand;
    TInteractiveCustomCommand InteractiveCustomCommand(&RemoteCustomCommand);
    UnicodeString PuttyPath = ToUnicodeString(PuttyPathEdit->currentText());
    PuttyPath = InteractiveCustomCommand.Complete(PuttyPath, false);
    IsSiteCommand = RemoteCustomCommand.IsSiteCommand(PuttyPath);
    IsPasswordCommand = RemoteCustomCommand.IsPasswordCommand(PuttyPath);
  }
  catch (...)
  {
    // noop
  }
  bool AnyPuttyPath = !PuttyPathEdit->currentText().isEmpty();
  PuttyPasswordCheck2->setEnabled(AnyPuttyPath && !IsPasswordCommand);
  AutoOpenInPuttyCheck->setEnabled(AnyPuttyPath);
  TelnetForFtpInPuttyCheck->setEnabled(AnyPuttyPath && !IsSiteCommand);

  // security
  SetMasterPasswordButton->setEnabled(AppConfiguration->UseMasterPassword);
  int SshHostCAIndex = ListIndex(SshHostCAsView);
  EditSshHostCAButton->setEnabled(SshHostCAIndex >= 0);
  RemoveSshHostCAButton->setEnabled(SshHostCAIndex >= 0);

  // network
  CustomExternalIpAddressEdit->setEnabled(CustomExternalIpAddressButton->isChecked());
  LocalPortNumberMinEdit->setEnabled(LocalPortNumberCheck->isChecked());
  LocalPortNumberMaxEdit->setEnabled(LocalPortNumberCheck->isChecked());

  // window
  AutoWorkspaceCombo->setEnabled(AutoSaveWorkspaceCheck->isChecked());
  AutoWorkspaceLabel->setEnabled(AutoWorkspaceCombo->isEnabled());
  AutoSaveWorkspacePasswordsCheck->setEnabled(
    !Configuration->DisablePasswordStoring &&
    AutoWorkspaceCombo->isEnabled());

  // logging
  LogProtocolCombo2->setEnabled(EnableLoggingCheck->isChecked());
  LogFileNameEdit3->setEnabled(LogProtocolCombo2->isEnabled());
  LogFileAppendButton->setEnabled(LogFileNameEdit3->isEnabled());
  LogFileOverwriteButton->setEnabled(LogFileNameEdit3->isEnabled());
  LogMaxSizeCheck->setEnabled(LogFileNameEdit3->isEnabled());
  LogMaxSizeCombo->setEnabled(LogMaxSizeCheck->isEnabled() && LogMaxSizeCheck->isChecked());
  LogMaxSizeCountCheck->setEnabled(LogMaxSizeCombo->isEnabled());
  LogMaxSizeCountEdit->setEnabled(LogMaxSizeCountCheck->isEnabled() && LogMaxSizeCountCheck->isChecked());

  LogSensitiveCheck->setEnabled(LogProtocolCombo2->isEnabled());

  ActionsLogFileNameEdit->setEnabled(EnableActionsLoggingCheck->isChecked());

  LogProtocolHintLabel->setVisible(LogProtocolCombo2->currentIndex() - BelowNormalLogLevels >= 2);

  // background
  ParallelTransferCheck->setEnabled(QueueParallelCheck->isChecked());
  ParallelTransferThresholdCombo->setEnabled(ParallelTransferCheck->isEnabled() && ParallelTransferCheck->isChecked());

  // The browse buttons of the paths follow their edits
  for (QToolButton * Button : findChildren<QToolButton *>())
  {
    QLineEdit * Edit = findChild<QLineEdit *>(Button->objectName().chopped(static_cast<int>(strlen("Browse"))));
    if (Edit != nullptr)
    {
      Button->setEnabled(Edit->isEnabled());
    }
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::SizeComboExit(QComboBox * Combo)
{
  // upstream's SizeComboExit: a valid size, else the focus stays
  __int64 Size;
  if (Combo->isEnabled() && !TryStrToSize(ToUnicodeString(Combo->currentText()), Size))
  {
    Combo->setFocus();
    Combo->lineEdit()->selectAll();
    throw Exception(MainInstructions(FMTLOAD(SIZE_INVALID, (ToUnicodeString(Combo->currentText())))));
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::accept()
{
  try
  {
    SizeComboExit(ParallelTransferThresholdCombo);
    SizeComboExit(LogMaxSizeCombo);
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
bool TPreferencesDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if (Event->type() == QEvent::KeyPress)
  {
    int Key = static_cast<QKeyEvent *>(Event)->key();
    if (Object == CustomCommandsView)
    {
      if (RemoveCommandButton->isEnabled() && (Key == Qt::Key_Delete))
      {
        RemoveCommand();
        return true;
      }
      if (AddCommandButton->isEnabled() && (Key == Qt::Key_Insert))
      {
        ExecuteHandler([this]() { AddEditCommand(false); });
        return true;
      }
    }
    else if (Object == CopyParamListView)
    {
      if (RemoveCopyParamButton->isEnabled() && (Key == Qt::Key_Delete))
      {
        RemoveCopyParam();
        return true;
      }
      if (AddCopyParamButton->isEnabled() && (Key == Qt::Key_Insert))
      {
        ExecuteHandler([this]() { AddEditCopyParam(cpmAdd); });
        return true;
      }
    }
    else if (Object == EditorListView3)
    {
      if (RemoveEditorButton->isEnabled() && (Key == Qt::Key_Delete))
      {
        RemoveEditor();
        return true;
      }
      if (AddEditorButton->isEnabled() && (Key == Qt::Key_Insert))
      {
        ExecuteHandler([this]() { AddEditEditor(false); });
        return true;
      }
    }
    else if (Object == FileColorsView)
    {
      if (RemoveFileColorButton->isEnabled() && (Key == Qt::Key_Delete))
      {
        RemoveFileColor();
        return true;
      }
      if (AddFileColorButton->isEnabled() && (Key == Qt::Key_Insert))
      {
        ExecuteHandler([this]() { AddEditFileColor(false); });
        return true;
      }
    }
    else if (Object == SshHostCAsView)
    {
      if (RemoveSshHostCAButton->isEnabled() && (Key == Qt::Key_Delete))
      {
        RemoveSshHostCA();
        return true;
      }
      if (AddSshHostCAButton->isEnabled() && (Key == Qt::Key_Insert))
      {
        ExecuteHandler([this]() { AddEditSshHostCA(false); });
        return true;
      }
    }
  }
  else if ((Event->type() == QEvent::FocusOut) &&
           ((Object == ParallelTransferThresholdCombo) || (Object == LogMaxSizeCombo)))
  {
    // Formatted as upstream's sizes once left
    QComboBox * Combo = static_cast<QComboBox *>(Object);
    __int64 Size;
    if (TryStrToSize(ToUnicodeString(Combo->currentText()), Size))
    {
      Combo->setEditText(ToQString(SizeToStr(Size)));
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UpdateCustomCommandsView()
{
  int Current = ListIndex(CustomCommandsView);
  CustomCommandsView->clear();
  for (int Index = 0; Index < FCustomCommandList->Count; Index++)
  {
    const TCustomCommandType * Command = FCustomCommandList->Commands[Index];
    QTreeWidgetItem * Item = new QTreeWidgetItem(CustomCommandsView);
    Item->setText(0, ToQString(StripHotkey(Command->Name)));
    Item->setText(1, ToQString(Command->Command));
    Item->setText(2, ToQString(LoadStr(FLAGSET(Command->Params, ccLocal) ? CUSTOM_COMMAND_LOCAL : CUSTOM_COMMAND_REMOTE)));
  }
  CustomCommandsView->resizeColumnToContents(0);
  CustomCommandsView->resizeColumnToContents(2);
  SetListIndex(CustomCommandsView, Current);
}
//---------------------------------------------------------------------------
TShortCuts TPreferencesDialog::GetShortCuts()
{
  TShortCuts ShortCuts;
  if (AppConfiguration->SharedBookmarks != NULL)
  {
    AppConfiguration->SharedBookmarks->ShortCuts(ShortCuts);
  }
  FCustomCommandList->ShortCuts(ShortCuts);
  return ShortCuts;
}
//---------------------------------------------------------------------------
void TPreferencesDialog::AddEditCommand(bool Edit)
{
  TCustomCommandType Command;
  int Index = ListIndex(CustomCommandsView);

  if (Edit)
  {
    Command = *FCustomCommandList->Commands[Index];
  }

  TShortCuts ShortCuts = GetShortCuts();

  if (DoCustomCommandDialog(Command, FCustomCommandList.get(), (Edit ? ccmEdit : ccmAdd), 0, NULL, &ShortCuts))
  {
    TCustomCommandType * ACommand = new TCustomCommandType(Command);
    if (Edit)
    {
      FCustomCommandList->Change(Index, ACommand);
    }
    else if (Index >= 0)
    {
      FCustomCommandList->Insert(Index, ACommand);
    }
    else
    {
      FCustomCommandList->Add(ACommand);
      Index = FCustomCommandList->Count - 1;
    }

    UpdateCustomCommandsView();
    SetListIndex(CustomCommandsView, Index);
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::RemoveCommand()
{
  int Index = ListIndex(CustomCommandsView);
  FCustomCommandList->Delete(Index);
  UpdateCustomCommandsView();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CustomCommandMove(int Source, int Dest)
{
  FCustomCommandList->Move(Source, Dest);
  UpdateCustomCommandsView();
  SetListIndex(CustomCommandsView, Dest);
  UpdateControls();
}
//---------------------------------------------------------------------------
const TCopyParamType * TPreferencesDialog::GetCopyParam(int Index)
{
  if (Index == 0)
  {
    return &FCopyParams;
  }
  else
  {
    return FCopyParamList->CopyParams[Index - 1];
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UpdateCopyParamListView()
{
  int Current = ListIndex(CopyParamListView);
  CopyParamListView->clear();
  for (int Index = 0; Index < 1 + FCopyParamList->Count; Index++)
  {
    QTreeWidgetItem * Item = new QTreeWidgetItem(CopyParamListView);
    if (Index == 0)
    {
      Item->setText(0, ToQString(StripHotkey(LoadStr(COPY_PARAM_DEFAULT))));
      // As upstream: the default settings stand out
      QFont Font = Item->font(0);
      Font.setBold(true);
      Item->setFont(0, Font);
    }
    else
    {
      Item->setText(0, ToQString(StripHotkey(FCopyParamList->Names[Index - 1])));
      Item->setText(1, ToQString(BooleanToStr(FCopyParamList->Rules[Index - 1] != NULL)));
    }
  }
  CopyParamListView->resizeColumnToContents(1);
  SetListIndex(CopyParamListView, Current);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::CopyParamMove(int Source, int Dest)
{
  DebugAssert((Source > 0) && (Dest > 0));
  FCopyParamList->Move(Source - 1, Dest - 1);
  UpdateCopyParamListView();
  SetListIndex(CopyParamListView, Dest);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::RemoveCopyParam()
{
  int Index = ListIndex(CopyParamListView);
  DebugAssert((Index >= 1) && (Index < (1 + FCopyParamList->Count)));
  FCopyParamList->Delete(Index - 1);
  UpdateCopyParamListView();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::AddEditCopyParam(TCopyParamPresetMode Mode)
{
  int Index = ListIndex(CopyParamListView);
  bool Result;
  if ((Index == 0) && (Mode == cpmEdit))
  {
    Result = DoCopyParamCustomDialog(FCopyParams, 0);
  }
  else
  {
    TCopyParamRuleData * CopyParamRuleData =
      (FDialogData != NULL ? FDialogData->CopyParamRuleData : NULL);
    // negative (when default is selected) means add to the end
    Index--;

    TCopyParamType DefaultCopyParams;
    // For cpmAdd use defaults.
    if (Mode == cpmDuplicate)
    {
      // Only used, when duplicating default settings (Index < 0)
      DefaultCopyParams = FCopyParams;
    }
    else if (Mode == cpmEdit)
    {
      // For cpmEdit, DefaultCopyParams is never used.
      DebugAssert(Index >= 0);
    }

    Result = DoCopyParamPresetDialog(FCopyParamList.get(), Index, Mode, CopyParamRuleData, DefaultCopyParams);
    if (Result)
    {
      UpdateCopyParamListView();
      SetListIndex(CopyParamListView, Index + 1);
    }
  }

  if (Result)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UpdateEditorListView()
{
  int Current = ListIndex(EditorListView3);
  EditorListView3->clear();
  for (int Index = 0; Index < FEditorList->Count; Index++)
  {
    const TEditorPreferences * Editor = FEditorList->Editors[Index];
    QTreeWidgetItem * Item = new QTreeWidgetItem(EditorListView3);
    Item->setText(0, ToQString(Editor->Name));
    Item->setText(1, ToQString(Editor->Data->FileMask.Masks));
    if (Editor->Data->Editor == edExternal)
    {
      Item->setText(2, ToQString(BooleanToStr(Editor->Data->ExternalEditorText)));
    }
  }
  EditorListView3->resizeColumnToContents(1);
  EditorListView3->resizeColumnToContents(2);
  SetListIndex(EditorListView3, Current);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::EditorMove(int Source, int Dest)
{
  FEditorList->Move(Source, Dest);
  UpdateEditorListView();
  SetListIndex(EditorListView3, Dest);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::RemoveEditor()
{
  int Index = ListIndex(EditorListView3);
  DebugAssert((Index >= 0) && (Index < FEditorList->Count));
  FEditorList->Delete(Index);
  UpdateEditorListView();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::AddEditEditor(bool Edit)
{
  int Index = ListIndex(EditorListView3);

  std::unique_ptr<TEditorPreferences> Editor;
  if (Edit)
  {
    Editor.reset(new TEditorPreferences(*FEditorList->Editors[Index]->Data));
  }
  else
  {
    Editor.reset(new TEditorPreferences());
  }

  bool DummyRemember = false;
  if (DoEditorPreferencesDialog(Editor->GetData(), DummyRemember, (Edit ? epmEdit : epmAdd), true))
  {
    if (Edit)
    {
      FEditorList->Change(Index, Editor.release());
    }
    else
    {
      if (Index < 0)
      {
        Index = FEditorList->Count;
        FEditorList->Add(Editor.release());
      }
      else
      {
        FEditorList->Insert(Index, Editor.release());
      }
    }

    UpdateEditorListView();
    SetListIndex(EditorListView3, Index);
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UpdateFileColorsView()
{
  int Current = ListIndex(FileColorsView);
  FileColorsView->clear();
  for (const TFileColorData & FileColor : FFileColors)
  {
    QTreeWidgetItem * Item = new QTreeWidgetItem(FileColorsView);
    Item->setText(0, ToQString(FileColor.FileMask.Masks));
    if (FileColor.Color != TColor(0))
    {
      Item->setForeground(0, TColorToQColor(FileColor.Color));
    }
  }
  SetListIndex(FileColorsView, Current);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::AddEditFileColor(bool Edit)
{
  TFileColorData FileColorData;
  int Index = ListIndex(FileColorsView);
  if (Edit)
  {
    FileColorData = FFileColors[Index];
  }
  else
  {
    FileColorData.FileMask = AnyMask;
  }

  if (DoFileColorDialog(FileColorData))
  {
    if (Edit)
    {
      FFileColors[Index] = FileColorData;
    }
    else if (Index < 0)
    {
      FFileColors.push_back(FileColorData);
      Index = static_cast<int>(FFileColors.size()) - 1;
    }
    else
    {
      FFileColors.insert(FFileColors.begin() + Index, FileColorData);
    }

    UpdateFileColorsView();
    SetListIndex(FileColorsView, Index);
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::RemoveFileColor()
{
  int Index = ListIndex(FileColorsView);
  FFileColors.erase(FFileColors.begin() + Index);
  UpdateFileColorsView();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::FileColorMove(int Source, int Dest)
{
  std::swap(FFileColors[Source], FFileColors[Dest]);
  UpdateFileColorsView();
  SetListIndex(FileColorsView, Dest);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UpdateSshHostCAsView()
{
  int Current = ListIndex(SshHostCAsView);
  SshHostCAsView->clear();
  for (const TSshHostCA & SshHostCA : FSshHostCAPlainList)
  {
    QTreeWidgetItem * Item = new QTreeWidgetItem(SshHostCAsView);
    Item->setText(0, ToQString(SshHostCA.Name));
    Item->setText(1, ToQString(SshHostCA.ValidityExpression));
  }
  SshHostCAsView->resizeColumnToContents(0);
  SetListIndex(SshHostCAsView, Current);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::AddEditSshHostCA(bool Edit)
{
  int Index = ListIndex(SshHostCAsView);
  TSshHostCA SshHostCA;
  if (Edit)
  {
    SshHostCA = FSshHostCAPlainList[Index];
  }
  if (DoSshHostCADialog(!Edit, SshHostCA))
  {
    if (Edit)
    {
      FSshHostCAPlainList[Index] = SshHostCA;
    }
    else
    {
      FSshHostCAPlainList.push_back(SshHostCA);
      Index = static_cast<int>(FSshHostCAPlainList.size()) - 1;
    }
    UpdateSshHostCAsView();
    SetListIndex(SshHostCAsView, Index);
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::RemoveSshHostCA()
{
  int Index = ListIndex(SshHostCAsView);
  FSshHostCAPlainList.erase(FSshHostCAPlainList.begin() + Index);
  UpdateSshHostCAsView();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPreferencesDialog::PanelFontButtonClick()
{
  bool Ok = false;
  QFont Font = QFontDialog::getFont(&Ok, FPanelFont, this);
  if (Ok)
  {
    FPanelFont = Font;
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::EditorFontButtonClick()
{
  bool Ok = false;
  QFont Font = QFontDialog::getFont(&Ok, FEditorFont, this, QString(), QFontDialog::MonospacedFonts);
  if (Ok)
  {
    FEditorFont = Font;
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::EditorColorButtonClick(bool Background, const QPoint & Pos)
{
  // upstream's CreateEditorBackgroundColorMenu (the light colors of the
  // sessions) and CreateColorPopupMenu
  TColor & Color = Background ? FEditorBackgroundColor : FEditorFontColor;
  if (ColorMenuPopup(Pos, Color, Background))
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
bool TPreferencesDialog::CanSetMasterPassword()
{
  bool Result;
  bool Retry;
  do
  {
    Retry = false;
    Result = !AnyOtherInstanceOfSelf();

    if (!Result)
    {
      unsigned int Answer =
        MessageDialog(
          LoadStr(MASTER_PASSWORD_OTHER_INSTANCE), qtConfirmation,
          qaRetry | qaIgnore | qaCancel);

      switch (Answer)
      {
        case qaRetry:
          Retry = true;
          break;

        case qaIgnore:
          Result = true;
          break;

        case qaCancel:
        default:
          // noop
          break;
      }
    }
  }
  while (Retry && !Result);

  return Result;
}
//---------------------------------------------------------------------------
void TPreferencesDialog::MasterPasswordChanged(
  const UnicodeString & Message, TStrings * RecryptPasswordErrors)
{
  // Save master password.
  // This is unlikely to fail, as storage is written explicitly already
  // when writing the recrypted passwords
  Configuration->SaveExplicit();

  TQueryType QueryType = qtInformation;
  UnicodeString AMessage = Message;
  if (RecryptPasswordErrors->Count > 0)
  {
    AMessage = FMTLOAD(MASTER_PASSWORD_RECRYPT_ERRORS, (AMessage));
    QueryType = qtWarning;
  }
  MoreMessageDialog(
    AMessage, RecryptPasswordErrors, QueryType, qaOK);
}
//---------------------------------------------------------------------------
void TPreferencesDialog::ChangeMasterPassword(const UnicodeString & Message)
{
  UnicodeString NewPassword;
  if (DoChangeMasterPasswordDialog(NewPassword))
  {
    std::unique_ptr<TStrings> RecryptPasswordErrors(new TStringList());
    AppConfiguration->ChangeMasterPassword(NewPassword, RecryptPasswordErrors.get());
    // upstream's TWinConfiguration::RecryptPasswords: the sessions too
    TTerminalManager::Instance()->RecryptPasswords();
    MasterPasswordChanged(Message, RecryptPasswordErrors.get());
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::UseMasterPasswordCheckClick()
{
  if (UseMasterPasswordCheck->isChecked() != AppConfiguration->UseMasterPassword)
  {
    ::rtl::TFinally Finally([&]()
    {
      UseMasterPasswordCheck->setChecked(AppConfiguration->UseMasterPassword);
      UpdateControls();
    });
    if (CanSetMasterPassword())
    {
      if (UseMasterPasswordCheck->isChecked())
      {
        ChangeMasterPassword(LoadStr(MASTER_PASSWORD_SET2));
      }
      else
      {
        if (DoMasterPasswordDialog())
        {
          std::unique_ptr<TStrings> RecryptPasswordErrors(new TStringList());
          AppConfiguration->ClearMasterPassword(RecryptPasswordErrors.get());
          TTerminalManager::Instance()->RecryptPasswords();
          MasterPasswordChanged(LoadStr(MASTER_PASSWORD_CLEARED2), RecryptPasswordErrors.get());
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::DesktopIconButtonClick()
{
  if (MessageDialog(MainInstructions(LoadStr(CONFIRM_CREATE_ICON)), qtConfirmation, qaYes | qaNo) == qaYes)
  {
    CreateAppDesktopShortCut();
  }
}
//---------------------------------------------------------------------------
void TPreferencesDialog::RegisterAsUrlHandlersButtonClick()
{
  // OpenSCP: the desktop entry declares the URL schemes; this makes it their
  // default application
  QString DesktopEntry = QStringLiteral("openscp.desktop");
  if (QStandardPaths::locate(QStandardPaths::ApplicationsLocation, DesktopEntry).isEmpty())
  {
    throw Exception(MainInstructions(ToUnicodeString(tr("OpenSCP is not installed (its desktop entry %1 was not found).").arg(DesktopEntry))));
  }
  QStringList Arguments = { QStringLiteral("default"), DesktopEntry };
  for (const wchar_t * Scheme : { L"sftp", L"scp", L"ftp", L"ftps", L"ftpes", L"dav", L"davs", L"s3" })
  {
    Arguments.append(QStringLiteral("x-scheme-handler/") + QString::fromWCharArray(Scheme));
  }
  QProcess Process;
  Process.start(QStringLiteral("xdg-mime"), Arguments);
  if (!Process.waitForFinished() || (Process.exitStatus() != QProcess::NormalExit) || (Process.exitCode() != 0))
  {
    throw ExtException(
      ToUnicodeString(tr("Cannot make OpenSCP the default handler of the URL addresses.")),
      ToUnicodeString(Process.errorString() + QLatin1Char('\n') + QString::fromLocal8Bit(Process.readAllStandardError())));
  }
  MessageDialog(MainInstructions(ToUnicodeString(tr("OpenSCP is now the default handler of the URL addresses."))), qtInformation, qaOK);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoPreferencesDialog(TPreferencesMode APreferencesMode, TPreferencesDialogData * DialogData)
{
  TPreferencesDialog Dialog(nullptr, APreferencesMode);
  bool Result = Dialog.Execute(DialogData);
  if (Result)
  {
    Configuration->SaveExplicit();
  }
  return Result;
}
//---------------------------------------------------------------------------
