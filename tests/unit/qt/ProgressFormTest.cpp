// L3 tests of the progress window (dialogs/ProgressForm.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <ProgressForm.h>

#include <QLabel>
#include <QProgressBar>
#include <QPushButton>

#include <gtest/gtest.h>

#include <cstdlib>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class TProgressHandler
{
public:
  explicit TProgressHandler(TProgressForm * Form) : FForm(Form) {}

  void Progress(TFileOperationProgressType & ProgressData)
  {
    if (ProgressData.InProgress)
    {
      FForm->SetProgressData(ProgressData);
      if (FForm->GetCancel() > csContinue)
      {
        ProgressData.SetCancelAtLeast(FForm->GetCancel());
      }
    }
  }

  void Finished(TFileOperation, TOperationSide, bool, const UnicodeString &, bool, bool, TOnceDoneOperation &)
  {
  }

private:
  TProgressForm * FForm;
};
//---------------------------------------------------------------------------
TFileOperationProgressType * CreateProgress(TProgressHandler & Handler)
{
  return
    new TFileOperationProgressType(
      ::rtl::MethodPtr(&Handler, &TProgressHandler::Progress), ::rtl::MethodPtr(&Handler, &TProgressHandler::Finished));
}
//---------------------------------------------------------------------------
QString LabelText(QWidget * Form, const char * Name)
{
  return Child<QLabel>(Form, Name)->text();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(ProgressFormTest, Upload)
{
  TProgressForm Form(nullptr, true);
  Form.show();
  TProgressHandler Handler(&Form);
  std::unique_ptr<TFileOperationProgressType> Progress(CreateProgress(Handler));
  Progress->Start(foCopy, osLocal, 2, false, L"/remote/dir/", 0, odoIdle);
  Progress->SetTotalSize(1000);
  Progress->SetFile(L"/local/dir/a.txt");
  Progress->SetLocalSize(400);
  Progress->SetTransferSize(400);
  Progress->AddLocallyUsed(400);
  Progress->AddTransferred(200);
  // The window updates at intervals; the last data shows on the next update
  QTest::qWait(250);
  Progress->AddTransferred(0);

  EXPECT_EQ(LabelText(&Form, "FileLabel"), QStringLiteral("/local/dir/a.txt"));
  EXPECT_EQ(LabelText(&Form, "TargetPathLabel"), QStringLiteral("/remote/dir/"));
  EXPECT_EQ(Child<QProgressBar>(&Form, "FileProgress")->value(), 50);
  EXPECT_EQ(Child<QProgressBar>(&Form, "OperationProgress")->value(), 20);
  EXPECT_EQ(LabelText(&Form, "BytesTransferredLabel"), QStringLiteral("200 B"));
  EXPECT_TRUE(Form.windowTitle().startsWith(QStringLiteral("20% Uploading")));
  EXPECT_TRUE(Child<QWidget>(&Form, "TransferPanel")->isVisible());
  EXPECT_TRUE(Child<QPushButton>(&Form, "SkipItem")->isVisible());

  const char * GrabDir = getenv("OPENSCP_QT_TEST_GRAB_DIR");
  if (GrabDir != NULL)
  {
    Form.grab().save(QString::fromLocal8Bit(GrabDir) + QStringLiteral("/progress-upload.png"));
  }
  Progress->Stop();
}
//---------------------------------------------------------------------------
TEST(ProgressFormTest, CancelDelete)
{
  TProgressForm Form(nullptr, false);
  Form.show();
  TProgressHandler Handler(&Form);
  std::unique_ptr<TFileOperationProgressType> Progress(CreateProgress(Handler));
  Progress->Start(foDelete, osRemote, 3);
  Progress->SetFile(L"/remote/dir/b.txt");
  EXPECT_FALSE(Child<QWidget>(&Form, "TransferPanel")->isVisible());
  EXPECT_FALSE(Child<QPushButton>(&Form, "SkipItem")->isVisible());
  EXPECT_TRUE(Form.windowTitle().contains(QStringLiteral("Deleting")));

  Child<QPushButton>(&Form, "CancelItem")->click();
  EXPECT_EQ(Form.GetCancel(), csCancel);
  // The next progress cancels the operation
  Progress->SetFile(L"/remote/dir/c.txt");
  EXPECT_EQ(Progress->Cancel, csCancel);
  EXPECT_FALSE(Child<QPushButton>(&Form, "CancelItem")->isEnabled());
  Progress->Stop();
}
//---------------------------------------------------------------------------
