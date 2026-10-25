// L3 tests of the copy dialog and the transfer settings
// (dialogs/CopyDialog.cpp, CopyParamsFrame.cpp, EditMaskDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <CopyParamsFrame.h>
#include <FileMasks.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The text of a message dialog (with or without the main instruction)
QString MessageText(QWidget * Dialog)
{
  QLabel * Label = Child<QLabel>(Dialog, "MainMessage");
  if (Label == nullptr)
  {
    Label = Child<QLabel>(Dialog, "Message");
  }
  return Label->text();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(CopyParamsFrameTest, ParamsRoundTrip)
{
  TCopyParamsFrame Frame(nullptr);
  TCopyParamType Params;
  Params.TransferMode = tmAutomatic;
  Params.AsciiFileMask.Masks = L"*.txt; *.md";
  Params.FileNameCase = ncLowerCase;
  Params.PreserveRights = true;
  Params.Rights = TRights(0750);
  Params.PreserveTime = false;
  Params.IncludeFileMask.Masks = L"*.c | *.o";
  Params.CPSLimit = 100 * 1024;
  Params.NewerOnly = true;
  Params.ExcludeEmptyDirectories = true;
  Frame.SetParams(Params);

  EXPECT_TRUE(Child<QRadioButton>(&Frame, "TMAutomaticButton")->isChecked());
  EXPECT_TRUE(Child<QComboBox>(&Frame, "AsciiFileMaskCombo")->isEnabled());
  EXPECT_TRUE(Child<QLineEdit>(&Frame, "RightsEdit")->isEnabled());
  EXPECT_EQ(Child<QLineEdit>(&Frame, "RightsEdit")->text(), QStringLiteral("rwxr-x--- (+x)"));
  EXPECT_FALSE(Child<QCheckBox>(&Frame, "PreserveTimeDirsCheck")->isEnabled());
  EXPECT_EQ(Child<QComboBox>(&Frame, "SpeedCombo")->currentText(), QStringLiteral("100"));

  TCopyParamType Result = Frame.GetParams();
  EXPECT_TRUE(Result == Params);
  EXPECT_EQ(Result.IncludeFileMask.Masks, UnicodeString(L"*.c | *.o"));
  EXPECT_EQ(Result.CPSLimit, 100u * 1024);
}
//---------------------------------------------------------------------------
TEST(CopyParamsFrameTest, ControlsChangeTheParams)
{
  TCopyParamsFrame Frame(nullptr);
  Frame.SetParams(TCopyParamType());
  Child<QRadioButton>(&Frame, "TMTextButton")->click();
  Child<QCheckBox>(&Frame, "PreserveRightsCheck")->click();
  QLineEdit * RightsEdit = Child<QLineEdit>(&Frame, "RightsEdit");
  RightsEdit->setFocus();
  RightsEdit->clear();
  QTest::keyClicks(RightsEdit, QStringLiteral("600"));
  Frame.Validate();
  EXPECT_EQ(RightsEdit->text(), QStringLiteral("rw-------"));
  Child<QComboBox>(&Frame, "SpeedCombo")->setEditText(QStringLiteral("Unlimited"));
  TCopyParamType Result = Frame.GetParams();
  EXPECT_EQ(Result.TransferMode, tmAscii);
  EXPECT_TRUE(Result.PreserveRights);
  EXPECT_EQ(Result.Rights.Number, 0600);
  EXPECT_EQ(Result.CPSLimit, 0u);
}
//---------------------------------------------------------------------------
TEST(CopyParamsFrameTest, AttributesDisableControls)
{
  TCopyParamsFrame Frame(nullptr);
  Frame.SetCopyParamAttrs(cpaNoTransferMode | cpaNoRights);
  EXPECT_FALSE(Child<QGroupBox>(&Frame, "TransferModeGroup")->isEnabled());
  EXPECT_FALSE(Child<QCheckBox>(&Frame, "PreserveRightsCheck")->isEnabled());
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "PreserveTimeCheck")->isEnabled());
  Frame.SetCopyParamAttrs(cpaIncludeMaskOnly);
  EXPECT_FALSE(Child<QGroupBox>(&Frame, "CommonPropertiesGroup")->isEnabled());
  EXPECT_FALSE(Child<QGroupBox>(&Frame, "ChangeCaseGroup")->isEnabled());
  EXPECT_TRUE(Child<QComboBox>(&Frame, "IncludeFileMaskCombo")->isEnabled());
}
//---------------------------------------------------------------------------
TEST(CopyParamsFrameTest, InvalidValuesAreReported)
{
  TCopyParamsFrame Frame(nullptr);
  Frame.SetParams(TCopyParamType());
  Child<QComboBox>(&Frame, "IncludeFileMaskCombo")->setEditText(QStringLiteral("*.txt | | *.c"));
  QString Error;
  WhenModal([&](QWidget * Dialog)
  {
    Error = MessageText(Dialog);
    Child<QPushButton>(Dialog, "OK")->click();
  });
  EXPECT_THROW(Frame.Validate(), EAbort);
  EXPECT_FALSE(Error.isEmpty());

  Child<QComboBox>(&Frame, "IncludeFileMaskCombo")->setEditText(QString());
  Child<QComboBox>(&Frame, "SpeedCombo")->setEditText(QStringLiteral("fast"));
  Error.clear();
  WhenModal([&](QWidget * Dialog)
  {
    Error = MessageText(Dialog);
    Child<QPushButton>(Dialog, "OK")->click();
  });
  EXPECT_THROW(Frame.Validate(), EAbort);
  EXPECT_FALSE(Error.isEmpty());
}
//---------------------------------------------------------------------------
TEST(CopyDialogTest, MoveWithFileMaskInTheTarget)
{
  std::unique_ptr<TStrings> FileList(new TStringList());
  FileList->Add(L"/remote/a.txt");
  FileList->Add(L"/remote/b.txt");
  UnicodeString TargetDirectory = L"/tmp";
  TGUICopyParamType Params;
  int OutputOptions = 0;
  QString Title, Label, Text;
  bool QueueEnabled = true;
  WhenModal([&](QWidget * Dialog)
  {
    Title = Dialog->windowTitle();
    Label = Child<QLabel>(Dialog, "DirectoryLabel")->text();
    QComboBox * Edit = Child<QComboBox>(Dialog, "LocalDirectoryEdit");
    Text = Edit->currentText();
    QueueEnabled = Child<QCheckBox>(Dialog, "QueueCheck2")->isEnabled();
    Edit->setEditText(QStringLiteral("/tmp/*.bak"));
    Child<QCheckBox>(Dialog, "NeverShowAgainCheck")->click();
    Child<QPushButton>(Dialog, "OkButton")->click();
  });
  bool Result = DoCopyDialog(false, true, FileList.get(), TargetDirectory, &Params,
    coDisableQueue | coDoNotShowAgain, 0, &OutputOptions);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Title, QStringLiteral("Download and Delete"));
  EXPECT_EQ(Label, QStringLiteral("Download 2 files to local directory and delete originals:"));
  EXPECT_EQ(Text, QStringLiteral("/tmp/*.*"));
  EXPECT_FALSE(QueueEnabled);
  EXPECT_EQ(TargetDirectory, UnicodeString(L"/tmp/"));
  EXPECT_EQ(Params.FileMask, UnicodeString(L"*.bak"));
  EXPECT_TRUE(FLAGSET(OutputOptions, cooDoNotShowAgain));
}
//---------------------------------------------------------------------------
TEST(CopyDialogTest, SingleFileToNewName)
{
  std::unique_ptr<TStrings> FileList(new TStringList());
  FileList->Add(L"/home/user/a.txt");
  UnicodeString TargetDirectory = L"/remote";
  TGUICopyParamType Params;
  QString Label;
  bool NeverShowAgainVisible = true;
  WhenModal([&](QWidget * Dialog)
  {
    Label = Child<QLabel>(Dialog, "DirectoryLabel")->text();
    NeverShowAgainVisible = Child<QCheckBox>(Dialog, "NeverShowAgainCheck")->isVisible();
    Child<QComboBox>(Dialog, "RemoteDirectoryEdit")->setEditText(QStringLiteral("/remote/sub/b.txt"));
    Child<QPushButton>(Dialog, "OkButton")->click();
  });
  ASSERT_TRUE(DoCopyDialog(true, false, FileList.get(), TargetDirectory, &Params, 0, 0, NULL));
  EXPECT_EQ(Label, QStringLiteral("Upload file 'a.txt' to remote directory:"));
  EXPECT_FALSE(NeverShowAgainVisible);
  EXPECT_EQ(TargetDirectory, UnicodeString(L"/remote/sub/"));
  EXPECT_EQ(Params.FileMask, UnicodeString(L"b.txt"));
}
//---------------------------------------------------------------------------
TEST(CopyDialogTest, TransferSettingsCustomized)
{
  std::unique_ptr<TStrings> FileList(new TStringList());
  FileList->Add(L"/home/user/a.txt");
  UnicodeString TargetDirectory = L"/remote";
  TGUICopyParamType Params;
  QString InfoBefore, InfoAfter;
  WhenModal([&](QWidget * Dialog)
  {
    InfoBefore = Child<QLabel>(Dialog, "CopyParamLabel")->text();
    WhenModal([&](QWidget * Settings)
    {
      Child<QCheckBox>(Settings, "NewerOnlyCheck")->click();
      Child<QPushButton>(Settings, "OkButton")->click();
      // Once the settings dialog returned
      QTimer::singleShot(50, [&, Dialog]()
      {
        InfoAfter = Child<QLabel>(Dialog, "CopyParamLabel")->text();
        Child<QPushButton>(Dialog, "OkButton")->click();
      });
    });
    Child<QPushButton>(Dialog, "TransferSettingsButton")->click();
  });
  ASSERT_TRUE(DoCopyDialog(true, false, FileList.get(), TargetDirectory, &Params, 0, 0, NULL));
  EXPECT_FALSE(InfoBefore.contains(QStringLiteral("New and updated files only")));
  EXPECT_TRUE(InfoAfter.contains(QStringLiteral("New and updated files only")));
  EXPECT_TRUE(Params.NewerOnly);
}
//---------------------------------------------------------------------------
TEST(EditMaskDialogTest, ComposesTheMask)
{
  TFileMasks Mask(L"*.txt | *.bak");
  QString Composed;
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Child<QPlainTextEdit>(Dialog, "IncludeFileMasksMemo")->toPlainText(), QStringLiteral("*.txt"));
    EXPECT_EQ(Child<QPlainTextEdit>(Dialog, "ExcludeFileMasksMemo")->toPlainText(), QStringLiteral("*.bak"));
    Child<QPlainTextEdit>(Dialog, "ExcludeDirectoryMasksMemo")->setPlainText(QStringLiteral("tmp/"));
    Child<QPlainTextEdit>(Dialog, "IncludeFileMasksMemo")->setPlainText(QStringLiteral("*.txt\n*.md"));
    Composed = Child<QPlainTextEdit>(Dialog, "MaskMemo")->toPlainText();
    Child<QCheckBox>(Dialog, "ExcludeDirectoryAllCheck")->click();
    EXPECT_EQ(Child<QPlainTextEdit>(Dialog, "ExcludeDirectoryMasksMemo")->toPlainText(), QStringLiteral("*"));
    Child<QCheckBox>(Dialog, "ExcludeDirectoryAllCheck")->click();
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  ASSERT_TRUE(DoEditMaskDialog(Mask));
  EXPECT_EQ(Composed, QStringLiteral("*.txt; *.md | *.bak; tmp/"));
  EXPECT_EQ(Mask.Masks, UnicodeString(L"*.txt; *.md | *.bak; tmp/"));
}
//---------------------------------------------------------------------------
