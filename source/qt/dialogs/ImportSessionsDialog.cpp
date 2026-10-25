// The import of sites (upstream's forms/ImportSessions.dfm): the sessions
// of PuTTY, FileZilla, OpenSSH, an INI file of upstream's format and the
// hosts of OpenSSH's known_hosts.
//
// Not ported: KiTTY (of Windows only).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <CoreMain.h>
#include <HierarchicalStorage.h>
#include <PuttyTools.h>
#include <QtInterface.h>
#include <SessionData.h>

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QMimeData>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
const int PuttyIndex = 0;
const int FilezillaIndex = 1;
const int OpensshIndex = 2;
const int IniIndex = 3;
const int KnownHostsIndex = 4;
//---------------------------------------------------------------------------
class TImportSessionsDialog : public QDialog
{
public:
  explicit TImportSessionsDialog(QWidget * Parent);

  void Init(std::vector<TStoredSessionList *> * SessionLists, std::vector<UnicodeString> * Errors);
  bool Execute();
  TStoredSessionList * SelectSessionsForImport(UnicodeString & Error);
  UnicodeString GetIniFileName() const { return FIniFileName; }

private:
  std::vector<TStoredSessionList *> * FSessionLists;
  std::vector<UnicodeString> * FErrors;
  UnicodeString FIniFileName;
  std::unique_ptr<TStoredSessionList> FPastedKnownHosts;
  std::unique_ptr<TStoredSessionList> FIniImportSessionList;
  int FLoading;

  QComboBox * SourceComboBox;
  QStackedWidget * ListStack;
  QTreeWidget * SessionListView2;
  QLabel * ErrorLabel;
  QPushButton * CheckAllButton;
  QPushButton * PasteButton;
  QPushButton * BrowseButton;
  QPushButton * OKButton;

