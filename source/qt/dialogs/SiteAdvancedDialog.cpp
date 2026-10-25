// The advanced settings of a site (upstream's forms/SiteAdvanced.dfm): the
// pages of the navigation tree that apply to the protocol of the site.
//
// Not ported: the PuTTY's terminal settings of the site (upstream edits them
// in PuTTY through its registry; PuTTY on Linux keeps its sessions in its
// own files) and the generation of keys by PuTTYgen (a command line tool
// on Linux).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Cryptography.h>
#include <PuttyTools.h>
#include <S3FileSystem.h>
#include <SessionData.h>
#include <TerminalManager.h>

#include "TreePagesDialog.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>
#include <QTreeWidget>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
const UnicodeString DefaultRecycleBinPath = L"/tmp";
//---------------------------------------------------------------------------
// The pages, in the order of upstream's (the top level ones and those
// under them)
enum TSitePage
{
  spEnvironment = 1, spDirectories, spRecycleBin, spEncryption, spSftp, spScp, spFtp, spS3, spWebDav,
  spConnection, spProxy, spTunnel, spSsl, spSsh, spKex, spAuthentication, spBugs, spNote
};
//---------------------------------------------------------------------------
void ComboAutoSwitchInitialize(QComboBox * Combo)
{
  Combo->clear();
  Combo->addItem(ToQString(LoadStr(AUTO_SWITCH_AUTO)));
  Combo->addItem(ToQString(LoadStr(AUTO_SWITCH_OFF)));
  Combo->addItem(ToQString(LoadStr(AUTO_SWITCH_ON)));
}
//---------------------------------------------------------------------------
void ComboAutoSwitchLoad(QComboBox * Combo, TAutoSwitch Value)
{
  int Index = 2 - Value;
  Combo->setCurrentIndex(((Index >= 0) && (Index < Combo->count())) ? Index : 0);
}
//---------------------------------------------------------------------------
TAutoSwitch ComboAutoSwitchSave(QComboBox * Combo)
{
  return static_cast<TAutoSwitch>(2 - Combo->currentIndex());
}
//---------------------------------------------------------------------------
void CheckBoxAutoSwitchLoad(QCheckBox * CheckBox, TAutoSwitch Value)
{
  switch (Value)
  {
    case asOn:
      CheckBox->setCheckState(Qt::Checked);
      break;
    case asOff:
      CheckBox->setCheckState(Qt::Unchecked);
      break;
    default:
      CheckBox->setCheckState(Qt::PartiallyChecked);
      break;
  }
}
//---------------------------------------------------------------------------
TAutoSwitch CheckBoxAutoSwitchSave(QCheckBox * CheckBox)
{
  switch (CheckBox->checkState())
  {
    case Qt::Checked:
      return asOn;
    case Qt::Unchecked:
      return asOff;
    default:
      return asAuto;
  }
}
//---------------------------------------------------------------------------
class TSiteAdvancedDialog : public TTreePagesDialog
{
public:
  explicit TSiteAdvancedDialog(QWidget * Parent);
  virtual ~TSiteAdvancedDialog();

  bool Execute(TSessionData * SessionData);

protected:
  virtual void accept() override;
  virtual void UpdateControls() override;

private:
  TSessionData * FSessionData;
  int FNoUpdate;
  TColor FColor;
  UnicodeString FLastPrivateKey;
  bool FKeyHasCertificate;

  // Environment
  QLabel * EOLTypeLabel;
  QComboBox * EOLTypeCombo;
  QLabel * UtfLabel;
  QComboBox * UtfCombo;
  QLabel * TimeDifferenceLabel;
  QSpinBox * TimeDifferenceEdit;
  QLabel * TimeDifferenceHoursLabel;
  QSpinBox * TimeDifferenceMinutesEdit;
  QLabel * TimeDifferenceMinutesLabel;
  QCheckBox * TimeDifferenceAutoCheck;
  QCheckBox * TrimVMSVersionsCheck;
  QGroupBox * DSTModeGroup;
  QRadioButton * DSTModeUnixCheck;
  QRadioButton * DSTModeWinCheck;
  // Directories
  QLineEdit * LocalDirectoryEdit;
  QLineEdit * RemoteDirectoryEdit;
  QCheckBox * UpdateDirectoriesCheck;
  QCheckBox * SynchronizeBrowsingCheck;
  QCheckBox * CacheDirectoriesCheck;
  QCheckBox * CacheDirectoryChangesCheck;
  QCheckBox * PreserveDirectoryChangesCheck;
  QCheckBox * ResolveSymlinksCheck;
  QCheckBox * FollowDirectorySymlinksCheck;
  // Recycle bin
  QCheckBox * DeleteToRecycleBinCheck;
  QCheckBox * OverwrittenToRecycleBinCheck;
  QLabel * RecycleBinPathLabel;
  QLineEdit * RecycleBinPathEdit;
  // Encryption
  QCheckBox * EncryptFilesCheck;
  QGroupBox * EncryptFilesGroup;
  QLineEdit * EncryptKeyPasswordEdit;
  QCheckBox * ShowEncryptionKeyCheck;
  // SFTP
  QComboBox * SFTPMaxVersionCombo;
  QComboBox * SftpServerEdit;
  QCheckBox * AllowScpFallbackCheck;
  QComboBox * SFTPRealPathCombo;
  QCheckBox * UsePosixRenameCheck;
  QComboBox * SFTPBugSymlinkCombo;
  QComboBox * SFTPBugSignedTSCombo;
  // SCP/Shell
  QComboBox * ShellEdit;
  QComboBox * ReturnVarEdit;
  QGroupBox * ScpLsOptionsGroup;
  QComboBox * ListingCommandEdit;
  QCheckBox * IgnoreLsWarningsCheck;
  QCheckBox * SCPLsFullTimeAutoCheck;
  QGroupBox * OtherShellOptionsGroup;
  QCheckBox * LookupUserGroupsCheck;
  QCheckBox * ClearAliasesCheck;
  QCheckBox * UnsetNationalVarsCheck;
  QCheckBox * Scp1CompatibilityCheck;
  // FTP
  QPlainTextEdit * PostLoginCommandsMemo;
  QLabel * FtpListAllLabel;
  QComboBox * FtpListAllCombo;
  QComboBox * FtpUseMlsdCombo;
  QLabel * FtpForcePasvIpLabel;
  QComboBox * FtpForcePasvIpCombo;
  QLineEdit * FtpAccountEdit;
  QComboBox * FtpHostCombo;
  QCheckBox * VMSAllRevisionsCheck;
  // S3
  QComboBox * S3DefaultReqionCombo;
  QComboBox * S3UrlStyleCombo;
  QCheckBox * S3RequesterPaysCheck;
  QLabel * S3SessionTokenLabel;
  QPlainTextEdit * S3SessionTokenMemo;
  QLabel * S3RoleArnLabel;
  QLineEdit * S3RoleArnEdit;
  // WebDAV
  QCheckBox * WebDavLiberalEscapingCheck;
  QCheckBox * WebDavCrossDomainRedirectsCheck;
  // Connection
  QCheckBox * FtpPasvModeCheck;
  QCheckBox * BufferSizeCheck;
  QSpinBox * TimeoutEdit;
  QGroupBox * PingGroup;
  QRadioButton * PingOffButton;
  QRadioButton * PingNullPacketButton;
  QRadioButton * PingDummyCommandButton;
  QLabel * PingIntervalLabel;
  QSpinBox * PingIntervalSecEdit;
  QGroupBox * FtpPingGroup;
  QRadioButton * FtpPingOffButton;
  QRadioButton * FtpPingDummyCommandButton;
  QRadioButton * FtpPingDirectoryListingButton;
  QLabel * FtpPingIntervalLabel;
  QSpinBox * FtpPingIntervalSecEdit;
  QGroupBox * IPvGroup;
  QRadioButton * IPAutoButton;
  QRadioButton * IPv4Button;
  QRadioButton * IPv6Button;
  // Proxy
  QLabel * ProxyMethodLabel;
  QComboBox * SshProxyMethodCombo;
  QComboBox * FtpProxyMethodCombo;
  QComboBox * NeonProxyMethodCombo;
  QLabel * ProxyHostLabel;
  QLineEdit * ProxyHostEdit;
  QLabel * ProxyPortLabel;
  QSpinBox * ProxyPortEdit;
  QLabel * ProxyUsernameLabel;
  QLineEdit * ProxyUsernameEdit;
  QLabel * ProxyPasswordLabel;
  QLineEdit * ProxyPasswordEdit;
  QGroupBox * ProxySettingsGroup;
  QLabel * ProxyTelnetCommandLabel;
  QLineEdit * ProxyTelnetCommandEdit;
  QLabel * ProxyLocalCommandLabel;
  QLineEdit * ProxyLocalCommandEdit;
  QPushButton * ProxyLocalCommandBrowseButton;
  QCheckBox * ProxyLocalhostCheck;
  QComboBox * ProxyDNSCombo;
  // Tunnel
  QCheckBox * TunnelCheck;
  QGroupBox * TunnelSessionGroup;
  QLineEdit * TunnelHostNameEdit;
  QSpinBox * TunnelPortNumberEdit;
  QLineEdit * TunnelUserNameEdit;
  QLineEdit * TunnelPasswordEdit;
  QGroupBox * TunnelOptionsGroup;
  QComboBox * TunnelLocalPortNumberEdit;
  QGroupBox * TunnelAuthenticationParamsGroup;
  QLineEdit * TunnelPrivateKeyEdit3;
  // TLS/SSL
  QComboBox * MinTlsVersionCombo;
  QComboBox * MaxTlsVersionCombo;
  QCheckBox * SslSessionReuseCheck2;
  QGroupBox * TlsAuthenticationGroup;
  QLineEdit * TlsCertificateFileEdit;
  // SSH
  QCheckBox * CompressionCheck;
  QListWidget * CipherListBox;
  QPushButton * CipherUpButton;
  QPushButton * CipherDownButton;
  QCheckBox * Ssh2LegacyDESCheck;
  // Key exchange
  QListWidget * KexListBox;
  QPushButton * KexUpButton;
  QPushButton * KexDownButton;
  QCheckBox * AuthGSSAPIKEXCheck;
  QGroupBox * KexReexchangeGroup;
  QSpinBox * RekeyTimeEdit;
  QLineEdit * RekeyDataEdit;
  // Authentication
  QCheckBox * SshNoUserAuthCheck;
  QGroupBox * AuthenticationGroup;
  QCheckBox * TryAgentCheck2;
  QCheckBox * AuthKICheck;
  QCheckBox * AuthKIPasswordCheck;
  QGroupBox * AuthenticationParamsGroup;
  QCheckBox * AgentFwdCheck;
  QLineEdit * PrivateKeyEdit3;
  QPushButton * PrivateKeyToolsButton;
  QPushButton * PrivateKeyViewButton;
  QLabel * DetachedCertificateLabel;
  QLineEdit * DetachedCertificateEdit;
  QCheckBox * AuthGSSAPICheck3;
  QCheckBox * GSSAPIFwdTGTCheck;
  // Bugs
  QComboBox * BugHMAC2Combo;
  QComboBox * BugDeriveKey2Combo;
  QComboBox * BugRSAPad2Combo;
  QComboBox * BugPKSessID2Combo;
  QComboBox * BugRekey2Combo;
  QComboBox * BugMaxPkt2Combo;
  QComboBox * BugIgnore2Combo;
  QComboBox * BugWinAdjCombo;
  // Note
  QPlainTextEdit * NoteMemo;
  QPushButton * ColorButton;

  void CreateEnvironmentSheets();
  void CreateConnectionSheets();
  void CreateSshSheets();
  QGridLayout * AddGrid(QGroupBox * Group);
  QComboBox * AddGridCombo(QGridLayout * Grid, QWidget * Parent, const char * Name, const QString & Caption,
    const QStringList & Items, bool Editable = false, QLabel ** Label = nullptr);
  QLineEdit * AddGridEdit(QGridLayout * Grid, QWidget * Parent, const char * Name, const QString & Caption, QLabel ** Label = nullptr);
  QSpinBox * AddGridSpin(QGridLayout * Grid, QWidget * Parent, const char * Name, const QString & Caption,
    int Min, int Max, int Step = 1, QLabel ** Label = nullptr);
  // A file of keys or certificates, its browse button
  QLineEdit * AddFileEdit(QWidget * Parent, QHBoxLayout * Row, const char * Name, const QString & Title,
    const UnicodeString & Filter, std::function<void(UnicodeString & Name)> AfterDialog);
  QListWidget * AddAlgListBox(QWidget * Parent, QHBoxLayout * Row, const char * Name, QPushButton *& UpButton,
    QPushButton *& DownButton, const char * UpName, const char * DownName);
  void AlgMove(QListWidget * ListBox, int Source, int Dest);

  void LoadSession();
  void SaveSession(TSessionData * SessionData);
  // A copy of the session with the settings of the dialog
  TSessionData * GetSessionData();
  bool IsDefaultSftpServer();
  static bool HasCertificate(const UnicodeString & FileName);
  static bool IsNeon(TFSProtocol FSProtocol);
  static int LastSupportedFtpProxyMethod();
  static bool SupportedFtpProxyMethod(int Method);
  static int GetSupportedFtpProxyMethod(int Method);
  static int GetSupportedNeonProxyMethod(int Method);
  TProxyMethod GetProxyMethod();
  int GetFtpProxyLogonType();
  static TTlsVersion IndexToTlsVersion(int Index);
  static int TlsVersionToIndex(TTlsVersion TlsVersion);

