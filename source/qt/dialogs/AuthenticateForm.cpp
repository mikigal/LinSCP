// The authentication window (upstream's forms/Authenticate.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <Terminal.h>

#include "AuthenticateForm.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QEventLoop>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
TAuthenticateForm::TAuthenticateForm(QWidget * Parent) :
  QDialog(Parent != nullptr ? Parent : QApplication::activeWindow())
{
  setObjectName(QStringLiteral("AuthenticateForm"));
  FTerminal = NULL;
  FSessionData = NULL;
  FLoop = nullptr;
  FResult = arNone;

  QVBoxLayout * MainLayout = new QVBoxLayout(this);

  TopPanel = new QWidget(this);
  TopPanel->setObjectName(QStringLiteral("TopPanel"));
  QVBoxLayout * TopLayout = new QVBoxLayout(TopPanel);
  TopLayout->setContentsMargins(0, 0, 0, 0);
  LogView = new QListWidget(TopPanel);
  LogView->setObjectName(QStringLiteral("LogView"));
  LogView->setWordWrap(true);
  LogView->setSelectionMode(QAbstractItemView::NoSelection);
  LogView->setFocusPolicy(Qt::NoFocus);
  TopLayout->addWidget(LogView);
  MainLayout->addWidget(TopPanel, 1);

  PasswordPanel = new QWidget(this);
  PasswordPanel->setObjectName(QStringLiteral("PasswordPanel"));
  QVBoxLayout * PasswordLayout = new QVBoxLayout(PasswordPanel);
  PasswordLayout->setContentsMargins(0, 0, 0, 0);
  QWidget * PromptEditPanel = new QWidget(PasswordPanel);
  PromptEditPanel->setObjectName(QStringLiteral("PromptEditPanel"));
  PromptLayout = new QVBoxLayout(PromptEditPanel);
  PromptLayout->setContentsMargins(0, 0, 0, 0);
  PasswordLayout->addWidget(PromptEditPanel);
  SavePasswordCheck = new QCheckBox(tr("&Change stored password to this one"), PasswordPanel);
  SavePasswordCheck->setObjectName(QStringLiteral("SavePasswordCheck"));
  PasswordLayout->addWidget(SavePasswordCheck);
  SessionRememberPasswordCheck = new QCheckBox(tr("&Remember password for this session"), PasswordPanel);
  SessionRememberPasswordCheck->setObjectName(QStringLiteral("SessionRememberPasswordCheck"));
  PasswordLayout->addWidget(SessionRememberPasswordCheck);
  QHBoxLayout * PasswordButtons = new QHBoxLayout();
  PasswordButtons->addStretch(1);
  PasswordOKButton = new QPushButton(tr("OK"), PasswordPanel);
  PasswordOKButton->setObjectName(QStringLiteral("PasswordOKButton"));
  PasswordButtons->addWidget(PasswordOKButton);
  PasswordCancelButton = new QPushButton(tr("Cancel"), PasswordPanel);
  PasswordCancelButton->setObjectName(QStringLiteral("PasswordCancelButton"));
  PasswordButtons->addWidget(PasswordCancelButton);
  PasswordLayout->addLayout(PasswordButtons);
  PasswordPanel->hide();
  MainLayout->addWidget(PasswordPanel);
  connect(PasswordOKButton, &QPushButton::clicked, this, [this]() { Finish(arOK); });
  connect(PasswordCancelButton, &QPushButton::clicked, this, [this]() { Finish(arCancel); });

  BannerPanel = new QWidget(this);
  BannerPanel->setObjectName(QStringLiteral("BannerPanel"));
  QVBoxLayout * BannerLayout = new QVBoxLayout(BannerPanel);
  BannerLayout->setContentsMargins(0, 0, 0, 0);
  BannerMemo = new QPlainTextEdit(BannerPanel);
  BannerMemo->setObjectName(QStringLiteral("BannerMemo"));
  BannerMemo->setReadOnly(true);
  BannerLayout->addWidget(BannerMemo, 1);
  QHBoxLayout * BannerButtons = new QHBoxLayout();
  NeverShowAgainCheck = new QCheckBox(tr("&Never show this banner again"), BannerPanel);
  NeverShowAgainCheck->setObjectName(QStringLiteral("NeverShowAgainCheck"));
  BannerButtons->addWidget(NeverShowAgainCheck);
  BannerButtons->addStretch(1);
  BannerCloseButton = new QPushButton(tr("Continue"), BannerPanel);
  BannerCloseButton->setObjectName(QStringLiteral("BannerCloseButton"));
  BannerButtons->addWidget(BannerCloseButton);
  BannerLayout->addLayout(BannerButtons);
  BannerPanel->hide();
  MainLayout->addWidget(BannerPanel, 1);
  connect(BannerCloseButton, &QPushButton::clicked, this, [this]() { Finish(arOK); });

  for (QPushButton * Button : findChildren<QPushButton *>())
  {
    Button->setAutoDefault(false);
  }

  resize(fontMetrics().averageCharWidth() * 70, fontMetrics().lineSpacing() * 16);
}
//---------------------------------------------------------------------------
TAuthenticateForm::~TAuthenticateForm()
{
}
//---------------------------------------------------------------------------
void TAuthenticateForm::Init(TTerminal * Terminal)
{
  FTerminal = Terminal;
  FSessionData = Terminal->SessionData;
  AdjustControls();
}
//---------------------------------------------------------------------------
void TAuthenticateForm::AdjustControls()
{
  UnicodeString ACaption;
  if (FStatus.IsEmpty())
  {
    ACaption = FSessionData->SessionName;
  }
  else
  {
    ACaption = FStatus + TitleSeparator + FSessionData->SessionName;
  }
  setWindowTitle(ToQString(FormatMainFormCaption(ACaption)));
}
//---------------------------------------------------------------------------
void TAuthenticateForm::ShowAsModal()
{
  setWindowModality(Qt::ApplicationModal);
  show();
  raise();
  activateWindow();
}
//---------------------------------------------------------------------------
void TAuthenticateForm::Log(const UnicodeString & Message, const UnicodeString & Additional)
{
  QListWidgetItem * Item = new QListWidgetItem(ToQString(Message), LogView);
  if (!Additional.IsEmpty())
  {
    Item->setToolTip(ToQString(Additional));
  }
  LogView->scrollToItem(Item);
}
//---------------------------------------------------------------------------
void TAuthenticateForm::DoCancel()
{
  if (OnCancel)
  {
    OnCancel(this);
  }
}
//---------------------------------------------------------------------------
void TAuthenticateForm::Finish(TResult Result)
{
  FResult = Result;
  if (FLoop != nullptr)
  {
    FLoop->quit();
  }
}
//---------------------------------------------------------------------------
void TAuthenticateForm::closeEvent(QCloseEvent * Event)
{
  if (FLoop != nullptr)
  {
    // Closing a prompt cancels it, the connection follows
    Event->ignore();
    Finish(arCancel);
  }
  else
  {
    DoCancel();
    QDialog::closeEvent(Event);
  }
}
//---------------------------------------------------------------------------
void TAuthenticateForm::keyPressEvent(QKeyEvent * Event)
{
  if ((Event->key() == Qt::Key_Escape) && (Event->modifiers() == Qt::NoModifier))
  {
    if (FLoop != nullptr)
    {
      Finish(arCancel);
    }
    else
    {
      DoCancel();
    }
  }
  else
  {
    QDialog::keyPressEvent(Event);
  }
}
//---------------------------------------------------------------------------
bool TAuthenticateForm::Execute(const UnicodeString & Status, QWidget * Panel, QWidget * FocusControl,
  QPushButton * DefaultButton, bool Zoom, bool ForceLog)
{
  bool WasVisible = isVisible();
  DebugAssert(FStatus.IsEmpty());
  FStatus = Status;
  AdjustControls();
  DefaultButton->setDefault(true);
  // Without the log when the window is not showing the connection
  bool ShowLog = !Zoom && (WasVisible || ForceLog);
  TopPanel->setVisible(ShowLog);
  Panel->show();
  if (!WasVisible)
  {
    ShowAsModal();
  }
  if (ShowLog && (LogView->count() > 0))
  {
    LogView->scrollToBottom();
  }
  FocusControl->setFocus();

  // The terminal thread waits until the prompt is answered; the events are
  // processed meanwhile, as upstream's loop of Application->HandleMessage
  FResult = arNone;
  QApplication::setOverrideCursor(Qt::ArrowCursor);
  {
    QEventLoop Loop;
    FLoop = &Loop;
    Loop.exec();
    FLoop = nullptr;
  }
  QApplication::restoreOverrideCursor();

  Panel->hide();
  TopPanel->show();
  DefaultButton->setDefault(false);
  if (!WasVisible)
  {
    hide();
  }
  FStatus = L"";
  AdjustControls();

  bool Result = (FResult == arOK);
  if (!Result)
  {
    // As upstream: this may end in cancelling the terminal thread, so the
    // alternatives of a declined prompt are not tried while connecting
    DoCancel();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TAuthenticateForm::PromptUser(TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
  TStrings * Prompts, TStrings * Results, bool ForceLog, bool StoredCredentialsTried)
{
  // The prompts of this call
  while (QLayoutItem * Item = PromptLayout->takeAt(0))
  {
    delete Item->widget();
    delete Item;
  }

  if (!Instructions.IsEmpty())
  {
    QLabel * Label = new QLabel(ToQString(Instructions), PasswordPanel);
    Label->setObjectName(QStringLiteral("InstructionsLabel"));
    Label->setWordWrap(true);
    Label->setTextFormat(Qt::PlainText);
    Label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    PromptLayout->addWidget(Label);
  }

  std::vector<QLineEdit *> Edits;
  for (int Index = 0; Index < Prompts->Count; Index++)
  {
    QLabel * Label = new QLabel(ToQString(Prompts->Strings[Index]), PasswordPanel);
    Label->setObjectName(QStringLiteral("PromptLabel%1").arg(Index + 1));
    Label->setWordWrap(true);
    PromptLayout->addWidget(Label);
    QLineEdit * Edit = new QLineEdit(PasswordPanel);
    Edit->setObjectName(QStringLiteral("PromptEdit%1").arg(Index + 1));
    bool Echo = FLAGSET(reinterpret_cast<intptr_t>(Prompts->Objects[Index]), pupEcho);
    if (!Echo)
    {
      Edit->setEchoMode(QLineEdit::Password);
    }
    Edit->setText(ToQString(Results->Strings[Index]));
    Label->setBuddy(Edit);
    PromptLayout->addWidget(Edit);
    Edits.push_back(Edit);
  }

  bool ShowSessionRememberPassword = false;
  bool ShowSavePassword = false;
  TSessionData * Data = NULL;
  if (IsPasswordPrompt(Kind, Prompts) && StoredCredentialsTried)
  {
    Data = StoredSessions->FindSame(FSessionData);
    ShowSavePassword = (Data != NULL) && !Data->Password.IsEmpty();
  }
  // Not both remembering and saving the password
  if (!ShowSavePassword &&
      (Prompts->Count == 1) &&
      FLAGSET(reinterpret_cast<intptr_t>(Prompts->Objects[0]), pupRemember) &&
      DebugAlwaysTrue(IsPasswordOrPassphrasePrompt(Kind, Prompts)))
  {
    ShowSessionRememberPassword = true;
  }
  SavePasswordCheck->setChecked(false);
  SavePasswordCheck->setVisible(ShowSavePassword);
  SessionRememberPasswordCheck->setChecked(false);
  SessionRememberPasswordCheck->setVisible(ShowSessionRememberPassword);

  QWidget * FocusControl = !Edits.empty() ? static_cast<QWidget *>(Edits.front()) : PasswordOKButton;
  bool Result = Execute(Name, PasswordPanel, FocusControl, PasswordOKButton, false, ForceLog);
  if (Result)
  {
    for (int Index = 0; Index < static_cast<int>(Edits.size()); Index++)
    {
      Results->Strings[Index] = ToUnicodeString(Edits[Index]->text());
      intptr_t Flags = reinterpret_cast<intptr_t>(Prompts->Objects[Index]);
      Flags = (Flags & ~pupRemember) | FLAGMASK(((Index == 0) && SessionRememberPasswordCheck->isChecked()), pupRemember);
      Prompts->Objects[Index] = reinterpret_cast<TObject *>(Flags);
    }
    if (SavePasswordCheck->isChecked())
    {
      DebugAssert(Data != NULL);
      DebugAssert(Results->Count >= 1);
      FSessionData->Password = Results->Strings[0];
      Data->Password = Results->Strings[0];
      // modified only, explicit
      StoredSessions->Save(false, true);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TAuthenticateForm::Banner(const UnicodeString & Banner, bool & NeverShowAgain, int Options, unsigned int & Params)
{
  BannerMemo->setPlainText(ToQString(Banner));
  BannerMemo->setFont(
    FLAGSET(Params, bpMonospacedFont) ? QFontDatabase::systemFont(QFontDatabase::FixedFont) : font());
  NeverShowAgainCheck->setVisible(FLAGCLEAR(Options, boDisableNeverShowAgain));
  NeverShowAgainCheck->setChecked(NeverShowAgain);
  bool Result = Execute(LoadStr(AUTHENTICATION_BANNER), BannerPanel, BannerCloseButton, BannerCloseButton, true, false);
  if (Result)
  {
    NeverShowAgain = NeverShowAgainCheck->isChecked();
  }
}
//---------------------------------------------------------------------------
