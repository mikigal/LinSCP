// The sessions of the GUI and the handlers of the events of their
// terminals (upstream's TTerminalManager, windows/TerminalManager.cpp): the
// sessions of the tabs with their queues, the active one, connecting with
// the authentication window, the prompts and queries of the terminals, the
// errors, reconnecting (also of inactive sessions in the background).
//---------------------------------------------------------------------------
#ifndef TerminalManagerH
#define TerminalManagerH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <Queue.h>
#include <Terminal.h>

#include <mutex>
#include <utility>
#include <vector>
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

  // The state of the session (the current directories, the name of the
  // tab), for the tab and the workspaces
  TSessionData * StateData;
  // The states of the panels while the session is not shown
  TObject * LocalExplorerState;
  TObject * RemoteExplorerState;
  TDateTime ReopenStart;
  // The thread that opens the terminal, while it opens
  TTerminalThread * TerminalThread;
  // To distinguish sessions that were explicitly disconnected and
  // should not be reconnected when their tab is activated.
  bool Disconnected;
  bool DisconnectedTemporarily;
  UnicodeString DisconnectMessage;
  // Sessions that should not close when they fail to connect
  // (i.e. those that were ever connected or were opened as a part of a workspace)
  bool Permanent;
  // The error of the session lost while it was not active, shown once it is
  UnicodeString InactiveTerminationMessage;
  TDateTime NextInactiveReconnect;
  // The timestamp of an edited file whose upload rescheduled the reconnect
  TDateTime LastInactiveSave;
  UnicodeString OrigRemoteDirectory;
};
//---------------------------------------------------------------------------
class TTerminalManager : public TTerminalList
{
public:
  static TTerminalManager * Instance(bool ForceCreation = true);
  static void DestroyInstance();

  TTerminalManager();
  virtual ~TTerminalManager();

  TManagedTerminal * NewManagedTerminal(TSessionData * Data);
  // The sessions of the site manager (a site, a folder, a workspace),
  // not connected yet; the first one, or NULL
  TManagedTerminal * NewSessions(TList * DataList);
  virtual void __fastcall FreeTerminal(TTerminal * Terminal) override;
  void Move(TTerminal * Source, TTerminal * Target);
  void DisconnectActiveTerminalIfPermanentFreeOtherwise(const UnicodeString & Message);
  // The session closes, its tab stays
  void DisconnectActiveTerminal(const UnicodeString & Message = UnicodeString());
  void ReconnectActiveTerminal();
  void FreeActiveTerminal();
  void CycleTerminals(bool Forward);
  // Connects a terminal that is not the active one (the separate shell
  // session of the active one); false when it failed (the error was shown)
  bool ConnectTerminal(TTerminal * Terminal);
  void SetActiveTerminalWithAutoReconnect(TManagedTerminal * Value);
  // An exception of an operation of the GUI
  void HandleException(Exception * E);
  // A connection is opening (for the automation's "wait-idle")
  bool IsBusy();
  // The upkeep of the sessions and their queues, the events of the queues
  // (periodically, upstream calls it when the application is idle too)
  void Idle();
  // The name of the session for its tab (with its directory, when other
  // sessions have the same name)
  UnicodeString GetSessionTitle(TManagedTerminal * Session, bool Unique);
  UnicodeString FormatTerminalNoteMessage(TManagedTerminal * Terminal, const UnicodeString & Message);
  UnicodeString GetActiveSessionAppTitle();
  // The sessions as the stored sessions of a workspace (to save)
  void SaveWorkspace(TList * DataList);
  bool IsOpenedTerminal(TTerminal * Terminal);
  bool IsAvailableTerminal(TTerminal * Terminal);
  bool IsReconnectingTerminal(TTerminal * Terminal);
  bool IsAvailableTerminalForSite(TTerminal * Terminal, TSessionData * Data);
  TManagedTerminal * FindActiveTerminalForSite(TSessionData * Data);
  TTerminalQueue * FindQueueForTerminal(TTerminal * Terminal);
  // The session is opened in an SSH client (upstream's "Open in PuTTY")
  bool CanOpenInPutty();
  void OpenInPutty();

