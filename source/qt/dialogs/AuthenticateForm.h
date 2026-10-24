// The authentication window (upstream's TAuthenticateForm,
// forms/Authenticate.dfm): the log of the connection while it opens, the
// prompts for credentials and the authentication banner.
//---------------------------------------------------------------------------
#ifndef AuthenticateFormH
#define AuthenticateFormH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QDialog>

#include <functional>
//---------------------------------------------------------------------------
class QCheckBox;
class QEventLoop;
class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QVBoxLayout;
class TSessionData;
//---------------------------------------------------------------------------
class TAuthenticateForm : public QDialog
{
  Q_OBJECT

public:
  explicit TAuthenticateForm(QWidget * Parent = nullptr);
  virtual ~TAuthenticateForm();

  void Init(TTerminal * Terminal);
  // Shown while the connection opens, blocking the other windows, without
  // a modal event loop of its own
  void ShowAsModal();
  void Log(const UnicodeString & Message, const UnicodeString & Additional = UnicodeString());
  bool PromptUser(TPromptKind Kind, const UnicodeString & Name, const UnicodeString & Instructions,
    TStrings * Prompts, TStrings * Results, bool ForceLog, bool StoredCredentialsTried);
  void Banner(const UnicodeString & Banner, bool & NeverShowAgain, int Options, unsigned int & Params);

  TTerminal * GetTerminal() const { return FTerminal; }
  // The user cancels the connection
  std::function<void(TAuthenticateForm * Form)> OnCancel;

protected:
  virtual void closeEvent(QCloseEvent * Event) override;
  virtual void keyPressEvent(QKeyEvent * Event) override;

private:
  enum TResult { arNone, arOK, arCancel };

  TTerminal * FTerminal;
  TSessionData * FSessionData;
  UnicodeString FStatus;
  QEventLoop * FLoop;
  TResult FResult;

  QWidget * TopPanel;
  QListWidget * LogView;
  QWidget * PasswordPanel;
  QVBoxLayout * PromptLayout;
  QCheckBox * SavePasswordCheck;
  QCheckBox * SessionRememberPasswordCheck;
  QPushButton * PasswordOKButton;
  QPushButton * PasswordCancelButton;
  QWidget * BannerPanel;
  QPlainTextEdit * BannerMemo;
  QCheckBox * NeverShowAgainCheck;
  QPushButton * BannerCloseButton;

  void AdjustControls();
  void DoCancel();
  void Finish(TResult Result);
  bool Execute(const UnicodeString & Status, QWidget * Panel, QWidget * FocusControl,
    QPushButton * DefaultButton, bool Zoom, bool ForceLog);
};
//---------------------------------------------------------------------------
#endif
