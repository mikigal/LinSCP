// The progress window of operations (upstream's forms/Progress.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include "ProgressForm.h"

#include <QApplication>
#include <QCloseEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
// The interval of the updates of the window (upstream updates it with a
// timer; the progress is reported far more often)
static const int UpdateInterval = 200;
//---------------------------------------------------------------------------
static QLabel * CreateLabel(QWidget * Parent, const char * Name, const QString & Caption = QString())
{
  QLabel * Label = new QLabel(Caption, Parent);
  Label->setObjectName(QString::fromLatin1(Name));
  return Label;
}
//---------------------------------------------------------------------------
TProgressForm::TProgressForm(QWidget * Parent, bool AllowSkip) :
  QDialog(Parent)
{
  setObjectName(QStringLiteral("ProgressForm"));
  FDataGot = false;
  FCancel = csContinue;
  FAllowSkip = AllowSkip;
  FLastOperation = foNone;
  FLastSide = osCurrent;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);

  QGridLayout * MainPanel = new QGridLayout();
  MainLayout->addLayout(MainPanel);
  PathLabel = CreateLabel(this, "PathLabel", ToQString(LoadStr(PROGRESS_FILE_LABEL)));
  MainPanel->addWidget(PathLabel, 0, 0);
  FileLabel = CreateLabel(this, "FileLabel");
  FileLabel->setTextFormat(Qt::PlainText);
  FileLabel->setMinimumWidth(fontMetrics().averageCharWidth() * 55);
  MainPanel->addWidget(FileLabel, 0, 1);
  TargetLabel = CreateLabel(this, "TargetLabel", tr("Target:"));
  MainPanel->addWidget(TargetLabel, 1, 0);
  TargetPathLabel = CreateLabel(this, "TargetPathLabel");
  TargetPathLabel->setTextFormat(Qt::PlainText);
  MainPanel->addWidget(TargetPathLabel, 1, 1);
  MainPanel->setColumnStretch(1, 1);
  OperationProgress = new QProgressBar(this);
  OperationProgress->setObjectName(QStringLiteral("OperationProgress"));
  OperationProgress->setRange(0, 100);
  MainLayout->addWidget(OperationProgress);

  TransferPanel = new QWidget(this);
  TransferPanel->setObjectName(QStringLiteral("TransferPanel"));
  QGridLayout * TransferLayout = new QGridLayout(TransferPanel);
  TransferLayout->setContentsMargins(0, 0, 0, 0);
  StartTimeLabelLabel = CreateLabel(TransferPanel, "StartTimeLabelLabel", tr("Start time:"));
  TransferLayout->addWidget(StartTimeLabelLabel, 0, 0);
  StartTimeLabel = CreateLabel(TransferPanel, "StartTimeLabel");
  TransferLayout->addWidget(StartTimeLabel, 0, 1);
  TimeLeftLabelLabel = CreateLabel(TransferPanel, "TimeLeftLabelLabel", tr("Time left:"));
  TransferLayout->addWidget(TimeLeftLabelLabel, 0, 0);
  TimeLeftLabel = CreateLabel(TransferPanel, "TimeLeftLabel");
  TransferLayout->addWidget(TimeLeftLabel, 0, 1);
  TransferLayout->addWidget(CreateLabel(TransferPanel, "Label3", tr("Time elapsed:")), 0, 2);
  TimeElapsedLabel = CreateLabel(TransferPanel, "TimeElapsedLabel");
  TransferLayout->addWidget(TimeElapsedLabel, 0, 3);
  TransferLayout->addWidget(CreateLabel(TransferPanel, "Label4", tr("Bytes transferred:")), 1, 0);
  BytesTransferredLabel = CreateLabel(TransferPanel, "BytesTransferredLabel");
  TransferLayout->addWidget(BytesTransferredLabel, 1, 1);
  TransferLayout->addWidget(CreateLabel(TransferPanel, "Label12", tr("Speed:")), 1, 2);
  CPSLabel = CreateLabel(TransferPanel, "CPSLabel");
  TransferLayout->addWidget(CPSLabel, 1, 3);
  TransferLayout->setColumnStretch(1, 1);
  TransferLayout->setColumnStretch(3, 1);
  FileProgress = new QProgressBar(TransferPanel);
  FileProgress->setObjectName(QStringLiteral("FileProgress"));
  FileProgress->setRange(0, 100);
  TransferLayout->addWidget(FileProgress, 2, 0, 1, 4);
  MainLayout->addWidget(TransferPanel);

  QHBoxLayout * ButtonLayout = new QHBoxLayout();
  MainLayout->addLayout(ButtonLayout);
  ButtonLayout->addStretch(1);
  SkipItem = new QPushButton(tr("&Skip this file"), this);
  SkipItem->setObjectName(QStringLiteral("SkipItem"));
  SkipItem->setAutoDefault(false);
  ButtonLayout->addWidget(SkipItem);
  CancelItem = new QPushButton(tr("Cancel"), this);
  CancelItem->setObjectName(QStringLiteral("CancelItem"));
  CancelItem->setAutoDefault(false);
  ButtonLayout->addWidget(CancelItem);
  connect(CancelItem, &QPushButton::clicked, this, [this]() { CancelOperation(); });
  connect(SkipItem, &QPushButton::clicked, this, [this]() { SetCancelLower(csCancelFile); });

  layout()->setSizeConstraint(QLayout::SetFixedSize);
  setWindowTitle(tr("Operation"));
}
//---------------------------------------------------------------------------
void TProgressForm::ShowAsModal()
{
  setWindowModality(Qt::ApplicationModal);
  show();
  raise();
  activateWindow();
}
//---------------------------------------------------------------------------
UnicodeString TProgressForm::ProgressStr(const TFileOperationProgressType * ProgressData)
{
  static const int Captions[] = { 0, 0, PROGRESS_DELETE,
    PROGRESS_SETPROPERTIES, 0, PROGRESS_CUSTOM_COMAND, PROGRESS_CALCULATE_SIZE,
    PROGRESS_REMOTE_MOVE, PROGRESS_REMOTE_COPY, PROGRESS_GETPROPERTIES,
    PROGRESS_CALCULATE_CHECKSUM, PROGRESS_LOCK, PROGRESS_UNLOCK };
  int Id;
  if (ProgressData->IsTransfer())
  {
    Id = (ProgressData->Side == osLocal) ? PROGRESS_UPLOAD : PROGRESS_DOWNLOAD;
  }
  else
  {
    int Index = static_cast<int>(ProgressData->Operation) - 1;
    Id = ((Index >= 0) && (Index < static_cast<int>(std::size(Captions)))) ? Captions[Index] : 0;
  }
  UnicodeString Result = (Id != 0) ? LoadStr(Id) : UnicodeString();
  if (!ProgressData->IsIndeterminate())
  {
    Result = FORMAT(L"%d%% %s", (ProgressData->OverallProgress(), Result));
  }
  return Result;
}
//---------------------------------------------------------------------------
void TProgressForm::SetProgressData(TFileOperationProgressType & AData)
{
  // The first file shows at once, the rest at intervals
  bool InstantUpdate = (FileLabel->text().isEmpty() && !AData.FileName.IsEmpty()) || !FSinceLastUpdate.isValid();
  FData.AssignButKeepSuspendState(AData);
  FDataGot = true;
  if (InstantUpdate || FSinceLastUpdate.hasExpired(UpdateInterval))
  {
    UpdateControls();
    FSinceLastUpdate.start();
  }
  // The operation runs in the main thread
  ProcessGUI(InstantUpdate);
}
//---------------------------------------------------------------------------
void TProgressForm::UpdateControls()
{
  bool TransferOperation = FData.IsTransfer();
  bool Indeterminate = FData.IsIndeterminate();

  CancelItem->setEnabled(FCancel < csCancel);
  SkipItem->setVisible(TransferOperation && FAllowSkip);
  SkipItem->setEnabled(FCancel < csCancelFile);

  if ((FData.Operation != FLastOperation) || (FData.Side != FLastSide))
  {
    CancelItem->setText((FData.Operation == foCalculateSize) ? ToQString(LoadStr(SKIP_BUTTON)) : tr("Cancel"));
    TransferPanel->setVisible(TransferOperation);
    TargetLabel->setVisible(TransferOperation);
    TargetPathLabel->setVisible(TransferOperation);
    PathLabel->setText(ToQString(LoadStr((FData.Operation == foCalculateSize) ? PROGRESS_PATH_LABEL : PROGRESS_FILE_LABEL)));
    FLastOperation = FData.Operation;
    FLastSide = FData.Side;
  }

  StartTimeLabelLabel->setVisible(!FData.TotalSizeSet);
  StartTimeLabel->setVisible(!FData.TotalSizeSet);
  TimeLeftLabelLabel->setVisible(FData.TotalSizeSet);
  TimeLeftLabel->setVisible(FData.TotalSizeSet);

  UnicodeString FileCaption;
  if (FData.Operation == foCalculateSize)
  {
    FileCaption = (FData.Side == osRemote) ? UnixExtractFileDir(FData.FullFileName) : ExtractFileDir(FData.FullFileName);
  }
  else if ((FData.Side == osRemote) || !FData.Temp)
  {
    FileCaption = FData.FileName;
  }
  else
  {
    FileCaption = ExtractFileName(FData.FileName);
  }
  FileLabel->setText(ToQString(FileCaption));

  if (Indeterminate)
  {
    OperationProgress->setRange(0, 0);
  }
  else
  {
    OperationProgress->setRange(0, 100);
    OperationProgress->setValue(std::max(0, FData.OverallProgress()));
  }
  setWindowTitle(ToQString(FormatMainFormCaption(ProgressStr(&FData))));

  if (TransferOperation)
  {
    TargetPathLabel->setText(
      ToQString(((FData.Side == osLocal) || !FData.Temp) ? FData.Directory : LoadStr(PROGRESS_TEMP_DIR)));
    StartTimeLabel->setText(ToQString(FData.StartTime.TimeString()));
    if (FData.TotalSizeSet)
    {
      UnicodeString TimeLeftCaption;
      // As upstream's CanShowTimeEstimate
      if (SecondsBetween(FData.StartTime, Now()) >= 3)
      {
        TimeLeftCaption = FormatDateTimeSpan(FData.TotalTimeLeft());
      }
      else
      {
        TimeLeftCaption = LoadStr(PROGRESS_TIME_LEFT_CALCULATING);
      }
      TimeLeftLabel->setText(ToQString(TimeLeftCaption));
    }
    TimeElapsedLabel->setText(ToQString(FormatDateTimeSpan(FData.TimeElapsed())));
    BytesTransferredLabel->setText(ToQString(FormatBytes(FData.TotalTransferred)));
    CPSLabel->setText(ToQString(FORMAT(L"%s/s", (FormatBytes(FData.CPS())))));
    FileProgress->setValue(FData.TransferProgress());
  }
}
//---------------------------------------------------------------------------
void TProgressForm::SetCancelLower(TCancelStatus ACancel)
{
  if (FCancel < ACancel)
  {
    FCancel = ACancel;
    UpdateControls();
  }
}
//---------------------------------------------------------------------------
void TProgressForm::ClearCancel()
{
  FCancel = csContinue;
  UpdateControls();
}
//---------------------------------------------------------------------------
void TProgressForm::CancelOperation()
{
  if (FDataGot)
  {
    TCancelStatus ACancel;
    // A file transfer cannot be cancelled in its middle without closing
    // the connection
    if (FData.TransferringFile &&
        (FData.TimeExpected() > GUIConfiguration->IgnoreCancelBeforeFinish))
    {
      unsigned int Result = MessageDialog(LoadStr(CANCEL_OPERATION_FATAL2), qtWarning, qaYes | qaNo | qaCancel);
      switch (Result)
      {
        case qaYes:
          ACancel = csCancelTransfer;
          break;
        case qaNo:
          ACancel = csCancel;
          break;
        default:
          ACancel = csContinue;
          break;
      }
    }
    else
    {
      ACancel = csCancel;
    }
    SetCancelLower(ACancel);
  }
}
//---------------------------------------------------------------------------
void TProgressForm::closeEvent(QCloseEvent * Event)
{
  // The window closes with the operation
  Event->ignore();
  CancelOperation();
}
//---------------------------------------------------------------------------
void TProgressForm::keyPressEvent(QKeyEvent * Event)
{
  if ((Event->key() == Qt::Key_Escape) && (Event->modifiers() == Qt::NoModifier))
  {
    CancelOperation();
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
