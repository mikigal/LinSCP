// L3 tests of the message dialog (dialogs/MessageDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <MessageDialog.h>

#include <QCheckBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QStringList ButtonNames(QWidget * Dialog)
{
  QStringList Result;
  for (QPushButton * Button : Dialog->findChildren<QPushButton *>())
  {
    Result.append(Button->objectName());
  }
  return Result;
}
//---------------------------------------------------------------------------
QString ImageName(QWidget * Dialog)
{
  return Child<QLabel>(Dialog, "Image")->property("automationImageName").toString();
}
//---------------------------------------------------------------------------
void ClickWhenModal(const char * Name)
{
  WhenModal([Name](QWidget * Dialog) { Child<QPushButton>(Dialog, Name)->click(); });
}
//---------------------------------------------------------------------------
class TSubmitHandler
{
public:
  int Submits = 0;
  unsigned int Answer = 0;

  void Submit(TObject * /*Sender*/, unsigned int & AAnswer)
  {
    Submits++;
    AAnswer = Answer;
  }

  void Timer(unsigned int & Result)
  {
    Submits++;
    if (Submits >= 2)
    {
      Result = qaAbort;
    }
  }
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(MessageDialogTest, ButtonsOfAnswersInOrder)
{
  QStringList Names;
  QString Image;
  bool YesDefault = false;
  WhenModal([&](QWidget * Dialog)
  {
    Names = ButtonNames(Dialog);
    Image = ImageName(Dialog);
    YesDefault = Child<QPushButton>(Dialog, "Yes")->isDefault();
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("MessageForm"));
    EXPECT_EQ(Dialog->windowTitle(), QStringLiteral("Confirm"));
    Child<QPushButton>(Dialog, "No")->click();
  });
  unsigned int Answer = MessageDialog(L"Continue?", qtConfirmation, qaCancel | qaNo | qaYes);
  EXPECT_EQ(Answer, qaNo);
  EXPECT_EQ(Names, (QStringList{ QStringLiteral("Yes"), QStringLiteral("No"), QStringLiteral("Cancel") }));
  EXPECT_TRUE(YesDefault);
  EXPECT_EQ(Image, QStringLiteral("Help Blue"));
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, ImagesOfTypes)
{
  // upstream's images of the types, or of the parameters
  std::vector<std::pair<TQueryType, QString>> Types = {
    { qtWarning, QStringLiteral("Warning") }, { qtError, QStringLiteral("Error") },
    { qtInformation, QStringLiteral("Information") } };
  for (const auto & Type : Types)
  {
    QString Image;
    WhenModal([&](QWidget * Dialog)
    {
      Image = ImageName(Dialog);
      Child<QPushButton>(Dialog, "OK")->click();
    });
    MessageDialog(L"Message", Type.first, qaOK);
    EXPECT_EQ(Image, Type.second);
  }
  TMessageParams Params;
  Params.ImageName = L"Delete file";
  QString Image;
  WhenModal([&](QWidget * Dialog)
  {
    Image = ImageName(Dialog);
    Child<QPushButton>(Dialog, "Cancel")->click();
  });
  EXPECT_EQ(MessageDialog(L"Delete?", qtConfirmation, qaOK | qaCancel, &Params), qaCancel);
  EXPECT_EQ(Image, QStringLiteral("Delete file"));
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, EscapeIsTheCancelAnswer)
{
  WhenModal([](QWidget * Dialog) { static_cast<QDialog *>(Dialog)->reject(); });
  EXPECT_EQ(MessageDialog(L"Continue?", qtConfirmation, qaYes | qaNo | qaCancel), qaCancel);
  // Without Cancel, No cancels
  WhenModal([](QWidget * Dialog) { static_cast<QDialog *>(Dialog)->reject(); });
  EXPECT_EQ(MessageDialog(L"Continue?", qtConfirmation, qaYes | qaNo), qaNo);
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, MainInstructionsAndMoreMessages)
{
  QString MainText;
  QString BodyText;
  QString MemoText;
  WhenModal([&](QWidget * Dialog)
  {
    MainText = Child<QLabel>(Dialog, "MainMessage")->text();
    BodyText = Child<QLabel>(Dialog, "Message")->text();
    MemoText = Child<QPlainTextEdit>(Dialog, "MessageMemo")->toPlainText();
    Child<QPushButton>(Dialog, "OK")->click();
  });
  std::unique_ptr<TStrings> More(TextToStringList(L"first\nsecond"));
  unsigned int Answer =
    MoreMessageDialog(MainInstructions(L"Something failed.") + L"\n\nThe details.", More.get(), qtError, qaOK);
  EXPECT_EQ(Answer, qaOK);
  EXPECT_EQ(MainText, QStringLiteral("Something failed."));
  EXPECT_EQ(BodyText, QStringLiteral("The details."));
  EXPECT_EQ(MemoText, QStringLiteral("first\nsecond\n"));
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, AliasesRenameAndSubmit)
{
  TSubmitHandler Handler;
  Handler.Answer = qaSkip;
  TQueryButtonAlias Aliases[2];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = L"&Reconnect";
  Aliases[1].Button = qaIgnore;
  Aliases[1].Alias = L"&Copy";
  Aliases[1].OnSubmit = ::rtl::MethodPtr(&Handler, &TSubmitHandler::Submit);
  TMessageParams Params;
  Params.Aliases = Aliases;
  Params.AliasesCount = std::size(Aliases);

  QString RetryCaption;
  WhenModal([&](QWidget * Dialog)
  {
    RetryCaption = Child<QPushButton>(Dialog, "Retry")->text();
    Child<QPushButton>(Dialog, "Ignore")->click();
  });
  EXPECT_EQ(MessageDialog(L"Lost", qtError, qaRetry | qaIgnore | qaAbort, &Params), qaSkip);
  EXPECT_EQ(RetryCaption, QStringLiteral("&Reconnect"));
  EXPECT_EQ(Handler.Submits, 1);

  // A handler that gives no answer keeps the dialog open
  Handler.Answer = 0;
  WhenModal([&](QWidget * Dialog)
  {
    Child<QPushButton>(Dialog, "Ignore")->click();
    Child<QPushButton>(Dialog, "Abort")->click();
  });
  EXPECT_EQ(MessageDialog(L"Lost", qtError, qaRetry | qaIgnore | qaAbort, &Params), qaAbort);
  EXPECT_EQ(Handler.Submits, 2);
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, NeverAskAgain)
{
  TMessageParams Params(mpNeverAskAgainCheck);
  bool NoEnabled = true;
  WhenModal([&](QWidget * Dialog)
  {
    QCheckBox * Check = Child<QCheckBox>(Dialog, "NeverAskAgainCheck");
    EXPECT_EQ(Check->text(), QStringLiteral("Never ask me a&gain"));
    Check->setChecked(true);
    NoEnabled = Child<QPushButton>(Dialog, "No")->isEnabled();
    Child<QPushButton>(Dialog, "Yes")->click();
  });
  EXPECT_EQ(MessageDialog(L"Delete?", qtConfirmation, qaYes | qaNo, &Params), qaNeverAskAgain);
  EXPECT_FALSE(NoEnabled);

  // Unchecked, the answer is as clicked
  ClickWhenModal("Yes");
  EXPECT_EQ(MessageDialog(L"Delete?", qtConfirmation, qaYes | qaNo, &Params), qaYes);
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, Timeout)
{
  TMessageParams Params;
  Params.Timeout = 1000;
  Params.TimeoutAnswer = qaRetry;
  QString Caption;
  WhenModal([&](QWidget * Dialog) { Caption = Child<QPushButton>(Dialog, "Retry")->text(); });
  EXPECT_EQ(MessageDialog(L"Retry?", qtWarning, qaRetry | qaAbort, &Params), qaRetry);
  EXPECT_EQ(Caption, QStringLiteral("&Retry (1 s)"));
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, Timer)
{
  TSubmitHandler Handler;
  TMessageParams Params;
  Params.Timer = 20;
  Params.TimerEvent = ::rtl::MethodPtr(&Handler, &TSubmitHandler::Timer);
  Params.TimerMessage = L"Waiting";
  Params.TimerAnswers = qaAbort;
  QString Text;
  WhenModal([&](QWidget * Dialog) { Text = Child<QLabel>(Dialog, "Message")->text(); });
  EXPECT_EQ(MessageDialog(L"Original", qtConfirmation, qaOK, &Params), qaAbort);
  EXPECT_EQ(Text, QStringLiteral("Waiting"));
  EXPECT_EQ(Handler.Submits, 2);
}
//---------------------------------------------------------------------------
TEST(MessageDialogTest, ExceptionWithMoreMessages)
{
  QString MainText;
  QString MemoText;
  WhenModal([&](QWidget * Dialog)
  {
    QLabel * Label = Child<QLabel>(Dialog, "Message");
    MainText = (Label != nullptr) ? Label->text() : Child<QLabel>(Dialog, "MainMessage")->text();
    MemoText = Child<QPlainTextEdit>(Dialog, "MessageMemo")->toPlainText();
    Child<QPushButton>(Dialog, "OK")->click();
  });
  // Delphi-style objects are allocated on the heap (zero-filled)
  std::unique_ptr<ExtException> E(new ExtException(L"Connection failed.", L"Network error: Connection refused."));
  ShowExtendedException(E.get());
  EXPECT_EQ(MainText, QStringLiteral("Connection failed."));
  EXPECT_EQ(MemoText, QStringLiteral("Network error: Connection refused.\n"));
}
//---------------------------------------------------------------------------
