// The location profiles: pairs of a local and a remote directory, of the
// site or shared, in folders (upstream's TLocationProfilesDialog,
// forms/LocationProfiles.dfm).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Bookmarks.h>
#include <Terminal.h>

#include "CustomDialog.h"
#include "ShortCuts.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDropEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
void BookmarkNameValidateName(const UnicodeString & Name)
{
  if (Name.IsEmpty() || IsNumber(Name))
  {
    throw Exception(FMTLOAD(BOOKMARK_INVALID_NAME, (Name)));
  }
}
//---------------------------------------------------------------------------
void BookmarkFolderValidateName(const UnicodeString & Name, bool AllowEmpty)
{
  if ((!AllowEmpty && Name.IsEmpty()) || Name.Pos(L"\\"))
  {
    throw Exception(FMTLOAD(BOOKMARK_FOLDER_INVALID_NAME, (Name)));
  }
}
//---------------------------------------------------------------------------
QComboBox * CreateNameCombo(QWidget * Parent, TStrings * Items)
{
  QComboBox * Combo = new QComboBox(Parent);
  Combo->setObjectName(QStringLiteral("NameCombo"));
  Combo->setEditable(true);
  Combo->setInsertPolicy(QComboBox::NoInsert);
  Combo->setCompleter(nullptr);
  for (int Index = 0; Index < Items->Count; Index++)
  {
    Combo->addItem(ToQString(Items->Strings[Index]));
  }
  return Combo;
}
//---------------------------------------------------------------------------
class TBookmarkNameDialog : public TCustomDialog
{
public:
  TBookmarkNameDialog(TStrings * PeerBookmarks, bool AllowShared);

