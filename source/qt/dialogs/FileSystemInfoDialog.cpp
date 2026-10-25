// The information about the server and the protocol (upstream's
// forms/FileSystemInfo.dfm): the protocol, the host key and certificate
// fingerprints, the capabilities, the space available.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <Interface.h>
#include <QtInterface.h>
#include <Terminal.h>
#include <TextsWin.h>

#include <QApplication>
#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
class TFileSystemInfoDialog : public QDialog
{
public:
  TFileSystemInfoDialog(QWidget * Parent, TGetSpaceAvailableEvent OnGetSpaceAvailable);

  void Execute(const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
    const UnicodeString & SpaceAvailablePath);

private:
  typedef std::function<void(QWidget * Control, int Label, UnicodeString Value)> TAddItem;

  TGetSpaceAvailableEvent FOnGetSpaceAvailable;
  TSessionInfo FSessionInfo;
  TFileSystemInfo FFileSystemInfo;
  TSpaceAvailable FSpaceAvailable;
  bool FSpaceAvailableLoaded;
  QWidget * FLastFeededControl;
  int FLastListItem;
  UnicodeString FClipboard;

  QTabWidget * PageControl;
  QWidget * SpaceAvailableSheet;
  QGroupBox * HostKeyGroup;
  QLineEdit * HostKeyAlgorithmEdit;
  QLineEdit * HostKeyFingerprintSHA256Edit;
  QLineEdit * HostKeyFingerprintMD5Edit;
  QGroupBox * CertificateGroup;
  QLineEdit * CertificateFingerprintSha256Edit;
  QLineEdit * CertificateFingerprintSha1Edit;
  QTreeWidget * ServerView;
  QTreeWidget * ProtocolView;
  QGroupBox * InfoGroup;
  QPlainTextEdit * InfoMemo;
  QLineEdit * SpaceAvailablePathEdit;
  QPushButton * SpaceAvailableButton;
  QTreeWidget * SpaceAvailableView;

