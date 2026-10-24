// The message dialog (upstream's TMessageForm, forms/MessageDlg.cpp): a
// message with an optional main instruction and more messages, and the
// buttons of the answers of a query.
//---------------------------------------------------------------------------
#ifndef MessageDialogH
#define MessageDialogH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QDialog>

#include <map>
//---------------------------------------------------------------------------
class QAbstractButton;
class QCheckBox;
class QPushButton;
class QTimer;
//---------------------------------------------------------------------------
class TMessageForm : public QDialog
{
  Q_OBJECT

public:
  TMessageForm(const UnicodeString & Msg, TStrings * MoreMessages, TQueryType Type,
    unsigned int Answers, const TMessageParams * Params, QWidget * Parent);

  unsigned int Execute();

protected:
  virtual void keyPressEvent(QKeyEvent * Event) override;
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;
  virtual void done(int Result) override;

private:
  unsigned int FAnswers;
  const TMessageParams * FParams;
  UnicodeString FMessageText;
  UnicodeString FMoreMessagesText;
  std::map<QAbstractButton *, TButtonSubmitEvent> FSubmitEvents;
  QCheckBox * FNeverAskAgainCheck;
  QTimer * FTimer;
  // The countdown of the timeout (Params->Timeout) on the timeout button
  QTimer * FTimeoutTimer;
  QPushButton * FTimeoutButton;
  QString FTimeoutButtonCaption;
  int FTimeoutLeft;
  unsigned int FTimeoutResponse;
  QPointF FTimeoutCursorPos;

  void ButtonClicked(QAbstractButton * Button, unsigned int Answer);
  void NeverAskAgainCheckToggled(bool Checked);
  void TimerElapsed();
  void InitiateTimeout(int Timeout);
  void TimeoutElapsed();
  void StopTimeout();
  void UpdateTimeoutButton();
  UnicodeString GetFormText();
};
//---------------------------------------------------------------------------
#endif
