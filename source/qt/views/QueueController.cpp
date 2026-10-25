// The list of the transfer queue (upstream's windows/QueueController.cpp).
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <Queue.h>

#include "QueueController.h"

#include <QApplication>
#include <QKeyEvent>
#include <QPalette>
#include <QTreeWidget>
//---------------------------------------------------------------------------
// The size format of the file panels (upstream's FormatSizeBytes preference)
static const TFormatBytesStyle FormatSizeBytes = fbKilobytes;
//---------------------------------------------------------------------------
TQueueController::TQueueController(QTreeWidget * ListView) :
  QObject(ListView)
{
  FListView = ListView;
  DebugAssert(FListView != NULL);
  connect(FListView, &QTreeWidget::itemDoubleClicked, this, [this]() { QueueViewDblClick(); });
  FListView->installEventFilter(this);
  FQueueStatus = NULL;
}
//---------------------------------------------------------------------------
TQueueController::~TQueueController()
{
}
//---------------------------------------------------------------------------
TQueueItemProxy * TQueueController::QueueViewItemToQueueItem(QTreeWidgetItem * Item)
{
  return static_cast<TQueueItemProxy *>(Item->data(0, Qt::UserRole).value<void *>());
}
//---------------------------------------------------------------------------
TQueueOperation TQueueController::DefaultOperation()
{
  if (FListView->currentItem() != NULL)
  {
    TQueueItemProxy * QueueItem = QueueViewItemToQueueItem(FListView->currentItem());

    switch (QueueItem->Status)
    {
      case TQueueItem::qsPending:
        return qoItemExecute;

      case TQueueItem::qsQuery:
        return qoItemQuery;

      case TQueueItem::qsError:
        return qoItemError;

      case TQueueItem::qsPrompt:
        return qoItemPrompt;

      case TQueueItem::qsProcessing:
        return qoItemPause;

      case TQueueItem::qsPaused:
        return qoItemResume;

      default:
        break;
    }
  }

  return qoNone;
}
//---------------------------------------------------------------------------
bool TQueueController::AllowOperation(TQueueOperation Operation, unsigned long * Param)
{
  TQueueItemProxy * QueueItem = NULL;
  int FocusedIndex = -1;

  if (FListView->currentItem() != NULL)
  {
    QueueItem = QueueViewItemToQueueItem(FListView->currentItem());
    FocusedIndex = FListView->indexOfTopLevelItem(FListView->currentItem());
  }

  switch (Operation)
  {
    case qoItemUserAction:
      return (QueueItem != NULL) && TQueueItem::IsUserActionStatus(QueueItem->Status);

    case qoItemQuery:
      return (QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsQuery);

    case qoItemError:
      return (QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsError);

    case qoItemPrompt:
      return (QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsPrompt);

    case qoItemDelete:
      return (QueueItem != NULL);

    case qoItemExecute:
      return (QueueItem != NULL) && (QueueItem->Status == TQueueItem::qsPending);

    case qoItemUp:
      return (QueueItem != NULL) &&
        (QueueItem->Status == TQueueItem::qsPending) &&
        // it's not first pending item,
        // this is based on assumption that pending items occupy single line always
        (FocusedIndex > 0) &&
        (QueueViewItemToQueueItem(FListView->topLevelItem(FocusedIndex - 1))->Status == TQueueItem::qsPending);

    case qoItemDown:
      return (QueueItem != NULL) &&
        (QueueItem->Status == TQueueItem::qsPending) &&
        (FocusedIndex < (FListView->topLevelItemCount() - 1));

    case qoItemPause:
      return (QueueItem != NULL) &&
        (QueueItem->Status == TQueueItem::qsProcessing);

    case qoItemResume:
      return (QueueItem != NULL) &&
        (QueueItem->Status == TQueueItem::qsPaused);

    case qoItemSpeed:
      {
        bool Result =
          (QueueItem != NULL) && (QueueItem->Status != TQueueItem::qsDone) &&
          TFileOperationProgressType::IsTransferOperation(QueueItem->Info->Operation);
        if (Result && (Param != NULL))
        {
          unsigned int CPSLimit = 0;
          Result = QueueItem->GetCPSLimit(CPSLimit);
          *Param = CPSLimit;
        }
        return Result;
      }

    case qoPauseAll:
    case qoResumeAll:
      {
        TQueueItem::TStatus Status =
          (Operation == qoPauseAll) ? TQueueItem::qsProcessing : TQueueItem::qsPaused;
        bool Result = false;
        // can be NULL when action update is triggered while disconnecting
        if (FQueueStatus != NULL)
        {
          for (int i = FQueueStatus->DoneCount; !Result && (i < FQueueStatus->DoneAndActiveCount); i++)
          {
            QueueItem = FQueueStatus->Items[i];
            Result = (QueueItem->Status == Status);
          }
        }
        return Result;
      }

    case qoDeleteAllDone:
      return (FQueueStatus != NULL) && (FQueueStatus->DoneCount > 0);

    case qoDeleteAll:
      return (FQueueStatus != NULL) && (FQueueStatus->Count > 0);

    default:
      DebugFail();
      return false;
  }
}
//---------------------------------------------------------------------------
void TQueueController::ExecuteOperation(TQueueOperation Operation, unsigned long Param)
{
  TQueueItemProxy * QueueItem = NULL;

  if (FListView->currentItem() != NULL)
  {
    QueueItem = QueueViewItemToQueueItem(FListView->currentItem());
  }

  switch (Operation)
  {
    case qoItemUserAction:
    case qoItemQuery:
    case qoItemError:
    case qoItemPrompt:
      if (QueueItem != NULL)
      {
        QueueItem->ProcessUserAction();
      }
      break;

    case qoItemExecute:
      if (QueueItem != NULL)
      {
        QueueItem->ExecuteNow();
      }
      break;

    case qoItemUp:
    case qoItemDown:
      if (QueueItem != NULL)
      {
        QueueItem->Move(Operation == qoItemUp);
      }
      break;

    case qoItemDelete:
      if (QueueItem != NULL)
      {
        QueueItem->Delete();
      }
      break;

    case qoItemPause:
      if (QueueItem != NULL)
      {
        QueueItem->Pause();
      }
      break;

    case qoItemResume:
      if (QueueItem != NULL)
      {
        QueueItem->Resume();
      }
      break;

    case qoItemSpeed:
      if (QueueItem != NULL)
      {
        QueueItem->SetCPSLimit(Param);
      }
      break;

    case qoPauseAll:
    case qoResumeAll:
      {
        for (int i = FQueueStatus->DoneCount; i < FQueueStatus->DoneAndActiveCount; i++)
        {
          QueueItem = FQueueStatus->Items[i];
          if ((Operation == qoPauseAll) && (QueueItem->Status == TQueueItem::qsProcessing))
          {
            QueueItem->Pause();
          }
          else if ((Operation == qoResumeAll) && (QueueItem->Status == TQueueItem::qsPaused))
          {
            QueueItem->Resume();
          }
        }
      }
      break;

    case qoDeleteAllDone:
    case qoDeleteAll:
      {
        int Count = (Operation == qoDeleteAll) ? FQueueStatus->Count : FQueueStatus->DoneCount;
        for (int i = 0; i < Count; i++)
        {
          QueueItem = FQueueStatus->Items[i];
          QueueItem->Delete();
        }
      }
      break;

    default:
      DebugFail();
      break;
  }
}
//---------------------------------------------------------------------------
static UnicodeString GetTime(TFileOperationProgressType * ProgressData)
{
  UnicodeString Result;
  if (ProgressData->TotalSizeSet)
  {
    Result = FormatDateTimeSpan(ProgressData->TotalTimeLeft());
  }
  else
  {
    Result = FormatDateTimeSpan(ProgressData->TimeElapsed());
  }
  return Result;
}
//---------------------------------------------------------------------------
static UnicodeString FormatProgress(int Progress)
{
  return FORMAT(L"%d%%", (Progress));
}
//---------------------------------------------------------------------------
static UnicodeString FormatOverallProgress(TFileOperationProgressType * ProgressData)
{
  return FormatProgress(ProgressData->OverallProgress());
}
//---------------------------------------------------------------------------
// The images of upstream's queue image list, from the icon theme
static QIcon QueueImage(int Index)
{
  static const char * Names[] =
  {
    "go-down", "go-bottom", "go-up", "go-top", "dialog-question", "dialog-error", "dialog-password",
    "media-playback-pause", "edit-delete"
  };
  return ((Index >= 0) && (Index < static_cast<int>(std::size(Names)))) ?
    QIcon::fromTheme(QString::fromLatin1(Names[Index])) : QIcon();
}
//---------------------------------------------------------------------------
void TQueueController::FillQueueViewItem(QTreeWidgetItem * Item,
  TQueueItemProxy * QueueItem, bool Detail, bool OnlyLine)
{
  DebugAssert(!Detail || (QueueItem->Status != TQueueItem::qsPending));

  Item->setData(0, Qt::UserRole, QVariant::fromValue(static_cast<void *>(QueueItem)));

  UnicodeString ProgressStr;
  int Image = -1;

  switch (QueueItem->Status)
  {
    case TQueueItem::qsDone:
      ProgressStr = LoadStr(QUEUE_DONE);
      break;

    case TQueueItem::qsPending:
      ProgressStr = LoadStr(QUEUE_PENDING);
      break;

    case TQueueItem::qsConnecting:
      ProgressStr = LoadStr(QUEUE_CONNECTING);
      break;

    case TQueueItem::qsQuery:
      ProgressStr = LoadStr(QUEUE_QUERY);
      Image = 4;
      break;

    case TQueueItem::qsError:
      ProgressStr = LoadStr(QUEUE_ERROR);
      Image = 5;
      break;

    case TQueueItem::qsPrompt:
      ProgressStr = LoadStr(QUEUE_PROMPT);
      Image = 6;
      break;

    case TQueueItem::qsPaused:
      ProgressStr = LoadStr(QUEUE_PAUSED);
      Image = 7;
      break;

    default:
      break;
  }

  int State = -1;
  UnicodeString Values[6];
  TFileOperationProgressType * ProgressData = QueueItem->ProgressData;
  TQueueItem::TInfo * Info = QueueItem->Info;
  UnicodeString OperationName;

  if (SimpleOperation(QueueItem))
  {
    DebugAssert(!Detail && Info->Primary);
    State = 8;

    if ((ProgressData != NULL) && (QueueItem->Status != TQueueItem::qsDone))
    {
      Values[0] = ProgressData->FileName;

      Values[3] = GetTime(ProgressData);

      if (ProgressStr.IsEmpty())
      {
        if (ProgressData->Count > 1)
        {
          ProgressStr = FormatOverallProgress(ProgressData);
        }
        else
        {
          ProgressStr = LoadStr(QUEUE_DELETING);
        }
      }
    }
    else
    {
      Values[0] = Info->Source;
    }

    Values[5] = ProgressStr;
  }
  else if (!Detail && Info->Primary)
  {
    switch (Info->Operation)
    {
      case foCopy:
        State = ((Info->Side == osLocal) ? 2 : 0);
        break;

      case foMove:
        State = ((Info->Side == osLocal) ? 3 : 1);
        break;

      default:
        DebugFail();
    }

    if (!OnlyLine)
    {
      Image = -1;
      ProgressStr = L"";
    }

    // If both are empty, it's bootstrap item => do not show anything
    if (!Info->Source.IsEmpty() || !Info->Destination.IsEmpty())
    {
      // cannot use ProgressData->Temp as it is set only after the transfer actually starts
      Values[0] = Info->Source.IsEmpty() ? LoadStr(PROGRESS_TEMP_DIR) : Info->Source;
      Values[1] = Info->Destination.IsEmpty() ? LoadStr(PROGRESS_TEMP_DIR) : Info->Destination;
    }

    __int64 TotalTransferred = QueueItem->TotalTransferred;
    if (TotalTransferred >= 0)
    {
      Values[2] = FormatPanelBytes(TotalTransferred, FormatSizeBytes);
    }

    if (ProgressData != NULL)
    {
      if (ProgressData->Operation == Info->Operation)
      {
        if (QueueItem->Status != TQueueItem::qsDone)
        {
          UnicodeString Time = GetTime(ProgressData);
          unsigned int CPS = ProgressData->CPS();
          Values[3] = Time;
          Values[4] = FORMAT(L"%s/s", (FormatBytes(CPS)));
        }

        if (ProgressStr.IsEmpty())
        {
          ProgressStr = FormatOverallProgress(ProgressData);
        }
      }
      else if (ProgressData->Operation == foCalculateSize)
      {
        ProgressStr = LoadStr(QUEUE_LISTING);
      }
    }
    Values[5] = ProgressStr;
  }
  else
  {
    if (ProgressData != NULL)
    {
      UnicodeString FileName;
      if ((Info->Side == osRemote) || !ProgressData->Temp)
      {
        FileName = ProgressData->FileName;
      }
      else
      {
        FileName = ExtractFileName(ProgressData->FileName);
      }
      Values[0] = FileName;

      if (ProgressData->Operation == Info->Operation)
      {
        __int64 TransferredSize = ProgressData->TransferredSize;
        Values[2] = FormatPanelBytes(TransferredSize, FormatSizeBytes);

        if (ProgressStr.IsEmpty())
        {
          int Progress = ProgressData->TransferProgress();
          ProgressStr = FormatProgress(Progress);
        }
      }
    }
    Values[5] = ProgressStr;
  }

  // The state and the image of the list view items are both shown as the
  // icon of the first column, the image (of a pending user action) first
  int Icon = (Image >= 0) ? Image : State;
  Item->setIcon(0, QueueImage(Icon));
  QString Operation;
  switch (State)
  {
    case 0: Operation = QObject::tr("Download"); break;
    case 1: Operation = QObject::tr("Download and delete"); break;
    case 2: Operation = QObject::tr("Upload"); break;
    case 3: Operation = QObject::tr("Upload and delete"); break;
    case 8: Operation = QObject::tr("Delete"); break;
  }
  Item->setToolTip(0, Operation);
  for (int Index = 0; Index < static_cast<int>(std::size(Values)); Index++)
  {
    Item->setText(Index + 1, ToQString(Values[Index]));
  }
  // The completed items are grayed, as upstream draws them
  QBrush Foreground =
    (QueueItem->Status == TQueueItem::qsDone) ?
      QApplication::palette().brush(QPalette::Disabled, QPalette::Text) : QBrush();
  for (int Column = 0; Column < Item->columnCount(); Column++)
  {
    Item->setForeground(Column, Foreground);
  }
}
//---------------------------------------------------------------------------
QTreeWidgetItem * TQueueController::InsertItemFor(TQueueItemProxy * QueueItem, int Index)
{
  QTreeWidgetItem * Item;
  if (Index == FListView->topLevelItemCount())
  {
    Item = new QTreeWidgetItem();
    FListView->addTopLevelItem(Item);
  }
  else if (QueueViewItemToQueueItem(FListView->topLevelItem(Index)) != QueueItem)
  {
    Item = new QTreeWidgetItem();
    FListView->insertTopLevelItem(Index, Item);
  }
  else
  {
    Item = FListView->topLevelItem(Index);
  }
  return Item;
}
//---------------------------------------------------------------------------
void TQueueController::UpdateQueueStatus(TTerminalQueueStatus * QueueStatus)
{
  FQueueStatus = QueueStatus;

  if (FQueueStatus != NULL)
  {
    int Index = 0;
    for (int ItemIndex = 0; ItemIndex < FQueueStatus->Count; ItemIndex++)
    {
      TQueueItemProxy * QueueItem = FQueueStatus->Items[ItemIndex];

      int Index2 = Index;
      while ((Index2 < FListView->topLevelItemCount()) &&
             (QueueViewItemToQueueItem(FListView->topLevelItem(Index2)) != QueueItem))
      {
        Index2++;
      }

      if (Index2 < FListView->topLevelItemCount())
      {
        while (Index < Index2)
        {
          delete FListView->takeTopLevelItem(Index);
          Index2--;
        }
      }

      QTreeWidgetItem * Item = InsertItemFor(QueueItem, Index);
      bool HasDetailsLine = UseDetailsLine(ItemIndex, QueueItem);
      FillQueueViewItem(Item, QueueItem, false, !HasDetailsLine);
      Index++;

      DebugAssert((QueueItem->Status != TQueueItem::qsPending) ==
        (ItemIndex < FQueueStatus->DoneAndActiveCount));

      if (HasDetailsLine)
      {
        Item = InsertItemFor(QueueItem, Index);
        FillQueueViewItem(Item, QueueItem, true, false);
        Index++;
      }
    }

    while (Index < FListView->topLevelItemCount())
    {
      delete FListView->takeTopLevelItem(Index);
    }
  }
  else
  {
    FListView->clear();
  }

  DoChange();
}
//---------------------------------------------------------------------------
bool TQueueController::SimpleOperation(TQueueItemProxy * QueueItem)
{
  return (QueueItem->Info->Operation == foDelete); // basically any non-transfer, but we support delete now only
}
//---------------------------------------------------------------------------
bool TQueueController::UseDetailsLine(int ItemIndex, TQueueItemProxy * QueueItem)
{
  return
    (ItemIndex >= FQueueStatus->DoneCount) &&
    (ItemIndex < FQueueStatus->DoneAndActiveCount) &&
    QueueItem->Info->Primary &&
    !QueueItem->Info->SingleFile &&
    !SimpleOperation(QueueItem) &&
    ((QueueItem->ProgressData == NULL) || !QueueItem->ProgressData->Done);
}
//---------------------------------------------------------------------------
void TQueueController::RefreshQueueItem(TQueueItemProxy * QueueItem)
{
  int Index = -1;
  for (int I = 0; (Index < 0) && (I < FListView->topLevelItemCount()); I++)
  {
    if (QueueViewItemToQueueItem(FListView->topLevelItem(I)) == QueueItem)
    {
      Index = I;
    }
  }
  if (DebugAlwaysFalse(Index < 0))
  {
    return;
  }
  QTreeWidgetItem * ListItem = FListView->topLevelItem(Index);

  QTreeWidgetItem * NextListItem = NULL;
  if (Index + 1 < FListView->topLevelItemCount())
  {
    NextListItem = FListView->topLevelItem(Index + 1);
    if (QueueViewItemToQueueItem(NextListItem) != QueueItem)
    {
      NextListItem = NULL;
    }
  }

  bool HasDetailsLine = UseDetailsLine(QueueItem->Index, QueueItem);
  FillQueueViewItem(ListItem, QueueItem, false, !HasDetailsLine);

  if (HasDetailsLine)
  {
    if (NextListItem == NULL)
    {
      NextListItem = new QTreeWidgetItem();
      FListView->insertTopLevelItem(Index + 1, NextListItem);
    }
    FillQueueViewItem(NextListItem, QueueItem, true, false);
  }
  else
  {
    delete NextListItem;
  }

  DoChange();
}
//---------------------------------------------------------------------------
bool TQueueController::QueueItemNeedsFrequentRefresh(TQueueItemProxy * QueueItem)
{
  return
    (TQueueItem::IsUserActionStatus(QueueItem->Status) ||
     (QueueItem->Status == TQueueItem::qsPaused));
}
//---------------------------------------------------------------------------
void TQueueController::DoChange()
{
  if (OnChange)
  {
    OnChange();
  }
}
//---------------------------------------------------------------------------
void TQueueController::QueueViewDblClick()
{
  TQueueOperation Operation = DefaultOperation();

  if (Operation != qoNone)
  {
    ExecuteOperation(Operation);
  }
}
//---------------------------------------------------------------------------
bool TQueueController::eventFilter(QObject * Object, QEvent * Event)
{
  if ((Object == FListView) && (Event->type() == QEvent::KeyPress))
  {
    QKeyEvent * KeyEvent = static_cast<QKeyEvent *>(Event);
    if ((KeyEvent->key() == Qt::Key_Return) || (KeyEvent->key() == Qt::Key_Enter))
    {
      TQueueOperation Operation = DefaultOperation();

      if (Operation != qoNone)
      {
        ExecuteOperation(Operation);
      }
      return true;
    }
    else if (KeyEvent->key() == Qt::Key_Delete)
    {
      if (AllowOperation(qoItemDelete))
      {
        ExecuteOperation(qoItemDelete);
      }
      return true;
    }
  }
  return QObject::eventFilter(Object, Event);
}
//---------------------------------------------------------------------------
bool TQueueController::GetEmpty()
{
  return (FQueueStatus == NULL) || (FQueueStatus->Count == 0);
}
//---------------------------------------------------------------------------
TQueueItemProxy * TQueueController::GetFocusedPrimaryItem()
{
  TQueueItemProxy * Result = NULL;
  QTreeWidgetItem * PrimaryItemOfFocused = FListView->currentItem();
  if (PrimaryItemOfFocused != NULL)
  {
    int Index = FListView->indexOfTopLevelItem(PrimaryItemOfFocused);
    while (!QueueViewItemToQueueItem(PrimaryItemOfFocused)->Info->Primary &&
           DebugAlwaysTrue(Index > 0))
    {
      Index--;
      PrimaryItemOfFocused = FListView->topLevelItem(Index);
    }
    Result = QueueViewItemToQueueItem(PrimaryItemOfFocused);
  }

  return Result;
}
//---------------------------------------------------------------------------