  bool Execute(UnicodeString & Name, bool & Shared);

protected:
  virtual void DoValidate() override;

private:
  QComboBox * NameCombo;
  QCheckBox * SharedCheck;
};
//---------------------------------------------------------------------------
TBookmarkNameDialog::TBookmarkNameDialog(TStrings * PeerBookmarks, bool AllowShared)
{
  setObjectName(QStringLiteral("BookmarkNameDialog"));
  setWindowTitle(ToQString(LoadStr(ADD_BOOKMARK_CAPTION)));

  NameCombo = CreateNameCombo(this, PeerBookmarks);
  AddEditLikeControl(NameCombo, CreateLabel(LoadStr(ADD_BOOKMARK_PROMPT)));

  if (AllowShared)
  {
    SharedCheck = CreateAndAddCheckBox(LoadStr(ADD_BOOKMARK_SHARED));
    SharedCheck->setObjectName(QStringLiteral("SharedCheck"));
  }
  else
  {
    SharedCheck = nullptr;
  }
  setMinimumWidth(fontMetrics().averageCharWidth() * 55);
}
//---------------------------------------------------------------------------
void TBookmarkNameDialog::DoValidate()
{
  BookmarkNameValidateName(ToUnicodeString(NameCombo->currentText()));
  TCustomDialog::DoValidate();
}
//---------------------------------------------------------------------------
bool TBookmarkNameDialog::Execute(UnicodeString & Name, bool & Shared)
{
  NameCombo->setEditText(ToQString(Name));
  if (SharedCheck != nullptr)
  {
    SharedCheck->setChecked(Shared);
  }
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    Name = ToUnicodeString(NameCombo->currentText());
    if (SharedCheck != nullptr)
    {
      Shared = SharedCheck->isChecked();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class TBookmarkFolderDialog : public TCustomDialog
{
public:
  explicit TBookmarkFolderDialog(TStrings * Folders);

  bool Execute(UnicodeString & Name);

protected:
  virtual void DoValidate() override;

private:
  QComboBox * NameCombo;
};
//---------------------------------------------------------------------------
TBookmarkFolderDialog::TBookmarkFolderDialog(TStrings * Folders)
{
  setObjectName(QStringLiteral("BookmarkFolderDialog"));
  setWindowTitle(ToQString(LoadStr(MOVE_BOOKMARK_CAPTION)));

  NameCombo = CreateNameCombo(this, Folders);
  AddEditLikeControl(NameCombo, CreateLabel(LoadStr(MOVE_BOOKMARK_PROMPT)));
  setMinimumWidth(fontMetrics().averageCharWidth() * 45);
}
//---------------------------------------------------------------------------
void TBookmarkFolderDialog::DoValidate()
{
  BookmarkFolderValidateName(ToUnicodeString(NameCombo->currentText()), true);
  TCustomDialog::DoValidate();
}
//---------------------------------------------------------------------------
bool TBookmarkFolderDialog::Execute(UnicodeString & Name)
{
  NameCombo->setEditText(ToQString(Name));
  bool Result = TCustomDialog::Execute();
  if (Result)
  {
    Name = ToUnicodeString(NameCombo->currentText());
  }
  return Result;
}
//---------------------------------------------------------------------------
// The tree of the profiles; a dropped profile is moved by the dialog
// (upstream's DropTarget handling)
class TProfilesView : public QTreeWidget
{
public:
  explicit TProfilesView(QWidget * Parent) : QTreeWidget(Parent) {}

  std::function<void(QTreeWidgetItem * Source, QTreeWidgetItem * Target)> OnMove;
  bool IsEditing() const { return (state() == EditingState); }

protected:
  virtual void dragMoveEvent(QDragMoveEvent * Event) override
  {
    QTreeWidget::dragMoveEvent(Event);
    QTreeWidgetItem * Target = itemAt(Event->position().toPoint());
    if ((Target == nullptr) || (Target == currentItem()))
    {
      Event->ignore();
    }
  }

  virtual void dropEvent(QDropEvent * Event) override
  {
    QTreeWidgetItem * Source = currentItem();
    QTreeWidgetItem * Target = itemAt(Event->position().toPoint());
    // The tree itself does not move anything
    Event->setDropAction(Qt::IgnoreAction);
    Event->accept();
    if ((Source != nullptr) && (Target != nullptr) && (Source != Target) && OnMove)
    {
      OnMove(Source, Target);
    }
  }
};
//---------------------------------------------------------------------------
struct TProfilesPage
{
  QWidget * Sheet;
  TProfilesView * View;
  QPushButton * AddButton;
  QPushButton * RemoveButton;
  QPushButton * RenameButton;
  QPushButton * MoveToButton;
  QPushButton * UpButton;
  QPushButton * DownButton;
  QPushButton * ShortCutButton;
  TBookmarkList * BookmarkList;
  // The sorted names of the folders, with their items
  TStringList * Folders;
};
//---------------------------------------------------------------------------
class TLocationProfilesDialog : public QDialog
{
public:
  TLocationProfilesDialog();
  virtual ~TLocationProfilesDialog();

  bool Execute();

  UnicodeString GetLocalDirectory();
  void SetLocalDirectory(const UnicodeString & Value);
  UnicodeString GetRemoteDirectory();
  void SetRemoteDirectory(const UnicodeString & Value);
  void SetLocalDirectories(TStrings * Value) { SetDirectories(LocalDirectoryEdit, Value); }
  void SetRemoteDirectories(TStrings * Value) { SetDirectories(RemoteDirectoryEdit, Value); }
  void SetOperationSide(TOperationSide Value) { FOperationSide = Value; }
  void SetTerminal(TTerminal * Value) { FTerminal = Value; }
  void SetMode(TOpenDirectoryMode Value) { FMode = Value; }

protected:
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  TOperationSide FOperationSide;
  TTerminal * FTerminal;
  TOpenDirectoryMode FMode;
  UnicodeString FSessionKey;
  bool FChanging;
  bool FBookmarkSelected;
  int FUpdating;
  TProfilesPage FSession;
  TProfilesPage FShared;

  QComboBox * LocalDirectoryEdit;
  QComboBox * RemoteDirectoryEdit;
  QTabWidget * PageControl;
  QPushButton * OKBtn;

  void CreatePage(TProfilesPage & Page, const char * Prefix, const QString & Caption, bool ShortCut);
  void SetDirectories(QComboBox * Edit, TStrings * Value);
  TProfilesPage & GetPage(QWidget * Sheet);
  QWidget * GetProfilesSheet();
  static TBookmark * GetBookmark(QTreeWidgetItem * Item);
  QTreeWidgetItem * CreateItem(const UnicodeString & Text, TBookmark * Bookmark);
  static int AbsoluteIndex(QTreeWidgetItem * Item);
  static void TakeItem(QTreeWidgetItem * Item);
  void SetItemText(QTreeWidgetItem * Item, const UnicodeString & Text);
  void LoadBookmarks(TProfilesPage & Page, TBookmarkList * Source);
  bool ProfileMatch(QTreeWidgetItem * Item);
  void FindProfile(TProfilesPage & Page);
  void FindProfile();
  void UpdateControls();
  void UpdateProfilesControls(TProfilesPage & Page);
  bool AddAsBookmark(TProfilesPage & Page, bool Initial);
  void RemoveBookmark(TProfilesPage & Page);
  void BookmarkMove(TProfilesPage & Page, QTreeWidgetItem * Source, QTreeWidgetItem * Dest);
  void BookmarkButtonClick(TProfilesPage & Page, bool Up);
  void BookmarkMoveToButtonClick(TProfilesPage & Page);
  QTreeWidgetItem * InsertFolder(TProfilesPage & Page, const UnicodeString & Name, QTreeWidgetItem * Item);
  void RenameBookmark(TProfilesPage & Page);
  void ProfilesViewEdited(TProfilesPage & Page, QTreeWidgetItem * Item);
  void ProfilesViewChange(TProfilesPage & Page);
  void ShortCutBookmarkButtonClick(TProfilesPage & Page);
  void DirectoryEditChange();
  UnicodeString BookmarkText(TBookmark * Bookmark);
};
//---------------------------------------------------------------------------
QPushButton * CreateButton(QWidget * Parent, const QString & Name, const QString & Caption)
{
  QPushButton * Button = new QPushButton(Caption, Parent);
  Button->setObjectName(Name);
  Button->setAutoDefault(false);
  return Button;
}
//---------------------------------------------------------------------------
QComboBox * CreateDirectoryEdit(QWidget * Parent, const char * Name)
{
  QComboBox * Edit = new QComboBox(Parent);
  Edit->setObjectName(QString::fromLatin1(Name));
  Edit->setEditable(true);
  Edit->setInsertPolicy(QComboBox::NoInsert);
  Edit->setCompleter(nullptr);
  Edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  return Edit;
}
//---------------------------------------------------------------------------
TLocationProfilesDialog::TLocationProfilesDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("LocationProfilesDialog"));
  setWindowTitle(tr("Location profiles"));
  FOperationSide = osLocal;
  FTerminal = NULL;
  FMode = odBrowse;
  FChanging = false;
  FBookmarkSelected = false;
  FUpdating = 0;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGridLayout * EditLayout = new QGridLayout();
  QLabel * LocalDirectoryLabel = new QLabel(tr("&Local directory:"), this);
  LocalDirectoryLabel->setObjectName(QStringLiteral("LocalDirectoryLabel"));
  EditLayout->addWidget(LocalDirectoryLabel, 0, 0, 1, 2);
  LocalDirectoryEdit = CreateDirectoryEdit(this, "LocalDirectoryEdit");
  LocalDirectoryEdit->setMinimumWidth(fontMetrics().averageCharWidth() * 60);
  LocalDirectoryLabel->setBuddy(LocalDirectoryEdit);
  EditLayout->addWidget(LocalDirectoryEdit, 1, 0);
  QPushButton * LocalDirectoryBrowseButton = CreateButton(this, QStringLiteral("LocalDirectoryBrowseButton"), tr("B&rowse..."));
  connect(LocalDirectoryBrowseButton, &QPushButton::clicked, this, [this]()
  {
    QString Directory =
      QFileDialog::getExistingDirectory(this, ToQString(LoadStr(SELECT_LOCAL_DIRECTORY)), LocalDirectoryEdit->currentText());
    if (!Directory.isEmpty())
    {
      LocalDirectoryEdit->setEditText(Directory);
    }
  });
  EditLayout->addWidget(LocalDirectoryBrowseButton, 1, 1);
  QLabel * RemoteDirectoryLabel = new QLabel(tr("&Remote directory:"), this);
  RemoteDirectoryLabel->setObjectName(QStringLiteral("RemoteDirectoryLabel"));
  EditLayout->addWidget(RemoteDirectoryLabel, 2, 0, 1, 2);
  RemoteDirectoryEdit = CreateDirectoryEdit(this, "RemoteDirectoryEdit");
  RemoteDirectoryLabel->setBuddy(RemoteDirectoryEdit);
  EditLayout->addWidget(RemoteDirectoryEdit, 3, 0, 1, 2);
  for (QComboBox * Edit : { LocalDirectoryEdit, RemoteDirectoryEdit })
  {
    connect(Edit, &QComboBox::currentTextChanged, this, [this]() { DirectoryEditChange(); });
  }
  MainLayout->addLayout(EditLayout);

  PageControl = new QTabWidget(this);
  PageControl->setObjectName(QStringLiteral("PageControl"));
  CreatePage(FSession, "Session", tr("Site Location profiles"), false);
  CreatePage(FShared, "Shared", tr("Shared Location profiles"), true);
  MainLayout->addWidget(PageControl, 1);

  QHBoxLayout * ButtonsLayout = new QHBoxLayout();
  QPushButton * SwitchButton = CreateButton(this, QStringLiteral("SwitchButton"), tr("&Bookmarks..."));
  connect(SwitchButton, &QPushButton::clicked, this, [this]()
  {
    AppConfiguration->UseLocationProfiles = false;
    reject();
  });
  ButtonsLayout->addWidget(SwitchButton);
  ButtonsLayout->addStretch(1);
  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  OKBtn = Buttons->button(QDialogButtonBox::Ok);
  OKBtn->setObjectName(QStringLiteral("OKBtn"));
  Buttons->button(QDialogButtonBox::Cancel)->setObjectName(QStringLiteral("CancelBtn"));
  connect(Buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  ButtonsLayout->addWidget(Buttons);
  MainLayout->addLayout(ButtonsLayout);
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::CreatePage(TProfilesPage & Page, const char * Prefix, const QString & Caption, bool ShortCut)
{
  QString APrefix = QString::fromLatin1(Prefix);
  Page.BookmarkList = new TBookmarkList();
  Page.Folders = CreateSortedStringList();
  Page.Sheet = new QWidget(PageControl);
  Page.Sheet->setObjectName(APrefix + QStringLiteral("ProfilesSheet"));
  QHBoxLayout * Layout = new QHBoxLayout(Page.Sheet);
  Page.View = new TProfilesView(Page.Sheet);
  Page.View->setObjectName(APrefix + QStringLiteral("ProfilesView"));
  Page.View->setHeaderHidden(true);
  Page.View->setMinimumHeight(fontMetrics().height() * 12);
  Page.View->setEditTriggers(QAbstractItemView::EditKeyPressed);
  Page.View->setDragDropMode(QAbstractItemView::InternalMove);
  Page.View->installEventFilter(this);
  Layout->addWidget(Page.View, 1);
  QVBoxLayout * ButtonLayout = new QVBoxLayout();
  auto Name = [&](const char * Before, const char * After)
  {
    return QString::fromLatin1(Before) + APrefix + QString::fromLatin1(After);
  };
  Page.AddButton = CreateButton(Page.Sheet, Name("Add", "BookmarkButton"), tr("&Add..."));
  Page.RemoveButton = CreateButton(Page.Sheet, Name("Remove", "BookmarkButton"), tr("Remo&ve"));
  Page.RenameButton = CreateButton(Page.Sheet, Name("Rename", "BookmarkButton"), tr("Re&name"));
  Page.MoveToButton = CreateButton(Page.Sheet, Name("", "BookmarkMoveToButton"), tr("&Move To..."));
  Page.UpButton = CreateButton(Page.Sheet, Name("Up", "BookmarkButton"), tr("&Up"));
  Page.DownButton = CreateButton(Page.Sheet, Name("Down", "BookmarkButton"), tr("&Down"));
  for (QPushButton * Button :
         { Page.AddButton, Page.RemoveButton, Page.RenameButton, Page.MoveToButton, Page.UpButton, Page.DownButton })
  {
    ButtonLayout->addWidget(Button);
  }
  Page.ShortCutButton = nullptr;
  if (ShortCut)
  {
    Page.ShortCutButton = CreateButton(Page.Sheet, Name("ShortCut", "BookmarkButton"), tr("&Shortcut..."));
    ButtonLayout->addWidget(Page.ShortCutButton);
  }
  ButtonLayout->addStretch(1);
  Layout->addLayout(ButtonLayout);
  PageControl->addTab(Page.Sheet, Caption);

  TProfilesPage * P = &Page;
  connect(Page.AddButton, &QPushButton::clicked, this, [this, P]() { AddAsBookmark(*P, false); });
  connect(Page.RemoveButton, &QPushButton::clicked, this, [this, P]() { RemoveBookmark(*P); });
  connect(Page.RenameButton, &QPushButton::clicked, this, [this, P]() { RenameBookmark(*P); });
  connect(Page.MoveToButton, &QPushButton::clicked, this, [this, P]() { BookmarkMoveToButtonClick(*P); });
  connect(Page.UpButton, &QPushButton::clicked, this, [this, P]() { BookmarkButtonClick(*P, true); });
  connect(Page.DownButton, &QPushButton::clicked, this, [this, P]() { BookmarkButtonClick(*P, false); });
  if (Page.ShortCutButton != nullptr)
  {
    connect(Page.ShortCutButton, &QPushButton::clicked, this, [this, P]() { ShortCutBookmarkButtonClick(*P); });
  }
  connect(Page.View, &QTreeWidget::currentItemChanged, this, [this, P]()
  {
    if (FUpdating == 0)
    {
      ProfilesViewChange(*P);
    }
  });
  connect(Page.View, &QTreeWidget::itemSelectionChanged, this, [this]() { UpdateControls(); });
  connect(Page.View, &QTreeWidget::itemChanged, this, [this, P](QTreeWidgetItem * Item)
  {
    if (FUpdating == 0)
    {
      ProfilesViewEdited(*P, Item);
    }
  });
  connect(Page.View, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem * Item)
  {
    if (OKBtn->isEnabled() && (GetBookmark(Item) != NULL))
    {
      accept();
    }
  });
  connect(Page.View, &QTreeWidget::itemExpanded, this, [this, P](QTreeWidgetItem * Item)
  {
    if (FUpdating == 0)
    {
      P->BookmarkList->NodeOpened[ToUnicodeString(Item->text(0))] = true;
    }
  });
  connect(Page.View, &QTreeWidget::itemCollapsed, this, [this, P](QTreeWidgetItem * Item)
  {
    if (FUpdating == 0)
    {
      P->BookmarkList->NodeOpened[ToUnicodeString(Item->text(0))] = false;
    }
  });
  Page.View->OnMove = [this, P](QTreeWidgetItem * Source, QTreeWidgetItem * Target)
  {
    if (GetBookmark(Source) != NULL)
    {
      BookmarkMove(*P, Source, Target);
    }
  };
}
//---------------------------------------------------------------------------
TLocationProfilesDialog::~TLocationProfilesDialog()
{
  for (TProfilesPage * Page : { &FSession, &FShared })
  {
    delete Page->BookmarkList;
    delete Page->Folders;
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::SetDirectories(QComboBox * Edit, TStrings * Value)
{
  QString Text = Edit->currentText();
  {
    TAutoFlag ChangingFlag(FChanging);
    Edit->clear();
    for (int Index = 0; Index < Value->Count; Index++)
    {
      Edit->addItem(ToQString(Value->Strings[Index]));
    }
    Edit->setEditText(Text);
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::SetLocalDirectory(const UnicodeString & Value)
{
  LocalDirectoryEdit->setEditText(ToQString(Value));
}
//---------------------------------------------------------------------------
UnicodeString TLocationProfilesDialog::GetLocalDirectory()
{
  return ExcludeTrailingBackslash(ToUnicodeString(LocalDirectoryEdit->currentText()));
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::SetRemoteDirectory(const UnicodeString & Value)
{
  RemoteDirectoryEdit->setEditText(ToQString(Value));
}
//---------------------------------------------------------------------------
UnicodeString TLocationProfilesDialog::GetRemoteDirectory()
{
  return UnixExcludeTrailingBackslash(ToUnicodeString(RemoteDirectoryEdit->currentText()));
}
//---------------------------------------------------------------------------
TProfilesPage & TLocationProfilesDialog::GetPage(QWidget * Sheet)
{
  return (Sheet == FShared.Sheet) ? FShared : FSession;
}
//---------------------------------------------------------------------------
QWidget * TLocationProfilesDialog::GetProfilesSheet()
{
  return AppConfiguration->UseSharedBookmarks ? FShared.Sheet : FSession.Sheet;
}
//---------------------------------------------------------------------------
TBookmark * TLocationProfilesDialog::GetBookmark(QTreeWidgetItem * Item)
{
  return (Item != nullptr) ? static_cast<TBookmark *>(Item->data(0, Qt::UserRole).value<void *>()) : NULL;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLocationProfilesDialog::CreateItem(const UnicodeString & Text, TBookmark * Bookmark)
{
  QTreeWidgetItem * Item = new QTreeWidgetItem();
  Item->setText(0, ToQString(Text));
  Item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void *>(Bookmark)));
  Item->setIcon(0, QIcon::fromTheme((Bookmark != NULL) ? QStringLiteral("bookmarks") : QStringLiteral("folder")));
  Qt::ItemFlags Flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsDropEnabled;
  if (Bookmark != NULL)
  {
    Flags |= Qt::ItemIsDragEnabled;
  }
  Item->setFlags(Flags);
  return Item;
}
//---------------------------------------------------------------------------
int TLocationProfilesDialog::AbsoluteIndex(QTreeWidgetItem * Item)
{
  int Result = 0;
  for (QTreeWidgetItemIterator I(Item->treeWidget()); *I != Item; ++I)
  {
    Result++;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::TakeItem(QTreeWidgetItem * Item)
{
  if (Item->parent() != nullptr)
  {
    Item->parent()->removeChild(Item);
  }
  else
  {
    Item->treeWidget()->takeTopLevelItem(Item->treeWidget()->indexOfTopLevelItem(Item));
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::SetItemText(QTreeWidgetItem * Item, const UnicodeString & Text)
{
  TAutoNestingCounter UpdatingCounter(FUpdating);
  Item->setText(0, ToQString(Text));
}
//---------------------------------------------------------------------------
UnicodeString TLocationProfilesDialog::BookmarkText(TBookmark * Bookmark)
{
  UnicodeString Result = Bookmark->Name;
  if (!Result.IsEmpty() && (Bookmark->ShortCut != 0))
  {
    Result = FORMAT(L"%s (%s)", (Result, ShortCutToText(Bookmark->ShortCut)));
  }
  return Result;
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::LoadBookmarks(TProfilesPage & Page, TBookmarkList * Source)
{
  TAutoNestingCounter UpdatingCounter(FUpdating);
  if (Source != NULL)
  {
    Page.BookmarkList->Assign(Source);
  }
  else
  {
    Page.BookmarkList->Clear();
  }

  Page.Folders->Clear();
  for (int Index = 0; Index < Page.BookmarkList->Count; Index++)
  {
    TBookmark * Bookmark = Page.BookmarkList->Bookmarks[Index];
    if (!Bookmark->Node.IsEmpty() && (Page.Folders->IndexOf(Bookmark->Node) < 0))
    {
      Page.Folders->Add(Bookmark->Node);
    }
  }

  Page.View->clear();
  for (int Index = 0; Index < Page.Folders->Count; Index++)
  {
    QTreeWidgetItem * Item = CreateItem(Page.Folders->Strings[Index], NULL);
    Page.View->addTopLevelItem(Item);
    Page.Folders->Objects[Index] = reinterpret_cast<TObject *>(Item);
  }

  for (int Index = 0; Index < Page.BookmarkList->Count; Index++)
  {
    TBookmark * Bookmark = Page.BookmarkList->Bookmarks[Index];
    QTreeWidgetItem * Item = CreateItem(BookmarkText(Bookmark), Bookmark);
    if (!Bookmark->Node.IsEmpty())
    {
      QTreeWidgetItem * Parent =
        reinterpret_cast<QTreeWidgetItem *>(Page.Folders->Objects[Page.Folders->IndexOf(Bookmark->Node)]);
      Parent->addChild(Item);
      if (Parent->childCount() == 1)
      {
        // only now, when folder node has its first child, we can eventually expand it
        Parent->setExpanded(Page.BookmarkList->NodeOpened[ToUnicodeString(Parent->text(0))]);
      }
    }
    else
    {
      Page.View->addTopLevelItem(Item);
    }
  }
}
//---------------------------------------------------------------------------
bool TLocationProfilesDialog::Execute()
{
  if (DebugAlwaysTrue(FTerminal != NULL))
  {
    // cache session key, in case terminal is closed while the window is open
    FSessionKey = FTerminal->SessionData->SessionKey;
    LoadBookmarks(FSession, AppConfiguration->Bookmarks[FSessionKey]);
    LoadBookmarks(FShared, AppConfiguration->SharedBookmarks);
  }
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    PageControl->setCurrentWidget(GetProfilesSheet());
  }
  if (FMode == odAddBookmark)
  {
    AddAsBookmark(GetPage(GetProfilesSheet()), true);
  }
  FindProfile();
  ((FOperationSide == osLocal) ? LocalDirectoryEdit : RemoteDirectoryEdit)->setFocus();
  UpdateControls();

  FBookmarkSelected = false;
  bool Result = (exec() == QDialog::Accepted);
  if (FTerminal != NULL)
  {
    AppConfiguration->Bookmarks[FSessionKey] = FSession.BookmarkList;
    AppConfiguration->SharedBookmarks = FShared.BookmarkList;
    AppConfiguration->UseSharedBookmarks = (PageControl->currentWidget() == FShared.Sheet);
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TLocationProfilesDialog::ProfileMatch(QTreeWidgetItem * Item)
{
  TBookmark * Bookmark = GetBookmark(Item);
  return
    (Bookmark != NULL) &&
    (Bookmark->Local == GetLocalDirectory()) &&
    (Bookmark->Remote == GetRemoteDirectory());
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::FindProfile(TProfilesPage & Page)
{
  QTreeWidgetItem * Selected = Page.View->currentItem();
  if ((Selected == nullptr) || !Selected->isSelected() || !ProfileMatch(Selected))
  {
    QTreeWidgetItem * Match = nullptr;
    for (QTreeWidgetItemIterator I(Page.View); (*I != nullptr) && (Match == nullptr); ++I)
    {
      if (ProfileMatch(*I))
      {
        Match = *I;
      }
    }

    TAutoNestingCounter UpdatingCounter(FUpdating);
    if (Match != nullptr)
    {
      Page.View->setCurrentItem(Match);
      Page.View->scrollToItem(Match);
    }
    else
    {
      Page.View->setCurrentItem(nullptr);
      Page.View->clearSelection();
    }
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::FindProfile()
{
  FindProfile(FSession);
  FindProfile(FShared);
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::UpdateProfilesControls(TProfilesPage & Page)
{
  QTreeWidgetItem * Selected = Page.View->currentItem();
  if ((Selected != nullptr) && !Selected->isSelected())
  {
    Selected = nullptr;
  }
  bool HasBookmark = (GetBookmark(Selected) != NULL);
  auto Sibling = [&](int Offset) -> QTreeWidgetItem *
  {
    QTreeWidgetItem * Parent = Selected->parent();
    int Index = (Parent != nullptr) ? Parent->indexOfChild(Selected) : Page.View->indexOfTopLevelItem(Selected);
    Index += Offset;
    int Count = (Parent != nullptr) ? Parent->childCount() : Page.View->topLevelItemCount();
    if ((Index < 0) || (Index >= Count))
    {
      return nullptr;
    }
    return (Parent != nullptr) ? Parent->child(Index) : Page.View->topLevelItem(Index);
  };
  Page.AddButton->setEnabled(!GetLocalDirectory().IsEmpty() || !GetRemoteDirectory().IsEmpty());
  Page.RemoveButton->setEnabled(Selected != nullptr);
  Page.RenameButton->setEnabled(Selected != nullptr);
  Page.MoveToButton->setEnabled(HasBookmark);
  if (Page.ShortCutButton != nullptr)
  {
    Page.ShortCutButton->setEnabled(HasBookmark);
  }
  Page.UpButton->setEnabled(HasBookmark && (GetBookmark(Sibling(-1)) != NULL));
  Page.DownButton->setEnabled(HasBookmark && (GetBookmark(Sibling(1)) != NULL));
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::UpdateControls()
{
  OKBtn->setEnabled(!GetLocalDirectory().IsEmpty() || !GetRemoteDirectory().IsEmpty());
  UpdateProfilesControls(FSession);
  UpdateProfilesControls(FShared);
}
//---------------------------------------------------------------------------
bool TLocationProfilesDialog::AddAsBookmark(TProfilesPage & APage, bool Initial)
{
  TProfilesPage * Page = &APage;
  UnicodeString LocalDirectory = GetLocalDirectory();
  UnicodeString RemoteDirectory = GetRemoteDirectory();
  DebugAssert(!LocalDirectory.IsEmpty() || !RemoteDirectory.IsEmpty());

  UnicodeString BookmarkName;
  if (((FOperationSide == osLocal) && !LocalDirectory.IsEmpty()) || RemoteDirectory.IsEmpty())
  {
    BookmarkName = LocalDirectory;
  }
  else
  {
    BookmarkName = RemoteDirectory;
  }

  QTreeWidgetItem * Selected = Page->View->currentItem();
  if ((Selected != nullptr) && !Selected->isSelected())
  {
    Selected = nullptr;
  }
  TBookmark * SelectedBookmark = NULL;
  UnicodeString SelectedNode;
  if (Selected != nullptr)
  {
    SelectedBookmark = GetBookmark(Selected);
    SelectedNode = (SelectedBookmark != NULL) ? SelectedBookmark->Node : ToUnicodeString(Selected->text(0));
  }

  std::unique_ptr<TStrings> PeerBookmarks(new TStringList());
  for (int Index = 0; Index < Page->BookmarkList->Count; Index++)
  {
    TBookmark * Bookmark = Page->BookmarkList->Bookmarks[Index];
    if (Bookmark->Node == SelectedNode)
    {
      PeerBookmarks->Add(Bookmark->Name);
    }
  }

  TBookmarkNameDialog Dialog(PeerBookmarks.get(), Initial);
  bool Shared = AppConfiguration->UseSharedBookmarks;
  bool Result = Dialog.Execute(BookmarkName, Shared);
  if (Result)
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    if (Initial)
    {
      AppConfiguration->UseSharedBookmarks = Shared;
      PageControl->setCurrentWidget(GetProfilesSheet());
      Page = &GetPage(PageControl->currentWidget());
      Selected = nullptr;
      SelectedBookmark = NULL;
      SelectedNode = UnicodeString();
    }

    TBookmark * Bookmark = Page->BookmarkList->FindByName(SelectedNode, BookmarkName);
    if (Bookmark != NULL)
    {
      Bookmark->Local = LocalDirectory;
      Bookmark->Remote = RemoteDirectory;

      for (QTreeWidgetItemIterator I(Page->View); *I != nullptr; ++I)
      {
        if (GetBookmark(*I) == Bookmark)
        {
          Selected = *I;
          break;
        }
      }
    }
    else
    {
      Bookmark = new TBookmark();
      Bookmark->Name = BookmarkName;
      Bookmark->Local = LocalDirectory;
      Bookmark->Remote = RemoteDirectory;
      QTreeWidgetItem * Item = CreateItem(BookmarkText(Bookmark), Bookmark);
      if (SelectedBookmark != NULL)
      {
        Bookmark->Node = SelectedBookmark->Node;
        Page->BookmarkList->InsertBefore(SelectedBookmark, Bookmark);
        QTreeWidgetItem * Parent = Selected->parent();
        if (Parent != nullptr)
        {
          Parent->insertChild(Parent->indexOfChild(Selected), Item);
        }
        else
        {
          Page->View->insertTopLevelItem(Page->View->indexOfTopLevelItem(Selected), Item);
        }
      }
      else if (Selected != nullptr)
      {
        // must be a folder
        DebugAssert(Selected->parent() == nullptr); // more than one level of folders is not supported
        Bookmark->Node = ToUnicodeString(Selected->text(0));
        Page->BookmarkList->Add(Bookmark);
        Selected->addChild(Item);
        Selected->setExpanded(true);
      }
      else
      {
        Page->BookmarkList->Add(Bookmark);
        Page->View->addTopLevelItem(Item);
      }
      Selected = Item;
    }
    Page->View->setCurrentItem(Selected);
  }

  UpdateControls();
  return Result;
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::RemoveBookmark(TProfilesPage & Page)
{
  QTreeWidgetItem * Item = Page.View->currentItem();
  DebugAssert(Item != nullptr);
  TBookmark * Bookmark = GetBookmark(Item);
  if (Bookmark != NULL)
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    Page.BookmarkList->Delete(Bookmark);
    QTreeWidgetItem * Parent = Item->parent();
    delete Item;
    if ((Parent != nullptr) && (Parent->childCount() == 0))
    {
      Page.Folders->Delete(Page.Folders->IndexOfObject(reinterpret_cast<TObject *>(Parent)));
      delete Parent;
    }
  }
  else
  {
    UnicodeString Message = MainInstructions(LoadStr(DELETE_BOOKMARK_FOLDER));
    if (MessageDialog(Message, qtConfirmation, qaYes | qaNo) == qaYes)
    {
      TAutoNestingCounter UpdatingCounter(FUpdating);
      for (int Index = 0; Index < Item->childCount(); Index++)
      {
        Page.BookmarkList->Delete(GetBookmark(Item->child(Index)));
      }
      Page.Folders->Delete(Page.Folders->IndexOfObject(reinterpret_cast<TObject *>(Item)));
      delete Item;
    }
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::BookmarkMove(TProfilesPage & Page, QTreeWidgetItem * Source, QTreeWidgetItem * Dest)
{
  TBookmark * Bookmark = GetBookmark(Source);
  DebugAssert(Bookmark != NULL);
  QTreeWidgetItem * PrevFolderItem = Source->parent();

  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    TBookmark * DestBookmark = GetBookmark(Dest);
    if (DestBookmark == NULL)
    {
      Bookmark->Node = (Dest != nullptr) ? ToUnicodeString(Dest->text(0)) : UnicodeString();
      Page.BookmarkList->MoveTo(Page.BookmarkList->Bookmarks[Page.BookmarkList->Count - 1], Bookmark, false);
      TakeItem(Source);
      if (Dest != nullptr)
      {
        Dest->addChild(Source);
        Dest->setExpanded(true);
      }
      else
      {
        Page.View->addTopLevelItem(Source);
      }
    }
    else
    {
      bool Before = (AbsoluteIndex(Source) > AbsoluteIndex(Dest));
      Bookmark->Node = DestBookmark->Node;
      Page.BookmarkList->MoveTo(DestBookmark, Bookmark, Before);
      TakeItem(Source);
      QTreeWidgetItem * Parent = Dest->parent();
      int Index = (Parent != nullptr) ? Parent->indexOfChild(Dest) : Page.View->indexOfTopLevelItem(Dest);
      if (!Before)
      {
        Index++;
      }
      if (Parent != nullptr)
      {
        Parent->insertChild(Index, Source);
      }
      else
      {
        Page.View->insertTopLevelItem(Index, Source);
      }
    }

    if ((PrevFolderItem != nullptr) && (PrevFolderItem->childCount() == 0))
    {
      Page.Folders->Delete(Page.Folders->IndexOfObject(reinterpret_cast<TObject *>(PrevFolderItem)));
      delete PrevFolderItem;
    }

    Page.View->setCurrentItem(Source);
    Page.View->scrollToItem(Source);
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::BookmarkButtonClick(TProfilesPage & Page, bool Up)
{
  QTreeWidgetItem * Item = Page.View->currentItem();
  DebugAssert((Item != nullptr) && (GetBookmark(Item) != NULL));
  QTreeWidgetItem * Parent = Item->parent();
  int Index = ((Parent != nullptr) ? Parent->indexOfChild(Item) : Page.View->indexOfTopLevelItem(Item)) + (Up ? -1 : 1);
  int Count = (Parent != nullptr) ? Parent->childCount() : Page.View->topLevelItemCount();
  QTreeWidgetItem * Target = nullptr;
  if ((Index >= 0) && (Index < Count))
  {
    Target = (Parent != nullptr) ? Parent->child(Index) : Page.View->topLevelItem(Index);
  }
  BookmarkMove(Page, Item, (Target != nullptr) ? Target : Parent);
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TLocationProfilesDialog::InsertFolder(TProfilesPage & Page, const UnicodeString & Name, QTreeWidgetItem * Item)
{
  // The folders go first, sorted
  int Index = Page.Folders->AddObject(Name, reinterpret_cast<TObject *>(Item));
  Page.View->insertTopLevelItem(Index, Item);
  return Item;
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::BookmarkMoveToButtonClick(TProfilesPage & Page)
{
  QTreeWidgetItem * Item = Page.View->currentItem();
  TBookmark * Bookmark = GetBookmark(Item);
  DebugAssert(Bookmark != NULL);

  TBookmarkFolderDialog Dialog(Page.Folders);
  UnicodeString NodeName = Bookmark->Node;
  if (Dialog.Execute(NodeName) && (NodeName != Bookmark->Node))
  {
    QTreeWidgetItem * FolderItem;
    int Index = Page.Folders->IndexOf(NodeName);
    if (NodeName.IsEmpty())
    {
      FolderItem = nullptr;
    }
    else if (Index >= 0)
    {
      FolderItem = reinterpret_cast<QTreeWidgetItem *>(Page.Folders->Objects[Index]);
    }
    else
    {
      TAutoNestingCounter UpdatingCounter(FUpdating);
      FolderItem = InsertFolder(Page, NodeName, CreateItem(NodeName, NULL));
    }

    BookmarkMove(Page, Item, FolderItem);
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::RenameBookmark(TProfilesPage & Page)
{
  QTreeWidgetItem * Item = Page.View->currentItem();
  DebugAssert(Item != nullptr);
  if (Item != nullptr)
  {
    Page.View->setFocus();
    // The name, without the shortcut
    TBookmark * Bookmark = GetBookmark(Item);
    if (Bookmark != NULL)
    {
      SetItemText(Item, Bookmark->Name);
    }
    Page.View->editItem(Item);
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::ProfilesViewEdited(TProfilesPage & Page, QTreeWidgetItem * Item)
{
  UnicodeString S = ToUnicodeString(Item->text(0));
  TBookmark * Bookmark = GetBookmark(Item);
  try
  {
    if (Bookmark != NULL)
    {
      BookmarkNameValidateName(S);
      Bookmark->Name = S;
      SetItemText(Item, BookmarkText(Bookmark));
    }
    else
    {
      int OldIndex = Page.Folders->IndexOfObject(reinterpret_cast<TObject *>(Item));
      DebugAssert(OldIndex >= 0);
      UnicodeString OldName = Page.Folders->Strings[OldIndex];
      if (S != OldName)
      {
        BookmarkFolderValidateName(S, false);
        if ((Page.Folders->IndexOf(S) >= 0) && AnsiCompareText(S, OldName))
        {
          throw Exception(FMTLOAD(DUPLICATE_BOOKMARK_FOLDER, (S)));
        }
        TAutoNestingCounter UpdatingCounter(FUpdating);
        bool Expanded = Item->isExpanded();
        Page.Folders->Delete(OldIndex);
        Page.View->takeTopLevelItem(Page.View->indexOfTopLevelItem(Item));
        InsertFolder(Page, S, Item);
        Item->setExpanded(Expanded);
        for (int Index = 0; Index < Item->childCount(); Index++)
        {
          GetBookmark(Item->child(Index))->Node = S;
        }
        Page.View->setCurrentItem(Item);
        Page.View->scrollToItem(Item);
      }
    }
  }
  catch (Exception & E)
  {
    // Back to the previous name
    if (Bookmark != NULL)
    {
      SetItemText(Item, BookmarkText(Bookmark));
    }
    else
    {
      SetItemText(Item, Page.Folders->Strings[Page.Folders->IndexOfObject(reinterpret_cast<TObject *>(Item))]);
    }
    ShowExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::ProfilesViewChange(TProfilesPage & Page)
{
  TBookmark * Bookmark = GetBookmark(Page.View->currentItem());
  if (Bookmark != NULL)
  {
    {
      TAutoFlag ChangingFlag(FChanging);
      LocalDirectoryEdit->setEditText(ToQString(Bookmark->Local));
      RemoteDirectoryEdit->setEditText(ToQString(Bookmark->Remote));
    }
    // try to locate the same profile in the other set
    FindProfile();
    FBookmarkSelected = true;
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::DirectoryEditChange()
{
  if (!FChanging)
  {
    FindProfile();
    UpdateControls();
    FBookmarkSelected = false;
  }
}
//---------------------------------------------------------------------------
void TLocationProfilesDialog::ShortCutBookmarkButtonClick(TProfilesPage & Page)
{
  QTreeWidgetItem * Item = Page.View->currentItem();
  TBookmark * Bookmark = GetBookmark(Item);
  DebugAssert(Bookmark != NULL);

  TShortCuts ShortCuts;
  AppConfiguration->CustomCommandShortCuts(ShortCuts);
  Page.BookmarkList->ShortCuts(ShortCuts);
  TShortCut ShortCut = Bookmark->ShortCut;
  if (DoShortCutDialog(ShortCut, ShortCuts))
  {
    Bookmark->ShortCut = ShortCut;
    SetItemText(Item, BookmarkText(Bookmark));
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
bool TLocationProfilesDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if (Event->type() == QEvent::KeyPress)
  {
    for (TProfilesPage * Page : { &FSession, &FShared })
    {
      if ((Object == Page->View) && !Page->View->IsEditing() &&
          (Page->View->currentItem() != nullptr))
      {
        int Key = static_cast<QKeyEvent *>(Event)->key();
        if (Key == Qt::Key_Delete)
        {
          RemoveBookmark(*Page);
          return true;
        }
        else if (Key == Qt::Key_F2)
        {
          RenameBookmark(*Page);
          return true;
        }
      }
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool LocationProfilesDialog(TOpenDirectoryMode Mode, TOperationSide Side,
  UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TStrings * LocalDirectories, TStrings * RemoteDirectories, TTerminal * Terminal)
{
  TLocationProfilesDialog Dialog;
  Dialog.SetLocalDirectory(LocalDirectory);
  Dialog.SetRemoteDirectory(RemoteDirectory);
  Dialog.SetOperationSide(Side);
  Dialog.SetTerminal(Terminal);
  Dialog.SetRemoteDirectories(RemoteDirectories);
  Dialog.SetLocalDirectories(LocalDirectories);
  Dialog.SetMode(Mode);
  bool Result = Dialog.Execute();
  if (Result)
  {
    LocalDirectory = Dialog.GetLocalDirectory();
    RemoteDirectory = Dialog.GetRemoteDirectory();
  }
  return Result;
}
//---------------------------------------------------------------------------
