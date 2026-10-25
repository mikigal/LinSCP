// The file panels of the Commander (upstream's directory views with their
// path combo boxes, toolbars and status bars, forms/ScpCommander.dfm): the
// local panel and the remote panel.
//---------------------------------------------------------------------------
#ifndef DirPanelH
#define DirPanelH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <AppConfiguration.h>
#include <CopyParam.h>

#include "DirModel.h"

#include <QElapsedTimer>
#include <QTreeView>
#include <QWidget>

#include <functional>
#include <unordered_set>
#include <vector>
//---------------------------------------------------------------------------
class QAction;
class QComboBox;
class QDropEvent;
class QFileSystemWatcher;
class QLabel;
class QMenu;
class QMimeData;
class QModelIndex;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTimer;
class QToolBar;
class TDirModel;
class TDirPanel;
class TLocalDirModel;
class TRemoteDirModel;
class TLocalDirTree;
class TRemoteDirTree;
class TRemoteFile;
class TTerminal;
//---------------------------------------------------------------------------
// How the selection of a panel changes (upstream's TSelectMode)
enum TSelectMode { smAll, smNone, smInvert };
//---------------------------------------------------------------------------
// The files of a panel: dragging the selected files, dropping on the view
// or on its directories (the target, highlighted); the selection by the
// keyboard and the mouse (upstream's TCustomNortonLikeListView)
class TDirView : public QTreeView
{
  Q_OBJECT

public:
  explicit TDirView(TDirPanel * Panel);

  TDirPanel * GetPanel() const { return FPanel; }

  virtual void setModel(QAbstractItemModel * Model) override;
  virtual void keyboardSearch(const QString & Search) override;
  // The keyboard (nlKeyboard) or also the mouse (nlOn) only focus the
  // files, Insert and Space select them
  void SetNortonLike(TNortonLikeMode Value) { FNortonLike = Value; }
  TNortonLikeMode GetNortonLike() const { return FNortonLike; }
  // Toggles the selection of the focused file (Insert)
  void SelectCurrentItem(bool FocusNext);
  // The flags of the selection when the focus moves to another file
  QItemSelectionModel::SelectionFlags FocusCommand() const;

protected:
  virtual QItemSelectionModel::SelectionFlags selectionCommand(
    const QModelIndex & Index, const QEvent * Event = nullptr) const override;
  virtual void keyPressEvent(QKeyEvent * Event) override;
  virtual void mousePressEvent(QMouseEvent * Event) override;
  virtual void mouseMoveEvent(QMouseEvent * Event) override;
  virtual void mouseReleaseEvent(QMouseEvent * Event) override;
  virtual void startDrag(Qt::DropActions SupportedActions) override;
  virtual void dragEnterEvent(QDragEnterEvent * Event) override;
  virtual void dragMoveEvent(QDragMoveEvent * Event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent * Event) override;
  virtual void dropEvent(QDropEvent * Event) override;
  virtual void drawRow(QPainter * Painter, const QStyleOptionViewItem & Option, const QModelIndex & Index) const override;

private:
  TDirPanel * FPanel;
  int FDropTargetRow;
  // The incremental search (upstream's PanelSearch)
  QString FSearchText;
  QElapsedTimer FSearchTimer;
  TNortonLikeMode FNortonLike;
  // All the selected files were selected by clicks (then the keyboard
  // clears the selection in the nlKeyboard mode), while a click selects
  bool FAnyAndAllSelectedImplicitly;
  bool FSelectingImplicitly;
  // The keys do not change the selection
  bool FNoSelectionUpdate;

  bool IsSearching() const;
  void MouseEvent(QMouseEvent * Event, const std::function<void()> & Handler);

  // The directory the files would be dropped to (the parent directory or
  // a directory under the pointer, else the panel's), with its row
  UnicodeString DropTarget(QDropEvent * Event, int & Row);
  void SetDropTargetRow(int Row);
};
//---------------------------------------------------------------------------
// What a drop on a panel would do; the action is Qt::IgnoreAction when the
// files cannot be dropped there
struct TDropParams
{
  const QMimeData * Data;
  // The panel the files are dragged from, or NULL (another application)
  TDirPanel * SourcePanel;
  Qt::DropActions PossibleActions;
  Qt::DropAction ProposedAction;
  Qt::KeyboardModifiers Modifiers;
  UnicodeString TargetDirectory;
  bool OnDirectory;
};
//---------------------------------------------------------------------------
// The state of a panel while another session is shown (upstream's
// TDirViewState): the history and the focused file
class TDirPanelState : public TObject
{
public:
  std::vector<UnicodeString> History;
  int HistoryIndex;
  UnicodeString FocusedFile;
};
//---------------------------------------------------------------------------
class TDirPanel : public QWidget
{
  Q_OBJECT

public:
  TDirPanel(TOperationSide Side, QWidget * Parent);

