// The file panels of the Commander.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <RemoteFiles.h>
#include <Terminal.h>

#include "DirModel.h"
#include "DirPanel.h"

#include <QAction>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QToolBar>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
TDirPanel::TDirPanel(TOperationSide Side, QWidget * Parent) :
  QWidget(Parent),
  FModel(nullptr),
  FShowHiddenFiles(false),
  FSide(Side),
  FHistoryIndex(-1),
  FNavigatingHistory(false),
  FUpdatingPath(false)
{
  QString Prefix = (Side == osLocal) ? QStringLiteral("Local") : QStringLiteral("Remote");
  setObjectName(Prefix + QStringLiteral("Panel"));

  QVBoxLayout * Layout = new QVBoxLayout(this);
  Layout->setContentsMargins(0, 0, 0, 0);
  Layout->setSpacing(2);

  PathToolbar = new QToolBar(this);
  PathToolbar->setObjectName(Prefix + QStringLiteral("PathToolbar"));
  PathToolbar->setIconSize(QSize(16, 16));
  Layout->addWidget(PathToolbar);

  PathComboBox = new QComboBox(PathToolbar);
  PathComboBox->setObjectName(Prefix + QStringLiteral("PathComboBox"));
  PathComboBox->setEditable(true);
  PathComboBox->setInsertPolicy(QComboBox::NoInsert);
  PathComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  PathComboBox->installEventFilter(this);
  PathToolbar->addWidget(PathComboBox);
  connect(PathComboBox->lineEdit(), &QLineEdit::returnPressed, this, [this]() { PathComboActivated(); });
  connect(PathComboBox, &QComboBox::activated, this, [this]() { PathComboActivated(); });

  BackAction = CreateAction(Prefix + QStringLiteral("BackAction"), tr("&Back"), QStringLiteral("go-previous"),
    QKeySequence(Qt::ALT | Qt::Key_Left), &TDirPanel::Back);
  ForwardAction = CreateAction(Prefix + QStringLiteral("ForwardAction"), tr("&Forward"), QStringLiteral("go-next"),
    QKeySequence(Qt::ALT | Qt::Key_Right), &TDirPanel::Forward);
  ParentDirAction = CreateAction(Prefix + QStringLiteral("ParentDirAction"), tr("&Parent Directory"), QStringLiteral("go-up"),
    QKeySequence(Qt::Key_Backspace), &TDirPanel::ParentDirectory);
  RootDirAction = CreateAction(Prefix + QStringLiteral("RootDirAction"), tr("&Root Directory"), QStringLiteral("go-top"),
    QKeySequence(Qt::CTRL | Qt::Key_Backslash), &TDirPanel::RootDirectory);
  HomeDirAction = CreateAction(Prefix + QStringLiteral("HomeDirAction"), tr("&Home Directory"), QStringLiteral("go-home"),
    QKeySequence(Qt::CTRL | Qt::Key_H), &TDirPanel::HomeDirActionExecute);
  RefreshAction = CreateAction(Prefix + QStringLiteral("RefreshAction"), tr("&Refresh"), QStringLiteral("view-refresh"),
    QKeySequence(Qt::CTRL | Qt::Key_R), &TDirPanel::RefreshActionExecute);

  DirView = new QTreeView(this);
  DirView->setObjectName(Prefix + QStringLiteral("DirView"));
  DirView->setRootIsDecorated(false);
  DirView->setItemsExpandable(false);
  DirView->setUniformRowHeights(true);
  DirView->setAllColumnsShowFocus(true);
  DirView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  DirView->setSelectionBehavior(QAbstractItemView::SelectRows);
  DirView->setEditTriggers(QAbstractItemView::NoEditTriggers);
  DirView->setSortingEnabled(true);
  DirView->installEventFilter(this);
  connect(DirView, &QTreeView::activated, this, [this](const QModelIndex & Index) { ItemActivated(Index.row()); });
  Layout->addWidget(DirView, 1);

  StatusBar = new QLabel(this);
  StatusBar->setObjectName(Prefix + QStringLiteral("StatusBar"));
  StatusBar->setFrameShape(QFrame::StyledPanel);
  StatusBar->setFrameShadow(QFrame::Sunken);
  Layout->addWidget(StatusBar);
}
//---------------------------------------------------------------------------
QAction * TDirPanel::CreateAction(const QString & Name, const QString & Caption, const QString & IconName,
  const QKeySequence & ShortCut, void (TDirPanel::*Method)())
{
  QAction * Action = new QAction(QIcon::fromTheme(IconName), Caption, this);
  Action->setObjectName(Name);
  Action->setShortcut(ShortCut);
  // Of the panel with the focus
  Action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  addAction(Action);
  PathToolbar->addAction(Action);
  connect(Action, &QAction::triggered, this, [this, Method]() { (this->*Method)(); });
  return Action;
}
//---------------------------------------------------------------------------
void TDirPanel::SetModel(TDirModel * Model)
{
  FModel = Model;
  DirView->setModel(Model);
  DirView->sortByColumn(0, Qt::AscendingOrder);
  QHeaderView * Header = DirView->header();
  Header->setStretchLastSection(false);
  Header->setSectionResizeMode(0, QHeaderView::Stretch);
  for (int Column = 1; Column < Model->columnCount(); Column++)
  {
    Header->setSectionResizeMode(Column, QHeaderView::Interactive);
    Header->resizeSection(Column, (Model->Column(Column) == dvChanged) ? 140 : 80);
  }
  connect(DirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() { UpdateStatusBar(); });
}
//---------------------------------------------------------------------------
bool TDirPanel::eventFilter(QObject * Object, QEvent * Event)
{
  if ((Event->type() == QEvent::FocusIn) && ((Object == DirView) || (Object == PathComboBox)))
  {
    Q_EMIT Entered(this);
  }
  return QWidget::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TDirPanel::PathComboActivated()
{
  if (!FUpdatingPath)
  {
    UnicodeString Path = ToUnicodeString(PathComboBox->currentText()).Trim();
    if (!Path.IsEmpty() && (Path != GetPath()))
    {
      ChangeDirectory(Path);
    }
    DirView->setFocus();
  }
}
//---------------------------------------------------------------------------
void TDirPanel::ItemActivated(int Row)
{
  if ((Row >= 0) && (Row < FModel->Count()))
  {
    const TDirViewItem & Item = FModel->Item(Row);
    if (Item.IsParentDirectory)
    {
      ParentDirectory();
    }
    else if (Item.IsDirectory)
    {
      ChangeDirectory(Item.FileName);
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::ParentDirectory()
{
  if (IsReady() && (GetPath() != L"/"))
  {
    ChangeDirectory(PARENTDIRECTORY);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::RootDirectory()
{
  if (IsReady())
  {
    ChangeDirectory(L"/");
  }
}
//---------------------------------------------------------------------------
void TDirPanel::Back()
{
  if (IsReady() && (FHistoryIndex > 0))
  {
    TAutoFlag NavigatingFlag(FNavigatingHistory);
    ChangeDirectory(FHistory[FHistoryIndex - 1]);
    FHistoryIndex--;
    UpdateActions();
  }
}
//---------------------------------------------------------------------------
void TDirPanel::Forward()
{
  if (IsReady() && (FHistoryIndex + 1 < static_cast<int>(FHistory.size())))
  {
    TAutoFlag NavigatingFlag(FNavigatingHistory);
    ChangeDirectory(FHistory[FHistoryIndex + 1]);
    FHistoryIndex++;
    UpdateActions();
  }
}
//---------------------------------------------------------------------------
void TDirPanel::PathChanged()
{
  UnicodeString Path = GetPath();
  if (!FNavigatingHistory &&
      ((FHistoryIndex < 0) || (FHistory[FHistoryIndex] != Path)))
  {
    FHistory.resize(FHistoryIndex + 1);
    FHistory.push_back(Path);
    FHistoryIndex = static_cast<int>(FHistory.size()) - 1;
  }

  // Going up focuses the directory we come from
  FFocusAfterReload = L"";
  if (!FPrevPath.IsEmpty() && (FPrevPath != Path) && (UnixExtractFileDir(FPrevPath) == Path))
  {
    FFocusAfterReload = UnixExtractFileName(FPrevPath);
  }
  FPrevPath = Path;

  {
    TAutoFlag UpdatingFlag(FUpdatingPath);
    PathComboBox->clear();
    UnicodeString APath = Path;
    while (true)
    {
      PathComboBox->addItem(ToQString(APath));
      if (APath == L"/")
      {
        break;
      }
      UnicodeString Parent = UnixExtractFileDir(APath);
      if (Parent.IsEmpty() || (Parent == APath))
      {
        break;
      }
      APath = Parent;
    }
    PathComboBox->setCurrentIndex(0);
  }

  // The focus of the new directory is set when its files are loaded
  DirView->selectionModel()->clear();
  UpdateActions();
}
//---------------------------------------------------------------------------
void TDirPanel::SaveFocus()
{
  // Keeps the focused file over a reload of the same directory
  if (FFocusAfterReload.IsEmpty() && (FModel != nullptr))
  {
    QModelIndex Current = DirView->currentIndex();
    if (Current.isValid() && (Current.row() < FModel->Count()))
    {
      FFocusAfterReload = FModel->Item(Current.row()).FileName;
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::FilesReloaded()
{
  int Row = FFocusAfterReload.IsEmpty() ? -1 : FModel->FindFile(FFocusAfterReload);
  FFocusAfterReload = L"";
  if ((Row < 0) && (FModel->Count() > 0))
  {
    Row = 0;
  }
  if (Row >= 0)
  {
    QModelIndex Index = FModel->index(Row, 0);
    DirView->selectionModel()->setCurrentIndex(Index, QItemSelectionModel::NoUpdate);
    DirView->scrollTo(Index);
  }
  UpdateStatusBar();
}
//---------------------------------------------------------------------------
void TDirPanel::FocusFile(const UnicodeString & FileName)
{
  int Row = FModel->FindFile(FileName);
  if (Row >= 0)
  {
    QModelIndex Index = FModel->index(Row, 0);
    DirView->selectionModel()->setCurrentIndex(Index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    DirView->scrollTo(Index);
  }
}
//---------------------------------------------------------------------------
std::vector<int> TDirPanel::GetSelectedRows() const
{
  std::vector<int> Result;
  if (IsReady() && (FModel != nullptr))
  {
    for (const QModelIndex & Index : DirView->selectionModel()->selectedRows())
    {
      if (!FModel->Item(Index.row()).IsParentDirectory)
      {
        Result.push_back(Index.row());
      }
    }
    if (Result.empty())
    {
      QModelIndex Current = DirView->currentIndex();
      if (Current.isValid() && !FModel->Item(Current.row()).IsParentDirectory)
      {
        Result.push_back(Current.row());
      }
    }
    std::sort(Result.begin(), Result.end());
  }
  return Result;
}
//---------------------------------------------------------------------------
void TDirPanel::SetShowHiddenFiles(bool Value)
{
  if (FShowHiddenFiles != Value)
  {
    FShowHiddenFiles = Value;
    if (IsReady())
    {
      Reload();
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::UpdateStatusBar()
{
  UnicodeString Text;
  if (IsReady() && (FModel != nullptr))
  {
    __int64 FilesSize;
    int FilesCount;
    FModel->Totals(FilesSize, FilesCount);
    __int64 SelectedSize = 0;
    int SelectedCount = 0;
    for (const QModelIndex & Index : DirView->selectionModel()->selectedRows())
    {
      const TDirViewItem & Item = FModel->Item(Index.row());
      if (!Item.IsParentDirectory)
      {
        SelectedSize += Item.Size;
        SelectedCount++;
      }
    }
    Text =
      FMTLOAD(FILE_INFO_FORMAT,
        (FormatBytes(SelectedSize), FormatBytes(FilesSize), FormatNumber(SelectedCount), FormatNumber(FilesCount)));
  }
  StatusBar->setText(ToQString(Text));
}
//---------------------------------------------------------------------------
void TDirPanel::UpdateActions()
{
  bool Ready = IsReady();
  BackAction->setEnabled(Ready && (FHistoryIndex > 0));
  ForwardAction->setEnabled(Ready && (FHistoryIndex + 1 < static_cast<int>(FHistory.size())));
  ParentDirAction->setEnabled(Ready && (GetPath() != L"/"));
  RootDirAction->setEnabled(Ready);
  HomeDirAction->setEnabled(Ready);
  RefreshAction->setEnabled(Ready);
  PathComboBox->setEnabled(Ready);
}
//---------------------------------------------------------------------------
TLocalDirPanel::TLocalDirPanel(QWidget * Parent) :
  TDirPanel(osLocal, Parent)
{
  FLocalModel = new TLocalDirModel(this);
  SetModel(FLocalModel);

  // Changes of the directory reload it (once they settle)
  FReloadTimer = new QTimer(this);
  FReloadTimer->setSingleShot(true);
  FReloadTimer->setInterval(250);
  connect(FReloadTimer, &QTimer::timeout, this, [this]() { Reload(); });
  FWatcher = new QFileSystemWatcher(this);
  connect(FWatcher, &QFileSystemWatcher::directoryChanged, FReloadTimer, qOverload<>(&QTimer::start));
}
//---------------------------------------------------------------------------
UnicodeString TLocalDirPanel::GetPath() const
{
  return FLocalModel->GetPath();
}
//---------------------------------------------------------------------------
void TLocalDirPanel::Load(const UnicodeString & Path)
{
  FLocalModel->Load(Path, FShowHiddenFiles);
  if (!FWatcher->directories().isEmpty())
  {
    FWatcher->removePaths(FWatcher->directories());
  }
  FWatcher->addPath(ToQString(Path));
}
//---------------------------------------------------------------------------
void TLocalDirPanel::ChangeDirectory(const UnicodeString & Path)
{
  UnicodeString CurrentPath = GetPath();
  UnicodeString NewPath;
  if (Path == PARENTDIRECTORY)
  {
    NewPath = UnixExtractFileDir(CurrentPath);
  }
  else if (StartsStr(L"/", Path))
  {
    NewPath = Path;
  }
  else if (StartsStr(L"~", Path))
  {
    NewPath = ToUnicodeString(QDir::homePath()) + Path.SubString(2, Path.Length() - 1);
  }
  else
  {
    NewPath = UnixIncludeTrailingBackslash(CurrentPath) + Path;
  }
  // Typed paths may have "." and ".."; the names of the files keep their
  // bytes
  if ((NewPath.Pos(L"/./") > 0) || (NewPath.Pos(L"/../") > 0) || EndsStr(L"/..", NewPath) || EndsStr(L"/.", NewPath))
  {
    NewPath = ToUnicodeString(QDir::cleanPath(ToQString(NewPath)));
  }
  if (NewPath.IsEmpty())
  {
    NewPath = L"/";
  }
  else if ((NewPath.Length() > 1) && EndsStr(L"/", NewPath))
  {
    NewPath = UnixExcludeTrailingBackslash(NewPath);
  }

  Load(NewPath);
  PathChanged();
  FilesReloaded();
}
//---------------------------------------------------------------------------
void TLocalDirPanel::Reload()
{
  SaveFocus();
  Load(GetPath());
  FilesReloaded();
}
//---------------------------------------------------------------------------
void TLocalDirPanel::HomeDirectory()
{
  ChangeDirectory(ToUnicodeString(QDir::homePath()));
}
//---------------------------------------------------------------------------
void TLocalDirPanel::ItemActivated(int Row)
{
  if ((Row >= 0) && (Row < FModel->Count()) && !FModel->Item(Row).IsDirectory)
  {
    // As upstream's local panel: the file opens in its application
    UnicodeString FileName = UnixIncludeTrailingBackslash(GetPath()) + FModel->Item(Row).FileName;
    QDesktopServices::openUrl(QUrl::fromLocalFile(ToQString(FileName)));
  }
  else
  {
    TDirPanel::ItemActivated(Row);
  }
}
//---------------------------------------------------------------------------
TRemoteDirPanel::TRemoteDirPanel(QWidget * Parent) :
  TDirPanel(osRemote, Parent),
  FTerminal(NULL)
{
  FRemoteModel = new TRemoteDirModel(this);
  SetModel(FRemoteModel);

  // The view or the "not connected" page in its place
  QVBoxLayout * Layout = static_cast<QVBoxLayout *>(layout());
  int Index = Layout->indexOf(DirView);
  FStack = new QStackedWidget(this);
  FStack->setObjectName(QStringLiteral("RemoteStack"));
  Layout->insertWidget(Index, FStack, 1);
  FStack->addWidget(DirView);
  QWidget * NoSessionPage = new QWidget(FStack);
  NoSessionPage->setObjectName(QStringLiteral("RemoteNoSessionPage"));
  QVBoxLayout * PageLayout = new QVBoxLayout(NoSessionPage);
  PageLayout->addStretch(1);
  QLabel * Label = new QLabel(tr("Not connected."), NoSessionPage);
  Label->setObjectName(QStringLiteral("RemoteNoSessionLabel"));
  Label->setAlignment(Qt::AlignCenter);
  PageLayout->addWidget(Label);
  QPushButton * NewSessionButton = new QPushButton(tr("&New Session..."), NoSessionPage);
  NewSessionButton->setObjectName(QStringLiteral("RemoteNewSessionButton"));
  PageLayout->addWidget(NewSessionButton, 0, Qt::AlignCenter);
  PageLayout->addStretch(1);
  FStack->addWidget(NoSessionPage);
  connect(NewSessionButton, &QPushButton::clicked, this, [this]() { Q_EMIT NewSessionRequested(); });

  SetTerminal(NULL);
}
//---------------------------------------------------------------------------
bool TRemoteDirPanel::IsReady() const
{
  return (FTerminal != NULL) && FTerminal->Active;
}
//---------------------------------------------------------------------------
UnicodeString TRemoteDirPanel::GetPath() const
{
  return IsReady() ? FTerminal->CurrentDirectory : UnicodeString();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::SetTerminal(TTerminal * Terminal)
{
  if ((FTerminal != NULL) && (FTerminal != Terminal))
  {
    FTerminal->OnChangeDirectory = NULL;
    FTerminal->OnReadDirectory = NULL;
  }
  FTerminal = Terminal;
  if (FTerminal != NULL)
  {
    FTerminal->OnChangeDirectory = ::rtl::MethodPtr(this, &TRemoteDirPanel::TerminalChangeDirectory);
    FTerminal->OnReadDirectory = ::rtl::MethodPtr(this, &TRemoteDirPanel::TerminalReadDirectory);
  }

  if (IsReady())
  {
    if (GetPath() != ToUnicodeString(PathComboBox->currentText()))
    {
      PathChanged();
    }
    LoadFiles();
    FStack->setCurrentIndex(0);
  }
  else
  {
    FRemoteModel->Clear();
    PathComboBox->clear();
    FStack->setCurrentIndex(1);
  }
  UpdateActions();
  UpdateStatusBar();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::TerminalChangeDirectory(TObject * /*Sender*/)
{
  PathChanged();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::TerminalReadDirectory(TObject * /*Sender*/, Boolean /*ReloadOnly*/)
{
  LoadFiles();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::LoadFiles()
{
  SaveFocus();
  FRemoteModel->Load(FTerminal->Files, FShowHiddenFiles);
  FilesReloaded();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::ChangeDirectory(const UnicodeString & Path)
{
  if (IsReady())
  {
    FTerminal->ChangeDirectory(Path);
  }
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::Reload()
{
  if (IsReady())
  {
    FTerminal->ReloadDirectory();
  }
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::HomeDirectory()
{
  if (IsReady())
  {
    FTerminal->HomeDirectory();
  }
}
//---------------------------------------------------------------------------
TRemoteFile * TRemoteDirPanel::GetFile(int Row) const
{
  return FRemoteModel->Item(Row).File;
}
//---------------------------------------------------------------------------
