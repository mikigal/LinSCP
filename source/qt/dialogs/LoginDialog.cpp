// The site manager (upstream's forms/Login.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <S3FileSystem.h>

#include "LoginDialog.h"

#include <QAbstractItemDelegate>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStyle>
#include <QTreeWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
static const TFSProtocol FSOrder[] = { fsSFTPonly, fsSCPonly, fsFTP, fsWebDAV, fsS3 };
//---------------------------------------------------------------------------
enum
{
  NodeKindRole = Qt::UserRole,
  NodeSessionRole,
};
//---------------------------------------------------------------------------
bool DoLoginDialog(TList * DataList, QWidget * Parent, bool NeedTerminal)
{
  DebugAssert(DataList != NULL);
  TLoginDialog LoginDialog(Parent);
  LoginDialog.Init(NeedTerminal);
  return LoginDialog.Execute(DataList);
}
//---------------------------------------------------------------------------
TLoginDialog::TLoginDialog(QWidget * Parent) :
  QDialog(Parent)
{
  FNewSiteData = new TSessionData(L"");
  FSessionData = NULL;
  FDataList = NULL;
  FNeedTerminal = false;
  FEditing = false;
  FNewSiteKeepName = false;
  FForceNewSite = false;
  FLoading = 0;
  FNoUpdate = 0;
  FUpdatePortWithProtocol = true;
  FDefaultPort = 0;
  FRenamingItem = nullptr;

  CreateControls();
}
//---------------------------------------------------------------------------
TLoginDialog::~TLoginDialog()
{
  delete FNewSiteData;
  InvalidateSessionData();
}
//---------------------------------------------------------------------------
QAction * TLoginDialog::CreateAction(const char * Name, const QString & Caption, void (TLoginDialog::*Method)())
{
  QAction * Action = new QAction(Caption, this);
  Action->setObjectName(QString::fromLatin1(Name));
  connect(Action, &QAction::triggered, this, [this, Method]() { (this->*Method)(); });
  return Action;
}
//---------------------------------------------------------------------------
static void BindButton(QPushButton * Button, QAction * Action)
{
  QObject::connect(Button, &QPushButton::clicked, Action, &QAction::trigger);
}
//---------------------------------------------------------------------------
void TLoginDialog::CreateControls()
{
  setObjectName(QStringLiteral("LoginDialog"));
  setWindowTitle(tr("Login"));

  EditSessionAction = CreateAction("EditSessionAction", tr("&Edit"), &TLoginDialog::EditSessionActionExecute);
  SaveSessionAction = CreateAction("SaveSessionAction", tr("&Save"), &TLoginDialog::SaveSessionActionExecute);
  SaveAsSessionAction = CreateAction("SaveAsSessionAction", tr("Save &As..."), &TLoginDialog::SaveAsSessionActionExecute);
  DeleteSessionAction = CreateAction("DeleteSessionAction", tr("&Delete"), &TLoginDialog::DeleteSessionActionExecute);
  DeleteSessionAction->setShortcut(QKeySequence::Delete);
  DeleteSessionAction->setShortcutContext(Qt::WidgetShortcut);
  RenameSessionAction = CreateAction("RenameSessionAction", tr("&Rename"), &TLoginDialog::RenameSessionActionExecute);
  RenameSessionAction->setShortcut(QKeySequence(Qt::Key_F2));
  RenameSessionAction->setShortcutContext(Qt::WidgetShortcut);
  NewSessionFolderAction =
    CreateAction("NewSessionFolderAction", tr("Ne&w Folder..."), &TLoginDialog::NewSessionFolderActionExecute);
  CloneToNewSiteAction = CreateAction("CloneToNewSiteAction", tr("&Clone to New Site"), &TLoginDialog::CloneToNewSite);
  EditCancelAction = CreateAction("EditCancelAction", tr("Cancel"), &TLoginDialog::EditCancelActionExecute);
  LoginAction = CreateAction("LoginAction", ToQString(LoadStr(LOGIN_LOGIN)), &TLoginDialog::LoginActionExecute);
  SetDefaultSessionAction =
    CreateAction("SetDefaultSessionAction", tr("Set De&faults"), &TLoginDialog::SetDefaultSessionActionExecute);
  ResetNewSessionAction = CreateAction("ResetNewSessionAction", tr("&Reset"), &TLoginDialog::ResetNewSessionActionExecute);
  PasteUrlAction = CreateAction("PasteUrlAction", tr("Paste Session &URL"), &TLoginDialog::PasteUrlActionExecute);
  PasteUrlAction->setShortcut(QKeySequence::Paste);
  PasteUrlAction->setShortcutContext(Qt::WidgetShortcut);

  QHBoxLayout * MainLayout = new QHBoxLayout(this);

  // Sites
  QVBoxLayout * SitesLayout = new QVBoxLayout();
  MainLayout->addLayout(SitesLayout, 1);
  SessionTree = new QTreeWidget(this);
  SessionTree->setObjectName(QStringLiteral("SessionTree"));
  SessionTree->setHeaderHidden(true);
  SessionTree->setColumnCount(1);
  SessionTree->setSortingEnabled(false);
  SessionTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
  SessionTree->setMinimumWidth(fontMetrics().averageCharWidth() * 36);
  SessionTree->addAction(DeleteSessionAction);
  SessionTree->addAction(RenameSessionAction);
  SessionTree->addAction(PasteUrlAction);
  SitesLayout->addWidget(SessionTree, 1);
  connect(SessionTree, &QTreeWidget::currentItemChanged, this,
    [this](QTreeWidgetItem * Current, QTreeWidgetItem * Previous) { SessionTreeCurrentItemChanged(Current, Previous); });
  connect(SessionTree, &QTreeWidget::itemDoubleClicked, this,
    [this](QTreeWidgetItem * Node) { SessionTreeActivated(Node); });
  connect(SessionTree, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem * Node) { UpdateNodeIcon(Node); });
  connect(SessionTree, &QTreeWidget::itemCollapsed, this, [this](QTreeWidgetItem * Node) { UpdateNodeIcon(Node); });
  connect(SessionTree, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem * Node) { SessionTreeItemChanged(Node); });
  connect(SessionTree->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, [this]()
  {
    if (FRenamingItem != nullptr)
    {
      FRenamingItem->setFlags(FRenamingItem->flags() & ~Qt::ItemIsEditable);
      FRenamingItem = nullptr;
      UpdateControls();
    }
  });

  QHBoxLayout * SitesButtonsLayout = new QHBoxLayout();
  SitesLayout->addLayout(SitesButtonsLayout);
  SitesButtonsLayout->addStretch(1);
  ManageButton = new QPushButton(tr("&Manage"), this);
  ManageButton->setObjectName(QStringLiteral("ManageButton"));
  ManageButton->setAutoDefault(false);
  ManageMenu = new QMenu(this);
  ManageMenu->setObjectName(QStringLiteral("ManageSitePopupMenu"));
  ManageMenu->addAction(LoginAction);
  ManageMenu->addSeparator();
  ManageMenu->addAction(EditSessionAction);
  ManageMenu->addAction(SaveAsSessionAction);
  ManageMenu->addAction(DeleteSessionAction);
  ManageMenu->addAction(RenameSessionAction);
  ManageMenu->addAction(CloneToNewSiteAction);
  ManageMenu->addSeparator();
  ManageMenu->addAction(NewSessionFolderAction);
  ManageMenu->addSeparator();
  ManageMenu->addAction(PasteUrlAction);
  ManageMenu->addAction(SetDefaultSessionAction);
  ManageMenu->addAction(ResetNewSessionAction);
  ManageButton->setMenu(ManageMenu);
  connect(ManageMenu, &QMenu::aboutToShow, this, [this]() { UpdateActions(); });
  SitesButtonsLayout->addWidget(ManageButton);

  // Session
  QVBoxLayout * MainPanelLayout = new QVBoxLayout();
  MainLayout->addLayout(MainPanelLayout, 1);

  SitePanel = new QWidget(this);
  SitePanel->setObjectName(QStringLiteral("SitePanel"));
  QVBoxLayout * SitePanelLayout = new QVBoxLayout(SitePanel);
  SitePanelLayout->setContentsMargins(0, 0, 0, 0);
  MainPanelLayout->addWidget(SitePanel);

  BasicGroup = new QGroupBox(tr("Session"), SitePanel);
  BasicGroup->setObjectName(QStringLiteral("BasicGroup"));
  SitePanelLayout->addWidget(BasicGroup);
  QGridLayout * Grid = new QGridLayout(BasicGroup);
  Grid->setColumnStretch(0, 2);
  Grid->setColumnStretch(1, 1);
  Grid->setColumnStretch(2, 1);
  Grid->setColumnStretch(3, 1);

  TransferProtocolLabel = new QLabel(tr("&File protocol:"), BasicGroup);
  TransferProtocolLabel->setObjectName(QStringLiteral("Label22"));
  Grid->addWidget(TransferProtocolLabel, 0, 0, 1, 2);
  TransferProtocolCombo = new QComboBox(BasicGroup);
  TransferProtocolCombo->setObjectName(QStringLiteral("TransferProtocolCombo"));
  TransferProtocolCombo->addItems({ QStringLiteral("SFTP"), QStringLiteral("SCP"), QStringLiteral("FTP"),
    QStringLiteral("WebDAV"), QStringLiteral("Amazon S3") });
  Grid->addWidget(TransferProtocolCombo, 1, 0, 1, 2);
  TransferProtocolView = new QLineEdit(BasicGroup);
  TransferProtocolView->setObjectName(QStringLiteral("TransferProtocolView"));
  TransferProtocolView->setReadOnly(true);
  Grid->addWidget(TransferProtocolView, 1, 0, 1, 2);
  TransferProtocolLabel->setBuddy(TransferProtocolCombo);
  connect(TransferProtocolCombo, &QComboBox::currentIndexChanged, this, [this]() { TransferProtocolComboChange(); });

  FtpsLabel = new QLabel(tr("&Encryption:"), BasicGroup);
  FtpsLabel->setObjectName(QStringLiteral("FtpsLabel"));
  Grid->addWidget(FtpsLabel, 0, 2, 1, 2);
  WebDavsLabel = new QLabel(tr("&Encryption:"), BasicGroup);
  WebDavsLabel->setObjectName(QStringLiteral("WebDavsLabel"));
  Grid->addWidget(WebDavsLabel, 0, 2, 1, 2);
  QString NoEncryption = tr("No encryption");
  FtpsCombo = new QComboBox(BasicGroup);
  FtpsCombo->setObjectName(QStringLiteral("FtpsCombo"));
  FtpsCombo->addItems({ NoEncryption, ToQString(LoadStr(FTPS_IMPLICIT)), ToQString(LoadStr(FTPS_EXPLICIT)) });
  Grid->addWidget(FtpsCombo, 1, 2, 1, 2);
  WebDavsCombo = new QComboBox(BasicGroup);
  WebDavsCombo->setObjectName(QStringLiteral("WebDavsCombo"));
  WebDavsCombo->addItems({ NoEncryption, ToQString(LoadStr(FTPS_IMPLICIT)) });
  Grid->addWidget(WebDavsCombo, 1, 2, 1, 2);
  EncryptionView = new QLineEdit(BasicGroup);
  EncryptionView->setObjectName(QStringLiteral("EncryptionView"));
  EncryptionView->setReadOnly(true);
  Grid->addWidget(EncryptionView, 1, 2, 1, 2);
  FtpsLabel->setBuddy(FtpsCombo);
  WebDavsLabel->setBuddy(WebDavsCombo);
  connect(FtpsCombo, &QComboBox::currentIndexChanged, this, [this]() { EncryptionComboChange(); });
  connect(WebDavsCombo, &QComboBox::currentIndexChanged, this, [this]() { EncryptionComboChange(); });

  QLabel * HostNameLabel = new QLabel(tr("&Hostname:"), BasicGroup);
  HostNameLabel->setObjectName(QStringLiteral("Label1"));
  Grid->addWidget(HostNameLabel, 2, 0, 1, 3);
  HostNameEdit = new QLineEdit(BasicGroup);
  HostNameEdit->setObjectName(QStringLiteral("HostNameEdit"));
  HostNameEdit->setMaxLength(255);
  HostNameEdit->installEventFilter(this);
  Grid->addWidget(HostNameEdit, 3, 0, 1, 3);
  HostNameLabel->setBuddy(HostNameEdit);
  connect(HostNameEdit, &QLineEdit::textChanged, this, [this]() { DataChange(); });

  QLabel * PortNumberLabel = new QLabel(tr("Po&rt number:"), BasicGroup);
  PortNumberLabel->setObjectName(QStringLiteral("Label2"));
  Grid->addWidget(PortNumberLabel, 2, 3);
  PortNumberEdit = new QSpinBox(BasicGroup);
  PortNumberEdit->setObjectName(QStringLiteral("PortNumberEdit"));
  PortNumberEdit->setRange(1, 65535);
  PortNumberEdit->setAlignment(Qt::AlignRight);
  Grid->addWidget(PortNumberEdit, 3, 3);
  PortNumberLabel->setBuddy(PortNumberEdit);
  connect(PortNumberEdit, &QSpinBox::valueChanged, this, [this]() { PortNumberEditChange(); });

  UserNameLabel = new QLabel(tr("&Username:"), BasicGroup);
  UserNameLabel->setObjectName(QStringLiteral("UserNameLabel"));
  FUserNameLabel = UserNameLabel->text();
  Grid->addWidget(UserNameLabel, 4, 0, 1, 2);
  UserNameEdit = new QLineEdit(BasicGroup);
  UserNameEdit->setObjectName(QStringLiteral("UserNameEdit"));
  UserNameEdit->setMaxLength(128);
  Grid->addWidget(UserNameEdit, 5, 0, 1, 2);
  UserNameLabel->setBuddy(UserNameEdit);
  connect(UserNameEdit, &QLineEdit::textChanged, this, [this]() { DataChange(); });

  PasswordLabel = new QLabel(tr("&Password:"), BasicGroup);
  PasswordLabel->setObjectName(QStringLiteral("PasswordLabel"));
  FPasswordLabel = PasswordLabel->text();
  Grid->addWidget(PasswordLabel, 4, 2, 1, 2);
  PasswordEdit = new QLineEdit(BasicGroup);
  PasswordEdit->setObjectName(QStringLiteral("PasswordEdit"));
  PasswordEdit->setMaxLength(100);
  PasswordEdit->setEchoMode(QLineEdit::Password);
  Grid->addWidget(PasswordEdit, 5, 2, 1, 2);
  PasswordLabel->setBuddy(PasswordEdit);
  connect(PasswordEdit, &QLineEdit::textChanged, this, [this]() { DataChange(); });

  BasicFtpPanel = new QWidget(BasicGroup);
  BasicFtpPanel->setObjectName(QStringLiteral("BasicFtpPanel"));
  QHBoxLayout * FtpLayout = new QHBoxLayout(BasicFtpPanel);
  FtpLayout->setContentsMargins(0, 0, 0, 0);
  AnonymousLoginCheck = new QCheckBox(tr("A&nonymous login"), BasicFtpPanel);
  AnonymousLoginCheck->setObjectName(QStringLiteral("AnonymousLoginCheck"));
  FtpLayout->addWidget(AnonymousLoginCheck);
  FtpLayout->addStretch(1);
  Grid->addWidget(BasicFtpPanel, 6, 0, 1, 4);
  connect(AnonymousLoginCheck, &QCheckBox::toggled, this, [this]() { AnonymousLoginCheckClick(); });

  BasicS3Panel = new QWidget(BasicGroup);
  BasicS3Panel->setObjectName(QStringLiteral("BasicS3Panel"));
  QHBoxLayout * S3Layout = new QHBoxLayout(BasicS3Panel);
  S3Layout->setContentsMargins(0, 0, 0, 0);
  S3CredentialsEnvCheck3 = new QCheckBox(tr("&Credentials from AWS environment:"), BasicS3Panel);
  S3CredentialsEnvCheck3->setObjectName(QStringLiteral("S3CredentialsEnvCheck3"));
  S3Layout->addWidget(S3CredentialsEnvCheck3);
  S3ProfileCombo = new QComboBox(BasicS3Panel);
  S3ProfileCombo->setObjectName(QStringLiteral("S3ProfileCombo"));
  S3ProfileCombo->setEditable(true);
  S3ProfileCombo->setInsertPolicy(QComboBox::NoInsert);
  S3Layout->addWidget(S3ProfileCombo, 1);
  Grid->addWidget(BasicS3Panel, 7, 0, 1, 4);
  connect(S3CredentialsEnvCheck3, &QCheckBox::toggled, this, [this]() { S3CredentialsEnvCheck3Click(); });
  connect(S3ProfileCombo, &QComboBox::currentTextChanged, this, [this]() { S3CredentialsEnvCheck3Click(); });

  QHBoxLayout * SiteButtonsLayout = new QHBoxLayout();
  Grid->addLayout(SiteButtonsLayout, 8, 0, 1, 4);
  SaveButton = new QPushButton(SaveSessionAction->text(), BasicGroup);
  SaveButton->setObjectName(QStringLiteral("SaveButton"));
  BindButton(SaveButton, SaveSessionAction);
  SiteButtonsLayout->addWidget(SaveButton);
  EditButton = new QPushButton(EditSessionAction->text(), BasicGroup);
  EditButton->setObjectName(QStringLiteral("EditButton"));
  BindButton(EditButton, EditSessionAction);
  SiteButtonsLayout->addWidget(EditButton);
  EditCancelButton = new QPushButton(EditCancelAction->text(), BasicGroup);
  EditCancelButton->setObjectName(QStringLiteral("EditCancelButton"));
  BindButton(EditCancelButton, EditCancelAction);
  SiteButtonsLayout->addWidget(EditCancelButton);
  SiteButtonsLayout->addStretch(1);
  // The advanced settings of sites (upstream's AdvancedButton) are not
  // ported yet

  NoteGroup = new QGroupBox(tr("Note"), SitePanel);
  NoteGroup->setObjectName(QStringLiteral("NoteGroup"));
  QVBoxLayout * NoteLayout = new QVBoxLayout(NoteGroup);
  NoteMemo = new QPlainTextEdit(NoteGroup);
  NoteMemo->setObjectName(QStringLiteral("NoteMemo"));
  NoteMemo->setReadOnly(true);
  NoteMemo->setFrameShape(QFrame::NoFrame);
  NoteMemo->setMaximumHeight(fontMetrics().lineSpacing() * 4);
  NoteLayout->addWidget(NoteMemo);
  SitePanelLayout->addWidget(NoteGroup);

  // Site folder or workspace
  ContentsGroupBox = new QGroupBox(this);
  ContentsGroupBox->setObjectName(QStringLiteral("ContentsGroupBox"));
  QVBoxLayout * ContentsLayout = new QVBoxLayout(ContentsGroupBox);
  QLabel * ContentsLabel = new QLabel(tr("Name:"), ContentsGroupBox);
  ContentsLabel->setObjectName(QStringLiteral("ContentsLabel"));
  ContentsLayout->addWidget(ContentsLabel);
  ContentsNameEdit = new QLineEdit(ContentsGroupBox);
  ContentsNameEdit->setObjectName(QStringLiteral("ContentsNameEdit"));
  ContentsNameEdit->setReadOnly(true);
  ContentsLayout->addWidget(ContentsNameEdit);
  ContentsMemo = new QPlainTextEdit(ContentsGroupBox);
  ContentsMemo->setObjectName(QStringLiteral("ContentsMemo"));
  ContentsMemo->setReadOnly(true);
  ContentsLayout->addWidget(ContentsMemo);
  MainPanelLayout->addWidget(ContentsGroupBox, 1);

  MainPanelLayout->addStretch(1);

  QHBoxLayout * ButtonLayout = new QHBoxLayout();
  MainPanelLayout->addLayout(ButtonLayout);
  ButtonLayout->addStretch(1);
  LoginButton = new QPushButton(LoginAction->text(), this);
  LoginButton->setObjectName(QStringLiteral("LoginButton"));
  LoginButton->setIcon(QIcon::fromTheme(QStringLiteral("network-connect")));
  BindButton(LoginButton, LoginAction);
  ButtonLayout->addWidget(LoginButton);
  CloseButton = new QPushButton(tr("Close"), this);
  CloseButton->setObjectName(QStringLiteral("CloseButton"));
  connect(CloseButton, &QPushButton::clicked, this, &QDialog::reject);
  ButtonLayout->addWidget(CloseButton);

  for (QPushButton * Button : findChildren<QPushButton *>())
  {
    Button->setAutoDefault(false);
  }

  resize(fontMetrics().averageCharWidth() * 110, fontMetrics().lineSpacing() * 26);
}
//---------------------------------------------------------------------------
void TLoginDialog::Init(bool NeedTerminal)
{
  FNeedTerminal = NeedTerminal;
  LoadSessions();
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLoginDialog::InvalidateSessionData()
{
  delete FSessionData;
  FSessionData = NULL;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::Selected()
{
  return SessionTree->currentItem();
}
//---------------------------------------------------------------------------
void TLoginDialog::Select(QTreeWidgetItem * Node)
{
  SessionTree->setCurrentItem(Node);
  if (Node != nullptr)
  {
    SessionTree->scrollToItem(Node);
  }
}
//---------------------------------------------------------------------------
TLoginDialog::TNodeKind TLoginDialog::NodeKind(QTreeWidgetItem * Node)
{
  return static_cast<TNodeKind>(Node->data(0, NodeKindRole).toInt());
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsNewSiteNode(QTreeWidgetItem * Node)
{
  return (Node != nullptr) && (NodeKind(Node) == nkNewSite);
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsSiteNode(QTreeWidgetItem * Node)
{
  return (Node != nullptr) && (NodeKind(Node) == nkSite);
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsSessionNode(QTreeWidgetItem * Node)
{
  return IsSiteNode(Node) || IsNewSiteNode(Node);
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsFolderNode(QTreeWidgetItem * Node)
{
  return (Node != nullptr) && (NodeKind(Node) == nkFolder);
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsWorkspaceNode(QTreeWidgetItem * Node)
{
  return (Node != nullptr) && (NodeKind(Node) == nkWorkspace);
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsFolderOrWorkspaceNode(QTreeWidgetItem * Node)
{
  return IsFolderNode(Node) || IsWorkspaceNode(Node);
}
//---------------------------------------------------------------------------
TSessionData * TLoginDialog::GetNodeSession(QTreeWidgetItem * Node)
{
  return DebugNotNull(reinterpret_cast<TSessionData *>(Node->data(0, NodeSessionRole).value<quintptr>()));
}
//---------------------------------------------------------------------------
void TLoginDialog::SetNodeSession(QTreeWidgetItem * Node, TSessionData * Data)
{
  Node->setData(0, NodeSessionRole, QVariant::fromValue(reinterpret_cast<quintptr>(Data)));
}
//---------------------------------------------------------------------------
UnicodeString TLoginDialog::NodeText(QTreeWidgetItem * Node)
{
  return ToUnicodeString(Node->text(0));
}
//---------------------------------------------------------------------------
int TLoginDialog::CompareNodes(QTreeWidgetItem * Node1, QTreeWidgetItem * Node2)
{
  // The new site first, then workspaces, folders and sites
  int Result = static_cast<int>(NodeKind(Node1)) - static_cast<int>(NodeKind(Node2));
  if (Result == 0)
  {
    Result = CompareLogicalText(NodeText(Node1), NodeText(Node2), true);
  }
  return Result;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::Child(QTreeWidgetItem * Parent, int Index)
{
  return (Parent == nullptr) ? SessionTree->topLevelItem(Index) : Parent->child(Index);
}
//---------------------------------------------------------------------------
int TLoginDialog::ChildCount(QTreeWidgetItem * Parent)
{
  return (Parent == nullptr) ? SessionTree->topLevelItemCount() : Parent->childCount();
}
//---------------------------------------------------------------------------
void TLoginDialog::InsertNode(QTreeWidgetItem * Parent, QTreeWidgetItem * Node)
{
  int Index = 0;
  while ((Index < ChildCount(Parent)) && (CompareNodes(Child(Parent, Index), Node) <= 0))
  {
    Index++;
  }
  if (Parent == nullptr)
  {
    SessionTree->insertTopLevelItem(Index, Node);
  }
  else
  {
    Parent->insertChild(Index, Node);
  }
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::AddSessionPath(UnicodeString Path, bool CanCreate, bool IsWorkspace)
{
  QTreeWidgetItem * Parent = nullptr;
  while (!Path.IsEmpty())
  {
    UnicodeString Folder = CutToChar(Path, L'/', false);
    QTreeWidgetItem * Node = nullptr;
    // Folders may have the same names as sites on the same level
    for (int Index = 0; (Node == nullptr) && (Index < ChildCount(Parent)); Index++)
    {
      QTreeWidgetItem * ANode = Child(Parent, Index);
      if (IsFolderOrWorkspaceNode(ANode) && AnsiSameText(NodeText(ANode), Folder))
      {
        Node = ANode;
      }
    }

    if (Node == nullptr)
    {
      if (!CanCreate)
      {
        return nullptr;
      }
      Node = new QTreeWidgetItem();
      Node->setText(0, ToQString(Folder));
      Node->setData(0, NodeKindRole, (IsWorkspace ? nkWorkspace : nkFolder));
      InsertNode(Parent, Node);
      UpdateNodeIcon(Node);
    }
    Parent = Node;
  }
  return Parent;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::AddSession(TSessionData * Data)
{
  QTreeWidgetItem * Parent = AddSessionPath(UnixExtractFilePath(Data->Name), true, Data->IsWorkspace);
  QTreeWidgetItem * Node = new QTreeWidgetItem();
  Node->setText(0, ToQString(UnixExtractFileName(Data->Name)));
  Node->setData(0, NodeKindRole, nkSite);
  SetNodeSession(Node, Data);
  InsertNode(Parent, Node);
  UpdateNodeIcon(Node);
  return Node;
}
//---------------------------------------------------------------------------
void TLoginDialog::UpdateNodeIcon(QTreeWidgetItem * Node)
{
  QIcon Icon;
  switch (NodeKind(Node))
  {
    case nkNewSite:
      Icon = QIcon::fromTheme(QStringLiteral("list-add"), style()->standardIcon(QStyle::SP_FileIcon));
      break;
    case nkWorkspace:
      Icon = QIcon::fromTheme(QStringLiteral("window-duplicate"), style()->standardIcon(QStyle::SP_DesktopIcon));
      break;
    case nkFolder:
      Icon =
        Node->isExpanded() ?
          QIcon::fromTheme(QStringLiteral("folder-open"), style()->standardIcon(QStyle::SP_DirOpenIcon)) :
          QIcon::fromTheme(QStringLiteral("folder"), style()->standardIcon(QStyle::SP_DirClosedIcon));
      break;
    case nkSite:
      Icon = QIcon::fromTheme(QStringLiteral("network-server"), style()->standardIcon(QStyle::SP_ComputerIcon));
      break;
  }
  Node->setIcon(0, Icon);
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::GetNewSiteNode()
{
  QTreeWidgetItem * Result = SessionTree->topLevelItem(0);
  DebugAssert(IsNewSiteNode(Result));
  return Result;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::FindSessionNode(TSessionData * SessionData, bool ByName)
{
  for (QTreeWidgetItemIterator It(SessionTree); *It != nullptr; ++It)
  {
    QTreeWidgetItem * Node = *It;
    if (IsSessionNode(Node))
    {
      TSessionData * Data = GetNodeSession(Node);
      if ((!ByName && (Data == SessionData)) ||
          (ByName && IsSiteNode(Node) && (Data->Name == SessionData->Name)))
      {
        return Node;
      }
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
UnicodeString TLoginDialog::SessionNodePath(QTreeWidgetItem * Node)
{
  UnicodeString Path;
  if ((Node != nullptr) && !IsNewSiteNode(Node))
  {
    Path = NodeText(Node);
    Node = Node->parent();
    while (Node != nullptr)
    {
      Path.Insert(UnixIncludeTrailingBackslash(NodeText(Node)), 1);
      Node = Node->parent();
    }
  }
  return Path;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLoginDialog::SessionFolderNode(QTreeWidgetItem * Node)
{
  QTreeWidgetItem * Result;
  if (IsSessionNode(Node))
  {
    Result = Node->parent();
  }
  else if (IsFolderNode(Node))
  {
    Result = Node;
  }
  else
  {
    Result = nullptr;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TLoginDialog::HasNodeAnySession(QTreeWidgetItem * Node, bool NeedCanOpen)
{
  bool Result = false;
  for (int Index = 0; !Result && (Index < Node->childCount()); Index++)
  {
    QTreeWidgetItem * ANode = Node->child(Index);
    Result =
      (IsSiteNode(ANode) && (!NeedCanOpen || StoredSessions->CanOpen(GetNodeSession(ANode)))) ||
      HasNodeAnySession(ANode, NeedCanOpen);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TLoginDialog::GetFolderOrWorkspaceContents(QTreeWidgetItem * Node, const UnicodeString & CommonRoot)
{
  UnicodeString Contents;
  std::unique_ptr<TStrings> Names(StoredSessions->GetFolderOrWorkspaceList(SessionNodePath(Node)));
  for (int Index = 0; Index < Names->Count; Index++)
  {
    UnicodeString Name = Names->Strings[Index];
    if (StartsStr(CommonRoot, Name))
    {
      Name.Delete(1, CommonRoot.Length());
    }
    Contents += Name + L"\n";
  }
  return Contents;
}
//---------------------------------------------------------------------------
void TLoginDialog::CheckDuplicateFolder(QTreeWidgetItem * Parent, const UnicodeString & Text, QTreeWidgetItem * Node)
{
  // Folders may have the same names as sites on the same level
  for (int Index = 0; Index < ChildCount(Parent); Index++)
  {
    QTreeWidgetItem * ANode = Child(Parent, Index);
    if ((ANode != Node) && IsFolderOrWorkspaceNode(ANode) && AnsiSameText(NodeText(ANode), Text))
    {
      throw Exception(MainInstructions(FMTLOAD(LOGIN_DUPLICATE_SESSION_FOLDER_WORKSPACE, (Text))));
    }
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::CheckIsSessionFolder(QTreeWidgetItem * Node)
{
  if ((Node != nullptr) && (Node->parent() != nullptr))
  {
    CheckIsSessionFolder(Node->parent());
  }
  if (IsWorkspaceNode(Node))
  {
    throw Exception(FMTLOAD(WORKSPACE_NOT_FOLDER, (SessionNodePath(Node))));
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::LoadSessions()
{
  {
    TAutoNestingCounter LoadingCounter(FLoading);
    SessionTree->clear();

    QTreeWidgetItem * Node = new QTreeWidgetItem();
    Node->setText(0, ToQString(LoadStr(LOGIN_NEW_SITE_NODE)));
    Node->setData(0, NodeKindRole, nkNewSite);
    SetNodeSession(Node, FNewSiteData);
    SessionTree->addTopLevelItem(Node);
    UpdateNodeIcon(Node);

    DebugAssert(StoredSessions != NULL);
    for (int Index = 0; Index < StoredSessions->Count; Index++)
    {
      AddSession(StoredSessions->Sessions[Index]);
    }
    Select(GetNewSiteNode());
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLoginDialog::ResetNewSiteData()
{
  if (DebugAlwaysTrue(StoredSessions != NULL))
  {
    FNewSiteData->CopyData(StoredSessions->DefaultSettings);
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::NewSite()
{
  QTreeWidgetItem * NewSiteNode = GetNewSiteNode();
  if (Selected() != NewSiteNode)
  {
    // loads the contents
    Select(NewSiteNode);
  }
  else
  {
    LoadContents();
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::Default()
{
  ResetNewSiteData();
  NewSite();
}
//---------------------------------------------------------------------------
void TLoginDialog::LoadContents()
{
  bool UseContentsPanel = false;
  QTreeWidgetItem * Node = Selected();
  if (IsSessionNode(Node))
  {
    LoadSession(GetSelectedSession());
  }
  else if (IsFolderOrWorkspaceNode(Node))
  {
    UnicodeString NodePath = SessionNodePath(Node);
    ContentsNameEdit->setText(ToQString(NodePath));
    UnicodeString CommonRoot = IsFolderNode(Node) ? UnixIncludeTrailingBackslash(NodePath) : UnicodeString();
    ContentsMemo->setPlainText(ToQString(GetFolderOrWorkspaceContents(Node, CommonRoot)));
    ContentsGroupBox->setTitle(ToQString(LoadStr(IsFolderNode(Node) ? LOGIN_SITE_FOLDER_CAPTION : LOGIN_WORKSPACE_CAPTION)));
    UseContentsPanel = true;
  }
  SitePanel->setVisible(!UseContentsPanel);
  ContentsGroupBox->setVisible(UseContentsPanel);
}
//---------------------------------------------------------------------------
void TLoginDialog::LoadSession(TSessionData * SessionData)
{
  {
    // The controls change without the reactions to the user's changes, as
    // in the VCL, where programmatic changes of combo boxes are not
    // notified
    TAutoNestingCounter NoUpdateCounter(FNoUpdate);
    QSignalBlocker BlockPort(PortNumberEdit);
    QSignalBlocker BlockFtps(FtpsCombo);
    QSignalBlocker BlockWebDavs(WebDavsCombo);
    QSignalBlocker BlockProtocol(TransferProtocolCombo);
    QSignalBlocker BlockS3Env(S3CredentialsEnvCheck3);
    QSignalBlocker BlockS3Profile(S3ProfileCombo);

    PortNumberEdit->setValue(SessionData->PortNumber);

    int FtpsIndex = FtpsToIndex(SessionData->Ftps);
    FtpsCombo->setCurrentIndex(FtpsIndex);
    WebDavsCombo->setCurrentIndex(std::min(FtpsIndex, WebDavsCombo->count() - 1));
    EncryptionView->setText(FtpsCombo->currentText());

    bool AllowScpFallback;
    TransferProtocolCombo->setCurrentIndex(FSProtocolToIndex(SessionData->FSProtocol, AllowScpFallback));
    TransferProtocolView->setText(TransferProtocolCombo->currentText());

    HostNameEdit->setText(ToQString(SessionData->HostName));
    UserNameEdit->setText(ToQString(SessionData->UserName));

    bool Editable = IsEditable();
    if (Editable)
    {
      PasswordEdit->setText(ToQString(NormalizeString(SessionData->Password)));
    }
    else
    {
      PasswordEdit->setText(SessionData->HasPassword() ? QString(16, QLatin1Char('?')) : QString());
    }

    S3CredentialsEnvCheck3->setChecked(SessionData->S3CredentialsEnv);
    S3ProfileCombo->setEditText(ToQString(DefaultStr(SessionData->S3Profile, GetS3GeneralName())));
  }
  UpdateS3Credentials();

  NoteGroup->setVisible(!Trim(SessionData->Note).IsEmpty());
  NoteMemo->setPlainText(ToQString(SessionData->Note));

  FDefaultPort = DefaultPort();
  FUpdatePortWithProtocol = true;

  if (SessionData != FSessionData)
  {
    InvalidateSessionData();
    // The settings of the session that this dialog does not show are
    // copied only when they can be changed
    if (IsEditable())
    {
      FSessionData = new TSessionData(L"");
      FSessionData->Assign(SessionData);
    }
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TLoginDialog::SaveSession(TSessionData * SessionData)
{
  if (DebugAlwaysTrue(FSessionData != NULL))
  {
    SessionData->Assign(FSessionData);
  }

  SessionData->FSProtocol = GetFSProtocol(true);

  if (SessionData->FSProtocol == fsS3)
  {
    SessionData->S3CredentialsEnv = S3CredentialsEnvCheck3->isChecked();
    SessionData->S3Profile = GetS3Profile();
  }

  if (SessionData->HasAutoCredentials())
  {
    SessionData->UserName = UnicodeString();
    SessionData->Password = UnicodeString();
    SessionData->S3SessionToken = UnicodeString();
  }
  else
  {
    SessionData->UserName = ToUnicodeString(UserNameEdit->text()).Trim();
    SessionData->Password = ToUnicodeString(PasswordEdit->text());
  }

  SessionData->PortNumber = PortNumberEdit->value();
  // After the user name, as the host name may be user@host
  SessionData->HostName = ToUnicodeString(HostNameEdit->text()).Trim();
  SessionData->Ftps = GetFtps();

  TSessionData * EditingSessionData = GetEditingSessionData();
  SessionData->Name =
    (EditingSessionData != NULL) ? EditingSessionData->Name :
      (FNewSiteKeepName ? SessionData->Name : SessionData->DefaultSessionName);
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsEditable()
{
  return IsNewSiteNode(Selected()) || FEditing;
}
//---------------------------------------------------------------------------
TSessionData * TLoginDialog::GetSelectedSession()
{
  QTreeWidgetItem * Node = Selected();
  return IsSessionNode(Node) ? GetNodeSession(Node) : NULL;
}
//---------------------------------------------------------------------------
TSessionData * TLoginDialog::GetSessionData()
{
  TSessionData * Result = GetSelectedSession();
  if (Result == FNewSiteData)
  {
    SaveSession(FNewSiteData);
  }
  return Result;
}
//---------------------------------------------------------------------------
TSessionData * TLoginDialog::GetEditingSessionData()
{
  return FEditing ? DebugNotNull(GetSelectedSession()) : NULL;
}
//---------------------------------------------------------------------------
UnicodeString TLoginDialog::GetS3GeneralName()
{
  return LoadStr(LOGIN_S3_GENERAL_CREDENTIALS);
}
//---------------------------------------------------------------------------
UnicodeString TLoginDialog::GetS3Profile()
{
  UnicodeString Result;
  UnicodeString Text = ToUnicodeString(S3ProfileCombo->currentText());
  if (Text != GetS3GeneralName())
  {
    Result = Text;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TLoginDialog::LoadS3Profiles()
{
  std::unique_ptr<TStringList> Items(new TStringList());
  std::unique_ptr<TStrings> Profiles(GetS3Profiles());
  Items->AddStrings(Profiles.get());
  Items->Sort();
  Items->Insert(0, GetS3GeneralName());
  QSignalBlocker Block(S3ProfileCombo);
  QString Text = S3ProfileCombo->currentText();
  for (int Index = 0; Index < Items->Count; Index++)
  {
    S3ProfileCombo->addItem(ToQString(Items->Strings[Index]));
  }
  S3ProfileCombo->setEditText(Text);
}
//---------------------------------------------------------------------------
void TLoginDialog::UpdateS3Credentials()
{
  if (S3CredentialsEnvCheck3->isChecked())
  {
    UnicodeString S3Profile = GetS3Profile();
    UserNameEdit->setText(ToQString(S3EnvUserName(S3Profile)));
    PasswordEdit->setText(ToQString(S3EnvPassword(S3Profile)));
    // Kept when the check box is unchecked and checked again, as the user
    // name and the password are
    if (FSessionData != NULL)
    {
      FSessionData->S3SessionToken = S3EnvSessionToken(S3Profile);
      FSessionData->S3RoleArn = S3EnvRoleArn(S3Profile);
    }
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::UpdateControls()
{
  bool Editable = IsEditable();

  TFSProtocol FSProtocol = GetFSProtocol(false);
  bool SshProtocol = IsSshProtocol(FSProtocol);
  bool FtpProtocol = (FSProtocol == fsFTP);
  bool WebDavProtocol = (FSProtocol == fsWebDAV);
  bool S3Protocol = (FSProtocol == fsS3);

  // session
  FtpsCombo->setVisible(Editable && FtpProtocol);
  FtpsLabel->setVisible(FtpProtocol);
  WebDavsCombo->setVisible(Editable && (WebDavProtocol || S3Protocol));
  WebDavsLabel->setVisible(WebDavProtocol || S3Protocol);
  EncryptionView->setVisible(!Editable && (FtpProtocol || WebDavProtocol || S3Protocol));

  BasicFtpPanel->setVisible(FtpProtocol && Editable);
  BasicS3Panel->setVisible(S3Protocol && Editable);
  if (BasicS3Panel->isVisibleTo(this) && (S3ProfileCombo->count() == 0))
  {
    LoadS3Profiles();
  }
  {
    QSignalBlocker Block(AnonymousLoginCheck);
    AnonymousLoginCheck->setChecked(
      SameText(ToUnicodeString(UserNameEdit->text()), AnonymousUserName) &&
      SameText(ToUnicodeString(PasswordEdit->text()), AnonymousPassword));
  }
  TransferProtocolCombo->setVisible(Editable);
  TransferProtocolView->setVisible(!Editable);
  HostNameEdit->setReadOnly(!Editable);
  PortNumberEdit->setReadOnly(!Editable);
  PortNumberEdit->setButtonSymbols(Editable ? QAbstractSpinBox::UpDownArrows : QAbstractSpinBox::NoButtons);
  bool S3CredentialsEnv = S3Protocol && S3CredentialsEnvCheck3->isChecked();
  bool NoAuth =
    Editable && (FSessionData != NULL) &&
    ((SshProtocol && FSessionData->SshNoUserAuth) || S3CredentialsEnv);
  UserNameEdit->setReadOnly(!Editable);
  UserNameEdit->setEnabled(!NoAuth);
  UserNameLabel->setEnabled(!NoAuth);
  PasswordEdit->setReadOnly(!Editable);
  PasswordEdit->setEnabled(!NoAuth);
  PasswordLabel->setEnabled(!NoAuth);
  UserNameLabel->setText(S3Protocol ? ToQString(LoadStr(S3_ACCESS_KEY_ID_PROMPT)) : FUserNameLabel);
  PasswordLabel->setText(S3Protocol ? ToQString(LoadStr(S3_SECRET_ACCESS_KEY_PROMPT)) : FPasswordLabel);
  S3ProfileCombo->setEnabled(S3CredentialsEnv);

  ManageButton->setEnabled(!FEditing);
  CloseButton->setEnabled(!FEditing);

  UpdateActions();

  bool Renaming = (FRenamingItem != nullptr);
  LoginButton->setDefault(!FEditing && !Renaming && !IsCloneToNewSiteDefault());
  SaveButton->setDefault(FEditing);

  // The buttons of the actions are shown when they can be used
  SaveButton->setVisible(SaveSessionAction->isEnabled());
  EditButton->setVisible(EditSessionAction->isEnabled());
  EditCancelButton->setVisible(EditCancelAction->isEnabled());
  LoginButton->setEnabled(LoginAction->isEnabled());
  LoginButton->setText(LoginAction->text());
  SaveAsSessionAction->setVisible(FEditing);
}
//---------------------------------------------------------------------------
void TLoginDialog::UpdateActions()
{
  QTreeWidgetItem * Node = Selected();
  bool SiteSelected = IsSiteNode(Node);
  bool FolderOrWorkspaceSelected = IsFolderOrWorkspaceNode(Node);
  bool Editable = IsEditable();
  bool Special = SiteSelected && GetSelectedSession()->Special;

  EditSessionAction->setEnabled(SiteSelected && !FEditing);
  CloneToNewSiteAction->setEnabled(SiteSelected && !FEditing);
  EditCancelAction->setEnabled(FEditing);
  DeleteSessionAction->setEnabled((SiteSelected && !Special && !FEditing) || FolderOrWorkspaceSelected);
  RenameSessionAction->setEnabled((SiteSelected && !Special && !FEditing) || FolderOrWorkspaceSelected);
  LoginAction->setEnabled(FNeedTerminal ? IsSiteAndCanOpen() : CanOpen());
  LoginAction->setText(ToQString(LoadStr(FolderOrWorkspaceSelected ? LOGIN_OPEN : LOGIN_LOGIN)));
  SaveSessionAction->setEnabled(Editable);
  SaveAsSessionAction->setEnabled(Editable);
  NewSessionFolderAction->setEnabled(!FEditing);
  SetDefaultSessionAction->setEnabled(IsNewSiteNode(Node));
  ResetNewSessionAction->setEnabled(IsNewSiteNode(Node));
  UnicodeString ClipboardUrl;
  PasteUrlAction->setEnabled(
    TextFromClipboard(ClipboardUrl, true) && !ClipboardUrl.IsEmpty() &&
    FLAGSET(StoredSessions->GetUrlInfo(ClipboardUrl), piProtocolDefined));
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsCloneToNewSiteDefault()
{
  return
    !FEditing && (FRenamingItem == nullptr) && IsSiteNode(Selected()) &&
    !StoredSessions->CanOpen(GetSessionData());
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsSiteAndCanOpen()
{
  TSessionData * Data = GetSessionData();
  return (Data != NULL) && StoredSessions->CanOpen(Data) && !FEditing;
}
//---------------------------------------------------------------------------
bool TLoginDialog::IsFolderOrWorkspaceAndCanOpen()
{
  return IsFolderOrWorkspaceNode(Selected()) && HasNodeAnySession(Selected(), true);
}
//---------------------------------------------------------------------------
bool TLoginDialog::CanOpen()
{
  return IsSiteAndCanOpen() || IsFolderOrWorkspaceAndCanOpen();
}
//---------------------------------------------------------------------------
void TLoginDialog::DataChange()
{
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLoginDialog::SessionTreeCurrentItemChanged(QTreeWidgetItem * Current, QTreeWidgetItem * Previous)
{
  if (FLoading == 0)
  {
    if (FEditing && (Previous != nullptr))
    {
      // Back to the edited site while the user decides about the changes
      {
        TAutoNestingCounter LoadingCounter(FLoading);
        SessionTree->setCurrentItem(Previous);
      }
      if (EnsureNotEditing() && (Current != Selected()))
      {
        Select(Current);
      }
    }
    else
    {
      PersistNewSiteIfNeeded(Previous);
      LoadContents();
      UpdateControls();
    }
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::PersistNewSiteIfNeeded(QTreeWidgetItem * Node)
{
  if (IsNewSiteNode(Node))
  {
    SaveSession(FNewSiteData);
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::SessionTreeActivated(QTreeWidgetItem * Node)
{
  if ((Node == Selected()) && EnsureNotEditing())
  {
    if (IsCloneToNewSiteDefault())
    {
      CloneToNewSite();
    }
    else if (CanOpen() && (IsSessionNode(Node) || IsWorkspaceNode(Node)))
    {
      LoginActionExecute();
    }
  }
}
//---------------------------------------------------------------------------
bool TLoginDialog::EnsureNotEditing()
{
  bool Result = !FEditing;
  if (!Result)
  {
    UnicodeString Message = MainInstructions(LoadStr(LOGIN_SAVE_EDITING));
    unsigned int Answer = MessageDialog(Message, qtConfirmation, qaYes | qaNo | qaCancel);
    switch (Answer)
    {
      case qaYes:
        SaveAsSession(false);
        Result = true;
        break;

      case qaNo:
        CancelEditing();
        LoadContents();
        UpdateControls();
        Result = true;
        break;

      default:
        break;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TLoginDialog::CancelEditing()
{
  FEditing = false;
}
//---------------------------------------------------------------------------
void TLoginDialog::EditSession()
{
  HostNameEdit->setFocus();
}
//---------------------------------------------------------------------------
void TLoginDialog::EditSessionActionExecute()
{
  if (DebugAlwaysTrue(GetSelectedSession() != NULL))
  {
    FEditing = true;
    EditSession();
    // Reloads the site, with its password
    LoadContents();
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::SaveAsSession(bool ForceDialog)
{
  ParseHostName();

  std::unique_ptr<TSessionData> SessionData(new TSessionData(L""));
  SaveSession(SessionData.get());

  TSessionData * EditingSessionData = GetEditingSessionData();

  // Empty folders are known to this dialog only
  std::unique_ptr<TStrings> NewFolders(new TStringList());
  for (QTreeWidgetItemIterator It(SessionTree); *It != nullptr; ++It)
  {
    if (IsFolderNode(*It) && ((*It)->childCount() == 0))
    {
      NewFolders->Add(SessionNodePath(*It));
    }
  }

  TSessionData * NewSession = DoSaveSession(SessionData.get(), EditingSessionData, ForceDialog, NewFolders.get());
  if (NewSession != NULL)
  {
    QTreeWidgetItem * ParentNode = AddSessionPath(UnixExtractFilePath(NewSession->SessionName), false, false);
    CheckIsSessionFolder(ParentNode);

    QTreeWidgetItem * Node = FindSessionNode(NewSession, false);
    if (Node == nullptr)
    {
      Node = AddSession(NewSession);
    }

    FEditing = false;
    if (Selected() != Node)
    {
      Select(Node);
    }
    SessionTree->setFocus();

    // Shows the site as saved (the password only if it was saved)
    LoadContents();
    UpdateControls();

    ResetNewSiteData();
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::SaveSessionActionExecute()
{
  // For the new site, saving is "save as"
  SaveAsSession(IsNewSiteNode(Selected()));
}
//---------------------------------------------------------------------------
void TLoginDialog::SaveAsSessionActionExecute()
{
  SaveAsSession(true);
}
//---------------------------------------------------------------------------
void TLoginDialog::DeleteSessionActionExecute()
{
  QTreeWidgetItem * Node = Selected();
  if (IsSiteNode(Node))
  {
    TSessionData * Session = GetSelectedSession();
    UnicodeString Message = MainInstructions(FMTLOAD(CONFIRM_DELETE_SESSION, (Session->SessionName)));
    if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) == qaOK)
    {
      Session->Remove();
      StoredSessions->Remove(Session);
      delete Node;
    }
  }
  else if (IsFolderOrWorkspaceNode(Node))
  {
    int Sessions = 0;
    for (QTreeWidgetItemIterator It(Node); (*It != nullptr) && ((*It == Node) || (*It)->parent() != nullptr); ++It)
    {
      QTreeWidgetItem * ANode = *It;
      bool InNode = false;
      for (QTreeWidgetItem * Parent = ANode->parent(); !InNode && (Parent != nullptr); Parent = Parent->parent())
      {
        InNode = (Parent == Node);
      }
      if (InNode && IsSiteNode(ANode))
      {
        TSessionData * Session = GetNodeSession(ANode);
        if (Session->Special)
        {
          Select(ANode);
          throw Exception(FMTLOAD(LOGIN_DELETE_SPECIAL_SESSION, (Session->SessionName)));
        }
        Sessions++;
      }
    }

    UnicodeString Path = SessionNodePath(Node);
    int Prompt = IsFolderNode(Node) ? LOGIN_DELETE_SESSION_FOLDER : LOGIN_DELETE_WORKSPACE;
    if ((Sessions == 0) ||
        (MessageDialog(MainInstructions(FMTLOAD(Prompt, (Path, Sessions))), qtConfirmation, qaOK | qaCancel) == qaOK))
    {
      std::vector<QTreeWidgetItem *> Nodes;
      for (QTreeWidgetItemIterator It(Node); *It != nullptr; ++It)
      {
        bool InNode = false;
        for (QTreeWidgetItem * Parent = (*It)->parent(); !InNode && (Parent != nullptr); Parent = Parent->parent())
        {
          InNode = (Parent == Node);
        }
        if (InNode && IsSiteNode(*It))
        {
          Nodes.push_back(*It);
        }
      }
      for (QTreeWidgetItem * ANode : Nodes)
      {
        TSessionData * Session = GetNodeSession(ANode);
        Session->Remove();
        StoredSessions->Remove(Session);
      }
      delete Node;
    }
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLoginDialog::RenameSessionActionExecute()
{
  QTreeWidgetItem * Node = Selected();
  if (Node != nullptr)
  {
    SessionTree->setFocus();
    FRenamingItem = Node;
    FRenamingText = Node->text(0);
    Node->setFlags(Node->flags() | Qt::ItemIsEditable);
    SessionTree->editItem(Node);
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::SessionTreeItemChanged(QTreeWidgetItem * Node)
{
  if ((Node == FRenamingItem) && (Node->text(0) != FRenamingText))
  {
    QString OldText = FRenamingText;
    QString NewText = Node->text(0);
    // The edit is committed: the item is not renamed again
    FRenamingItem = nullptr;
    Node->setFlags(Node->flags() & ~Qt::ItemIsEditable);
    {
      QSignalBlocker Block(SessionTree);
      Node->setText(0, OldText);
    }
    try
    {
      SessionTreeEdited(Node, ToUnicodeString(NewText));
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
    }
    LoadContents();
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::SessionTreeEdited(QTreeWidgetItem * Node, const UnicodeString & S)
{
  if ((NodeText(Node) != S) && !S.IsEmpty())
  {
    TSessionData::ValidateName(S);
    QTreeWidgetItem * Parent = Node->parent();
    if (IsSiteNode(Node))
    {
      TSessionData * Session = GetNodeSession(Node);
      UnicodeString Path = UnixExtractFilePath(Session->Name) + S;

      SessionNameValidate(Path, Session->Name);

      // removes from the storage
      Session->Remove();

      TSessionData * NewSession = StoredSessions->NewSession(Path, Session);
      // modified only, explicit
      StoredSessions->Save(false, true);
      // The same session, when only the letter case changed
      if (Session != NewSession)
      {
        // The overwritten site
        QTreeWidgetItem * Overwritten = FindSessionNode(NewSession, false);
        if (Overwritten != nullptr)
        {
          delete Overwritten;
        }
        SetNodeSession(Node, NewSession);
        StoredSessions->Remove(Session);
      }
    }
    else
    {
      CheckDuplicateFolder(Parent, S, Node);

      UnicodeString ParentPath = UnixIncludeTrailingBackslash(SessionNodePath(Parent));
      UnicodeString OldRoot = ParentPath + NodeText(Node);
      UnicodeString NewRoot = ParentPath + S;

      bool AnySession = false;
      for (QTreeWidgetItemIterator It(Node); *It != nullptr; ++It)
      {
        QTreeWidgetItem * ANode = *It;
        bool InNode = false;
        for (QTreeWidgetItem * AParent = ANode->parent(); !InNode && (AParent != nullptr); AParent = AParent->parent())
        {
          InNode = (AParent == Node);
        }
        if (InNode && IsSiteNode(ANode))
        {
          AnySession = true;
          TSessionData * Session = GetNodeSession(ANode);

          Session->Remove();

          UnicodeString Path = Session->Name;
          DebugAssert(Path.SubString(1, OldRoot.Length()) == OldRoot);
          Path.Delete(1, OldRoot.Length());
          Path.Insert(NewRoot, 1);

          TSessionData * NewSession = StoredSessions->NewSession(Path, Session);
          if (NewSession != Session)
          {
            SetNodeSession(ANode, NewSession);
            StoredSessions->Remove(Session);
          }
        }
      }

      if (AnySession)
      {
        // modified only, explicit
        StoredSessions->Save(false, true);
      }
    }

    // In the sorted position
    bool Expanded = Node->isExpanded();
    {
      TAutoNestingCounter LoadingCounter(FLoading);
      QSignalBlocker Block(SessionTree);
      if (Parent == nullptr)
      {
        SessionTree->takeTopLevelItem(SessionTree->indexOfTopLevelItem(Node));
      }
      else
      {
        Parent->removeChild(Node);
      }
      Node->setText(0, ToQString(S));
      InsertNode(Parent, Node);
      Node->setExpanded(Expanded);
    }
    Select(Node);
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::NewSessionFolderActionExecute()
{
  UnicodeString Name =
    UnixIncludeTrailingBackslash(SessionNodePath(SessionFolderNode(Selected()))) + LoadStr(NEW_FOLDER);
  if (InputDialog(LoadStr(LOGIN_NEW_SESSION_FOLDER_CAPTION), LoadStr(LOGIN_NEW_SESSION_FOLDER_PROMPT), Name, NULL, true))
  {
    Name = UnixExcludeTrailingBackslash(Name);
    if (!Name.IsEmpty())
    {
      QTreeWidgetItem * Parent = AddSessionPath(UnixExtractFilePath(Name), true, false);
      CheckIsSessionFolder(Parent);
      CheckDuplicateFolder(Parent, UnixExtractFileName(Name), nullptr);

      QTreeWidgetItem * Node = AddSessionPath(Name, true, false);
      Select(Node);
    }
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::CloneToNewSite()
{
  FNewSiteData->CopyData(GetSelectedSession());
  FNewSiteData->MakeUniqueIn(StoredSessions);
  FNewSiteKeepName = true;
  NewSite();
  EditSession();
}
//---------------------------------------------------------------------------
void TLoginDialog::EditCancelActionExecute()
{
  CancelEditing();
  // Back to the saved settings
  LoadContents();
  UpdateControls();
  SessionTree->setFocus();
}
//---------------------------------------------------------------------------
void TLoginDialog::LoginActionExecute()
{
  accept();
}
//---------------------------------------------------------------------------
void TLoginDialog::SetDefaultSessionActionExecute()
{
  UnicodeString Message = MainInstructions(LoadStr(SET_DEFAULT_SESSION_SETTINGS));
  if (MessageDialog(Message, qtConfirmation, qaOK | qaCancel) == qaOK)
  {
    std::unique_ptr<TSessionData> SessionData(new TSessionData(L""));
    SaveSession(SessionData.get());
    if (!Configuration->DisablePasswordStoring && SessionData->HasAnyPassword() && AppConfiguration->UseMasterPassword)
    {
      AppConfiguration->AskForMasterPasswordIfNotSet();
    }
    StoredSessions->DefaultSettings = SessionData.get();

    Default();
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::ResetNewSessionActionExecute()
{
  Default();
}
//---------------------------------------------------------------------------
void TLoginDialog::ParseUrl(const UnicodeString & Url)
{
  std::unique_ptr<TSessionData> SessionData(new TSessionData(L""));
  SaveSession(SessionData.get());

  // Otherwise the colons would be taken as the syntax of the URL
  if (IsIPv6Literal(Url))
  {
    UnicodeString IPv6Literal = Url;
    if (HasIP6LiteralBrackets(IPv6Literal))
    {
      IPv6Literal = StripIP6LiteralBrackets(IPv6Literal);
    }
    SessionData->HostName = IPv6Literal;
  }
  else
  {
    int ParsedInfo;
    // Not as a stored site
    SessionData->ParseUrl(Url, NULL, NULL, ParsedInfo, NULL, NULL, pufPreferProtocol);
    SessionData->RequireDirectories = false;
    if (FLAGSET(ParsedInfo, piUnsafeSettings) &&
        (MessageDialog(LoadStr(UNSAFE_SESSION), qtConfirmation, qaOK | qaCancel) != qaOK))
    {
      Abort();
    }
  }

  LoadSession(SessionData.get());
}
//---------------------------------------------------------------------------
void TLoginDialog::ParseHostName()
{
  UnicodeString HostName = ToUnicodeString(HostNameEdit->text()).Trim();
  if (IsEditable() && !HostName.IsEmpty())
  {
    std::unique_ptr<TSessionData> SessionData(new TSessionData(EmptyStr));
    int ParsedInfo;
    SessionData->ParseUrl(HostName, NULL, NULL, ParsedInfo, NULL, NULL, pufPreferProtocol);
    SessionData->RequireDirectories = false;
    std::unique_ptr<TSessionData> HostNameSessionData(new TSessionData(EmptyStr));
    HostNameSessionData->HostName = HostName;
    // The host name is a URL, or user@host
    if ((HostNameSessionData->HostName != HostName) ||
        !SessionData->IsSameDecrypted(HostNameSessionData.get()))
    {
      ParseUrl(HostName);
    }
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::PasteUrlActionExecute()
{
  UnicodeString ClipboardUrl;
  if (TextFromClipboard(ClipboardUrl, true) && !ClipboardUrl.IsEmpty())
  {
    if (!IsEditable())
    {
      Select(GetNewSiteNode());
    }
    if (DebugAlwaysTrue(IsEditable()))
    {
      ParseUrl(ClipboardUrl);
    }
    HostNameEdit->setFocus();
    HostNameEdit->selectAll();
  }
}
//---------------------------------------------------------------------------
int TLoginDialog::FSProtocolToIndex(TFSProtocol FSProtocol, bool & AllowScpFallback)
{
  if (FSProtocol == fsSFTP)
  {
    AllowScpFallback = true;
    bool Dummy;
    return FSProtocolToIndex(fsSFTPonly, Dummy);
  }
  else
  {
    AllowScpFallback = false;
    for (int Index = 0; Index < static_cast<int>(std::size(FSOrder)); Index++)
    {
      if (FSOrder[Index] == FSProtocol)
      {
        return Index;
      }
    }
    // SFTP is always present
    return FSProtocolToIndex(fsSFTP, AllowScpFallback);
  }
}
//---------------------------------------------------------------------------
TFSProtocol TLoginDialog::IndexToFSProtocol(int Index, bool AllowScpFallback)
{
  bool InBounds = (Index >= 0) && (Index < static_cast<int>(std::size(FSOrder)));
  TFSProtocol Result = fsSFTP;
  if (InBounds)
  {
    Result = FSOrder[Index];
    if ((Result == fsSFTPonly) && AllowScpFallback)
    {
      Result = fsSFTP;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int TLoginDialog::FtpsToIndex(TFtps Ftps)
{
  switch (Ftps)
  {
    default:
      DebugFail();
    case ftpsNone:
      return 0;

    case ftpsImplicit:
      return 1;

    case ftpsExplicitTls:
    case ftpsExplicitSsl:
      return 2;
  }
}
//---------------------------------------------------------------------------
TFtps TLoginDialog::GetFtps()
{
  TFSProtocol FSProtocol = GetFSProtocol(false);
  int Index =
    (((FSProtocol == fsWebDAV) || (FSProtocol == fsS3)) ? WebDavsCombo->currentIndex() : FtpsCombo->currentIndex());
  TFtps Ftps;
  switch (Index)
  {
    default:
      DebugFail();
    case 0:
      Ftps = ftpsNone;
      break;

    case 1:
      Ftps = ftpsImplicit;
      break;

    case 2:
      Ftps = ftpsExplicitTls;
      break;
  }
  return Ftps;
}
//---------------------------------------------------------------------------
TFSProtocol TLoginDialog::GetFSProtocol(bool RequireScpFallbackDistinction)
{
  bool AllowScpFallback = false;
  if (RequireScpFallbackDistinction && DebugAlwaysTrue(FSessionData != NULL))
  {
    FSProtocolToIndex(FSessionData->FSProtocol, AllowScpFallback);
  }
  return IndexToFSProtocol(TransferProtocolCombo->currentIndex(), AllowScpFallback);
}
//---------------------------------------------------------------------------
int TLoginDialog::DefaultPort()
{
  return ::DefaultPort(GetFSProtocol(false), GetFtps());
}
//---------------------------------------------------------------------------
void TLoginDialog::TransferProtocolComboChange()
{
  if (FNoUpdate == 0)
  {
    if (GetFSProtocol(false) == fsS3)
    {
      QSignalBlocker Block(WebDavsCombo);
      WebDavsCombo->setCurrentIndex(FtpsToIndex(ftpsImplicit));
      HostNameEdit->setText(ToQString(S3HostName));
    }
    else
    {
      try
      {
        UnicodeString S3Profile = GetS3Profile();
        if (ToUnicodeString(HostNameEdit->text()) == S3HostName)
        {
          HostNameEdit->clear();
        }
        if (ToUnicodeString(UserNameEdit->text()) == S3EnvUserName(S3Profile, NULL, true))
        {
          UserNameEdit->clear();
        }
        if (ToUnicodeString(PasswordEdit->text()) == S3EnvPassword(S3Profile, NULL, true))
        {
          PasswordEdit->clear();
        }
        if (FSessionData != NULL)
        {
          if (FSessionData->S3SessionToken == S3EnvSessionToken(S3Profile, NULL, true))
          {
            FSessionData->S3SessionToken = EmptyStr;
          }
          if (FSessionData->S3RoleArn == S3EnvRoleArn(S3Profile, NULL, true))
          {
            FSessionData->S3RoleArn = EmptyStr;
          }
        }
      }
      catch (...)
      {
        // noop
      }
    }

    QSignalBlocker Block(S3CredentialsEnvCheck3);
    S3CredentialsEnvCheck3->setChecked(false);
  }

  UpdatePortWithProtocol();
  DataChange();
}
//---------------------------------------------------------------------------
void TLoginDialog::UpdatePortWithProtocol()
{
  int ADefaultPort = DefaultPort();
  if ((FNoUpdate == 0) && FUpdatePortWithProtocol)
  {
    TAutoNestingCounter NoUpdateCounter(FNoUpdate);
    if (PortNumberEdit->value() == FDefaultPort)
    {
      PortNumberEdit->setValue(ADefaultPort);
    }
  }
  FDefaultPort = ADefaultPort;
}
//---------------------------------------------------------------------------
void TLoginDialog::EncryptionComboChange()
{
  UpdatePortWithProtocol();
  DataChange();
}
//---------------------------------------------------------------------------
void TLoginDialog::PortNumberEditChange()
{
  if (FNoUpdate == 0)
  {
    bool WellKnownPort = false;
    TFSProtocol FSProtocol = TFSProtocol();
    TFtps Ftps = ftpsNone;

    // For ambiguous port numbers, the current protocol is kept (HTTPS vs
    // S3, FTP vs FTPES)
    TFSProtocol CurrentFSProtocol = GetFSProtocol(false);

    int PortNumber = PortNumberEdit->value();
    if (PortNumber == SshPortNumber)
    {
      FSProtocol = fsSFTP;
      WellKnownPort = true;
    }
    else if (PortNumber == FtpPortNumber)
    {
      FSProtocol = fsFTP;
      if ((CurrentFSProtocol == FSProtocol) && (GetFtps() == ftpsExplicitTls))
      {
        Ftps = ftpsExplicitTls;
      }
      WellKnownPort = true;
    }
    else if (PortNumber == FtpsImplicitPortNumber)
    {
      FSProtocol = fsFTP;
      Ftps = ftpsImplicit;
      WellKnownPort = true;
    }
    else if (PortNumber == HTTPPortNumber)
    {
      FSProtocol = (CurrentFSProtocol == fsS3) ? fsS3 : fsWebDAV;
      WellKnownPort = true;
    }
    else if (PortNumber == HTTPSPortNumber)
    {
      FSProtocol = (CurrentFSProtocol == fsS3) ? fsS3 : fsWebDAV;
      Ftps = ftpsImplicit;
      WellKnownPort = true;
    }

    if (WellKnownPort)
    {
      bool AllowScpFallback;
      int ProtocolIndex = FSProtocolToIndex(FSProtocol, AllowScpFallback);
      if ((TransferProtocolCombo->currentIndex() == ProtocolIndex) && (GetFtps() == Ftps))
      {
        FUpdatePortWithProtocol = true;
      }
      else
      {
        FUpdatePortWithProtocol = false;

        TAutoNestingCounter NoUpdateCounter(FNoUpdate);
        QSignalBlocker BlockProtocol(TransferProtocolCombo);
        QSignalBlocker BlockFtps(FtpsCombo);
        QSignalBlocker BlockWebDavs(WebDavsCombo);
        TransferProtocolCombo->setCurrentIndex(ProtocolIndex);
        int FtpsIndex = FtpsToIndex(Ftps);
        FtpsCombo->setCurrentIndex(FtpsIndex);
        WebDavsCombo->setCurrentIndex(std::min(FtpsIndex, WebDavsCombo->count() - 1));
        FDefaultPort = DefaultPort();
      }
    }
  }

  DataChange();
}
//---------------------------------------------------------------------------
void TLoginDialog::AnonymousLoginCheckClick()
{
  if (AnonymousLoginCheck->isChecked())
  {
    UserNameEdit->setText(ToQString(AnonymousUserName));
    PasswordEdit->setText(ToQString(AnonymousPassword));
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLoginDialog::S3CredentialsEnvCheck3Click()
{
  if (FNoUpdate == 0)
  {
    UpdateS3Credentials();
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
bool TLoginDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if ((Object == HostNameEdit) && (Event->type() == QEvent::FocusOut))
  {
    try
    {
      ParseHostName();
    }
    catch (Exception & E)
    {
      ShowExtendedException(&E);
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TLoginDialog::keyPressEvent(QKeyEvent * Event)
{
  // Escape cancels the editing of a site, as upstream's EditCancelButton is
  // the cancel button then
  if ((Event->key() == Qt::Key_Escape) && (Event->modifiers() == Qt::NoModifier) && FEditing)
  {
    EditCancelActionExecute();
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
TSessionData * TLoginDialog::CloneSelectedSession()
{
  QTreeWidgetItem * Node = Selected();
  std::unique_ptr<TSessionData> Data2(new TSessionData(L""));
  if (IsSiteNode(Node))
  {
    Data2->Assign(GetNodeSession(Node));
  }
  else if (DebugAlwaysTrue(IsNewSiteNode(Node)))
  {
    TSessionData * Data = GetSessionData();
    Data2->Assign(Data);
    // Ad-hoc sessions have no name, unlike the stored sites
    Data2->Name = L"";
  }
  return Data2.release();
}
//---------------------------------------------------------------------------
void TLoginDialog::SaveDataList(TList * DataList)
{
  DataList->Clear();

  QTreeWidgetItem * Node = Selected();
  if (IsFolderOrWorkspaceNode(Node))
  {
    StoredSessions->GetFolderOrWorkspace(SessionNodePath(Node), DataList);
  }
  else
  {
    ParseHostName();
    DataList->Add(CloneSelectedSession());
  }
}
//---------------------------------------------------------------------------
void TLoginDialog::SaveState()
{
  std::unique_ptr<TStringList> OpenedStoredSessionFolders(CreateSortedStringList());
  for (QTreeWidgetItemIterator It(SessionTree); *It != nullptr; ++It)
  {
    if (IsFolderNode(*It) && (*It)->isExpanded())
    {
      OpenedStoredSessionFolders->Add(SessionNodePath(*It));
    }
  }
  AppConfiguration->OpenedStoredSessionFolders = OpenedStoredSessionFolders->CommaText;

  UnicodeString LastStoredSession = SessionNodePath(Selected());
  if (IsFolderNode(Selected()))
  {
    LastStoredSession = UnixIncludeTrailingBackslash(LastStoredSession);
  }
  AppConfiguration->LastStoredSession = LastStoredSession;
}
//---------------------------------------------------------------------------
void TLoginDialog::LoadState()
{
  std::unique_ptr<TStringList> OpenedStoredSessionFolders(CreateSortedStringList());
  OpenedStoredSessionFolders->CommaText = AppConfiguration->OpenedStoredSessionFolders;
  for (QTreeWidgetItemIterator It(SessionTree); *It != nullptr; ++It)
  {
    if (IsFolderNode(*It) && (OpenedStoredSessionFolders->IndexOf(SessionNodePath(*It)) >= 0))
    {
      (*It)->setExpanded(true);
    }
  }

  UnicodeString LastStoredSession = AppConfiguration->LastStoredSession;
  if (!FForceNewSite && !LastStoredSession.IsEmpty())
  {
    UnicodeString Path = UnixExcludeTrailingBackslash(LastStoredSession);
    bool Folder = (Path != LastStoredSession);

    UnicodeString ParentPath = UnixExtractFilePath(Path);
    QTreeWidgetItem * Parent = nullptr;
    bool Found = true;
    if (!ParentPath.IsEmpty())
    {
      Parent = AddSessionPath(ParentPath, false, false);
      Found = (Parent != nullptr);
    }
    if (Found)
    {
      UnicodeString Name = UnixExtractFileName(Path);
      for (int Index = 0; Index < ChildCount(Parent); Index++)
      {
        QTreeWidgetItem * Node = Child(Parent, Index);
        if (SameText(NodeText(Node), Name) && (IsFolderNode(Node) == Folder) && !IsNewSiteNode(Node))
        {
          Select(Node);
          break;
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TLoginDialog::Execute(TList * DataList)
{
  FDataList = DataList;
  if (DataList->Count > 0)
  {
    TSessionData * SessionData = reinterpret_cast<TSessionData *>(DataList->Items[0]);
    QTreeWidgetItem * Node = FindSessionNode(SessionData, true);
    if (Node != nullptr)
    {
      Select(Node);
    }
    else
    {
      FNewSiteData->CopyData(SessionData);
      FNewSiteData->Special = false;
      if (!FNewSiteData->IsSameDecrypted(StoredSessions->DefaultSettings))
      {
        // Starts with the new site
        FForceNewSite = true;
      }
      NewSite();
    }
  }
  else
  {
    Default();
  }

  LoadState();
  LoadContents();
  UpdateControls();
  if (IsNewSiteNode(Selected()))
  {
    HostNameEdit->setFocus();
  }
  else
  {
    SessionTree->setFocus();
  }

  int AResult = exec();
  bool Result = CanOpen() && (AResult == QDialog::Accepted);
  SaveState();
  return Result;
}
//---------------------------------------------------------------------------
void TLoginDialog::done(int Result)
{
  if (EnsureNotEditing())
  {
    if ((Result == QDialog::Accepted) && CanOpen())
    {
      SaveDataList(FDataList);
    }
    QDialog::done(Result);
  }
}
//---------------------------------------------------------------------------
