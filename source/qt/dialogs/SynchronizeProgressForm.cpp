// The progress of the comparison of directories (upstream's
// forms/SynchronizeProgress.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include "SynchronizeProgressForm.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
static QLabel * CreateLabel(QWidget * Parent, const char * Name, const QString & Caption = QString())
{
  QLabel * Label = new QLabel(Caption, Parent);
  Label->setObjectName(QString::fromLatin1(Name));
  return Label;
}
//---------------------------------------------------------------------------
TSynchronizeProgressForm::TSynchronizeProgressForm(QWidget * Parent, int Files, bool LocalLocal) :
  QDialog(Parent)
{
  setObjectName(QStringLiteral("SynchronizeProgressForm"));
  FStarted = false;
  FCanceled = false;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QGridLayout * Grid = new QGridLayout();
  LeftLabel = CreateLabel(this, "LeftLabel", tr("Local:"));
  Grid->addWidget(LeftLabel, 0, 0);
  LeftDirectoryLabel = CreateLabel(this, "LeftDirectoryLabel");
  LeftDirectoryLabel->setTextFormat(Qt::PlainText);
  LeftDirectoryLabel->setMinimumWidth(fontMetrics().averageCharWidth() * 55);
  Grid->addWidget(LeftDirectoryLabel, 0, 1, 1, 3);
  RightLabel = CreateLabel(this, "RightLabel", tr("Remote:"));
  Grid->addWidget(RightLabel, 1, 0);
  RightDirectoryLabel = CreateLabel(this, "RightDirectoryLabel");
  RightDirectoryLabel->setTextFormat(Qt::PlainText);
  Grid->addWidget(RightDirectoryLabel, 1, 1, 1, 3);
  StartTimeLabelLabel = CreateLabel(this, "StartTimeLabelLabel", tr("Start time:"));
  Grid->addWidget(StartTimeLabelLabel, 2, 0);
  StartTimeLabel = CreateLabel(this, "StartTimeLabel", QStringLiteral("00:00:00"));
  Grid->addWidget(StartTimeLabel, 2, 1);
  TimeLeftLabelLabel = CreateLabel(this, "TimeLeftLabelLabel", tr("Time left:"));
  Grid->addWidget(TimeLeftLabelLabel, 2, 0);
  TimeLeftLabel = CreateLabel(this, "TimeLeftLabel", QStringLiteral("00:00:00"));
  Grid->addWidget(TimeLeftLabel, 2, 1);
  Grid->addWidget(CreateLabel(this, "Label3", tr("Time elapsed:")), 2, 2);
  TimeElapsedLabel = CreateLabel(this, "TimeElapsedLabel", QStringLiteral("00:00:00"));
  Grid->addWidget(TimeElapsedLabel, 2, 3);
  Grid->setColumnStretch(1, 1);
  Grid->setColumnStretch(3, 1);
  MainLayout->addLayout(Grid);
  OperationProgress = new QProgressBar(this);
  OperationProgress->setObjectName(QStringLiteral("OperationProgress"));
  MainLayout->addWidget(OperationProgress);
  QHBoxLayout * ButtonLayout = new QHBoxLayout();
  ButtonLayout->addStretch(1);
  CancelItem = new QPushButton(tr("Cancel"), this);
  CancelItem->setObjectName(QStringLiteral("CancelItem"));
  CancelItem->setAutoDefault(false);
  connect(CancelItem, &QPushButton::clicked, this, [this]() { CancelOperation(); });
  ButtonLayout->addWidget(CancelItem);
  MainLayout->addLayout(ButtonLayout);
  layout()->setSizeConstraint(QLayout::SetFixedSize);

  bool KnowsTotalFiles = (Files >= 0);
  // A busy indicator (upstream's marquee), when the number is not known
  OperationProgress->setRange(0, KnowsTotalFiles ? Files : 0);
  TimeLeftLabelLabel->setVisible(KnowsTotalFiles);
  TimeLeftLabel->setVisible(KnowsTotalFiles);
  StartTimeLabelLabel->setVisible(!KnowsTotalFiles);
  StartTimeLabel->setVisible(!KnowsTotalFiles);
  if (LocalLocal)
  {
    LeftLabel->setText(ToQString(LoadStr(SYNCHRONIZE_PROGRESS_LEFT_DIR)));
    RightLabel->setText(ToQString(LoadStr(SYNCHRONIZE_PROGRESS_RIGHT_DIR)));
  }

  UpdateTimer = new QTimer(this);
  UpdateTimer->setInterval(500);
  connect(UpdateTimer, &QTimer::timeout, this, [this]() { UpdateControls(); });
}
//---------------------------------------------------------------------------
void TSynchronizeProgressForm::Start()
{
  FStarted = true;
  FStartTime = Now();
  UpdateTimer->start();
  StartTimeLabel->setText(ToQString(FStartTime.TimeString()));
  OperationProgress->setValue(OperationProgress->minimum());
  UpdateControls();
  // As TProgressForm: blocks the other windows, the comparison runs in the
  // main thread
  setWindowModality(Qt::ApplicationModal);
  show();
  raise();
  activateWindow();
}
//---------------------------------------------------------------------------
int TSynchronizeProgressForm::SetData(
  const UnicodeString & Directory1, const UnicodeString & Directory2, int Progress, bool & Continue)
{
  DebugAssert(FStarted);
  LeftDirectoryLabel->setText(ToQString(Directory1));
  RightDirectoryLabel->setText(ToQString(Directory2));
  if (OperationProgress->maximum() > 0)
  {
    OperationProgress->setValue(std::min(Progress, OperationProgress->maximum()));
  }
  Continue = !FCanceled;

  UpdateControls();
  QCoreApplication::processEvents();
  return CalculateProgress();
}
//---------------------------------------------------------------------------
int TSynchronizeProgressForm::CalculateProgress()
{
  return
    (OperationProgress->maximum() == 0) ? -1 :
      ((OperationProgress->value() * 100) / OperationProgress->maximum());
}
//---------------------------------------------------------------------------
void TSynchronizeProgressForm::UpdateControls()
{
  TDateTime Elapsed;
  UnicodeString ACaption = LoadStr(SYNCHRONIZE_PROGRESS_COMPARE);
  if (FStarted)
  {
    Elapsed = Now() - FStartTime;
    int Progress = CalculateProgress();
    if (Progress >= 0)
    {
      ACaption = FORMAT(L"%d%% %s", (Progress, ACaption));
    }
  }
  else
  {
    Elapsed = EncodeTimeVerbose(0, 0, 0, 0);
  }
  setWindowTitle(ToQString(FormatMainFormCaption(ACaption)));
  TimeElapsedLabel->setText(ToQString(FormatDateTimeSpan(Elapsed)));
  UnicodeString TimeLeftCaption;
  int Position = OperationProgress->value();
  if (FStarted && (OperationProgress->maximum() > 0) && (Position > 0) &&
      (double(Elapsed) * SecsPerDay >= 3))
  {
    TDateTime TimeLeft = TDateTime(double(double(Elapsed) * (OperationProgress->maximum() - Position) / Position));
    TimeLeftCaption = FormatDateTimeSpan(TimeLeft);
  }
  else
  {
    TimeLeftCaption = LoadStr(PROGRESS_TIME_LEFT_CALCULATING);
  }
  TimeLeftLabel->setText(ToQString(TimeLeftCaption));
  CancelItem->setEnabled(!FCanceled);
}
//---------------------------------------------------------------------------
void TSynchronizeProgressForm::CancelOperation()
{
  FCanceled = true;
  UpdateControls();
}
//---------------------------------------------------------------------------
void TSynchronizeProgressForm::closeEvent(QCloseEvent * Event)
{
  // Closing cancels; the window goes once the comparison stops
  CancelOperation();
  Event->ignore();
}
//---------------------------------------------------------------------------
void TSynchronizeProgressForm::keyPressEvent(QKeyEvent * Event)
{
  if (Event->key() == Qt::Key_Escape)
  {
    CancelOperation();
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
