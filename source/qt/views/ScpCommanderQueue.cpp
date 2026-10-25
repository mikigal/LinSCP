// The transfer queue of the main window (upstream has it in
// forms/CustomScpExplorer.cpp, with the actions of forms/NonVisual.cpp):
// the queue panel below the file panels, its toolbar and the operations
// on the queue items.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <TerminalManager.h>

#include "QueueController.h"
#include "ScpCommander.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QSplitter>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateQueueActions()
{
  auto QueueAction = [this](const char * Name, const QString & Caption, const QString & Hint, const QString & IconName,
    TQueueOperation Operation)
  {
    QAction * Action = new QAction(Caption, this);
    Action->setObjectName(QString::fromLatin1(Name));
    Action->setToolTip(Hint);
    if (!IconName.isEmpty())
    {
      Action->setIcon(QIcon::fromTheme(IconName));
    }
    addAction(Action);
    connect(Action, &QAction::triggered, this, [this, Operation]() { ExecuteQueueOperation(Operation); });
    return Action;
  };

  QueueEnableAction = new QAction(tr("&Process Queue"), this);
  QueueEnableAction->setObjectName(QStringLiteral("QueueEnableAction"));
  QueueEnableAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
  QueueEnableAction->setCheckable(true);
  QueueEnableAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Q));
  addAction(QueueEnableAction);
  connect(QueueEnableAction, &QAction::triggered, this, [this]()
  {
    if (FQueue != NULL)
    {
      FQueue->Enabled = !FQueue->Enabled;
    }
    UpdateQueueActions();
  });
  // Upstream's Ctrl+Q is the Quit of the Linux desktops here
  QueueGoToAction = QueueAction("QueueGoToAction", tr("&Go To"), tr("Go to transfer queue list"), QString(), qoGoTo);
  QueueItemQueryAction = QueueAction("QueueItemQueryAction", tr("&Show Query"),
    tr("Show pending query of selected queue item"), QStringLiteral("dialog-question"), qoItemQuery);
  QueueItemErrorAction = QueueAction("QueueItemErrorAction", tr("&Show Error"),
    tr("Show pending error message of selected queue item"), QStringLiteral("dialog-error"), qoItemError);
  QueueItemPromptAction = QueueAction("QueueItemPromptAction", tr("&Show Prompt"),
    tr("Show pending prompt of selected queue item"), QStringLiteral("dialog-password"), qoItemPrompt);
  QueueItemExecuteAction = QueueAction("QueueItemExecuteAction", tr("&Execute Now"),
    tr("Execute selected queue item now"), QStringLiteral("system-run"), qoItemExecute);
  QueueItemPauseAction = QueueAction("QueueItemPauseAction", tr("&Suspend"),
    tr("Suspend selected queue item"), QStringLiteral("media-playback-pause"), qoItemPause);
  QueueItemResumeAction = QueueAction("QueueItemResumeAction", tr("&Resume"),
    tr("Resume selected suspended queue item"), QStringLiteral("media-playback-start"), qoItemResume);
  QueueItemDeleteAction = QueueAction("QueueItemDeleteAction", tr("&Cancel"),
    tr("Remove selected queue item"), QStringLiteral("process-stop"), qoItemDelete);
  QueueItemUpAction = QueueAction("QueueItemUpAction", tr("Move &Up"),
    tr("Move selected queue item up to be processed earlier"), QStringLiteral("go-up"), qoItemUp);
  QueueItemDownAction = QueueAction("QueueItemDownAction", tr("Move &Down"),
    tr("Move selected queue item down to be processed later"), QStringLiteral("go-down"), qoItemDown);
  QueuePauseAllAction = QueueAction("QueuePauseAllAction", tr("&Suspend All"),
    tr("Suspend all running queue items"), QStringLiteral("media-playback-pause"), qoPauseAll);
  QueueResumeAllAction = QueueAction("QueueResumeAllAction", tr("&Resume All"),
    tr("Resume all suspended queue items"), QStringLiteral("media-playback-start"), qoResumeAll);
  QueueDeleteAllDoneAction = QueueAction("QueueDeleteAllDoneAction", tr("Delete All &Completed"),
    tr("Remove all completed queue items"), QStringLiteral("edit-clear-history"), qoDeleteAllDone);
  QueueDeleteAllAction = QueueAction("QueueDeleteAllAction", tr("&Cancel All"),
    tr("Remove all queue items"), QStringLiteral("process-stop"), qoDeleteAll);

  // What to do once the queue is empty; upstream also shuts down or
  // suspends the computer
  FQueueOnceEmptyGroup = new QActionGroup(this);
  QueueIdleOnceEmptyAction = new QAction(tr("&Stay Idle"), FQueueOnceEmptyGroup);
  QueueIdleOnceEmptyAction->setObjectName(QStringLiteral("QueueIdleOnceEmptyAction"));
  QueueIdleOnceEmptyAction->setToolTip(tr("Stay idle once the queue is empty"));
  QueueDisconnectOnceEmptyAction2 = new QAction(tr("&Disconnect Session"), FQueueOnceEmptyGroup);
  QueueDisconnectOnceEmptyAction2->setObjectName(QStringLiteral("QueueDisconnectOnceEmptyAction2"));
  QueueDisconnectOnceEmptyAction2->setToolTip(tr("Disconnect the session once the queue is empty"));
  for (QAction * Action : FQueueOnceEmptyGroup->actions())
  {
    Action->setCheckable(true);
    addAction(Action);
  }
  QueueIdleOnceEmptyAction->setChecked(true);

  FQueueShowGroup = new QActionGroup(this);
  QueueShowAction = new QAction(tr("&Show"), FQueueShowGroup);
  QueueShowAction->setObjectName(QStringLiteral("QueueShowAction"));
  QueueShowAction->setToolTip(tr("Show queue list"));
  QueueHideWhenEmptyAction = new QAction(tr("Hide when &Empty"), FQueueShowGroup);
  QueueHideWhenEmptyAction->setObjectName(QStringLiteral("QueueHideWhenEmptyAction"));
  QueueHideWhenEmptyAction->setToolTip(tr("Hide queue list when it is empty"));
  QueueHideAction = new QAction(tr("&Hide"), FQueueShowGroup);
  QueueHideAction->setObjectName(QStringLiteral("QueueHideAction"));
  QueueHideAction->setToolTip(tr("Hide queue list"));
  const std::pair<QAction *, TQueueViewShow> ShowActions[] =
    { { QueueShowAction, qvShow }, { QueueHideWhenEmptyAction, qvHideWhenEmpty }, { QueueHideAction, qvHide } };
  for (const std::pair<QAction *, TQueueViewShow> & ShowAction : ShowActions)
  {
    QAction * Action = ShowAction.first;
    TQueueViewShow Show = ShowAction.second;
    Action->setCheckable(true);
    Action->setChecked(AppConfiguration->QueueViewShow == Show);
    addAction(Action);
    connect(Action, &QAction::triggered, this, [this, Show]() { QueueViewShowChange(Show); });
  }
}
//---------------------------------------------------------------------------
QMenu * TScpCommanderForm::CreateQueueMenu()
{
  QMenu * Menu = new QMenu(tr("&Queue"), this);
  Menu->setObjectName(QStringLiteral("QueueMenu"));
  Menu->addAction(QueueEnableAction);
  Menu->addSeparator();
  Menu->addAction(QueueGoToAction);
  Menu->addSeparator();
  Menu->addAction(QueuePauseAllAction);
  Menu->addAction(QueueResumeAllAction);
  Menu->addAction(QueueDeleteAllDoneAction);
  Menu->addAction(QueueDeleteAllAction);
  Menu->addSeparator();
  QMenu * OnceEmptyMenu = Menu->addMenu(tr("Once &Empty"));
  OnceEmptyMenu->setObjectName(QStringLiteral("QueueOnceEmptyMenu"));
  OnceEmptyMenu->addActions(FQueueOnceEmptyGroup->actions());
  return Menu;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::CreateQueuePanel()
{
  QueuePanel = new QWidget(FQueueSplitter);
  QueuePanel->setObjectName(QStringLiteral("QueuePanel"));
  QVBoxLayout * Layout = new QVBoxLayout(QueuePanel);
  Layout->setContentsMargins(0, 0, 0, 0);
  Layout->setSpacing(0);
  QueueLabel = new QLabel(QueuePanel);
  QueueLabel->setObjectName(QStringLiteral("QueueLabel"));
  QueueLabel->setContentsMargins(4, 2, 4, 2);
  Layout->addWidget(QueueLabel);

  QueueToolbar = new QToolBar(tr("Queue"), QueuePanel);
  QueueToolbar->setObjectName(QStringLiteral("QueueToolbar"));
  QueueToolbar->setIconSize(QSize(16, 16));
  QueueToolbar->addAction(QueueEnableAction);
  QueueToolbar->addSeparator();
  for (QAction * Action :
         { QueueItemQueryAction, QueueItemErrorAction, QueueItemPromptAction, QueueItemExecuteAction,
           QueueItemPauseAction, QueueItemResumeAction, QueueItemDeleteAction })
  {
    QueueToolbar->addAction(Action);
  }
  QueueToolbar->addSeparator();
  QueueToolbar->addAction(QueueItemUpAction);
  QueueToolbar->addAction(QueueItemDownAction);
  QueueToolbar->addSeparator();
  QueueToolbar->addAction(QueueDeleteAllDoneAction);
  QueueToolbar->addSeparator();
  QToolButton * OnceEmptyButton = new QToolButton(QueueToolbar);
  OnceEmptyButton->setObjectName(QStringLiteral("QueueCycleOnceEmptyButton"));
  OnceEmptyButton->setIcon(QIcon::fromTheme(QStringLiteral("system-shutdown")));
  OnceEmptyButton->setToolTip(tr("Action to perform once the queue list is empty"));
  OnceEmptyButton->setPopupMode(QToolButton::InstantPopup);
  QMenu * OnceEmptyMenu = new QMenu(OnceEmptyButton);
  OnceEmptyMenu->addActions(FQueueOnceEmptyGroup->actions());
  OnceEmptyButton->setMenu(OnceEmptyMenu);
  QueueToolbar->addWidget(OnceEmptyButton);
  Layout->addWidget(QueueToolbar);

  QueueView3 = new QTreeWidget(QueuePanel);
  QueueView3->setObjectName(QStringLiteral("QueueView3"));
  QueueView3->setRootIsDecorated(false);
  QueueView3->setUniformRowHeights(true);
  QueueView3->setSelectionMode(QAbstractItemView::SingleSelection);
  QueueView3->setHeaderLabels(
    { tr("Operation"), tr("Source"), tr("Destination"), tr("Transferred"), tr("Time"), tr("Speed"), tr("Progress") });
  const int Widths[] = { 70, 170, 170, 80, 80, 80, 80 };
  for (int Column = 0; Column < static_cast<int>(std::size(Widths)); Column++)
  {
    QueueView3->setColumnWidth(Column, Widths[Column]);
  }
  QueueView3->setContextMenuPolicy(Qt::ActionsContextMenu);
  for (QAction * Action :
         { QueueItemQueryAction, QueueItemErrorAction, QueueItemPromptAction, QueueItemExecuteAction,
           QueueItemPauseAction, QueueItemResumeAction, QueueItemDeleteAction, QueueItemUpAction, QueueItemDownAction })
  {
    QueueView3->addAction(Action);
  }
  connect(QueueView3, &QTreeWidget::currentItemChanged, this, [this]() { UpdateQueueActions(); });
  Layout->addWidget(QueueView3, 1);

  FQueueController = new TQueueController(QueueView3);
  FQueueController->OnChange = [this]() { UpdateQueueActions(); };
  UpdateQueueLabel();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SetQueue(TTerminalQueue * Value)
{
  if (FQueue != Value)
  {
    if (FQueue != NULL)
    {
      FQueue->OnListUpdate = NULL;
      FQueue->OnQueueItemUpdate = NULL;
    }
    FQueue = Value;
    if (FQueue != NULL)
    {
      DebugAssert(FQueue->OnListUpdate == NULL);
      FQueue->OnListUpdate = ::rtl::MethodPtr(this, &TScpCommanderForm::QueueListUpdate);
      DebugAssert(FQueue->OnQueueItemUpdate == NULL);
      FQueue->OnQueueItemUpdate = ::rtl::MethodPtr(this, &TScpCommanderForm::QueueItemUpdate);
    }
    QueueChanged();
  }
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::IsAnythingQueued()
{
  return (FQueueStatus != NULL) && (FQueueStatus->ActiveAndPendingPrimaryCount > 0);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateQueueStatus(bool QueueChanging)
{
  {
    std::lock_guard<std::mutex> Guard(FQueueStatusSection);

    FQueueStatusInvalidated = false;

    if (FQueue != NULL)
    {
      FQueueStatus = FQueue->CreateStatus(FQueueStatus);
    }
  }

  {
    TAutoFlag Flag(FQueueStatusUpdating);
    FQueueController->UpdateQueueStatus(FQueueStatus);
  }
  UpdateQueueLabel();

  UpdateQueueView();

  bool IsEmpty = !IsAnythingQueued();

  TTerminal * Terminal = GetTerminal();
  if (IsEmpty && (Terminal != NULL))
  {
    TOnceDoneOperation OnceDoneOperation = CurrentQueueOnceEmptyOperation();
    ResetQueueOnceEmptyOperation();

    if ((FQueue != NULL) && !AppConfiguration->EnableQueueByDefault && !QueueChanging)
    {
      FQueue->Enabled = false;
    }

    if ((OnceDoneOperation != odoIdle) && !IsBusy())
    {
      Terminal->CloseOnCompletion(OnceDoneOperation, LoadStr(CLOSED_ON_QUEUE_EMPTY));
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateQueueView()
{
  TQueueViewShow Show = AppConfiguration->QueueViewShow;
  bool Visible =
    (Show == qvShow) ||
    ((Show == qvHideWhenEmpty) && (FQueueStatus != NULL) && (FQueueStatus->Count > 0));
  if (QueuePanel->isHidden() == Visible)
  {
    QueuePanel->setVisible(Visible);
    if (Visible)
    {
      FQueueSplitter->setSizes({ FQueueSplitter->height() - AppConfiguration->QueueViewHeight, AppConfiguration->QueueViewHeight });
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::QueueViewShowChange(TQueueViewShow Show)
{
  AppConfiguration->QueueViewShow = Show;
  UpdateQueueView();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::QueueChanged()
{
  if (FQueueStatus != NULL)
  {
    // UpdateFileList implementation relies on the status being recreated when session changes
    delete FQueueStatus;
    FQueueStatus = NULL;
  }
  UpdateQueueStatus(true);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::QueueListUpdate(TTerminalQueue * Queue)
{
  // In the threads of the queue
  if (FQueue == Queue)
  {
    FQueueStatusInvalidated = true;
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::QueueItemUpdate(TTerminalQueue * Queue, TQueueItem * Item)
{
  // In the threads of the queue
  if (FQueue == Queue)
  {
    std::lock_guard<std::mutex> Guard(FQueueStatusSection);

    // this may be running in parallel with QueueChanged
    if (FQueueStatus != NULL)
    {
      TQueueItemProxy * QueueItem = FQueueStatus->FindByQueueItem(Item);

      if (Item->Status == TQueueItem::qsDone)
      {
        FRefreshLocalDirectory = (QueueItem == NULL) ||
          (!QueueItem->Info->ModifiedLocal.IsEmpty());
        FRefreshRemoteDirectory = (QueueItem == NULL) ||
          (!QueueItem->Info->ModifiedRemote.IsEmpty());
      }

      if (QueueItem != NULL)
      {
        QueueItem->UserData = reinterpret_cast<void *>(true);
        FQueueItemInvalidated = true;
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RefreshQueueItems()
{
  if (FQueueStatus != NULL)
  {
    bool QueueAutoPopup = GUIConfiguration->QueueAutoPopup;
    bool Refresh = FQueueItemInvalidated;
    FQueueItemInvalidated = false;

    int Limit = Refresh ? FQueueStatus->Count : FQueueStatus->DoneAndActiveCount;

    bool Updated = false;
    for (int Index = 0; Index < Limit; Index++)
    {
      bool Update = false;
      TQueueItemProxy * QueueItem = FQueueStatus->Items[Index];
      bool WasUserAction = TQueueItem::IsUserActionStatus(QueueItem->Status);
      if (!IsBusy() && QueueAutoPopup && WasUserAction &&
          (FPendingQueueActionItem == NULL))
      {
        FPendingQueueActionItem = QueueItem;
        QTimer::singleShot(10, this, [this]() { UserActionTimer(); });
      }

      if (reinterpret_cast<intptr_t>(QueueItem->UserData) != 0)
      {
        QueueItem->UserData = reinterpret_cast<void *>(false);
        QueueItem->Update();
        Updated = true;
        Update = true;
      }
      else if (FQueueController->QueueItemNeedsFrequentRefresh(QueueItem))
      {
        Update = true;
      }

      if (Update)
      {
        FQueueController->RefreshQueueItem(QueueItem);
      }
    }

    if (Updated)
    {
      UpdateQueueActions();
      UpdateQueueLabel();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UserActionTimer()
{
  ::rtl::TFinally Finally([&]() { FPendingQueueActionItem = NULL; });
  // The status may have been recreated meanwhile
  if (GUIConfiguration->QueueAutoPopup && (FPendingQueueActionItem != NULL) && (FQueueStatus != NULL))
  {
    bool Valid = false;
    for (int Index = 0; !Valid && (Index < FQueueStatus->Count); Index++)
    {
      Valid = (FQueueStatus->Items[Index] == FPendingQueueActionItem);
    }
    if (Valid && TQueueItem::IsUserActionStatus(FPendingQueueActionItem->Status))
    {
      FPendingQueueActionItem->ProcessUserAction();
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateQueueLabel()
{
  UnicodeString Caption = LoadStr(QUEUE_CAPTION);
  if (FQueueStatus != NULL)
  {
    int ActiveAndPendingPrimaryCount = FQueueStatus->ActiveAndPendingPrimaryCount;
    if (ActiveAndPendingPrimaryCount > 0)
    {
      Caption = FORMAT(L"%s (%d)", (Caption, ActiveAndPendingPrimaryCount));
    }
  }
  QueueLabel->setText(ToQString(Caption));
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::AllowQueueOperation(TQueueOperation Operation)
{
  switch (Operation)
  {
    case qoGoTo:
      return !QueuePanel->isHidden() && QueueView3->isEnabled();

    case qoOnceEmpty:
      return IsAnythingQueued();

    default:
      return FQueueController->AllowOperation(Operation);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ExecuteQueueOperation(TQueueOperation Operation)
{
  if (Operation == qoGoTo)
  {
    if (DebugAlwaysTrue(!QueuePanel->isHidden()))
    {
      QueueView3->setFocus();
    }
  }
  else if (AllowQueueOperation(Operation))
  {
    FQueueController->ExecuteOperation(Operation);
  }
  UpdateQueueActions();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateQueueActions()
{
  bool HasQueue = (FQueue != NULL);
  QueueEnableAction->setEnabled(HasQueue);
  QueueEnableAction->setChecked(HasQueue && FQueue->Enabled);
  QueueGoToAction->setEnabled(AllowQueueOperation(qoGoTo));
  const std::pair<QAction *, TQueueOperation> Actions[] =
  {
    { QueueItemQueryAction, qoItemQuery }, { QueueItemErrorAction, qoItemError },
    { QueueItemPromptAction, qoItemPrompt }, { QueueItemExecuteAction, qoItemExecute },
    { QueueItemPauseAction, qoItemPause }, { QueueItemResumeAction, qoItemResume },
    { QueueItemDeleteAction, qoItemDelete }, { QueueItemUpAction, qoItemUp }, { QueueItemDownAction, qoItemDown },
    { QueuePauseAllAction, qoPauseAll }, { QueueResumeAllAction, qoResumeAll },
    { QueueDeleteAllDoneAction, qoDeleteAllDone }, { QueueDeleteAllAction, qoDeleteAll },
  };
  for (const std::pair<QAction *, TQueueOperation> & Action : Actions)
  {
    Action.first->setEnabled(HasQueue && AllowQueueOperation(Action.second));
  }
  FQueueOnceEmptyGroup->setEnabled(AllowQueueOperation(qoOnceEmpty));
}
//---------------------------------------------------------------------------
TOnceDoneOperation TScpCommanderForm::CurrentQueueOnceEmptyOperation()
{
  return QueueDisconnectOnceEmptyAction2->isChecked() ? odoDisconnect : odoIdle;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ResetQueueOnceEmptyOperation()
{
  QueueIdleOnceEmptyAction->setChecked(true);
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::CanCloseQueue()
{
  bool Result = (FQueue == NULL) || FQueue->IsEmpty;
  if (!Result)
  {
    Result = (MessageDialog(LoadStr(PENDING_QUEUE_ITEMS2), qtWarning, qaOK | qaCancel) == qaOK);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TScpCommanderForm::AddQueueItem(TTransferDirection Direction, TStrings * FileList,
  const UnicodeString & TargetDirectory, TGUICopyParamType & CopyParam, int Params)
{
  DebugAssert(FQueue != NULL);
  TTerminal * Terminal = GetTerminal();

  TQueueItem * QueueItem;
  if (Direction == tdToRemote)
  {
    CopyParam.IncludeFileMask.SetRoots(FileList, TargetDirectory);
    QueueItem =
      new TUploadQueueItem(
        Terminal, FileList, TargetDirectory, &CopyParam, Params, CopyParam.QueueParallel);
  }
  else
  {
    CopyParam.IncludeFileMask.SetRoots(TargetDirectory, FileList);
    QueueItem =
      new TDownloadQueueItem(
        Terminal, FileList, TargetDirectory, &CopyParam, Params, CopyParam.QueueParallel);
  }
  FQueue->AddItem(QueueItem);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::QueueEvent(TTerminal * /*Terminal*/, TTerminalQueue * /*Queue*/, TQueueEvent Event)
{
  // Upstream notifies (a balloon of the tray icon) that the queue got
  // empty or waits for the user, and signals the completion of the
  // operation (a beep); the notifications are of milestone M7
  switch (Event)
  {
    case qeEmptyButMonitored:
    case qeEmpty:
    case qePendingUserAction:
      break;

    default:
      DebugFail();
  }
}
//---------------------------------------------------------------------------
