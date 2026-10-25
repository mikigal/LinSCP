// The progress window of operations (upstream's TProgressForm,
// forms/Progress.dfm): the file, the target, the overall progress and, for
// transfers, the progress of the file, the time and the speed.
//---------------------------------------------------------------------------
#ifndef ProgressFormH
#define ProgressFormH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <FileOperationProgress.h>

#include <QDialog>
#include <QElapsedTimer>
//---------------------------------------------------------------------------
class QLabel;
class QProgressBar;
class QPushButton;
//---------------------------------------------------------------------------
class TProgressForm : public QDialog
{
  Q_OBJECT

public:
  TProgressForm(QWidget * Parent, bool AllowSkip);

  // Shown while the operation runs, blocking the other windows, without a
  // modal event loop of its own (the operation runs in the main thread)
  void ShowAsModal();
  void SetProgressData(TFileOperationProgressType & AData);
  TCancelStatus GetCancel() const { return FCancel; }
  void ClearCancel();
  static UnicodeString ProgressStr(const TFileOperationProgressType * ProgressData);

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  TFileOperationProgressType FData;
  bool FDataGot;
  TCancelStatus FCancel;
  bool FAllowSkip;
  TFileOperation FLastOperation;
  TOperationSide FLastSide;
  QElapsedTimer FSinceLastUpdate;

  QLabel * PathLabel;
  QLabel * FileLabel;
  QLabel * TargetLabel;
  QLabel * TargetPathLabel;
  QProgressBar * OperationProgress;
  QWidget * TransferPanel;
  QLabel * StartTimeLabelLabel;
  QLabel * StartTimeLabel;
  QLabel * TimeLeftLabelLabel;
  QLabel * TimeLeftLabel;
  QLabel * TimeElapsedLabel;
  QLabel * BytesTransferredLabel;
  QLabel * CPSLabel;
  QProgressBar * FileProgress;
  QPushButton * CancelItem;
  QPushButton * SkipItem;

  void UpdateControls();
  void CancelOperation();
  void SetCancelLower(TCancelStatus ACancel);
};
//---------------------------------------------------------------------------
#endif
