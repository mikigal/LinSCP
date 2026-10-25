// The main window with the Commander interface (upstream's
// TScpCommanderForm, forms/ScpCommander.dfm and CustomScpExplorer.dfm, with
// the actions of forms/NonVisual.dfm): the local and the remote panel, the
// menus and toolbars, and the commands on the files of the current panel.
//---------------------------------------------------------------------------
#ifndef ScpCommanderH
#define ScpCommanderH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <AppConfiguration.h>
#include <CustomCommands.h>
#include <FileOperationProgress.h>
#include <Queue.h>

#include "QueueController.h"

#include <Notifications.h>

#include <QMainWindow>
#include <QPointer>
#include <QStringList>

#include <mutex>
//---------------------------------------------------------------------------
class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QMenu;
class QMimeData;
class QSplitter;
class QSystemTrayIcon;
class QUrl;
class QTimer;
class QToolBar;
class QTabBar;
class QTreeWidget;
class TBookmark;
class TEditorManager;
struct TEditedFileData;
struct TEditorData;
class TBookmarkList;
class TDirPanel;
struct TDropParams;
class TLocalDirPanel;
class TManagedTerminal;
class TOptions;
class TPasteWatcher;
class TProgressForm;
class TRemoteDirPanel;
class TSynchronizeProgressForm;
class TTaskbarProgress;
class TTerminal;
//---------------------------------------------------------------------------
// How a transfer was started (upstream's TTransferOperationParam)
struct TTransferOperationParam
{
  // The target, else the directory of the other panel
  UnicodeString TargetDirectory;
  // By drag and drop or by pasting (DDTransferConfirmation)
  bool DragDrop = false;
  // Without the copy dialog (a double-click)
  bool NoConfirmation = false;
};
//---------------------------------------------------------------------------
class TScpCommanderForm : public QMainWindow
{
  Q_OBJECT

public:
  explicit TScpCommanderForm(QWidget * Parent = nullptr);
  virtual ~TScpCommanderForm();