  TOperationSide GetSide() const { return FSide; }
  QTreeView * GetDirView() const { return DirView; }
  // A directory is shown (the remote panel needs a session)
  virtual bool IsReady() const = 0;
  virtual UnicodeString GetPath() const = 0;
  virtual void ChangeDirectory(const UnicodeString & Path) = 0;
  virtual void Reload() = 0;
  virtual void HomeDirectory() = 0;
  void ParentDirectory();
  void RootDirectory();
  void Back();
  void Forward();

  // The selected files, or the focused file when none is selected; never
  // the parent directory
  std::vector<int> GetSelectedRows() const;
  const TDirModel * GetModel() const { return FModel; }
  void FocusFile(const UnicodeString & FileName);
  // Edits the name of the focused file in place; OnRename renames it
  void StartRename();
  std::function<void(TDirPanel * Panel, const UnicodeString & FileName, const UnicodeString & NewName)> OnRename;
  void SetShowHiddenFiles(bool Value);
  // The preferences of the panels (the font, the sizes, the sorting, ...)
  virtual void ApplyConfiguration();
  void UpdateStatusBar();
  // The visited directories
  const std::vector<UnicodeString> & GetHistory() const { return FHistory; }
  // The full path of the directory of a row (".." is the parent directory)
  UnicodeString RowDirectory(int Row) const;
  // The state of the panel for the session (TDirPanelState); restoring
  // NULL starts a new history
  TObject * SaveState();
  void RestoreState(TObject * State);

  // The selection (upstream's TCustomDirView): the files matching the
  // filter, all the files, none, the inverse, the focused file
  void SelectFiles(const TFileFilter & Filter, bool Select);
  void SelectAll(TSelectMode Mode);
  void SelectCurrentItem();
  // The files selected before an operation, to be selected again
  void SaveSelectedNames();
  void RestoreSelectedNames();
  bool GetSelectedNamesSaved() const { return !FSavedNames.empty(); }
  // The selected files and all the files (without the parent directory)
  int GetSelCount() const;
  int GetFilesCount() const;
  // The focused file, not the parent directory
  bool HasFocusedFile() const;
  UnicodeString GetFocusedFileName() const;
  // Selects the files that are newer than in the other panel or missing
  // there, by the criteria (upstream's CompareFiles)
  void CompareFiles(TDirPanel * Other, bool CompareByTime, bool CompareBySize);
  // Only the files matching the mask are shown (the filter dialog)
  void Filter();
  void SetMask(const UnicodeString & Mask);
  UnicodeString GetMask() const;
  void SetRefreshShortCut(const QKeySequence & ShortCut);
  // The address shows the name of the directory only (upstream's
  // ShowFullAddress of the Explorer)
  void SetShowFullAddress(bool Value);

  // The menu of the panel (upstream's Local and Remote menus: Go To, the
  // bookmarks, Sort, Columns, Filter); the options of the panel (the tree,
  // the status bar)
  void FillMenu(QMenu * Menu, QAction * PathToClipboardAction);
  void FillOptionsMenu(QMenu * Menu);
  // The directory tree (upstream's DriveView), above the files or left of
  // them, its size
  void SetTreeVisible(bool Visible);
  bool GetTreeVisible() const;
  void SetTreeOnLeft(bool Value);
  int GetTreeSize() const;
  void SetTreeSize(int Value);
  void SetStatusBarVisible(bool Visible);
  bool GetStatusBarVisible() const;
  // The sort, the widths, the visibility and the order of the columns, in
  // the format of upstream's DirViewParams
  UnicodeString GetDirViewParams() const;
  void SetDirViewParams(const UnicodeString & Value);
  void ResetLayoutColumns();

  // Drag and drop, done by the main window: a drag of the selected files
  // starts; the action of a drop of the data on the panel; the files are
  // dropped
  std::function<void(TDirPanel * Panel)> OnStartDrag;
  std::function<Qt::DropAction(TDirPanel * Panel, const TDropParams & Params)> OnDragOver;
  std::function<void(TDirPanel * Panel, const TDropParams & Params, Qt::DropAction Action)> OnDrop;

Q_SIGNALS:
  // The panel got the focus (it is the current panel)
  void Entered(TDirPanel * Panel);
  // Another directory is shown
  void DirectoryChanged(TDirPanel * Panel);
  // A file (not a directory) was double-clicked or entered
  void FileActivated(TDirPanel * Panel);
  // The commands on the bookmarks, done by the main window
  void OpenDirectoryRequested(TDirPanel * Panel);
  void AddBookmarkRequested(TDirPanel * Panel);
  // The menu of the bookmarks is about to show (to be filled)
  void BookmarksMenuRequested(TDirPanel * Panel, QMenu * Menu);
  // The count of the hidden files in the status bar was clicked
  void ToggleHiddenFilesRequested();

protected:
  QToolBar * PathToolbar;
  QComboBox * PathComboBox;
  TDirView * DirView;
  QLabel * StatusBar;
  QLabel * HiddenFilesLabel;
  QLabel * FilteredFilesLabel;
  QAction * BackAction;
  QAction * ForwardAction;
  QAction * ParentDirAction;
  QAction * RootDirAction;
  QAction * HomeDirAction;
  QAction * RefreshAction;
  QAction * OpenDirAction;
  QAction * AddBookmarkAction2;
  QAction * FilterAction;
  QAction * TreeAction;
  QAction * StatusBarAction;
  QAction * SortAscendingAction;
  QMenu * OpenDirMenu;
  QMenu * SortMenu;
  QMenu * ColumnsMenu;
  QSplitter * FViewSplitter;
  QWidget * FTree;
  QWidget * FStatusPanel;
  TDirModel * FModel;
  bool FShowHiddenFiles;

