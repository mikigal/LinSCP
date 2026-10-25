// The file panels of the Commander (upstream's directory views with their
// path combo boxes, toolbars and status bars, forms/ScpCommander.dfm): the
// local panel and the remote panel.
//---------------------------------------------------------------------------
#ifndef DirPanelH
#define DirPanelH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <CopyParam.h>

#include <QWidget>

#include <functional>
#include <vector>
//---------------------------------------------------------------------------
class QAction;
class QComboBox;
class QFileSystemWatcher;
class QLabel;
class QMenu;
class QModelIndex;
class QStackedWidget;
class QTimer;
class QToolBar;
class QTreeView;
class TDirModel;
class TLocalDirModel;
class TRemoteDirModel;
class TRemoteFile;
class TTerminal;
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

Q_SIGNALS:
  // The panel got the focus (it is the current panel)
  void Entered(TDirPanel * Panel);
  // Another directory is shown
  void DirectoryChanged(TDirPanel * Panel);
  // The commands on the bookmarks, done by the main window
  void OpenDirectoryRequested(TDirPanel * Panel);
  void AddBookmarkRequested(TDirPanel * Panel);
  // The menu of the bookmarks is about to show (to be filled)
  void BookmarksMenuRequested(TDirPanel * Panel, QMenu * Menu);

protected:
  QToolBar * PathToolbar;
  QComboBox * PathComboBox;
  QTreeView * DirView;
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

protected:
  virtual void ItemActivated(int Row) override;

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
