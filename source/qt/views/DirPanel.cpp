// The file panels of the Commander.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <RemoteFiles.h>
#include <Terminal.h>

#include "DirModel.h"
#include "DirPanel.h"
#include "DirTree.h"

#include <QAbstractItemDelegate>
#include <QApplication>
#include <QAction>
#include <QActionGroup>
#include <QComboBox>
#include <QContextMenuEvent>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPointer>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
// The editor of the names of the files, named for the end-to-end tests
class TRenameDelegate : public QStyledItemDelegate
{
public:
  explicit TRenameDelegate(QObject * Parent) : QStyledItemDelegate(Parent) {}

  virtual QWidget * createEditor(QWidget * Parent, const QStyleOptionViewItem & Option, const QModelIndex & Index) const override
  {
    QWidget * Editor = QStyledItemDelegate::createEditor(Parent, Option, Index);
    Editor->setObjectName(QStringLiteral("RenameEdit"));
    return Editor;
  }
};
//---------------------------------------------------------------------------
TDirView::TDirView(TDirPanel * Panel) :
  QTreeView(Panel),
  FPanel(Panel),
  FDropTargetRow(-1),
  FNortonLike(nlKeyboard),
  FAnyAndAllSelectedImplicitly(false),
  FSelectingImplicitly(false),
  FNoSelectionUpdate(false)
{
  setDragEnabled(true);
  setAcceptDrops(true);
  setDragDropMode(QAbstractItemView::DragDrop);
  setDropIndicatorShown(false);
}
//---------------------------------------------------------------------------
void TDirView::setModel(QAbstractItemModel * Model)
{
  QTreeView::setModel(Model);
  connect(selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]()
  {
    if (!FSelectingImplicitly)
    {
      FAnyAndAllSelectedImplicitly = false;
    }
  });
}
//---------------------------------------------------------------------------
QItemSelectionModel::SelectionFlags TDirView::FocusCommand() const
{
  QItemSelectionModel::SelectionFlags Result;
  if (FNortonLike == nlOff)
  {
    Result = QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows;
  }
  else if ((FNortonLike == nlKeyboard) && FAnyAndAllSelectedImplicitly)
  {
    // The file selected by a click is unselected
    Result = QItemSelectionModel::Clear;
  }
  else
  {
    Result = QItemSelectionModel::NoUpdate;
  }
  return Result;
}
//---------------------------------------------------------------------------
QItemSelectionModel::SelectionFlags TDirView::selectionCommand(const QModelIndex & Index, const QEvent * Event) const
{
  if ((FNortonLike != nlOff) && (Event != nullptr))
  {
    switch (Event->type())
    {
      case QEvent::KeyPress:
        {
          const QKeyEvent * KeyEvent = static_cast<const QKeyEvent *>(Event);
          switch (KeyEvent->key())
          {
            case Qt::Key_Down:
            case Qt::Key_Up:
            case Qt::Key_Left:
            case Qt::Key_Right:
            case Qt::Key_Home:
            case Qt::Key_End:
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
              if (FNoSelectionUpdate || KeyEvent->modifiers().testFlag(Qt::ControlModifier))
              {
                return QItemSelectionModel::NoUpdate;
              }
              return FocusCommand();

            default:
              break;
          }
        }
        break;

      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
      case QEvent::MouseMove:
        // Only the clicks with Ctrl or Shift select (nlOn), the right button
        // never does
        if (FNortonLike == nlOn)
        {
          const QMouseEvent * MouseEvent = static_cast<const QMouseEvent *>(Event);
          bool RightButton =
            (MouseEvent->button() == Qt::RightButton) || MouseEvent->buttons().testFlag(Qt::RightButton);
          if (RightButton || !(MouseEvent->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier)))
          {
            return QItemSelectionModel::NoUpdate;
          }
        }
        break;

      default:
        break;
    }
  }
  return QTreeView::selectionCommand(Index, Event);
}
//---------------------------------------------------------------------------
bool TDirView::IsSearching() const
{
  return
    !FSearchText.isEmpty() && FSearchTimer.isValid() &&
    (FSearchTimer.elapsed() < QApplication::keyboardInputInterval());
}
//---------------------------------------------------------------------------
void TDirView::SelectCurrentItem(bool FocusNext)
{
  const TDirModel * Model = FPanel->GetModel();
  if ((Model != nullptr) && (Model->Count() > 0))
  {
    QModelIndex Current = currentIndex();
    if (!Current.isValid())
    {
      Current = Model->index(0, 0);
    }
    // The parent directory is never selected
    if (!Model->Item(Current.row()).IsParentDirectory)
    {
      selectionModel()->select(Current, QItemSelectionModel::Toggle | QItemSelectionModel::Rows);
    }
    if (FocusNext && (Current.row() + 1 < Model->Count()))
    {
      QModelIndex Next = Model->index(Current.row() + 1, 0);
      selectionModel()->setCurrentIndex(Next, QItemSelectionModel::NoUpdate);
      scrollTo(Next);
    }
  }
}
//---------------------------------------------------------------------------
void TDirView::keyPressEvent(QKeyEvent * Event)
{
  Qt::KeyboardModifiers Modifiers = Event->modifiers() & ~Qt::KeypadModifier;
  int Key = Event->key();
  if ((FNortonLike != nlOff) && (state() != EditingState))
  {
    if ((Key == Qt::Key_Insert) && (Modifiers == Qt::NoModifier))
    {
      SelectCurrentItem(true);
      Event->accept();
      return;
    }
    // Space continues the incremental search, else it selects
    if ((Key == Qt::Key_Space) && (Modifiers == Qt::NoModifier) && !IsSearching())
    {
      SelectCurrentItem(false);
      Event->accept();
      return;
    }
    // Without a horizontal scroll bar, left and right go to the first and
    // the last file
    if (((Key == Qt::Key_Left) || (Key == Qt::Key_Right)) && !horizontalScrollBar()->isVisible())
    {
      QKeyEvent HomeEnd(
        Event->type(), (Key == Qt::Key_Left) ? Qt::Key_Home : Qt::Key_End, Event->modifiers(), Event->text(),
        Event->isAutoRepeat(), Event->count());
      keyPressEvent(&HomeEnd);
      Event->setAccepted(HomeEnd.isAccepted());
      return;
    }
    // Shift selects the files the focus moves over, never unselects
    bool Movement =
      (Key == Qt::Key_Down) || (Key == Qt::Key_Up) || (Key == Qt::Key_Home) || (Key == Qt::Key_End) ||
      (Key == Qt::Key_PageUp) || (Key == Qt::Key_PageDown);
    if (Movement && (Modifiers == Qt::ShiftModifier))
    {
      QModelIndex OldCurrent = currentIndex();
      {
        TAutoFlag NoSelectionUpdateFlag(FNoSelectionUpdate);
        QTreeView::keyPressEvent(Event);
      }
      QModelIndex NewCurrent = currentIndex();
      const TDirModel * Model = FPanel->GetModel();
      if (OldCurrent.isValid() && NewCurrent.isValid() && (Model != nullptr))
      {
        std::vector<int> Rows;
        for (int Row = std::min(OldCurrent.row(), NewCurrent.row()); Row <= std::max(OldCurrent.row(), NewCurrent.row()); Row++)
        {
          if (!Model->Item(Row).IsParentDirectory)
          {
            Rows.push_back(Row);
          }
        }
        QItemSelection Selection;
        for (int Row : Rows)
        {
          Selection.select(Model->index(Row, 0), Model->index(Row, Model->columnCount() - 1));
        }
        selectionModel()->select(Selection, QItemSelectionModel::Select);
      }
      return;
    }
  }
  QTreeView::keyPressEvent(Event);
}
//---------------------------------------------------------------------------
void TDirView::MouseEvent(QMouseEvent * Event, const std::function<void()> & Handler)
{
  // The selection by a click without Ctrl and Shift is implicit
  bool Implicit = !(Event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier));
  bool PrevSelectingImplicitly = FSelectingImplicitly;
  FSelectingImplicitly = Implicit;
  Handler();
  FSelectingImplicitly = PrevSelectingImplicitly;
  if (Implicit && (Event->type() != QEvent::MouseMove) && selectionModel()->hasSelection())
  {
    FAnyAndAllSelectedImplicitly = true;
  }
}
//---------------------------------------------------------------------------
void TDirView::mousePressEvent(QMouseEvent * Event)
{
  MouseEvent(Event, [this, Event]() { QTreeView::mousePressEvent(Event); });
}
//---------------------------------------------------------------------------
void TDirView::mouseMoveEvent(QMouseEvent * Event)
{
  MouseEvent(Event, [this, Event]() { QTreeView::mouseMoveEvent(Event); });
}
//---------------------------------------------------------------------------
void TDirView::mouseReleaseEvent(QMouseEvent * Event)
{
  MouseEvent(Event, [this, Event]() { QTreeView::mouseReleaseEvent(Event); });
}
//---------------------------------------------------------------------------
void TDirView::keyboardSearch(const QString & Search)
{
  TIncrementalSearch Mode = AppConfiguration->PanelSearch;
  const TDirModel * Model = FPanel->GetModel();
  if ((Mode == isOff) || Search.isEmpty() || (Model == nullptr) || (Model->Count() == 0))
  {
    return;
  }

  // As Qt's: the typed letters add up until a pause
  bool Continue = FSearchTimer.isValid() && (FSearchTimer.elapsed() < QApplication::keyboardInputInterval());
  FSearchTimer.restart();
  // The same letter again goes to the next file
  bool Next = !Continue || ((FSearchText.length() == 1) && (FSearchText == Search));
  FSearchText = Continue && !Next ? (FSearchText + Search) : Search;

  int Start = currentIndex().isValid() ? currentIndex().row() : 0;
  for (int Offset = (Next ? 1 : 0); Offset <= Model->Count(); Offset++)
  {
    int Row = (Start + Offset) % Model->Count();
    const TDirViewItem & Item = Model->Item(Row);
    if (Item.IsParentDirectory)
    {
      continue;
    }
    bool Matches;
    QString Name = ToQString(Item.FileName);
    switch (Mode)
    {
      case isNameStartOnly:
        Matches = Name.startsWith(FSearchText, Qt::CaseInsensitive);
        break;

      case isName:
        Matches = Name.contains(FSearchText, Qt::CaseInsensitive);
        break;

      default:
        Matches = false;
        for (int Column = 0; !Matches && (Column < Model->columnCount()); Column++)
        {
          Matches = Model->data(Model->index(Row, Column)).toString().contains(FSearchText, Qt::CaseInsensitive);
        }
        break;
    }
    if (Matches)
    {
      QModelIndex Index = Model->index(Row, 0);
      selectionModel()->setCurrentIndex(Index, FocusCommand());
      scrollTo(Index);
      break;
    }
  }
}
//---------------------------------------------------------------------------
void TDirView::startDrag(Qt::DropActions /*SupportedActions*/)
{
  if (FPanel->OnStartDrag)
  {
    FPanel->OnStartDrag(FPanel);
  }
}
//---------------------------------------------------------------------------
UnicodeString TDirView::DropTarget(QDropEvent * Event, int & Row)
{
  Row = -1;
  QModelIndex Index = indexAt(Event->position().toPoint());
  const TDirModel * Model = FPanel->GetModel();
  if (Index.isValid() && (Index.row() < Model->Count()) && Model->Item(Index.row()).IsDirectory)
  {
    Row = Index.row();
    return FPanel->RowDirectory(Row);
  }
  return FPanel->GetPath();
}
//---------------------------------------------------------------------------
static TDropParams DropParams(QDropEvent * Event, const UnicodeString & TargetDirectory, bool OnDirectory)
{
  TDropParams Result;
  Result.Data = Event->mimeData();
  TDirView * SourceView = qobject_cast<TDirView *>(Event->source());
  Result.SourcePanel = (SourceView != nullptr) ? SourceView->GetPanel() : nullptr;
  Result.PossibleActions = Event->possibleActions();
  Result.ProposedAction = Event->proposedAction();
  Result.Modifiers = Event->modifiers();
  Result.TargetDirectory = TargetDirectory;
  Result.OnDirectory = OnDirectory;
  return Result;
}
//---------------------------------------------------------------------------
void TDirView::dragEnterEvent(QDragEnterEvent * Event)
{
  dragMoveEvent(Event);
  // A widget that ignores the enter gets no moves, while the files may be
  // dropped elsewhere on the view (a drag starts over its own files)
  if (!Event->isAccepted() && FPanel->IsReady() && FPanel->OnDragOver)
  {
    Event->setDropAction(Qt::IgnoreAction);
    Event->accept();
  }
}
//---------------------------------------------------------------------------
void TDirView::dragMoveEvent(QDragMoveEvent * Event)
{
  // Scrolls near the edges
  QTreeView::dragMoveEvent(Event);
  int Row;
  UnicodeString TargetDirectory = DropTarget(Event, Row);
  Qt::DropAction Action = Qt::IgnoreAction;
  if (FPanel->IsReady() && FPanel->OnDragOver)
  {
    Action = FPanel->OnDragOver(FPanel, DropParams(Event, TargetDirectory, (Row >= 0)));
  }
  if (Action != Qt::IgnoreAction)
  {
    Event->setDropAction(Action);
    Event->accept();
    SetDropTargetRow(Row);
  }
  else
  {
    Event->ignore();
    SetDropTargetRow(-1);
  }
}
//---------------------------------------------------------------------------
void TDirView::dragLeaveEvent(QDragLeaveEvent * Event)
{
  QTreeView::dragLeaveEvent(Event);
  SetDropTargetRow(-1);
}
//---------------------------------------------------------------------------
void TDirView::contextMenuEvent(QContextMenuEvent * Event)
{
  // The popup of the files under the pointer or of the focused file of the
  // keyboard, else of the directory
  QModelIndex Index;
  QPoint Pos;
  if (Event->reason() == QContextMenuEvent::Keyboard)
  {
    Index = currentIndex();
    QRect Rect = Index.isValid() ? visualRect(Index) : QRect();
    if (Rect.isValid() && viewport()->rect().intersects(Rect))
    {
      Pos = QPoint(Rect.left() + Rect.height(), Rect.bottom());
    }
    else
    {
      Index = QModelIndex();
      Pos = viewport()->rect().center();
    }
  }
  else
  {
    Pos = Event->pos();
    Index = indexAt(Pos);
  }
  const TDirModel * Model = FPanel->GetModel();
  bool OnFile = FPanel->IsReady() && Index.isValid() && !Model->Item(Index.row()).IsParentDirectory;
  // A file not selected is the only one selected, as in the file managers
  if (OnFile && !selectionModel()->isSelected(Index))
  {
    selectionModel()->setCurrentIndex(Index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  }
  Event->accept();
  Q_EMIT FPanel->ContextPopupRequested(FPanel, OnFile, viewport()->mapToGlobal(Pos));
}
//---------------------------------------------------------------------------
void TDirView::dropEvent(QDropEvent * Event)
{
  SetDropTargetRow(-1);
  int Row;
  UnicodeString TargetDirectory = DropTarget(Event, Row);
  TDropParams Params = DropParams(Event, TargetDirectory, (Row >= 0));
  Qt::DropAction Action = Qt::IgnoreAction;
  if (FPanel->IsReady() && FPanel->OnDragOver)
  {
    Action = FPanel->OnDragOver(FPanel, Params);
  }
  if ((Action != Qt::IgnoreAction) && FPanel->OnDrop)
  {
    Event->setDropAction(Action);
    Event->accept();
    FPanel->OnDrop(FPanel, Params, Action);
  }
  else
  {
    Event->ignore();
  }
  stopAutoScroll();
  setState(NoState);
}
//---------------------------------------------------------------------------
void TDirView::SetDropTargetRow(int Row)
{
  if (FDropTargetRow != Row)
  {
    FDropTargetRow = Row;
    viewport()->update();
  }
}
//---------------------------------------------------------------------------
void TDirView::drawRow(QPainter * Painter, const QStyleOptionViewItem & Option, const QModelIndex & Index) const
{
  if (Index.row() == FDropTargetRow)
  {
    QStyleOptionViewItem DropOption(Option);
    DropOption.state |= QStyle::State_Selected;
    QTreeView::drawRow(Painter, DropOption, Index);
  }
  else
  {
    QTreeView::drawRow(Painter, Option, Index);
  }
  // The focused file, which the keyboard does not select (the focus
  // rectangle of upstream's list views)
  if ((FNortonLike != nlOff) && hasFocus() && (Index.row() == currentIndex().row()))
  {
    QColor Color = palette().color(QPalette::Highlight);
    if (selectionModel()->isRowSelected(Index.row()))
    {
      Color = Color.darker(150);
    }
    Painter->save();
    Painter->setPen(QPen(Color, 1));
    Painter->setBrush(Qt::NoBrush);
    Painter->drawRect(QRect(-header()->offset(), Option.rect.y(), header()->length(), Option.rect.height()).adjusted(0, 0, -1, -1));
    Painter->restore();
  }
}
//---------------------------------------------------------------------------
TDirPanel::TDirPanel(TOperationSide Side, QWidget * Parent) :
  QWidget(Parent),
  FModel(nullptr),
  FShowHiddenFiles(false),
  FSide(Side),
  FHistoryIndex(-1),
  FNavigatingHistory(false),
  FUpdatingPath(false),
  FShowFullAddress(true)
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
  OpenDirAction = CreateAction(Prefix + QStringLiteral("OpenDirAction"), tr("&Open Directory/Bookmark..."),
    QStringLiteral("document-open-folder"), QKeySequence(Qt::CTRL | Qt::Key_O), &TDirPanel::OpenDirActionExecute);
  // The bookmarks, in the drop down menu of the button
  OpenDirMenu = new QMenu(this);
  OpenDirMenu->setObjectName(Prefix + QStringLiteral("OpenDirMenu"));
  connect(OpenDirMenu, &QMenu::aboutToShow, this, [this]()
  {
    qDeleteAll(OpenDirMenu->findChildren<QMenu *>(QString(), Qt::FindDirectChildrenOnly));
    OpenDirMenu->clear();
    Q_EMIT BookmarksMenuRequested(this, OpenDirMenu);
  });
  if (QToolButton * OpenDirButton = qobject_cast<QToolButton *>(PathToolbar->widgetForAction(OpenDirAction)))
  {
    OpenDirButton->setMenu(OpenDirMenu);
    OpenDirButton->setPopupMode(QToolButton::MenuButtonPopup);
  }
  AddBookmarkAction2 = CreateAction(Prefix + QStringLiteral("AddBookmarkAction2"), tr("&Add Path to Bookmarks"),
    QStringLiteral("bookmark-new"), QKeySequence(Qt::CTRL | Qt::Key_B), &TDirPanel::AddBookmarkActionExecute);
  // In the menu of the panel only
  FilterAction = new QAction(QIcon::fromTheme(QStringLiteral("view-filter")), tr("&Filter..."), this);
  FilterAction->setObjectName(Prefix + QStringLiteral("FilterAction"));
  FilterAction->setToolTip(tr("Filter displayed files"));
  FilterAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_F));
  FilterAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  addAction(FilterAction);
  connect(FilterAction, &QAction::triggered, this, [this]() { ExecuteHandler([this]() { Filter(); }); });

  DirView = new TDirView(this);
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
  DirView->setItemDelegate(new TRenameDelegate(DirView));
  connect(DirView->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, [this]()
  {
    if (FModel != nullptr)
    {
      FModel->SetEditableRow(-1);
    }
  });
  connect(DirView, &QTreeView::activated, this, [this](const QModelIndex & Index) { ItemActivated(Index.row()); });
  // The directory tree (of the panel) and the files
  FTree = nullptr;
  FViewSplitter = new QSplitter(Qt::Vertical, this);
  FViewSplitter->setObjectName(Prefix + QStringLiteral("ViewSplitter"));
  FViewSplitter->setChildrenCollapsible(false);
  FViewSplitter->addWidget(DirView);
  Layout->addWidget(FViewSplitter, 1);

  TreeAction = new QAction(tr("&Tree"), this);
  TreeAction->setObjectName(Prefix + QStringLiteral("TreeAction"));
  TreeAction->setToolTip(tr("Hide/show directory tree"));
  TreeAction->setShortcut(QKeySequence(Qt::CTRL | Qt::ALT | Qt::Key_T));
  TreeAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
  TreeAction->setCheckable(true);
  addAction(TreeAction);
  connect(TreeAction, &QAction::triggered, this, [this](bool Checked) { SetTreeVisible(Checked); });
  StatusBarAction = new QAction(tr("Status &Bar"), this);
  StatusBarAction->setObjectName(Prefix + QStringLiteral("StatusBarAction2"));
  StatusBarAction->setToolTip(tr("Hide/show status bar"));
  StatusBarAction->setCheckable(true);
  StatusBarAction->setChecked(true);
  connect(StatusBarAction, &QAction::triggered, this, [this](bool Checked) { SetStatusBarVisible(Checked); });
  CreateSortMenu(Prefix);
  CreateColumnsMenu(Prefix);

  // The selected and all the files, the hidden ones and the filtered ones,
  // clicked to show them
  FStatusPanel = new QWidget(this);
  FStatusPanel->setObjectName(Prefix + QStringLiteral("StatusPanel"));
  QHBoxLayout * StatusLayout = new QHBoxLayout(FStatusPanel);
  StatusLayout->setContentsMargins(0, 0, 0, 0);
  StatusLayout->setSpacing(2);
  StatusBar = new QLabel(FStatusPanel);
  StatusBar->setObjectName(Prefix + QStringLiteral("StatusBar"));
  HiddenFilesLabel = new QLabel(FStatusPanel);
  HiddenFilesLabel->setObjectName(Prefix + QStringLiteral("HiddenFilesLabel"));
  HiddenFilesLabel->setToolTip(tr("Click to show hidden files"));
  FilteredFilesLabel = new QLabel(FStatusPanel);
  FilteredFilesLabel->setObjectName(Prefix + QStringLiteral("FilteredFilesLabel"));
  FilteredFilesLabel->setToolTip(tr("Click to change filter"));
  for (QLabel * Label : { StatusBar, HiddenFilesLabel, FilteredFilesLabel })
  {
    Label->setFrameShape(QFrame::StyledPanel);
    Label->setFrameShadow(QFrame::Sunken);
    StatusLayout->addWidget(Label, (Label == StatusBar) ? 1 : 0);
  }
  HiddenFilesLabel->installEventFilter(this);
  FilteredFilesLabel->installEventFilter(this);
  HiddenFilesLabel->setVisible(false);
  FilteredFilesLabel->setVisible(false);
  Layout->addWidget(FStatusPanel);
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
  connect(Action, &QAction::triggered, this, [this, Method]() { ExecuteHandler([this, Method]() { (this->*Method)(); }); });
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
  Header->setSectionsMovable(true);
  for (int Column = 1; Column < Model->columnCount(); Column++)
  {
    Header->setSectionResizeMode(Column, QHeaderView::Interactive);
    Header->resizeSection(Column, DefaultColumnWidth(Column));
  }
  // The columns are shown and hidden by the menu of the header
  Header->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(Header, &QHeaderView::customContextMenuRequested, this, [this](const QPoint & Pos)
  {
    ColumnsMenu->exec(DirView->header()->mapToGlobal(Pos));
  });
  connect(Header, &QHeaderView::sortIndicatorChanged, this, [this]() { UpdateSortMenu(); });
  connect(DirView->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this]() { UpdateStatusBar(); });
  Model->OnRename = [this](const UnicodeString & FileName, const UnicodeString & NewName)
  {
    // Not from the commit of the editor, which the reload would disturb
    QTimer::singleShot(0, this, [this, FileName, NewName]()
    {
      if (OnRename)
      {
        OnRename(this, FileName, NewName);
      }
    });
  };
}
//---------------------------------------------------------------------------
int TDirPanel::DefaultColumnWidth(int Column) const
{
  // Widths for typical values
  QString Sample;
  switch (FModel->Column(Column))
  {
    case dvSize:
      Sample = QStringLiteral("99,999 KB");
      break;
    case dvChanged:
      Sample = QStringLiteral("00/00/0000 00:00:00");
      break;
    case dvType:
      Sample = QStringLiteral("Plain text");
      break;
    case dvRights:
      Sample = QStringLiteral("rwxrwxrwx");
      break;
    case dvExt:
      Sample = QStringLiteral("ext");
      break;
    case dvLinkTarget:
      Sample = QStringLiteral("/usr/local/target");
      break;
    default:
      Sample = QStringLiteral("username");
      break;
  }
  return DirView->fontMetrics().horizontalAdvance(Sample) + 32;
}
//---------------------------------------------------------------------------
void TDirPanel::InitTree(QWidget * Tree)
{
  FTree = Tree;
  FViewSplitter->insertWidget(0, FTree);
  FViewSplitter->setStretchFactor(1, 1);
  FTree->setVisible(false);
}
//---------------------------------------------------------------------------
void TDirPanel::SetTreeVisible(bool Visible)
{
  if (FTree != nullptr)
  {
    FTree->setVisible(Visible);
    if (Visible)
    {
      UpdateTree();
    }
  }
  TreeAction->setChecked(Visible);
}
//---------------------------------------------------------------------------
bool TDirPanel::GetTreeVisible() const
{
  return (FTree != nullptr) && !FTree->isHidden();
}
//---------------------------------------------------------------------------
void TDirPanel::SetTreeOnLeft(bool Value)
{
  FViewSplitter->setOrientation(Value ? Qt::Horizontal : Qt::Vertical);
}
//---------------------------------------------------------------------------
int TDirPanel::GetTreeSize() const
{
  QList<int> Sizes = FViewSplitter->sizes();
  return (GetTreeVisible() && (Sizes.size() > 1)) ? Sizes[0] : 0;
}
//---------------------------------------------------------------------------
void TDirPanel::SetTreeSize(int Value)
{
  if (Value > 0)
  {
    int Total = (FViewSplitter->orientation() == Qt::Horizontal) ? FViewSplitter->width() : FViewSplitter->height();
    FViewSplitter->setSizes({ Value, std::max(Total - Value, Value) });
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SetStatusBarVisible(bool Visible)
{
  FStatusPanel->setVisible(Visible);
  StatusBarAction->setChecked(Visible);
}
//---------------------------------------------------------------------------
bool TDirPanel::GetStatusBarVisible() const
{
  return !FStatusPanel->isHidden();
}
//---------------------------------------------------------------------------
void TDirPanel::CreateSortMenu(const QString & Prefix)
{
  // upstream's Sort submenus, the shortcuts as of its Commander
  SortMenu = new QMenu(tr("&Sort"), this);
  SortMenu->setObjectName(Prefix + QStringLiteral("SortMenu"));
  SortAscendingAction = SortMenu->addAction(tr("&Ascending"));
  SortAscendingAction->setObjectName(Prefix + QStringLiteral("SortAscendingAction2"));
  SortAscendingAction->setCheckable(true);
  connect(SortAscendingAction, &QAction::triggered, this, [this](bool Checked)
  {
    DirView->sortByColumn(FModel->GetSortColumn(), Checked ? Qt::AscendingOrder : Qt::DescendingOrder);
  });
  SortMenu->addSeparator();
  QActionGroup * Group = new QActionGroup(SortMenu);
  struct { TDirViewColumn Column; const char * Name; QString Caption; int Key; } Sorts[] =
  {
    { dvName, "SortByNameAction2", tr("By &Name"), Qt::Key_F3 },
    { dvExt, "SortByExtAction2", tr("By &Extension"), Qt::Key_F4 },
    { dvSize, "SortBySizeAction2", tr("By &Size"), Qt::Key_F6 },
    { dvType, "SortByTypeAction2", tr("By &Type"), 0 },
    { dvChanged, "SortByChangedAction3", tr("By Date &modified"), Qt::Key_F5 },
    { dvRights, "SortByRightsAction2", tr("By &Permissions"), Qt::Key_F7 },
    { dvOwner, "SortByOwnerAction2", tr("By &Owner"), 0 },
    { dvGroup, "SortByGroupAction2", tr("By &Group"), 0 },
  };
  for (auto & Sort : Sorts)
  {
    TDirViewColumn Column = Sort.Column;
    QAction * Action = SortMenu->addAction(Sort.Caption);
    Action->setObjectName(Prefix + QString::fromLatin1(Sort.Name));
    Action->setCheckable(true);
    Action->setData(static_cast<int>(Column));
    Action->setActionGroup(Group);
    if (Sort.Key != 0)
    {
      Action->setShortcut(QKeySequence(Qt::CTRL | static_cast<Qt::Key>(Sort.Key)));
      Action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
      addAction(Action);
    }
    connect(Action, &QAction::triggered, this, [this, Column]()
    {
      int Index = FModel->ColumnIndex(Column);
      if (Index >= 0)
      {
        // The same column again reverses the order, as upstream's
        Qt::SortOrder Order =
          (FModel->GetSortColumn() == Index) && (FModel->GetSortOrder() == Qt::AscendingOrder) ?
            Qt::DescendingOrder : Qt::AscendingOrder;
        DirView->sortByColumn(Index, Order);
      }
    });
  }
  connect(SortMenu, &QMenu::aboutToShow, this, [this]() { UpdateSortMenu(); });
}
//---------------------------------------------------------------------------
void TDirPanel::UpdateSortMenu()
{
  if (FModel == nullptr)
  {
    return;
  }
  SortAscendingAction->setChecked(FModel->GetSortOrder() == Qt::AscendingOrder);
  for (QAction * Action : SortMenu->actions())
  {
    if (Action->data().isValid())
    {
      int Index = FModel->ColumnIndex(static_cast<TDirViewColumn>(Action->data().toInt()));
      Action->setVisible(Index >= 0);
      Action->setEnabled(Index >= 0);
      Action->setChecked((Index >= 0) && (FModel->GetSortColumn() == Index));
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::CreateColumnsMenu(const QString & Prefix)
{
  ColumnsMenu = new QMenu(tr("&Columns"), this);
  ColumnsMenu->setObjectName(Prefix + QStringLiteral("ColumnsMenu"));
  connect(ColumnsMenu, &QMenu::aboutToShow, this, [this]() { UpdateColumnsMenu(); });
}
//---------------------------------------------------------------------------
void TDirPanel::UpdateColumnsMenu()
{
  // Created once the columns are known
  if (ColumnsMenu->isEmpty() && (FModel != nullptr))
  {
    QString Prefix = (FSide == osLocal) ? QStringLiteral("Local") : QStringLiteral("Remote");
    for (int Column = 0; Column < FModel->columnCount(); Column++)
    {
      QString Caption = FModel->headerData(Column, Qt::Horizontal).toString();
      QAction * Action = ColumnsMenu->addAction(tr("Show %1 column").arg(Caption));
      Action->setObjectName(Prefix + QStringLiteral("ShowHideColumn%1Action").arg(Column));
      Action->setCheckable(true);
      Action->setData(Column);
      // The name is always shown
      Action->setEnabled(Column > 0);
      connect(Action, &QAction::triggered, this, [this, Column](bool Checked)
      {
        DirView->setColumnHidden(Column, !Checked);
      });
    }
    ColumnsMenu->addSeparator();
    QAction * AutoSizeAction = ColumnsMenu->addAction(tr("Size &Automatically"));
    AutoSizeAction->setObjectName(Prefix + QStringLiteral("AutoSizeColumnsAction"));
    connect(AutoSizeAction, &QAction::triggered, this, [this]()
    {
      for (int Column = 1; Column < FModel->columnCount(); Column++)
      {
        if (!DirView->isColumnHidden(Column))
        {
          DirView->resizeColumnToContents(Column);
        }
      }
    });
    QAction * ResetAction = ColumnsMenu->addAction(tr("&Reset Layout"));
    ResetAction->setObjectName(Prefix + QStringLiteral("ResetLayoutColumnsAction"));
    connect(ResetAction, &QAction::triggered, this, [this]() { ResetLayoutColumns(); });
  }
  for (QAction * Action : ColumnsMenu->actions())
  {
    if (Action->isCheckable())
    {
      Action->setChecked(!DirView->isColumnHidden(Action->data().toInt()));
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::ResetLayoutColumns()
{
  QHeaderView * Header = DirView->header();
  for (int Column = 0; Column < FModel->columnCount(); Column++)
  {
    Header->moveSection(Header->visualIndex(Column), Column);
    DirView->setColumnHidden(Column, (Column > 0) && IsColumnHiddenByDefault(FModel->Column(Column)));
    if (Column > 0)
    {
      Header->resizeSection(Column, DefaultColumnWidth(Column));
    }
  }
  DirView->sortByColumn(0, Qt::AscendingOrder);
}
//---------------------------------------------------------------------------
UnicodeString TDirPanel::GetDirViewParams() const
{
  // upstream's "sort|widths;@pixels-per-inch|order": the sort column and
  // the order, the width and the visibility of each column, the columns
  // in their order
  QHeaderView * Header = DirView->header();
  UnicodeString Widths;
  UnicodeString Order;
  for (int Column = 0; Column < FModel->columnCount(); Column++)
  {
    Widths += FORMAT(L"%d,%d;", (Header->sectionSize(Column), (DirView->isColumnHidden(Column) ? 0 : 1)));
    AddToList(Order, IntToStr(Header->logicalIndex(Column)), L";");
  }
  return
    FORMAT(L"%d;%d;0|%s@%d|%s",
      (FModel->GetSortColumn(), ((FModel->GetSortOrder() == Qt::AscendingOrder) ? 1 : 0), Widths,
       DirView->logicalDpiX(), Order));
}
//---------------------------------------------------------------------------
void TDirPanel::SetDirViewParams(const UnicodeString & Value)
{
  if (Value.IsEmpty())
  {
    ResetLayoutColumns();
    return;
  }
  QHeaderView * Header = DirView->header();
  UnicodeString Buf = Value;
  UnicodeString SortStr = CutToChar(Buf, L'|', true);
  UnicodeString WidthsStr = CutToChar(Buf, L'@', true);
  int PixelsPerInch = StrToIntDef(CutToChar(Buf, L'|', true), DirView->logicalDpiX());
  if (PixelsPerInch <= 0)
  {
    PixelsPerInch = DirView->logicalDpiX();
  }
  UnicodeString OrderStr = Buf;

  for (int Column = 0; (Column < FModel->columnCount()) && !WidthsStr.IsEmpty(); Column++)
  {
    UnicodeString ColStr = CutToChar(WidthsStr, L';', true);
    int Width = StrToIntDef(CutToChar(ColStr, L',', true), 0);
    bool Visible = (StrToIntDef(CutToChar(ColStr, L',', true), 1) != 0);
    if ((Column > 0) && (Width > 0))
    {
      Header->resizeSection(Column, Width * DirView->logicalDpiX() / PixelsPerInch);
    }
    DirView->setColumnHidden(Column, (Column > 0) && !Visible);
  }

  // The columns in their order
  int Visual = 0;
  while (!OrderStr.IsEmpty() && (Visual < FModel->columnCount()))
  {
    int Column = StrToIntDef(CutToChar(OrderStr, L';', true), -1);
    if ((Column >= 0) && (Column < FModel->columnCount()))
    {
      Header->moveSection(Header->visualIndex(Column), Visual);
      Visual++;
    }
  }

  int SortColumn = StrToIntDef(CutToChar(SortStr, L';', true), 0);
  bool SortAscending = (StrToIntDef(CutToChar(SortStr, L';', true), 1) != 0);
  if ((SortColumn < 0) || (SortColumn >= FModel->columnCount()))
  {
    SortColumn = 0;
  }
  DirView->sortByColumn(SortColumn, SortAscending ? Qt::AscendingOrder : Qt::DescendingOrder);
}
//---------------------------------------------------------------------------
void TDirPanel::FillMenu(QMenu * Menu, QAction * PathToClipboardAction)
{
  QString Prefix = (FSide == osLocal) ? QStringLiteral("Local") : QStringLiteral("Remote");
  QMenu * GoToMenu = Menu->addMenu(tr("&Go To"));
  GoToMenu->setObjectName(Prefix + QStringLiteral("GoToMenu"));
  GoToMenu->addActions({ OpenDirAction, ParentDirAction, RootDirAction, HomeDirAction });
  GoToMenu->addSeparator();
  GoToMenu->addActions({ BackAction, ForwardAction });
  Menu->addAction(RefreshAction);
  Menu->addAction(AddBookmarkAction2);
  if (PathToClipboardAction != nullptr)
  {
    Menu->addAction(PathToClipboardAction);
  }
  Menu->addSeparator();
  Menu->addMenu(SortMenu);
  Menu->addMenu(ColumnsMenu);
  Menu->addAction(FilterAction);
}
//---------------------------------------------------------------------------
void TDirPanel::HomeDirActionExecute()
{
  if (!OnHomeDirectory || !OnHomeDirectory(this))
  {
    HomeDirectory();
  }
}
//---------------------------------------------------------------------------
void TDirPanel::FillDirViewPopup(QMenu * Menu, QAction * PathToClipboardAction)
{
  QMenu * GoToMenu = Menu->addMenu(tr("&Go To"));
  GoToMenu->addActions({ OpenDirAction, ParentDirAction, RootDirAction, HomeDirAction });
  GoToMenu->addSeparator();
  GoToMenu->addActions({ BackAction, ForwardAction });
  Menu->addAction(RefreshAction);
  Menu->addAction(AddBookmarkAction2);
  Menu->addAction(FilterAction);
  if (PathToClipboardAction != nullptr)
  {
    Menu->addAction(PathToClipboardAction);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SetCalculatedSize(const UnicodeString & FileName, __int64 Size)
{
  int Row = FModel->FindFile(FileName);
  if (Row >= 0)
  {
    FModel->SetCalculatedSize(Row, Size);
    UpdateStatusBar();
  }
}
//---------------------------------------------------------------------------
void TDirPanel::FillOptionsMenu(QMenu * Menu)
{
  Menu->addAction(TreeAction);
  Menu->addAction(StatusBarAction);
}
//---------------------------------------------------------------------------
void TDirPanel::StartRename()
{
  QModelIndex Current = DirView->currentIndex();
  if (IsReady() && Current.isValid() && !FModel->Item(Current.row()).IsParentDirectory)
  {
    QModelIndex NameIndex = FModel->index(Current.row(), 0);
    FModel->SetEditableRow(NameIndex.row());
    DirView->edit(NameIndex);
    // Without the extension of files, unless RenameWholeName
    const TDirViewItem & Item = FModel->Item(Current.row());
    if (!AppConfiguration->RenameWholeName && !Item.IsDirectory)
    {
      QString Name = ToQString(Item.FileName);
      int P = Name.lastIndexOf(QLatin1Char('.'));
      if (P > 0)
      {
        QPointer<QLineEdit> Editor = DirView->findChild<QLineEdit *>(QStringLiteral("RenameEdit"));
        // Once the editor got the focus (which selects all)
        QTimer::singleShot(0, this, [Editor, P]()
        {
          if (Editor)
          {
            Editor->setSelection(0, P);
          }
        });
      }
    }
  }
}
//---------------------------------------------------------------------------
bool TDirPanel::eventFilter(QObject * Object, QEvent * Event)
{
  if ((Event->type() == QEvent::FocusIn) && ((Object == DirView) || (Object == PathComboBox)))
  {
    Q_EMIT Entered(this);
  }
  else if ((Event->type() == QEvent::MouseButtonRelease) && (Object == HiddenFilesLabel))
  {
    Q_EMIT ToggleHiddenFilesRequested();
  }
  else if ((Event->type() == QEvent::MouseButtonRelease) && (Object == FilteredFilesLabel))
  {
    ExecuteHandler([this]() { Filter(); });
  }
  return QWidget::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TDirPanel::PathComboActivated()
{
  if (!FUpdatingPath)
  {
    UnicodeString Path = ToUnicodeString(PathComboBox->currentText()).Trim();
    // The name of the directory shown instead of its path
    bool ShortAddress = !FShowFullAddress && (Path == UnixExtractFileName(GetPath()));
    if (!Path.IsEmpty() && (Path != GetPath()) && !ShortAddress)
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
    else
    {
      Q_EMIT FileActivated(this);
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
    if (!FShowFullAddress)
    {
      PathComboBox->setEditText(ToQString(IsUnixRootPath(Path) ? Path : UnixExtractFileName(Path)));
    }
  }

  // The focus of the new directory is set when its files are loaded
  DirView->selectionModel()->clear();
  UpdateActions();
  Q_EMIT DirectoryChanged(this);
}
//---------------------------------------------------------------------------
void TDirPanel::SaveFocus()
{
  // Keeps the focused file and the selected ones over a reload of the same
  // directory
  if (FModel != nullptr)
  {
    if (FFocusAfterReload.IsEmpty())
    {
      QModelIndex Current = DirView->currentIndex();
      if (Current.isValid() && (Current.row() < FModel->Count()))
      {
        FFocusAfterReload = FModel->Item(Current.row()).FileName;
      }
    }
    FSelectionAfterReload.clear();
    for (const QModelIndex & Index : DirView->selectionModel()->selectedRows())
    {
      FSelectionAfterReload.insert(FModel->Item(Index.row()).FileName);
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
  if (!FSelectionAfterReload.empty())
  {
    std::vector<int> Rows;
    for (int ARow = 0; ARow < FModel->Count(); ARow++)
    {
      if (FSelectionAfterReload.count(FModel->Item(ARow).FileName) > 0)
      {
        Rows.push_back(ARow);
      }
    }
    FSelectionAfterReload.clear();
    SelectRows(Rows, true);
  }
  UpdateStatusBar();
}
//---------------------------------------------------------------------------
TObject * TDirPanel::SaveState()
{
  TDirPanelState * State = new TDirPanelState();
  State->History = FHistory;
  State->HistoryIndex = FHistoryIndex;
  QModelIndex Current = DirView->currentIndex();
  if (IsReady() && Current.isValid() && (Current.row() < FModel->Count()))
  {
    State->FocusedFile = FModel->Item(Current.row()).FileName;
  }
  return State;
}
//---------------------------------------------------------------------------
void TDirPanel::RestoreState(TObject * AState)
{
  TDirPanelState * State = dynamic_cast<TDirPanelState *>(AState);
  if (State != nullptr)
  {
    FHistory = State->History;
    FHistoryIndex = State->HistoryIndex;
  }
  else
  {
    FHistory.clear();
    FHistoryIndex = -1;
  }
  // The directory shown is the current one of the history
  if (IsReady())
  {
    UnicodeString Path = GetPath();
    if ((FHistoryIndex < 0) || (FHistoryIndex >= static_cast<int>(FHistory.size())) || (FHistory[FHistoryIndex] != Path))
    {
      FHistory.resize(std::max(0, std::min(FHistoryIndex + 1, static_cast<int>(FHistory.size()))));
      FHistory.push_back(Path);
      FHistoryIndex = static_cast<int>(FHistory.size()) - 1;
    }
    FPrevPath = Path;
    if ((State != nullptr) && !State->FocusedFile.IsEmpty())
    {
      int Row = FModel->FindFile(State->FocusedFile);
      if (Row >= 0)
      {
        QModelIndex Index = FModel->index(Row, 0);
        DirView->selectionModel()->setCurrentIndex(Index, QItemSelectionModel::NoUpdate);
        DirView->scrollTo(Index);
      }
    }
  }
  UpdateActions();
}
//---------------------------------------------------------------------------
void TDirPanel::FocusFile(const UnicodeString & FileName)
{
  int Row = FModel->FindFile(FileName);
  if (Row >= 0)
  {
    QModelIndex Index = FModel->index(Row, 0);
    DirView->selectionModel()->setCurrentIndex(Index, DirView->FocusCommand());
    DirView->scrollTo(Index);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SelectRows(const std::vector<int> & Rows, bool Select)
{
  // The ranges of the consecutive rows
  QItemSelection Selection;
  size_t Index = 0;
  while (Index < Rows.size())
  {
    size_t End = Index;
    while ((End + 1 < Rows.size()) && (Rows[End + 1] == Rows[End] + 1))
    {
      End++;
    }
    Selection.select(FModel->index(Rows[Index], 0), FModel->index(Rows[End], FModel->columnCount() - 1));
    Index = End + 1;
  }
  if (!Selection.isEmpty())
  {
    DirView->selectionModel()->select(Selection, Select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SelectFiles(const TFileFilter & Filter, bool Select)
{
  // upstream's ItemMatchesFilter: the directories match as files too
  // when asked
  TFileMasks Masks(Filter.Masks);
  std::vector<int> Rows;
  for (int Row = 0; Row < FModel->Count(); Row++)
  {
    const TDirViewItem & Item = FModel->Item(Row);
    if (!Item.IsParentDirectory)
    {
      TFileMasks::TParams MaskParams;
      MaskParams.Size = Item.Size;
      MaskParams.Modification = Item.Modification;
      bool ImplicitMatch;
      bool Matches =
        Filter.Masks.IsEmpty() ||
        (Masks.Matches(Item.FileName, false, Item.IsDirectory, &MaskParams, true, ImplicitMatch) && !ImplicitMatch) ||
        (Item.IsDirectory && Filter.Directories &&
         Masks.Matches(Item.FileName, false, false, &MaskParams, true, ImplicitMatch) && !ImplicitMatch);
      if (Matches)
      {
        Rows.push_back(Row);
      }
    }
  }
  SelectRows(Rows, Select);
}
//---------------------------------------------------------------------------
void TDirPanel::SelectAll(TSelectMode Mode)
{
  QItemSelectionModel * Selection = DirView->selectionModel();
  std::vector<int> Select;
  for (int Row = 0; Row < FModel->Count(); Row++)
  {
    if (!FModel->Item(Row).IsParentDirectory &&
        ((Mode == smAll) || ((Mode == smInvert) && !Selection->isRowSelected(Row))))
    {
      Select.push_back(Row);
    }
  }
  // Keeps the focused file
  QModelIndex Current = DirView->currentIndex();
  Selection->select(QItemSelection(), QItemSelectionModel::Clear);
  SelectRows(Select, true);
  if (Current.isValid())
  {
    Selection->setCurrentIndex(Current, QItemSelectionModel::NoUpdate);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SelectCurrentItem()
{
  DirView->SelectCurrentItem(DirView->GetNortonLike() != nlOff);
}
//---------------------------------------------------------------------------
void TDirPanel::SaveSelectedNames()
{
  FSavedNames.clear();
  for (const QModelIndex & Index : DirView->selectionModel()->selectedRows())
  {
    FSavedNames.insert(FModel->Item(Index.row()).FileName);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::RestoreSelectedNames()
{
  std::vector<int> Rows;
  for (int Row = 0; Row < FModel->Count(); Row++)
  {
    if (FSavedNames.count(FModel->Item(Row).FileName) > 0)
    {
      Rows.push_back(Row);
    }
  }
  QModelIndex Current = DirView->currentIndex();
  DirView->selectionModel()->select(QItemSelection(), QItemSelectionModel::Clear);
  SelectRows(Rows, true);
  if (Current.isValid())
  {
    DirView->selectionModel()->setCurrentIndex(Current, QItemSelectionModel::NoUpdate);
  }
}
//---------------------------------------------------------------------------
int TDirPanel::GetSelCount() const
{
  int Result = 0;
  if (IsReady())
  {
    for (const QModelIndex & Index : DirView->selectionModel()->selectedRows())
    {
      if (!FModel->Item(Index.row()).IsParentDirectory)
      {
        Result++;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int TDirPanel::GetFilesCount() const
{
  __int64 Size;
  int Count = 0;
  if (IsReady())
  {
    FModel->Totals(Size, Count);
  }
  return Count;
}
//---------------------------------------------------------------------------
bool TDirPanel::HasFocusedFile() const
{
  QModelIndex Current = DirView->currentIndex();
  return IsReady() && Current.isValid() && (Current.row() < FModel->Count()) && !FModel->Item(Current.row()).IsParentDirectory;
}
//---------------------------------------------------------------------------
UnicodeString TDirPanel::GetFocusedFileName() const
{
  return HasFocusedFile() ? FModel->Item(DirView->currentIndex().row()).FileName : UnicodeString();
}
//---------------------------------------------------------------------------
void TDirPanel::CompareFiles(TDirPanel * Other, bool CompareByTime, bool CompareBySize)
{
  // upstream's ProcessChangedFiles
  const TDirModel * OtherModel = Other->GetModel();
  std::vector<int> Changed;
  for (int Row = 0; Row < FModel->Count(); Row++)
  {
    const TDirViewItem & Item = FModel->Item(Row);
    if (!Item.IsDirectory)
    {
      bool AChanged = false;
      int MirrorRow = OtherModel->FindFile(Item.FileName);
      if (MirrorRow < 0)
      {
        AChanged = true;
      }
      else
      {
        const TDirViewItem & MirrorItem = OtherModel->Item(MirrorRow);
        bool SameTime;
        if (CompareByTime)
        {
          TDateTime FileTime = Item.Modification;
          TDateTime MirrorFileTime = MirrorItem.Modification;
          TModificationFmt Precision = LessDateTimePrecision(Item.ModificationFmt, MirrorItem.ModificationFmt);
          FileTime = ReduceDateTimePrecision(FileTime, Precision);
          MirrorFileTime = ReduceDateTimePrecision(MirrorFileTime, Precision);
          SameTime = (FileTime == MirrorFileTime);
          if (Precision == mfFull)
          {
            // 1 ms more solves the rounding issues
            MirrorFileTime = MirrorFileTime + EncodeTimeVerbose(0, 0, 1, 1);
          }
          AChanged = (FileTime > MirrorFileTime);
        }
        else
        {
          SameTime = true;
        }
        if (!AChanged && SameTime && CompareBySize)
        {
          AChanged = (Item.Size != MirrorItem.Size);
        }
      }
      if (AChanged)
      {
        Changed.push_back(Row);
      }
    }
  }
  QModelIndex Current = DirView->currentIndex();
  DirView->selectionModel()->select(QItemSelection(), QItemSelectionModel::Clear);
  SelectRows(Changed, true);
  if (Current.isValid())
  {
    DirView->selectionModel()->setCurrentIndex(Current, QItemSelectionModel::NoUpdate);
  }
}
//---------------------------------------------------------------------------
void TDirPanel::Filter()
{
  if (IsReady())
  {
    UnicodeString Mask = GetMask();
    if (DoFilterMaskDialog(this, Mask))
    {
      SetMask(TFileMasks::NormalizeMask(Mask));
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SetShowFullAddress(bool Value)
{
  if (FShowFullAddress != Value)
  {
    FShowFullAddress = Value;
    if (IsReady())
    {
      TAutoFlag UpdatingFlag(FUpdatingPath);
      UnicodeString Path = GetPath();
      PathComboBox->setEditText(
        ToQString((FShowFullAddress || IsUnixRootPath(Path)) ? Path : UnixExtractFileName(Path)));
    }
  }
}
//---------------------------------------------------------------------------
void TDirPanel::SetRefreshShortCut(const QKeySequence & ShortCut)
{
  RefreshAction->setShortcut(ShortCut);
}
//---------------------------------------------------------------------------
UnicodeString TDirPanel::GetMask() const
{
  return FModel->GetMask();
}
//---------------------------------------------------------------------------
void TDirPanel::SetMask(const UnicodeString & Mask)
{
  if (GetMask() != Mask)
  {
    FModel->SetMask(Mask);
    if (IsReady())
    {
      Reload();
    }
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
UnicodeString TDirPanel::RowDirectory(int Row) const
{
  const TDirViewItem & Item = FModel->Item(Row);
  UnicodeString Path = GetPath();
  if (Item.IsParentDirectory)
  {
    UnicodeString Parent = UnixExtractFileDir(Path);
    return Parent.IsEmpty() ? UnicodeString(L"/") : Parent;
  }
  return UnixIncludeTrailingBackslash(Path) + Item.FileName;
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
void TDirPanel::ApplyConfiguration()
{
  QFont Font = PanelFont(AppConfiguration->PanelFont);
  if (DirView->font() != Font)
  {
    DirView->setFont(Font);
  }
  DirView->SetNortonLike(AppConfiguration->NortonLikeMode);
  if (FFileColors != AppConfiguration->FileColors)
  {
    FFileColors = AppConfiguration->FileColors;
    TFileColorData::TList FileColors;
    TFileColorData::LoadList(FFileColors, FileColors);
    FModel->SetFileColors(FileColors);
  }
  FModel->SetDisplayOptions(
    AppConfiguration->FormatSizeBytes, AppConfiguration->NaturalOrderNumericalSorting,
    AppConfiguration->AlwaysSortDirectoriesByName);
  SetShowHiddenFiles(AppConfiguration->ShowHiddenFiles);
  UpdateStatusBar();
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
  int HiddenCount = (IsReady() && (FModel != nullptr)) ? FModel->GetHiddenCount() : 0;
  HiddenFilesLabel->setText(ToQString(FMTLOAD(FILE_INFO_HIDDEN2, (FormatNumber(HiddenCount)))));
  HiddenFilesLabel->setVisible(HiddenCount > 0);
  int FilteredCount = (IsReady() && (FModel != nullptr)) ? FModel->GetFilteredCount() : 0;
  FilteredFilesLabel->setText(ToQString(FMTLOAD(FILE_INFO_FILTERED2, (FormatNumber(FilteredCount)))));
  FilteredFilesLabel->setVisible(FilteredCount > 0);
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
  OpenDirAction->setEnabled(Ready);
  AddBookmarkAction2->setEnabled(Ready);
  FilterAction->setEnabled(Ready);
  PathComboBox->setEnabled(Ready);
}
//---------------------------------------------------------------------------
TLocalDirPanel::TLocalDirPanel(QWidget * Parent) :
  TDirPanel(osLocal, Parent)
{
  FLocalModel = new TLocalDirModel(this);
  SetModel(FLocalModel);
  ResetLayoutColumns();

  FLocalTree = new TLocalDirTree(this);
  FLocalTree->setObjectName(QStringLiteral("LocalDriveView"));
  InitTree(FLocalTree);
  connect(FLocalTree, &TLocalDirTree::DirectorySelected, this, [this](const UnicodeString & Path)
  {
    if (Path != GetPath())
    {
      ExecuteHandler([this, Path]() { ChangeDirectory(Path); });
    }
  });

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
  UpdateTree();
}
//---------------------------------------------------------------------------
void TLocalDirPanel::UpdateTree()
{
  if (GetTreeVisible())
  {
    FLocalTree->SetPath(GetPath());
  }
}
//---------------------------------------------------------------------------
void TLocalDirPanel::ApplyConfiguration()
{
  TDirPanel::ApplyConfiguration();
  FLocalTree->SetShowHiddenFiles(FShowHiddenFiles);
}
//---------------------------------------------------------------------------
bool TLocalDirPanel::IsColumnHiddenByDefault(TDirViewColumn Column) const
{
  return (Column == dvExt);
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
  ChangeDirectory(FHomeDirectory.IsEmpty() ? ToUnicodeString(QDir::homePath()) : FHomeDirectory);
}
//---------------------------------------------------------------------------
TRemoteDirPanel::TRemoteDirPanel(QWidget * Parent) :
  TDirPanel(osRemote, Parent),
  FTerminal(NULL),
  FShownReady(false),
  FShowInaccesibleDirectories(true)
{
  FRemoteModel = new TRemoteDirModel(this);
  SetModel(FRemoteModel);
  ResetLayoutColumns();

  FRemoteTree = new TRemoteDirTree(this);
  FRemoteTree->setObjectName(QStringLiteral("RemoteDriveView"));
  InitTree(FRemoteTree);
  connect(FRemoteTree, &TRemoteDirTree::DirectorySelected, this, [this](const UnicodeString & Path)
  {
    if (IsReady() && !UnixSamePath(Path, GetPath()))
    {
      ExecuteHandler([this, Path]() { ChangeDirectory(Path); });
    }
  });

  // The view or the "not connected" page in its place
  QVBoxLayout * Layout = static_cast<QVBoxLayout *>(layout());
  int Index = Layout->indexOf(FViewSplitter);
  FStack = new QStackedWidget(this);
  FStack->setObjectName(QStringLiteral("RemoteStack"));
  Layout->insertWidget(Index, FStack, 1);
  FStack->addWidget(FViewSplitter);
  QWidget * NoSessionPage = new QWidget(FStack);
  NoSessionPage->setObjectName(QStringLiteral("RemoteNoSessionPage"));
  QVBoxLayout * PageLayout = new QVBoxLayout(NoSessionPage);
  PageLayout->addStretch(1);
  FNoSessionLabel = new QLabel(tr("Not connected."), NoSessionPage);
  FNoSessionLabel->setObjectName(QStringLiteral("RemoteNoSessionLabel"));
  FNoSessionLabel->setAlignment(Qt::AlignCenter);
  PageLayout->addWidget(FNoSessionLabel);
  FNewSessionButton = new QPushButton(tr("&New Session..."), NoSessionPage);
  FNewSessionButton->setObjectName(QStringLiteral("RemoteNewSessionButton"));
  PageLayout->addWidget(FNewSessionButton, 0, Qt::AlignCenter);
  // upstream's ReconnectToolbar: the session of the tab was disconnected
  ReconnectItem = new QPushButton(QIcon::fromTheme(QStringLiteral("view-refresh")), tr("&Reconnect"), NoSessionPage);
  ReconnectItem->setObjectName(QStringLiteral("ReconnectItem"));
  PageLayout->addWidget(ReconnectItem, 0, Qt::AlignCenter);
  ReconnectLabel = new QLabel(NoSessionPage);
  ReconnectLabel->setObjectName(QStringLiteral("ReconnectLabel"));
  ReconnectLabel->setAlignment(Qt::AlignCenter);
  ReconnectLabel->setWordWrap(true);
  PageLayout->addWidget(ReconnectLabel);
  PageLayout->addStretch(1);
  FStack->addWidget(NoSessionPage);
  connect(FNewSessionButton, &QPushButton::clicked, this, [this]() { Q_EMIT NewSessionRequested(); });
  connect(ReconnectItem, &QPushButton::clicked, this, [this]() { Q_EMIT ReconnectRequested(); });

  // Not shown yet
  FShownReady = true;
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
  // The files of the terminal shown follow its events
  if ((FTerminal == Terminal) && (FShownReady == ((Terminal != NULL) && Terminal->Active)))
  {
    return;
  }
  if ((FTerminal != NULL) && (FTerminal != Terminal))
  {
    FTerminal->OnChangeDirectory = NULL;
    FTerminal->OnReadDirectory = NULL;
  }
  FTerminal = Terminal;
  FRemoteTree->SetTerminal(Terminal);
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
    // A session of a tab is disconnected, else there is none
    bool Disconnected = (FTerminal != NULL);
    FNoSessionLabel->setText(Disconnected ? tr("Disconnected.") : tr("Not connected."));
    FNewSessionButton->setVisible(!Disconnected);
    ReconnectItem->setVisible(Disconnected);
    if (!Disconnected)
    {
      ReconnectLabel->clear();
    }
    ReconnectLabel->setVisible(Disconnected && !ReconnectLabel->text().isEmpty());
    FStack->setCurrentIndex(1);
  }
  FShownReady = IsReady();
  UpdateActions();
  UpdateStatusBar();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::SetDisconnectMessage(const UnicodeString & Message)
{
  ReconnectLabel->setText(ToQString(Message));
  ReconnectLabel->setVisible((FTerminal != NULL) && !IsReady() && !Message.IsEmpty());
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
  FRemoteModel->Load(FTerminal->Files, FShowHiddenFiles, FShowInaccesibleDirectories);
  FilesReloaded();
  UpdateTree();
  Q_EMIT FilesLoaded();
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::UpdateTree()
{
  if (GetTreeVisible())
  {
    FRemoteTree->LoadDirectory();
  }
}
//---------------------------------------------------------------------------
bool TRemoteDirPanel::IsColumnHiddenByDefault(TDirViewColumn Column) const
{
  return (Column == dvGroup) || (Column == dvExt) || (Column == dvLinkTarget) || (Column == dvType);
}
//---------------------------------------------------------------------------
void TRemoteDirPanel::ApplyConfiguration()
{
  TDirPanel::ApplyConfiguration();
  FRemoteTree->SetShowHiddenFiles(FShowHiddenFiles);
  FRemoteTree->SetShowInaccesibleDirectories(AppConfiguration->ShowInaccesibleDirectories);
  if (FShowInaccesibleDirectories != AppConfiguration->ShowInaccesibleDirectories)
  {
    FShowInaccesibleDirectories = AppConfiguration->ShowInaccesibleDirectories;
    if (IsReady())
    {
      LoadFiles();
    }
  }
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
