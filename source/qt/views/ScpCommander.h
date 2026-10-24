// The main window with the Commander interface (upstream's
// TScpCommanderForm, forms/ScpCommander.dfm and CustomScpExplorer.dfm).
//---------------------------------------------------------------------------
#ifndef ScpCommanderH
#define ScpCommanderH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <FileOperationProgress.h>

#include <QMainWindow>
//---------------------------------------------------------------------------
class QSplitter;
class TLocalDirPanel;
class TRemoteDirPanel;
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

private:
  QSplitter * FPanelSplitter;
  TLocalDirPanel * FLocalPanel;
  TRemoteDirPanel * FRemotePanel;
};
//---------------------------------------------------------------------------
#endif
