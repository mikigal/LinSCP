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
#include <FileOperationProgress.h>
#include <Queue.h>

#include "QueueController.h"

#include <QMainWindow>

#include <mutex>
//---------------------------------------------------------------------------
class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QSplitter;
class QTimer;
class QToolBar;
class QTreeWidget;
class TDirPanel;
class TLocalDirPanel;
class TProgressForm;
class TRemoteDirPanel;
class TTerminal;
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
  // The active session changed (opened, closed)
  void TerminalChanged();
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

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  QSplitter * FPanelSplitter;
  TLocalDirPanel * FLocalPanel;
  TRemoteDirPanel * FRemotePanel;
  TDirPanel * FCurrentPanel;
  TProgressForm * FProgressForm;
  QLabel * FSessionStatusLabel;
  QComboBox * FTransferList;
  int FBusy;
  int FUpdatingTransferList;
  // The preset selected by the user, and the one selected by its rule
  UnicodeString FCopyParamDefault;
  UnicodeString FCopyParamAutoSelected;
  // The session whose local directory the local panel went to
  TTerminal * FLocalDirectoryTerminal;
  QTimer * FIdleTimer;
  // The operations on files report their files to it (the file find dialog)
  TFileOperationFinishedEvent FOnFileOperationFinished;
  TTerminal * FFileFindTerminal;
  bool FRefreshLocalDirectory;
  bool FRefreshRemoteDirectory;

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
  QAction * DisconnectSessionAction;
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
  QAction * RemoteFindFilesAction2;
  QAction * ShowHiddenFilesAction;
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
  QAction * QueueShowAction;
  QAction * QueueHideWhenEmptyAction;
  QAction * QueueHideAction;

  QAction * CreateAction(const char * Name, const QString & Caption, const QString & IconName,
    const QList<QKeySequence> & ShortCuts, void (TScpCommanderForm::*Method)());
  void CreateMenus();
  void CreateToolbars();
  void UpdateActions();
  void UpdateTransferList();
  void TransferListChange();
  void TransferPresetAutoSelect();
  void SetCurrentPanel(TDirPanel * Panel);
  TDirPanel * OtherPanel(TDirPanel * Panel) const;
  TTerminal * GetTerminal() const;
  // The selected files of the panel: full paths (and the remote files)
  TStrings * CreateFileList(TDirPanel * Panel);
  void DestroyProgressForm();
  void Idle();

  // The transfer queue (ScpCommanderQueue.cpp)
  void CreateQueueActions();
  void CreateQueuePanel();
  QMenu * CreateQueueMenu();
  void SetQueue(TTerminalQueue * Value);
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

  void NewSessionActionExecute() { NewSession(); }
  void DisconnectSessionActionExecute();
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
  void DoFindFiles(TTerminal * ATerminal, UnicodeString Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void DoFocusRemotePath(TTerminal * ATerminal, const UnicodeString & Path);
  bool CanOperateOnFoundFiles(TTerminal * ATerminal);
  void ShowHiddenFilesActionExecute();
  void DeleteFiles(bool Alternative);
  bool DeleteFiles(TOperationSide Side, TStrings * FileList, bool Alternative);
  void ExecuteCopyMoveFileOperation(TFileOperation Operation);
  bool ExecuteCopyMoveFileOperation(TFileOperation Operation, TOperationSide Side, TStrings * FileList);
  bool CopyParamDialog(TTransferDirection Direction, TTransferType Type,
    TStrings * FileList, UnicodeString & TargetDirectory, TGUICopyParamType & CopyParam, bool Confirm);
  void ExecuteRemoteTransfer(bool Move);
  bool RemoteTransferDialog(TStrings * FileList, UnicodeString & Target, UnicodeString & FileMask,
    bool & DirectCopy, bool Move);
  bool RemoteTransferFiles(TStrings * FileList, bool Move);
  bool DoDirectoryExists(void * Session, const UnicodeString & Directory);
  bool NeedSecondarySessionForRemoteCopy(TStrings * FileList);
  TCopyParamType TemporaryFileCopyParam(bool ForceText);
  // Downloads the files to a new temporary directory
  void TemporarilyDownloadFiles(TStrings * FileList, bool ForceText, UnicodeString & RootTempDir,
    UnicodeString & TempDir);
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
