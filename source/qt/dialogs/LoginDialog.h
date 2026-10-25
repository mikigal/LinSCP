// The site manager (upstream's TLoginDialog, forms/Login.dfm): the tree of
// the stored sites and their folders, the basic settings of the selected
// site, and the commands to open, edit and organize them.
//---------------------------------------------------------------------------
#ifndef LoginDialogH
#define LoginDialogH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QDialog>
//---------------------------------------------------------------------------
class QAction;
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QLineEdit;
class QMenu;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTreeWidget;
class QToolButton;
class QTreeWidgetItem;
class TSessionData;
//---------------------------------------------------------------------------
class TLoginDialog : public QDialog
{
  Q_OBJECT

public:
  explicit TLoginDialog(QWidget * Parent = nullptr);
  virtual ~TLoginDialog();

  void Init(bool NeedTerminal);
  bool Execute(TList * DataList);

protected:
  virtual void done(int Result) override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  enum TNodeKind { nkNewSite, nkWorkspace, nkFolder, nkSite };

  QTreeWidget * SessionTree;
  QPushButton * ManageButton;
  QPushButton * ToolsMenuButton;
  QMenu * ToolsPopupMenu;
  QWidget * SitePanel;
  QGroupBox * BasicGroup;
  QLabel * TransferProtocolLabel;
  QComboBox * TransferProtocolCombo;
  QLineEdit * TransferProtocolView;
  QLabel * FtpsLabel;
  QComboBox * FtpsCombo;
  QLabel * WebDavsLabel;
  QComboBox * WebDavsCombo;
  QLineEdit * EncryptionView;
  QLineEdit * HostNameEdit;
  QSpinBox * PortNumberEdit;
  QLabel * UserNameLabel;
  QLineEdit * UserNameEdit;
  QLabel * PasswordLabel;
  QLineEdit * PasswordEdit;
  QWidget * BasicFtpPanel;
  QCheckBox * AnonymousLoginCheck;
  QWidget * BasicS3Panel;
  QCheckBox * S3CredentialsEnvCheck3;
  QComboBox * S3ProfileCombo;
  QPushButton * SaveButton;
  QPushButton * EditButton;
  QPushButton * EditCancelButton;
  QToolButton * AdvancedButton;
  QMenu * SessionAdvancedPopupMenu;
  QGroupBox * NoteGroup;
  QPlainTextEdit * NoteMemo;
  QGroupBox * ContentsGroupBox;
  QLineEdit * ContentsNameEdit;
  QPlainTextEdit * ContentsMemo;
  QPushButton * LoginButton;
  QPushButton * CloseButton;
  QMenu * ManageMenu;

  QAction * EditSessionAction;
  QAction * SaveSessionAction;
  QAction * SaveAsSessionAction;
  QAction * DeleteSessionAction;
  QAction * RenameSessionAction;
  QAction * NewSessionFolderAction;
  QAction * PreferencesAction;
  QAction * CloneToNewSiteAction;
  QAction * EditCancelAction;
  QAction * LoginAction;
  QAction * SetDefaultSessionAction;
  QAction * ResetNewSessionAction;
  QAction * PasteUrlAction;
  QAction * PuttyAction;
  QAction * DesktopIconAction;
  QAction * ImportSessionsAction;
  QAction * ImportAction;
  QAction * ExportAction;
  QAction * CleanUpAction;
  QAction * SessionAdvancedAction;
  QAction * SessionRawAction;
  QAction * CopyParamRuleAction;
  QAction * PreferencesLoggingAction;

  TSessionData * FNewSiteData;
  // A copy of the edited site with the settings that this dialog does not
  // show (upstream's advanced settings)
  TSessionData * FSessionData;
  TList * FDataList;
  bool FNeedTerminal;
  bool FEditing;
  bool FNewSiteKeepName;
  bool FForceNewSite;
  int FLoading;
  int FNoUpdate;
  bool FUpdatePortWithProtocol;
  int FDefaultPort;
  QString FUserNameLabel;
  QString FPasswordLabel;
  QTreeWidgetItem * FRenamingItem;
  QString FRenamingText;

  void CreateControls();
  QAction * CreateAction(const char * Name, const QString & Caption, void (TLoginDialog::*Method)());