  void SetModel(TDirModel * Model);
  // The tree of the panel, in the splitter above the files
  void InitTree(QWidget * Tree);
  // The tree shows the directory
  virtual void UpdateTree() = 0;
  virtual bool IsColumnHiddenByDefault(TDirViewColumn Column) const = 0;
  // The directory changed: the history, the path box, the focus
  void PathChanged();
  // The files were reloaded: the focus is kept
  void FilesReloaded();
  void SaveFocus();
  virtual void ItemActivated(int Row);
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;
  void UpdateActions();

private:
  TOperationSide FSide;
  std::vector<UnicodeString> FHistory;
  int FHistoryIndex;
  bool FNavigatingHistory;
  UnicodeString FPrevPath;
  UnicodeString FFocusAfterReload;
  // The selected files of a reload of the directory
  std::unordered_set<UnicodeString> FSelectionAfterReload;
  std::unordered_set<UnicodeString> FSavedNames;
  bool FUpdatingPath;
  bool FShowFullAddress;
  UnicodeString FFileColors;

  QAction * CreateAction(const QString & Name, const QString & Caption, const QString & IconName,
    const QKeySequence & ShortCut, void (TDirPanel::*Method)());
  void PathComboActivated();
  // Selects or unselects the rows
  void SelectRows(const std::vector<int> & Rows, bool Select);
  void CreateSortMenu(const QString & Prefix);
  void CreateColumnsMenu(const QString & Prefix);
  void UpdateSortMenu();
  void UpdateColumnsMenu();
  int DefaultColumnWidth(int Column) const;
  void RefreshActionExecute() { Reload(); }
  void HomeDirActionExecute() { HomeDirectory(); }
  void OpenDirActionExecute() { Q_EMIT OpenDirectoryRequested(this); }
  void AddBookmarkActionExecute() { Q_EMIT AddBookmarkRequested(this); }
};
//---------------------------------------------------------------------------
class TLocalDirPanel : public TDirPanel
{
  Q_OBJECT

public:
  explicit TLocalDirPanel(QWidget * Parent);

  virtual bool IsReady() const override { return true; }
  virtual UnicodeString GetPath() const override;
  virtual void ChangeDirectory(const UnicodeString & Path) override;
  virtual void Reload() override;
  virtual void HomeDirectory() override;
  // The directory of the Home command, the user's home when empty
  void SetHomeDirectory(const UnicodeString & Value) { FHomeDirectory = Value; }
  virtual void ApplyConfiguration() override;

protected:
  virtual bool IsColumnHiddenByDefault(TDirViewColumn Column) const override;
  virtual void UpdateTree() override;

private:
  TLocalDirModel * FLocalModel;
  TLocalDirTree * FLocalTree;
  UnicodeString FHomeDirectory;
  QFileSystemWatcher * FWatcher;
  QTimer * FReloadTimer;

  void Load(const UnicodeString & Path);
};
//---------------------------------------------------------------------------
class TRemoteDirPanel : public TDirPanel
{
  Q_OBJECT

public:
  explicit TRemoteDirPanel(QWidget * Parent);

  // The terminal whose directory is shown, or NULL (or not connected)
  void SetTerminal(TTerminal * Terminal);
  TTerminal * GetTerminal() const { return FTerminal; }
  virtual bool IsReady() const override;
  virtual UnicodeString GetPath() const override;
  virtual void ChangeDirectory(const UnicodeString & Path) override;
  virtual void Reload() override;
  virtual void HomeDirectory() override;
  TRemoteFile * GetFile(int Row) const;

  // Why the session (shown disconnected) was disconnected
  void SetDisconnectMessage(const UnicodeString & Message);
  virtual void ApplyConfiguration() override;

protected:
  virtual bool IsColumnHiddenByDefault(TDirViewColumn Column) const override;
  virtual void UpdateTree() override;

Q_SIGNALS:
  void NewSessionRequested();
  void ReconnectRequested();
  // The files of the directory were read
  void FilesLoaded();

private:
  TRemoteDirModel * FRemoteModel;
  TRemoteDirTree * FRemoteTree;
  TTerminal * FTerminal;
  bool FShownReady;
  bool FShowInaccesibleDirectories;
  QStackedWidget * FStack;
  QLabel * FNoSessionLabel;
  QLabel * ReconnectLabel;
  QPushButton * FNewSessionButton;
  QPushButton * ReconnectItem;

  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void LoadFiles();
};
//---------------------------------------------------------------------------
#endif
