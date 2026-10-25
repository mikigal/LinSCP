// The sessions of the GUI (upstream's windows/TerminalManager.cpp).
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <AppConfiguration.h>

#include "AuthenticateForm.h"
#include "ScpCommander.h"
#include "TerminalManager.h"
#include "OpenInPutty.h"

#include <QCoreApplication>
#include <QThread>
//---------------------------------------------------------------------------
TTerminalManager * TTerminalManager::FInstance = NULL;
//---------------------------------------------------------------------------
TManagedTerminal::TManagedTerminal(TSessionData * SessionData, TConfiguration * Configuration) :
  TTerminal(SessionData, Configuration)
{
  LocalExplorerState = NULL;
  RemoteExplorerState = NULL;
  ReopenStart = TDateTime();
  TerminalThread = NULL;
  Disconnected = false;
  DisconnectedTemporarily = false;
  Permanent = false;
  NextInactiveReconnect = TDateTime();
  LastInactiveSave = TDateTime();
  StateData = new TSessionData(L"");
  StateData->Assign(SessionData);
  StateData->LocalDirectory = StateData->LocalDirectoryExpanded;
}
//---------------------------------------------------------------------------
TManagedTerminal::~TManagedTerminal()
{
  delete StateData;
  delete LocalExplorerState;
  delete RemoteExplorerState;
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
TTerminalManager::TTerminalManager() :
  TTerminalList(Configuration)
{
  FScpExplorer = NULL;
  FActiveSession = NULL;
  FDestroying = false;
  FTerminalPendingAction = tpNull;
  FAuthenticateForm = NULL;
  FAuthenticating = 0;
  FBusyToken = NULL;
  FKeepAuthenticateForm = false;
  FUpdating = 0;
  FOpeningTerminal = NULL;
  FReconnectingInactiveTerminal = NULL;

  DebugAssert(AppConfiguration->OnMasterPasswordPrompt == NULL);
  AppConfiguration->OnMasterPasswordPrompt = ::rtl::MethodPtr(this, &TTerminalManager::MasterPasswordPrompt);
  DebugAssert(Configuration->OnChange == NULL);
  Configuration->OnChange = ::rtl::MethodPtr(this, &TTerminalManager::ConfigurationChange);

  FMaxSessions = AppConfiguration->MaxSessions;

  FSessionList = new TStringList();
}
//---------------------------------------------------------------------------
TTerminalManager::~TTerminalManager()
{
  FreeAll();

  DebugAssert(ScpExplorer == NULL);

  Configuration->OnChange = NULL;
  AppConfiguration->OnMasterPasswordPrompt = NULL;

  delete FSessionList;
  CloseAuthenticateForm();
}
//---------------------------------------------------------------------------
void TTerminalManager::SetQueueConfiguration(TTerminalQueue * Queue)
{
  Queue->TransfersLimit = Configuration->QueueTransfersLimit;
  Queue->KeepDoneItemsFor =
    (GUIConfiguration->QueueKeepDoneItems ? GUIConfiguration->QueueKeepDoneItemsFor : 0);
}
//---------------------------------------------------------------------------
TTerminalQueue * TTerminalManager::NewQueue(TTerminal * Terminal)
{
  TTerminalQueue * Queue = new TTerminalQueue(Terminal, Configuration);
  SetQueueConfiguration(Queue);
  Queue->Enabled = AppConfiguration->EnableQueueByDefault;
  Queue->OnQueryUser = ::rtl::MethodPtr(this, &TTerminalManager::TerminalQueryUser);
  Queue->OnPromptUser = ::rtl::MethodPtr(this, &TTerminalManager::TerminalPromptUser);
  Queue->OnShowExtendedException = ::rtl::MethodPtr(this, &TTerminalManager::TerminalShowExtendedException);
  Queue->OnEvent = ::rtl::MethodPtr(this, &TTerminalManager::QueueEvent);
  return Queue;
}
//---------------------------------------------------------------------------
TManagedTerminal * TTerminalManager::CreateManagedTerminal(TSessionData * Data)
{
  return new TManagedTerminal(Data, Configuration);
}
//---------------------------------------------------------------------------
TTerminal * TTerminalManager::CreateTerminal(TSessionData * Data)
{
  return CreateManagedTerminal(Data);
}
//---------------------------------------------------------------------------
TManagedTerminal * TTerminalManager::GetSession(int Index)
{
  return DebugNotNull(dynamic_cast<TManagedTerminal *>(TTerminalList::Terminals[Index]));
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
TManagedTerminal * TTerminalManager::DoNewSession(TSessionData * Data)
{
  if (Count >= FMaxSessions)
  {
    UnicodeString Msg = FMTLOAD(TOO_MANY_SESSIONS, (Count));
    if (MessageDialog(Msg, qtConfirmation, qaOK | qaCancel) == qaCancel)
    {
      Abort();
    }
    FMaxSessions = FMaxSessions * 3 / 2; // increase limit before the next warning by 50%
  }
  TManagedTerminal * Session = DebugNotNull(dynamic_cast<TManagedTerminal *>(TTerminalList::NewTerminal(Data)));
  try
  {
    FQueues.push_back(NewQueue(Session));

    SetupTerminal(Session);
  }
  catch (...)
  {
    if (Session != NULL)
    {
      FreeTerminal(Session);
    }
    throw;
  }

  return Session;
}
//---------------------------------------------------------------------------
TTerminal * TTerminalManager::NewTerminal(TSessionData * Data)
{
  TTerminal * Terminal = DoNewSession(Data);
  DoSessionListChanged();
  return Terminal;
}
//---------------------------------------------------------------------------
TManagedTerminal * TTerminalManager::NewManagedTerminal(TSessionData * Data)
{
  return DebugNotNull(dynamic_cast<TManagedTerminal *>(NewTerminal(Data)));
}
//---------------------------------------------------------------------------
TManagedTerminal * TTerminalManager::NewSessions(TList * DataList)
{
  TManagedTerminal * Result = NULL;
  for (int Index = 0; Index < DataList->Count; Index++)
  {
    TSessionData * Data = reinterpret_cast<TSessionData *>(DataList->Items[Index]);
    TManagedTerminal * Session = DoNewSession(Data);
    // When opening workspace/folder, keep the sessions open, even if they fail to connect.
    // We cannot detect a folder here, so we "guess" it by a session set size.
    // After all, no one will have a folder with a one session only (while a workspace with one session is likely).
    // And when when opening a folder with a one session only, it's not that big problem, if we treat it the same way
    // as when opening the session only.
    // Also closing a workspace session will remove the session from the workspace.
    // While closing a folder session won't remove the session from the folder.
    Session->Permanent = Data->IsWorkspace || (DataList->Count > 1);
    if (Result == NULL)
    {
      Result = Session;
    }
    else if (AppConfiguration->WorkspaceConnectAll)
    {
      Session->NextInactiveReconnect = IncSecond(Now(), 3);
      AppLogFmt(L"Scheduling connect of session %s", (Data->SessionName));
    }
  }
  DoSessionListChanged();
  return Result;
}
//---------------------------------------------------------------------------
void TTerminalManager::FreeActiveTerminal()
{
  if (FTerminalPendingAction == tpNull)
  {
    DebugAssert(ActiveSession != NULL);
    FreeTerminal(ActiveSession);
  }
  else
  {
    DebugAssert(FTerminalPendingAction == tpNone);
    FTerminalPendingAction = tpFree;
  }
}
//---------------------------------------------------------------------------
TTerminalThread * TTerminalManager::CreateTerminalThread(TTerminal * Terminal)
{
  TTerminalThread * TerminalThread = new TTerminalThread(Terminal);
  TerminalThread->OnIdle = TNotifyEvent(NULL, TerminalThreadIdle);
  return TerminalThread;
}
//---------------------------------------------------------------------------
void TTerminalManager::ReconnectingTerminal(TManagedTerminal * ManagedTerminal)
{
  // particularly when we are reconnecting RemoteDirectory of managed terminal
  // hold the last used remote directory as opposite to session data, which holds
  // the default remote directory.
  // make sure the last used directory is used, but the default is preserved too
  ManagedTerminal->OrigRemoteDirectory = ManagedTerminal->SessionData->RemoteDirectory;
  ManagedTerminal->SessionData->RemoteDirectory = ManagedTerminal->StateData->RemoteDirectory;

  if (ManagedTerminal->ReopenStart == TDateTime())
  {
    ManagedTerminal->ReopenStart = Now();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::ReconnectedTerminal(TManagedTerminal * ManagedTerminal)
{
  ManagedTerminal->SessionData->RemoteDirectory = ManagedTerminal->OrigRemoteDirectory;
  if (ManagedTerminal->Active)
  {
    ManagedTerminal->ReopenStart = TDateTime();
    ManagedTerminal->Permanent = true;
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
  TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Terminal);
  // it must be managed terminal, unless it is secondary terminal (of managed terminal)
  DebugAssert((ManagedTerminal != NULL) || (dynamic_cast<TSecondaryTerminal *>(Terminal) != NULL));

  {
    ::rtl::TFinally ReconnectedFinally([&]()
    {
      if (ManagedTerminal != NULL)
      {
        ReconnectedTerminal(ManagedTerminal);
      }
    });
    TValueRestorer<TTerminal *> OpeningTerminalRestorer(FOpeningTerminal, Terminal);
    TTerminalThread * TerminalThread;
    bool ReconnectingInactiveTerminal = (Terminal == FReconnectingInactiveTerminal);
    if (ReconnectingInactiveTerminal)
    {
      DebugAssert(Terminal == ActiveTerminal);
      TerminalThread = ManagedTerminal->TerminalThread;
      FReconnectingInactiveTerminal = NULL;
    }
    else
    {
      TerminalThread = CreateTerminalThread(Terminal);
    }
    // OpenSCP: the terminals are never abandoned (left to their threads
    // when they do not react to cancelling), as upstream does with the
    // active one: it would have to be replaced in the window and the
    // queue of its session refers to it
    TerminalThread->AllowAbandon = false;
    ::rtl::TFinally Finally([&]()
    {
      TerminalThread->OnIdle = NULL;
      if (ManagedTerminal != NULL)
      {
        ManagedTerminal->TerminalThread = NULL;
      }
      DebugCheck(TerminalThread->Release());
    });
    if (ReconnectingInactiveTerminal && DebugAlwaysTrue(Reopen))
    {
      TerminalThread->ContinueTerminalReopenInteractive();
    }
    else
    {
      if (ManagedTerminal != NULL)
      {
        ReconnectingTerminal(ManagedTerminal);
        ManagedTerminal->Disconnected = false;
        ManagedTerminal->DisconnectedTemporarily = false;
        ManagedTerminal->DisconnectMessage = EmptyStr;
        DebugAssert(ManagedTerminal->TerminalThread == NULL);
        ManagedTerminal->TerminalThread = TerminalThread;
      }

      if (Reopen)
      {
        TerminalThread->TerminalReopen();
      }
      else
      {
        TerminalThread->TerminalOpen();
      }
    }
  }

  if (DebugAlwaysTrue(Terminal->Active) && !Reopen && GUIConfiguration->QueueBootstrap)
  {
    FindQueueForTerminal(Terminal)->AddItem(new TBootstrapQueueItem());
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::CloseAuthenticateForm()
{
  delete FAuthenticateForm;
  FAuthenticateForm = NULL;
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

      if (ScpExplorer != NULL)
      {
        DebugAssert(ActiveTerminal->Status == ssOpened);
        SessionReady();
      }

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
    DisconnectActiveTerminalIfPermanentFreeOtherwise(EmptyStr);
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminalManager::DisconnectActiveTerminalIfPermanentFreeOtherwise(const UnicodeString & Message)
{
  if (ActiveTerminal->Permanent)
  {
    DisconnectActiveTerminal(Message);
  }
  else
  {
    FreeActiveTerminal();
  }
}
//---------------------------------------------------------------------------
bool TTerminalManager::ConnectActiveTerminal()
{
  bool Result = ConnectActiveTerminalImpl(false);

  if (Result && AppConfiguration->AutoOpenInPutty && CanOpenInPutty())
  {
    try
    {
      OpenInPutty();
    }
    catch (Exception & E)
    {
      ShowExtendedExceptionEx(NULL, &E);
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TTerminalManager::DisconnectActiveTerminal(const UnicodeString & Message)
{
  DebugAssert(ActiveTerminal != NULL);
  if (ActiveTerminal->Active)
  {
    ActiveTerminal->Close();
  }

  int Index = IndexOf(ActiveTerminal);

  TTerminalQueue * OldQueue = FQueues[Index];
  TTerminalQueue * NewQueue = this->NewQueue(ActiveTerminal);
  FQueues[Index] = NewQueue;
  if (ScpExplorer != NULL)
  {
    ScpExplorer->SetQueue(NewQueue);
  }
  delete OldQueue;

  ActiveTerminal->Disconnected = true;
  ActiveTerminal->DisconnectMessage = Message;
  if (ScpExplorer != NULL)
  {
    SessionReady(); // in case it was never connected
    ScpExplorer->TerminalDisconnected();
  }
  // disconnecting duplidate session removes need to distinguish the only connected session with short path
  DoSessionListChanged();
}
//---------------------------------------------------------------------------
void TTerminalManager::ReconnectActiveTerminal()
{
  DebugAssert(ActiveTerminal != NULL);

  if ((ScpExplorer != NULL) &&
      (ActiveTerminal != FReconnectingInactiveTerminal) &&
      (ScpExplorer->GetManagedSession() == ActiveTerminal))
  {
    ScpExplorer->UpdateSession(ActiveTerminal);
  }

  try
  {
    if (FTerminalPendingAction == tpNull)
    {
      ConnectActiveTerminalImpl(true);
    }
    else
    {
      FTerminalPendingAction = tpReconnect;
    }
  }
  catch (...)
  {
    DisconnectActiveTerminal();
    throw;
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::FreeAll()
{
  TAutoFlag DestroyingFlag(FDestroying);
  while (Count > 0)
  {
    FreeTerminal(Sessions[0]);
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::FreeTerminalCleanup(TTerminal * ATerminal, bool IsActiveSession)
{
  int Index = IndexOf(ATerminal);
  Extract(ATerminal);

  TTerminalQueue * Queue = FQueues[Index];
  FQueues.erase(FQueues.begin() + Index);

  if (IsActiveSession)
  {
    TManagedTerminal * NewActiveTerminal;
    bool LastTerminalClosed = false;

    if (FDestroying)
    {
      NewActiveTerminal = NULL;
    }
    else
    {
      if (Count > 0)
      {
        NewActiveTerminal = Sessions[Index < Count ? Index : Index - 1];
        if (!NewActiveTerminal->Active && !NewActiveTerminal->Disconnected)
        {
          NewActiveTerminal->Disconnected = true;
          NewActiveTerminal->DisconnectedTemporarily = true;
          NewActiveTerminal->DisconnectMessage = EmptyStr;
        }
      }
      else
      {
        // OpenSCP: there are no local browser sessions to replace the last
        // session with (upstream's GetReplacementForLastSession)
        NewActiveTerminal = NULL;
        LastTerminalClosed = true;
      }
    }
    DoSetActiveSession(NewActiveTerminal, false, LastTerminalClosed);
  }

  // only now all references to/from queue (particularly events to explorer) are cleared
  delete Queue;
}
//---------------------------------------------------------------------------
void TTerminalManager::FreeTerminal(TTerminal * Terminal)
{
  TTerminal * ATerminal = Terminal;
  bool IsActiveSession = ((ActiveSession != NULL) && (Terminal == ActiveSession));
  ::rtl::TFinally Finally([&]()
  {
    FreeTerminalCleanup(ATerminal, IsActiveSession);

    delete Terminal; // noop if abandoned

    DoSessionListChanged();
  });

  if (!IsActiveSession)
  {
    // For active session, done by DoSetActiveSession below.
    // For inactive session, needs to be done here before potentially abandoning the session.
    SaveTerminal(Terminal);
  }

  TManagedTerminal * ManagedSession = DebugNotNull(dynamic_cast<TManagedTerminal *>(Terminal));

  // we want the Login dialog to open on auto-workspace name,
  // as set in TScpCommanderForm::closeEvent
  if (!FDestroying || !AppConfiguration->AutoSaveWorkspace)
  {
    if (StoredSessions->FindSame(Terminal->SessionData) != NULL)
    {
      AppConfiguration->LastStoredSession = Terminal->SessionData->Name;
    }
  }

  if (ScpExplorer != NULL)
  {
    ScpExplorer->TerminalRemoved(Terminal);
  }

  if (ManagedSession == FReconnectingInactiveTerminal)
  {
    DebugAssert(!IsActiveSession);
    AppLog(L"Cancelling background reconnect before closing session");
    TTerminalThread * TerminalThread = DebugNotNull(ManagedSession->TerminalThread);
    DebugAssert(TerminalThread->AllowAbandon);
    TerminalThread->Cancel();
    // OpenSCP: also until the reconnect completes (upstream waits for the
    // thread to be abandoned only)
    while (!TerminalThread->Abandoned() && (TerminalThread->IsTerminalReopenComplete() == trrPending))
    {
      Sleep(50);
    }

    if (!TerminalThread->Release())
    {
      AppLog(L"Abandoned background reconnect");
      TerminalThread->Terminate();
      Terminal = NULL;
    }
    else
    {
      AppLog(L"Background reconnect stopped");
      ManagedSession->TerminalThread = NULL;
    }
    FReconnectingInactiveTerminal = NULL;
  }
  else if (Terminal->Active)
  {
    Terminal->Close();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::UpdateScpExplorer(TManagedTerminal * Session, TTerminalQueue * Queue)
{
  FScpExplorer->SetManagedSession(Session);
  FScpExplorer->SetQueue(Queue);
}
//---------------------------------------------------------------------------
void TTerminalManager::UpdateScpExplorer()
{
  UpdateScpExplorer(ActiveSession, ActiveQueue);
}
//---------------------------------------------------------------------------
void TTerminalManager::SetScpExplorer(TScpCommanderForm * Value)
{
  if (ScpExplorer != Value)
  {
    // changing explorer is not supported yet
    DebugAssert((ScpExplorer == NULL) || (Value == NULL));
    FScpExplorer = Value;
    if (FScpExplorer != NULL)
    {
      UpdateScpExplorer();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::SetActiveSession(TManagedTerminal * Value)
{
  DoSetActiveSession(Value, false, false);
}
//---------------------------------------------------------------------------
void TTerminalManager::SetActiveTerminalWithAutoReconnect(TManagedTerminal * Value)
{
  DoSetActiveSession(Value, true, false);
}
//---------------------------------------------------------------------------
void TTerminalManager::DoSetActiveSession(TManagedTerminal * Value, bool AutoReconnect, bool LastTerminalClosed)
{
  if (ActiveSession != Value)
  {
    bool ReconnectingInactiveTerminal =
      (Value != NULL) &&
      (FReconnectingInactiveTerminal == Value) &&
      (ScpExplorer != NULL) &&
      DebugAlwaysTrue(FReconnectingInactiveTerminal->TerminalThread != NULL);

    TManagedTerminal * PActiveSession = ActiveSession;
    FActiveSession = Value;
    if (ScpExplorer != NULL)
    {
      if ((ActiveSession != NULL) &&
          !ReconnectingInactiveTerminal &&
          ((ActiveSession->Status == ssOpened) || ActiveSession->Disconnected))
      {
        SessionReady();
      }
      else
      {
        UpdateScpExplorer(NULL, NULL);
      }
    }
    UpdateAppTitle();

    if (PActiveSession != NULL)
    {
      if (PActiveSession->DisconnectedTemporarily && DebugAlwaysTrue(PActiveSession->Disconnected))
      {
        PActiveSession->Disconnected = false;
        PActiveSession->DisconnectedTemporarily = false;
      }

      if (!PActiveSession->Active)
      {
        SaveTerminal(PActiveSession);
      }
    }

    if (ActiveSession != NULL)
    {
      if (ReconnectingInactiveTerminal)
      {
        DebugAssert(FTerminalPendingAction == tpNull); // the only assumed and tested scenario
        ReconnectActiveTerminal();
      }
      else if (!ActiveSession->Active &&
               !ActiveSession->InactiveTerminationMessage.IsEmpty())
      {
        UnicodeString Message = ActiveSession->InactiveTerminationMessage;
        ActiveSession->InactiveTerminationMessage = EmptyStr;
        if (AutoReconnect || GUIConfiguration->SessionReopenAutoInactive)
        {
          ReconnectActiveTerminal();
        }
        else
        {
          // finally show pending terminal message,
          // this gives user also possibility to reconnect
          std::unique_ptr<ESshFatal> E(new ESshFatal(NULL, Message));
          E->InactiveTerminationMessage = true;
          ActiveTerminal->ShowExtendedException(E.get());
        }
      }

      // LastTerminalClosed is true only for a replacement local session,
      // and it should never happen that it fails to be activated
      if (LastTerminalClosed && DebugAlwaysFalse(Value != ActiveSession))
      {
        LastTerminalClosed = false; // just in case
      }
    }
    else
    {
      LastTerminalClosed = true;
    }

    if (LastTerminalClosed && !Updating && (ScpExplorer != NULL))
    {
      ScpExplorer->LastTerminalClosed();
    }

    if ((ActiveSession != NULL) &&
        !ActiveSession->Active && !ActiveSession->Disconnected)
    {
      DebugAssert(ActiveSession->InactiveTerminationMessage.IsEmpty());
      ConnectActiveTerminal();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::UpdateAppTitle()
{
  if (ScpExplorer != NULL)
  {
    ScpExplorer->UpdateCaption();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::SaveTerminal(TTerminal * Terminal)
{
  TSessionData * Data = StoredSessions->FindSame(Terminal->SessionData);
  if (Data != NULL)
  {
    TManagedTerminal * ManagedTerminal = dynamic_cast<TManagedTerminal *>(Terminal);
    DebugAssert(ManagedTerminal != NULL);

    bool Changed = false;
    if (Terminal->SessionData->UpdateDirectories)
    {
      Data->CopyDirectoriesStateData(ManagedTerminal->StateData);
      Changed = true;
    }

    if (Changed)
    {
      // modified only, implicit
      StoredSessions->Save(false, false);
    }
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
void TTerminalManager::ConfigurationChange(TObject * /*Sender*/)
{
  // The configuration changes in the threads of the terminals too (the
  // host keys, ...)
  if (!IsMainThread())
  {
    QMetaObject::invokeMethod(QCoreApplication::instance(), [this]() { ConfigurationChange(NULL); }, Qt::QueuedConnection);
  }
  else
  {
    for (TTerminalQueue * Queue : FQueues)
    {
      SetQueueConfiguration(Queue);
    }
    if (FScpExplorer != NULL)
    {
      FScpExplorer->ConfigurationChanged();
    }
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::SessionReady()
{
  UpdateScpExplorer();
  ScpExplorer->SessionReady();
}
//---------------------------------------------------------------------------
TStrings * TTerminalManager::GetSessionList()
{
  FSessionList->Clear();
  for (int Index = 0; Index < Count; Index++)
  {
    TManagedTerminal * Terminal = Sessions[Index];
    UnicodeString Name = GetSessionTitle(Terminal, true);
    FSessionList->AddObject(Name, Terminal);
  }
  return FSessionList;
}
//---------------------------------------------------------------------------
int TTerminalManager::GetActiveSessionIndex()
{
  return (ActiveSession != NULL) ? IndexOf(ActiveSession) : -1;
}
//---------------------------------------------------------------------------
void TTerminalManager::SetActiveSessionIndex(int Value)
{
  ActiveSession = Sessions[Value];
}
//---------------------------------------------------------------------------
UnicodeString TTerminalManager::GetSessionTitle(TManagedTerminal * Session, bool Unique)
{
  UnicodeString Result = Session->SessionData->SessionName;
  if (Unique &&
      (AppConfiguration->SessionTabNameFormat != stnfNone))
  {
    int Index = IndexOf(Session);
    // not for background transfer sessions and disconnected sessions
    if ((Index >= 0) && Session->Active)
    {
      for (int Index2 = 0; Index2 < Count; Index2++)
      {
        UnicodeString Name = Sessions[Index2]->SessionData->SessionName;
        if ((Sessions[Index2] != Session) &&
            Sessions[Index2]->Active &&
            SameText(Name, Result))
        {
          UnicodeString Path = ExtractShortName(Session->CurrentDirectory, true);
          if (!Path.IsEmpty())
          {
            const int MaxPathLen = 16;
            if ((AppConfiguration->SessionTabNameFormat == stnfShortPathTrunc) &&
                (Path.Length() > MaxPathLen))
            {
              Path = Path.SubString(1, MaxPathLen - 2) + Ellipsis;
            }
            Result = FORMAT(L"%s (%s)", (Result, Path));
          }
          break;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TTerminalManager::FormatTerminalNoteMessage(TManagedTerminal * Terminal, const UnicodeString & Message)
{
  return FORMAT(L"%s: %s", (GetSessionTitle(Terminal, true), Message));
}
//---------------------------------------------------------------------------
UnicodeString TTerminalManager::GetActiveSessionAppTitle()
{
  UnicodeString Result;
  if (ActiveSession != NULL)
  {
    Result = GetSessionTitle(ActiveSession, false);
  }
  return Result;
}
//---------------------------------------------------------------------------
TTerminalQueue * TTerminalManager::GetActiveQueue()
{
  TTerminalQueue * Result = NULL;
  if (ActiveSession != NULL)
  {
    Result = FQueues[ActiveSessionIndex];
  }
  return Result;
}
//---------------------------------------------------------------------------
void TTerminalManager::CycleTerminals(bool Forward)
{
  if (Count > 0)
  {
    int Index = ActiveSessionIndex;
    Index += Forward ? 1 : -1;
    if (Index < 0)
    {
      Index = Count - 1;
    }
    else if (Index >= Count)
    {
      Index = 0;
    }
    ActiveSessionIndex = Index;
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::StartInactiveTerminalReconnect(TManagedTerminal * Terminal)
{
  ReconnectingTerminal(Terminal);
  Terminal->NextInactiveReconnect = TDateTime(); // just in case
  FReconnectingInactiveTerminal = Terminal;

  DebugAssert(Terminal->TerminalThread == NULL);
  TTerminalThread * TerminalThread = CreateTerminalThread(Terminal);
  Terminal->TerminalThread = TerminalThread;
  TerminalThread->AllowAbandon = true;
  TerminalThread->StartTerminalReopenNonInteractive();
  FReconnectingNote = FormatTerminalNoteMessage(Terminal, LoadStr(SESSION_RECONNECTING));
  ScpExplorer->PostNote(FReconnectingNote);
}
//---------------------------------------------------------------------------
void TTerminalManager::CheckInactiveTerminalReconnect()
{
  TManagedTerminal * Terminal = DebugNotNull(FReconnectingInactiveTerminal);
  TTerminalThread * TerminalThread = DebugNotNull(Terminal->TerminalThread);
  TTerminalReopenResult ReopenResult = TerminalThread->IsTerminalReopenComplete();
  UnicodeString SessionName = Terminal->SessionData->SessionName;
  if (ReopenResult == trrPending)
  {
    AppLog(FORMAT(L"Inactive session %s is still being reconnected", (SessionName)));
  }
  else
  {
    {
      ::rtl::TFinally Finally([&]()
      {
        ReconnectedTerminal(Terminal);
        Terminal->TerminalThread = NULL;
        FReconnectingInactiveTerminal = NULL;
      });
      DebugCheck(TerminalThread->Release());
    }

    if (ReopenResult == trrSucceeded)
    {
      AppLog(FORMAT(L"Inactive session %s was reconnected", (SessionName)));
      ScpExplorer->CancelNote(FReconnectingNote);
      UnicodeString ReconnectMessage = LoadStr(SESSION_RECONNECTED);
      // This is an indication that we got tray popup on disconnect, so counter that with the same on reconnect
      if (!Terminal->InactiveTerminationMessage.IsEmpty())
      {
        ScpExplorer->InactiveTerminalNotify(Terminal, ReconnectMessage, qtInformation);
      }
      // otherwise (= when connecting a workspace on the background), notify more subtly with just status bar note
      else
      {
        ScpExplorer->PostNote(FormatTerminalNoteMessage(Terminal, ReconnectMessage));
      }
      // update session tab
      ScpExplorer->TerminalStatusChanged(Terminal);
    }
    else
    {
      if (ReopenResult == trrFailed)
      {
        // in case it was disabled meanwhile
        if (GUIConfiguration->SessionReopenAutoInactive)
        {
          int Interval = 2 * GUIConfiguration->SessionReopenAutoIdle;
          AppLog(FORMAT(L"Inactive session %s failed to reconnect, will try again in %d ms", (SessionName, Interval)));
          // Might want to consider SessionReopenTimeout
          Terminal->NextInactiveReconnect = IncMilliSecond(Now(), Interval);
        }
        else
        {
          Terminal->NextInactiveReconnect = TDateTime();
        }
      }
      else if (ReopenResult == trrNeedsInteraction)
      {
        AppLog(FORMAT(L"Inactive session %s needs interaction", (SessionName)));
        ScpExplorer->CancelNote(FReconnectingNote);
        ScpExplorer->InactiveTerminalNotify(Terminal, LoadStr(SESSION_NEED_INTERACTION), qtInformation);
        Terminal->NextInactiveReconnect = TDateTime();
      }
      try
      {
        if (Terminal->Active)
        {
          Terminal->Close();
        }
      }
      catch (...)
      {
      }
    }

    FReconnectingNote = EmptyStr;
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::Idle()
{
  for (int Index = 0; Index < Count; Index++)
  {
    TManagedTerminal * Terminal = Sessions[Index];
    // OpenSCP: not the session that opens (its thread processes the events
    // of the GUI, which call this)
    if (Terminal == FOpeningTerminal)
    {
      continue;
    }
    try
    {
      if (Terminal == FReconnectingInactiveTerminal)
      {
        CheckInactiveTerminalReconnect();
      }
      else
      {
        // make sure Idle is called on the thread that runs the terminal
        if (Terminal->TerminalThread != NULL)
        {
          Terminal->TerminalThread->Idle();
        }
        else
        {
          if (Terminal->Active)
          {
            Terminal->Idle();
          }
          else if ((Terminal != ActiveTerminal) &&
                   ((!Terminal->InactiveTerminationMessage.IsEmpty() && GUIConfiguration->SessionReopenAutoInactive) ||
                    (Terminal->InactiveTerminationMessage.IsEmpty() && AppConfiguration->WorkspaceConnectAll)) &&
                   (FReconnectingInactiveTerminal == NULL) &&
                   (Terminal->NextInactiveReconnect != TDateTime()) &&
                   (Terminal->NextInactiveReconnect < Now()))
          {
            StartInactiveTerminalReconnect(Terminal);
          }
        }

        // The session may have been freed (a lost connection of the active
        // one, not reconnected)
        if ((Index < Count) && (Sessions[Index] == Terminal) && Terminal->Active)
        {
          DebugAssert(Index < static_cast<int>(FQueues.size()));
          if (Index < static_cast<int>(FQueues.size()))
          {
            FQueues[Index]->Idle();
          }
        }
      }
    }
    catch (Exception & E)
    {
      if (Terminal == ActiveTerminal)
      {
        // throw further, so that the exception is handled in proper place
        // (a lost connection is offered to reconnect)
        HandleException(&E);
      }
      else
      {
        if (DebugAlwaysTrue(ScpExplorer != NULL))
        {
          ScpExplorer->InactiveTerminalNotify(Terminal, EmptyStr, qtError, &E);
        }

        if (!Terminal->Active)
        {
          // if session is lost, save the error message and rethrow it
          // once the terminal gets activated
          Terminal->InactiveTerminationMessage = E.Message;
          Terminal->NextInactiveReconnect = IncMilliSecond(Now(), GUIConfiguration->SessionReopenAutoIdle);
          if (ScpExplorer != NULL)
          {
            ScpExplorer->TerminalStatusChanged(Terminal);
          }
        }
      }
    }
  }

  TTerminalQueue * QueueWithEvent;
  do
  {
    QueueWithEvent = NULL;
    TQueueEvent QueueEvent = TQueueEvent(); // shut up

    {
      std::lock_guard<std::mutex> Guard(FQueueSection);
      if (!FQueueEvents.empty())
      {
        QueueWithEvent = FQueueEvents[0].first;
        QueueEvent = FQueueEvents[0].second;
        FQueueEvents.erase(FQueueEvents.begin());
      }
    }

    if (QueueWithEvent != NULL)
    {
      std::vector<TTerminalQueue *>::iterator I = std::find(FQueues.begin(), FQueues.end(), QueueWithEvent);
      // the session may not exist anymore
      if (I != FQueues.end())
      {
        TManagedTerminal * Terminal = Sessions[static_cast<int>(I - FQueues.begin())];
        // we can hardly have a queue event without explorer
        DebugAssert(ScpExplorer != NULL);
        if (ScpExplorer != NULL)
        {
          ScpExplorer->QueueEvent(Terminal, QueueWithEvent, QueueEvent);
        }
      }
    }
  }
  while (QueueWithEvent != NULL);
}
//---------------------------------------------------------------------------
void TTerminalManager::Move(TTerminal * Source, TTerminal * Target)
{
  int SourceIndex = IndexOf(Source);
  int TargetIndex = IndexOf(Target);
  TTerminalList::Move(SourceIndex, TargetIndex);
  TTerminalQueue * Queue = FQueues[SourceIndex];
  FQueues.erase(FQueues.begin() + SourceIndex);
  FQueues.insert(FQueues.begin() + TargetIndex, Queue);
  DoSessionListChanged();
  // when there are indexed sessions with the same name,
  // the index may change when reordering the sessions
  UpdateAppTitle();
}
//---------------------------------------------------------------------------
void TTerminalManager::DoSessionListChanged()
{
  if ((FScpExplorer != NULL) && !Updating)
  {
    FScpExplorer->SessionListChanged();
  }
}
//---------------------------------------------------------------------------
void TTerminalManager::SaveWorkspace(TList * DataList)
{
  for (int Index = 0; Index < Count; Index++)
  {
    TManagedTerminal * ManagedTerminal = Sessions[Index];
    TSessionData * Data = StoredSessions->SaveWorkspaceData(ManagedTerminal->StateData, Index);
    if (ManagedTerminal->Active)
    {
      ManagedTerminal->UpdateSessionCredentials(Data);
    }
    DataList->Add(Data);
  }
}
//---------------------------------------------------------------------------
bool TTerminalManager::IsOpenedTerminal(TTerminal * Terminal)
{
  return
    (Terminal != NULL) &&
    Terminal->Active;
}
//---------------------------------------------------------------------------
bool TTerminalManager::IsAvailableTerminal(TTerminal * Terminal)
{
  return
    IsOpenedTerminal(Terminal) &&
    !IsReconnectingTerminal(Terminal);
}
//---------------------------------------------------------------------------
bool TTerminalManager::IsReconnectingTerminal(TTerminal * Terminal)
{
  return
    (Terminal != NULL) &&
    ((Terminal == FOpeningTerminal) || (Terminal == FReconnectingInactiveTerminal));
}
//---------------------------------------------------------------------------
bool TTerminalManager::IsAvailableTerminalForSite(TTerminal * Terminal, TSessionData * Data)
{
  bool Result = IsAvailableTerminal(Terminal);
  if (Result)
  {
    std::unique_ptr<TSessionData> TerminalData(Terminal->SessionData->Clone());
    Terminal->UpdateSessionCredentials(TerminalData.get());
    Result = TerminalData->IsSameSite(Data);
  }
  return Result;
}
//---------------------------------------------------------------------------
TManagedTerminal * TTerminalManager::FindActiveTerminalForSite(TSessionData * Data)
{
  TManagedTerminal * Result = NULL;
  for (int Index = 0; (Result == NULL) && (Index < Count); Index++)
  {
    TManagedTerminal * Terminal = Sessions[Index];
    if (IsAvailableTerminalForSite(Terminal, Data))
    {
      Result = Terminal;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TTerminalQueue * TTerminalManager::FindQueueForTerminal(TTerminal * Terminal)
{
  int Index = IndexOf(Terminal);
  return FQueues[Index];
}
//---------------------------------------------------------------------------
void TTerminalManager::QueueEvent(TTerminalQueue * Queue, TQueueEvent Event)
{
  std::lock_guard<std::mutex> Guard(FQueueSection);
  FQueueEvents.push_back(std::make_pair(Queue, Event));
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

  if (FScpExplorer != NULL)
  {
    Answer = FScpExplorer->MoreMessageDialog(AQuery, MoreMessages, Type, Answers, &MessageParams);
  }
  else
  {
    Answer = MoreMessageDialog(AQuery, MoreMessages, Type, Answers, &MessageParams);
  }
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
bool TTerminalManager::CanOpenInPutty()
{
  return (ActiveTerminal != NULL) && ActiveTerminal->Active && !GUIConfiguration->PuttyPath.Trim().IsEmpty();
}
//---------------------------------------------------------------------------
void TTerminalManager::OpenInPutty()
{
  Configuration->Usage->Inc(L"OpenInPutty");

  std::unique_ptr<TSessionData> Data(new TSessionData(L""));
  Data->Assign(ActiveTerminal->SessionData);
  ActiveTerminal->UpdateSessionCredentials(Data.get());
  if (ActiveTerminal->TunnelLocalPortNumber != 0)
  {
    Data->ConfigureTunnel(ActiveTerminal->TunnelLocalPortNumber);
  }
  OpenSessionInPutty(Data.get());
}
//---------------------------------------------------------------------------
