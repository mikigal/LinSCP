// The main window with the Commander interface (upstream's
// TScpCommanderForm, forms/ScpCommander.dfm and CustomScpExplorer.dfm, with
// the actions of forms/NonVisual.dfm): the local and the remote panel, the
// menus and toolbars, and the commands on the files of the current panel.
//---------------------------------------------------------------------------
#ifndef ScpCommanderH
#define ScpCommanderH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <FileOperationProgress.h>

#include <QMainWindow>
//---------------------------------------------------------------------------
class QAction;
class QComboBox;
class QLabel;
class QSplitter;
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
  QAction * ShowHiddenFilesAction;

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
  void ShowHiddenFilesActionExecute();
  void DeleteFiles(bool Alternative);
  void ExecuteCopyMoveFileOperation(TFileOperation Operation);
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