  QTreeWidgetItem * Selected();
  void Select(QTreeWidgetItem * Node);
  static TNodeKind NodeKind(QTreeWidgetItem * Node);
  static bool IsNewSiteNode(QTreeWidgetItem * Node);
  static bool IsSiteNode(QTreeWidgetItem * Node);
  static bool IsSessionNode(QTreeWidgetItem * Node);
  static bool IsFolderNode(QTreeWidgetItem * Node);
  static bool IsWorkspaceNode(QTreeWidgetItem * Node);
  static bool IsFolderOrWorkspaceNode(QTreeWidgetItem * Node);
  static TSessionData * GetNodeSession(QTreeWidgetItem * Node);
  static UnicodeString NodeText(QTreeWidgetItem * Node);
  void SetNodeSession(QTreeWidgetItem * Node, TSessionData * Data);
  void InsertNode(QTreeWidgetItem * Parent, QTreeWidgetItem * Node);
  static int CompareNodes(QTreeWidgetItem * Node1, QTreeWidgetItem * Node2);
  QTreeWidgetItem * Child(QTreeWidgetItem * Parent, int Index);
  int ChildCount(QTreeWidgetItem * Parent);
  QTreeWidgetItem * AddSessionPath(UnicodeString Path, bool CanCreate, bool IsWorkspace);
  QTreeWidgetItem * AddSession(TSessionData * Data);
  void UpdateNodeIcon(QTreeWidgetItem * Node);
  QTreeWidgetItem * GetNewSiteNode();
  QTreeWidgetItem * FindSessionNode(TSessionData * SessionData, bool ByName);
  UnicodeString SessionNodePath(QTreeWidgetItem * Node);
  QTreeWidgetItem * SessionFolderNode(QTreeWidgetItem * Node);
  bool HasNodeAnySession(QTreeWidgetItem * Node, bool NeedCanOpen = false);
  UnicodeString GetFolderOrWorkspaceContents(QTreeWidgetItem * Node, const UnicodeString & CommonRoot);
  void CheckDuplicateFolder(QTreeWidgetItem * Parent, const UnicodeString & Text, QTreeWidgetItem * Node);
  void CheckIsSessionFolder(QTreeWidgetItem * Node);

  void LoadSessions();
  void LoadContents();
  void LoadSession(TSessionData * SessionData);
  void SaveSession(TSessionData * SessionData);
  void InvalidateSessionData();
  void ResetNewSiteData();
  void NewSite();
  void Default();
  TSessionData * GetSelectedSession();
  TSessionData * GetSessionData();
  TSessionData * GetEditingSessionData();
  TSessionData * CloneSelectedSession();
  void SaveDataList(TList * DataList);
  bool IsEditable();
  bool IsCloneToNewSiteDefault();
  bool IsSiteAndCanOpen();
  bool IsFolderOrWorkspaceAndCanOpen();
  bool CanOpen();
  bool EnsureNotEditing();
  void CancelEditing();
  void PersistNewSiteIfNeeded(QTreeWidgetItem * Node);
  void SaveState();
  void LoadState();
  void UpdateControls();
  void UpdateActions();

  TFSProtocol GetFSProtocol(bool RequireScpFallbackDistinction);
  int FSProtocolToIndex(TFSProtocol FSProtocol, bool & AllowScpFallback);
  TFSProtocol IndexToFSProtocol(int Index, bool AllowScpFallback);
  static int FtpsToIndex(TFtps Ftps);
  TFtps GetFtps();
  int DefaultPort();
  void UpdatePortWithProtocol();
  UnicodeString GetS3GeneralName();
  UnicodeString GetS3Profile();
  void LoadS3Profiles();
  void UpdateS3Credentials();
  void ParseUrl(const UnicodeString & Url);
  void ParseHostName();

  void SessionTreeCurrentItemChanged(QTreeWidgetItem * Current, QTreeWidgetItem * Previous);
  void SessionTreeActivated(QTreeWidgetItem * Node);
  void SessionTreeEdited(QTreeWidgetItem * Node, const UnicodeString & S);
  void SessionTreeItemChanged(QTreeWidgetItem * Node);
  void TransferProtocolComboChange();
  void EncryptionComboChange();
  void PortNumberEditChange();
  void AnonymousLoginCheckClick();
  void S3CredentialsEnvCheck3Click();
  void DataChange();

  void EditSession();
  void EditSessionActionExecute();
  void SaveAsSession(bool ForceDialog);
  void SaveSessionActionExecute();
  void SaveAsSessionActionExecute();
  void DeleteSessionActionExecute();
  void RenameSessionActionExecute();
  void PreferencesActionExecute();
  void NewSessionFolderActionExecute();
  void CloneToNewSite();
  void EditCancelActionExecute();
  void LoginActionExecute();
  void SetDefaultSessionActionExecute();
  void ResetNewSessionActionExecute();
  void PasteUrlActionExecute();
  void SessionAdvancedActionExecute() { SessionAdvanced(false); }
  void SessionRawActionExecute() { SessionAdvanced(true); }
  void SessionAdvanced(bool Raw);
  void CopyParamRuleActionExecute();
  void PuttyActionExecute();
  void DesktopIconActionExecute();
  void ImportSessionsActionExecute();
  // The configuration restored from a backup (the application restarts),
  // backed up, the application data cleaned up
  void ImportActionExecute();
  void ExportActionExecute();
  void CleanUpActionExecute();
  void PreferencesLoggingActionExecute();
};
//---------------------------------------------------------------------------
#endif
