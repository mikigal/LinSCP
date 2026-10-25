// Opening a directory and managing the bookmarks (upstream's
// TOpenDirectoryDialog, forms/OpenDirectory.dfm): the directory, the
// bookmarks of the site and the shared ones (with their shortcuts).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Bookmarks.h>
#include <Terminal.h>

#include "CustomDialog.h"
#include "ShortCuts.h"

#include <QAbstractItemModel>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The controls of a page of bookmarks (upstream tells them by their tags)
struct TBookmarksPage
{
  QWidget * Sheet;
  QListWidget * List;
  QPushButton * AddButton;
  QPushButton * RemoveButton;
  QPushButton * UpButton;
  QPushButton * DownButton;
  QPushButton * ShortCutButton;
  TBookmarkList * BookmarkList;
};
//---------------------------------------------------------------------------
class TOpenDirectoryDialog : public QDialog
{
public:
  TOpenDirectoryDialog();
  virtual ~TOpenDirectoryDialog();

  bool Execute();

  UnicodeString GetDirectory();
  void SetDirectory(const UnicodeString & Value);
  void SetOperationSide(TOperationSide Value);
  void SetDirectories(TStrings * Value);
  void SetMode(TOpenDirectoryMode Value);
  void SetTerminal(TTerminal * Value) { FTerminal = Value; }
  void SetAllowSwitch(bool Value) { FAllowSwitch = Value; }

protected:
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  TOperationSide FOperationSide;
  TTerminal * FTerminal;
  TOpenDirectoryMode FMode;
  bool FAllowSwitch;
  bool FBookmarkSelected;
  int FUpdating;
  TBookmarksPage FSession;
  TBookmarksPage FShared;

  QLabel * EditLabel;
  QComboBox * LocalDirectoryEdit;
  QComboBox * RemoteDirectoryEdit;
  QPushButton * LocalDirectoryBrowseButton;
  QTabWidget * PageControl;
  QPushButton * SwitchButton;
  QPushButton * OKBtn;

