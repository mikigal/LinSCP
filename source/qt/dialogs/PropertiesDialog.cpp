// The properties of files (upstream's forms/Properties.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <GUIConfiguration.h>

#include "DirModel.h"
#include "PropertiesDialog.h"
#include "RightsFrame.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFocusEvent>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QStyle>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
bool DoPropertiesDialog(TStrings * FileList,
  const UnicodeString & Directory, const TRemoteTokenList * GroupList,
  const TRemoteTokenList * UserList, TStrings * ChecksumAlgs,
  TRemoteProperties * Properties,
  int AllowedChanges, int Options, TCalculateSizeEvent OnCalculateSize,
  TCalculateChecksumEvent OnCalculateChecksum)
{
  TPropertiesDialog Dialog(QApplication::activeWindow(),
    FileList, Directory, GroupList, UserList, ChecksumAlgs, AllowedChanges, Options,
    OnCalculateSize, OnCalculateChecksum);
  return Dialog.Execute(*Properties);
}
//---------------------------------------------------------------------------
// An edit box that looks like a label, its text can be selected and copied
// (upstream's ReadOnlyControl of an edit)
static void ReadOnlyControl(QLineEdit * Edit)
{
  Edit->setReadOnly(true);
  Edit->setFrame(false);
  QPalette Palette = Edit->palette();
  Palette.setColor(QPalette::Base, Qt::transparent);
  Edit->setPalette(Palette);
}
//---------------------------------------------------------------------------
static QFrame * CreateBevel(QWidget * Parent, const char * Name)
{
  QFrame * Bevel = new QFrame(Parent);
  Bevel->setObjectName(QString::fromLatin1(Name));
  Bevel->setFrameShape(QFrame::HLine);
  Bevel->setFrameShadow(QFrame::Sunken);
  return Bevel;
}
//---------------------------------------------------------------------------
TPropertiesDialog::TPropertiesDialog(QWidget * Parent,
  TStrings * FileList, const UnicodeString & Directory,
  const TRemoteTokenList * GroupList, const TRemoteTokenList * UserList,
  TStrings * ChecksumAlgs,
  int AllowedChanges, int Options, TCalculateSizeEvent OnCalculateSize,
  TCalculateChecksumEvent OnCalculateChecksum) :
  QDialog(Parent)
{
  FOnCalculateSize = OnCalculateSize;
  FOnCalculateChecksum = OnCalculateChecksum;
  FShown = false;

  CreateControls();
  RightsFrame->OnChange = [this]() { ControlChange(); };

  FFileList = new TStringList();
  FFileList->Assign(FileList);
  FAllowedChanges = AllowedChanges;
  if (FLAGSET(FAllowedChanges, cpAcl))
  {
    DebugAssert(FLAGCLEAR(FAllowedChanges, cpMode));
    RightsLabel->setText(ToQString(LoadStr(PROPERTIES_ACL)));
    RightsFrame->DisplayAsAcl();
  }
  FOptions = Options;

  FAllowCalculateStats = false;
  FStatsNotCalculated = false;
  FChecksumLoaded = false;
  FMultipleChecksum = false;

  LocationLabel->setText(ToQString(Directory));

  FGroupList = GroupList;
  FUserList = UserList;
  FChecksumAlgs = ChecksumAlgs;

  ChecksumUnknownLabel->setText(ToQString(LoadStr(PROPERTIES_CHECKSUM_UNKNOWN)));
  LoadInfo();
}
//---------------------------------------------------------------------------
TPropertiesDialog::~TPropertiesDialog()
{
  delete FFileList;
  FFileList = NULL;
}
//---------------------------------------------------------------------------
void TPropertiesDialog::CreateControls()
{
  setObjectName(QStringLiteral("PropertiesDialog"));
  setWindowTitle(tr("Properties"));

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  PageControl = new QTabWidget(this);
  PageControl->setObjectName(QStringLiteral("PageControl"));
  MainLayout->addWidget(PageControl, 1);
  connect(PageControl, &QTabWidget::currentChanged, this, [this]() { PageControlChange(); });

  // Common
  CommonSheet = new QWidget(PageControl);
  CommonSheet->setObjectName(QStringLiteral("CommonSheet"));
  PageControl->addTab(CommonSheet, tr("Common"));
  QGridLayout * Grid = new QGridLayout(CommonSheet);
  int Row = 0;

  FileIconImage = new QLabel(CommonSheet);
  FileIconImage->setObjectName(QStringLiteral("FileIconImage"));
  FileIconImage->setFixedSize(32, 32);
  Grid->addWidget(FileIconImage, Row, 0);
  FileLabel = new QLineEdit(CommonSheet);
  FileLabel->setObjectName(QStringLiteral("FileLabel"));
  ReadOnlyControl(FileLabel);
  Grid->addWidget(FileLabel, Row, 1, 1, 2);
  Row++;
  Grid->addWidget(CreateBevel(CommonSheet, "Bevel1"), Row++, 0, 1, 3);

  QLabel * Label1 = new QLabel(tr("Location:"), CommonSheet);
  Label1->setObjectName(QStringLiteral("Label1"));
  Grid->addWidget(Label1, Row, 0);
  LocationLabel = new QLineEdit(CommonSheet);
  LocationLabel->setObjectName(QStringLiteral("LocationLabel"));
  ReadOnlyControl(LocationLabel);
  Grid->addWidget(LocationLabel, Row, 1, 1, 2);
  Row++;

  QLabel * Label2 = new QLabel(tr("Size:"), CommonSheet);
  Label2->setObjectName(QStringLiteral("Label2"));
  Grid->addWidget(Label2, Row, 0);
  SizeLabel = new QLineEdit(CommonSheet);
  SizeLabel->setObjectName(QStringLiteral("SizeLabel"));
  ReadOnlyControl(SizeLabel);
  Grid->addWidget(SizeLabel, Row, 1);
  CalculateSizeButton = new QPushButton(tr("C&alculate"), CommonSheet);
  CalculateSizeButton->setObjectName(QStringLiteral("CalculateSizeButton"));
  CalculateSizeButton->setAutoDefault(false);
  connect(CalculateSizeButton, &QPushButton::clicked, this, [this]() { CalculateSizeButtonClick(); });
  Grid->addWidget(CalculateSizeButton, Row, 2);
  Row++;

  LinksToLabelLabel = new QLabel(tr("Links to:"), CommonSheet);
  LinksToLabelLabel->setObjectName(QStringLiteral("LinksToLabelLabel"));
  Grid->addWidget(LinksToLabelLabel, Row, 0);
  LinksToLabel = new QLineEdit(CommonSheet);
  LinksToLabel->setObjectName(QStringLiteral("LinksToLabel"));
  ReadOnlyControl(LinksToLabel);
  Grid->addWidget(LinksToLabel, Row, 1, 1, 2);
  Row++;
  Grid->addWidget(CreateBevel(CommonSheet, "Bevel2"), Row++, 0, 1, 3);

  // The owner and the group: a combo box when they can be changed, a view
  // otherwise
  OwnerLabel = new QLabel(tr("Owner:"), CommonSheet);
  OwnerLabel->setObjectName(QStringLiteral("OwnerLabel"));
  Grid->addWidget(OwnerLabel, Row, 0);
  OwnerComboBox = new QComboBox(CommonSheet);
  OwnerComboBox->setObjectName(QStringLiteral("OwnerComboBox"));
  OwnerComboBox->setEditable(true);
  OwnerComboBox->setInsertPolicy(QComboBox::NoInsert);
  OwnerComboBox->installEventFilter(this);
  OwnerView = new QLineEdit(CommonSheet);
  OwnerView->setObjectName(QStringLiteral("OwnerView"));
  ReadOnlyControl(OwnerView);
  QHBoxLayout * OwnerLayout = new QHBoxLayout();
  OwnerLayout->addWidget(OwnerComboBox);
  OwnerLayout->addWidget(OwnerView);
  Grid->addLayout(OwnerLayout, Row, 1, 1, 2, Qt::AlignLeft);
  Row++;

  GroupLabel = new QLabel(tr("Group:"), CommonSheet);
  GroupLabel->setObjectName(QStringLiteral("GroupLabel"));
  Grid->addWidget(GroupLabel, Row, 0);
  GroupComboBox = new QComboBox(CommonSheet);
  GroupComboBox->setObjectName(QStringLiteral("GroupComboBox"));
  GroupComboBox->setEditable(true);
  GroupComboBox->setInsertPolicy(QComboBox::NoInsert);
  GroupComboBox->installEventFilter(this);
  GroupView = new QLineEdit(CommonSheet);
  GroupView->setObjectName(QStringLiteral("GroupView"));
  ReadOnlyControl(GroupView);
  QHBoxLayout * GroupLayout = new QHBoxLayout();
  GroupLayout->addWidget(GroupComboBox);
  GroupLayout->addWidget(GroupView);
  Grid->addLayout(GroupLayout, Row, 1, 1, 2, Qt::AlignLeft);
  Row++;
  for (QComboBox * Combo : { OwnerComboBox, GroupComboBox })
  {
    Combo->setMinimumWidth(fontMetrics().averageCharWidth() * 24);
    connect(Combo, &QComboBox::currentTextChanged, this, [this]() { ControlChange(); });
  }
  GroupOwnerRightsBevel = CreateBevel(CommonSheet, "GroupOwnerRightsBevel");
  Grid->addWidget(GroupOwnerRightsBevel, Row++, 0, 1, 3);

  RightsLabel = new QLabel(tr("Permissions:"), CommonSheet);
  RightsLabel->setObjectName(QStringLiteral("RightsLabel"));
  Grid->addWidget(RightsLabel, Row, 0, Qt::AlignTop);
  RightsFrame = new TRightsFrame(CommonSheet);
  Grid->addWidget(RightsFrame, Row, 1, 1, 2);
  Row++;
  RecursiveBevel = CreateBevel(CommonSheet, "RecursiveBevel");
  Grid->addWidget(RecursiveBevel, Row++, 0, 1, 3);

  RecursiveCheck2 = new QCheckBox(tr("Set owner, group and permissions &recursively"), CommonSheet);
  RecursiveCheck2->setObjectName(QStringLiteral("RecursiveCheck2"));
  connect(RecursiveCheck2, &QCheckBox::toggled, this, [this]() { ControlChange(); });
  Grid->addWidget(RecursiveCheck2, Row++, 0, 1, 3);
  Grid->setRowStretch(Row, 1);
  Grid->setColumnStretch(1, 1);

  // Checksum
  ChecksumSheet = new QWidget(PageControl);
  ChecksumSheet->setObjectName(QStringLiteral("ChecksumSheet"));
  PageControl->addTab(ChecksumSheet, tr("Checksum"));
  QGridLayout * ChecksumLayout = new QGridLayout(ChecksumSheet);
  QLabel * Label6 = new QLabel(tr("&Algorithm:"), ChecksumSheet);
  Label6->setObjectName(QStringLiteral("Label6"));
  ChecksumLayout->addWidget(Label6, 0, 0);
  ChecksumAlgEdit = new QComboBox(ChecksumSheet);
  ChecksumAlgEdit->setObjectName(QStringLiteral("ChecksumAlgEdit"));
  ChecksumAlgEdit->setEditable(true);
  ChecksumAlgEdit->setInsertPolicy(QComboBox::NoInsert);
  Label6->setBuddy(ChecksumAlgEdit);
  connect(ChecksumAlgEdit, &QComboBox::currentTextChanged, this, [this]() { ChecksumAlgEditChange(); });
  ChecksumLayout->addWidget(ChecksumAlgEdit, 0, 1);
  ChecksumButton = new QPushButton(tr("&Calculate checksum"), ChecksumSheet);
  ChecksumButton->setObjectName(QStringLiteral("ChecksumButton"));
  ChecksumButton->setAutoDefault(false);
  connect(ChecksumButton, &QPushButton::clicked, this, [this]() { CalculateChecksum(); });
  ChecksumLayout->addWidget(ChecksumButton, 0, 2);
  ChecksumGroup = new QGroupBox(tr("Checksum"), ChecksumSheet);
  ChecksumGroup->setObjectName(QStringLiteral("ChecksumGroup"));
  QVBoxLayout * ChecksumGroupLayout = new QVBoxLayout(ChecksumGroup);
  ChecksumUnknownLabel = new QLabel(ChecksumGroup);
  ChecksumUnknownLabel->setObjectName(QStringLiteral("ChecksumUnknownLabel"));
  ChecksumGroupLayout->addWidget(ChecksumUnknownLabel);
  ChecksumEdit = new QLineEdit(ChecksumGroup);
  ChecksumEdit->setObjectName(QStringLiteral("ChecksumEdit"));
  ReadOnlyControl(ChecksumEdit);
  ChecksumGroupLayout->addWidget(ChecksumEdit);
  ChecksumLayout->addWidget(ChecksumGroup, 1, 0, 1, 3);
  ChecksumView = new QTreeWidget(ChecksumSheet);
  ChecksumView->setObjectName(QStringLiteral("ChecksumView"));
  ChecksumView->setHeaderLabels({ tr("File"), tr("Checksum") });
  ChecksumView->setRootIsDecorated(false);
  ChecksumView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  ChecksumView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(ChecksumView, &QWidget::customContextMenuRequested, this, [this]() { CopyItems(ChecksumView); });
  ChecksumLayout->addWidget(ChecksumView, 2, 0, 1, 3);
  ChecksumLayout->setRowStretch(2, 1);
  ChecksumLayout->setColumnStretch(1, 1);

  // Tags
  TagsSheet = new QWidget(PageControl);
  TagsSheet->setObjectName(QStringLiteral("TagsSheet"));
  PageControl->addTab(TagsSheet, tr("Tags"));
  QGridLayout * TagsLayout = new QGridLayout(TagsSheet);
  TagsView = new QTreeWidget(TagsSheet);
  TagsView->setObjectName(QStringLiteral("TagsView"));
  TagsView->setHeaderLabels({ tr("Key"), tr("Value") });
  TagsView->setRootIsDecorated(false);
  TagsView->installEventFilter(this);
  connect(TagsView, &QTreeWidget::currentItemChanged, this, [this]() { UpdateControls(); });
  connect(TagsView, &QTreeWidget::itemDoubleClicked, this, [this]()
  {
    if (EditTagButton->isEnabled())
    {
      AddEditTag(false);
    }
  });
  TagsLayout->addWidget(TagsView, 0, 0, 4, 1);
  AddTagButton = new QPushButton(tr("&Add..."), TagsSheet);
  AddTagButton->setObjectName(QStringLiteral("AddTagButton"));
  connect(AddTagButton, &QPushButton::clicked, this, [this]() { AddEditTag(true); });
  TagsLayout->addWidget(AddTagButton, 0, 1);
  EditTagButton = new QPushButton(tr("&Edit..."), TagsSheet);
  EditTagButton->setObjectName(QStringLiteral("EditTagButton"));
  connect(EditTagButton, &QPushButton::clicked, this, [this]() { AddEditTag(false); });
  TagsLayout->addWidget(EditTagButton, 1, 1);
  RemoveTagButton = new QPushButton(tr("&Remove"), TagsSheet);
  RemoveTagButton->setObjectName(QStringLiteral("RemoveTagButton"));
  connect(RemoveTagButton, &QPushButton::clicked, this, [this]() { RemoveTagButtonClick(); });
  TagsLayout->addWidget(RemoveTagButton, 2, 1);
  TagsLayout->setRowStretch(3, 1);
  for (QPushButton * Button : { AddTagButton, EditTagButton, RemoveTagButton })
  {
    Button->setAutoDefault(false);
  }

  QDialogButtonBox * Buttons = new QDialogButtonBox(this);
  OkButton = Buttons->addButton(QDialogButtonBox::Ok);
  OkButton->setObjectName(QStringLiteral("OkButton"));
  CancelButton = Buttons->addButton(QDialogButtonBox::Cancel);
  CancelButton->setObjectName(QStringLiteral("CancelButton"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  setMinimumWidth(fontMetrics().averageCharWidth() * 64);
}
//---------------------------------------------------------------------------
bool TPropertiesDialog::Execute(TRemoteProperties & Properties)
{
  SetFileProperties(Properties);

  PageControl->setCurrentWidget(CommonSheet);
  if (OwnerComboBox->isVisibleTo(this) && OwnerComboBox->isEnabled())
  {
    OwnerComboBox->setFocus();
  }
  else if (GroupComboBox->isVisibleTo(this) && GroupComboBox->isEnabled())
  {
    GroupComboBox->setFocus();
  }
  else if (RightsFrame->isVisibleTo(this) && RightsFrame->isEnabled())
  {
    RightsFrame->GetCheck(TRights::rrUserRead)->setFocus();
  }
  else
  {
    CancelButton->setFocus();
  }

  if (DebugAlwaysTrue(FChecksumAlgs != NULL))
  {
    QSignalBlocker Blocker(ChecksumAlgEdit);
    for (int Index = 0; Index < FChecksumAlgs->Count; Index++)
    {
      ChecksumAlgEdit->addItem(ToQString(FChecksumAlgs->Strings[Index]));
    }
    int ChecksumIndex = FChecksumAlgs->IndexOf(GUIConfiguration->ChecksumAlg);
    if (ChecksumIndex < 0)
    {
      ChecksumIndex = 0;
    }
    ChecksumAlgEdit->setCurrentIndex(ChecksumIndex);
  }
  ResetChecksum();

  FShown = true;
  UpdateControls();

  bool Result = (exec() == QDialog::Accepted);

  if (Result)
  {
    Properties = GetFileProperties();
  }

  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TPropertiesDialog::LoadRemoteToken(const TRemoteToken & Token)
{
  UnicodeString Result;
  if (FLAGSET(FOptions, poUserGroupByID))
  {
    if (Token.IDValid)
    {
      if (Token.NameValid)
      {
        Result = FORMAT(L"%s [%d]", (Token.Name, int(Token.ID)));
      }
      else
      {
        Result = FORMAT(L"[%d]", (int(Token.ID)));
      }
    }
    else
    {
      // be it valid or not
      Result = Token.Name;
    }
  }
  else
  {
    // what if name is not filled in?
    Result = Token.Name;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TPropertiesDialog::LoadRemoteToken(
  QComboBox * ComboBox, QLineEdit * View, QLabel * Label, bool Valid, const TRemoteToken & Token, int Change)
{
  UnicodeString Value = Valid ? LoadRemoteToken(Token) : EmptyStr;
  {
    QSignalBlocker Blocker(ComboBox);
    ComboBox->setEditText(ToQString(Value));
  }
  View->setText(ToQString(Value));
  bool AllowedChange = FLAGSET(FAllowedChanges, Change);
  ComboBox->setVisible(AllowedChange);
  View->setVisible(Valid && !AllowedChange);
  QWidget * FocusControl = AllowedChange ? static_cast<QWidget *>(ComboBox) : static_cast<QWidget *>(View);
  Label->setBuddy(FocusControl);
  Label->setVisible(!FocusControl->isHidden());
}
//---------------------------------------------------------------------------
void TPropertiesDialog::LoadRemoteTokens(QComboBox * ComboBox, const TRemoteTokenList * List)
{
  QSignalBlocker Blocker(ComboBox);
  ComboBox->clear();
  if (List != NULL)
  {
    int Count = List->Count();
    for (int Index = 0; Index < Count; Index++)
    {
      ComboBox->addItem(ToQString(LoadRemoteToken(*List->Token(Index))));
    }
  }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::LoadInfo()
{
  DebugAssert(FFileList->Count > 0);
  FMultiple = FFileList->Count > 1;
  FMultipleChecksum = FMultiple;

  FAllowCalculateStats = false;
  FStatsNotCalculated = false;

  __int64 FilesSize = 0;
  TCalculateSizeStats Stats;

  for (int Index = 0; Index < FFileList->Count; Index++)
  {
    TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->Objects[Index]);
    if (File->IsDirectory)
    {
      Stats.Directories++;
      // we should use TTerminal::CanRecurseToDirectory instead
      if (!File->IsSymLink)
      {
        FAllowCalculateStats = true;
        FStatsNotCalculated = true;
        FMultipleChecksum = true;
      }
    }
    else
    {
      Stats.Files++;
    }

    if (File->IsSymLink)
    {
      Stats.SymLinks++;
    }
    FilesSize += File->Size;
  }

  // before it gets eventualy cleared if !FMultiple
  bool ShowTags = FLAGSET(FOptions, poTags) && (Stats.Files == 1) && (Stats.Directories == 0);

  LoadRemoteTokens(GroupComboBox, FGroupList);
  LoadRemoteTokens(OwnerComboBox, FUserList);

  FAnyDirectories = (Stats.Directories > 0);
  RightsFrame->SetAllowAddXToDirectories(FAnyDirectories);

  if (!FMultiple)
  {
    // Show only file name, if we have only single file/directory.
    // For directory, this changes, once "Calculate" button is pressed
    Stats = TCalculateSizeStats();
  }

  LoadStats(FilesSize, Stats);

  RightsFrame->SetAllowUndef(FMultiple);

  int IconSize = QApplication::style()->pixelMetric(QStyle::PM_LargeIconSize, nullptr, this);
  FileIconImage->setFixedSize(IconSize, IconSize);
  if (!FMultiple)
  {
    TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->Objects[0]);
    DebugAssert(File);

    QString Type;
    QIcon Icon;
    FileTypeAndIcon(ToQString(File->FileName), File->IsDirectory, Type, Icon);
    FileIconImage->setPixmap(Icon.pixmap(IconSize, IconSize));

    LinksToLabelLabel->setVisible(File->IsSymLink);
    LinksToLabel->setVisible(File->IsSymLink);
    if (File->IsSymLink)
    {
      LinksToLabel->setText(ToQString(File->LinkTo));
    }

    setWindowTitle(ToQString(FMTLOAD(PROPERTIES_FILE_CAPTION, (File->FileName))));
  }
  else
  {
    // Upstream's list has the names of remote files, the list here the
    // full paths (also of local files)
    TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->Objects[0]);
    setWindowTitle(ToQString(FMTLOAD(PROPERTIES_FILES_CAPTION, (File->FileName))));
    LinksToLabelLabel->hide();
    LinksToLabel->hide();
    QIcon Icon = QIcon::fromTheme(QStringLiteral("document-multiple"), QIcon::fromTheme(QStringLiteral("edit-copy")));
    FileIconImage->setPixmap(Icon.pixmap(IconSize, IconSize));
  }

  ChecksumGroup->setVisible(!FMultipleChecksum);
  ChecksumView->setVisible(FMultipleChecksum);

  PageControl->setTabVisible(PageControl->indexOf(TagsSheet), ShowTags);
}
//---------------------------------------------------------------------------
void TPropertiesDialog::LoadStats(__int64 FilesSize, const TCalculateSizeStats & Stats)
{
  UnicodeString SizeStr;
  UnicodeString FilesStr;
  if (FStatsNotCalculated)
  {
    SizeStr = LoadStr(PROPERTIES_UNKNOWN_SIZE);
  }
  else
  {
    SizeStr = FormatBytes(FilesSize);
    UnicodeString SizeUnorderedStr = FormatBytes(FilesSize, fbNone);
    if (SizeStr != SizeUnorderedStr)
    {
      SizeStr = FORMAT(L"%s (%s)", (SizeStr, SizeUnorderedStr));
    }
  }

  if (((Stats.Files + Stats.Directories) == 0) && !FMultiple)
  {
    TRemoteFile * File = static_cast<TRemoteFile *>(FFileList->Objects[0]);
    DebugAssert(File != NULL);
    FilesStr = File->FileName;
  }
  else
  {
    if (Stats.Files > 0)
    {
      FilesStr = (Stats.Files == 1) ? FMTLOAD(PROPERTIES_FILE, (Stats.Files)) :
        FMTLOAD(PROPERTIES_FILES, (Stats.Files));
      if (Stats.Directories > 0)
      {
        FilesStr = FORMAT(L"%s, ", (FilesStr));
      }
    }
    if (Stats.Directories > 0)
    {
      FilesStr += (Stats.Directories == 1) ? FMTLOAD(PROPERTIES_DIRECTORY, (Stats.Directories)) :
        FMTLOAD(PROPERTIES_DIRECTORIES, (Stats.Directories));
    }
    if (Stats.SymLinks > 0)
    {
      UnicodeString SymLinksStr;
      SymLinksStr = (Stats.SymLinks == 1) ? FMTLOAD(PROPERTIES_SYMLINK, (Stats.SymLinks)) :
        FMTLOAD(PROPERTIES_SYMLINKS, (Stats.SymLinks));
      FilesStr = FORMAT(L"%s (%s)", (FilesStr, SymLinksStr));
    }
  }

  SizeLabel->setText(ToQString(SizeStr));
  FileLabel->setText(ToQString(FilesStr));
}
//---------------------------------------------------------------------------
void TPropertiesDialog::SetFileProperties(const TRemoteProperties & Value)
{
  TValidProperties Valid;
  if (Value.Valid.Contains(vpRights) && (FLAGSET(FAllowedChanges, cpMode) || FLAGSET(FAllowedChanges, cpAcl)))
  {
    Valid << vpRights;
  }
  if (Value.Valid.Contains(vpOwner) && FLAGSET(FAllowedChanges, cpOwner))
  {
    Valid << vpOwner;
  }
  if (Value.Valid.Contains(vpGroup) && FLAGSET(FAllowedChanges, cpGroup))
  {
    Valid << vpGroup;
  }
  FOrigProperties = Value;
  FOrigProperties.Valid = Valid;
  FOrigProperties.Recursive = false;

  bool HasRights = Value.Valid.Contains(vpRights);
  RightsFrame->setVisible(HasRights);
  RightsLabel->setVisible(HasRights);
  if (HasRights)
  {
    RightsFrame->SetRights(Value.Rights);
    RightsFrame->SetAddXToDirectories(Value.AddXToDirectories);
  }
  else
  {
    RightsFrame->SetRights(TRights());
    RightsFrame->SetAddXToDirectories(false);
  }

  LoadRemoteToken(GroupComboBox, GroupView, GroupLabel, Value.Valid.Contains(vpGroup), Value.Group, cpGroup);
  LoadRemoteToken(OwnerComboBox, OwnerView, OwnerLabel, Value.Valid.Contains(vpOwner), Value.Owner, cpOwner);

  bool HasAnything = !GroupLabel->isHidden() || !OwnerLabel->isHidden() || HasRights;
  GroupOwnerRightsBevel->setVisible(HasAnything);

  RecursiveCheck2->setChecked(Value.Recursive);
  RecursiveCheck2->setVisible(
    (!GroupComboBox->isHidden() ||
     !OwnerComboBox->isHidden() ||
     // Recursion is always supported for permissions and never for ACL.
     // If this ever changes we will have to introduce respective capability check.
     (HasRights && !FLAGSET(FAllowedChanges, cpAcl))) &&
    FAnyDirectories);
  RecursiveBevel->setVisible(!RecursiveCheck2->isHidden() || HasRights);

  if (PageControl->isTabVisible(PageControl->indexOf(TagsSheet)))
  {
    TagsView->clear();
    if (Value.Valid.Contains(vpTags))
    {
      std::unique_ptr<TStrings> Tags(TextToStringList(Value.Tags));
      for (int Index = 0; Index + 1 < Tags->Count; Index += 2)
      {
        AddTag(Tags->Strings[Index], Tags->Strings[Index + 1]);
      }
    }
    TagsView->resizeColumnToContents(0);
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TPropertiesDialog::StoreRemoteToken(unsigned int ID,
  const UnicodeString & Text, const TRemoteTokenList * List, TRemoteToken & Result)
{
  DebugAssert(List != NULL);
  const TRemoteToken * Token = List->Find(ID);
  if (Token == NULL)
  {
    Result.ID = ID;
    Result.Name = Text;
  }
  else
  {
    Result = *Token;
  }
}
//---------------------------------------------------------------------------
TRemoteToken TPropertiesDialog::StoreRemoteToken(const TRemoteToken & Orig,
  UnicodeString Text, int Message, const TRemoteTokenList * List)
{
  TRemoteToken Result;
  Text = Text.Trim();
  if (!Text.IsEmpty())
  {
    if (FLAGSET(FOptions, poUserGroupByID))
    {
      DebugAssert(List != NULL);
      int IDStart = Text.LastDelimiter(L"[");
      if (!Text.IsEmpty() && (IDStart >= 0) && (Text[Text.Length()] == L']'))
      {
        int ID;
        UnicodeString IDStr = Text.SubString(IDStart + 1, Text.Length() - IDStart - 1);
        if (!TryStrToInt(IDStr, ID))
        {
          throw Exception(Message);
        }
        else
        {
          StoreRemoteToken(ID, Text.SubString(1, IDStart - 1).Trim(), List, Result);
        }
      }
      else
      {
        const TRemoteToken * Token = List->Find(Text);
        if (Token == NULL)
        {
          int ID;
          if (TryStrToInt(Text, ID))
          {
            StoreRemoteToken(ID, Text, List, Result);
          }
          else
          {
            throw Exception(MainInstructions(FMTLOAD(PROPERTIES_UNKNOWN_TOKEN, (Text))));
          }
        }
        else
        {
          Result = *Token;
        }
      }
    }
    else
    {
      Result.Name = Text;
    }
  }

  if (LoadRemoteToken(Result) == LoadRemoteToken(Orig))
  {
    Result = Orig;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TPropertiesDialog::StoreRemoteToken(QComboBox * ComboBox,
  int ChangeFlag, TValidProperty PropertyFlag, const TRemoteToken & Orig,
  TRemoteToken & Token, int Message, const TRemoteTokenList * List,
  TRemoteProperties & Properties)
{
  UnicodeString Text = ToUnicodeString(ComboBox->currentText()).Trim();
  if (FLAGSET(FAllowedChanges, ChangeFlag))
  {
    Token = StoreRemoteToken(Orig, Text, Message, List);
    if (Token.IsSet)
    {
      Properties.Valid << PropertyFlag;
    }
  }
}
//---------------------------------------------------------------------------
TRemoteProperties TPropertiesDialog::GetFileProperties()
{
  TRemoteProperties Result;

  if (FLAGSET(FAllowedChanges, cpMode) || FLAGSET(FAllowedChanges, cpAcl))
  {
    Result.Valid << vpRights;
    Result.Rights = RightsFrame->GetRights();
    Result.AddXToDirectories = RightsFrame->GetAddXToDirectories();
  }

  StoreRemoteToken(GroupComboBox, cpGroup, vpGroup, FOrigProperties.Group,
    Result.Group, PROPERTIES_INVALID_GROUP, FGroupList, Result);
  StoreRemoteToken(OwnerComboBox, cpOwner, vpOwner, FOrigProperties.Owner,
    Result.Owner, PROPERTIES_INVALID_OWNER, FUserList, Result);

  Result.Recursive = RecursiveCheck2->isChecked();

  if (PageControl->isTabVisible(PageControl->indexOf(TagsSheet)))
  {
    std::unique_ptr<TStrings> Tags(new TStringList());
    for (int Index = 0; Index < TagsView->topLevelItemCount(); Index++)
    {
      QTreeWidgetItem * Item = TagsView->topLevelItem(Index);
      Tags->Add(ToUnicodeString(Item->text(0)));
      Tags->Add(ToUnicodeString(Item->text(1)));
    }
    Result.Tags = Tags->Text;
    Result.Valid << vpTags;
  }

  return Result;
}
//---------------------------------------------------------------------------
void TPropertiesDialog::ControlChange()
{
  if (FShown)
  {
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::UpdateControls()
{
  // No point enabling recursive check if there's no change allowed (supported),
  // i.e. with WebDAV.
  bool AnyAllowedChanges =
    FLAGSET(FAllowedChanges, cpGroup) || FLAGSET(FAllowedChanges, cpOwner) ||
    FLAGSET(FAllowedChanges, cpMode) || FLAGSET(FAllowedChanges, cpAcl);
  RecursiveCheck2->setEnabled(AnyAllowedChanges);

  bool Allow;
  try
  {
    Allow =
      !TRemoteProperties::ChangedProperties(FOrigProperties, GetFileProperties()).Valid.Empty() ||
      (RecursiveCheck2->isEnabled() && RecursiveCheck2->isChecked());
  }
  catch (...)
  {
    // when properties are invalid allow submitting the form,
    // because that reveals the cause to the user, otherwise he/she
    // may not be able to tell what is wrong

    Allow = true;
  }
  OkButton->setEnabled(Allow);

  GroupComboBox->setEnabled(FLAGSET(FAllowedChanges, cpGroup));
  OwnerComboBox->setEnabled(FLAGSET(FAllowedChanges, cpOwner));
  RightsFrame->setEnabled(FLAGSET(FAllowedChanges, cpMode) || FLAGSET(FAllowedChanges, cpAcl));
  CalculateSizeButton->setVisible(FAllowCalculateStats);

  if (!FMultiple)
  {
    // when setting properties for one file only, allow undef state
    // only when the input right explicitly requires it or
    // when "recursive" is on (possible for directory only).
    bool AllowUndef =
      (FOrigProperties.Valid.Contains(vpRights) &&
       FOrigProperties.Rights.AllowUndef) ||
      (RecursiveCheck2->isChecked());
    if (!AllowUndef)
    {
      // when disallowing undef state, make sure, all undef are turned into unset
      RightsFrame->SetRights(TRights(RightsFrame->GetRights().NumberSet));
    }
    RightsFrame->SetAllowUndef(AllowUndef);
  }

  PageControl->setTabEnabled(PageControl->indexOf(ChecksumSheet), ChecksumSupported());
  ChecksumButton->setEnabled(ChecksumSupported() && !ChecksumAlgEdit->currentText().isEmpty());
  ChecksumEdit->setVisible(!ChecksumEdit->text().isEmpty());
  ChecksumUnknownLabel->setVisible(ChecksumEdit->isHidden());

  EditTagButton->setEnabled(TagsView->currentItem() != nullptr);
  RemoveTagButton->setEnabled(TagsView->currentItem() != nullptr);
}
//---------------------------------------------------------------------------
void TPropertiesDialog::accept()
{
  // Invalid owners and groups are reported, the dialog stays open
  try
  {
    GetFileProperties();
  }
  catch (Exception & E)
  {
    ShowExtendedException(&E);
    return;
  }
  QDialog::accept();
}
//---------------------------------------------------------------------------
void TPropertiesDialog::CalculateSizeButtonClick()
{
  DebugAssert(FOnCalculateSize);

  bool DoClose = false;
  setEnabled(false);
  ::rtl::TFinally Finally([&]()
  {
    setEnabled(true);
    if (DoClose)
    {
      reject();
    }
  });
  __int64 Size;
  TCalculateSizeStats Stats;
  FOnCalculateSize(FFileList, Size, Stats, DoClose);
  FStatsNotCalculated = false;
  LoadStats(Size, Stats);
}
//---------------------------------------------------------------------------
void TPropertiesDialog::ResetChecksum()
{
  ChecksumView->clear();
  ChecksumEdit->clear();
}
//---------------------------------------------------------------------------
void TPropertiesDialog::CalculateChecksum()
{
  DebugAssert(FOnCalculateChecksum);

  ResetChecksum();
  FChecksumLoaded = true;
  FAlgUsed = UnicodeString();

  bool DoClose = false;
  {
    ::rtl::TFinally Finally([&]()
    {
      if (DoClose)
      {
        reject();
      }
    });
    FOnCalculateChecksum(ToUnicodeString(ChecksumAlgEdit->currentText()), FFileList,
      ::rtl::MethodPtr(this, &TPropertiesDialog::CalculatedChecksum), DoClose);
  }

  // If we successfully used the selected checksum, remember it (in normalized form)
  if (!FAlgUsed.IsEmpty())
  {
    GUIConfiguration->ChecksumAlg = FAlgUsed;
  }

  ChecksumView->resizeColumnToContents(0);
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPropertiesDialog::CalculatedChecksum(
  const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash)
{
  if (FMultipleChecksum)
  {
    QTreeWidgetItem * Item = new QTreeWidgetItem(ChecksumView);
    Item->setText(0, ToQString(FileName));
    Item->setText(1, ToQString(Hash));
  }
  else
  {
    ChecksumEdit->setText(ToQString(Hash));
  }
  FAlgUsed = Alg;
  UpdateControls();
}
//---------------------------------------------------------------------------
void TPropertiesDialog::NeedChecksum()
{
  if (!FChecksumLoaded && ChecksumSupported())
  {
    CalculateChecksum();
  }
}
//---------------------------------------------------------------------------
bool TPropertiesDialog::ChecksumSupported()
{
  return static_cast<bool>(FOnCalculateChecksum);
}
//---------------------------------------------------------------------------
void TPropertiesDialog::PageControlChange()
{
  if (FShown && (PageControl->currentWidget() == ChecksumSheet))
  {
    // Calculated when the page shows (the progress of the calculation is
    // over the dialog)
    QMetaObject::invokeMethod(this, [this]() { NeedChecksum(); }, Qt::QueuedConnection);
  }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::ChecksumAlgEditChange()
{
  if (FShown)
  {
    ResetChecksum();
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::CopyItems(QTreeWidget * View)
{
  QMenu Menu(this);
  Menu.setObjectName(QStringLiteral("ListViewMenu"));
  QAction * Copy = Menu.addAction(tr("&Copy"));
  Copy->setObjectName(QStringLiteral("Copy"));
  Copy->setEnabled(!View->selectedItems().isEmpty());
  if (Menu.exec(QCursor::pos()) == Copy)
  {
    QStringList Lines;
    QString SingleText;
    for (QTreeWidgetItem * Item : View->selectedItems())
    {
      SingleText = Item->text(1);
      QString Entry = Item->text(0);
      if (!SingleText.isEmpty())
      {
        Entry += QStringLiteral(" = ") + SingleText;
      }
      Lines.append(Entry);
    }
    QApplication::clipboard()->setText(
      ((View == ChecksumView) && (Lines.size() == 1)) ? SingleText : Lines.join(QLatin1Char('\n')) + QLatin1Char('\n'));
  }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::ValidateRemoteToken(
  const TRemoteToken & Orig, int Message, QComboBox * ComboBox, const TRemoteTokenList * List)
{
  // The token is normalized when valid; an invalid one is reported when
  // the dialog is submitted
  try
  {
    UnicodeString Text = LoadRemoteToken(StoreRemoteToken(Orig, ToUnicodeString(ComboBox->currentText()), Message, List));
    if (ToQString(Text) != ComboBox->currentText())
    {
      ComboBox->setEditText(ToQString(Text));
    }
  }
  catch (Exception &)
  {
  }
}
//---------------------------------------------------------------------------
bool TPropertiesDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if (Event->type() == QEvent::FocusOut)
  {
    if (Object == GroupComboBox)
    {
      ValidateRemoteToken(FOrigProperties.Group, PROPERTIES_INVALID_GROUP, GroupComboBox, FGroupList);
    }
    else if (Object == OwnerComboBox)
    {
      ValidateRemoteToken(FOrigProperties.Owner, PROPERTIES_INVALID_OWNER, OwnerComboBox, FUserList);
    }
  }
  else if ((Object == TagsView) && (Event->type() == QEvent::KeyPress))
  {
    QKeyEvent * KeyEvent = static_cast<QKeyEvent *>(Event);
    if (RemoveTagButton->isEnabled() && (KeyEvent->key() == Qt::Key_Delete))
    {
      RemoveTagButtonClick();
      return true;
    }
    if (AddTagButton->isEnabled() && (KeyEvent->key() == Qt::Key_Insert))
    {
      AddEditTag(true);
      return true;
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TPropertiesDialog::AddTag(const UnicodeString & Key, const UnicodeString & Value)
{
  QTreeWidgetItem * Item = new QTreeWidgetItem(TagsView);
  Item->setText(0, ToQString(Key));
  Item->setText(1, ToQString(Value));
  return Item;
}
//---------------------------------------------------------------------------
void TPropertiesDialog::AddEditTag(bool Add)
{
  std::unique_ptr<TStrings> Tags(CreateSortedStringList(true));
  QTreeWidgetItem * ItemFocused = TagsView->currentItem();
  for (int Index = 0; Index < TagsView->topLevelItemCount(); Index++)
  {
    QTreeWidgetItem * Item = TagsView->topLevelItem(Index);
    if (Add || (Item != ItemFocused))
    {
      Tags->Add(ToUnicodeString(Item->text(0)));
    }
  }

  UnicodeString Key, Value;
  if (!Add)
  {
    Key = ToUnicodeString(ItemFocused->text(0));
    Value = ToUnicodeString(ItemFocused->text(1));
  }
  if (DoTagDialog(Add, Tags.get(), Key, Value))
  {
    if (Add)
    {
      QTreeWidgetItem * Item = AddTag(Key, Value);
      TagsView->setCurrentItem(Item);
      TagsView->scrollToItem(Item);
    }
    else
    {
      ItemFocused->setText(0, ToQString(Key));
      ItemFocused->setText(1, ToQString(Value));
    }
    TagsView->resizeColumnToContents(0);
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TPropertiesDialog::RemoveTagButtonClick()
{
  QTreeWidgetItem * Item = TagsView->currentItem();
  if (Item != nullptr)
  {
    int Index = TagsView->indexOfTopLevelItem(Item);
    delete Item;
    int Count = TagsView->topLevelItemCount();
    if (Count > 0)
    {
      TagsView->setCurrentItem(TagsView->topLevelItem((Index < Count) ? Index : Count - 1));
    }
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