  QTreeWidget * CreateListView(QWidget * Parent, const char * Name);
  QLineEdit * AddFingerprint(QGridLayout * Grid, QWidget * Parent, const char * Name, const QString & Caption);
  UnicodeString CapabilityStr(TFSCapability Capability);
  UnicodeString CapabilityStr(TFSCapability Capability1, TFSCapability Capability2);
  static UnicodeString SpaceStr(__int64 Bytes);
  void Feed(TAddItem AddItem);
  void ControlsAddItem(QWidget * Control, int Label, UnicodeString Value);
  void ClipboardAddItem(QWidget * Control, int Label, UnicodeString Value);
  void FeedControls();
  void UpdateControls();
  void CheckSpaceAvailable();
  void NeedSpaceAvailable();
  bool SpaceAvailableSupported() const { return FOnGetSpaceAvailable != nullptr; }
};
//---------------------------------------------------------------------------
TFileSystemInfoDialog::TFileSystemInfoDialog(QWidget * Parent, TGetSpaceAvailableEvent OnGetSpaceAvailable) :
  QDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("FileSystemInfoDialog"));
  setWindowTitle(tr("Server and protocol information"));
  FOnGetSpaceAvailable = OnGetSpaceAvailable;
  FSpaceAvailableLoaded = false;
  FLastFeededControl = nullptr;
  FLastListItem = 0;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  PageControl = new QTabWidget(this);
  PageControl->setObjectName(QStringLiteral("PageControl"));
  MainLayout->addWidget(PageControl, 1);

  QWidget * ProtocolSheet = new QWidget(PageControl);
  ProtocolSheet->setObjectName(QStringLiteral("ProtocolSheet"));
  QVBoxLayout * ProtocolLayout = new QVBoxLayout(ProtocolSheet);
  ServerView = CreateListView(ProtocolSheet, "ServerView");
  ProtocolLayout->addWidget(ServerView, 1);
  HostKeyGroup = new QGroupBox(tr("Server host key fingerprints"), ProtocolSheet);
  HostKeyGroup->setObjectName(QStringLiteral("HostKeyGroup"));
  QGridLayout * HostKeyGrid = new QGridLayout(HostKeyGroup);
  HostKeyAlgorithmEdit = AddFingerprint(HostKeyGrid, HostKeyGroup, "HostKeyAlgorithmEdit", tr("Algorithm:"));
  HostKeyFingerprintSHA256Edit = AddFingerprint(HostKeyGrid, HostKeyGroup, "HostKeyFingerprintSHA256Edit", tr("SHA-256:"));
  HostKeyFingerprintMD5Edit = AddFingerprint(HostKeyGrid, HostKeyGroup, "HostKeyFingerprintMD5Edit", tr("MD5:"));
  ProtocolLayout->addWidget(HostKeyGroup);
  CertificateGroup = new QGroupBox(tr("Certificate fingerprint"), ProtocolSheet);
  CertificateGroup->setObjectName(QStringLiteral("CertificateGroup"));
  QGridLayout * CertificateGrid = new QGridLayout(CertificateGroup);
  CertificateFingerprintSha256Edit =
    AddFingerprint(CertificateGrid, CertificateGroup, "CertificateFingerprintSha256Edit", tr("SHA-256:"));
  CertificateFingerprintSha1Edit =
    AddFingerprint(CertificateGrid, CertificateGroup, "CertificateFingerprintSha1Edit", tr("SHA-1:"));
  QPushButton * CertificateViewButton = new QPushButton(tr("&Full certificate"), CertificateGroup);
  CertificateViewButton->setObjectName(QStringLiteral("CertificateViewButton"));
  CertificateViewButton->setAutoDefault(false);
  CertificateGrid->addWidget(CertificateViewButton, CertificateGrid->rowCount(), 1, Qt::AlignLeft);
  connect(CertificateViewButton, &QPushButton::clicked, this, [this]()
  {
    MessageDialog(FSessionInfo.Certificate, qtInformation, qaOK);
  });
  ProtocolLayout->addWidget(CertificateGroup);
  PageControl->addTab(ProtocolSheet, tr("Protocol"));

  QWidget * CapabilitiesSheet = new QWidget(PageControl);
  CapabilitiesSheet->setObjectName(QStringLiteral("CapabilitiesSheet"));
  QVBoxLayout * CapabilitiesLayout = new QVBoxLayout(CapabilitiesSheet);
  ProtocolView = CreateListView(CapabilitiesSheet, "ProtocolView");
  CapabilitiesLayout->addWidget(ProtocolView, 1);
  InfoGroup = new QGroupBox(tr("Additional information"), CapabilitiesSheet);
  InfoGroup->setObjectName(QStringLiteral("InfoGroup"));
  QVBoxLayout * InfoLayout = new QVBoxLayout(InfoGroup);
  InfoMemo = new QPlainTextEdit(InfoGroup);
  InfoMemo->setObjectName(QStringLiteral("InfoMemo"));
  InfoMemo->setReadOnly(true);
  InfoLayout->addWidget(InfoMemo);
  CapabilitiesLayout->addWidget(InfoGroup);
  PageControl->addTab(CapabilitiesSheet, tr("Capabilities"));

  SpaceAvailableSheet = new QWidget(PageControl);
  SpaceAvailableSheet->setObjectName(QStringLiteral("SpaceAvailableSheet"));
  QVBoxLayout * SpaceLayout = new QVBoxLayout(SpaceAvailableSheet);
  QHBoxLayout * PathRow = new QHBoxLayout();
  QLabel * Label1 = new QLabel(tr("&Path:"), SpaceAvailableSheet);
  PathRow->addWidget(Label1);
  SpaceAvailablePathEdit = new QLineEdit(SpaceAvailableSheet);
  SpaceAvailablePathEdit->setObjectName(QStringLiteral("SpaceAvailablePathEdit"));
  Label1->setBuddy(SpaceAvailablePathEdit);
  PathRow->addWidget(SpaceAvailablePathEdit, 1);
  SpaceAvailableButton = new QPushButton(tr("Check &space"), SpaceAvailableSheet);
  SpaceAvailableButton->setObjectName(QStringLiteral("SpaceAvailableButton"));
  SpaceAvailableButton->setAutoDefault(false);
  PathRow->addWidget(SpaceAvailableButton);
  SpaceLayout->addLayout(PathRow);
  SpaceAvailableView = CreateListView(SpaceAvailableSheet, "SpaceAvailableView");
  SpaceLayout->addWidget(SpaceAvailableView, 1);
  PageControl->addTab(SpaceAvailableSheet, tr("Space available"));
  connect(SpaceAvailableButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { CheckSpaceAvailable(); }); });
  connect(SpaceAvailablePathEdit, &QLineEdit::textChanged, this, [this]() { UpdateControls(); });
  connect(SpaceAvailablePathEdit, &QLineEdit::returnPressed, SpaceAvailableButton, &QPushButton::click);
  connect(PageControl, &QTabWidget::currentChanged, this, [this]()
  {
    if (PageControl->currentWidget() == SpaceAvailableSheet)
    {
      ExecuteHandler([this]() { NeedSpaceAvailable(); });
    }
  });

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  Buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("CloseButton"));
  QPushButton * ClipboardButton = Buttons->addButton(tr("&Copy to Clipboard"), QDialogButtonBox::ActionRole);
  ClipboardButton->setObjectName(QStringLiteral("ClipboardButton"));
  connect(ClipboardButton, &QPushButton::clicked, this, [this]()
  {
    ExecuteHandler([this]()
    {
      NeedSpaceAvailable();
      FLastFeededControl = nullptr;
      FClipboard = UnicodeString();
      Feed([this](QWidget * Control, int Label, UnicodeString Value) { ClipboardAddItem(Control, Label, Value); });
      CopyToClipboard(FClipboard);
    });
  });
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"FileSystemInfo.dfm"));

  resize(fontMetrics().averageCharWidth() * 80, fontMetrics().height() * 32);
}
//---------------------------------------------------------------------------
QTreeWidget * TFileSystemInfoDialog::CreateListView(QWidget * Parent, const char * Name)
{
  QTreeWidget * View = new QTreeWidget(Parent);
  View->setObjectName(QString::fromLatin1(Name));
  View->setRootIsDecorated(false);
  View->setUniformRowHeights(true);
  View->setHeaderLabels({ tr("Item"), tr("Value") });
  View->setSelectionMode(QAbstractItemView::ExtendedSelection);
  return View;
}
//---------------------------------------------------------------------------
QLineEdit * TFileSystemInfoDialog::AddFingerprint(QGridLayout * Grid, QWidget * Parent, const char * Name, const QString & Caption)
{
  QLabel * Label = new QLabel(Caption, Parent);
  QLineEdit * Edit = new QLineEdit(Parent);
  Edit->setObjectName(QString::fromLatin1(Name));
  Edit->setReadOnly(true);
  Label->setBuddy(Edit);
  int Row = Grid->rowCount();
  Grid->addWidget(Label, Row, 0);
  Grid->addWidget(Edit, Row, 1);
  return Edit;
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::Execute(const TSessionInfo & SessionInfo,
  const TFileSystemInfo & FileSystemInfo, const UnicodeString & SpaceAvailablePath)
{
  FSessionInfo = SessionInfo;
  FFileSystemInfo = FileSystemInfo;
  SpaceAvailablePathEdit->setText(ToQString(SpaceAvailablePath));
  UpdateControls();
  FeedControls();
  exec();
}
//---------------------------------------------------------------------------
UnicodeString TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability)
{
  return BooleanToStr(FFileSystemInfo.IsCapable[Capability]);
}
//---------------------------------------------------------------------------
UnicodeString TFileSystemInfoDialog::CapabilityStr(TFSCapability Capability1, TFSCapability Capability2)
{
  return FORMAT(L"%s/%s", (CapabilityStr(Capability1), CapabilityStr(Capability2)));
}
//---------------------------------------------------------------------------
UnicodeString TFileSystemInfoDialog::SpaceStr(__int64 Bytes)
{
  UnicodeString Result;
  if (Bytes == 0)
  {
    Result = LoadStr(FSINFO_BYTES_UNKNOWN);
  }
  else
  {
    Result = FormatBytes(Bytes);
    UnicodeString SizeUnorderedStr = FormatBytes(Bytes, fbNone);
    if (Result != SizeUnorderedStr)
    {
      Result = FORMAT(L"%s (%s)", (Result, SizeUnorderedStr));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::Feed(TAddItem AddItem)
{
  AddItem(ServerView, FSINFO_REMOTE_SYSTEM, FFileSystemInfo.RemoteSystem);
  AddItem(ServerView, FSINFO_FS_PROTOCOL, FFileSystemInfo.ProtocolName);
  AddItem(ServerView, FSINFO_CRYPTOGRAPHIC_PROTOCOL, FSessionInfo.SecurityProtocolName);
  AddItem(ServerView, FSINFO_SSH_IMPLEMENTATION, FSessionInfo.SshImplementation);

  UnicodeString Str = FSessionInfo.CSCipher;
  if (FSessionInfo.CSCipher != FSessionInfo.SCCipher)
  {
    Str += FORMAT(L"/%s", (FSessionInfo.SCCipher));
  }
  AddItem(ServerView, FSINFO_CIPHER, Str);

  Str = DefaultStr(FSessionInfo.CSCompression, LoadStr(NO_STR));
  if (FSessionInfo.CSCompression != FSessionInfo.SCCompression)
  {
    Str += FORMAT(L"/%s", (DefaultStr(FSessionInfo.SCCompression, LoadStr(NO_STR))));
  }
  AddItem(ServerView, FSINFO_COMPRESSION, Str);

  AddItem(HostKeyFingerprintSHA256Edit, 0, FSessionInfo.HostKeyFingerprintSHA256);
  AddItem(HostKeyFingerprintMD5Edit, 0, FSessionInfo.HostKeyFingerprintMD5);
  AddItem(CertificateFingerprintSha256Edit, 0, FSessionInfo.CertificateFingerprintSHA256);
  AddItem(CertificateFingerprintSha1Edit, 0, FSessionInfo.CertificateFingerprintSHA1);

  AddItem(ProtocolView, FSINFO_MODE_CHANGING, CapabilityStr(fcModeChanging));
  AddItem(ProtocolView, FSINFO_ACL_CHANGING, CapabilityStr(fcAclChangingFiles));
  AddItem(ProtocolView, FSINFO_OWNER_GROUP_CHANGING, CapabilityStr(fcGroupChanging));
  UnicodeString AnyCommand;
  if (!FFileSystemInfo.IsCapable[fcShellAnyCommand] && FFileSystemInfo.IsCapable[fcAnyCommand])
  {
    AnyCommand = LoadStr(FSINFO_PROTOCOL_ANY_COMMAND);
  }
  else
  {
    AnyCommand = CapabilityStr(fcAnyCommand);
  }
  AddItem(ProtocolView, FSINFO_ANY_COMMAND, AnyCommand);
  AddItem(ProtocolView, FSINFO_SYMBOLIC_HARD_LINK, CapabilityStr(fcSymbolicLink, fcHardLink));
  AddItem(ProtocolView, FSINFO_USER_GROUP_LISTING, CapabilityStr(fcUserGroupListing));
  AddItem(ProtocolView, FSINFO_REMOTE_COPY, CapabilityStr(fcRemoteCopy));
  AddItem(ProtocolView, FSINFO_CHECKING_SPACE_AVAILABLE, CapabilityStr(fcCheckingSpaceAvailable));
  AddItem(ProtocolView, FSINFO_CALCULATING_CHECKSUM, CapabilityStr(fcCalculatingChecksum));
  AddItem(ProtocolView, FSINFO_NATIVE_TEXT_MODE, CapabilityStr(fcNativeTextMode));

  AddItem(InfoMemo, 0, FFileSystemInfo.AdditionalInfo);

  AddItem(SpaceAvailableView, FSINFO_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.BytesOnDevice));
  AddItem(SpaceAvailableView, FSINFO_UNUSED_BYTES_ON_DEVICE, SpaceStr(FSpaceAvailable.UnusedBytesOnDevice));
  AddItem(SpaceAvailableView, FSINFO_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.BytesAvailableToUser));
  AddItem(SpaceAvailableView, FSINFO_UNUSED_BYTES_AVAILABLE_TO_USER, SpaceStr(FSpaceAvailable.UnusedBytesAvailableToUser));
  AddItem(SpaceAvailableView, FSINFO_BYTES_PER_ALLOCATION_UNIT, SpaceStr(FSpaceAvailable.BytesPerAllocationUnit));
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::ControlsAddItem(QWidget * Control, int Label, UnicodeString Value)
{
  if (FLastFeededControl != Control)
  {
    FLastFeededControl = Control;
    FLastListItem = 0;
  }

  if ((Control == HostKeyFingerprintSHA256Edit) || (Control == HostKeyFingerprintMD5Edit))
  {
    HostKeyGroup->setVisible(!Value.IsEmpty());
    UnicodeString Alg1 = CutToChar(Value, L' ', true);
    UnicodeString Alg2 = CutToChar(Value, L' ', true);
    HostKeyAlgorithmEdit->setText(ToQString(FORMAT(L"%s %s", (Alg1, Alg2))));
    static_cast<QLineEdit *>(Control)->setText(ToQString(Value));
  }
  else if ((Control == CertificateFingerprintSha256Edit) || (Control == CertificateFingerprintSha1Edit))
  {
    CertificateGroup->setVisible(!Value.IsEmpty());
    static_cast<QLineEdit *>(Control)->setText(ToQString(Value));
  }
  else if (Control == InfoMemo)
  {
    InfoGroup->setVisible(!Value.IsEmpty());
    InfoMemo->setPlainText(ToQString(Value));
  }
  else
  {
    QTreeWidget * ListView = static_cast<QTreeWidget *>(Control);
    if (!Value.IsEmpty())
    {
      QTreeWidgetItem * Item =
        (ListView->topLevelItemCount() > FLastListItem) ? ListView->topLevelItem(FLastListItem) : new QTreeWidgetItem(ListView);
      FLastListItem++;
      Item->setText(0, ToQString(LoadStr(Label)));
      Item->setText(1, ToQString(Value));
      // The space not known is grayed (upstream's SpaceAvailableViewCustomDrawItem)
      QColor Color =
        (Value == LoadStr(FSINFO_BYTES_UNKNOWN)) ? palette().color(QPalette::Disabled, QPalette::Text) : QColor();
      Item->setForeground(1, Color.isValid() ? QBrush(Color) : QBrush());
    }
  }
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::ClipboardAddItem(QWidget * Control, int Label, UnicodeString Value)
{
  // The controls of the disabled page of the space available are skipped
  if (Control->isEnabled() && !Value.IsEmpty())
  {
    QGroupBox * Group = qobject_cast<QGroupBox *>(Control->parentWidget());
    QWidget * ControlGroup = (Group != nullptr) ? static_cast<QWidget *>(Group) : Control;
    if (FLastFeededControl != ControlGroup)
    {
      if (FLastFeededControl != nullptr)
      {
        FClipboard += UnicodeString::StringOfChar(L'-', 60) + L"\n";
      }

      if (Group != nullptr)
      {
        FClipboard += FORMAT(L"%s\n", (ToUnicodeString(Group->title())));
      }

      FLastFeededControl = ControlGroup;
    }

    if (qobject_cast<QTreeWidget *>(Control) == nullptr)
    {
      // With the label of the edit
      for (QLabel * ALabel : Control->parentWidget()->findChildren<QLabel *>(QString(), Qt::FindDirectChildrenOnly))
      {
        if (ALabel->buddy() == Control)
        {
          UnicodeString S = RemoveSuffix(StripHotkey(ToUnicodeString(ALabel->text())), L":");
          FClipboard += FORMAT(L"%s = ", (S));
        }
      }
      Value = RemoveSuffix(Value, L"\n");
      FClipboard += FORMAT(L"%s\n", (Value));
    }
    else
    {
      FClipboard += FORMAT(L"%s = %s\n", (LoadStr(Label), Value));
    }
  }
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::FeedControls()
{
  FLastFeededControl = nullptr;
  Feed([this](QWidget * Control, int Label, UnicodeString Value) { ControlsAddItem(Control, Label, Value); });
  for (QTreeWidget * View : { ServerView, ProtocolView, SpaceAvailableView })
  {
    View->resizeColumnToContents(0);
  }
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::UpdateControls()
{
  PageControl->setTabEnabled(PageControl->indexOf(SpaceAvailableSheet), SpaceAvailableSupported());
  SpaceAvailableButton->setEnabled(SpaceAvailableSupported() && !SpaceAvailablePathEdit->text().isEmpty());
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::CheckSpaceAvailable()
{
  DebugAssert(FOnGetSpaceAvailable != nullptr);
  DebugAssert(!SpaceAvailablePathEdit->text().isEmpty());

  FSpaceAvailableLoaded = true;

  bool DoClose = false;
  ::rtl::TFinally Finally([&]()
  {
    FeedControls();
    if (DoClose)
    {
      close();
    }
  });
  FOnGetSpaceAvailable(ToUnicodeString(SpaceAvailablePathEdit->text()), FSpaceAvailable, DoClose);
}
//---------------------------------------------------------------------------
void TFileSystemInfoDialog::NeedSpaceAvailable()
{
  if (!FSpaceAvailableLoaded && SpaceAvailableSupported())
  {
    CheckSpaceAvailable();
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void DoFileSystemInfoDialog(const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath, TGetSpaceAvailableEvent OnGetSpaceAvailable)
{
  TFileSystemInfoDialog Dialog(nullptr, OnGetSpaceAvailable);
  Dialog.Execute(SessionInfo, FileSystemInfo, SpaceAvailablePath);
}
//---------------------------------------------------------------------------