  void CreatePage(TBookmarksPage & Page, const char * Prefix, const QString & Caption, bool ShortCut);
  QComboBox * GetCurrentEdit();
  TBookmarksPage & GetPage(QWidget * Sheet);
  void UpdateControls();
  void UpdateBookmarkControls(TBookmarksPage & Page);
  UnicodeString BookmarkDirectory(TBookmark * Bookmark);
  UnicodeString BookmarkText(TBookmark * Bookmark);
  TBookmark * GetBookmark(QListWidget * List, int Index);
  void LoadBookmarks(TBookmarksPage & Page, TBookmarkList * Source);
  int FindBookmark(QListWidget * List, const UnicodeString & Bookmark);
  void AddAsBookmark(TBookmarksPage & Page);
  void RemoveBookmark(TBookmarksPage & Page);
  void BookmarkSelected(TBookmarksPage & Page);
  void BookmarkMove(TBookmarksPage & Page, int Source, int Dest);
  void BookmarkDragged(TBookmarksPage & Page, int Source, int Dest);
  void SelectBookmark(QListWidget * List);
  void DirectoryEditChange();
  void ShortCutBookmarkButtonClick(TBookmarksPage & Page);
  void LocalDirectoryBrowseButtonClick();
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
TOpenDirectoryDialog::TOpenDirectoryDialog() :
  QDialog(QApplication::activeWindow())
{
  setObjectName(QStringLiteral("OpenDirectoryDialog"));
  FOperationSide = osCurrent;
  FTerminal = NULL;
  FMode = odBrowse;
  FAllowSwitch = false;
  FBookmarkSelected = false;
  FUpdating = 0;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  // The image left of the directory, as upstream's
  QHBoxLayout * TopLayout = new QHBoxLayout();
  TopLayout->addWidget(CreateDialogImage(this, L"Open folder"), 0, Qt::AlignTop);
  QVBoxLayout * DirectoryLayout = new QVBoxLayout();
  TopLayout->addLayout(DirectoryLayout, 1);
  MainLayout->addLayout(TopLayout);
  EditLabel = new QLabel(tr("&Open directory:"), this);
  EditLabel->setObjectName(QStringLiteral("EditLabel"));
  DirectoryLayout->addWidget(EditLabel);
  QHBoxLayout * EditLayout = new QHBoxLayout();
  for (QComboBox ** Edit : { &LocalDirectoryEdit, &RemoteDirectoryEdit })
  {
    *Edit = new QComboBox(this);
    (*Edit)->setEditable(true);
    (*Edit)->setInsertPolicy(QComboBox::NoInsert);
    (*Edit)->setCompleter(nullptr);
    (*Edit)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    (*Edit)->setVisible(false);
    connect(*Edit, &QComboBox::currentTextChanged, this, [this]() { DirectoryEditChange(); });
    EditLayout->addWidget(*Edit, 1);
  }
  LocalDirectoryEdit->setObjectName(QStringLiteral("LocalDirectoryEdit"));
  RemoteDirectoryEdit->setObjectName(QStringLiteral("RemoteDirectoryEdit"));
  LocalDirectoryEdit->setMinimumWidth(fontMetrics().averageCharWidth() * 55);
  RemoteDirectoryEdit->setMinimumWidth(LocalDirectoryEdit->minimumWidth());
  LocalDirectoryBrowseButton = CreateButton(this, QStringLiteral("LocalDirectoryBrowseButton"), tr("B&rowse..."));
  connect(LocalDirectoryBrowseButton, &QPushButton::clicked, this, [this]() { LocalDirectoryBrowseButtonClick(); });
  EditLayout->addWidget(LocalDirectoryBrowseButton);
  DirectoryLayout->addLayout(EditLayout);

  PageControl = new QTabWidget(this);
  PageControl->setObjectName(QStringLiteral("PageControl"));
  CreatePage(FSession, "Session", tr("Site bookmarks"), false);
  CreatePage(FShared, "Shared", tr("Shared bookmarks"), true);
  connect(PageControl, &QTabWidget::currentChanged, this, [this]()
  {
    if (FUpdating == 0)
    {
      BookmarkSelected(GetPage(PageControl->currentWidget()));
    }
  });
  MainLayout->addWidget(PageControl, 1);

  QHBoxLayout * ButtonsLayout = new QHBoxLayout();
  SwitchButton = CreateButton(this, QStringLiteral("SwitchButton"), tr("&Location Profiles..."));
  connect(SwitchButton, &QPushButton::clicked, this, [this]()
  {
    AppConfiguration->UseLocationProfiles = true;
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
  AddHelpButton(Buttons, FormHelpKeyword(L"OpenDirectory.dfm"));

  SetOperationSide(osLocal);
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::CreatePage(TBookmarksPage & Page, const char * Prefix, const QString & Caption, bool ShortCut)
{
  QString APrefix = QString::fromLatin1(Prefix);
  Page.BookmarkList = new TBookmarkList();
  Page.Sheet = new QWidget(PageControl);
  Page.Sheet->setObjectName(APrefix + QStringLiteral("BookmarksSheet"));
  QHBoxLayout * Layout = new QHBoxLayout(Page.Sheet);
  Page.List = new QListWidget(Page.Sheet);
  Page.List->setObjectName(APrefix + QStringLiteral("BookmarksList"));
  Page.List->setMinimumHeight(fontMetrics().height() * 10);
  Page.List->setDragDropMode(QAbstractItemView::InternalMove);
  Page.List->installEventFilter(this);
  Layout->addWidget(Page.List, 1);
  QVBoxLayout * ButtonLayout = new QVBoxLayout();
  Page.AddButton = CreateButton(Page.Sheet, QStringLiteral("Add") + APrefix + QStringLiteral("BookmarkButton"), tr("&Add"));
  Page.RemoveButton =
    CreateButton(Page.Sheet, QStringLiteral("Remove") + APrefix + QStringLiteral("BookmarkButton"), tr("Remo&ve"));
  Page.UpButton = CreateButton(Page.Sheet, QStringLiteral("Up") + APrefix + QStringLiteral("BookmarkButton"), tr("&Up"));
  Page.DownButton = CreateButton(Page.Sheet, QStringLiteral("Down") + APrefix + QStringLiteral("BookmarkButton"), tr("&Down"));
  Page.ShortCutButton = nullptr;
  for (QPushButton * Button : { Page.AddButton, Page.RemoveButton, Page.UpButton, Page.DownButton })
  {
    ButtonLayout->addWidget(Button);
  }
  if (ShortCut)
  {
    Page.ShortCutButton =
      CreateButton(Page.Sheet, QStringLiteral("ShortCut") + APrefix + QStringLiteral("BookmarkButton"), tr("&Shortcut..."));
    ButtonLayout->addWidget(Page.ShortCutButton);
  }
  ButtonLayout->addStretch(1);
  Layout->addLayout(ButtonLayout);
  PageControl->addTab(Page.Sheet, Caption);

  TBookmarksPage * P = &Page;
  connect(Page.AddButton, &QPushButton::clicked, this, [this, P]() { AddAsBookmark(*P); });
  connect(Page.RemoveButton, &QPushButton::clicked, this, [this, P]() { RemoveBookmark(*P); });
  connect(Page.UpButton, &QPushButton::clicked, this, [this, P]()
  {
    BookmarkMove(*P, P->List->currentRow(), P->List->currentRow() - 1);
  });
  connect(Page.DownButton, &QPushButton::clicked, this, [this, P]()
  {
    BookmarkMove(*P, P->List->currentRow(), P->List->currentRow() + 1);
  });
  if (Page.ShortCutButton != nullptr)
  {
    connect(Page.ShortCutButton, &QPushButton::clicked, this, [this, P]() { ShortCutBookmarkButtonClick(*P); });
  }
  connect(Page.List, &QListWidget::currentRowChanged, this, [this, P]()
  {
    if (FUpdating == 0)
    {
      BookmarkSelected(*P);
    }
  });
  connect(Page.List, &QListWidget::itemDoubleClicked, this, [this, P]()
  {
    if (P->List->currentRow() >= 0)
    {
      accept();
    }
  });
  connect(Page.List->model(), &QAbstractItemModel::rowsMoved, this,
    [this, P](const QModelIndex &, int Start, int, const QModelIndex &, int Row)
    {
      if (FUpdating == 0)
      {
        BookmarkDragged(*P, Start, (Row > Start) ? (Row - 1) : Row);
      }
    });
}
//---------------------------------------------------------------------------
TOpenDirectoryDialog::~TOpenDirectoryDialog()
{
  delete FSession.BookmarkList;
  delete FShared.BookmarkList;
}
//---------------------------------------------------------------------------
TBookmarksPage & TOpenDirectoryDialog::GetPage(QWidget * Sheet)
{
  return (Sheet == FShared.Sheet) ? FShared : FSession;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::SetOperationSide(TOperationSide Value)
{
  if (FOperationSide != Value)
  {
    UnicodeString ADirectory = GetDirectory();
    FOperationSide = Value;
    SetDirectory(ADirectory);
    RemoteDirectoryEdit->setVisible(false);
    LocalDirectoryEdit->setVisible(false);
    GetCurrentEdit()->setVisible(true);
    EditLabel->setBuddy(GetCurrentEdit());
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::SetDirectory(const UnicodeString & Value)
{
  GetCurrentEdit()->setEditText(ToQString(Value));
  DirectoryEditChange();
}
//---------------------------------------------------------------------------
UnicodeString TOpenDirectoryDialog::GetDirectory()
{
  UnicodeString Text = ToUnicodeString(GetCurrentEdit()->currentText());
  return (FOperationSide == osRemote) ? UnixExcludeTrailingBackslash(Text) : ExcludeTrailingBackslash(Text);
}
//---------------------------------------------------------------------------
QComboBox * TOpenDirectoryDialog::GetCurrentEdit()
{
  return (FOperationSide == osRemote) ? RemoteDirectoryEdit : LocalDirectoryEdit;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::UpdateBookmarkControls(TBookmarksPage & Page)
{
  UnicodeString Directory = GetDirectory();
  int ItemIndex = Page.List->currentRow();
  Page.AddButton->setEnabled(!Directory.IsEmpty() && (FindBookmark(Page.List, Directory) < 0));
  Page.RemoveButton->setEnabled(ItemIndex >= 0);
  if (Page.ShortCutButton != nullptr)
  {
    Page.ShortCutButton->setEnabled(ItemIndex >= 0);
  }
  Page.UpButton->setEnabled(ItemIndex > 0);
  Page.DownButton->setEnabled((ItemIndex >= 0) && (ItemIndex < Page.List->count() - 1));
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::UpdateControls()
{
  OKBtn->setEnabled(!GetDirectory().IsEmpty());
  LocalDirectoryBrowseButton->setVisible(FOperationSide == osLocal);
  SwitchButton->setVisible(FAllowSwitch);
  PageControl->setTabVisible(PageControl->indexOf(FSession.Sheet), (FTerminal != NULL));

  UpdateBookmarkControls(FSession);
  UpdateBookmarkControls(FShared);
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::SetDirectories(TStrings * Value)
{
  QComboBox * Edit = GetCurrentEdit();
  QString Text = Edit->currentText();
  Edit->clear();
  for (int Index = 0; Index < Value->Count; Index++)
  {
    Edit->addItem(ToQString(Value->Strings[Index]));
  }
  Edit->setEditText(Text);
}
//---------------------------------------------------------------------------
UnicodeString TOpenDirectoryDialog::BookmarkDirectory(TBookmark * Bookmark)
{
  return Bookmark->GetSideDirectory(FOperationSide);
}
//---------------------------------------------------------------------------
UnicodeString TOpenDirectoryDialog::BookmarkText(TBookmark * Bookmark)
{
  UnicodeString Result = BookmarkDirectory(Bookmark);
  if (!Result.IsEmpty() && (Bookmark->ShortCut != 0))
  {
    Result = FORMAT(L"%s (%s)", (Result, ShortCutToText(Bookmark->ShortCut)));
  }
  return Result;
}
//---------------------------------------------------------------------------
TBookmark * TOpenDirectoryDialog::GetBookmark(QListWidget * List, int Index)
{
  return static_cast<TBookmark *>(List->item(Index)->data(Qt::UserRole).value<void *>());
}
//---------------------------------------------------------------------------
static QListWidgetItem * CreateItem(const UnicodeString & Text, TBookmark * Bookmark)
{
  QListWidgetItem * Item = new QListWidgetItem(ToQString(Text));
  Item->setData(Qt::UserRole, QVariant::fromValue(static_cast<void *>(Bookmark)));
  return Item;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::LoadBookmarks(TBookmarksPage & Page, TBookmarkList * Source)
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

  Page.List->clear();
  for (int Index = 0; Index < Page.BookmarkList->Count; Index++)
  {
    TBookmark * Bookmark = Page.BookmarkList->Bookmarks[Index];
    UnicodeString Directory = BookmarkDirectory(Bookmark);
    if (!Directory.IsEmpty() && (FindBookmark(Page.List, Directory) < 0))
    {
      Page.List->addItem(CreateItem(BookmarkText(Bookmark), Bookmark));
    }
  }
}
//---------------------------------------------------------------------------
bool TOpenDirectoryDialog::Execute()
{
  UnicodeString SessionKey;
  if (FTerminal != NULL)
  {
    // cache session key, in case terminal is closed while the window is open
    SessionKey = FTerminal->SessionData->SessionKey;
    LoadBookmarks(FSession, AppConfiguration->Bookmarks[SessionKey]);
  }

  LoadBookmarks(FShared, AppConfiguration->SharedBookmarks);
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    QWidget * Page =
      ((FTerminal == NULL) || AppConfiguration->UseSharedBookmarks) ? FShared.Sheet : FSession.Sheet;
    PageControl->setCurrentWidget(Page);
  }
  DirectoryEditChange();
  if (FMode == odAddBookmark)
  {
    AddAsBookmark(GetPage(PageControl->currentWidget()));
  }

  UpdateControls();
  if (FMode == odBrowse)
  {
    GetCurrentEdit()->setFocus();
  }
  else
  {
    GetPage(PageControl->currentWidget()).List->setFocus();
  }
  FBookmarkSelected = false;
  bool Result = (exec() == QDialog::Accepted);
  AppConfiguration->SharedBookmarks = FShared.BookmarkList;
  if (FTerminal != NULL)
  {
    AppConfiguration->Bookmarks[SessionKey] = FSession.BookmarkList;
    // Do not remember that shared page was selected,
    // if it was selected implicitly because there's no site page.
    AppConfiguration->UseSharedBookmarks = (PageControl->currentWidget() == FShared.Sheet);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::AddAsBookmark(TBookmarksPage & Page)
{
  UnicodeString Directory = GetDirectory();
  if (Directory.IsEmpty() || (FindBookmark(Page.List, Directory) >= 0))
  {
    return;
  }

  TBookmark * Bookmark = new TBookmark();
  if (FOperationSide == osRemote)
  {
    Bookmark->Remote = Directory;
  }
  else
  {
    Bookmark->Local = Directory;
  }
  GetCurrentEdit()->lineEdit()->selectAll();
  Bookmark->Name = Directory;

  // would always be equal to Directory atm,
  // as only difference can be a shorcut, which is not set
  UnicodeString Text = BookmarkText(Bookmark);

  TAutoNestingCounter UpdatingCounter(FUpdating);
  int ItemIndex = Page.List->currentRow();
  if (ItemIndex >= 0)
  {
    Page.BookmarkList->InsertBefore(GetBookmark(Page.List, ItemIndex), Bookmark);
    Page.List->insertItem(ItemIndex, CreateItem(Text, Bookmark));
    Page.List->setCurrentRow(ItemIndex);
  }
  else
  {
    Page.BookmarkList->Add(Bookmark);
    Page.List->addItem(CreateItem(Text, Bookmark));
    Page.List->setCurrentRow(Page.List->count() - 1);
  }

  UpdateControls();
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::RemoveBookmark(TBookmarksPage & Page)
{
  int PrevItemIndex = Page.List->currentRow();
  TBookmark * Bookmark = GetBookmark(Page.List, PrevItemIndex);
  DebugAssert(Bookmark != NULL);
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    Page.BookmarkList->Delete(Bookmark);
    delete Page.List->takeItem(PrevItemIndex);
    Page.List->setCurrentRow(std::min(PrevItemIndex, Page.List->count() - 1));
  }
  UpdateControls();
  BookmarkSelected(Page);
}
//---------------------------------------------------------------------------
int TOpenDirectoryDialog::FindBookmark(QListWidget * List, const UnicodeString & Bookmark)
{
  for (int Index = 0; Index < List->count(); Index++)
  {
    UnicodeString Directory = BookmarkDirectory(GetBookmark(List, Index));
    bool Same =
      (FOperationSide == osRemote) ? (AnsiCompareStr(Directory, Bookmark) == 0) : (AnsiCompareText(Directory, Bookmark) == 0);
    if (Same)
    {
      return Index;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::BookmarkSelected(TBookmarksPage & Page)
{
  if (Page.List->currentRow() >= 0)
  {
    SetDirectory(BookmarkDirectory(GetBookmark(Page.List, Page.List->currentRow())));
  }
  UpdateControls();
  FBookmarkSelected = true;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::BookmarkMove(TBookmarksPage & Page, int Source, int Dest)
{
  QListWidget * List = Page.List;
  if ((Source >= 0) && (Source < List->count()) &&
      (Dest >= 0) && (Dest < List->count()))
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    Page.BookmarkList->MoveTo(GetBookmark(List, Dest), GetBookmark(List, Source), (Source > Dest));
    List->insertItem(Dest, List->takeItem(Source));
    List->setCurrentRow(Dest);
    List->setFocus();
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::BookmarkDragged(TBookmarksPage & Page, int Source, int Dest)
{
  // The list moved the item already; the bookmark it was dropped at is next
  // to it now
  QListWidget * List = Page.List;
  if (Source != Dest)
  {
    int Neighbour = (Source > Dest) ? (Dest + 1) : (Dest - 1);
    Page.BookmarkList->MoveTo(GetBookmark(List, Neighbour), GetBookmark(List, Dest), (Source > Dest));
  }
  UpdateControls();
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::SelectBookmark(QListWidget * List)
{
  int ItemIndex = FindBookmark(List, GetDirectory());
  if (ItemIndex >= 0)
  {
    TAutoNestingCounter UpdatingCounter(FUpdating);
    List->setCurrentRow(ItemIndex);
  }
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::DirectoryEditChange()
{
  SelectBookmark(FSession.List);
  SelectBookmark(FShared.List);
  UpdateControls();
  FBookmarkSelected = false;
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::SetMode(TOpenDirectoryMode Value)
{
  FMode = Value;
  setWindowTitle(ToQString(LoadStr((FMode == odBrowse) ? OPEN_DIRECTORY_BROWSE_CAPTION : OPEN_DIRECTORY_ADD_BOOMARK_ACTION)));
}
//---------------------------------------------------------------------------
bool TOpenDirectoryDialog::eventFilter(QObject * Object, QEvent * Event)
{
  if (Event->type() == QEvent::KeyPress)
  {
    for (TBookmarksPage * Page : { &FSession, &FShared })
    {
      if ((Object == Page->List) && (static_cast<QKeyEvent *>(Event)->key() == Qt::Key_Delete) &&
          (Page->List->currentRow() >= 0))
      {
        RemoveBookmark(*Page);
        return true;
      }
    }
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::LocalDirectoryBrowseButtonClick()
{
  QString Directory =
    QFileDialog::getExistingDirectory(this, ToQString(LoadStr(SELECT_LOCAL_DIRECTORY)), LocalDirectoryEdit->currentText());
  if (!Directory.isEmpty())
  {
    LocalDirectoryEdit->setEditText(Directory);
  }
}
//---------------------------------------------------------------------------
void TOpenDirectoryDialog::ShortCutBookmarkButtonClick(TBookmarksPage & Page)
{
  int Index = Page.List->currentRow();
  TBookmark * Bookmark = GetBookmark(Page.List, Index);
  DebugAssert(Bookmark != NULL);

  TShortCuts ShortCuts;
  AppConfiguration->CustomCommandShortCuts(ShortCuts);
  Page.BookmarkList->ShortCuts(ShortCuts);
  TShortCut ShortCut = Bookmark->ShortCut;
  if (DoShortCutDialog(ShortCut, ShortCuts))
  {
    Bookmark->ShortCut = ShortCut;
    Page.List->item(Index)->setText(ToQString(BookmarkText(Bookmark)));
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
bool DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side, UnicodeString & Directory,
  TStrings * Directories, TTerminal * Terminal, bool AllowSwitch)
{
  TOpenDirectoryDialog Dialog;
  Dialog.SetMode(Mode);
  Dialog.SetOperationSide(Side);
  Dialog.SetDirectory(Directory);
  Dialog.SetDirectories(Directories);
  Dialog.SetTerminal(Terminal);
  Dialog.SetAllowSwitch(AllowSwitch);
  bool Result = Dialog.Execute();
  if (Result)
  {
    Directory = Dialog.GetDirectory();
  }
  return Result;
}
//---------------------------------------------------------------------------