  void PrivateKeyAfterDialog(UnicodeString & Name);
  void PrivateKeyViewButtonClick();
  void PrivateKeyUploadItemClick();
  void GenerateKeyButtonClick();
  void ValidateEncryptKey();
  void ProxyAutodetectButtonClick();
  void ColorButtonClick();
  void UpdateColorButton();
};
//---------------------------------------------------------------------------
TSiteAdvancedDialog::TSiteAdvancedDialog(QWidget * Parent) :
  TTreePagesDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("SiteAdvancedDialog"));
  setWindowTitle(tr("Advanced Site Settings"));
  FSessionData = NULL;
  FNoUpdate = 0;
  FColor = TColor(0);
  FKeyHasCertificate = false;

  CreateEnvironmentSheets();
  CreateConnectionSheets();
  CreateSshSheets();
  QWidget * NoteSheet = AddSheet("NoteSheet", tr("Note"), spNote, nullptr);
  QGroupBox * NoteGroup = AddGroup(NoteSheet, "NoteGroup", tr("Note"));
  NoteMemo = new QPlainTextEdit(NoteGroup);
  NoteMemo->setObjectName(QStringLiteral("NoteMemo"));
  NoteMemo->setTabChangesFocus(true);
  NoteGroup->layout()->addWidget(NoteMemo);
  NavigationTree->expandAll();

  QHBoxLayout * ButtonsLayout = new QHBoxLayout();
  ColorButton = new QPushButton(tr("&Color"), this);
  ColorButton->setObjectName(QStringLiteral("ColorButton"));
  ColorButton->setAutoDefault(false);
  connect(ColorButton, &QPushButton::clicked, this, [this]() { ColorButtonClick(); });
  ButtonsLayout->addWidget(ColorButton);
  ButtonsLayout->addStretch(1);
  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  Buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("OKBtn"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelBtn"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  ButtonsLayout->addWidget(Buttons);
  // The page of the documentation of the current page (upstream's FormHelp)
  AddHelpButton(Buttons, [this]() { return FormHelpKeyword(L"SiteAdvanced.dfm", ActiveSheetName()); });
  MainLayout()->addLayout(ButtonsLayout);

  ConnectControls();
  for (QListWidget * ListBox : { CipherListBox, KexListBox })
  {
    connect(ListBox, &QListWidget::currentRowChanged, this, [this]() { UpdateControls(); });
  }
  connect(PostLoginCommandsMemo, &QPlainTextEdit::textChanged, this, [this]() { UpdateControls(); });

  resize(fontMetrics().averageCharWidth() * 120, fontMetrics().height() * 38);
}
//---------------------------------------------------------------------------
TSiteAdvancedDialog::~TSiteAdvancedDialog()
{
}
//---------------------------------------------------------------------------
QGridLayout * TSiteAdvancedDialog::AddGrid(QGroupBox * Group)
{
  QGridLayout * Grid = new QGridLayout();
  static_cast<QBoxLayout *>(Group->layout())->addLayout(Grid);
  Grid->setColumnStretch(1, 1);
  return Grid;
}
//---------------------------------------------------------------------------
QComboBox * TSiteAdvancedDialog::AddGridCombo(QGridLayout * Grid, QWidget * Parent, const char * Name,
  const QString & Caption, const QStringList & Items, bool Editable, QLabel ** Label)
{
  QLabel * ALabel = new QLabel(Caption, Parent);
  QComboBox * Combo = CreateCombo(Parent, Name, Items, Editable);
  ALabel->setBuddy(Combo);
  int Row = Grid->rowCount();
  Grid->addWidget(ALabel, Row, 0);
  Grid->addWidget(Combo, Row, 1);
  if (Label != nullptr)
  {
    *Label = ALabel;
  }
  return Combo;
}
//---------------------------------------------------------------------------
QLineEdit * TSiteAdvancedDialog::AddGridEdit(
  QGridLayout * Grid, QWidget * Parent, const char * Name, const QString & Caption, QLabel ** Label)
{
  QLabel * ALabel = new QLabel(Caption, Parent);
  QLineEdit * Edit = new QLineEdit(Parent);
  Edit->setObjectName(QString::fromLatin1(Name));
  ALabel->setBuddy(Edit);
  int Row = Grid->rowCount();
  Grid->addWidget(ALabel, Row, 0);
  Grid->addWidget(Edit, Row, 1);
  if (Label != nullptr)
  {
    *Label = ALabel;
  }
  return Edit;
}
//---------------------------------------------------------------------------
QSpinBox * TSiteAdvancedDialog::AddGridSpin(QGridLayout * Grid, QWidget * Parent, const char * Name,
  const QString & Caption, int Min, int Max, int Step, QLabel ** Label)
{
  QLabel * ALabel = new QLabel(Caption, Parent);
  QSpinBox * Spin = CreateSpin(Parent, Name, Min, Max, Step);
  ALabel->setBuddy(Spin);
  int Row = Grid->rowCount();
  Grid->addWidget(ALabel, Row, 0);
  Grid->addWidget(Spin, Row, 1, Qt::AlignLeft);
  if (Label != nullptr)
  {
    *Label = ALabel;
  }
  return Spin;
}
//---------------------------------------------------------------------------
QLineEdit * TSiteAdvancedDialog::AddFileEdit(QWidget * Parent, QHBoxLayout * Row, const char * Name,
  const QString & Title, const UnicodeString & Filter, std::function<void(UnicodeString & Name)> AfterDialog)
{
  QLineEdit * Edit = new QLineEdit(Parent);
  Edit->setObjectName(QString::fromLatin1(Name));
  Row->addWidget(Edit, 1);
  QToolButton * Browse = new QToolButton(Parent);
  Browse->setObjectName(QString::fromLatin1(Name) + QStringLiteral("Browse"));
  Browse->setText(QStringLiteral("..."));
  Browse->setToolTip(tr("Browse"));
  Row->addWidget(Browse);
  connect(Browse, &QToolButton::clicked, this, [this, Edit, Title, Filter, AfterDialog]()
  {
    ExecuteHandler([&]()
    {
      // Reset key stats, even if new key is not selected, this way the
      // browse button gives the user chance to reload the "certificate"
      // state in case the key file is recreated with certificate
      FLastPrivateKey = EmptyStr;
      UnicodeString Path = ExpandEnvironmentVariables(ToUnicodeString(Edit->text()));
      QString FileName =
        QFileDialog::getOpenFileName(this, Title, ToQString(Path), FileDialogFilter(Filter));
      if (!FileName.isEmpty())
      {
        UnicodeString Name = ToUnicodeString(FileName);
        if (AfterDialog && (Name != ToUnicodeString(Edit->text())))
        {
          AfterDialog(Name);
        }
        Edit->setText(ToQString(Name));
      }
      UpdateControls();
    });
  });
  return Edit;
}
//---------------------------------------------------------------------------
QListWidget * TSiteAdvancedDialog::AddAlgListBox(QWidget * Parent, QHBoxLayout * Row, const char * Name,
  QPushButton *& UpButton, QPushButton *& DownButton, const char * UpName, const char * DownName)
{
  QListWidget * ListBox = new QListWidget(Parent);
  ListBox->setObjectName(QString::fromLatin1(Name));
  // Reordered by dragging too
  ListBox->setDragDropMode(QAbstractItemView::InternalMove);
  ListBox->setDefaultDropAction(Qt::MoveAction);
  Row->addWidget(ListBox, 1);
  QVBoxLayout * ButtonsLayout = new QVBoxLayout();
  UpButton = CreateButton(Parent, UpName, tr("&Up"));
  DownButton = CreateButton(Parent, DownName, tr("&Down"));
  ButtonsLayout->addWidget(UpButton);
  ButtonsLayout->addWidget(DownButton);
  ButtonsLayout->addStretch(1);
  Row->addLayout(ButtonsLayout);
  connect(UpButton, &QPushButton::clicked, this, [this, ListBox]()
  {
    AlgMove(ListBox, ListBox->currentRow(), ListBox->currentRow() - 1);
  });
  connect(DownButton, &QPushButton::clicked, this, [this, ListBox]()
  {
    AlgMove(ListBox, ListBox->currentRow(), ListBox->currentRow() + 1);
  });
  return ListBox;
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::AlgMove(QListWidget * ListBox, int Source, int Dest)
{
  if ((Source >= 0) && (Source < ListBox->count()) &&
      (Dest >= 0) && (Dest < ListBox->count()))
  {
    QListWidgetItem * Item = ListBox->takeItem(Source);
    ListBox->insertItem(Dest, Item);
    ListBox->setCurrentRow(Dest);
    ListBox->setFocus();
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::CreateEnvironmentSheets()
{
  QWidget * EnvironmentSheet = AddSheet("EnvironmentSheet", tr("Environment"), spEnvironment, nullptr);
  QGroupBox * EnvironmentGroup = AddGroup(EnvironmentSheet, "EnvironmentGroup", tr("Server environment"));
  QGridLayout * Grid = AddGrid(EnvironmentGroup);
  EOLTypeCombo = AddGridCombo(Grid, EnvironmentGroup, "EOLTypeCombo",
    tr("End-&of-line characters (if not indicated by server):"), { QStringLiteral("LF"), QStringLiteral("CR/LF") },
    false, &EOLTypeLabel);
  UtfCombo = AddGridCombo(Grid, EnvironmentGroup, "UtfCombo", tr("&UTF-8 encoding for filenames:"), {}, false, &UtfLabel);
  ComboAutoSwitchInitialize(UtfCombo);
  TimeDifferenceLabel = new QLabel(tr("Time &zone offset:"), EnvironmentGroup);
  QHBoxLayout * TimeRow = new QHBoxLayout();
  TimeDifferenceEdit = CreateSpin(EnvironmentGroup, "TimeDifferenceEdit", -25, 25);
  TimeDifferenceLabel->setBuddy(TimeDifferenceEdit);
  TimeDifferenceHoursLabel = new QLabel(tr("hours"), EnvironmentGroup);
  TimeDifferenceMinutesEdit = CreateSpin(EnvironmentGroup, "TimeDifferenceMinutesEdit", -45, 45, 15);
  TimeDifferenceMinutesLabel = new QLabel(tr("minutes"), EnvironmentGroup);
  TimeRow->addWidget(TimeDifferenceEdit);
  TimeRow->addWidget(TimeDifferenceHoursLabel);
  TimeRow->addWidget(TimeDifferenceMinutesEdit);
  TimeRow->addWidget(TimeDifferenceMinutesLabel);
  TimeRow->addStretch(1);
  int Row = Grid->rowCount();
  Grid->addWidget(TimeDifferenceLabel, Row, 0);
  Grid->addLayout(TimeRow, Row, 1);
  TimeDifferenceAutoCheck = AddCheck(EnvironmentGroup, "TimeDifferenceAutoCheck", tr("Detect &automatically"));
  TrimVMSVersionsCheck = AddCheck(EnvironmentGroup, "TrimVMSVersionsCheck", tr("&Trim VMS version numbers"));
  DSTModeGroup = AddGroup(EnvironmentSheet, "DSTModeGroup", tr("Daylight saving time"));
  DSTModeUnixCheck = AddRadio(DSTModeGroup, "DSTModeUnixCheck", tr("Adjust remote timestamp to local co&nventions"));
  DSTModeWinCheck = AddRadio(DSTModeGroup, "DSTModeWinCheck", tr("Adjust remote timestamp with &DST"));
  static_cast<QVBoxLayout *>(EnvironmentSheet->layout())->addStretch(1);
  QTreeWidgetItem * EnvironmentNode = NodeOf(spEnvironment);

  QWidget * DirectoriesSheet = AddSheet("DirectoriesSheet", tr("Directories"), spDirectories, EnvironmentNode);
  QGroupBox * DirectoriesGroup = AddGroup(DirectoriesSheet, "DirectoriesGroup", tr("Directories"));
  QLabel * RemoteDirectoryLabel = new QLabel(tr("&Remote directory:"), DirectoriesGroup);
  DirectoriesGroup->layout()->addWidget(RemoteDirectoryLabel);
  RemoteDirectoryEdit = new QLineEdit(DirectoriesGroup);
  RemoteDirectoryEdit->setObjectName(QStringLiteral("RemoteDirectoryEdit"));
  RemoteDirectoryLabel->setBuddy(RemoteDirectoryEdit);
  DirectoriesGroup->layout()->addWidget(RemoteDirectoryEdit);
  QLabel * LocalDirectoryLabel = new QLabel(tr("&Local directory:"), DirectoriesGroup);
  DirectoriesGroup->layout()->addWidget(LocalDirectoryLabel);
  QHBoxLayout * LocalRow = AddRow(DirectoriesGroup);
  LocalDirectoryEdit = CreatePathEdit(DirectoriesGroup, LocalRow, "LocalDirectoryEdit", true, tr("Select local directory"));
  LocalDirectoryLabel->setBuddy(LocalDirectoryEdit);
  UpdateDirectoriesCheck = AddCheck(DirectoriesGroup, "UpdateDirectoriesCheck", tr("Re&member last used directory"));
  SynchronizeBrowsingCheck = AddCheck(DirectoriesGroup, "SynchronizeBrowsingCheck", tr("&Synchronize browsing"));
  QGroupBox * DirectoryOptionsGroup = AddGroup(DirectoriesSheet, "DirectoryOptionsGroup", tr("Directory reading options"));
  CacheDirectoriesCheck = AddCheck(DirectoryOptionsGroup, "CacheDirectoriesCheck", tr("Cache &visited remote directories"));
  CacheDirectoryChangesCheck = AddCheck(DirectoryOptionsGroup, "CacheDirectoryChangesCheck", tr("Cache &directory changes"));
  PreserveDirectoryChangesCheck = AddCheck(DirectoryOptionsGroup, "PreserveDirectoryChangesCheck", tr("&Permanent cache"));
  ResolveSymlinksCheck = AddCheck(DirectoryOptionsGroup, "ResolveSymlinksCheck", tr("Resolve symbolic li&nks"));
  FollowDirectorySymlinksCheck =
    AddCheck(DirectoryOptionsGroup, "FollowDirectorySymlinksCheck", tr("&Follow symbolic links to directories"));
  static_cast<QVBoxLayout *>(DirectoriesSheet->layout())->addStretch(1);

  QWidget * RecycleBinSheet = AddSheet("RecycleBinSheet", tr("Recycle bin"), spRecycleBin, EnvironmentNode);
  QGroupBox * RecycleBinGroup = AddGroup(RecycleBinSheet, "RecycleBinGroup", tr("Recycle bin"));
  DeleteToRecycleBinCheck =
    AddCheck(RecycleBinGroup, "DeleteToRecycleBinCheck", tr("&Preserve deleted remote files to recycle bin"));
  OverwrittenToRecycleBinCheck =
    AddCheck(RecycleBinGroup, "OverwrittenToRecycleBinCheck", tr("Preserve &overwritten remote files to recycle bin (SFTP only)"));
  QGridLayout * RecycleGrid = AddGrid(RecycleBinGroup);
  RecycleBinPathEdit = AddGridEdit(RecycleGrid, RecycleBinGroup, "RecycleBinPathEdit", tr("&Remote recycle bin:"), &RecycleBinPathLabel);
  static_cast<QVBoxLayout *>(RecycleBinSheet->layout())->addStretch(1);

  QWidget * EncryptionSheet = AddSheet("EncryptionSheet", tr("Encryption"), spEncryption, EnvironmentNode);
  EncryptFilesCheck = AddCheck(EncryptionSheet, "EncryptFilesCheck", tr("&Encrypt files"));
  EncryptFilesGroup = AddGroup(EncryptionSheet, "EncryptFilesGroup", tr("Encryption options"));
  QGridLayout * EncryptGrid = AddGrid(EncryptFilesGroup);
  EncryptKeyPasswordEdit = AddGridEdit(EncryptGrid, EncryptFilesGroup, "EncryptKeyPasswordEdit", tr("Encryption &key:"));
  EncryptKeyPasswordEdit->setEchoMode(QLineEdit::Password);
  QHBoxLayout * EncryptRow = AddRow(EncryptFilesGroup);
  ShowEncryptionKeyCheck = new QCheckBox(tr("&Show key"), EncryptFilesGroup);
  ShowEncryptionKeyCheck->setObjectName(QStringLiteral("ShowEncryptionKeyCheck"));
  EncryptRow->addWidget(ShowEncryptionKeyCheck);
  EncryptRow->addStretch(1);
  QPushButton * GenerateKeyButton = CreateButton(EncryptFilesGroup, "GenerateKeyButton", tr("&Generate Key"));
  EncryptRow->addWidget(GenerateKeyButton);
  connect(ShowEncryptionKeyCheck, &QCheckBox::toggled, this, [this](bool Checked)
  {
    EncryptKeyPasswordEdit->setEchoMode(Checked ? QLineEdit::Normal : QLineEdit::Password);
  });
  connect(GenerateKeyButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { GenerateKeyButtonClick(); }); });
  static_cast<QVBoxLayout *>(EncryptionSheet->layout())->addStretch(1);

  QWidget * SftpSheet = AddSheet("SftpSheet", tr("SFTP"), spSftp, EnvironmentNode);
  QGroupBox * SFTPProtocolGroup = AddGroup(SftpSheet, "SFTPProtocolGroup", tr("Protocol options"));
  QGridLayout * SftpGrid = AddGrid(SFTPProtocolGroup);
  SftpServerEdit = AddGridCombo(SftpGrid, SFTPProtocolGroup, "SftpServerEdit", tr("SFTP ser&ver:"),
    { tr("Default"), QStringLiteral("/bin/sftp-server"), QStringLiteral("sudo su -c /bin/sftp-server") }, true);
  SFTPMaxVersionCombo = AddGridCombo(SftpGrid, SFTPProtocolGroup, "SFTPMaxVersionCombo", tr("&Preferred SFTP protocol version:"),
    { tr("Auto"), QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"), QStringLiteral("4"),
      QStringLiteral("5"), QStringLiteral("6") });
  SFTPRealPathCombo = AddGridCombo(SftpGrid, SFTPProtocolGroup, "SFTPRealPathCombo", tr("Ca&nonicalize paths on the server"), {});
  ComboAutoSwitchInitialize(SFTPRealPathCombo);
  AllowScpFallbackCheck = AddCheck(SFTPProtocolGroup, "AllowScpFallbackCheck", tr("Allow SCP &fallback"));
  UsePosixRenameCheck = AddCheck(SFTPProtocolGroup, "UsePosixRenameCheck", tr("&Use POSIX rename"));
  QGroupBox * SFTPBugsGroupBox = AddGroup(SftpSheet, "SFTPBugsGroupBox", tr("Detection of known bugs in SFTP servers"));
  QGridLayout * SftpBugsGrid = AddGrid(SFTPBugsGroupBox);
  SFTPBugSymlinkCombo = AddGridCombo(SftpBugsGrid, SFTPBugsGroupBox, "SFTPBugSymlinkCombo", tr("&Reverses order of link command arguments:"), {});
  ComboAutoSwitchInitialize(SFTPBugSymlinkCombo);
  SFTPBugSignedTSCombo =
    AddGridCombo(SftpBugsGrid, SFTPBugsGroupBox, "SFTPBugSignedTSCombo", tr("&Misinterprets file timestamps prior to 1970:"), {});
  ComboAutoSwitchInitialize(SFTPBugSignedTSCombo);
  static_cast<QVBoxLayout *>(SftpSheet->layout())->addStretch(1);

  QWidget * ScpSheet = AddSheet("ScpSheet", ToQString(LoadStr(LOGIN_SCP_SHELL_PAGE)), spScp, EnvironmentNode);
  QGroupBox * ShellGroup = AddGroup(ScpSheet, "ShellGroup", tr("Shell"));
  QGridLayout * ShellGrid = AddGrid(ShellGroup);
  ShellEdit = AddGridCombo(ShellGrid, ShellGroup, "ShellEdit", tr("S&hell:"),
    { tr("Default"), QStringLiteral("/bin/bash"), QStringLiteral("/bin/ksh"), QStringLiteral("/bin/sh"), QStringLiteral("sudo su -") }, true);
  ReturnVarEdit = AddGridCombo(ShellGrid, ShellGroup, "ReturnVarEdit", tr("&Return code variable:"),
    { tr("Autodetect"), QStringLiteral("?"), QStringLiteral("status") }, true);
  ScpLsOptionsGroup = AddGroup(ScpSheet, "ScpLsOptionsGroup", tr("Directory listing"));
  QGridLayout * LsGrid = AddGrid(ScpLsOptionsGroup);
  ListingCommandEdit = AddGridCombo(LsGrid, ScpLsOptionsGroup, "ListingCommandEdit", tr("Listing &command:"),
    { QStringLiteral("ls -la"), QStringLiteral("ls -gla") }, true);
  IgnoreLsWarningsCheck = AddCheck(ScpLsOptionsGroup, "IgnoreLsWarningsCheck", tr("Ignore LS &warnings"));
  SCPLsFullTimeAutoCheck = AddCheck(ScpLsOptionsGroup, "SCPLsFullTimeAutoCheck", tr("Try to get &full timestamp"));
  OtherShellOptionsGroup = AddGroup(ScpSheet, "OtherShellOptionsGroup", tr("Other options"));
  LookupUserGroupsCheck = AddCheck(OtherShellOptionsGroup, "LookupUserGroupsCheck", tr("Lookup &user groups"));
  LookupUserGroupsCheck->setTristate(true);
  ClearAliasesCheck = AddCheck(OtherShellOptionsGroup, "ClearAliasesCheck", tr("Clear a&liases"));
  UnsetNationalVarsCheck = AddCheck(OtherShellOptionsGroup, "UnsetNationalVarsCheck", tr("Clear &national variables"));
  Scp1CompatibilityCheck = AddCheck(OtherShellOptionsGroup, "Scp1CompatibilityCheck", tr("Use scp&2 with scp1 compatibility"));
  static_cast<QVBoxLayout *>(ScpSheet->layout())->addStretch(1);

  QWidget * FtpSheet = AddSheet("FtpSheet", tr("FTP"), spFtp, EnvironmentNode);
  QGroupBox * FtpGroup = AddGroup(FtpSheet, "FtpGroup", tr("Protocol options"));
  QLabel * Label25 = new QLabel(tr("Post login &commands:"), FtpGroup);
  FtpGroup->layout()->addWidget(Label25);
  PostLoginCommandsMemo = new QPlainTextEdit(FtpGroup);
  PostLoginCommandsMemo->setObjectName(QStringLiteral("PostLoginCommandsMemo"));
  PostLoginCommandsMemo->setTabChangesFocus(true);
  PostLoginCommandsMemo->setMaximumHeight(fontMetrics().lineSpacing() * 5);
  Label25->setBuddy(PostLoginCommandsMemo);
  FtpGroup->layout()->addWidget(PostLoginCommandsMemo);
  QGridLayout * FtpGrid = AddGrid(FtpGroup);
  FtpListAllCombo = AddGridCombo(FtpGrid, FtpGroup, "FtpListAllCombo", tr("&Support for listing of hidden files:"), {}, false, &FtpListAllLabel);
  ComboAutoSwitchInitialize(FtpListAllCombo);
  FtpUseMlsdCombo = AddGridCombo(FtpGrid, FtpGroup, "FtpUseMlsdCombo", tr("Use &MLSD command for directory listing"), {});
  ComboAutoSwitchInitialize(FtpUseMlsdCombo);
  FtpForcePasvIpCombo = AddGridCombo(FtpGrid, FtpGroup, "FtpForcePasvIpCombo",
    tr("&Force IP address for passive mode connections:"), {}, false, &FtpForcePasvIpLabel);
  ComboAutoSwitchInitialize(FtpForcePasvIpCombo);
  FtpHostCombo = AddGridCombo(FtpGrid, FtpGroup, "FtpHostCombo", tr("Use &HOST command to select host on the server"), {});
  ComboAutoSwitchInitialize(FtpHostCombo);
  FtpAccountEdit = AddGridEdit(FtpGrid, FtpGroup, "FtpAccountEdit", tr("&Account:"));
  VMSAllRevisionsCheck = AddCheck(FtpGroup, "VMSAllRevisionsCheck", tr("Display all file &revisions on VMS servers"));
  static_cast<QVBoxLayout *>(FtpSheet->layout())->addStretch(1);

  QWidget * S3Sheet = AddSheet("S3Sheet", tr("S3"), spS3, EnvironmentNode);
  QGroupBox * S3Group = AddGroup(S3Sheet, "S3Group", tr("Protocol options"));
  QGridLayout * S3Grid = AddGrid(S3Group);
  QStringList Regions;
  for (const char * Region :
         { "af-south-1", "ap-east-1", "ap-east-2", "ap-northeast-1", "ap-northeast-2", "ap-northeast-3", "ap-south-1",
           "ap-south-2", "ap-southeast-1", "ap-southeast-2", "ap-southeast-3", "ap-southeast-4", "ap-southeast-5",
           "ap-southeast-6", "ap-southeast-7", "ca-central-1", "ca-west-1", "cn-north-1", "cn-northwest-1",
           "eu-central-1", "eu-central-2", "eu-north-1", "eu-south-1", "eu-south-2", "eu-west-1", "eu-west-2",
           "eu-west-3", "il-central-1", "me-central-1", "me-south-1", "mx-central-1", "sa-east-1", "us-east-1",
           "us-east-2", "us-gov-east-1", "us-gov-west-1", "us-west-1", "us-west-2" })
  {
    Regions.append(QString::fromLatin1(Region));
  }
  S3DefaultReqionCombo = AddGridCombo(S3Grid, S3Group, "S3DefaultReqionCombo", tr("&Default region:"), Regions, true);
  S3UrlStyleCombo = AddGridCombo(S3Grid, S3Group, "S3UrlStyleCombo", tr("&URL style:"), { tr("Virtual Host"), tr("Path") });
  S3RequesterPaysCheck = AddCheck(S3Group, "S3RequesterPaysCheck", tr("Requester &pays"));
  QGroupBox * S3AuthenticationGroup = AddGroup(S3Sheet, "S3AuthenticationGroup", tr("Authentication"));
  S3SessionTokenLabel = new QLabel(tr("&Session token:"), S3AuthenticationGroup);
  S3AuthenticationGroup->layout()->addWidget(S3SessionTokenLabel);
  S3SessionTokenMemo = new QPlainTextEdit(S3AuthenticationGroup);
  S3SessionTokenMemo->setObjectName(QStringLiteral("S3SessionTokenMemo"));
  S3SessionTokenMemo->setTabChangesFocus(true);
  S3SessionTokenMemo->setMaximumHeight(fontMetrics().lineSpacing() * 5);
  S3SessionTokenLabel->setBuddy(S3SessionTokenMemo);
  S3AuthenticationGroup->layout()->addWidget(S3SessionTokenMemo);
  QGridLayout * S3AuthGrid = AddGrid(S3AuthenticationGroup);
  S3RoleArnEdit = AddGridEdit(S3AuthGrid, S3AuthenticationGroup, "S3RoleArnEdit", tr("&Role ARN:"), &S3RoleArnLabel);
  static_cast<QVBoxLayout *>(S3Sheet->layout())->addStretch(1);

  QWidget * WebDavSheet = AddSheet("WebDavSheet", tr("WebDAV"), spWebDav, EnvironmentNode);
  QGroupBox * WebdavGroup = AddGroup(WebDavSheet, "WebdavGroup", tr("Protocol options"));
  WebDavLiberalEscapingCheck =
    AddCheck(WebdavGroup, "WebDavLiberalEscapingCheck", tr("&Tolerate non-encoded special characters in filenames"));
  WebDavCrossDomainRedirectsCheck =
    AddCheck(WebdavGroup, "WebDavCrossDomainRedirectsCheck", tr("&Allow redirects to other hosts"));
  static_cast<QVBoxLayout *>(WebDavSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::CreateConnectionSheets()
{
  QWidget * ConnSheet = AddSheet("ConnSheet", tr("Connection"), spConnection, nullptr);
  QGroupBox * ConnectionGroup = AddGroup(ConnSheet, "ConnectionGroup", tr("Connection"));
  FtpPasvModeCheck = AddCheck(ConnectionGroup, "FtpPasvModeCheck", tr("&Passive mode"));
  BufferSizeCheck = AddCheck(ConnectionGroup, "BufferSizeCheck", tr("Optimize connection &buffer size"));
  QGroupBox * TimeoutGroup = AddGroup(ConnSheet, "TimeoutGroup", tr("Timeouts"));
  QHBoxLayout * TimeoutRow = AddRow(TimeoutGroup);
  QLabel * Label11 = new QLabel(tr("Server &response timeout:"), TimeoutGroup);
  TimeoutRow->addWidget(Label11);
  TimeoutEdit = CreateSpin(TimeoutGroup, "TimeoutEdit", 5, 6000, 5);
  Label11->setBuddy(TimeoutEdit);
  TimeoutRow->addWidget(TimeoutEdit);
  TimeoutRow->addWidget(new QLabel(tr("seconds"), TimeoutGroup));
  TimeoutRow->addStretch(1);

  PingGroup = AddGroup(ConnSheet, "PingGroup", tr("Keepalives"));
  PingOffButton = AddRadio(PingGroup, "PingOffButton", tr("&Off"));
  PingNullPacketButton = AddRadio(PingGroup, "PingNullPacketButton", tr("Sending of &null SSH packets"));
  PingDummyCommandButton = AddRadio(PingGroup, "PingDummyCommandButton", tr("Executing &dummy protocol commands"));
  QHBoxLayout * PingRow = AddRow(PingGroup);
  PingIntervalLabel = new QLabel(tr("Seconds &between keepalives:"), PingGroup);
  PingRow->addWidget(PingIntervalLabel);
  PingIntervalSecEdit = CreateSpin(PingGroup, "PingIntervalSecEdit", 1, 3600);
  PingIntervalLabel->setBuddy(PingIntervalSecEdit);
  PingRow->addWidget(PingIntervalSecEdit);
  PingRow->addStretch(1);

  FtpPingGroup = AddGroup(ConnSheet, "FtpPingGroup", tr("Keepalives"));
  FtpPingOffButton = AddRadio(FtpPingGroup, "FtpPingOffButton", tr("&Off"));
  FtpPingDummyCommandButton = AddRadio(FtpPingGroup, "FtpPingDummyCommandButton", tr("Executing &dummy protocol commands"));
  FtpPingDirectoryListingButton =
    AddRadio(FtpPingGroup, "FtpPingDirectoryListingButton", tr("&And additionally reading the current directory"));
  QHBoxLayout * FtpPingRow = AddRow(FtpPingGroup);
  FtpPingIntervalLabel = new QLabel(tr("Seconds &between keepalives:"), FtpPingGroup);
  FtpPingRow->addWidget(FtpPingIntervalLabel);
  FtpPingIntervalSecEdit = CreateSpin(FtpPingGroup, "FtpPingIntervalSecEdit", 1, 3600);
  FtpPingIntervalLabel->setBuddy(FtpPingIntervalSecEdit);
  FtpPingRow->addWidget(FtpPingIntervalSecEdit);
  FtpPingRow->addStretch(1);

  IPvGroup = AddGroup(ConnSheet, "IPvGroup", tr("Internet protocol version"));
  QHBoxLayout * IPvRow = AddRow(IPvGroup);
  IPAutoButton = new QRadioButton(tr("A&uto"), IPvGroup);
  IPAutoButton->setObjectName(QStringLiteral("IPAutoButton"));
  IPv4Button = new QRadioButton(tr("IPv&4"), IPvGroup);
  IPv4Button->setObjectName(QStringLiteral("IPv4Button"));
  IPv6Button = new QRadioButton(tr("IPv&6"), IPvGroup);
  IPv6Button->setObjectName(QStringLiteral("IPv6Button"));
  IPvRow->addWidget(IPAutoButton);
  IPvRow->addWidget(IPv4Button);
  IPvRow->addWidget(IPv6Button);
  IPvRow->addStretch(1);
  static_cast<QVBoxLayout *>(ConnSheet->layout())->addStretch(1);
  QTreeWidgetItem * ConnectionNode = NodeOf(spConnection);

  QWidget * ProxySheet = AddSheet("ProxySheet", tr("Proxy"), spProxy, ConnectionNode);
  QGroupBox * ProxyTypeGroup = AddGroup(ProxySheet, "ProxyTypeGroup", tr("Proxy"));
  QGridLayout * ProxyGrid = AddGrid(ProxyTypeGroup);
  ProxyMethodLabel = new QLabel(tr("Proxy &type:"), ProxyTypeGroup);
  SshProxyMethodCombo = CreateCombo(ProxyTypeGroup, "SshProxyMethodCombo",
    { tr("None"), QStringLiteral("SOCKS4"), QStringLiteral("SOCKS5"), QStringLiteral("HTTP"), QStringLiteral("Telnet"), tr("Local") });
  FtpProxyMethodCombo = CreateCombo(ProxyTypeGroup, "FtpProxyMethodCombo",
    { tr("None"), QStringLiteral("SOCKS4"), QStringLiteral("SOCKS5"), QStringLiteral("HTTP"), QStringLiteral("SITE %host"),
      QStringLiteral("USER %proxyuser, USER %user@%host"), QStringLiteral("OPEN %host"),
      QStringLiteral("USER %proxyuser, USER %user"), QStringLiteral("USER %user@%host"),
      QStringLiteral("USER %proxyuser@%host"), QStringLiteral("USER %user@%host %proxyuser"),
      QStringLiteral("USER %user@%proxyuser@%host") });
  NeonProxyMethodCombo = CreateCombo(ProxyTypeGroup, "NeonProxyMethodCombo",
    { tr("None"), QStringLiteral("SOCKS4"), QStringLiteral("SOCKS5"), QStringLiteral("HTTP") });
  QHBoxLayout * MethodRow = new QHBoxLayout();
  MethodRow->addWidget(SshProxyMethodCombo);
  MethodRow->addWidget(FtpProxyMethodCombo);
  MethodRow->addWidget(NeonProxyMethodCombo);
  MethodRow->addStretch(1);
  QPushButton * ProxyAutodetectButton = CreateButton(ProxyTypeGroup, "ProxyAutodetectButton", tr("&Autodetect"));
  MethodRow->addWidget(ProxyAutodetectButton);
  connect(ProxyAutodetectButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { ProxyAutodetectButtonClick(); }); });
  ProxyGrid->addWidget(ProxyMethodLabel, 0, 0);
  ProxyGrid->addLayout(MethodRow, 0, 1);
  ProxyHostEdit = AddGridEdit(ProxyGrid, ProxyTypeGroup, "ProxyHostEdit", tr("Pro&xy hostname:"), &ProxyHostLabel);
  ProxyPortEdit = AddGridSpin(ProxyGrid, ProxyTypeGroup, "ProxyPortEdit", tr("Po&rt number:"), 1, 65535, 1, &ProxyPortLabel);
  ProxyUsernameEdit = AddGridEdit(ProxyGrid, ProxyTypeGroup, "ProxyUsernameEdit", tr("&Username:"), &ProxyUsernameLabel);
  ProxyPasswordEdit = AddGridEdit(ProxyGrid, ProxyTypeGroup, "ProxyPasswordEdit", tr("&Password:"), &ProxyPasswordLabel);
  ProxyPasswordEdit->setEchoMode(QLineEdit::Password);

  ProxySettingsGroup = AddGroup(ProxySheet, "ProxySettingsGroup", tr("Proxy settings"));
  QGridLayout * ProxySettingsGrid = AddGrid(ProxySettingsGroup);
  QString PatternsHint = ToQString(LoadStr(LOGIN_PROXY_COMMAND_PATTERNS_HINT));
  ProxyTelnetCommandEdit =
    AddGridEdit(ProxySettingsGrid, ProxySettingsGroup, "ProxyTelnetCommandEdit", tr("Telnet co&mmand:"), &ProxyTelnetCommandLabel);
  ProxyTelnetCommandEdit->setToolTip(PatternsHint);
  ProxyLocalCommandLabel = new QLabel(tr("Local proxy co&mmand:"), ProxySettingsGroup);
  QHBoxLayout * LocalCommandRow = new QHBoxLayout();
  ProxyLocalCommandEdit = new QLineEdit(ProxySettingsGroup);
  ProxyLocalCommandEdit->setObjectName(QStringLiteral("ProxyLocalCommandEdit"));
  ProxyLocalCommandEdit->setToolTip(PatternsHint);
  ProxyLocalCommandLabel->setBuddy(ProxyLocalCommandEdit);
  LocalCommandRow->addWidget(ProxyLocalCommandEdit, 1);
  ProxyLocalCommandBrowseButton = CreateButton(ProxySettingsGroup, "ProxyLocalCommandBrowseButton", tr("&Browse..."));
  LocalCommandRow->addWidget(ProxyLocalCommandBrowseButton);
  connect(ProxyLocalCommandBrowseButton, &QPushButton::clicked, this, [this]()
  {
    QString FileName = QFileDialog::getOpenFileName(this, ToQString(LoadStr(LOGIN_SELECT_LOCAL_PROXY)));
    if (!FileName.isEmpty())
    {
      ProxyLocalCommandEdit->setText(ToQString(AddPathQuotes(ToUnicodeString(FileName))));
    }
  });
  int Row = ProxySettingsGrid->rowCount();
  ProxySettingsGrid->addWidget(ProxyLocalCommandLabel, Row, 0);
  ProxySettingsGrid->addLayout(LocalCommandRow, Row, 1);
  ProxyLocalhostCheck = new QCheckBox(tr("Co&nsider proxying local host connections"), ProxySettingsGroup);
  ProxyLocalhostCheck->setObjectName(QStringLiteral("ProxyLocalhostCheck"));
  ProxySettingsGrid->addWidget(ProxyLocalhostCheck, ProxySettingsGrid->rowCount(), 0, 1, 2);
  ProxyDNSCombo = AddGridCombo(ProxySettingsGrid, ProxySettingsGroup, "ProxyDNSCombo", tr("Do &DNS name lookup at proxy end:"),
    { tr("Auto"), tr("No"), tr("Yes") });
  static_cast<QVBoxLayout *>(ProxySheet->layout())->addStretch(1);

  QWidget * TunnelSheet = AddSheet("TunnelSheet", tr("Tunnel"), spTunnel, ConnectionNode);
  TunnelCheck = AddCheck(TunnelSheet, "TunnelCheck", tr("Connect &through SSH tunnel"));
  TunnelSessionGroup = AddGroup(TunnelSheet, "TunnelSessionGroup", tr("Host to setup tunnel on"));
  QGridLayout * TunnelGrid = AddGrid(TunnelSessionGroup);
  TunnelHostNameEdit = AddGridEdit(TunnelGrid, TunnelSessionGroup, "TunnelHostNameEdit", tr("&Hostname:"));
  TunnelPortNumberEdit = AddGridSpin(TunnelGrid, TunnelSessionGroup, "TunnelPortNumberEdit", tr("Po&rt number:"), 1, 65535);
  TunnelUserNameEdit = AddGridEdit(TunnelGrid, TunnelSessionGroup, "TunnelUserNameEdit", tr("&Username:"));
  TunnelPasswordEdit = AddGridEdit(TunnelGrid, TunnelSessionGroup, "TunnelPasswordEdit", tr("&Password:"));
  TunnelPasswordEdit->setEchoMode(QLineEdit::Password);
  TunnelOptionsGroup = AddGroup(TunnelSheet, "TunnelOptionsGroup", tr("Tunnel options"));
  QGridLayout * TunnelOptionsGrid = AddGrid(TunnelOptionsGroup);
  QStringList Ports = { tr("Autoselect") };
  for (int Index = Configuration->TunnelLocalPortNumberLow; Index <= Configuration->TunnelLocalPortNumberHigh; Index++)
  {
    Ports.append(QString::number(Index));
  }
  TunnelLocalPortNumberEdit =
    AddGridCombo(TunnelOptionsGrid, TunnelOptionsGroup, "TunnelLocalPortNumberEdit", tr("&Local tunnel port:"), Ports, true);
  TunnelAuthenticationParamsGroup =
    AddGroup(TunnelSheet, "TunnelAuthenticationParamsGroup", tr("Tunnel authentication parameters"));
  QLabel * Label18 = new QLabel(tr("Private &key file:"), TunnelAuthenticationParamsGroup);
  TunnelAuthenticationParamsGroup->layout()->addWidget(Label18);
  QHBoxLayout * TunnelKeyRow = AddRow(TunnelAuthenticationParamsGroup);
  TunnelPrivateKeyEdit3 = AddFileEdit(TunnelAuthenticationParamsGroup, TunnelKeyRow, "TunnelPrivateKeyEdit3",
    tr("Select private key file"), LoadStr(LOGIN_PUBLIC_KEY_FILTER),
    [](UnicodeString & Name) { VerifyAndConvertKey(Name, true); });
  Label18->setBuddy(TunnelPrivateKeyEdit3);
  static_cast<QVBoxLayout *>(TunnelSheet->layout())->addStretch(1);

  QWidget * SslSheet = AddSheet("SslSheet", tr("TLS/SSL"), spSsl, ConnectionNode);
  QGroupBox * TlsGroup = AddGroup(SslSheet, "TlsGroup", tr("TLS options"));
  QGridLayout * TlsGrid = AddGrid(TlsGroup);
  QStringList TlsVersions = { QStringLiteral("TLS 1.0"), QStringLiteral("TLS 1.1"), QStringLiteral("TLS 1.2"), QStringLiteral("TLS 1.3") };
  MinTlsVersionCombo = AddGridCombo(TlsGrid, TlsGroup, "MinTlsVersionCombo", tr("Mi&nimum TLS version:"), TlsVersions);
  MaxTlsVersionCombo = AddGridCombo(TlsGrid, TlsGroup, "MaxTlsVersionCombo", tr("Ma&ximum TLS version:"), TlsVersions);
  connect(MinTlsVersionCombo, &QComboBox::activated, this, [this]()
  {
    if (MaxTlsVersionCombo->currentIndex() < MinTlsVersionCombo->currentIndex())
    {
      MaxTlsVersionCombo->setCurrentIndex(MinTlsVersionCombo->currentIndex());
    }
  });
  connect(MaxTlsVersionCombo, &QComboBox::activated, this, [this]()
  {
    if (MinTlsVersionCombo->currentIndex() > MaxTlsVersionCombo->currentIndex())
    {
      MinTlsVersionCombo->setCurrentIndex(MaxTlsVersionCombo->currentIndex());
    }
  });
  SslSessionReuseCheck2 = AddCheck(TlsGroup, "SslSessionReuseCheck2", tr("&Reuse TLS session ID for data connections"));
  TlsAuthenticationGroup = AddGroup(SslSheet, "TlsAuthenticationGroup", tr("Authentication parameters"));
  QLabel * Label4 = new QLabel(tr("&Client certificate file:"), TlsAuthenticationGroup);
  TlsAuthenticationGroup->layout()->addWidget(Label4);
  QHBoxLayout * CertificateRow = AddRow(TlsAuthenticationGroup);
  TlsCertificateFileEdit = AddFileEdit(TlsAuthenticationGroup, CertificateRow, "TlsCertificateFileEdit",
    tr("Select client certificate file"),
    L"Certificates and private key files (*.pfx;*.p12;*.key;*.pem)|*.pfx;*.p12;*.key;*.pem|All Files (*.*)|*.*",
    [](UnicodeString & Name) { VerifyCertificate(Name); });
  Label4->setBuddy(TlsCertificateFileEdit);
  static_cast<QVBoxLayout *>(SslSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::CreateSshSheets()
{
  QWidget * AdvancedSheet = AddSheet("AdvancedSheet", tr("SSH"), spSsh, nullptr);
  QGroupBox * ProtocolGroup = AddGroup(AdvancedSheet, "ProtocolGroup", tr("Protocol options"));
  CompressionCheck = AddCheck(ProtocolGroup, "CompressionCheck", tr("&Enable compression"));
  QGroupBox * EncryptionGroup = AddGroup(AdvancedSheet, "EncryptionGroup", tr("Encryption options"));
  QLabel * Label8 = new QLabel(tr("Encryption cipher selection &policy:"), EncryptionGroup);
  EncryptionGroup->layout()->addWidget(Label8);
  QHBoxLayout * CipherRow = AddRow(EncryptionGroup);
  CipherListBox = AddAlgListBox(EncryptionGroup, CipherRow, "CipherListBox", CipherUpButton, CipherDownButton,
    "CipherUpButton", "CipherDownButton");
  Label8->setBuddy(CipherListBox);
  Ssh2LegacyDESCheck = AddCheck(EncryptionGroup, "Ssh2LegacyDESCheck", tr("Enable legacy use of single-&DES"));
  static_cast<QVBoxLayout *>(AdvancedSheet->layout())->addStretch(1);
  QTreeWidgetItem * SshNode = NodeOf(spSsh);

  QWidget * KexSheet = AddSheet("KexSheet", tr("Key exchange"), spKex, SshNode);
  QGroupBox * KexOptionsGroup = AddGroup(KexSheet, "KexOptionsGroup", tr("Key exchange algorithm options"));
  QLabel * Label28 = new QLabel(tr("Algorithm selection &policy:"), KexOptionsGroup);
  KexOptionsGroup->layout()->addWidget(Label28);
  QHBoxLayout * KexRow = AddRow(KexOptionsGroup);
  KexListBox = AddAlgListBox(KexOptionsGroup, KexRow, "KexListBox", KexUpButton, KexDownButton, "KexUpButton", "KexDownButton");
  Label28->setBuddy(KexListBox);
  AuthGSSAPIKEXCheck = AddCheck(KexOptionsGroup, "AuthGSSAPIKEXCheck", tr("Attempt &GSSAPI key exchange"));
  KexReexchangeGroup = AddGroup(KexSheet, "KexReexchangeGroup", tr("Options controlling key re-exchange"));
  QGridLayout * RekeyGrid = AddGrid(KexReexchangeGroup);
  RekeyTimeEdit = AddGridSpin(RekeyGrid, KexReexchangeGroup, "RekeyTimeEdit", tr("Max &minutes before rekey (0 for no limit):"), 0, 1440);
  RekeyDataEdit = AddGridEdit(RekeyGrid, KexReexchangeGroup, "RekeyDataEdit", tr("Ma&x data before rekey (0 for no limit):"));
  static_cast<QVBoxLayout *>(KexSheet->layout())->addStretch(1);

  QWidget * AuthSheet = AddSheet("AuthSheet", tr("Authentication"), spAuthentication, SshNode);
  SshNoUserAuthCheck = AddCheck(AuthSheet, "SshNoUserAuthCheck", tr("&Bypass authentication entirely"));
  AuthenticationGroup = AddGroup(AuthSheet, "AuthenticationGroup", tr("Authentication options"));
  TryAgentCheck2 = AddCheck(AuthenticationGroup, "TryAgentCheck2", tr("&Attempt authentication using agent"));
  AuthKICheck = AddCheck(AuthenticationGroup, "AuthKICheck", tr("Attempt 'keyboard-&interactive' authentication"));
  AuthKIPasswordCheck = AddCheck(AuthenticationGroup, "AuthKIPasswordCheck", tr("Respond with a pass&word to the first prompt"));
  AuthenticationParamsGroup = AddGroup(AuthSheet, "AuthenticationParamsGroup", tr("Authentication parameters"));
  AgentFwdCheck = AddCheck(AuthenticationParamsGroup, "AgentFwdCheck", tr("Allow agent &forwarding"));
  QLabel * PrivateKeyLabel = new QLabel(tr("Private &key file:"), AuthenticationParamsGroup);
  AuthenticationParamsGroup->layout()->addWidget(PrivateKeyLabel);
  QHBoxLayout * KeyRow = AddRow(AuthenticationParamsGroup);
  PrivateKeyEdit3 = AddFileEdit(AuthenticationParamsGroup, KeyRow, "PrivateKeyEdit3",
    tr("Select private key file"), LoadStr(LOGIN_PUBLIC_KEY_FILTER),
    [this](UnicodeString & Name) { PrivateKeyAfterDialog(Name); });
  PrivateKeyLabel->setBuddy(PrivateKeyEdit3);
  QHBoxLayout * KeyButtonsRow = AddRow(AuthenticationParamsGroup);
  KeyButtonsRow->addStretch(1);
  PrivateKeyViewButton = CreateButton(AuthenticationParamsGroup, "PrivateKeyViewButton", tr("&Display Public Key"));
  KeyButtonsRow->addWidget(PrivateKeyViewButton);
  connect(PrivateKeyViewButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { PrivateKeyViewButtonClick(); }); });
  PrivateKeyToolsButton = CreateButton(AuthenticationParamsGroup, "PrivateKeyToolsButton", tr("&Tools"));
  QMenu * PrivateKeyMenu = new QMenu(PrivateKeyToolsButton);
  PrivateKeyMenu->setObjectName(QStringLiteral("PrivateKeyMenu"));
  QAction * PrivateKeyUploadItem = PrivateKeyMenu->addAction(tr("&Install Public Key into Server..."));
  PrivateKeyUploadItem->setObjectName(QStringLiteral("PrivateKeyUploadItem"));
  connect(PrivateKeyUploadItem, &QAction::triggered, this, [this]() { ExecuteHandler([this]() { PrivateKeyUploadItemClick(); }); });
  connect(PrivateKeyMenu, &QMenu::aboutToShow, this, [this, PrivateKeyUploadItem]()
  {
    std::unique_ptr<TSessionData> SessionData(GetSessionData());
    PrivateKeyUploadItem->setEnabled(
      SessionData->CanLogin && (NormalizeFSProtocol(SessionData->FSProtocol) == fsSFTP));
  });
  PrivateKeyToolsButton->setMenu(PrivateKeyMenu);
  KeyButtonsRow->addWidget(PrivateKeyToolsButton);
  DetachedCertificateLabel = new QLabel(tr("Certificate to &use with the private key:"), AuthenticationParamsGroup);
  AuthenticationParamsGroup->layout()->addWidget(DetachedCertificateLabel);
  QHBoxLayout * CertificateRow = AddRow(AuthenticationParamsGroup);
  DetachedCertificateEdit = AddFileEdit(AuthenticationParamsGroup, CertificateRow, "DetachedCertificateEdit",
    tr("Select certificate file"), L"Public key files (*.pub)|*.pub|All Files (*.*)|*.*", nullptr);
  DetachedCertificateLabel->setBuddy(DetachedCertificateEdit);
  QGroupBox * GSSAPIGroup = AddGroup(AuthSheet, "GSSAPIGroup", tr("GSSAPI"));
  AuthGSSAPICheck3 = AddCheck(GSSAPIGroup, "AuthGSSAPICheck3", tr("Attempt &GSSAPI authentication"));
  GSSAPIFwdTGTCheck = AddCheck(GSSAPIGroup, "GSSAPIFwdTGTCheck", tr("Allow GSSAPI c&redential delegation"));
  connect(AuthGSSAPICheck3, &QCheckBox::clicked, this, [](bool Checked)
  {
    if (Checked && !HasGSSAPI(L""))
    {
      ExecuteHandler([]() { throw Exception(LoadStr(GSSAPI_NOT_INSTALLED2)); });
    }
  });
  static_cast<QVBoxLayout *>(AuthSheet->layout())->addStretch(1);

  QWidget * BugsSheet = AddSheet("BugsSheet", tr("Bugs"), spBugs, SshNode);
  QGroupBox * BugsGroupBox = AddGroup(BugsSheet, "BugsGroupBox", tr("Detection of known bugs in SSH servers"));
  QGridLayout * BugsGrid = AddGrid(BugsGroupBox);
  struct { QComboBox ** Combo; const char * Name; QString Caption; } Bugs[] =
  {
    { &BugHMAC2Combo, "BugHMAC2Combo", tr("Miscomputes SSH H&MAC keys:") },
    { &BugDeriveKey2Combo, "BugDeriveKey2Combo", tr("Miscomputes SSH &encryption keys:") },
    { &BugRSAPad2Combo, "BugRSAPad2Combo", tr("Requires &padding on SSH RSA signatures:") },
    { &BugPKSessID2Combo, "BugPKSessID2Combo", tr("Misuses the sessio&n ID in SSH PK auth:") },
    { &BugRekey2Combo, "BugRekey2Combo", tr("Handles SSH &key re-exchange badly:") },
    { &BugMaxPkt2Combo, "BugMaxPkt2Combo", tr("Ignores SSH ma&ximum packet size:") },
    { &BugIgnore2Combo, "BugIgnore2Combo", tr("Chokes on SSH i&gnore messages:") },
    { &BugWinAdjCombo, "BugWinAdjCombo", tr("Chokes on OpenSCP's SSH '&winadj' requests") },
  };
  for (auto & Bug : Bugs)
  {
    *Bug.Combo = AddGridCombo(BugsGrid, BugsGroupBox, Bug.Name, Bug.Caption, {});
    ComboAutoSwitchInitialize(*Bug.Combo);
  }
  static_cast<QVBoxLayout *>(BugsSheet->layout())->addStretch(1);
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::LoadSession()
{
  {
    TAutoNestingCounter NoUpdateCounter(FNoUpdate);

    // Directories page
    SynchronizeBrowsingCheck->setChecked(FSessionData->SynchronizeBrowsing);
    LocalDirectoryEdit->setText(ToQString(FSessionData->LocalDirectory));
    RemoteDirectoryEdit->setText(ToQString(FSessionData->RemoteDirectory));
    UpdateDirectoriesCheck->setChecked(FSessionData->UpdateDirectories);
    CacheDirectoriesCheck->setChecked(FSessionData->CacheDirectories);
    CacheDirectoryChangesCheck->setChecked(FSessionData->CacheDirectoryChanges);
    PreserveDirectoryChangesCheck->setChecked(FSessionData->PreserveDirectoryChanges);
    ResolveSymlinksCheck->setChecked(FSessionData->ResolveSymlinks);
    FollowDirectorySymlinksCheck->setChecked(FSessionData->FollowDirectorySymlinks);

    // Environment page
    switch (FSessionData->DSTMode)
    {
      case dstmWin:
        DSTModeWinCheck->setChecked(true);
        break;

      default:
      case dstmKeep: // deprecated
      case dstmUnix:
        DSTModeUnixCheck->setChecked(true);
        break;
    }

    EOLTypeCombo->setCurrentIndex((FSessionData->EOLType == eolLF) ? 0 : 1);

    switch (FSessionData->NotUtf)
    {
      case asOn:
        UtfCombo->setCurrentIndex(1);
        break;

      case asOff:
        UtfCombo->setCurrentIndex(2);
        break;

      default:
        UtfCombo->setCurrentIndex(0);
        break;
    }

    int TimeDifferenceMin = TimeToMinutes(FSessionData->TimeDifference);
    TimeDifferenceEdit->setValue(TimeDifferenceMin / MinsPerHour);
    TimeDifferenceMinutesEdit->setValue(TimeDifferenceMin % MinsPerHour);
    TimeDifferenceAutoCheck->setChecked(FSessionData->TimeDifferenceAuto);

    TrimVMSVersionsCheck->setChecked(FSessionData->TrimVMSVersions);
    VMSAllRevisionsCheck->setChecked(FSessionData->VMSAllRevisions);

    // Environment/Recycle bin page
    DeleteToRecycleBinCheck->setChecked(FSessionData->DeleteToRecycleBin);
    OverwrittenToRecycleBinCheck->setChecked(FSessionData->OverwrittenToRecycleBin);
    RecycleBinPathEdit->setText(
      ToQString(!FSessionData->RecycleBinPath.IsEmpty() ? FSessionData->RecycleBinPath : DefaultRecycleBinPath));

    // SFTP page
    if (FSessionData->SftpServer.IsEmpty())
    {
      SftpServerEdit->setEditText(SftpServerEdit->itemText(0));
    }
    else
    {
      SftpServerEdit->setEditText(ToQString(FSessionData->SftpServer));
    }
    AllowScpFallbackCheck->setChecked(FSessionData->FSProtocol == fsSFTP);
    UsePosixRenameCheck->setChecked(FSessionData->UsePosixRename);

    if (FSessionData->SFTPMaxVersion < 0)
    {
      SFTPMaxVersionCombo->setCurrentIndex(0);
    }
    else
    {
      SFTPMaxVersionCombo->setCurrentIndex(FSessionData->SFTPMaxVersion + 1);
    }

    ComboAutoSwitchLoad(SFTPRealPathCombo, FSessionData->SFTPRealPath);
    ComboAutoSwitchLoad(SFTPBugSymlinkCombo, FSessionData->SFTPBug[sbSymlink]);
    ComboAutoSwitchLoad(SFTPBugSignedTSCombo, FSessionData->SFTPBug[sbSignedTS]);

    // FTP page
    FtpAccountEdit->setText(ToQString(FSessionData->FtpAccount));
    PostLoginCommandsMemo->setPlainText(ToQString(FSessionData->PostLoginCommands));
    ComboAutoSwitchLoad(FtpListAllCombo, FSessionData->FtpListAll);
    ComboAutoSwitchLoad(FtpUseMlsdCombo, FSessionData->FtpUseMlsd);
    ComboAutoSwitchLoad(FtpForcePasvIpCombo, FSessionData->FtpForcePasvIp);
    ComboAutoSwitchLoad(FtpHostCombo, FSessionData->FtpHost);

    // S3 page
    S3DefaultReqionCombo->setEditText(ToQString(FSessionData->S3DefaultRegion));
    S3UrlStyleCombo->setCurrentIndex((FSessionData->S3UrlStyle == s3usPath) ? 1 : 0);
    S3RequesterPaysCheck->setChecked(FSessionData->S3RequesterPays);

    UnicodeString S3SessionToken = FSessionData->S3SessionToken;
    UnicodeString S3RoleArn = FSessionData->S3RoleArn;
    if (FSessionData->HasAutoCredentials())
    {
      try
      {
        S3SessionToken = S3EnvSessionToken(FSessionData->S3Profile);
        S3RoleArn = S3EnvRoleArn(FSessionData->S3Profile);
      }
      catch (...)
      {
        // noop
      }
    }
    S3SessionTokenMemo->setPlainText(ToQString(S3SessionToken));
    S3RoleArnEdit->setText(ToQString(S3RoleArn));

    // Authentication page
    SshNoUserAuthCheck->setChecked(FSessionData->SshNoUserAuth);
    TryAgentCheck2->setChecked(FSessionData->TryAgent);
    AuthKICheck->setChecked(FSessionData->AuthKI);
    AuthKIPasswordCheck->setChecked(FSessionData->AuthKIPassword);
    AuthGSSAPICheck3->setChecked(FSessionData->AuthGSSAPI);
    GSSAPIFwdTGTCheck->setChecked(FSessionData->GSSAPIFwdTGT);
    AgentFwdCheck->setChecked(FSessionData->AgentFwd);
    PrivateKeyEdit3->setText(ToQString(FSessionData->PublicKeyFile));
    DetachedCertificateEdit->setText(ToQString(FSessionData->DetachedCertificate));

    // SSH page
    Ssh2LegacyDESCheck->setChecked(FSessionData->Ssh2DES);
    CompressionCheck->setChecked(FSessionData->Compression);

    CipherListBox->clear();
    for (int Index = 0; Index < CIPHER_COUNT; Index++)
    {
      QListWidgetItem * Item = new QListWidgetItem(ToQString(LoadStr(CIPHER_NAME_WARN + FSessionData->Cipher[Index])), CipherListBox);
      Item->setData(Qt::UserRole, static_cast<int>(FSessionData->Cipher[Index]));
    }

    // KEX page
    KexListBox->clear();
    for (int Index = 0; Index < KEX_COUNT; Index++)
    {
      QListWidgetItem * Item = new QListWidgetItem(ToQString(LoadStr(KEX_NAME_WARN + FSessionData->Kex[Index])), KexListBox);
      Item->setData(Qt::UserRole, static_cast<int>(FSessionData->Kex[Index]));
    }

    AuthGSSAPIKEXCheck->setChecked(FSessionData->AuthGSSAPIKEX);

    RekeyTimeEdit->setValue(FSessionData->RekeyTime);
    RekeyDataEdit->setText(ToQString(FSessionData->RekeyData));

    // Connection page
    FtpPasvModeCheck->setChecked(FSessionData->FtpPasvMode);
    BufferSizeCheck->setChecked((FSessionData->SendBuf > 0) && FSessionData->SshSimple);

    switch (FSessionData->PingType)
    {
      case ptNullPacket:
        PingNullPacketButton->setChecked(true);
        break;

      case ptDummyCommand:
        PingDummyCommandButton->setChecked(true);
        break;

      default:
        PingOffButton->setChecked(true);
        break;
    }
    PingIntervalSecEdit->setValue(FSessionData->PingInterval);
    switch (FSessionData->FtpPingType)
    {
      case fptDummyCommand0:
      case fptDummyCommand:
        FtpPingDummyCommandButton->setChecked(true);
        break;

      case fptDirectoryListing:
        FtpPingDirectoryListingButton->setChecked(true);
        break;

      default:
        FtpPingOffButton->setChecked(true);
        break;
    }
    FtpPingIntervalSecEdit->setValue(FSessionData->FtpPingInterval);
    TimeoutEdit->setValue(FSessionData->Timeout);

    switch (FSessionData->AddressFamily)
    {
      case afIPv4:
        IPv4Button->setChecked(true);
        break;

      case afIPv6:
        IPv6Button->setChecked(true);
        break;

      case afAuto:
      default:
        IPAutoButton->setChecked(true);
        break;
    }

    // Shell page
    ShellEdit->setEditText(FSessionData->DefaultShell ? ShellEdit->itemText(0) : ToQString(FSessionData->Shell));
    ReturnVarEdit->setEditText(FSessionData->DetectReturnVar ? ReturnVarEdit->itemText(0) : ToQString(FSessionData->ReturnVar));
    ListingCommandEdit->setEditText(ToQString(FSessionData->ListingCommand));

    CheckBoxAutoSwitchLoad(LookupUserGroupsCheck, FSessionData->LookupUserGroups);
    ClearAliasesCheck->setChecked(FSessionData->ClearAliases);
    IgnoreLsWarningsCheck->setChecked(FSessionData->IgnoreLsWarnings);
    Scp1CompatibilityCheck->setChecked(FSessionData->Scp1Compatibility);
    UnsetNationalVarsCheck->setChecked(FSessionData->UnsetNationalVars);
    SCPLsFullTimeAutoCheck->setChecked(FSessionData->SCPLsFullTime != asOff);

    // Proxy page
    SshProxyMethodCombo->setCurrentIndex(std::min(static_cast<int>(FSessionData->ProxyMethod), SshProxyMethodCombo->count() - 1));
    FtpProxyMethodCombo->setCurrentIndex(GetSupportedFtpProxyMethod(FSessionData->ProxyMethod));
    if (FSessionData->FtpProxyLogonType != 0)
    {
      FtpProxyMethodCombo->setCurrentIndex(LastSupportedFtpProxyMethod() + FSessionData->FtpProxyLogonType);
    }
    NeonProxyMethodCombo->setCurrentIndex(GetSupportedNeonProxyMethod(FSessionData->ProxyMethod));
    ProxyHostEdit->setText(ToQString(FSessionData->ProxyHost));
    ProxyPortEdit->setValue(FSessionData->ProxyPort);
    ProxyUsernameEdit->setText(ToQString(FSessionData->ProxyUsername));
    ProxyPasswordEdit->setText(ToQString(FSessionData->ProxyPassword));
    ProxyTelnetCommandEdit->setText(ToQString(FSessionData->ProxyTelnetCommand));
    ProxyLocalCommandEdit->setText(ToQString(FSessionData->ProxyLocalCommand));
    ProxyLocalhostCheck->setChecked(FSessionData->ProxyLocalhost);
    ProxyDNSCombo->setCurrentIndex(2 - FSessionData->ProxyDNS);

    // Bugs page
    ComboAutoSwitchLoad(BugHMAC2Combo, FSessionData->Bug[sbHMAC2]);
    ComboAutoSwitchLoad(BugDeriveKey2Combo, FSessionData->Bug[sbDeriveKey2]);
    ComboAutoSwitchLoad(BugRSAPad2Combo, FSessionData->Bug[sbRSAPad2]);
    ComboAutoSwitchLoad(BugPKSessID2Combo, FSessionData->Bug[sbPKSessID2]);
    ComboAutoSwitchLoad(BugRekey2Combo, FSessionData->Bug[sbRekey2]);
    ComboAutoSwitchLoad(BugMaxPkt2Combo, FSessionData->Bug[sbMaxPkt2]);
    ComboAutoSwitchLoad(BugIgnore2Combo, FSessionData->Bug[sbIgnore2]);
    ComboAutoSwitchLoad(BugWinAdjCombo, FSessionData->Bug[sbWinAdj]);

    // Tunnel page
    TunnelCheck->setChecked(FSessionData->Tunnel);
    TunnelUserNameEdit->setText(ToQString(FSessionData->TunnelUserName));
    TunnelPortNumberEdit->setValue(FSessionData->TunnelPortNumber);
    TunnelHostNameEdit->setText(ToQString(FSessionData->TunnelHostName));
    TunnelPasswordEdit->setText(ToQString(FSessionData->TunnelPassword));
    TunnelPrivateKeyEdit3->setText(ToQString(FSessionData->TunnelPublicKeyFile));
    if (FSessionData->TunnelAutoassignLocalPortNumber)
    {
      TunnelLocalPortNumberEdit->setEditText(TunnelLocalPortNumberEdit->itemText(0));
    }
    else
    {
      TunnelLocalPortNumberEdit->setEditText(QString::number(FSessionData->TunnelLocalPortNumber));
    }

    // Connection/TLS/SSL page
    MinTlsVersionCombo->setCurrentIndex(TlsVersionToIndex(FSessionData->MinTlsVersion));
    MaxTlsVersionCombo->setCurrentIndex(TlsVersionToIndex(FSessionData->MaxTlsVersion));
    SslSessionReuseCheck2->setChecked(FSessionData->SslSessionReuse);
    TlsCertificateFileEdit->setText(ToQString(FSessionData->TlsCertificateFile));

    // Note page
    NoteMemo->setPlainText(ToQString(FSessionData->Note));

    // Encryption page
    EncryptFilesCheck->setChecked(!FSessionData->EncryptKey.IsEmpty());
    EncryptKeyPasswordEdit->setText(ToQString(FSessionData->EncryptKey));

    // WebDAV page
    WebDavLiberalEscapingCheck->setChecked(FSessionData->WebDavLiberalEscaping);
    WebDavCrossDomainRedirectsCheck->setChecked(FSessionData->WebDavCrossDomainRedirects);

    // Color
    FColor = static_cast<TColor>(FSessionData->Color);
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
bool TSiteAdvancedDialog::IsDefaultSftpServer()
{
  return SftpServerEdit->currentText().isEmpty() || (SftpServerEdit->currentText() == SftpServerEdit->itemText(0));
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::SaveSession(TSessionData * SessionData)
{
  // SSH page
  SessionData->Compression = CompressionCheck->isChecked();
  SessionData->Ssh2DES = Ssh2LegacyDESCheck->isChecked();

  for (int Index = 0; Index < CIPHER_COUNT; Index++)
  {
    SessionData->Cipher[Index] = static_cast<TCipher>(CipherListBox->item(Index)->data(Qt::UserRole).toInt());
  }

  // Kex page
  for (int Index = 0; Index < KEX_COUNT; Index++)
  {
    SessionData->Kex[Index] = static_cast<TKex>(KexListBox->item(Index)->data(Qt::UserRole).toInt());
  }

  SessionData->AuthGSSAPIKEX = AuthGSSAPIKEXCheck->isChecked();

  SessionData->RekeyTime = RekeyTimeEdit->value();
  SessionData->RekeyData = ToUnicodeString(RekeyDataEdit->text());

  // Authentication page
  SessionData->SshNoUserAuth = SshNoUserAuthCheck->isChecked();
  SessionData->TryAgent = TryAgentCheck2->isChecked();
  SessionData->AuthKI = AuthKICheck->isChecked();
  SessionData->AuthKIPassword = AuthKIPasswordCheck->isChecked();
  SessionData->AuthGSSAPI = AuthGSSAPICheck3->isChecked();
  SessionData->GSSAPIFwdTGT = GSSAPIFwdTGTCheck->isChecked();
  SessionData->AgentFwd = AgentFwdCheck->isChecked();
  SessionData->PublicKeyFile = ToUnicodeString(PrivateKeyEdit3->text());
  // When the key has a certificate, the certificate box is disabled and
  // cannot be cleared, so it is not saved
  SessionData->DetachedCertificate = !FKeyHasCertificate ? ToUnicodeString(DetachedCertificateEdit->text()) : EmptyStr;

  // Connection page
  SessionData->FtpPasvMode = FtpPasvModeCheck->isChecked();
  SessionData->SendBuf = BufferSizeCheck->isChecked() ? DefaultSendBuf : 0;
  SessionData->SshSimple = BufferSizeCheck->isChecked();

  if (PingNullPacketButton->isChecked())
  {
    SessionData->PingType = ptNullPacket;
  }
  else if (PingDummyCommandButton->isChecked())
  {
    SessionData->PingType = ptDummyCommand;
  }
  else
  {
    SessionData->PingType = ptOff;
  }
  SessionData->PingInterval = PingIntervalSecEdit->value();
  if (FtpPingDummyCommandButton->isChecked())
  {
    SessionData->FtpPingType = fptDummyCommand;
  }
  else if (FtpPingDirectoryListingButton->isChecked())
  {
    SessionData->FtpPingType = fptDirectoryListing;
  }
  else
  {
    SessionData->FtpPingType = fptOff;
  }
  SessionData->FtpPingInterval = FtpPingIntervalSecEdit->value();
  SessionData->Timeout = TimeoutEdit->value();

  if (IPv4Button->isChecked())
  {
    SessionData->AddressFamily = afIPv4;
  }
  else if (IPv6Button->isChecked())
  {
    SessionData->AddressFamily = afIPv6;
  }
  else
  {
    SessionData->AddressFamily = afAuto;
  }

  // Directories page
  SessionData->SynchronizeBrowsing = SynchronizeBrowsingCheck->isChecked();
  SessionData->LocalDirectory = ToUnicodeString(LocalDirectoryEdit->text());
  SessionData->RemoteDirectory = ToUnicodeString(RemoteDirectoryEdit->text());
  SessionData->UpdateDirectories = UpdateDirectoriesCheck->isChecked();
  SessionData->CacheDirectories = CacheDirectoriesCheck->isChecked();
  SessionData->CacheDirectoryChanges = CacheDirectoryChangesCheck->isChecked();
  SessionData->PreserveDirectoryChanges = PreserveDirectoryChangesCheck->isChecked();
  SessionData->ResolveSymlinks = ResolveSymlinksCheck->isChecked();
  SessionData->FollowDirectorySymlinks = FollowDirectorySymlinksCheck->isChecked();

  // Environment page
  SessionData->DSTMode = DSTModeUnixCheck->isChecked() ? dstmUnix : dstmWin;
  SessionData->EOLType = (EOLTypeCombo->currentIndex() == 0) ? eolLF : eolCRLF;

  switch (UtfCombo->currentIndex())
  {
    case 1:
      SessionData->NotUtf = asOn;
      break;

    case 2:
      SessionData->NotUtf = asOff;
      break;

    default:
      SessionData->NotUtf = asAuto;
      break;
  }
  SessionData->TimeDifference =
    (double(TimeDifferenceEdit->value()) / HoursPerDay) +
    (double(TimeDifferenceMinutesEdit->value()) / MinsPerDay);
  SessionData->TimeDifferenceAuto = TimeDifferenceAutoCheck->isChecked();

  SessionData->TrimVMSVersions = TrimVMSVersionsCheck->isChecked();
  SessionData->VMSAllRevisions = VMSAllRevisionsCheck->isChecked();

  // Environment/Recycle bin page
  SessionData->DeleteToRecycleBin = DeleteToRecycleBinCheck->isChecked();
  SessionData->OverwrittenToRecycleBin = OverwrittenToRecycleBinCheck->isChecked();
  UnicodeString RecycleBinPath = ToUnicodeString(RecycleBinPathEdit->text());
  SessionData->RecycleBinPath =
    (SessionData->DeleteToRecycleBin || SessionData->OverwrittenToRecycleBin ||
     (RecycleBinPath != DefaultRecycleBinPath)) ?
      RecycleBinPath : UnicodeString();

  // SCP page
  SessionData->DefaultShell = (ShellEdit->currentText() == ShellEdit->itemText(0));
  SessionData->Shell = (SessionData->DefaultShell ? UnicodeString() : ToUnicodeString(ShellEdit->currentText()));
  SessionData->DetectReturnVar = (ReturnVarEdit->currentText() == ReturnVarEdit->itemText(0));
  SessionData->ReturnVar = (SessionData->DetectReturnVar ? UnicodeString() : ToUnicodeString(ReturnVarEdit->currentText()));
  SessionData->ListingCommand = ToUnicodeString(ListingCommandEdit->currentText());
  SessionData->LookupUserGroups = CheckBoxAutoSwitchSave(LookupUserGroupsCheck);
  SessionData->ClearAliases = ClearAliasesCheck->isChecked();
  SessionData->IgnoreLsWarnings = IgnoreLsWarningsCheck->isChecked();
  SessionData->Scp1Compatibility = Scp1CompatibilityCheck->isChecked();
  SessionData->UnsetNationalVars = UnsetNationalVarsCheck->isChecked();
  SessionData->SCPLsFullTime = SCPLsFullTimeAutoCheck->isChecked() ? asAuto : asOff;

  // SFTP page
  SessionData->SftpServer = (IsDefaultSftpServer() ? UnicodeString() : ToUnicodeString(SftpServerEdit->currentText()));
  if (SFTPMaxVersionCombo->currentIndex() == 0)
  {
    SessionData->SFTPMaxVersion = SFTPMaxVersionAuto;
  }
  else
  {
    SessionData->SFTPMaxVersion = SFTPMaxVersionCombo->currentIndex() - 1;
  }
  if (AllowScpFallbackCheck->isChecked() != (SessionData->FSProtocol == fsSFTP))
  {
    if (AllowScpFallbackCheck->isChecked())
    {
      DebugAssert(SessionData->FSProtocol == fsSFTPonly);
      SessionData->FSProtocol = fsSFTP;
    }
    else
    {
      DebugAssert(SessionData->FSProtocol == fsSFTP);
      SessionData->FSProtocol = fsSFTPonly;
    }
  }
  SessionData->UsePosixRename = UsePosixRenameCheck->isChecked();

  SessionData->SFTPRealPath = ComboAutoSwitchSave(SFTPRealPathCombo);
  SessionData->SFTPBug[sbSymlink] = ComboAutoSwitchSave(SFTPBugSymlinkCombo);
  SessionData->SFTPBug[sbSignedTS] = ComboAutoSwitchSave(SFTPBugSignedTSCombo);

  // FTP page
  SessionData->FtpAccount = ToUnicodeString(FtpAccountEdit->text());
  SessionData->PostLoginCommands = ToUnicodeString(PostLoginCommandsMemo->toPlainText());
  SessionData->FtpListAll = ComboAutoSwitchSave(FtpListAllCombo);
  SessionData->FtpUseMlsd = ComboAutoSwitchSave(FtpUseMlsdCombo);
  SessionData->FtpForcePasvIp = ComboAutoSwitchSave(FtpForcePasvIpCombo);
  SessionData->FtpHost = ComboAutoSwitchSave(FtpHostCombo);

  // S3 page
  SessionData->S3DefaultRegion = ToUnicodeString(S3DefaultReqionCombo->currentText());
  SessionData->S3UrlStyle = (S3UrlStyleCombo->currentIndex() == 1) ? s3usPath : s3usVirtualHost;
  SessionData->S3RequesterPays = S3RequesterPaysCheck->isChecked();
  if (SessionData->HasAutoCredentials())
  {
    SessionData->S3SessionToken = EmptyStr;
  }
  else
  {
    // Trim not to try to authenticate with a stray new-line
    SessionData->S3SessionToken = ToUnicodeString(S3SessionTokenMemo->toPlainText()).Trim();
  }
  SessionData->S3RoleArn = ToUnicodeString(S3RoleArnEdit->text());

  // Proxy page
  SessionData->ProxyMethod = GetProxyMethod();
  SessionData->FtpProxyLogonType = GetFtpProxyLogonType();
  SessionData->ProxyHost = ToUnicodeString(ProxyHostEdit->text());
  SessionData->ProxyPort = ProxyPortEdit->value();
  SessionData->ProxyUsername = ToUnicodeString(ProxyUsernameEdit->text());
  SessionData->ProxyPassword = ToUnicodeString(ProxyPasswordEdit->text());
  SessionData->ProxyTelnetCommand = ToUnicodeString(ProxyTelnetCommandEdit->text());
  SessionData->ProxyLocalCommand = ToUnicodeString(ProxyLocalCommandEdit->text());
  SessionData->ProxyLocalhost = ProxyLocalhostCheck->isChecked();
  SessionData->ProxyDNS = static_cast<TAutoSwitch>(2 - ProxyDNSCombo->currentIndex());

  // Bugs page
  SessionData->Bug[sbHMAC2] = ComboAutoSwitchSave(BugHMAC2Combo);
  SessionData->Bug[sbDeriveKey2] = ComboAutoSwitchSave(BugDeriveKey2Combo);
  SessionData->Bug[sbRSAPad2] = ComboAutoSwitchSave(BugRSAPad2Combo);
  SessionData->Bug[sbPKSessID2] = ComboAutoSwitchSave(BugPKSessID2Combo);
  SessionData->Bug[sbRekey2] = ComboAutoSwitchSave(BugRekey2Combo);
  SessionData->Bug[sbMaxPkt2] = ComboAutoSwitchSave(BugMaxPkt2Combo);
  SessionData->Bug[sbIgnore2] = ComboAutoSwitchSave(BugIgnore2Combo);
  SessionData->Bug[sbWinAdj] = ComboAutoSwitchSave(BugWinAdjCombo);

  // Tunnel page
  SessionData->Tunnel = TunnelCheck->isChecked();
  SessionData->TunnelUserName = ToUnicodeString(TunnelUserNameEdit->text());
  SessionData->TunnelPortNumber = TunnelPortNumberEdit->value();
  SessionData->TunnelHostName = ToUnicodeString(TunnelHostNameEdit->text());
  SessionData->TunnelPassword = ToUnicodeString(TunnelPasswordEdit->text());
  SessionData->TunnelPublicKeyFile = ToUnicodeString(TunnelPrivateKeyEdit3->text());
  if (TunnelLocalPortNumberEdit->currentText() == TunnelLocalPortNumberEdit->itemText(0))
  {
    SessionData->TunnelLocalPortNumber = 0;
  }
  else
  {
    SessionData->TunnelLocalPortNumber = StrToIntDef(ToUnicodeString(TunnelLocalPortNumberEdit->currentText()), 0);
  }

  // Connection/TLS/SSL page
  SessionData->MinTlsVersion = IndexToTlsVersion(MinTlsVersionCombo->currentIndex());
  SessionData->MaxTlsVersion = IndexToTlsVersion(MaxTlsVersionCombo->currentIndex());
  SessionData->SslSessionReuse = SslSessionReuseCheck2->isChecked();
  SessionData->TlsCertificateFile = ToUnicodeString(TlsCertificateFileEdit->text());

  // Note page
  SessionData->Note = ToUnicodeString(NoteMemo->toPlainText());

  // Encryption page
  SessionData->EncryptKey = EncryptFilesCheck->isChecked() ? ToUnicodeString(EncryptKeyPasswordEdit->text()) : UnicodeString();

  // WebDAV page
  SessionData->WebDavLiberalEscaping = WebDavLiberalEscapingCheck->isChecked();
  SessionData->WebDavCrossDomainRedirects = WebDavCrossDomainRedirectsCheck->isChecked();

  // Color
  SessionData->Color = FColor;
}
//---------------------------------------------------------------------------
TSessionData * TSiteAdvancedDialog::GetSessionData()
{
  std::unique_ptr<TSessionData> SessionData(new TSessionData(UnicodeString()));
  SessionData->Assign(FSessionData);
  SaveSession(SessionData.get());
  return SessionData.release();
}
//---------------------------------------------------------------------------
bool TSiteAdvancedDialog::HasCertificate(const UnicodeString & FileName)
{
  bool Result = false;
  try
  {
    UnicodeString UnusedComment;
    GetPublicKeyLine(FileName, UnusedComment, Result);
  }
  catch (...)
  {
  }
  return Result;
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::UpdateControls()
{
  if ((FNoUpdate > 0) || (FSessionData == NULL))
  {
    return;
  }
  TAutoNestingCounter NoUpdateCounter(FNoUpdate);

  bool SshProtocol = FSessionData->UsesSsh;
  bool SftpProtocol = (NormalizeFSProtocol(FSessionData->FSProtocol) == fsSFTP);
  bool ScpProtocol = (FSessionData->FSProtocol == fsSCPonly);
  bool FtpProtocol = (FSessionData->FSProtocol == fsFTP);
  bool WebDavProtocol = (FSessionData->FSProtocol == fsWebDAV);
  bool S3Protocol = (FSessionData->FSProtocol == fsS3);
  bool Neon = IsNeon(FSessionData->FSProtocol);
  bool Ssl = (FtpProtocol || WebDavProtocol || S3Protocol) && (FSessionData->Ftps != ftpsNone);

  // Connection sheet
  FtpPasvModeCheck->setEnabled(FtpProtocol);
  if (FtpProtocol &&
      (FtpProxyMethodCombo->currentIndex() != pmNone) &&
      SupportedFtpProxyMethod(FtpProxyMethodCombo->currentIndex()) &&
      !FtpPasvModeCheck->isChecked())
  {
    FtpPasvModeCheck->setChecked(true);
    MessageDialog(MainInstructions(LoadStr(FTP_PASV_MODE_REQUIRED)), qtInformation, qaOK);
  }
  BufferSizeCheck->setEnabled(SshProtocol || FtpProtocol || S3Protocol);
  PingGroup->setVisible(!FtpProtocol);
  PingGroup->setEnabled(SshProtocol);
  PingIntervalSecEdit->setEnabled(PingGroup->isEnabled() && !PingOffButton->isChecked());
  PingIntervalLabel->setEnabled(PingIntervalSecEdit->isEnabled());
  FtpPingGroup->setVisible(FtpProtocol);
  FtpPingIntervalSecEdit->setEnabled(!FtpPingOffButton->isChecked());
  FtpPingIntervalLabel->setEnabled(FtpPingIntervalSecEdit->isEnabled());
  IPvGroup->setEnabled(SshProtocol || FtpProtocol);
  IPAutoButton->setEnabled(IPvGroup->isEnabled() && SshProtocol);

  // SSH/Authentication sheet
  SetPageVisible(spAuthentication, SshProtocol);
  AuthenticationGroup->setEnabled(!SshNoUserAuthCheck->isChecked());
  AuthKIPasswordCheck->setEnabled(AuthenticationGroup->isEnabled() && AuthKICheck->isChecked());
  AuthenticationParamsGroup->setEnabled(AuthenticationGroup->isEnabled());
  AgentFwdCheck->setEnabled(AuthenticationParamsGroup->isEnabled() && TryAgentCheck2->isChecked());
  UnicodeString PrivateKey = ToUnicodeString(PrivateKeyEdit3->text());
  if (PrivateKey != FLastPrivateKey)
  {
    FLastPrivateKey = PrivateKey;
    FKeyHasCertificate = !FLastPrivateKey.IsEmpty() && HasCertificate(ExpandEnvironmentVariables(FLastPrivateKey));
  }
  PrivateKeyViewButton->setEnabled(AuthenticationParamsGroup->isEnabled() && !PrivateKey.IsEmpty());
  DetachedCertificateEdit->setEnabled(PrivateKeyViewButton->isEnabled() && !FKeyHasCertificate);
  DetachedCertificateLabel->setEnabled(DetachedCertificateEdit->isEnabled());
  if (QWidget * Browse = findChild<QWidget *>(QStringLiteral("DetachedCertificateEditBrowse")))
  {
    Browse->setEnabled(DetachedCertificateEdit->isEnabled());
  }
  AuthGSSAPICheck3->setEnabled(AuthenticationGroup->isEnabled());
  GSSAPIFwdTGTCheck->setEnabled(AuthGSSAPICheck3->isEnabled() && AuthGSSAPICheck3->isChecked());

  // SSH sheet
  SetPageVisible(spSsh, SshProtocol);
  CipherUpButton->setEnabled(CipherListBox->currentRow() > 0);
  CipherDownButton->setEnabled((CipherListBox->currentRow() >= 0) && (CipherListBox->currentRow() < CipherListBox->count() - 1));

  // SSH/Key exchange sheet
  SetPageVisible(spKex, SshProtocol);
  KexUpButton->setEnabled(KexListBox->currentRow() > 0);
  KexDownButton->setEnabled((KexListBox->currentRow() >= 0) && (KexListBox->currentRow() < KexListBox->count() - 1));
  KexReexchangeGroup->setEnabled(BugRekey2Combo->currentIndex() != 2);

  // SSH/Bugs sheet
  SetPageVisible(spBugs, SshProtocol);

  // Connection/Proxy sheet
  QComboBox * ProxyMethodCombo =
    (SshProtocol ? SshProxyMethodCombo : (FtpProtocol ? FtpProxyMethodCombo : NeonProxyMethodCombo));
  TProxyMethod ProxyMethod = GetProxyMethod();
  ProxyMethodLabel->setBuddy(ProxyMethodCombo);
  for (QComboBox * Combo : { SshProxyMethodCombo, FtpProxyMethodCombo, NeonProxyMethodCombo })
  {
    Combo->setVisible(Combo == ProxyMethodCombo);
  }
  if (!SshProtocol)
  {
    SshProxyMethodCombo->setCurrentIndex(ProxyMethod);
  }
  if (!FtpProtocol)
  {
    FtpProxyMethodCombo->setCurrentIndex(GetSupportedFtpProxyMethod(ProxyMethod));
  }
  if (!Neon)
  {
    NeonProxyMethodCombo->setCurrentIndex(GetSupportedNeonProxyMethod(ProxyMethod));
  }

  int FtpProxyLogonType = GetFtpProxyLogonType();
  UnicodeString ProxyCommand =
    ToUnicodeString((ProxyMethod == pmCmd) ? ProxyLocalCommandEdit->text() : ProxyTelnetCommandEdit->text());
  ProxyHostEdit->setEnabled(
    (ProxyMethod == pmSocks4) ||
    (ProxyMethod == pmSocks5) ||
    (ProxyMethod == pmHTTP) ||
    (ProxyMethod == pmTelnet) ||
    ((ProxyMethod == pmCmd) && AnsiContainsText(ProxyCommand, L"%proxyhost")) ||
    (FtpProxyLogonType > 0));
  ProxyHostLabel->setEnabled(ProxyHostEdit->isEnabled());
  ProxyPortEdit->setEnabled(
    (ProxyMethod == pmSocks4) ||
    (ProxyMethod == pmSocks5) ||
    (ProxyMethod == pmHTTP) ||
    (ProxyMethod == pmTelnet) ||
    ((ProxyMethod == pmCmd) && AnsiContainsText(ProxyCommand, L"%proxyport")) ||
    (FtpProxyLogonType > 0));
  ProxyPortLabel->setEnabled(ProxyPortEdit->isEnabled());
  ProxyUsernameEdit->setEnabled(
    // FZAPI does not support username for SOCKS4
    ((ProxyMethod == pmSocks4) && (SshProtocol || Neon)) ||
    (ProxyMethod == pmSocks5) ||
    (ProxyMethod == pmHTTP) ||
    (((ProxyMethod == pmTelnet) ||
      (ProxyMethod == pmCmd)) &&
     AnsiContainsText(ProxyCommand, L"%user")) ||
    ((FtpProxyLogonType > 0) && (FtpProxyLogonType != 3) && (FtpProxyLogonType != 5)));
  ProxyUsernameLabel->setEnabled(ProxyUsernameEdit->isEnabled());
  ProxyPasswordEdit->setEnabled(
    (ProxyMethod == pmSocks5) ||
    (ProxyMethod == pmHTTP) ||
    (((ProxyMethod == pmTelnet) ||
      (ProxyMethod == pmCmd)) &&
     AnsiContainsText(ProxyCommand, L"%pass")) ||
    ((FtpProxyLogonType > 0) && (FtpProxyLogonType != 3) && (FtpProxyLogonType != 5)));
  ProxyPasswordLabel->setEnabled(ProxyPasswordEdit->isEnabled());
  bool ProxySettings = (ProxyMethod != pmNone) && SshProtocol;
  ProxySettingsGroup->setVisible(SshProtocol);
  ProxySettingsGroup->setEnabled(ProxySettings);
  ProxyTelnetCommandEdit->setEnabled(ProxySettings && (ProxyMethod == pmTelnet));
  ProxyTelnetCommandLabel->setEnabled(ProxyTelnetCommandEdit->isEnabled());
  bool LocalCommand = (ProxyMethod == pmCmd);
  ProxyLocalCommandEdit->setVisible(LocalCommand);
  ProxyLocalCommandLabel->setVisible(LocalCommand);
  ProxyLocalCommandBrowseButton->setVisible(LocalCommand);
  ProxyTelnetCommandEdit->setVisible(!LocalCommand);
  ProxyTelnetCommandLabel->setVisible(!LocalCommand);

  // Environment/Directories sheet
  SynchronizeBrowsingCheck->setEnabled(
    AppConfiguration->PreservePanelState && !AppConfiguration->PreserveLocalDirectory);
  CacheDirectoryChangesCheck->setEnabled(!ScpProtocol || CacheDirectoriesCheck->isChecked());
  PreserveDirectoryChangesCheck->setEnabled(CacheDirectoryChangesCheck->isEnabled() && CacheDirectoryChangesCheck->isChecked());
  ResolveSymlinksCheck->setEnabled(SftpProtocol || ScpProtocol);
  FollowDirectorySymlinksCheck->setEnabled(SftpProtocol || ScpProtocol || FtpProtocol);

  // Environment sheet
  EOLTypeCombo->setEnabled(SftpProtocol || ScpProtocol);
  EOLTypeLabel->setEnabled(EOLTypeCombo->isEnabled());
  DSTModeGroup->setEnabled(SftpProtocol || ScpProtocol);
  UtfCombo->setEnabled(SftpProtocol || FtpProtocol || ScpProtocol);
  UtfLabel->setEnabled(UtfCombo->isEnabled());
  // should be enabled for fsSFTP (SCP fallback) too, but it would cause confusion
  bool FtpProtocolWithMlsdOff = FtpProtocol && (ComboAutoSwitchSave(FtpUseMlsdCombo) == asOff);
  TimeDifferenceAutoCheck->setEnabled(FtpProtocolWithMlsdOff);
  TimeDifferenceEdit->setEnabled((FtpProtocolWithMlsdOff && !TimeDifferenceAutoCheck->isChecked()) || ScpProtocol);
  TimeDifferenceLabel->setEnabled(TimeDifferenceEdit->isEnabled() || TimeDifferenceAutoCheck->isEnabled());
  TimeDifferenceHoursLabel->setEnabled(TimeDifferenceEdit->isEnabled());
  TimeDifferenceMinutesEdit->setEnabled(TimeDifferenceEdit->isEnabled());
  TimeDifferenceMinutesLabel->setEnabled(TimeDifferenceEdit->isEnabled());
  TrimVMSVersionsCheck->setEnabled(!S3Protocol);

  // Environment/Recycle bin sheet
  OverwrittenToRecycleBinCheck->setEnabled(SftpProtocol);
  RecycleBinPathEdit->setEnabled(
    DeleteToRecycleBinCheck->isChecked() ||
    (OverwrittenToRecycleBinCheck->isEnabled() && OverwrittenToRecycleBinCheck->isChecked()));
  RecycleBinPathLabel->setEnabled(RecycleBinPathEdit->isEnabled());

  // Environment/SFTP sheet
  SetPageVisible(spSftp, SftpProtocol);
  AllowScpFallbackCheck->setEnabled(IsDefaultSftpServer());

  // Environment/SCP/Shell sheet
  SetPageVisible(spScp, SshProtocol);
  NodeOf(spScp)->setText(0, ToQString(LoadStr(ScpProtocol ? LOGIN_SCP_SHELL_PAGE : LOGIN_SHELL_PAGE)));
  // hidden also for SFTP with SCP fallback, as one who wants to configure
  // these probably intends to use SCP and should select it explicitly
  ScpLsOptionsGroup->setVisible(ScpProtocol);
  OtherShellOptionsGroup->setVisible(ScpProtocol);

  // Environment/FTP sheet
  SetPageVisible(spFtp, FtpProtocol);
  FtpListAllCombo->setEnabled(ComboAutoSwitchSave(FtpUseMlsdCombo) == asOff);
  FtpListAllLabel->setEnabled(FtpListAllCombo->isEnabled());
  FtpForcePasvIpCombo->setEnabled(FtpPasvModeCheck->isChecked() && (IPAutoButton->isChecked() || IPv4Button->isChecked()));
  FtpForcePasvIpLabel->setEnabled(FtpForcePasvIpCombo->isEnabled());

  // Environment/S3 sheet
  SetPageVisible(spS3, S3Protocol);
  S3SessionTokenMemo->setEnabled(S3Protocol && !FSessionData->HasAutoCredentials());
  S3SessionTokenLabel->setEnabled(S3SessionTokenMemo->isEnabled());
  S3RoleArnEdit->setEnabled(S3SessionTokenMemo->isEnabled() && IsAmazonS3SessionData(FSessionData));
  S3RoleArnLabel->setEnabled(S3RoleArnEdit->isEnabled());

  // Connection/Tunnel sheet
  SetPageVisible(spTunnel, SshProtocol);
  TunnelSessionGroup->setEnabled(TunnelCheck->isChecked());
  TunnelOptionsGroup->setEnabled(TunnelSessionGroup->isEnabled());
  TunnelAuthenticationParamsGroup->setEnabled(TunnelSessionGroup->isEnabled());

  // Connection/TLS/SSL sheet
  SetPageVisible(spSsl, Ssl);
  // TLS session reuse is not configurable for WebDAV/S3 yet
  SslSessionReuseCheck2->setEnabled(Ssl && FtpProtocol);
  TlsAuthenticationGroup->setVisible(Ssl && (FtpProtocol || WebDavProtocol));

  // Environment/Encryption sheet
  SetPageVisible(spEncryption, SftpProtocol);
  EncryptFilesGroup->setEnabled(EncryptFilesCheck->isChecked());

  // Environment/WebDAV sheet
  SetPageVisible(spWebDav, WebDavProtocol);

  // A hidden page is not shown
  QTreeWidgetItem * Current = NavigationTree->currentItem();
  if ((Current == nullptr) || Current->isHidden())
  {
    SetActivePage(spEnvironment);
  }

  UpdateColorButton();
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::UpdateColorButton()
{
  // upstream's color image of the button
  if (FColor == TColor(0))
  {
    ColorButton->setIcon(QIcon());
  }
  else
  {
    int Size = fontMetrics().height();
    ColorButton->setIcon(ColorIcon(FColor, QSize(Size, Size)));
  }
}
//---------------------------------------------------------------------------
bool TSiteAdvancedDialog::Execute(TSessionData * SessionData)
{
  FSessionData = SessionData;
  LoadSession();
  SetActivePage(spEnvironment);
  NavigationTree->setFocus();

  bool Result = (exec() == QDialog::Accepted);

  if (Result)
  {
    SaveSession(SessionData);
  }

  return Result;
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::ValidateEncryptKey()
{
  if (EncryptFilesCheck->isChecked())
  {
    UnicodeString HexKey = ToUnicodeString(EncryptKeyPasswordEdit->text());
    if (!HexKey.IsEmpty())
    {
      try
      {
        ::ValidateEncryptKey(HexToBytes(HexKey));
      }
      catch (...)
      {
        SetActivePage(spEncryption);
        EncryptKeyPasswordEdit->setFocus();
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::accept()
{
  bool Valid = false;
  ExecuteHandler([&]()
  {
    VerifyKey(StripPathQuotes(ToUnicodeString(PrivateKeyEdit3->text())));
    VerifyKey(StripPathQuotes(ToUnicodeString(TunnelPrivateKeyEdit3->text())));
    VerifyCertificate(StripPathQuotes(ToUnicodeString(TlsCertificateFileEdit->text())));
    ValidateEncryptKey();
    Valid = true;
  });
  if (Valid)
  {
    QDialog::accept();
  }
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::PrivateKeyAfterDialog(UnicodeString & Name)
{
  VerifyAndConvertKey(Name, true);

  if (!Name.IsEmpty() && !HasCertificate(Name))
  {
    try
    {
      UnicodeString FileName = ExpandEnvironmentVariables(Name);
      TKeyType Type = KeyType(FileName);
      if ((Type == ktSSH2) || (Type == ktOpenSSHPEM) || (Type == ktOpenSSHNew) || (Type == ktSSHCom))
      {
        // This fails for encrypted keys
        TPrivateKey * PrivateKey = LoadKey(Type, FileName, EmptyStr);
        ::rtl::TFinally Finally([&]() { FreeKey(PrivateKey); });
        UnicodeString CertificateFileName = AddMatchingKeyCertificate(PrivateKey, FileName);
        DetachedCertificateEdit->setText(ToQString(CertificateFileName));
      }
    }
    catch (Exception &)
    {
      // swallow
    }
  }
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::PrivateKeyViewButtonClick()
{
  UnicodeString FileName = ToUnicodeString(PrivateKeyEdit3->text());
  VerifyAndConvertKey(FileName, false);
  PrivateKeyEdit3->setText(ToQString(FileName));
  UnicodeString CommentDummy;
  bool HasCertificate;
  UnicodeString Line = GetPublicKeyLine(FileName, CommentDummy, HasCertificate);
  std::unique_ptr<TStrings> Messages(TextToStringList(Line));

  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text = Line;

  TMessageParams Params;
  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
  Aliases[0].OnSubmit = ::rtl::MethodPtr(&ClipboardHandler, &TClipboardHandler::Copy);
  Params.Aliases = Aliases;
  Params.AliasesCount = std::size(Aliases);

  UnicodeString Message = LoadStr(HasCertificate ? LOGIN_KEY_WITH_CERTIFICATE : LOGIN_AUTHORIZED_KEYS);
  unsigned int Answers = qaOK | qaRetry;
  MoreMessageDialog(Message, Messages.get(), qtInformation, Answers, &Params);
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::PrivateKeyUploadItemClick()
{
  std::unique_ptr<TSessionData> SessionData(GetSessionData());
  // No SCP fallback, as SCP does not implement GetHomeDirectory
  SessionData->FSProtocol = fsSFTPonly;
  SessionData->RemoteDirectory = UnicodeString();

  UnicodeString FileName = ToUnicodeString(PrivateKeyEdit3->text());
  if (TTerminalManager::Instance()->UploadPublicKey(NULL, SessionData.get(), FileName))
  {
    PrivateKeyEdit3->setText(ToQString(FileName));
    PrivateKeyEdit3->setFocus();
  }
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::GenerateKeyButtonClick()
{
  UnicodeString Key = BytesToHex(GenerateEncryptKey());
  EncryptKeyPasswordEdit->setText(ToQString(Key));

  TClipboardHandler ClipboardHandler;
  ClipboardHandler.Text = Key;

  TMessageParams Params;
  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(COPY_KEY_BUTTON);
  Aliases[0].OnSubmit = ::rtl::MethodPtr(&ClipboardHandler, &TClipboardHandler::Copy);
  Params.Aliases = Aliases;
  Params.AliasesCount = std::size(Aliases);

  MessageDialog(LoadStr(ENCRYPT_KEY_GENERATED), qtInformation, qaOK | qaRetry, &Params);
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::ProxyAutodetectButtonClick()
{
  UnicodeString ProxyHost;
  int ProxyPort;
  if (AutodetectProxy(ProxyHost, ProxyPort))
  {
    ProxyHostEdit->setText(ToQString(ProxyHost));
    ProxyPortEdit->setValue(ProxyPort);

    SshProxyMethodCombo->setCurrentIndex(pmHTTP);
    FtpProxyMethodCombo->setCurrentIndex(pmHTTP);
    NeonProxyMethodCombo->setCurrentIndex(pmHTTP);

    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TSiteAdvancedDialog::ColorButtonClick()
{
  if (ColorMenuPopup(ColorButton->mapToGlobal(QPoint(0, ColorButton->height())), FColor, true))
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
int TSiteAdvancedDialog::LastSupportedFtpProxyMethod()
{
  return pmHTTP;
}
//---------------------------------------------------------------------------
bool TSiteAdvancedDialog::SupportedFtpProxyMethod(int Method)
{
  return (Method >= 0) && (Method <= LastSupportedFtpProxyMethod());
}
//---------------------------------------------------------------------------
int TSiteAdvancedDialog::GetSupportedFtpProxyMethod(int Method)
{
  return SupportedFtpProxyMethod(Method) ? Method : static_cast<int>(pmNone);
}
//---------------------------------------------------------------------------
int TSiteAdvancedDialog::GetSupportedNeonProxyMethod(int Method)
{
  return ((Method >= 0) && (Method <= pmHTTP)) ? Method : static_cast<int>(pmNone);
}
//---------------------------------------------------------------------------
bool TSiteAdvancedDialog::IsNeon(TFSProtocol FSProtocol)
{
  return (FSProtocol == fsWebDAV) || (FSProtocol == fsS3);
}
//---------------------------------------------------------------------------
TProxyMethod TSiteAdvancedDialog::GetProxyMethod()
{
  TProxyMethod Result;
  TFSProtocol FSProtocol = FSessionData->FSProtocol;
  if (FSessionData->UsesSsh)
  {
    Result = static_cast<TProxyMethod>(SshProxyMethodCombo->currentIndex());
  }
  else if (FSProtocol == fsFTP)
  {
    if (SupportedFtpProxyMethod(FtpProxyMethodCombo->currentIndex()))
    {
      Result = static_cast<TProxyMethod>(FtpProxyMethodCombo->currentIndex());
    }
    else
    {
      Result = pmNone;
    }
  }
  else if (IsNeon(FSProtocol))
  {
    Result = static_cast<TProxyMethod>(NeonProxyMethodCombo->currentIndex());
  }
  else
  {
    DebugFail();
    Result = pmNone;
  }
  return Result;
}
//---------------------------------------------------------------------------
int TSiteAdvancedDialog::GetFtpProxyLogonType()
{
  int Result;
  if (FSessionData->FSProtocol != fsFTP)
  {
    Result = 0;
  }
  else if (SupportedFtpProxyMethod(FtpProxyMethodCombo->currentIndex()))
  {
    Result = 0;
  }
  else
  {
    Result = FtpProxyMethodCombo->currentIndex() - LastSupportedFtpProxyMethod();
  }
  return Result;
}
//---------------------------------------------------------------------------
TTlsVersion TSiteAdvancedDialog::IndexToTlsVersion(int Index)
{
  switch (Index)
  {
    default:
      DebugFail();
    case 0:
      return tls10;
    case 1:
      return tls11;
    case 2:
      return tls12;
    case 3:
      return tls13;
  }
}
//---------------------------------------------------------------------------
int TSiteAdvancedDialog::TlsVersionToIndex(TTlsVersion TlsVersion)
{
  switch (TlsVersion)
  {
    default:
      DebugFail();
    case ssl2:
    case ssl3:
    case tls10:
      return 0;
    case tls11:
      return 1;
    case tls12:
      return 2;
    case tls13:
      return 3;
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoSiteAdvancedDialog(TSessionData * SessionData)
{
  TSiteAdvancedDialog Dialog(nullptr);
  return Dialog.Execute(SessionData);
}
//---------------------------------------------------------------------------
