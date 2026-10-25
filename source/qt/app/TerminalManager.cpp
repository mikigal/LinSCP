// The sessions of the GUI (upstream's windows/TerminalManager.cpp).
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <AppConfiguration.h>

#include "AuthenticateForm.h"
#include "ScpCommander.h"
#include "TerminalManager.h"

#include <QCoreApplication>
//---------------------------------------------------------------------------
TTerminalManager * TTerminalManager::FInstance = NULL;
//---------------------------------------------------------------------------
TManagedTerminal::TManagedTerminal(TSessionData * SessionData, TConfiguration * Configuration) :
  TTerminal(SessionData, Configuration)
{
  TerminalThread = NULL;
  Permanent = false;
}
//---------------------------------------------------------------------------
TManagedTerminal::~TManagedTerminal()
{
}
//---------------------------------------------------------------------------
TTerminalManager * TTerminalManager::Instance(bool ForceCreation)
{
  if ((FInstance == NULL) && ForceCreation)
  {
    FInstance = new TTerminalManager();
  }
  return FInstance;
}
//---------------------------------------------------------------------------
void TTerminalManager::DestroyInstance()
{
  delete FInstance;
  FInstance = NULL;
}
//---------------------------------------------------------------------------
TTerminalManager::TTerminalManager()
{
  FScpExplorer = NULL;
  FActiveTerminal = NULL;
  FAuthenticateForm = NULL;
  FAuthenticating = 0;
  FBusyToken = NULL;
  FKeepAuthenticateForm = false;
  FOpeningTerminal = NULL;
  FTerminalPendingAction = tpNull;

  DebugAssert(AppConfiguration->OnMasterPasswordPrompt == NULL);
  AppConfiguration->OnMasterPasswordPrompt = ::rtl::MethodPtr(this, &TTerminalManager::MasterPasswordPrompt);
  DebugAssert(Configuration->OnChange == NULL);
  Configuration->OnChange = ::rtl::MethodPtr(this, &TTerminalManager::ConfigurationChange);
}
//---------------------------------------------------------------------------
TTerminalManager::~TTerminalManager()
{
  if (FActiveTerminal != NULL)
  {
    TManagedTerminal * Terminal = FActiveTerminal;
    FActiveTerminal = NULL;
    delete Terminal;
  }
  CloseAuthenticateForm();
  AppConfiguration->OnMasterPasswordPrompt = NULL;
  Configuration->OnChange = NULL;
}
//---------------------------------------------------------------------------
void TTerminalManager::ConfigurationChange(TObject * /*Sender*/)
{
  // The configuration changes in the threads of the terminals too (the
  // host keys, ...)
  if (!IsMainThread())
  {
    QMetaObject::invokeMethod(QCoreApplication::instance(), [this]() { ConfigurationChange(NULL); }, Qt::QueuedConnection);
  }
  else if (FScpExplorer != NULL)
  {
    FScpExplorer->ConfigurationChanged();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::SetScpExplorer(TScpCommanderForm * Value)
{
  FScpExplorer = Value;
  UpdateScpExplorer();
}
//---------------------------------------------------------------------------
void TTerminalManager::UpdateScpExplorer()
{
  if (FScpExplorer != NULL)
  {
    FScpExplorer->TerminalChanged();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::SetupTerminal(TTerminal * Terminal)
{
  Terminal->OnQueryUser = ::rtl::MethodPtr(this, &TTerminalManager::TerminalQueryUser);
  Terminal->OnPromptUser = ::rtl::MethodPtr(this, &TTerminalManager::TerminalPromptUser);
  Terminal->OnDisplayBanner = ::rtl::MethodPtr(this, &TTerminalManager::TerminalDisplayBanner);
  Terminal->OnShowExtendedException = ::rtl::MethodPtr(this, &TTerminalManager::TerminalShowExtendedException);
  Terminal->OnProgress = ::rtl::MethodPtr(this, &TTerminalManager::OperationProgress);
  Terminal->OnFinished = ::rtl::MethodPtr(this, &TTerminalManager::OperationFinished);
  Terminal->OnInformation = ::rtl::MethodPtr(this, &TTerminalManager::TerminalInformation);
  Terminal->OnCustomCommand = ::rtl::MethodPtr(this, &TTerminalManager::TerminalCustomCommand);
}
//---------------------------------------------------------------------------
TManagedTerminal * TTerminalManager::NewSession(TSessionData * Data)
{
  if (FActiveTerminal != NULL)
  {
    FreeActiveTerminal();
  }
  FActiveTerminal = new TManagedTerminal(Data, Configuration);
  SetupTerminal(FActiveTerminal);
  UpdateScpExplorer();
  return FActiveTerminal;
}
//---------------------------------------------------------------------------
void TTerminalManager::FreeActiveTerminal()
{
  if (FTerminalPendingAction == tpNull)
  {
    std::unique_ptr<TManagedTerminal> Terminal(FActiveTerminal);
    FActiveTerminal = NULL;
    // The window lets the terminal go before it is freed
    UpdateScpExplorer();
    // The site manager opens on the site of the last session
    if (StoredSessions->FindSame(Terminal->SessionData) != NULL)
    {
      AppConfiguration->LastStoredSession = Terminal->SessionData->Name;
    }
    if (Terminal->Active)
    {
      Terminal->Close();
    }
  }
  else
  {
    DebugAssert(FTerminalPendingAction == tpNone);
    FTerminalPendingAction = tpFree;
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalThreadIdle(void * /*Data*/, TObject * /*Sender*/)
{
  QCoreApplication::processEvents();
}
//---------------------------------------------------------------------------
void TTerminalManager::DoConnectTerminal(TTerminal * Terminal, bool Reopen)
{
  // it must be managed terminal, unless it is secondary terminal (of managed terminal)
  TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Terminal);
  DebugAssert((ManagedTerminal != NULL) || (dynamic_cast<TSecondaryTerminal *>(Terminal) != NULL));

  TValueRestorer<TTerminal *> OpeningTerminalRestorer(FOpeningTerminal, Terminal);
  TTerminalThread * TerminalThread = new TTerminalThread(Terminal);
  TerminalThread->OnIdle = TNotifyEvent(NULL, TerminalThreadIdle);
  // The terminal cannot be abandoned (left to its thread when it does not
  // react to cancelling), as there is a single session yet
  TerminalThread->AllowAbandon = false;
  if (ManagedTerminal != NULL)
  {
    DebugAssert(ManagedTerminal->TerminalThread == NULL);
    ManagedTerminal->TerminalThread = TerminalThread;
  }
  ::rtl::TFinally Finally([&]()
  {
    TerminalThread->OnIdle = NULL;
    if (ManagedTerminal != NULL)
    {
      ManagedTerminal->TerminalThread = NULL;
    }
    DebugCheck(TerminalThread->Release());
  });
  if (Reopen)
  {
    TerminalThread->TerminalReopen();
  }
  else
  {
    TerminalThread->TerminalOpen();
  }
}
//---------------------------------------------------------------------------
bool TTerminalManager::ConnectActiveTerminalImpl(bool Reopen)
{
  TTerminalPendingAction Action;
  bool Result;
  do
  {
    Action = tpNull;
    Result = false;
    try
    {
      DebugAssert(ActiveTerminal != NULL);
      DoConnectTerminal(ActiveTerminal, Reopen);
      ActiveTerminal->Permanent = true;
      Result = true;
    }
    catch (Exception & E)
    {
      DebugAssert(FTerminalPendingAction == tpNull);
      TValueRestorer<TTerminalPendingAction> PendingActionRestorer(FTerminalPendingAction, tpNone);
      DebugAssert(ActiveTerminal != NULL);
      ActiveTerminal->ShowExtendedException(&E);
      Action = FTerminalPendingAction;
    }
  }
  while (Action == tpReconnect);

  if (Action == tpFree)
  {
    FreeActiveTerminal();
  }
  UpdateScpExplorer();

  return Result;
}
//---------------------------------------------------------------------------
bool TTerminalManager::ConnectTerminal(TTerminal * Terminal)
{
  bool Result = true;
  // were it an active terminal, it would allow abandoning, what this API cannot deal with
  DebugAssert(Terminal != ActiveTerminal);
  try
  {
    DoConnectTerminal(Terminal, false);
  }
  catch (Exception & E)
  {
    ShowExtendedExceptionEx(Terminal, &E);
    Result = false;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TTerminalManager::ConnectActiveTerminal()
{
  return ConnectActiveTerminalImpl(false);
}
//---------------------------------------------------------------------------
void TTerminalManager::ReconnectActiveTerminal()
{
  DebugAssert(ActiveTerminal != NULL);
  if (FTerminalPendingAction == tpNull)
  {
    ConnectActiveTerminalImpl(true);
  }
  else
  {
    FTerminalPendingAction = tpReconnect;
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::FatalExceptionAnswered(unsigned int Answer)
{
  // qaRetry is the "Reconnect" button of FatalExceptionMessageDialog
  if (Answer == qaRetry)
  {
    ReconnectActiveTerminal();
  }
  else
  {
    // Upstream keeps a session that was ever connected, disconnected, in
    // its tab; there are no tabs yet
    FreeActiveTerminal();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::HandleException(Exception * E)
{
  // Can be NULL, e.g. when the exception is thrown in the site manager
  if (ActiveTerminal != NULL)
  {
    ActiveTerminal->ShowExtendedException(E);
  }
  else
  {
    ShowExtendedException(E);
  }
}
//---------------------------------------------------------------------------
bool TTerminalManager::IsBusy()
{
  return (FOpeningTerminal != NULL) || (FAuthenticating > 0);
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalQueryUser(TObject * /*Sender*/,
  const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
  const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * /*Arg*/)
{
  TMessageParams MessageParams(Params);
  UnicodeString AQuery = Query;

  if ((Params != NULL) && FLAGSET(Params->Params, qpFatalAbort))
  {
    AQuery = FMTLOAD(WARN_FATAL_ERROR, (AQuery));
    if (!MessageParams.TimerMessage.IsEmpty())
    {
      MessageParams.TimerMessage = FMTLOAD(WARN_FATAL_ERROR, (MessageParams.TimerMessage));
    }
  }

  Answer = MoreMessageDialog(AQuery, MoreMessages, Type, Answers, &MessageParams);
}
//---------------------------------------------------------------------------
TAuthenticateForm * TTerminalManager::MakeAuthenticateForm(TTerminal * Terminal)
{
  TAuthenticateForm * Dialog = new TAuthenticateForm(FScpExplorer);
  Dialog->Init(Terminal);
  Dialog->OnCancel = [this](TAuthenticateForm * Form) { AuthenticateFormCancel(Form); };
  return Dialog;
}
//---------------------------------------------------------------------------
void TTerminalManager::AuthenticateFormCancel(TAuthenticateForm * Form)
{
  TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Form->GetTerminal());
  // Not managed for background transfers
  if (ManagedTerminal != NULL)
  {
    TTerminalThread * TerminalThread = ManagedTerminal->TerminalThread;
    if ((TerminalThread != NULL) && !TerminalThread->Cancelling)
    {
      Form->Log(LoadStr(AUTH_CANCELLING));
      TerminalThread->Cancel();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::CloseAuthenticateForm()
{
  delete FAuthenticateForm;
  FAuthenticateForm = NULL;
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalPromptUser(
  TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
  TStrings * Prompts, TStrings * Results, bool & Result, void * /*Arg*/)
{
  if (((Kind == pkPrompt) || (Kind == pkFileName)) && (FAuthenticateForm == NULL) &&
      (Terminal->Status != ssOpening))
  {
    DebugAssert(Instructions.IsEmpty());
    DebugAssert(Prompts->Count == 1);
    UnicodeString AResult = Results->Strings[0];
    Result = InputDialog(Name, Prompts->Strings[0], AResult);
    if (Result)
    {
      Results->Strings[0] = AResult;
    }
  }
  else
  {
    // Outside of the authentication, a window of the prompt only
    std::unique_ptr<TAuthenticateForm> OwnedAuthenticateForm;
    TAuthenticateForm * AuthenticateForm = FAuthenticateForm;
    if (AuthenticateForm == NULL)
    {
      OwnedAuthenticateForm.reset(MakeAuthenticateForm(Terminal));
      AuthenticateForm = OwnedAuthenticateForm.get();
    }
    Result = AuthenticateForm->PromptUser(Kind, Name, Instructions, Prompts, Results,
      (FAuthenticateForm != NULL), Terminal->StoredCredentialsTried);
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalDisplayBanner(
  TTerminal * Terminal, UnicodeString /*SessionName*/,
  const UnicodeString & Banner, bool & NeverShowAgain, int Options, unsigned int & Params)
{
  std::unique_ptr<TAuthenticateForm> OwnedAuthenticateForm;
  TAuthenticateForm * AuthenticateForm = FAuthenticateForm;
  if (AuthenticateForm == NULL)
  {
    OwnedAuthenticateForm.reset(MakeAuthenticateForm(Terminal));
    AuthenticateForm = OwnedAuthenticateForm.get();
  }
  AuthenticateForm->Banner(Banner, NeverShowAgain, Options, Params);
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalShowExtendedException(
  TTerminal * Terminal, Exception * E, void * /*Arg*/)
{
  ShowExtendedExceptionEx(Terminal, E);
}
//---------------------------------------------------------------------------
void TTerminalManager::AuthenticatingDone()
{
  FAuthenticating--;
  if (FAuthenticating == 0)
  {
    BusyEnd(FBusyToken);
    FBusyToken = NULL;
  }
  if (!FKeepAuthenticateForm)
  {
    CloseAuthenticateForm();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalInformation(
  TTerminal * Terminal, const UnicodeString & Str, int Phase, const UnicodeString & Additional)
{
  if (Phase == 1)
  {
    if (FAuthenticating == 0)
    {
      FBusyToken = BusyStart();
    }
    FAuthenticating++;
  }
  else if (Phase == 0)
  {
    DebugAssert(FAuthenticating > 0);
    AuthenticatingDone();
  }
  else
  {
    if (FAuthenticating > 0)
    {
      bool ShowPending = false;
      if (FAuthenticateForm == NULL)
      {
        FAuthenticateForm = MakeAuthenticateForm(Terminal);
        ShowPending = true;
      }
      FAuthenticateForm->Log(Str, Additional);
      if (ShowPending)
      {
        FAuthenticateForm->ShowAsModal();
      }
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::TerminalCustomCommand(
  TTerminal * /*Terminal*/, const UnicodeString & /*Command*/, bool & /*Handled*/)
{
  // Upstream copies the custom commands of the "copy to clipboard" kind;
  // custom commands are not ported yet
}
//---------------------------------------------------------------------------
void TTerminalManager::OperationProgress(TFileOperationProgressType & ProgressData)
{
  if (FScpExplorer != NULL)
  {
    FScpExplorer->OperationProgress(ProgressData);
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::OperationFinished(TFileOperation Operation, TOperationSide Side,
  bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
  TOnceDoneOperation & OnceDoneOperation)
{
  if (FScpExplorer != NULL)
  {
    FScpExplorer->OperationFinished(Operation, Side, Temp, FileName, Success, NotCancelled, OnceDoneOperation);
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::MasterPasswordPrompt()
{
  // In other threads, when a background transfer needs the password and
  // all the prompts were cancelled
  if (!IsMainThread() || !DoMasterPasswordDialog())
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