  TStoredSessionList * GetSessionList(int Index) { return (*FSessionLists)[Index]; }
  static TSessionData * GetSessionData(QTreeWidgetItem * Item);
  void UpdateControls();
  void ClearSelections();
  void SaveSelection();
  void LoadSessions();
  bool ConvertKeyFile(UnicodeString & KeyFile, TStrings * ConvertedKeyFiles, TStrings * NotConvertedKeyFiles);
  void PasteButtonClick();
  void BrowseButtonClick();
};
//---------------------------------------------------------------------------
TImportSessionsDialog::TImportSessionsDialog(QWidget * Parent) :
  QDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("ImportSessionsDialog"));
  setWindowTitle(ToQString(LoadStr(IMPORT_CAPTION)));
  FSessionLists = nullptr;
  FErrors = nullptr;
  FLoading = 0;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QHBoxLayout * SourceRow = new QHBoxLayout();
  QLabel * Label = new QLabel(tr("&Import from:"), this);
  SourceRow->addWidget(Label);
  SourceComboBox = new QComboBox(this);
  SourceComboBox->setObjectName(QStringLiteral("SourceComboBox"));
  SourceComboBox->addItems({ QStringLiteral("PuTTY"), QStringLiteral("FileZilla"), QStringLiteral("OpenSSH"), tr("INI file"),
    QStringLiteral("known_hosts") });
  Label->setBuddy(SourceComboBox);
  SourceRow->addWidget(SourceComboBox, 1);
  BrowseButton = new QPushButton(tr("B&rowse..."), this);
  BrowseButton->setObjectName(QStringLiteral("BrowseButton"));
  SourceRow->addWidget(BrowseButton);
  PasteButton = new QPushButton(tr("&Paste"), this);
  PasteButton->setObjectName(QStringLiteral("PasteButton"));
  SourceRow->addWidget(PasteButton);
  MainLayout->addLayout(SourceRow);

  ListStack = new QStackedWidget(this);
  SessionListView2 = new QTreeWidget(ListStack);
  SessionListView2->setObjectName(QStringLiteral("SessionListView2"));
  SessionListView2->setHeaderHidden(true);
  SessionListView2->setRootIsDecorated(false);
  SessionListView2->setUniformRowHeights(true);
  SessionListView2->setMinimumHeight(fontMetrics().height() * 15);
  ListStack->addWidget(SessionListView2);
  ErrorLabel = new QLabel(ListStack);
  ErrorLabel->setObjectName(QStringLiteral("ErrorLabel"));
  ErrorLabel->setWordWrap(true);
  ErrorLabel->setAlignment(Qt::AlignCenter);
  ErrorLabel->setFrameShape(QFrame::StyledPanel);
  ListStack->addWidget(ErrorLabel);
  MainLayout->addWidget(ListStack, 1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OKButton = Buttons->button(QDialogButtonBox::Ok);
  OKButton->setObjectName(QStringLiteral("OKButton"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelButton"));
  CheckAllButton = Buttons->addButton(tr("Un/check &all"), QDialogButtonBox::ActionRole);
  CheckAllButton->setObjectName(QStringLiteral("CheckAllButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);

  connect(SourceComboBox, &QComboBox::currentIndexChanged, this, [this]()
  {
    if (FLoading == 0)
    {
      SaveSelection();
      LoadSessions();
    }
  });
  connect(SessionListView2, &QTreeWidget::itemChanged, this, [this]() { UpdateControls(); });
  connect(CheckAllButton, &QPushButton::clicked, this, [this]()
  {
    // upstream's ListViewCheckAll(caToggle): all checked are unchecked
    bool AllChecked = true;
    for (int Index = 0; Index < SessionListView2->topLevelItemCount(); Index++)
    {
      AllChecked = AllChecked && (SessionListView2->topLevelItem(Index)->checkState(0) == Qt::Checked);
    }
    for (int Index = 0; Index < SessionListView2->topLevelItemCount(); Index++)
    {
      SessionListView2->topLevelItem(Index)->setCheckState(0, AllChecked ? Qt::Unchecked : Qt::Checked);
    }
    UpdateControls();
  });
  connect(PasteButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { PasteButtonClick(); }); });
  connect(BrowseButton, &QPushButton::clicked, this, [this]() { ExecuteHandler([this]() { BrowseButtonClick(); }); });
  connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() { UpdateControls(); });

  resize(fontMetrics().averageCharWidth() * 70, fontMetrics().height() * 28);
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::Init(std::vector<TStoredSessionList *> * SessionLists, std::vector<UnicodeString> * Errors)
{
  FSessionLists = SessionLists;
  FErrors = Errors;

  // The first source with sessions
  int Index = 0;
  while ((Index < static_cast<int>(SessionLists->size())) && (GetSessionList(Index)->Count == 0))
  {
    Index++;
  }
  {
    TAutoNestingCounter LoadingCounter(FLoading);
    SourceComboBox->setCurrentIndex((Index < static_cast<int>(SessionLists->size())) ? Index : 0);
  }
  LoadSessions();
}
//---------------------------------------------------------------------------
TSessionData * TImportSessionsDialog::GetSessionData(QTreeWidgetItem * Item)
{
  return DebugNotNull(static_cast<TSessionData *>(Item->data(0, Qt::UserRole).value<void *>()));
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::UpdateControls()
{
  int Source = SourceComboBox->currentIndex();
  BrowseButton->setVisible(Source == IniIndex);
  PasteButton->setVisible(Source == KnownHostsIndex);
  const QMimeData * MimeData = QApplication::clipboard()->mimeData();
  PasteButton->setEnabled((MimeData != nullptr) && MimeData->hasText());
  bool AnyChecked = false;
  for (int Index = 0; Index < SessionListView2->topLevelItemCount(); Index++)
  {
    AnyChecked = AnyChecked || (SessionListView2->topLevelItem(Index)->checkState(0) == Qt::Checked);
  }
  OKButton->setEnabled(AnyChecked);
  CheckAllButton->setEnabled(SessionListView2->topLevelItemCount() > 0);
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::ClearSelections()
{
  for (TStoredSessionList * SessionList : *FSessionLists)
  {
    for (int Index = 0; Index < SessionList->Count; Index++)
    {
      SessionList->Sessions[Index]->Selected = false;
    }
  }
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::SaveSelection()
{
  for (int Index = 0; Index < SessionListView2->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = SessionListView2->topLevelItem(Index);
    GetSessionData(Item)->Selected = (Item->checkState(0) == Qt::Checked);
  }
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::LoadSessions()
{
  int Source = SourceComboBox->currentIndex();
  TStoredSessionList * SessionList = GetSessionList(Source);

  {
    QSignalBlocker Blocker(SessionListView2);
    SessionListView2->clear();
    for (int Index = 0; Index < SessionList->Count; Index++)
    {
      TSessionData * Session = SessionList->Sessions[Index];
      QTreeWidgetItem * Item = new QTreeWidgetItem(SessionListView2);
      Item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void *>(Session)));
      Item->setText(0, ToQString(Session->Name));
      Item->setFlags(Item->flags() | Qt::ItemIsUserCheckable);
      Item->setCheckState(0, Session->Selected ? Qt::Checked : Qt::Unchecked);
      UnicodeString InfoTip;
      if (Source == KnownHostsIndex)
      {
        UnicodeString Algs;
        UnicodeString HostKeys = Session->HostKey;
        while (!HostKeys.IsEmpty())
        {
          UnicodeString HostKey = CutToChar(HostKeys, L';', true);
          UnicodeString Alg = CutToChar(HostKey, L':', true);
          AddToList(Algs, Alg, L", ");
        }
        InfoTip = FMTLOAD(IMPORT_KNOWNHOSTS_INFO_TIP, (Session->HostName, Algs));
      }
      else
      {
        InfoTip = Session->InfoTip;
      }
      Item->setToolTip(0, ToQString(InfoTip));
    }
  }

  UnicodeString Error = (*FErrors)[Source];
  if ((SessionList->Count > 0) || Error.IsEmpty())
  {
    ListStack->setCurrentWidget(SessionListView2);
  }
  else
  {
    ErrorLabel->setText(ToQString(Error));
    ListStack->setCurrentWidget(ErrorLabel);
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
bool TImportSessionsDialog::ConvertKeyFile(
  UnicodeString & KeyFile, TStrings * ConvertedKeyFiles, TStrings * NotConvertedKeyFiles)
{
  bool ConvertedSession = false;
  if (!KeyFile.IsEmpty() && FileExists(ApiPath(KeyFile)))
  {
    UnicodeString CanonicalPath = GetCanonicalPath(KeyFile);
    // Reuses the keys converted already, saved under a custom name
    int CanonicalIndex = ConvertedKeyFiles->IndexOfName(CanonicalPath);
    if (CanonicalIndex >= 0)
    {
      KeyFile = ConvertedKeyFiles->ValueFromIndex[CanonicalIndex];
      ConvertedSession = true;
    }
    // The conversion the user refused is not asked again
    else if (NotConvertedKeyFiles->IndexOf(CanonicalPath) >= 0)
    {
      // noop
    }
    else
    {
      UnicodeString ConvertedFilename = ChangeFileExt(KeyFile, FORMAT(L".%s", (PuttyKeyExt)));
      UnicodeString FileName;
      if (FileExists(ApiPath(ConvertedFilename)))
      {
        FileName = ConvertedFilename;
        ConvertedSession = true;
      }
      else
      {
        FileName = KeyFile;
        TDateTime TimestampBefore, TimestampAfter;
        FileAge(FileName, TimestampBefore);
        try
        {
          VerifyAndConvertKey(FileName, true);
          FileAge(FileName, TimestampAfter);
          if ((KeyFile != FileName) || (TimestampBefore != TimestampAfter))
          {
            ConvertedSession = true;
          }
        }
        catch (EAbort &)
        {
          NotConvertedKeyFiles->Add(CanonicalPath);
        }
      }

      if (ConvertedSession)
      {
        KeyFile = FileName;
        ConvertedKeyFiles->Values[CanonicalPath] = FileName;
      }
    }
  }
  return ConvertedSession;
}
//---------------------------------------------------------------------------
bool TImportSessionsDialog::Execute()
{
  bool Result = (exec() == QDialog::Accepted);

  if (Result)
  {
    ClearSelections();
    SaveSelection();

    // The keys of OpenSSH are converted to PuTTY's
    if (SourceComboBox->currentIndex() == OpensshIndex)
    {
      std::unique_ptr<TStrings> ConvertedSessions(new TStringList());
      std::unique_ptr<TStrings> ConvertedKeyFiles(new TStringList());
      std::unique_ptr<TStrings> NotConvertedKeyFiles(CreateSortedStringList());
      for (int Index = 0; Index < SessionListView2->topLevelItemCount(); Index++)
      {
        QTreeWidgetItem * Item = SessionListView2->topLevelItem(Index);
        if (Item->checkState(0) == Qt::Checked)
        {
          TSessionData * Data = GetSessionData(Item);
          UnicodeString SessionKeys;

          UnicodeString PublicKeyFile = Data->PublicKeyFile;
          if (ConvertKeyFile(PublicKeyFile, ConvertedKeyFiles.get(), NotConvertedKeyFiles.get()))
          {
            Data->PublicKeyFile = PublicKeyFile;
            SessionKeys = PublicKeyFile;
          }

          UnicodeString TunnelPublicKeyFile = Data->TunnelPublicKeyFile;
          if (ConvertKeyFile(TunnelPublicKeyFile, ConvertedKeyFiles.get(), NotConvertedKeyFiles.get()))
          {
            Data->TunnelPublicKeyFile = TunnelPublicKeyFile;
            if (SessionKeys != TunnelPublicKeyFile)
            {
              AddToList(SessionKeys, TunnelPublicKeyFile, L", ");
            }
          }

          if (!SessionKeys.IsEmpty())
          {
            ConvertedSessions->Add(FORMAT(L"%s (%s)", (Data->Name, SessionKeys)));
          }
        }
      }

      if (ConvertedSessions->Count > 0)
      {
        UnicodeString Message =
          MainInstructions(FMTLOAD(IMPORT_CONVERTED_KEYS, (ConvertedKeyFiles->Count, ConvertedSessions->Count)));
        MoreMessageDialog(Message, ConvertedSessions.get(), qtInformation, qaOK);
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::PasteButtonClick()
{
  // Proceeds even when the clipboard has no text, "no host keys" shows
  UnicodeString Text;
  TextFromClipboard(Text, false);
  std::unique_ptr<TStrings> Lines(new TStringList());
  Lines->Text = Text;
  int Index = SourceComboBox->currentIndex();
  UnicodeString Error;
  FPastedKnownHosts.reset(Configuration->SelectKnownHostsSessionsForImport(Lines.get(), StoredSessions, Error));
  (*FSessionLists)[Index] = FPastedKnownHosts.get();
  (*FErrors)[Index] = Error;
  LoadSessions();
}
//---------------------------------------------------------------------------
TStoredSessionList * TImportSessionsDialog::SelectSessionsForImport(UnicodeString & Error)
{
  return Configuration->SelectSessionsForImport(StoredSessions, FIniFileName, Error);
}
//---------------------------------------------------------------------------
void TImportSessionsDialog::BrowseButtonClick()
{
  QString FileName = QFileDialog::getOpenFileName(this, ToQString(LoadStr(IMPORT_INI_TITLE)),
    ToQString(DefaultStr(FIniFileName, Configuration->GetDefaultIniFileExportPath())),
    FileDialogFilter(LoadStr(EXPORT_CONF_FILTER)));
  if (!FileName.isEmpty())
  {
    int Index = SourceComboBox->currentIndex();
    FIniFileName = ToUnicodeString(FileName);
    UnicodeString Error;
    FIniImportSessionList.reset(SelectSessionsForImport(Error));
    (*FSessionLists)[Index] = FIniImportSessionList.get();
    (*FErrors)[Index] = Error;
    LoadSessions();
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoImportSessionsDialog(TList * Imported)
{
  TImportSessionsDialog Dialog(nullptr);

  std::vector<TStoredSessionList *> SessionLists;
  std::vector<UnicodeString> Errors;
  UnicodeString Error;

  std::unique_ptr<TStoredSessionList> PuttyImportSessionList(
    AppConfiguration->SelectPuttySessionsForImport(StoredSessions, Error));
  SessionLists.push_back(PuttyImportSessionList.get());
  Errors.push_back(Error);

  Error = UnicodeString();
  std::unique_ptr<TStoredSessionList> FilezillaImportSessionList(
    Configuration->SelectFilezillaSessionsForImport(StoredSessions, Error));
  SessionLists.push_back(FilezillaImportSessionList.get());
  Errors.push_back(Error);

  Error = UnicodeString();
  std::unique_ptr<TStoredSessionList> OpensshImportSessionList(
    Configuration->SelectOpensshSessionsForImport(StoredSessions, Error));
  SessionLists.push_back(OpensshImportSessionList.get());
  Errors.push_back(Error);

  Error = UnicodeString();
  std::unique_ptr<TStoredSessionList> IniImportSessionList(Dialog.SelectSessionsForImport(Error));
  DebugAssert(IniIndex == static_cast<int>(SessionLists.size()));
  SessionLists.push_back(IniImportSessionList.get());
  Errors.push_back(Error);

  Error = UnicodeString();
  std::unique_ptr<TStoredSessionList> KnownHostsImportSessionList(
    Configuration->SelectKnownHostsSessionsForImport(StoredSessions, Error));
  DebugAssert(KnownHostsIndex == static_cast<int>(SessionLists.size()));
  SessionLists.push_back(KnownHostsImportSessionList.get());
  Errors.push_back(Error);

  Dialog.Init(&SessionLists, &Errors);

  bool Result = Dialog.Execute();

  if (Result)
  {
    StoredSessions->Import(SessionLists[PuttyIndex], true, Imported);
    AppConfiguration->ImportPuttyHostKeys(SessionLists[PuttyIndex]);

    StoredSessions->Import(SessionLists[FilezillaIndex], true, Imported);

    StoredSessions->Import(SessionLists[OpensshIndex], true, Imported);

    TStoredSessionList * AIniImportSessionList = SessionLists[IniIndex];
    if (StoredSessions->Import(AIniImportSessionList, true, Imported))
    {
      std::unique_ptr<THierarchicalStorage> IniStorage(TIniFileStorage::CreateFromPath(Dialog.GetIniFileName()));
      TStoredSessionList::ImportHostKeys(IniStorage.get(), AIniImportSessionList, true);
    }

    // The actual import is done by ImportSelectedKnownHosts
    TStoredSessionList * AKnownHostsImportSessionList = SessionLists[KnownHostsIndex];
    TStoredSessionList::SelectKnownHostsForSelectedSessions(AKnownHostsImportSessionList, SessionLists[OpensshIndex]);
    TStoredSessionList::ImportSelectedKnownHosts(AKnownHostsImportSessionList);
  }
  return Result;
}
//---------------------------------------------------------------------------
