// L3 tests of the dialog keeping the remote directory up to date
// (dialogs/SynchronizeDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <QtInterface.h>

#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QTreeWidget>

#include <gtest/gtest.h>

#include <vector>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// Stands for TSynchronizeController::StartStop
class TStartStopRecorder
{
public:
  std::vector<bool> Starts;
  TSynchronizeParamType StartParams;
  TCopyParamType StartCopyParam;
  TSynchronizeAbortEvent OnAbort;

  void StartStop(TObject * /*Sender*/, bool Start, const TSynchronizeParamType & Params,
    const TCopyParamType & CopyParam, TSynchronizeOptions * /*Options*/, TSynchronizeAbortEvent AOnAbort,
    TSynchronizeThreadsEvent /*OnSynchronizeThreads*/, TSynchronizeLog OnSynchronizeLog)
  {
    Starts.push_back(Start);
    if (Start)
    {
      StartParams = Params;
      StartCopyParam = CopyParam;
      OnAbort = AOnAbort;
      OnSynchronizeLog(NULL, slStart, L"Watching for changes in 1 directories...");
    }
  }
};
//---------------------------------------------------------------------------
TSynchronizeParamType DefaultParams()
{
  TSynchronizeParamType Params;
  Params.LocalDirectory = L"/home/user/local";
  Params.RemoteDirectory = L"/remote";
  Params.Params = TTerminal::spDelete;
  Params.Options = soRecurse | soSynchronizeAsk;
  return Params;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(SynchronizeDialogTest, StartAndStop)
{
  TStartStopRecorder Recorder;
  TSynchronizeParamType Params = DefaultParams();
  TCopyParamType CopyParam;
  CopyParam.PreserveTime = false;
  CopyParam.NewerOnly = true;
  bool SaveSettings = false;
  TFeedSynchronizeError OnFeedSynchronizeError;
  std::function<void()> OnSynchronizeAbort;
  std::vector<UnicodeString> SessionLog;
  bool PartialInitially = false, StopVisibleInitially = true, LogVisibleInitially = true;
  bool StopVisible = false, OptionsEnabled = true, FeedSet = false, SelectedOnlyEnabled = false;
  QStringList Log;
  bool StartVisibleAfter = false, LogVisibleAfter = true;
  WhenModal([&](QWidget * Dialog)
  {
    QCheckBox * SynchronizeCheck = Child<QCheckBox>(Dialog, "SynchronizeSynchronizeCheck");
    PartialInitially = (SynchronizeCheck->checkState() == Qt::PartiallyChecked);
    StopVisibleInitially = Child<QPushButton>(Dialog, "StopButton")->isVisible();
    LogVisibleInitially = Child<QWidget>(Dialog, "LogPanel")->isVisible();
    SelectedOnlyEnabled = Child<QCheckBox>(Dialog, "SynchronizeSelectedOnlyCheck")->isEnabled();
    SynchronizeCheck->setCheckState(Qt::Unchecked);
    Child<QCheckBox>(Dialog, "ContinueOnErrorCheck")->setChecked(true);
    Child<QPushButton>(Dialog, "StartButton")->click();

    StopVisible = Child<QPushButton>(Dialog, "StopButton")->isVisible();
    OptionsEnabled = Child<QWidget>(Dialog, "OptionsGroup")->isEnabled();
    FeedSet = (OnFeedSynchronizeError != nullptr);
    // The errors are logged only, when continuing on errors
    OnFeedSynchronizeError(L"Error listing directory", NULL, qtError);
    QTreeWidget * LogView = Child<QTreeWidget>(Dialog, "LogView");
    for (int Index = 0; Index < LogView->topLevelItemCount(); Index++)
    {
      Log.append(LogView->topLevelItem(Index)->text(1));
    }

    Child<QPushButton>(Dialog, "StopButton")->click();
    StartVisibleAfter = Child<QPushButton>(Dialog, "StartButton")->isVisible();
    LogVisibleAfter = Child<QWidget>(Dialog, "LogPanel")->isVisible();
    Child<QCheckBox>(Dialog, "SaveSettingsCheck")->setChecked(true);
    Child<QPushButton>(Dialog, "CancelButton")->click();
  });
  ASSERT_TRUE(DoSynchronizeDialog(Params, &CopyParam,
    ::rtl::MethodPtr(&Recorder, &TStartStopRecorder::StartStop), SaveSettings, soAllowSelectedOnly, 0,
    [](int, TSynchronizeOptions &) {},
    [&](const UnicodeString & Message) { SessionLog.push_back(Message); },
    OnFeedSynchronizeError, OnSynchronizeAbort));

  EXPECT_TRUE(PartialInitially);
  EXPECT_FALSE(StopVisibleInitially);
  EXPECT_FALSE(LogVisibleInitially);
  EXPECT_TRUE(SelectedOnlyEnabled);
  EXPECT_TRUE(StopVisible);
  EXPECT_FALSE(OptionsEnabled);
  EXPECT_TRUE(FeedSet);
  EXPECT_EQ(Log, QStringList({ QStringLiteral("Watching for changes in 1 directories..."),
    QStringLiteral("Error listing directory") }));
  EXPECT_TRUE(StartVisibleAfter);
  EXPECT_FALSE(LogVisibleAfter);

  EXPECT_EQ(Recorder.Starts, (std::vector<bool>{ true, false }));
  EXPECT_EQ(Recorder.StartParams.LocalDirectory, UnicodeString(L"/home/user/local"));
  EXPECT_EQ(Recorder.StartParams.RemoteDirectory, UnicodeString(L"/remote"));
  EXPECT_EQ(Recorder.StartParams.Params, int(TTerminal::spDelete));
  EXPECT_EQ(Recorder.StartParams.Options, soRecurse | soContinueOnError);
  // The time is always preserved, all files are uploaded
  EXPECT_TRUE(Recorder.StartCopyParam.PreserveTime);
  EXPECT_FALSE(Recorder.StartCopyParam.NewerOnly);

  EXPECT_TRUE(SaveSettings);
  EXPECT_EQ(Params.Options, soRecurse | soContinueOnError);
  EXPECT_EQ(SessionLog.size(), 2u);
  EXPECT_TRUE(OnFeedSynchronizeError == nullptr);
  EXPECT_TRUE(OnSynchronizeAbort == nullptr);
}
//---------------------------------------------------------------------------
TEST(SynchronizeDialogTest, AskedToSynchronizeThenAborted)
{
  TStartStopRecorder Recorder;
  TSynchronizeParamType Params = DefaultParams();
  TCopyParamType CopyParam;
  bool SaveSettings = false;
  TFeedSynchronizeError OnFeedSynchronizeError;
  std::function<void()> OnSynchronizeAbort;
  bool AbortSet = false;
  WhenModal([&](QWidget * Dialog)
  {
    AbortSet = (OnSynchronizeAbort != nullptr);
    // Not to synchronize first
    WhenModal([&](QWidget * Message)
    {
      Child<QPushButton>(Message, "No")->click();
      // Once started, the session is gone
      QTimer::singleShot(50, [&]() { Recorder.OnAbort(NULL, true); });
    });
    Child<QPushButton>(Dialog, "StartButton")->click();
  });
  ASSERT_TRUE(DoSynchronizeDialog(Params, &CopyParam,
    ::rtl::MethodPtr(&Recorder, &TStartStopRecorder::StartStop), SaveSettings, 0, 0,
    [](int, TSynchronizeOptions &) {}, [](const UnicodeString &) {},
    OnFeedSynchronizeError, OnSynchronizeAbort));
  EXPECT_TRUE(AbortSet);
  // Stopped and closed
  EXPECT_EQ(Recorder.Starts, (std::vector<bool>{ true, false }));
  EXPECT_EQ(Recorder.StartParams.Options, soRecurse);
  // Asking again next time
  EXPECT_EQ(Params.Options, soRecurse | soSynchronizeAsk);
}
//---------------------------------------------------------------------------
