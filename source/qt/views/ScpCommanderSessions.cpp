// The sessions of the Commander and their tabs (upstream's SessionsPageControl
// and the session handling of forms/CustomScpExplorer.cpp and
// forms/ScpCommander.cpp): the session shown with the state of its panels,
// opening, closing, duplicating, renaming, disconnecting and reconnecting
// sessions, the notes of the status bar.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <DesktopShortcut.h>
#include <TerminalManager.h>

#include "DirPanel.h"
#include "ScpCommander.h"

#include <QActionGroup>
#include <QApplication>
#include <QLabel>
#include <QMenu>
#include <QMouseEvent>
#include <QStatusBar>
#include <QStyle>
#include <QTabBar>
#include <QTimer>
#include <QTreeView>
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateSessionsPageControl()
{
  SessionsPageControl = new QTabBar(this);
  SessionsPageControl->setObjectName(QStringLiteral("SessionsPageControl"));
  SessionsPageControl->setDocumentMode(true);
  SessionsPageControl->setExpanding(false);
  SessionsPageControl->setElideMode(Qt::ElideRight);
  SessionsPageControl->setTabsClosable(true);
  SessionsPageControl->setMovable(true);
  SessionsPageControl->setUsesScrollButtons(true);
  SessionsPageControl->setFocusPolicy(Qt::NoFocus);
  SessionsPageControl->setContextMenuPolicy(Qt::CustomContextMenu);
  SessionsPageControl->installEventFilter(this);

  // Not within the handlers of the tab bar's events, as the sessions may
  // show modal windows (the site manager, the authentication)
  connect(SessionsPageControl, &QTabBar::tabBarClicked, this, [this](int Index)
  {
    if ((Index >= 0) && (GetSessionTabSession(Index) == NULL))
    {
      QTimer::singleShot(0, this, [this]() { ExecuteHandler([this]() { NewSession(); }); });
    }
  });
  connect(SessionsPageControl, &QTabBar::currentChanged, this, [this](int)
  {
    if (!FUpdatingSessionTabs)
    {
      QTimer::singleShot(0, this, [this]() { ExecuteHandler([this]() { SessionTabSwitched(); }); });
    }
  });
  connect(SessionsPageControl, &QTabBar::tabCloseRequested, this, [this](int Index)
  {
    TManagedTerminal * Session = GetSessionTabSession(Index);
    QTimer::singleShot(0, this, [this, Session]()
    {
      ExecuteHandler([this, Session]()
      {
        TTerminalManager * Manager = TTerminalManager::Instance();
        int AIndex = Manager->IndexOf(Session);
        if (AIndex >= 0)
        {
          CloseSessionTab(AIndex);
        }
      });
    });
  });
  // Reordered sessions follow their tabs once the drag ends
  connect(SessionsPageControl, &QTabBar::tabMoved, this, [this](int, int)
  {
    if (!FUpdatingSessionTabs)
    {
      SessionsPageControl->setProperty("Moved", true);
    }
  });
  connect(SessionsPageControl, &QTabBar::customContextMenuRequested, this, [this](const QPoint & Pos)
  {
    int Index = SessionsPageControl->tabAt(Pos);
    TManagedTerminal * Session = (Index >= 0) ? GetSessionTabSession(Index) : NULL;
    if ((Session != NULL) && !IsBusy())
    {
      ExecuteHandler([this, Session, Pos]()
      {
        // As upstream: the session of the tab is activated first
        TTerminalManager * Manager = TTerminalManager::Instance();
        Manager->ActiveSession = Session;
        if (Manager->ActiveSession == Session)
        {
          QMenu Menu(this);
          Menu.setObjectName(QStringLiteral("SessionsPopup"));
          Menu.addAction(CloseTabAction);
          Menu.addAction(DuplicateTabAction);
          Menu.addAction(RenameTabAction);
          Menu.addSeparator();
          Menu.addAction(DisconnectSessionAction);
          Menu.addAction(ReconnectSessionAction);
          Menu.addAction(SaveCurrentSessionAction2);
          Menu.addSeparator();
          Menu.addAction(NewSessionAction);
          Menu.exec(SessionsPageControl->mapToGlobal(Pos));
        }
      });
    }
  });
}
//---------------------------------------------------------------------------
TManagedTerminal * TScpCommanderForm::GetSessionTabSession(int Index)
{
  return reinterpret_cast<TManagedTerminal *>(SessionsPageControl->tabData(Index).value<quintptr>());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionListChanged()
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  int Count = (Manager != NULL) ? Manager->Count : 0;
  TAutoFlag UpdatingSessionTabsFlag(FUpdatingSessionTabs);

  if (Manager != NULL)
  {
    Configuration->Usage->SetMax(L"MaxOpenedSessions", Count);
  }

  // The "new tab" tab is added again as the last one, without the close
  // button of the session tabs
  int Tabs = SessionsPageControl->count();
  if ((Tabs > 0) && (GetSessionTabSession(Tabs - 1) == NULL))
  {
    SessionsPageControl->removeTab(Tabs - 1);
  }
  while (SessionsPageControl->count() > Count)
  {
    SessionsPageControl->removeTab(SessionsPageControl->count() - 1);
  }
  while (SessionsPageControl->count() < Count)
  {
    SessionsPageControl->addTab(QString());
  }
  for (int Index = 0; Index < Count; Index++)
  {
    SessionsPageControl->setTabData(Index, QVariant::fromValue(reinterpret_cast<quintptr>(Manager->Sessions[Index])));
    UpdateSessionTab(Index);
  }

  int NewTabIndex =
    SessionsPageControl->addTab(QIcon::fromTheme(QStringLiteral("tab-new"), QIcon::fromTheme(QStringLiteral("list-add"))), QString());
  SessionsPageControl->setTabData(NewTabIndex, QVariant::fromValue(quintptr(0)));
  // OpenSCP: without its second line (Shift opens a new window)
  UnicodeString NewTabHint = LoadStr(NEW_REMOTE_TAB_HINT);
  SessionsPageControl->setTabToolTip(NewTabIndex, ToQString(CutToChar(NewTabHint, L'\n', true)));
  for (QTabBar::ButtonPosition Position : { QTabBar::LeftSide, QTabBar::RightSide })
  {
    QWidget * Button = SessionsPageControl->tabButton(NewTabIndex, Position);
    if (Button != nullptr)
    {
      SessionsPageControl->setTabButton(NewTabIndex, Position, nullptr);
      delete Button;
    }
  }

  int ActiveSessionIndex = (Manager != NULL) ? Manager->ActiveSessionIndex : -1;
  SessionsPageControl->setCurrentIndex((ActiveSessionIndex >= 0) ? ActiveSessionIndex : NewTabIndex);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateSessionTab(int Index)
{
  TManagedTerminal * Session = GetSessionTabSession(Index);
  if (DebugAlwaysTrue(Session != NULL))
  {
    TTerminalManager * Manager = TTerminalManager::Instance();
    UnicodeString Title = Manager->GetSessionTitle(Session, true);
    SessionsPageControl->setTabText(Index, ToQString(Title).replace(QLatin1Char('&'), QStringLiteral("&&")));
    // Upstream shadows the tabs of the sessions that are not connected
    SessionsPageControl->setTabTextColor(Index,
      Session->Active ? QColor() : SessionsPageControl->palette().color(QPalette::Disabled, QPalette::WindowText));
    // The color of the site (upstream colors the tab)
    TColor Color = static_cast<TColor>(Session->StateData->Color);
    int Size = SessionsPageControl->style()->pixelMetric(QStyle::PM_SmallIconSize);
    SessionsPageControl->setTabIcon(Index, (Color != 0) ? ColorIcon(Color, QSize(Size, Size)) : QIcon());

    // upstream's GetTabHintDetails
    UnicodeString Hint = Manager->GetSessionTitle(Session, false);
    UnicodeString Details = Session->SessionData->DefaultSessionName;
    if (Details != Hint)
    {
      Hint += L"\n" + Details;
    }
    Hint += L"\n" + (!Session->Active ? LoadStr(STATUS_NOT_CONNECTED2) : Session->PeekCurrentDirectory());
    SessionsPageControl->setTabToolTip(Index, ToQString(Hint));
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalStatusChanged(TManagedTerminal * Terminal)
{
  int Index = TTerminalManager::Instance()->IndexOf(Terminal);
  if (DebugAlwaysTrue((Index >= 0) && (Index < SessionsPageControl->count())))
  {
    UpdateSessionTab(Index);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionTabSwitched()
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  int Index = SessionsPageControl->currentIndex();
  TManagedTerminal * Session = (Index >= 0) ? GetSessionTabSession(Index) : NULL;
  if ((Session != NULL) && (Session != Manager->ActiveSession) && !IsBusy() &&
      (QApplication::activeModalWidget() == nullptr))
  {
    Manager->ActiveSession = Session;
  }
  // The tab of the active session again (the "new tab" tab opens a new
  // session when clicked, the session cannot be changed now)
  if (GetSessionTabSession(SessionsPageControl->currentIndex()) != Manager->ActiveSession)
  {
    SessionListChanged();
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::eventFilter(QObject * Object, QEvent * Event)
{
  if (Object == SessionsPageControl)
  {
    if (Event->type() == QEvent::MouseButtonRelease)
    {
      QMouseEvent * MouseEvent = static_cast<QMouseEvent *>(Event);
      if (MouseEvent->button() == Qt::MiddleButton)
      {
        int Index = SessionsPageControl->tabAt(MouseEvent->position().toPoint());
        TManagedTerminal * Session = (Index >= 0) ? GetSessionTabSession(Index) : NULL;
        if (Session != NULL)
        {
          QTimer::singleShot(0, this, [this, Session]()
          {
            ExecuteHandler([this, Session]()
            {
              int AIndex = TTerminalManager::Instance()->IndexOf(Session);
              if (AIndex >= 0)
              {
                CloseSessionTab(AIndex);
              }
            });
          });
        }
      }
      else if (MouseEvent->button() == Qt::LeftButton)
      {
        // Once the tab bar ends the drag of a tab
        QTimer::singleShot(0, this, [this]()
        {
          if (SessionsPageControl->property("Moved").toBool())
          {
            SessionsPageControl->setProperty("Moved", false);
            // The sessions in the order of their tabs
            std::vector<TManagedTerminal *> Order;
            for (int Index = 0; Index < SessionsPageControl->count(); Index++)
            {
              TManagedTerminal * Session = GetSessionTabSession(Index);
              if (Session != NULL)
              {
                Order.push_back(Session);
              }
            }
            TTerminalManager * Manager = TTerminalManager::Instance();
            if (static_cast<int>(Order.size()) == Manager->Count)
            {
              for (int Target = 0; Target < static_cast<int>(Order.size()); Target++)
              {
                if (Manager->Sessions[Target] != Order[Target])
                {
                  Configuration->Usage->Inc(L"SessionTabMoves");
                  Manager->Move(Order[Target], Manager->Sessions[Target]);
                }
              }
            }
            // Also the "new tab" tab back to the end
            SessionListChanged();
          }
        });
      }
    }
  }
  return EventFilterPanels(Object, Event) || QMainWindow::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SetManagedSession(TManagedTerminal * Value)
{
  if (FManagedSession != Value)
  {
    SessionChanging();
    FManagedSession = Value;
    SessionChanged();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionChanging()
{
  if (FManagedSession != NULL)
  {
    UpdateSession(FManagedSession);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateSession(TManagedTerminal * Session)
{
  DebugAssert(Session == FManagedSession);
  delete Session->RemoteExplorerState;
  Session->RemoteExplorerState = NULL;
  delete Session->LocalExplorerState;
  Session->LocalExplorerState = NULL;

  if (AppConfiguration->PreservePanelState)
  {
    Session->RemoteExplorerState = FRemotePanel->SaveState();
    Session->LocalExplorerState = FLocalPanel->SaveState();
  }
  UpdateSessionData(Session->StateData);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateSessionData(TSessionData * Data)
{
  // Keep in sync with TSessionData::CopyStateData

  DebugAssert(Data != NULL);

  // This is inconsistent with how color (for example) is handled.
  Data->Name = FManagedSession->SessionData->Name;

  // cannot use RemoteDirView->Path, because it is empty if connection
  // was already closed
  // also only peek, we may not be connected at all atm
  // so make sure we do not try retrieving current directory from the server
  // (particularly with FTP)
  UnicodeString ACurrentDirectory = FManagedSession->PeekCurrentDirectory();
  if (!ACurrentDirectory.IsEmpty())
  {
    Data->RemoteDirectory = ACurrentDirectory;
  }

  Data->LocalDirectory = FLocalPanel->GetPath();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RestoreSessionLocalDirectory(const UnicodeString & LocalDirectory)
{
  if (!LocalDirectory.IsEmpty() &&
      (FFirstTerminal || !AppConfiguration->PreserveLocalDirectory))
  {
    try
    {
      FLocalPanel->ChangeDirectory(LocalDirectory);
    }
    catch (Exception & E)
    {
      FManagedSession->ShowExtendedException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionChanged()
{
  TManagedTerminal * Session = FManagedSession;
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  if ((Manager != NULL) && !Manager->Updating)
  {
    SessionListChanged();
  }

  FRemotePanel->SetTerminal(Session);
  if (Session != NULL)
  {
    if (Session->Active)
    {
      Session->RefreshDirectory();
    }
    FRemotePanel->RestoreState(AppConfiguration->PreservePanelState ? Session->RemoteExplorerState : NULL);

    bool RestoreState = AppConfiguration->PreservePanelState && !AppConfiguration->PreserveLocalDirectory;
    // reset home directory
    FLocalPanel->SetHomeDirectory(UnicodeString());
    RestoreSessionLocalDirectory(Session->StateData->LocalDirectory);
    FFirstTerminal = false;
    if (AppConfiguration->DefaultDirIsHome &&
        !Session->SessionData->UpdateDirectories)
    {
      FLocalPanel->SetHomeDirectory(Session->SessionData->LocalDirectoryExpanded);
    }
    if (RestoreState)
    {
      FLocalPanel->RestoreState(Session->LocalExplorerState);
    }
  }

  UpdateSessionStatus();
  UpdateTransferList();
  if (!FRemotePanel->IsReady() && (FCurrentPanel == FRemotePanel))
  {
    FLocalPanel->GetDirView()->setFocus();
    SetCurrentPanel(FLocalPanel);
  }
  if (Session != NULL)
  {
    TransferPresetAutoSelect();
  }
  UpdateCaption();
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionReady()
{
  // The session shown connected or got disconnected
  FRemotePanel->SetTerminal(FManagedSession);
  UpdateSessionStatus();
  // cannot rely on active page being page for active terminal,
  // as it can happen that active page is the "new session" page
  // (e.g. when reconnecting active terminal, while login dialog
  // invoked from "new session" page is modal)
  int ActiveSessionIndex = TTerminalManager::Instance()->ActiveSessionIndex;
  if ((ActiveSessionIndex >= 0) && (ActiveSessionIndex < SessionsPageControl->count()) &&
      (GetSessionTabSession(ActiveSessionIndex) != NULL))
  {
    UpdateSessionTab(ActiveSessionIndex);
  }
  UpdateTransferList();
  UpdateCaption();
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateSessionStatus()
{
  TTerminal * Terminal = GetTerminal();
  UnicodeString Status;
  UnicodeString Hint;
  if ((Terminal != NULL) && Terminal->Active)
  {
    // As upstream's status bar: the protocol of the file system (SFTP-3),
    // else of the session; the encryption in the hint
    const TSessionInfo & SessionInfo = Terminal->GetSessionInfo();
    const TFileSystemInfo & FileSystemInfo = Terminal->GetFileSystemInfo();
    Status = FileSystemInfo.ProtocolName.IsEmpty() ? SessionInfo.ProtocolName : FileSystemInfo.ProtocolName;
    Hint = SessionInfo.SecurityProtocolName;
  }
  FSessionStatusLabel->setText(ToQString(Status));
  FSessionStatusLabel->setToolTip(ToQString(Hint));
  FRemotePanel->SetDisconnectMessage(
    (FManagedSession != NULL) ? UnformatMessage(FManagedSession->DisconnectMessage) : UnicodeString());
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DetachTerminal(TTerminal * ATerminal)
{
  if (FFileFindTerminal == ATerminal)
  {
    FFileFindTerminal = NULL;
    HideFileFindDialog();
  }

  if (FClipboardTerminal == ATerminal)
  {
    ClipboardClear(); // implies ClipboardStop
  }

  if ((FOnSynchronizeAbort != nullptr) &&
      (ATerminal == FManagedSession))
  {
    FOnSynchronizeAbort();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalRemoved(TTerminal * Terminal)
{
  EditedFilesTerminalRemoved(Terminal);
  DetachTerminal(Terminal);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::TerminalDisconnected()
{
  DetachTerminal(GetTerminal());
  FRemotePanel->SetTerminal(FManagedSession);
  UpdateSessionStatus();
  UpdateActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::LastTerminalClosed()
{
  UpdateActions();
  // Not within the code that closed the session
  QTimer::singleShot(0, this, [this]() { ExecuteHandler([this]() { NeedSession(false); }); });
}
//---------------------------------------------------------------------------
void TScpCommanderForm::NeedSession(bool Startup)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  // Not while a session opens (the site manager shows again for an ad hoc
  // session that failed) or anything else is shown
  if ((Manager->Count == 0) && (FNewSessionNesting == 0) && !IsBusy() && isVisible() &&
      (QApplication::activeModalWidget() == nullptr))
  {
    // Cache, as the login dialog can change its value.
    bool ShowLogin = AppConfiguration->ShowLoginWhenNoSession;
    if (ShowLogin)
    {
      NewSession();
    }

    // Do not terminate, if we are only starting up and we are not showing Login dialog
    // (so there was no chance for the user to open any session yet)
    if ((ShowLogin || !Startup) &&
        !AppConfiguration->KeepOpenWhenNoSession &&
        (Manager->Count == 0))
    {
      close();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::NewSession()
{
  if (!IsBusy())
  {
    TAutoNestingCounter NewSessionCounter(FNewSessionNesting);
    TTerminalManager * Manager = TTerminalManager::Instance();
    std::unique_ptr<TObjectList> DataList(new TObjectList());
    bool Retry;
    // upstream's TTerminalManager::NewSession: an ad hoc session that
    // fails to connect is edited in the site manager again
    do
    {
      Retry = false;
      if (DoLoginDialog(DataList.get(), this, false) && (DataList->Count > 0))
      {
        TManagedTerminal * ANewSession = Manager->NewSessions(DataList.get());
        bool AdHoc =
          (DataList->Count == 1) &&
          (StoredSessions->FindSame(static_cast<TSessionData *>(DataList->Items[0])) == NULL);
        Manager->ActiveSession = ANewSession;
        Retry = AdHoc && (Manager->ActiveSession != ANewSession);
      }
    }
    while (Retry);

    if (FRemotePanel->IsReady())
    {
      FRemotePanel->GetDirView()->setFocus();
    }
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanCloseQueue(TTerminalQueue * Queue)
{
  DebugAssert(Queue != NULL);
  bool Result = Queue->IsEmpty;
  if (!Result)
  {
    activateWindow();
    Result = (MessageDialog(LoadStr(PENDING_QUEUE_ITEMS2), qtWarning, qaOK | qaCancel) == qaOK);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CloseSessionTab(int Index)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  TManagedTerminal * Session = Manager->Sessions[Index];
  if (IsBusy())
  {
    // noop
  }
  else if (Session == Manager->ActiveSession)
  {
    CloseTabActionExecute();
  }
  else if (!Session->Active)
  {
    Manager->FreeTerminal(Session);
  }
  else
  {
    TTerminalQueue * Queue = Manager->FindQueueForTerminal(Session);
    if (CanCloseQueue(Queue))
    {
      Manager->FreeTerminal(Session);
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CloseTabActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL) && CanCloseQueue())
  {
    TTerminalManager::Instance()->FreeActiveTerminal();
  }
}
//---------------------------------------------------------------------------
TSessionData * TScpCommanderForm::CloneCurrentSessionData()
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  std::unique_ptr<TSessionData> SessionData(new TSessionData(L""));
  SessionData->Assign(FManagedSession->SessionData);
  UpdateSessionData(SessionData.get());
  if (Manager->IsAvailableTerminal(FManagedSession))
  {
    FManagedSession->UpdateSessionCredentials(SessionData.get());
  }
  if (!FManagedSession->SessionData->HasSessionName())
  {
    // Particularly for "Workspace/XXXX" name, we need to reset it, as it would become user-visible
    // once IsWorkspace is cleared
    SessionData->Name = UnicodeString();
  }
  // Most uses of this method ends with saving the session, and we do not want to store the IsWorkspace flag.
  SessionData->IsWorkspace = false;
  return SessionData.release();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DuplicateTabActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL))
  {
    // current working directories become defaults here, what is not right
    std::unique_ptr<TSessionData> SessionData(CloneCurrentSessionData());

    TTerminalManager * Manager = TTerminalManager::Instance();
    TManagedTerminal * ASession = Manager->NewManagedTerminal(SessionData.get());
    // We definitely want these
    ASession->Disconnected = FManagedSession->Disconnected;
    // Not sure about these two
    ASession->Permanent = FManagedSession->Permanent;
    ASession->DisconnectedTemporarily = FManagedSession->DisconnectedTemporarily;
    Manager->ActiveSession = ASession;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RenameTabActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL))
  {
    UnicodeString Name = FManagedSession->SessionData->SessionName;
    if (InputDialog(LoadStr(RENAME_TAB_TITLE), LoadStr(RENAME_TAB_PROMPT), Name) &&
        // When submitting the default name, do not fix it
        (Name != FManagedSession->SessionData->SessionName))
    {
      // Checks for a slash only, so it's not that big deal that we do the check after submitting the dialog only.
      TSessionData::ValidateName(Name);

      // This is inconsistent with how color (for example) is handled.
      FManagedSession->SessionData->Name = Name;

      UpdateCaption();
      // Add/Remove distinguishing paths from sessions of the same name.
      SessionListChanged();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::NextTabActionExecute()
{
  if (!IsBusy())
  {
    TTerminalManager::Instance()->CycleTerminals(true);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PreviousTabActionExecute()
{
  if (!IsBusy())
  {
    TTerminalManager::Instance()->CycleTerminals(false);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DisconnectSessionActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL) && FManagedSession->Active && CanCloseQueue())
  {
    TTerminalManager::Instance()->DisconnectActiveTerminal();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ReconnectSessionActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL) && DebugAlwaysTrue(!FManagedSession->Active))
  {
    TTerminalManager::Instance()->ReconnectActiveTerminal();
    if (FRemotePanel->IsReady())
    {
      FRemotePanel->GetDirView()->setFocus();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SaveCurrentSessionActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL))
  {
    std::unique_ptr<TSessionData> SessionData(CloneCurrentSessionData());
    TSessionData * EditingSessionData = StoredSessions->FindSame(SessionData.get());
    DoSaveSession(SessionData.get(), EditingSessionData, true, NULL);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateOpenedTabsMenu(QMenu * Menu)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  TStrings * SessionList = Manager->SessionList;
  QActionGroup * Group = new QActionGroup(Menu);
  for (int Index = 0; Index < SessionList->Count; Index++)
  {
    TManagedTerminal * Session = static_cast<TManagedTerminal *>(SessionList->Objects[Index]);
    QAction * Action = Menu->addAction(ToQString(EscapeHotkey(SessionList->Strings[Index])));
    Action->setObjectName(QStringLiteral("OpenedTab%1").arg(Index));
    Action->setCheckable(true);
    Action->setChecked(Session == Manager->ActiveSession);
    Action->setActionGroup(Group);
    Action->setEnabled(!IsBusy());
    connect(Action, &QAction::triggered, this, [this, Session]()
    {
      ExecuteHandler([this, Session]()
      {
        TTerminalManager * AManager = TTerminalManager::Instance();
        if (!IsBusy() && (AManager->IndexOf(Session) >= 0))
        {
          AManager->ActiveSession = Session;
        }
      });
    });
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::PostNote(const UnicodeString & ANote, unsigned int Seconds)
{
  UnicodeString Note = ANote;
  int P = Note.Pos(L"\n");
  if (P > 0)
  {
    Note.SetLength(P - 1);
  }
  FNote = Note;
  if (Seconds == 0)
  {
    Seconds = AppConfiguration->NotificationsTimeout;
  }
  statusBar()->showMessage(ToQString(Note), Seconds * MSecsPerSec);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CancelNote(const UnicodeString & Note)
{
  if (FNote == Note)
  {
    FNote = UnicodeString();
    statusBar()->clearMessage();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::InactiveTerminalNotify(
  TManagedTerminal * Terminal, const UnicodeString & AMessage, TQueryType Type, Exception * E)
{
  UnicodeString Message = AMessage;
  if ((E == NULL) || ExceptionMessage(E, Message))
  {
    Notify(Terminal, Message, Type, false, [this, Terminal](const QString & ActivationToken)
    {
      // upstream's balloon click: the session is activated
      ActivateFromOtherApplication(ActivationToken);
      TTerminalManager * Manager = TTerminalManager::Instance();
      if (!IsBusy() && (Manager->IndexOf(Terminal) >= 0))
      {
        ExecuteHandler([Manager, Terminal]() { Manager->ActiveSession = Terminal; });
      }
    });
    if (!AppConfiguration->BalloonNotifications)
    {
      PostNote(TTerminalManager::Instance()->FormatTerminalNoteMessage(Terminal, UnformatMessage(Message)));
    }
  }
  TerminalStatusChanged(Terminal);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OpenedTabActionExecute(int Index)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  if (!IsBusy() && (Index < Manager->Count))
  {
    Manager->ActiveSessionIndex = Index;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionsTabsActionExecute()
{
  if (IsExplorer())
  {
    AppConfiguration->ExplorerSessionsTabs = SessionsTabsAction2->isChecked();
  }
  else
  {
    AppConfiguration->SessionsTabs = SessionsTabsAction2->isChecked();
  }
  SessionsPageControl->setVisible(SessionsTabsAction2->isChecked());
}
//---------------------------------------------------------------------------
UnicodeString TScpCommanderForm::WorkspaceName()
{
  return
    DefaultStr(AppConfiguration->LastWorkspace,
      DefaultStr(AppConfiguration->AutoWorkspace, LoadStr(NEW_WORKSPACE)));
}
//---------------------------------------------------------------------------
TObjectList * TScpCommanderForm::DoCollectWorkspace()
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  std::unique_ptr<TObjectList> DataList(new TObjectList());

  if (DebugAlwaysTrue(FManagedSession != NULL))
  {
    // Update ManagedSession->StateData
    UpdateSession(FManagedSession);
  }
  Manager->SaveWorkspace(DataList.get());

  return DataList.release();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoSaveWorkspace(
  const UnicodeString & Name, TObjectList * DataList, bool SavePasswords, bool Explicit)
{
  AppConfiguration->LastWorkspace = Name;

  if (!SavePasswords)
  {
    for (int Index = 0; Index < DataList->Count; Index++)
    {
      TSessionData * SessionData = dynamic_cast<TSessionData *>(DataList->Items[Index]);

      if (SessionData->Link.IsEmpty())
      {
        SessionData->ClearSessionPasswords();
      }
    }
  }

  StoredSessions->NewWorkspace(Name, DataList);
  // modified only
  StoredSessions->Save(false, Explicit);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::SaveWorkspace(bool EnableAutoSave)
{
  std::unique_ptr<TObjectList> DataList(DoCollectWorkspace());

  bool AnyNonStoredSessionWithPassword = false;
  bool AnyNonStoredNonWorkspaceSessionWithPassword = false;
  bool AllNonStoredSessionsAnonymous = true;

  TTerminalManager * Manager = TTerminalManager::Instance();
  for (int Index = 0; Index < DataList->Count; Index++)
  {
    TSessionData * SessionData = dynamic_cast<TSessionData *>(DataList->Items[Index]);

    if (SessionData->Link.IsEmpty())
    {
      if (SessionData->HasAnySessionPassword())
      {
        AnyNonStoredSessionWithPassword = true;
        if (!Manager->Sessions[Index]->SessionData->IsWorkspace)
        {
          AnyNonStoredNonWorkspaceSessionWithPassword = true;
        }
      }

      if (!SameText(SessionData->UserName, AnonymousUserName))
      {
        AllNonStoredSessionsAnonymous = false;
      }
    }
  }

  bool SavePasswords;
  bool * PSavePasswords;
  bool NotRecommendedSavingPasswords =
    !AppConfiguration->UseMasterPassword &&
    !AllNonStoredSessionsAnonymous;

  if (Configuration->DisablePasswordStoring ||
      !AnyNonStoredSessionWithPassword)
  {
    PSavePasswords = NULL;
    SavePasswords = false;
  }
  else
  {
    PSavePasswords = &SavePasswords;
    SavePasswords =
      !AnyNonStoredNonWorkspaceSessionWithPassword ||
      !NotRecommendedSavingPasswords;
  }

  UnicodeString Name = WorkspaceName();
  bool CreateShortcut = false;

  bool Result =
    DoSaveWorkspaceDialog(
      Name, PSavePasswords, NotRecommendedSavingPasswords, CreateShortcut,
      EnableAutoSave);

  if (Result)
  {
    DoSaveWorkspace(Name, DataList.get(), SavePasswords, true);

    if (CreateShortcut)
    {
      CreateDesktopSessionShortCut(Name);
    }

    if (EnableAutoSave)
    {
      AppConfiguration->AutoSaveWorkspace = true;
      AppConfiguration->AutoWorkspace = Name;
      if (PSavePasswords != NULL)
      {
        AppConfiguration->AutoSaveWorkspacePasswords = SavePasswords;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SaveWorkspaceActionExecute()
{
  if (!IsBusy() && (FManagedSession != NULL))
  {
    SaveWorkspace(false);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OpenStoredSession(TSessionData * Data)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  TManagedTerminal * Terminal = Manager->NewManagedTerminal(Data);
  Manager->ActiveSession = Terminal;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::DoOpenFolderOrWorkspace(
  const UnicodeString & Name, bool ConnectFirstTerminal, bool CheckMaxSessions)
{
  TTerminalManager * Manager = TTerminalManager::Instance();
  std::unique_ptr<TObjectList> DataList(new TObjectList());
  // This asks for a master password. Maybe we should postpone asking until a connection is actually opened.
  StoredSessions->GetFolderOrWorkspace(Name, DataList.get());
  if (!CheckMaxSessions || (DataList->Count <= AppConfiguration->MaxSessions))
  {
    TManagedTerminal * FirstSession = Manager->NewSessions(DataList.get());
    // FirstSession can be null, if none of the workspace sites exist anymore
    if (FirstSession != NULL)
    {
      if (!ConnectFirstTerminal)
      {
        FirstSession->Disconnected = true;
        FirstSession->DisconnectedTemporarily = true;
      }
      Manager->ActiveSession = FirstSession;
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::OpenFolderOrWorkspace(const UnicodeString & Name)
{
  DoOpenFolderOrWorkspace(Name, true, false);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateWorkspacesMenu(QMenu * Menu)
{
  StoredSessions->Reload();
  std::unique_ptr<TStrings> Workspaces(StoredSessions->GetWorkspaces());
  for (int Index = 0; Index < Workspaces->Count; Index++)
  {
    UnicodeString Name = Workspaces->Strings[Index];
    QAction * Action = Menu->addAction(ToQString(EscapeHotkey(Name)));
    Action->setObjectName(QStringLiteral("Workspace%1").arg(Index));
    // OpenSCP: without the second part (Shift opens a new window)
    UnicodeString Hint = FMTLOAD(WORKSPACE_HINT, (Name));
    Action->setToolTip(ToQString(CutToChar(Hint, L'(', true)));
    Action->setEnabled(!IsBusy());
    connect(Action, &QAction::triggered, this, [this, Name]()
    {
      ExecuteHandler([this, Name]()
      {
        if (!IsBusy())
        {
          OpenFolderOrWorkspace(Name);
        }
      });
    });
  }
  // Upstream does not show the menu without workspaces
  if (Workspaces->Count == 0)
  {
    QAction * Action = Menu->addAction(tr("(none)"));
    Action->setEnabled(false);
  }
}
//---------------------------------------------------------------------------
static UnicodeString GetSessionFolderRoot(TSessionData * Data, int Level)
{
  UnicodeString Path = Data->Name;
  UnicodeString Root;
  for (int ALevel = 0; ALevel < Level; ALevel++)
  {
    Root.Insert(CutToChar(Path, L'/', false) + L'/', Root.Length() + 1);
  }
  return Root;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateSessionListMenuLevel(QMenu * Menu, int Index, int Level)
{
  Menu->clear();

  UnicodeString Root;
  if (Level == 0)
  {
    Menu->addAction(SiteManagerAction);
    Root = L"";
  }
  else
  {
    DebugAssert(Index < StoredSessions->Count);
    TSessionData * Data = StoredSessions->Sessions[Index];

    Root = GetSessionFolderRoot(Data, Level);

    QAction * Item = Menu->addAction(ToQString(LoadStr(SAVEDSESSIONFOLDER_THIS_OPEN)));
    Item->setObjectName(QStringLiteral("SessionFolderThisItem"));
    UnicodeString FolderName = Root;
    if (DebugAlwaysTrue(!FolderName.IsEmpty() && FolderName[FolderName.Length()] == L'/'))
    {
      FolderName.Delete(FolderName.Length(), 1);
    }
    // OpenSCP: without the second part (Shift opens a new window)
    UnicodeString Hint = FMTLOAD(SAVEDSESSIONFOLDER_THIS_HINT, (FolderName));
    Item->setToolTip(ToQString(CutToChar(Hint, L'(', true)));
    Item->setEnabled(!IsBusy());
    connect(Item, &QAction::triggered, this, [this, FolderName]()
    {
      ExecuteHandler([this, FolderName]()
      {
        if (!IsBusy())
        {
          OpenFolderOrWorkspace(FolderName);
        }
      });
    });
  }

  Menu->addSeparator();

  QAction * FirstSession = nullptr;
  UnicodeString PrevName;
  while (Index < StoredSessions->Count)
  {
    TSessionData * Data = StoredSessions->Sessions[Index];
    if (!AnsiSameText(Data->Name.SubString(1, Root.Length()), Root))
    {
      // Sessions are sorted, so no chance further sessions may match
      break;
    }
    else if (!Data->IsWorkspace)
    {
      UnicodeString Name = Data->Name.SubString(Root.Length() + 1, Data->Name.Length() - Root.Length());
      int P = Name.Pos(L'/');
      if (P > 0)
      {
        Name.SetLength(P - 1);
      }

      if (Name != PrevName)
      {
        if (P > 0)
        {
          // The folders before the sites
          QMenu * Submenu = new QMenu(ToQString(EscapeHotkey(Name)), Menu);
          Submenu->setIcon(QIcon::fromTheme(QStringLiteral("folder")));
          Submenu->setObjectName(QStringLiteral("SessionFolder%1").arg(Index));
          int SubIndex = Index;
          connect(Submenu, &QMenu::aboutToShow, this, [this, Submenu, SubIndex, Level]()
          {
            ExecuteHandler([this, Submenu, SubIndex, Level]() { CreateSessionListMenuLevel(Submenu, SubIndex, Level + 1); });
          });
          Menu->insertMenu(FirstSession, Submenu);
        }
        else
        {
          QAction * Item = Menu->addAction(ToQString(EscapeHotkey(Name)));
          Item->setObjectName(QStringLiteral("Session%1").arg(Index));
          UnicodeString Hint = FMTLOAD(SAVEDSESSION_HINT, (Data->Name));
          Item->setToolTip(ToQString(CutToChar(Hint, L'(', true)));
          Item->setEnabled(!IsBusy());
          if (FirstSession == nullptr)
          {
            FirstSession = Item;
          }
          UnicodeString SessionName = Data->Name;
          connect(Item, &QAction::triggered, this, [this, SessionName]()
          {
            ExecuteHandler([this, SessionName]()
            {
              // The sites may have been reloaded meanwhile
              TSessionData * AData = static_cast<TSessionData *>(StoredSessions->FindByName(SessionName));
              if (!IsBusy() && (AData != NULL))
              {
                OpenStoredSession(AData);
              }
            });
          });
        }

        PrevName = Name;
      }
    }
    Index++;
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanCloseSessions()
{
  bool CanClose = true;
  TTerminalManager * Manager = TTerminalManager::Instance();

  int ActiveSessions = 0;
  for (int Index = 0; Index < Manager->Count; Index++)
  {
    if (Manager->Sessions[Index]->Active)
    {
      ActiveSessions++;
    }
  }
  // Confirm, when there's at least one active remote session or
  // when there are multiple (inactive or local) sessions and auto workspace saving is not enabled.
  if (((ActiveSessions > 0) || ((Manager->Count > 1) && !AppConfiguration->AutoSaveWorkspace)) &&
      AppConfiguration->ConfirmClosingSession)
  {
    unsigned int Result;
    TMessageParams Params(mpNeverAskAgainCheck);
    UnicodeString Message;
    int Answers = qaOK | qaCancel;
    TTerminal * Terminal = GetTerminal();
    // The current session is the only active session
    if ((ActiveSessions == 1) && (Terminal != NULL) && Terminal->Active)
    {
      if (!AppConfiguration->AutoSaveWorkspace)
      {
        Message = LoadStr(CLOSE_SESSION_WORKSPACE);
      }
      else
      {
        Message = LoadStr(CLOSE_SESSION);
      }
      Message = FORMAT(Message, (Terminal->SessionData->SessionName));
    }
    // Multiple active sessions or one active session, but it's not the current one
    else if (ActiveSessions > 0)
    {
      if (!AppConfiguration->AutoSaveWorkspace)
      {
        Message = LoadStr(CLOSE_SESSIONS_WORKSPACE3);
      }
      else
      {
        Message = LoadStr(CLOSE_SESSIONS);
      }
    }
    // Multiple tabs, but all are inactive or local, and saving of worspace is not enabled
    else
    {
      DebugAssert(Manager->Count > 1);
      DebugAssert(!AppConfiguration->AutoSaveWorkspace);
      Message = LoadStr(CLOSE_WORKSPACE);
    }

    UnicodeString Note;
    if (AppConfiguration->AutoSaveWorkspace)
    {
      Note = FMTLOAD(AUTO_WORKSPACE, (WorkspaceName()));
    }
    else
    {
      Note = LoadStr(AUTO_WORKSPACE_ENABLE);
      Answers = qaYes | qaNo | qaCancel;
    }
    Message = FORMAT("%s\n\n%s", (MainInstructions(Message), Note));

    activateWindow();
    Result = MessageDialog(Message, qtConfirmation, Answers, &Params);

    if (Result == qaNeverAskAgain)
    {
      AppConfiguration->ConfirmClosingSession = false;
    }

    if (Result == qaNo)
    {
      CanClose = SaveWorkspace(true);
      // note that the workspace will be saved redundatly again from FormClose
    }
    else
    {
      CanClose =
        (Result == qaOK) ||
        (Result == qaYes) || // CLOSE_SESSIONS_WORKSPACE variant
        (Result == qaNeverAskAgain);
    }
  }

  // OpenSCP: the transfers of all the sessions (upstream asks for the
  // queue of the current one)
  for (int Index = 0; CanClose && (Index < Manager->Count); Index++)
  {
    TManagedTerminal * Session = Manager->Sessions[Index];
    TTerminalQueue * Queue = Manager->FindQueueForTerminal(Session);
    if (Manager->IsAvailableTerminal(Session) && !Queue->IsEmpty)
    {
      CanClose = CanCloseQueue(Queue);
      break;
    }
  }

  return CanClose;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SessionsClosing()
{
  // Do not save empty workspace
  if (AppConfiguration->AutoSaveWorkspace && (FManagedSession != NULL))
  {
    std::unique_ptr<TObjectList> DataList(DoCollectWorkspace());
    UnicodeString Name = WorkspaceName();
    DoSaveWorkspace(
      Name, DataList.get(),
      !Configuration->DisablePasswordStoring &&
      AppConfiguration->AutoSaveWorkspacePasswords, false);
    AppConfiguration->LastStoredSession = Name;
  }

  AppLog(L"Closing main window");
}
//---------------------------------------------------------------------------
