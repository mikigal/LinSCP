// The message dialog and the functions that show it (upstream has them in
// forms/MessageDlg.cpp and windows/WinInterface.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include "MessageDialog.h"

#include <Translations.h>

#include <QApplication>
#include <QCheckBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
// The widest text of the message, in average character widths (upstream
// limits the dialog to a width in pixels at 96 DPI)
static const int MaxTextWidthChars = 60;
// The time of the timeout after the mouse moves (upstream's SuspendTime)
static const int TimeoutSuspendTime = 30 * MSecsPerSec;
//---------------------------------------------------------------------------
TMessageParams::TMessageParams(unsigned int AParams)
{
  Reset();
  Params = AParams;
}
//---------------------------------------------------------------------------
TMessageParams::TMessageParams(const TQueryParams * AParams)
{
  Reset();

  if (AParams != NULL)
  {
    Aliases = AParams->Aliases;
    AliasesCount = AParams->AliasesCount;
    Timer = AParams->Timer;
    TimerEvent = AParams->TimerEvent;
    TimerMessage = AParams->TimerMessage;
    TimerAnswers = AParams->TimerAnswers;
    TimerQueryType = AParams->TimerQueryType;
    Timeout = AParams->Timeout;
    TimeoutAnswer = AParams->TimeoutAnswer;
    TimeoutResponse = AParams->TimeoutResponse;

    if (FLAGSET(AParams->Params, qpNeverAskAgainCheck))
    {
      Params |= mpNeverAskAgainCheck;
    }
    if (FLAGSET(AParams->Params, qpAllowContinueOnError))
    {
      Params |= mpAllowContinueOnError;
    }
  }
}
//---------------------------------------------------------------------------
void TMessageParams::Reset()
{
  Params = 0;
  Aliases = NULL;
  AliasesCount = 0;
  Timer = 0;
  TimerEvent = NULL;
  TimerMessage = L"";
  TimerAnswers = 0;
  TimerQueryType = static_cast<TQueryType>(-1);
  Timeout = 0;
  TimeoutAnswer = 0;
  TimeoutResponse = 0;
  NeverAskAgainTitle = L"";
  NeverAskAgainAnswer = 0;
  NeverAskAgainCheckedInitially = false;
  CustomCaption = L"";
}
//---------------------------------------------------------------------------
static bool IsPositiveAnswer(unsigned int Answer)
{
  return (Answer == qaYes) || (Answer == qaOK) || (Answer == qaYesToAll);
}
//---------------------------------------------------------------------------
static int CalculateWidth(UnicodeString Text, void * Arg)
{
  return static_cast<QFontMetrics *>(Arg)->horizontalAdvance(ToQString(Text));
}
//---------------------------------------------------------------------------
static QLabel * CreateMessageLabel(QWidget * Parent, const QString & Name, const QString & Text)
{
  QLabel * Label = new QLabel(Parent);
  Label->setObjectName(Name);
  Label->setTextFormat(Qt::PlainText);
  Label->setWordWrap(true);
  Label->setTextInteractionFlags(Qt::TextSelectableByMouse);
  Label->setText(Text);
  // Wrapped at the limit, otherwise as wide as the longest line
  QFontMetrics Metrics(Label->font());
  int MaxWidth = Metrics.averageCharWidth() * MaxTextWidthChars;
  QRect Bounds = Metrics.boundingRect(QRect(0, 0, MaxWidth, INT_MAX), Qt::TextWordWrap | Qt::TextExpandTabs, Text);
  Label->setMinimumWidth(std::min(Bounds.width() + 1, MaxWidth));
  return Label;
}
//---------------------------------------------------------------------------
TMessageForm::TMessageForm(const UnicodeString & Msg, TStrings * MoreMessages, TQueryType Type,
    unsigned int Answers, const TMessageParams * Params, QWidget * Parent) :
  QDialog(Parent)
{
  setObjectName(QStringLiteral("MessageForm"));
  FAnswers = Answers;
  FParams = Params;
  FNeverAskAgainCheck = nullptr;
  FTimer = nullptr;
  FTimeoutTimer = nullptr;
  FTimeoutButton = nullptr;
  FTimeoutLeft = 0;
  FTimeoutResponse = 0;

  // Upstream adds the Help and Report buttons, which need the online
  // documentation and the bug report form of WinSCP
  Answers &= ~(qaHelp | qaReport);

  unsigned int DefaultAnswer;
  // As upstream: Yes is the default where it is aliased with OK (the host
  // key prompt)
  if (FLAGSET(Answers, qaYes))
  {
    DefaultAnswer = qaYes;
  }
  else if (FLAGSET(Answers, qaOK))
  {
    DefaultAnswer = qaOK;
  }
  else
  {
    DefaultAnswer = qaRetry;
  }

  QStyle::StandardPixmap Icon;
  UnicodeString Caption;
  switch (Type)
  {
    case qtConfirmation:
      Icon = QStyle::SP_MessageBoxQuestion;
      Caption = DelphiResourceString("Vcl_Consts_SMsgDlgConfirm", L"Confirm");
      break;
    case qtWarning:
      Icon = QStyle::SP_MessageBoxWarning;
      Caption = DelphiResourceString("Vcl_Consts_SMsgDlgWarning", L"Warning");
      break;
    case qtError:
      Icon = QStyle::SP_MessageBoxCritical;
      Caption = DelphiResourceString("Vcl_Consts_SMsgDlgError", L"Error");
      break;
    default:
      DebugAssert(Type == qtInformation);
      Icon = QStyle::SP_MessageBoxInformation;
      Caption = DelphiResourceString("Vcl_Consts_SMsgDlgInformation", L"Information");
      break;
  }
  if ((Params != NULL) && !Params->CustomCaption.IsEmpty())
  {
    Caption = Params->CustomCaption;
  }
  setWindowTitle(ToQString(Caption));

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  QHBoxLayout * ContentsLayout = new QHBoxLayout();
  MainLayout->addLayout(ContentsLayout);

  QLabel * Image = new QLabel(this);
  Image->setObjectName(QStringLiteral("Image"));
  int IconSize = style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, this);
  Image->setPixmap(style()->standardIcon(Icon, nullptr, this).pixmap(IconSize, IconSize));
  Image->setAlignment(Qt::AlignTop);
  ContentsLayout->addWidget(Image, 0, Qt::AlignTop);
  ContentsLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutHorizontalSpacing, nullptr, this));

  QVBoxLayout * TextLayout = new QVBoxLayout();
  ContentsLayout->addLayout(TextLayout, 1);

  UnicodeString BodyMsg = RemoveInteractiveMsgTag(Msg);
  UnicodeString MainMsg;
  if (ExtractMainInstructions(BodyMsg, MainMsg))
  {
    FMessageText = MainMsg + BodyMsg;
    BodyMsg = BodyMsg.TrimLeft();
  }
  else
  {
    FMessageText = BodyMsg;
  }
  ApplyTabs(FMessageText, L' ', NULL, NULL);
  // Tabs align the columns of the body, padded with hair spaces, as upstream
  QFontMetrics Metrics(font());
  ApplyTabs(BodyMsg, L'\x200A', CalculateWidth, &Metrics);

  if (!MainMsg.IsEmpty())
  {
    QLabel * MainLabel = CreateMessageLabel(this, QStringLiteral("MainMessage"), ToQString(MainMsg.TrimRight()));
    QFont MainFont = MainLabel->font();
    MainFont.setBold(true);
    MainFont.setPointSizeF(MainFont.pointSizeF() * 1.15);
    MainLabel->setFont(MainFont);
    TextLayout->addWidget(MainLabel);
  }
  if (!BodyMsg.Trim().IsEmpty())
  {
    TextLayout->addWidget(CreateMessageLabel(this, QStringLiteral("Message"), ToQString(BodyMsg.TrimRight())));
  }

  if ((MoreMessages != NULL) && (MoreMessages->Count > 0))
  {
    QPlainTextEdit * Memo = new QPlainTextEdit(this);
    Memo->setObjectName(QStringLiteral("MessageMemo"));
    Memo->setReadOnly(true);
    Memo->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    FMoreMessagesText = MoreMessages->Text;
    Memo->setPlainText(ToQString(FMoreMessagesText));
    Memo->setFixedHeight(Metrics.lineSpacing() * 5 + Memo->frameWidth() * 2 + 8);
    Memo->setMinimumWidth(Metrics.averageCharWidth() * 50);
    TextLayout->addWidget(Memo);
  }
  TextLayout->addStretch(1);

  MainLayout->addSpacing(style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing, nullptr, this));
  QHBoxLayout * ButtonLayout = new QHBoxLayout();
  MainLayout->addLayout(ButtonLayout);

  bool HasNeverAskAgain = (Params != NULL) && FLAGSET(Params->Params, mpNeverAskAgainCheck);
  if (HasNeverAskAgain)
  {
    UnicodeString NeverAskAgainCaption =
      !Params->NeverAskAgainTitle.IsEmpty() ?
        Params->NeverAskAgainTitle :
        // qaOK | qaIgnore is used, when custom "non-answer" button is required
        LoadStr(((Answers == qaOK) || (Answers == (qaOK | qaIgnore))) ? NEVER_SHOW_AGAIN : NEVER_ASK_AGAIN);
    FNeverAskAgainCheck = new QCheckBox(ToQString(NeverAskAgainCaption), this);
    FNeverAskAgainCheck->setObjectName(QStringLiteral("NeverAskAgainCheck"));
    ButtonLayout->addWidget(FNeverAskAgainCheck);
  }
  ButtonLayout->addStretch(1);

  unsigned int TimeoutAnswer = (Params != NULL) ? Params->TimeoutAnswer : 0;
  QPushButton * FirstButton = nullptr;
  for (unsigned int Answer = qaFirst; Answer <= qaLast; Answer = Answer << 1)
  {
    if (FLAGSET(Answers, Answer))
    {
      UnicodeString Name;
      UnicodeString ButtonCaption;
      AnswerNameAndCaption(Answer, Name, ButtonCaption);

      TButtonSubmitEvent OnSubmit = NULL;
      if (Params != NULL)
      {
        for (unsigned int Index = 0; Index < Params->AliasesCount; Index++)
        {
          const TQueryButtonAlias & Alias = Params->Aliases[Index];
          if (Alias.Button == Answer)
          {
            if (!Alias.Alias.IsEmpty())
            {
              ButtonCaption = Alias.Alias;
            }
            if (!Alias.ActionAlias.IsEmpty())
            {
              ButtonCaption = Alias.ActionAlias;
            }
            OnSubmit = Alias.OnSubmit;
            // Upstream groups the answers that are grouped with another one
            // (GroupWith) into the drop down menu of its button; here they
            // have buttons of their own
            break;
          }
        }
      }

      QPushButton * Button = new QPushButton(ToQString(ButtonCaption), this);
      Button->setObjectName(ToQString(Name));
      Button->setAutoDefault(false);
      Button->setDefault(Answer == DefaultAnswer);
      if (OnSubmit != NULL)
      {
        FSubmitEvents[Button] = OnSubmit;
      }
      connect(Button, &QPushButton::clicked, this, [this, Button, Answer]() { ButtonClicked(Button, Answer); });
      ButtonLayout->addWidget(Button);
      if (FirstButton == nullptr)
      {
        FirstButton = Button;
      }
      if ((Answer == TimeoutAnswer) && (Params != NULL) && (Params->Timeout > 0))
      {
        FTimeoutButton = Button;
      }
    }
  }
  if (FirstButton != nullptr)
  {
    FirstButton->setFocus();
  }

  if (FNeverAskAgainCheck != nullptr)
  {
    FNeverAskAgainCheck->setChecked(Params->NeverAskAgainCheckedInitially);
    NeverAskAgainCheckToggled(FNeverAskAgainCheck->isChecked());
    connect(FNeverAskAgainCheck, &QCheckBox::toggled, this, [this](bool Checked) { NeverAskAgainCheckToggled(Checked); });
  }

  if ((Params != NULL) && (Params->Timer > 0))
  {
    FTimer = new QTimer(this);
    FTimer->setObjectName(QStringLiteral("MessageTimer"));
    FTimer->setInterval(static_cast<int>(Params->Timer));
    connect(FTimer, &QTimer::timeout, this, [this]() { TimerElapsed(); });
    FTimer->start();
  }

  if (FTimeoutButton != nullptr)
  {
    FTimeoutResponse = Params->TimeoutResponse;
    InitiateTimeout(static_cast<int>(Params->Timeout));
  }

  setSizeGripEnabled(false);
  layout()->setSizeConstraint(QLayout::SetFixedSize);
}
//---------------------------------------------------------------------------
void TMessageForm::ButtonClicked(QAbstractButton * Button, unsigned int Answer)
{
  std::map<QAbstractButton *, TButtonSubmitEvent>::iterator Submit = FSubmitEvents.find(Button);
  if (Submit != FSubmitEvents.end())
  {
    // The handlers of the core do not use the sender
    unsigned int SubmitAnswer = 0;
    Submit->second(NULL, SubmitAnswer);
    if (SubmitAnswer != 0)
    {
      done(static_cast<int>(SubmitAnswer));
    }
  }
  else
  {
    done(static_cast<int>(Answer));
  }
}
//---------------------------------------------------------------------------
void TMessageForm::NeverAskAgainCheckToggled(bool Checked)
{
  unsigned int PositiveAnswer = 0;
  if (Checked)
  {
    if (FParams->NeverAskAgainAnswer > 0)
    {
      PositiveAnswer = FParams->NeverAskAgainAnswer;
    }
    else
    {
      for (unsigned int Answer = qaFirst; (PositiveAnswer == 0) && (Answer <= qaLast); Answer = Answer << 1)
      {
        if (FLAGSET(FAnswers, Answer) && IsPositiveAnswer(Answer))
        {
          PositiveAnswer = Answer;
        }
      }
    }
    DebugAssert(PositiveAnswer != 0);
  }

  // Only the positive answer can be remembered
  for (unsigned int Answer = qaFirst; Answer <= qaLast; Answer = Answer << 1)
  {
    if (FLAGSET(FAnswers, Answer) && (Answer != qaCancel))
    {
      UnicodeString Name;
      UnicodeString Caption;
      AnswerNameAndCaption(Answer, Name, Caption);
      QPushButton * Button = findChild<QPushButton *>(ToQString(Name));
      if (Button != nullptr)
      {
        Button->setEnabled(!Checked || (Answer == PositiveAnswer));
      }
    }
  }
}
//---------------------------------------------------------------------------
void TMessageForm::TimerElapsed()
{
  if (FParams->TimerEvent != NULL)
  {
    unsigned int Result = 0;
    FParams->TimerEvent(Result);
    if (Result != 0)
    {
      done(static_cast<int>(Result));
    }
  }
}
//---------------------------------------------------------------------------
void TMessageForm::InitiateTimeout(int Timeout)
{
  FTimeoutLeft = Timeout;
  FTimeoutButtonCaption = FTimeoutButton->text();
  FTimeoutTimer = new QTimer(this);
  FTimeoutTimer->setInterval(MSecsPerSec);
  connect(FTimeoutTimer, &QTimer::timeout, this, [this]() { TimeoutElapsed(); });
  FTimeoutTimer->start();
  // A key press or a click stops the countdown, a mouse move restarts it
  // with a longer time
  qApp->installEventFilter(this);
  UpdateTimeoutButton();
}
//---------------------------------------------------------------------------
void TMessageForm::TimeoutElapsed()
{
  if (FTimeoutLeft <= FTimeoutTimer->interval())
  {
    StopTimeout();
    unsigned int Answer = (FTimeoutResponse != 0) ? FTimeoutResponse : FParams->TimeoutAnswer;
    done(static_cast<int>(Answer));
  }
  else
  {
    FTimeoutLeft -= FTimeoutTimer->interval();
    UpdateTimeoutButton();
  }
}
//---------------------------------------------------------------------------
void TMessageForm::StopTimeout()
{
  if ((FTimeoutTimer != nullptr) && FTimeoutTimer->isActive())
  {
    FTimeoutTimer->stop();
    qApp->removeEventFilter(this);
    UpdateTimeoutButton();
  }
}
//---------------------------------------------------------------------------
void TMessageForm::UpdateTimeoutButton()
{
  if (FTimeoutTimer->isActive())
  {
    UnicodeString Caption = FMTLOAD(TIMEOUT_BUTTON, (ToUnicodeString(FTimeoutButtonCaption), int(FTimeoutLeft / MSecsPerSec)));
    FTimeoutButton->setText(ToQString(Caption));
  }
  else
  {
    FTimeoutButton->setText(FTimeoutButtonCaption);
  }
}
//---------------------------------------------------------------------------
bool TMessageForm::eventFilter(QObject * Object, QEvent * Event)
{
  switch (Event->type())
  {
    case QEvent::MouseMove:
      {
        QPointF Position = static_cast<QMouseEvent *>(Event)->globalPosition();
        if (FTimeoutCursorPos.isNull())
        {
          FTimeoutCursorPos = Position;
        }
        else if ((Position - FTimeoutCursorPos).manhattanLength() > 8)
        {
          FTimeoutCursorPos = Position;
          FTimeoutLeft = std::max(FParams->Timeout, static_cast<unsigned int>(TimeoutSuspendTime));
          UpdateTimeoutButton();
        }
      }
      break;

    case QEvent::MouseButtonPress:
    case QEvent::KeyPress:
      StopTimeout();
      break;

    default:
      break;
  }
  return QDialog::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
UnicodeString TMessageForm::GetFormText()
{
  UnicodeString DividerLine = GetDividerLine() + sLineBreak;
  UnicodeString ButtonCaptions;
  for (QPushButton * Button : findChildren<QPushButton *>())
  {
    ButtonCaptions += ToUnicodeString(Button->text()) + UnicodeString::StringOfChar(L' ', 3);
  }
  ButtonCaptions = ReplaceStr(ButtonCaptions, L"&", L"");
  UnicodeString MoreMessages;
  if (!FMoreMessagesText.IsEmpty())
  {
    MoreMessages = FMoreMessagesText + DividerLine;
  }
  return
    DividerLine + ToUnicodeString(windowTitle()) + sLineBreak +
    DividerLine + FMessageText + sLineBreak +
    DividerLine + MoreMessages +
    ButtonCaptions + sLineBreak +
    DividerLine;
}
//---------------------------------------------------------------------------
void TMessageForm::keyPressEvent(QKeyEvent * Event)
{
  if (Event->matches(QKeySequence::Copy))
  {
    AppLog(L"Copying message to clipboard");
    CopyToClipboard(GetFormText());
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TMessageForm::done(int Result)
{
  StopTimeout();
  if (FTimer != nullptr)
  {
    FTimer->stop();
  }
  QDialog::done(Result);
}
//---------------------------------------------------------------------------
unsigned int TMessageForm::Execute()
{
  unsigned int Answer = static_cast<unsigned int>(exec());
  // Escape and the close button of the window end the dialog without an
  // answer; the query has its own "cancel" answer
  if (Answer == 0)
  {
    Answer = CancelAnswer(FAnswers);
  }
  if ((FNeverAskAgainCheck != nullptr) && FNeverAskAgainCheck->isChecked())
  {
    bool PositiveAnswer =
      (FParams->NeverAskAgainAnswer > 0) ?
        (Answer == FParams->NeverAskAgainAnswer) :
        IsPositiveAnswer(Answer);
    if (PositiveAnswer)
    {
      Answer = qaNeverAskAgain;
    }
  }
  return Answer;
}
//---------------------------------------------------------------------------
unsigned int MoreMessageDialog(const UnicodeString & Message, TStrings * MoreMessages,
  TQueryType Type, unsigned int Answers, const TMessageParams * Params)
{
  AppLogFmt(L"Message dialog: %s", (Message));
  UnicodeString AMessage = Message;
  if ((Params != NULL) && (Params->Timer > 0))
  {
    if (Params->TimerAnswers > 0)
    {
      Answers = Params->TimerAnswers;
    }
    if (Params->TimerQueryType >= 0)
    {
      Type = Params->TimerQueryType;
    }
    if (!Params->TimerMessage.IsEmpty())
    {
      AMessage = Params->TimerMessage;
    }
  }
  unsigned int Result;
  {
    TMessageForm Dialog(AMessage, MoreMessages, Type, Answers, Params, QApplication::activeWindow());
    Result = Dialog.Execute();
  }
  AppLogFmt(L"Message dialog answer: %d", (Integer(Result)));
  return Result;
}
//---------------------------------------------------------------------------
unsigned int MessageDialog(const UnicodeString & Msg, TQueryType Type,
  unsigned int Answers, const TMessageParams * Params)
{
  return MoreMessageDialog(Msg, NULL, Type, Answers, Params);
}
//---------------------------------------------------------------------------
unsigned int MessageDialog(int Ident, TQueryType Type,
  unsigned int Answers, const TMessageParams * Params)
{
  return MessageDialog(LoadStr(Ident), Type, Answers, Params);
}
//---------------------------------------------------------------------------
unsigned int SimpleErrorDialog(const UnicodeString & Msg, const UnicodeString & MoreMessages)
{
  std::unique_ptr<TStrings> More;
  if (!MoreMessages.IsEmpty())
  {
    More.reset(TextToStringList(MoreMessages));
  }
  return MoreMessageDialog(Msg, More.get(), qtError, qaOK);
}
//---------------------------------------------------------------------------
unsigned int ExceptionMessageDialog(Exception * E, TQueryType Type,
  const UnicodeString & MessageFormat, unsigned int Answers, const TMessageParams * Params)
{
  TStrings * MoreMessages = NULL;
  ExtException * EE = dynamic_cast<ExtException *>(E);
  if (EE != NULL)
  {
    MoreMessages = EE->MoreMessages;
  }
  UnicodeString Message;
  // Called only for exceptions to display (ShouldDisplayException)
  DebugCheck(ExceptionMessageFormatted(E, Message));
  if (!MessageFormat.IsEmpty())
  {
    Message = FORMAT(MessageFormat, (UnformatMessage(Message)));
  }
  return MoreMessageDialog(Message, MoreMessages, Type, Answers, Params);
}
//---------------------------------------------------------------------------
unsigned int FatalExceptionMessageDialog(
  Exception * E, TQueryType Type, const UnicodeString & MessageFormat, unsigned int Answers,
  const TMessageParams * Params)
{
  DebugAssert(FLAGCLEAR(Answers, qaRetry));
  Answers |= qaRetry;
  TQueryButtonAlias Aliases[1];
  Aliases[0].Button = qaRetry;
  Aliases[0].Alias = LoadStr(RECONNECT_BUTTON);
  TMessageParams AParams;
  if (Params != NULL)
  {
    AParams = *Params;
  }
  DebugAssert(AParams.Aliases == NULL);
  AParams.Aliases = Aliases;
  AParams.AliasesCount = std::size(Aliases);
  return ExceptionMessageDialog(E, Type, MessageFormat, Answers, &AParams);
}
//---------------------------------------------------------------------------
