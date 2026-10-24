// The sessions of the GUI and the handlers of the events of their
// terminals (upstream's TTerminalManager, windows/TerminalManager.cpp):
// connecting with the authentication window, the prompts and queries of the
// terminals, the errors, reconnecting.
//---------------------------------------------------------------------------
#ifndef TerminalManagerH
#define TerminalManagerH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <Queue.h>
#include <Terminal.h>
//---------------------------------------------------------------------------
class TAuthenticateForm;
class TScpCommanderForm;
//---------------------------------------------------------------------------
enum TTerminalPendingAction { tpNull, tpNone, tpReconnect, tpFree };
//---------------------------------------------------------------------------
class TManagedTerminal : public TTerminal
{
public:
  TManagedTerminal(TSessionData * SessionData, TConfiguration * Configuration);
  virtual ~TManagedTerminal();

  // The thread that opens the terminal, while it opens
  TTerminalThread * TerminalThread;
  // Ever opened (a lost connection is offered to reconnect)
  bool Permanent;
};
//---------------------------------------------------------------------------
class TTerminalManager
{
public:
  static TTerminalManager * Instance(bool ForceCreation = true);
  static void DestroyInstance();

  TTerminalManager();
  ~TTerminalManager();

  // Replaces the active session with a new one (not connected yet)
  TManagedTerminal * NewSession(TSessionData * Data);
  // Connects the active session; false when it failed (the error was
  // shown), the session is freed then
  bool ConnectActiveTerminal();
  void ReconnectActiveTerminal();
  void FreeActiveTerminal();
  // An exception of an operation of the GUI
  void HandleException(Exception * E);
  // A connection is opening (for the automation's "wait-idle")
  bool IsBusy();

  TManagedTerminal * GetActiveTerminal() { return FActiveTerminal; }
  TScpCommanderForm * GetScpExplorer() { return FScpExplorer; }
  void SetScpExplorer(TScpCommanderForm * Value);
  // The fatal exception of the active terminal was shown, with its answer
  void FatalExceptionAnswered(unsigned int Answer);
  bool IsActiveTerminal(TTerminal * Terminal) { return (Terminal != NULL) && (Terminal == FActiveTerminal); }

  __declspec(property(get = GetActiveTerminal)) TManagedTerminal * ActiveTerminal;
  __declspec(property(get = GetScpExplorer, put = SetScpExplorer)) TScpCommanderForm * ScpExplorer;

private:
  static TTerminalManager * FInstance;
  TScpCommanderForm * FScpExplorer;
  TManagedTerminal * FActiveTerminal;
  TAuthenticateForm * FAuthenticateForm;
  int FAuthenticating;
  void * FBusyToken;
  bool FKeepAuthenticateForm;
  TTerminal * FOpeningTerminal;
  TTerminalPendingAction FTerminalPendingAction;

  void SetupTerminal(TTerminal * Terminal);
  void DoConnectTerminal(TTerminal * Terminal, bool Reopen);
  bool ConnectActiveTerminalImpl(bool Reopen);
  void UpdateScpExplorer();
  TAuthenticateForm * MakeAuthenticateForm(TTerminal * Terminal);
  void CloseAuthenticateForm();
  void AuthenticatingDone();
  void AuthenticateFormCancel(TAuthenticateForm * Form);
  void MasterPasswordPrompt();

  void TerminalQueryUser(TObject * Sender,
    const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void TerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results, bool & Result, void * Arg);
  void TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString & Banner, bool & NeverShowAgain,
    int Options, unsigned int & Params);
  void TerminalShowExtendedException(TTerminal * Terminal, Exception * E, void * Arg);
  void TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, int Phase, const UnicodeString & Additional);
  void TerminalCustomCommand(TTerminal * Terminal, const UnicodeString & Command, bool & Handled);
  void OperationProgress(TFileOperationProgressType & ProgressData);
  void OperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
    TOnceDoneOperation & OnceDoneOperation);
  static void TerminalThreadIdle(void * Data, TObject * Sender);
};
//---------------------------------------------------------------------------
#endif