  void UpdateCaption();
  // Opens a session selected in the site manager
  void NewSession();
  // The startup by the command line (upstream's Execute of WinMain.cpp):
  // opens its session (a site, a URL, a folder of sites) and runs its
  // operation, else the site manager (ScpCommanderCommandLine.cpp)
  void ExecuteCommandLine(TOptions * Params);
  // The command line of another instance: false when it cannot be taken
  // now, its session is opened later
  bool CommandLineFromAnotherInstance(const QStringList & Arguments, const QString & ActivationToken);
  // Minimizes the application (upstream's ApplicationMinimize), to the
  // system tray when asked or configured (ScpCommanderDesktop.cpp)
  void MinimizeApplication(bool ToTray);
  void RestoreApplication();
  static bool IsSystemTrayAvailable();
  // The session shown (upstream's ManagedSession), NULL while the active
  // one connects
  TManagedTerminal * GetManagedSession() const { return FManagedSession; }
  void SetManagedSession(TManagedTerminal * Value);
  // The transfer queue of the session shown
  void SetQueue(TTerminalQueue * Value);
  // The active session is ready (connected or disconnected in its tab)
  void SessionReady();
  // The sessions were opened, closed or reordered (ScpCommanderSessions.cpp)
  void SessionListChanged();
  // A session (connected or lost in the background)
  void TerminalStatusChanged(TManagedTerminal * Terminal);
  // The state of the panels goes to the session (its directories, ...)
  void UpdateSession(TManagedTerminal * Session);
  // The session is being freed
  void TerminalRemoved(TTerminal * Terminal);
  // The active session was disconnected, its tab stays
  void TerminalDisconnected();
  void LastTerminalClosed();
  // A note in the status bar, for the seconds (the timeout of the
  // notifications by default)
  void PostNote(const UnicodeString & Note, unsigned int Seconds = 0);
  void CancelNote(const UnicodeString & Note);
  // A notification about a session in the background (its error, ...)
  void InactiveTerminalNotify(TManagedTerminal * Terminal, const UnicodeString & Message, TQueryType Type,
    Exception * E = NULL);
  void OperationProgress(TFileOperationProgressType & ProgressData);
  void OperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
    TOnceDoneOperation & OnceDoneOperation);
  // An operation on files runs
  bool IsBusy() const { return (FBusy > 0); }
  // The configuration changed (the presets, ...)
  void ConfigurationChanged();
  // An event of the queue of the session (TTerminalManager::Idle)
  void QueueEvent(TTerminal * Terminal, TTerminalQueue * Queue, TQueueEvent Event);
  // A file of the editors was uploaded by the queue (called by its thread)
  void EditedFileUploaded(TTerminal * ATerminal, HANDLE UploadCompleteEvent);
  // The queries of the sessions; the errors are only logged when keeping
  // the remote directory up to date continues on errors
  unsigned int MoreMessageDialog(const UnicodeString & Message, TStrings * MoreMessages,
    TQueryType Type, unsigned int Answers, const TMessageParams * Params);

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void changeEvent(QEvent * Event) override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  // The session shown, NULL while the active one connects
  TManagedTerminal * FManagedSession;
  // The Commander or the Explorer (only the remote panel)
  TInterface FInterface;
  QMenu * FLocalMenu;
  QMenu * FMarkMenu;
  QMenu * FFilesMenu;
  QMenu * FCommandsMenu;
  QMenu * FTabsMenu;
  QMenu * FOptionsMenu;
  QMenu * FRemoteMenu;
  QMenu * FExplorerFilesMenu;
  QMenu * FExplorerCommandsMenu;
  QMenu * FViewMenu;
  QToolBar * FCommandsToolbar;
  QToolBar * FButtonsToolbar;
  QToolBar * FHotkeysToolbar;
  // The tabs of the sessions and the "new tab" tab (ScpCommanderSessions.cpp)
  QTabBar * SessionsPageControl;
  bool FUpdatingSessionTabs;
  int FNewSessionNesting;
  // Its local directory is shown regardless of PreserveLocalDirectory
  bool FFirstTerminal;
  UnicodeString FNote;
  QSplitter * FPanelSplitter;
  TLocalDirPanel * FLocalPanel;
  TRemoteDirPanel * FRemotePanel;
  TDirPanel * FCurrentPanel;
  TProgressForm * FProgressForm;
  TSynchronizeProgressForm * FSynchronizeProgressForm;
  QLabel * FSessionStatusLabel;
  QComboBox * FTransferList;
  int FBusy;
  int FUpdatingTransferList;
  // The preset selected by the user, and the one selected by its rule
  UnicodeString FCopyParamDefault;
  UnicodeString FCopyParamAutoSelected;
  QTimer * FIdleTimer;
  // The operations on files report their files to it (the file find dialog)
  TFileOperationFinishedEvent FOnFileOperationFinished;
  TTerminal * FFileFindTerminal;
  bool FRefreshLocalDirectory;
  bool FRefreshRemoteDirectory;
  // An operation started by the program, not by the user (keeping the
  // remote directory up to date)
  bool FAutoOperation;
  // While the remote directory is kept up to date
  TSynchronizeController * FSynchronizeController;
  TTerminal * FSynchronizeTerminal;
  TFeedSynchronizeError FOnFeedSynchronizeError;
  std::function<void()> FOnSynchronizeAbort;
  // The shortcuts of the shared bookmarks
  std::vector<QAction *> FBookmarkShortCutActions;
  // The custom commands
  TCustomCommandType FLastCustomCommand;
  std::unique_ptr<TStrings> FCapturedLog;
  std::vector<QAction *> FCustomCommandShortCutActions;
  // The opened and edited files
  std::unique_ptr<TEditorManager> FEditorManager;
  std::vector<QWidget *> FLocalEditors;
  // The files of the drag from a panel (full paths, the remote files with
  // their TRemoteFile objects), while it may be dropped
  std::unique_ptr<TStrings> FDragFileList;
  TDirPanel * FDragSourcePanel;
  bool FDragging;
  // A drop on a panel during a drag of ours, done once the drag ends
  std::function<void()> FPendingDropOperation;
  // The directory the target of a drag from the remote panel told
  QString FDragExtractDirectory;
  bool FDragExtracted;
  // The temporary directories of the files downloaded for other
  // applications, deleted after a while
  std::vector<UnicodeString> FDelayedDeletionList;
  // The desktop integration (ScpCommanderDesktop.cpp)
  std::unique_ptr<TNotifications> FNotifications;
  std::unique_ptr<TTaskbarProgress> FTaskbarProgress;
  QSystemTrayIcon * FTrayIcon;
  // The windows hidden to the system tray or minimized with the application
  std::vector<QPointer<QWidget>> FHiddenWindows;
  std::vector<QPointer<QWidget>> FMinimizedWindows;
  // The remote files copied to the clipboard, of the session
  std::unique_ptr<TStrings> FClipboardFileList;
  TTerminal * FClipboardTerminal;
  // The fake directory of the clipboard, found where it is pasted
  UnicodeString FClipboardFakeDirectory;
  std::unique_ptr<TPasteWatcher> FPasteWatcher;

  // The transfer queue (ScpCommanderQueue.cpp)
  QSplitter * FQueueSplitter;
  QWidget * QueuePanel;
  QLabel * QueueLabel;
  QToolBar * QueueToolbar;
  QTreeWidget * QueueView3;
  TQueueController * FQueueController;
  TTerminalQueue * FQueue;
  TTerminalQueueStatus * FQueueStatus;
  // The status is updated from the threads of the queue
  std::mutex FQueueStatusSection;
  bool FQueueStatusInvalidated;
  bool FQueueItemInvalidated;
  bool FQueueStatusUpdating;
  TQueueItemProxy * FPendingQueueActionItem;
  QActionGroup * FQueueShowGroup;
  QActionGroup * FQueueOnceEmptyGroup;

  QAction * NewSessionAction;
  QAction * CloseTabAction;
  QAction * DuplicateTabAction;
  QAction * RenameTabAction;
  QAction * NextTabAction;
  QAction * PreviousTabAction;
  QAction * DisconnectSessionAction;
  QAction * ReconnectSessionAction;
  QAction * SaveCurrentSessionAction2;
  QAction * ColorMenuAction2;
  QAction * FileSystemInfoAction;
  QAction * ChangePasswordAction;
  QAction * PrivateKeyUploadAction;
  QAction * ClearCachesAction;
  QAction * SaveWorkspaceAction;
  QAction * SiteManagerAction;
  QAction * SessionsTabsAction2;
  QAction * PreferencesAction;
  // Alt+1..9, Alt+0 activate the tabs (upstream's OpenSessionShortCut)
  std::vector<QAction *> FOpenedTabActions;
  QAction * CloseApplicationAction;
  QAction * CurrentCopyAction;
  QAction * CurrentMoveAction;
  QAction * RemoteCopyToAction;
  QAction * RemoteMoveToAction;
  QAction * CurrentRenameAction;
  QAction * CurrentDeleteAction;
  QAction * CurrentDeleteAlternativeAction;
  QAction * CurrentCreateDirAction;
  QAction * CurrentPropertiesAction;
  QAction * CurrentCopyToClipboardAction2;
  QAction * PasteAction3;
  QAction * FileListToClipboardAction;
  QAction * FullFileListToClipboardAction;
  QAction * LocalPathToClipboardAction2;
  QAction * RemotePathToClipboardAction2;
  QAction * RemoteFindFilesAction2;
  QAction * FullSynchronizeAction2;
  QAction * SynchronizeAction;
  QAction * ConsoleAction;
  QAction * PuttyAction;
  QAction * CurrentEditAction;
  QAction * CurrentEditInternalAction;
  QAction * CurrentOpenAction;
  QAction * NewFileAction;
  QAction * CustomCommandsEnterAction;
  QAction * CustomCommandsLastAction;
  QAction * CustomCommandsCustomizeAction;
  QAction * ShowHiddenFilesAction;
  // The Mark menu (ScpCommanderMark.cpp)
  QAction * SelectOneAction;
  QAction * SelectAction;
  QAction * UnselectAction;
  QAction * SelectAllAction;
  QAction * InvertSelectionAction;
  QAction * ClearSelectionAction;
  QAction * RestoreSelectionAction;
  QAction * SelectSameExtAction;
  QAction * UnselectSameExtAction;
  QAction * CompareDirectoriesAction2;
  QAction * QueueEnableAction;
  QAction * QueueGoToAction;
  QAction * QueueItemQueryAction;
  QAction * QueueItemErrorAction;
  QAction * QueueItemPromptAction;
  QAction * QueueItemExecuteAction;
  QAction * QueueItemPauseAction;
  QAction * QueueItemResumeAction;
  QAction * QueueItemDeleteAction;
  QAction * QueueItemUpAction;
  QAction * QueueItemDownAction;
  QAction * QueuePauseAllAction;
  QAction * QueueResumeAllAction;
  QAction * QueueDeleteAllDoneAction;
  QAction * QueueDeleteAllAction;
  QAction * QueueIdleOnceEmptyAction;
  QAction * QueueDisconnectOnceEmptyAction2;
  QAction * QueueSuspendOnceEmptyAction2;
  QAction * QueueShutDownOnceEmptyAction2;
  QAction * QueueShowAction;
  QAction * QueueHideWhenEmptyAction;
  QAction * QueueHideAction;

  QAction * CreateAction(const char * Name, const QString & Caption, const QString & IconName,
    const QList<QKeySequence> & ShortCuts, void (TScpCommanderForm::*Method)());
  void CreateMenus();
  // The menus of the interface in the menu bar
  void ArrangeMenus();
  void CreateToolbars();
  void UpdateActions();
  void UpdateTransferList();
  void TransferListChange();
  void TransferPresetAutoSelect();
  void SetCurrentPanel(TDirPanel * Panel);
  TDirPanel * OtherPanel(TDirPanel * Panel) const;
  TDirPanel * Panel(TOperationSide Side) const;
  TTerminal * GetTerminal() const;
  UnicodeString PathForCaption();
  bool IsExplorer() const { return (FInterface == ifExplorer); }
  // The layout, the menus, the toolbars and the keys of the interface
  void ApplyInterface();
  // The local directory of the Explorer's downloads and synchronizations,
  // the directory of the local panel of the Commander
  UnicodeString LocalDirectory();
  // The order of the panels (SwappedPanels)
  void UpdatePanelsLayout();
  // The layout of the panels (their columns, trees, status bars)
  void RestorePanelsParams();
  void StorePanelsParams();
  TPanelConfiguration PanelParams(TDirPanel * Panel);
  // The keys of the Commander or of the Explorer (ExplorerKeyboardShortcuts)
  void UpdateKeyboardShortcuts();
  // The operation took long enough to beep (BeepOnFinish)
  void OperationComplete(const TDateTime & StartTime);
  // The selected files of the panel (or of the row): full paths (and the
  // remote files)
  TStrings * CreateFileList(TDirPanel * Panel, bool FullPath = true, int Row = -1);
  void DestroyProgressForm();
  void CreateProgressForm(TSynchronizeProgress * SynchronizeProgress);
  void Idle();

  // The transfer queue (ScpCommanderQueue.cpp)
  void CreateQueueActions();
  void CreateQueuePanel();
  QMenu * CreateQueueMenu();
  void QueueListUpdate(TTerminalQueue * Queue);
  void QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item);
  void UpdateQueueStatus(bool QueueChanging);
  void QueueChanged();
  void UpdateQueueView();
  void UpdateQueueLabel();
  void UpdateQueueActions();
  void RefreshQueueItems();
  void UserActionTimer();
  bool IsAnythingQueued();
  // The queue is empty, or its items may be abandoned
  bool CanCloseQueue();
  void AddQueueItem(TTransferDirection Direction, TStrings * FileList,
    const UnicodeString & TargetDirectory, TGUICopyParamType & CopyParam, int Params);
  bool AllowQueueOperation(TQueueOperation Operation);
  void ExecuteQueueOperation(TQueueOperation Operation);
  TOnceDoneOperation CurrentQueueOnceEmptyOperation();
  void ResetQueueOnceEmptyOperation();
  void QueueViewShowChange(TQueueViewShow Show);

  // The sessions and their tabs (ScpCommanderSessions.cpp)
  void CreateSessionsPageControl();
  // The site manager when there is no session (on startup, once the last
  // one closed), as configured
  void NeedSession(bool Startup);
  // The protocol of the session in the status bar, the reason of its
  // disconnection in the remote panel
  void UpdateSessionStatus();
  // Tab switches the panels
  bool EventFilterPanels(QObject * Object, QEvent * Event);
  void SessionChanging();
  void SessionChanged();
  // The state of the session for its tab and the workspaces (its
  // directories, its name)
  void UpdateSessionData(TSessionData * Data);
  void RestoreSessionLocalDirectory(const UnicodeString & LocalDirectory);
  // The session of the tab, NULL for the "new tab" tab
  TManagedTerminal * GetSessionTabSession(int Index);
  void UpdateSessionTab(int Index);
  void SessionTabSwitched();
  void CloseSessionTab(int Index);
  bool CanCloseQueue(TTerminalQueue * Queue);
  // The session's things that cannot outlive it (the file find dialog, the
  // clipboard, keeping its directory up to date)
  void DetachTerminal(TTerminal * ATerminal);
  TSessionData * CloneCurrentSessionData();
  void CreateOpenedTabsMenu(QMenu * Menu);
  // The workspaces and the sites (ScpCommanderSessions.cpp)
  UnicodeString WorkspaceName();
  TObjectList * DoCollectWorkspace();
  void DoSaveWorkspace(const UnicodeString & Name, TObjectList * DataList, bool SavePasswords, bool Explicit);
  bool SaveWorkspace(bool EnableAutoSave);
  void OpenStoredSession(TSessionData * Data);
  void OpenFolderOrWorkspace(const UnicodeString & Name);
  void DoOpenFolderOrWorkspace(const UnicodeString & Name, bool ConnectFirstTerminal, bool CheckMaxSessions);
  void CreateWorkspacesMenu(QMenu * Menu);
  // The sites of a folder (the index of its first site and its level),
  // the folders in submenus
  void CreateSessionListMenuLevel(QMenu * Menu, int Index, int Level);
  // The sessions can be closed with the application (confirmed, their
  // queues abandoned), upstream's FormCloseQuery
  bool CanCloseSessions();
  // The workspace is saved automatically (upstream's FormClose)
  void SessionsClosing();
  void SaveWorkspaceActionExecute();
  void SessionsTabsActionExecute();
  void PreferencesActionExecute();
  void OpenedTabActionExecute(int Index);
  void NewSessionActionExecute() { NewSession(); }
  void CloseTabActionExecute();
  void DuplicateTabActionExecute();
  void RenameTabActionExecute();
  void NextTabActionExecute();
  void PreviousTabActionExecute();
  void DisconnectSessionActionExecute();
  void ReconnectSessionActionExecute();
  void SaveCurrentSessionActionExecute();
  // The color of the current tab (upstream's SessionColor)
  void ColorMenuActionExecute();
  void FileSystemInfoActionExecute();
  bool CanChangePassword();
  void ChangePasswordActionExecute();
  bool CanPrivateKeyUpload();
  void PrivateKeyUploadActionExecute();
  void ClearCachesActionExecute();
  void CloseApplicationActionExecute();
  void CurrentCopyActionExecute() { ExecuteCopyMoveFileOperation(foCopy); }
  void CurrentMoveActionExecute() { ExecuteCopyMoveFileOperation(foMove); }
  void RemoteCopyToActionExecute() { ExecuteRemoteTransfer(false); }
  void RemoteMoveToActionExecute() { ExecuteRemoteTransfer(true); }
  void CurrentRenameActionExecute();
  void CurrentDeleteActionExecute() { DeleteFiles(false); }
  void CurrentDeleteAlternativeActionExecute() { DeleteFiles(true); }
  void CurrentCreateDirActionExecute();
  void CurrentPropertiesActionExecute();
  void RemoteFindFilesActionExecute();
  void FullSynchronizeActionExecute();
  void SynchronizeActionExecute();
  void ConsoleActionExecute() { OpenConsole(); }
  void PuttyActionExecute();
  bool CanConsole();
  void OpenConsole(const UnicodeString & Command = UnicodeString());

  // The selection of the files (ScpCommanderMark.cpp)
  void CreateMarkActions();
  QMenu * CreateMarkMenu();
  void UpdateMarkActions();
  void SelectOneActionExecute();
  void SelectActionExecute() { SelectByMask(true); }
  void UnselectActionExecute() { SelectByMask(false); }
  void SelectAllActionExecute();
  void InvertSelectionActionExecute();
  void ClearSelectionActionExecute();
  void RestoreSelectionActionExecute();
  void SelectSameExtActionExecute() { SelectSameExt(true); }
  void UnselectSameExtActionExecute() { SelectSameExt(false); }
  void SelectByMask(bool Select);
  void SelectSameExt(bool Select);
  // Selects the files that differ between the panels
  void CompareDirectories();

  // The synchronization (ScpCommanderSynchronize.cpp)
  struct TSynchronizeParams
  {
    TSynchronizeMode Mode;
    const TCopyParamType * CopyParam;
    int Params;
    TSynchronizeChecklist * Checklist;
    TDateTime CollectElapsed;
    TDateTime * StartTime;
    TProcessedSynchronizationChecklistItem OnProcessedItem;
  };
  int DoFullSynchronizeDirectories(UnicodeString & Directory1, UnicodeString & Directory2,
    TSynchronizeMode & Mode, int Params, bool & SaveMode);
  void FullSynchronize(TSynchronizeParams & Params, TProcessedSynchronizationChecklistItem OnProcessedItem,
    TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems);
  void SynchronizeProcessedItem(void * Token, const TSynchronizeChecklist::TItem * ChecklistItem);
  void DoQueueSynchronize(void * Token);
  void DoSynchronizeChecklistCalculateSize(
    TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items, void * Token);
  void TerminalSynchronizeDirectory(const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    bool & Continue, bool Collect, const TSynchronizeOptions * Options);
  bool SynchronizeAllowSelectedOnly();
  // Keeping the remote directory up to date
  bool DoSynchronizeDirectories(UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory);
  void DoSynchronize(TSynchronizeController * Sender, const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
    const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
    TSynchronizeOptions * Options, bool Full);
  void DoSynchronizeInvalid(TSynchronizeController * Sender, const UnicodeString Directory,
    const UnicodeString ErrorStr);
  void DoSynchronizeTooManyDirectories(TSynchronizeController * Sender, int & MaxDirectories);
  void Synchronize(const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
    TSynchronizeMode Mode, const TCopyParamType & CopyParam, int Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options);
  void GetSynchronizeOptions(int Params, TSynchronizeOptions & Options);
  void DoFindFiles(TTerminal * ATerminal, UnicodeString Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void DoFocusRemotePath(TTerminal * ATerminal, const UnicodeString & Path);
  bool CanOperateOnFoundFiles(TTerminal * ATerminal);
  void ShowHiddenFilesActionExecute();
  // The bookmarks and location profiles (ScpCommanderBookmarks.cpp)
  void OpenDirectory(TOperationSide Side);
  void AddBookmark(TOperationSide Side);
  TStrings * CreateVisitedDirectories(TOperationSide Side);
  void DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side);
  void DoOpenDirectoryDialogBookmarks(TOpenDirectoryMode Mode, TOperationSide Side);
  bool TryOpenDirectory(TOperationSide Side, const UnicodeString & Path);
  void DoOpenBookmark(UnicodeString Local, UnicodeString Remote);
  bool OpenBookmark(TOperationSide Side, TBookmark * Bookmark);
  void OpenBookmarkByKey(TOperationSide Side, bool Shared, const UnicodeString & Node, const UnicodeString & Name);
  void CreateOpenDirMenuList(QMenu * Menu, TOperationSide Side, TBookmarkList * BookmarkList, bool Shared);
  void CreateOpenDirMenu(TDirPanel * APanel, QMenu * Menu);
  void UpdateBookmarkShortCuts();
  // The custom commands (ScpCommanderCustomCommands.cpp)
  bool CustomCommandRemoteAllowed();
  bool AnyFileSelected(TDirPanel * APanel);
  int CustomCommandState(const TCustomCommandType & Command, TCustomCommandListType ListType);
  void TerminalCaptureLog(const UnicodeString & AddedLine, TCaptureOutputType OutputType);
  void RemoteCustomCommand(TStrings * FileList, const TCustomCommandType & ACommand,
    const TCustomCommandData & Data, const UnicodeString & CommandCommand);
  UnicodeString DefaultDownloadTargetDirectory();
  void LocalCustomCommandPure(TStrings * FileList, const TCustomCommandType & ACommand, const UnicodeString & Command,
    TStrings * ALocalFileList, const TCustomCommandData & Data, bool LocalFileCommand, bool FileListCommand,
    UnicodeString * POutput);
  void LocalCustomCommandWithLocalFiles(const TCustomCommandType & ACommand, const UnicodeString & Command,
    const TCustomCommandData & Data, bool FileListCommand, UnicodeString * POutput);
  void LocalCustomCommand(TStrings * FileList, const TCustomCommandType & ACommand, TStrings * ALocalFileList,
    const TCustomCommandData & Data, const UnicodeString & CommandCommand);
  void CustomCommand(TStrings * FileList, const TCustomCommandType & ACommand, TStrings * ALocalFileList);
  void ExecuteCustomCommand(const TCustomCommandType & Command);
  void AdHocCustomCommand();
  void LastCustomCommand();
  void CustomCommandsCustomize();
  void CreateCustomCommandsMenu(QMenu * Menu);
  void UpdateCustomCommandShortCuts();
  // The opening and editing of files (ScpCommanderEditor.cpp)
  enum TExecuteFileBy { efShell, efInternalEditor, efExternalEditor, efDefaultEditor };
  void CreateEditorManager();
  bool RemoteExecuteForceText(TExecuteFileBy ExecuteFileBy, const TEditorData * ExternalEditor);
  void TemporaryDirectoryForRemoteFiles(const UnicodeString & RemoteDirectory, const TCopyParamType & CopyParam,
    UnicodeString & Result, UnicodeString & RootDirectory);
  void ExecuteFileNormalize(TExecuteFileBy & ExecuteFileBy, const TEditorData *& ExternalEditor,
    const UnicodeString & FileName, bool Local, const TFileMasks::TParams & MaskParams);
  void CustomExecuteFile(TOperationSide Side, TExecuteFileBy ExecuteFileBy, UnicodeString FileName,
    UnicodeString OriginalFileName, const TEditorData * ExternalEditor, UnicodeString LocalRootDirectory,
    UnicodeString RemoteDirectory, bool NewFile, const TDateTime & SourceTimestamp);
  void ExecuteFile(TOperationSide Side, TExecuteFileBy ExecuteFileBy, const TEditorData * ExternalEditor,
    UnicodeString FullFileName, TRemoteFile * File, const TFileMasks::TParams & MaskParams);
  // The selected files (or the focused one) of the panel
  void ExecuteFile(TOperationSide Side, TExecuteFileBy ExecuteFileBy, const TEditorData * ExternalEditor,
    bool AllSelected);
  void EditNew(TOperationSide Side);
  void ExecutedFileChanged(const UnicodeString & FileName, const TDateTime & Timestamp, TEditedFileData * Data,
    HANDLE UploadCompleteEvent, bool & Retry);
  void ExecutedFileReload(const UnicodeString & FileName, TEditedFileData * Data);
  void ExecutedFileEarlyClosed(const TEditedFileData * Data, bool & KeepOpen);
  void SaveAllInternalEditors();
  void AnyInternalEditorModified(bool & Modified);
  void LocalEditorClosed(QWidget * Editor);
  // The editors are closed before the window is (asked to save their files)
  bool CloseEditors();
  void ForceCloseEditors();
  // The edited files of the session lose it (upstream's FileTerminalRemoved)
  void EditedFilesTerminalRemoved(TTerminal * ATerminal);
  void FileActivated(TDirPanel * APanel);
  void DoEditFoundFiles(TTerminal * ATerminal, TStrings * FileList, TFileOperationFinishedEvent OnFileOperationFinished);
  void CurrentEditActionExecute();
  void CurrentEditInternalActionExecute();
  void CurrentOpenActionExecute();
  void NewFileActionExecute();
  void DeleteFiles(bool Alternative);
  bool DeleteFiles(TOperationSide Side, TStrings * FileList, bool Alternative);
  void ExecuteCopyMoveFileOperation(TFileOperation Operation);
  bool ExecuteCopyMoveFileOperation(TFileOperation Operation, TOperationSide Side, TStrings * FileList,
    const TTransferOperationParam & Param = TTransferOperationParam());
  bool CopyParamDialog(TTransferDirection Direction, TTransferType Type,
    TStrings * FileList, UnicodeString & TargetDirectory, TGUICopyParamType & CopyParam, bool Confirm,
    bool DragDrop = false);
  // The "do not show again" of the copy dialog (upstream's
  // HandleDoNotShowCopyDialogAgain)
  void HandleDoNotShowCopyDialogAgain(bool DragDrop, bool DoNotShowAgain);
  void ExecuteRemoteTransfer(bool Move);
  bool RemoteTransferDialog(TStrings * FileList, UnicodeString & Target, UnicodeString & FileMask,
    bool & DirectCopy, bool Move);
  // To the target directory (else the one of the remote panel), with the
  // dialog unless NoConfirmation
  bool RemoteTransferFiles(TStrings * FileList, bool Move,
    const UnicodeString & TargetDirectory = UnicodeString(), bool NoConfirmation = false);
  // Drag and drop (ScpCommanderDragDrop.cpp)
  void PanelStartDrag(TDirPanel * APanel);
  Qt::DropAction PanelDragOver(TDirPanel * APanel, const TDropParams & Params);
  void PanelDrop(TDirPanel * APanel, const TDropParams & Params, Qt::DropAction Action);
  // The local files of the data of a drag or of the clipboard, or NULL
  TStrings * LocalFileListFromMimeData(const QMimeData * Data);
  // The files of the drag were dropped to the directory of another
  // application (empty when it is not local)
  void DragExtracted(const UnicodeString & TargetDirectory, bool Move);
  // Downloads the remote files to a temporary directory for another
  // application; their URLs (none when it failed)
  QList<QUrl> TemporarilyDownloadForOtherApplication(TStrings * FileList);
  // A download to the temporary directory warns about the free space once
  // the size of the files is known (DDWarnLackOfTempSpace)
  bool FPendingTempSpaceWarn;
  void DoWarnLackOfTempSpace(const UnicodeString & Path, __int64 RequiredSpace, bool & Continue);
  void AddDelayedDirectoryDeletion(const UnicodeString & Directory, int SecDelay);
  void DoDelayedDeletion();
  // The desktop integration (ScpCommanderDesktop.cpp)
  // A notification (upstream's Notify and PopupTrayBalloon), of the session
  void Notify(TTerminal * ATerminal, const UnicodeString & Message, TQueryType Type,
    bool Important = false, TNotificationClickEvent OnClick = nullptr);
  // The application wants the attention of the user, when in the
  // background
  void FlashOnBackground();
  // Restores and activates the application for another application (with
  // its activation token)
  void ActivateFromOtherApplication(const QString & ActivationToken);
  void UpdateTrayIcon();
  void SetTaskbarProgress(TFileOperationProgressType * ProgressData);
  void SetQueueProgress();
  // The command line (ScpCommanderCommandLine.cpp)
  // Opens the session of the command line (upstream's GetLoginData), with
  // the site manager when it is not complete; false when not opened
  bool OpenCommandLineSession(
    const UnicodeString & SessionName, TOptions * Options, bool NeedTerminal, UnicodeString & DownloadFile);
  void CommandLineUpload(TStrings * FileList, int UseDefaults);
  // The file of the URL
  void CommandLineDownload(const UnicodeString & FileName, int UseDefaults);
  bool CanCommandLineFromAnotherInstance();
  // The clipboard (ScpCommanderClipboard.cpp)
  void CopyFilesToClipboard();
  void ClipboardStop();
  void ClipboardChanged();
  void ClipboardClear();
  // The clipboard has the remote files of the session copied by us
  bool DoesClipboardContainOurFiles();
  void ClipboardPasted(const UnicodeString & Directory);
  void ClipboardDownload(const UnicodeString & TargetDirectory, bool DragDrop);
  bool CanPasteFromClipboard();
  void PasteFromClipboard();
  void CopyTextToClipboard(TStrings * Strings);
  void FileListToClipboard(bool FullPath);
  void PathToClipboard(TDirPanel * APanel);
  void CurrentCopyToClipboardActionExecute() { CopyFilesToClipboard(); }
  void PasteActionExecute() { PasteFromClipboard(); }
  void FileListToClipboardActionExecute() { FileListToClipboard(false); }
  void FullFileListToClipboardActionExecute() { FileListToClipboard(true); }
  void LocalPathToClipboardActionExecute();
  void RemotePathToClipboardActionExecute();
  bool DoDirectoryExists(void * Session, const UnicodeString & Directory);
  bool NeedSecondarySessionForRemoteCopy(TStrings * FileList);
  TCopyParamType TemporaryFileCopyParam(bool ForceText);
  // Downloads the files to a new temporary directory
  void TemporarilyDownloadFiles(TStrings * FileList, bool ForceText, UnicodeString & RootTempDir,
    UnicodeString & TempDir, bool AutoOperation = false);
  void RenameFile(TDirPanel * Panel, const UnicodeString & FileName, const UnicodeString & NewName);
  bool SetProperties(TDirPanel * Panel, TStrings * FileList);
  void CalculateSize(TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats, bool & Close);
  void CalculateChecksum(const UnicodeString & Alg, TStrings * FileList,
    TCalculatedChecksumEvent OnCalculatedChecksum, bool & Close);
  bool CanCalculateChecksum();
  // The operation needs the separate shell session (asked to be opened)
  bool EnsureCommandSessionFallback(TFSCapability Capability);
};
//---------------------------------------------------------------------------
#endif