  TScpCommanderForm * GetScpExplorer() { return FScpExplorer; }
  void SetScpExplorer(TScpCommanderForm * Value);
  TManagedTerminal * GetActiveSession() { return FActiveSession; }
  void SetActiveSession(TManagedTerminal * Value);
  TManagedTerminal * GetActiveTerminal() { return FActiveSession; }
  TTerminalQueue * GetActiveQueue();
  int GetActiveSessionIndex();
  void SetActiveSessionIndex(int Value);
  TStrings * GetSessionList();
  TManagedTerminal * GetSession(int Index);
  bool IsUpdating() { return (FUpdating > 0); }
  bool IsActiveTerminal(TTerminal * Terminal) { return (Terminal != NULL) && (Terminal == FActiveSession); }

  __declspec(property(get = GetScpExplorer, put = SetScpExplorer)) TScpCommanderForm * ScpExplorer;
  __declspec(property(get = GetActiveSession, put = SetActiveSession)) TManagedTerminal * ActiveSession;
  // There are no local browser sessions, so it is the active session
  __declspec(property(get = GetActiveTerminal)) TManagedTerminal * ActiveTerminal;
  // The transfer queue of the active session
  __declspec(property(get = GetActiveQueue)) TTerminalQueue * ActiveQueue;
  __declspec(property(get = GetActiveSessionIndex, put = SetActiveSessionIndex)) int ActiveSessionIndex;
  // The titles of the sessions with the sessions as the objects
  __declspec(property(get = GetSessionList)) TStrings * SessionList;
  __declspec(property(get = GetSession)) TManagedTerminal * Sessions[];
  __declspec(property(get = IsUpdating)) bool Updating;

protected:
  virtual TTerminal * __fastcall CreateTerminal(TSessionData * Data) override;
  virtual TTerminal * __fastcall NewTerminal(TSessionData * Data) override;

private:
  static TTerminalManager * FInstance;
  TScpCommanderForm * FScpExplorer;
  TManagedTerminal * FActiveSession;
  bool FDestroying;
  TTerminalPendingAction FTerminalPendingAction;
  TStringList * FSessionList;
  // The queues of the sessions, by their indexes
  std::vector<TTerminalQueue *> FQueues;
  // The events of the queues come from their threads
  std::mutex FQueueSection;
  std::vector<std::pair<TTerminalQueue *, TQueueEvent>> FQueueEvents;
  TAuthenticateForm * FAuthenticateForm;
  int FAuthenticating;
  void * FBusyToken;
  bool FKeepAuthenticateForm;
  int FUpdating;
  int FMaxSessions;
  TTerminal * FOpeningTerminal;
  TManagedTerminal * FReconnectingInactiveTerminal;
  UnicodeString FReconnectingNote;

  TManagedTerminal * CreateManagedTerminal(TSessionData * Data);
  TManagedTerminal * DoNewSession(TSessionData * Data);
  void SetupTerminal(TTerminal * Terminal);
  TTerminalQueue * NewQueue(TTerminal * Terminal);
  void SetQueueConfiguration(TTerminalQueue * Queue);
  void QueueEvent(TTerminalQueue * Queue, TQueueEvent Event);
  TTerminalThread * CreateTerminalThread(TTerminal * Terminal);
  void ReconnectingTerminal(TManagedTerminal * ManagedTerminal);
  void ReconnectedTerminal(TManagedTerminal * ManagedTerminal);
  void DoConnectTerminal(TTerminal * Terminal, bool Reopen);
  bool ConnectActiveTerminalImpl(bool Reopen);
  bool ConnectActiveTerminal();
  void UpdateScpExplorer();
  void UpdateScpExplorer(TManagedTerminal * Session, TTerminalQueue * Queue);
  void DoSetActiveSession(TManagedTerminal * Value, bool AutoReconnect, bool LastTerminalClosed);
  void SessionReady();
  void SaveTerminal(TTerminal * Terminal);
  void UpdateAppTitle();
  void DoSessionListChanged();
  void FreeAll();
  void FreeTerminalCleanup(TTerminal * ATerminal, bool IsActiveSession);
  void StartInactiveTerminalReconnect(TManagedTerminal * Terminal);
  void CheckInactiveTerminalReconnect();
  TAuthenticateForm * MakeAuthenticateForm(TTerminal * Terminal);
  void CloseAuthenticateForm();
  void AuthenticatingDone();
  void AuthenticateFormCancel(TAuthenticateForm * Form);
  void MasterPasswordPrompt();
  void ConfigurationChange(TObject * Sender);

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
