// The progress of the comparison of directories (upstream's
// TSynchronizeProgressForm, forms/SynchronizeProgress.dfm): the directories
// being compared, the time and the progress by the files of the last
// comparison, if known.
//---------------------------------------------------------------------------
#ifndef SynchronizeProgressFormH
#define SynchronizeProgressFormH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QDialog>
//---------------------------------------------------------------------------
class QLabel;
class QProgressBar;
class QPushButton;
class QTimer;
//---------------------------------------------------------------------------
class TSynchronizeProgressForm : public QDialog
{
  Q_OBJECT

public:
  TSynchronizeProgressForm(QWidget * Parent, int Files, bool LocalLocal);

  void Start();
  int SetData(const UnicodeString & Directory1, const UnicodeString & Directory2, int Progress, bool & Continue);
  bool GetStarted() const { return FStarted; }

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  bool FStarted;
  bool FCanceled;
  TDateTime FStartTime;

  QLabel * LeftLabel;
  QLabel * LeftDirectoryLabel;
  QLabel * RightLabel;
  QLabel * RightDirectoryLabel;
  QLabel * StartTimeLabelLabel;
  QLabel * StartTimeLabel;
  QLabel * TimeLeftLabelLabel;
  QLabel * TimeLeftLabel;
  QLabel * TimeElapsedLabel;
  QProgressBar * OperationProgress;
  QPushButton * CancelItem;
  QTimer * UpdateTimer;

  int CalculateProgress();
  void UpdateControls();
  void CancelOperation();
};
//---------------------------------------------------------------------------
#endif
