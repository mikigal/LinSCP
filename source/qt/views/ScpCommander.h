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
  int FBusy;
  // The session whose local directory the local panel went to
  TTerminal * FLocalDirectoryTerminal;

  QAction * NewSessionAction;
  QAction * DisconnectSessionAction;
  QAction * CloseApplicationAction;
  QAction * CurrentCopyAction;
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
  void SetCurrentPanel(TDirPanel * Panel);
  TDirPanel * OtherPanel(TDirPanel * Panel) const;
  TTerminal * GetTerminal() const;
  // The selected files of the panel: full paths (and the remote files)
  TStrings * CreateFileList(TDirPanel * Panel);
  void DestroyProgressForm();

  void NewSessionActionExecute() { NewSession(); }
  void DisconnectSessionActionExecute();
  void CloseApplicationActionExecute();
  void CurrentCopyActionExecute();
  void CurrentRenameActionExecute();
  void CurrentDeleteActionExecute() { DeleteFiles(false); }
  void CurrentDeleteAlternativeActionExecute() { DeleteFiles(true); }
  void CurrentCreateDirActionExecute();
  void CurrentPropertiesActionExecute();
  void ShowHiddenFilesActionExecute();
  void DeleteFiles(bool Alternative);
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
