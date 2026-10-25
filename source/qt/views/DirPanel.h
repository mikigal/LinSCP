// The file panels of the Commander (upstream's directory views with their
// path combo boxes, toolbars and status bars, forms/ScpCommander.dfm): the
// local panel and the remote panel.
//---------------------------------------------------------------------------
#ifndef DirPanelH
#define DirPanelH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <CopyParam.h>

#include <QTreeView>
#include <QWidget>

#include <functional>
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
class QStackedWidget;
class QTimer;
class QToolBar;
class TDirModel;
class TDirPanel;
class TLocalDirModel;
class TRemoteDirModel;
class TRemoteFile;
class TTerminal;
//---------------------------------------------------------------------------
// The files of a panel: dragging the selected files, dropping on the view
// or on its directories (the target, highlighted)
class TDirView : public QTreeView
{
  Q_OBJECT

public:
  explicit TDirView(TDirPanel * Panel);

  TDirPanel * GetPanel() const { return FPanel; }

protected:
  virtual void startDrag(Qt::DropActions SupportedActions) override;
  virtual void dragEnterEvent(QDragEnterEvent * Event) override;
  virtual void dragMoveEvent(QDragMoveEvent * Event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent * Event) override;
  virtual void dropEvent(QDropEvent * Event) override;
  virtual void drawRow(QPainter * Painter, const QStyleOptionViewItem & Option, const QModelIndex & Index) const override;

private:
  TDirPanel * FPanel;
  int FDropTargetRow;

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
  void UpdateStatusBar();
  // The visited directories
  const std::vector<UnicodeString> & GetHistory() const { return FHistory; }
  // The full path of the directory of a row (".." is the parent directory)
  UnicodeString RowDirectory(int Row) const;

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

protected:
  QToolBar * PathToolbar;
  QComboBox * PathComboBox;
  TDirView * DirView;
  QLabel * StatusBar;
  QAction * BackAction;
  QAction * ForwardAction;
  QAction * ParentDirAction;
  QAction * RootDirAction;
  QAction * HomeDirAction;
  QAction * RefreshAction;
  QAction * OpenDirAction;
  QAction * AddBookmarkAction2;
  QMenu * OpenDirMenu;
  TDirModel * FModel;
  bool FShowHiddenFiles;

  void SetModel(TDirModel * Model);
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
  bool FUpdatingPath;

  QAction * CreateAction(const QString & Name, const QString & Caption, const QString & IconName,
    const QKeySequence & ShortCut, void (TDirPanel::*Method)());
  void PathComboActivated();
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

private:
  TLocalDirModel * FLocalModel;
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

Q_SIGNALS:
  void NewSessionRequested();

private:
  TRemoteDirModel * FRemoteModel;
  TTerminal * FTerminal;
  QStackedWidget * FStack;

  void TerminalChangeDirectory(TObject * Sender);
  void TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void LoadFiles();
};
//---------------------------------------------------------------------------
#endif
