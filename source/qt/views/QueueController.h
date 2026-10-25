// The list of the transfer queue (upstream's TQueueController,
// windows/QueueController.cpp, over a QTreeWidget in place of a VCL list
// view): a line per queue item, with a details line of the file being
// transferred, and the operations on the items.
//---------------------------------------------------------------------------
#ifndef QueueControllerH
#define QueueControllerH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QObject>

#include <functional>
//---------------------------------------------------------------------------
enum TQueueOperation { qoNone, qoGoTo, qoPreferences, qoItemUserAction,
  qoItemQuery, qoItemError, qoItemPrompt, qoItemDelete, qoItemExecute,
  qoItemUp, qoItemDown, qoItemPause, qoItemResume, qoItemSpeed, qoPauseAll, qoResumeAll,
  qoOnceEmpty, qoDeleteAllDone, qoDeleteAll };
class QTreeWidget;
class QTreeWidgetItem;
class TQueueItemProxy;
class TTerminalQueueStatus;
//---------------------------------------------------------------------------
class TQueueController : public QObject
{
public:
  explicit TQueueController(QTreeWidget * ListView);
  virtual ~TQueueController();

  TQueueOperation DefaultOperation();
  bool AllowOperation(TQueueOperation Operation, unsigned long * Param = NULL);
  void ExecuteOperation(TQueueOperation Operation, unsigned long Param = 0);

  void UpdateQueueStatus(TTerminalQueueStatus * QueueStatus);
  void RefreshQueueItem(TQueueItemProxy * QueueItem);
  static bool QueueItemNeedsFrequentRefresh(TQueueItemProxy * QueueItem);

  TQueueItemProxy * GetFocusedPrimaryItem();
  bool GetEmpty();

  std::function<void()> OnChange;

protected:
  virtual bool eventFilter(QObject * Object, QEvent * Event) override;

private:
  QTreeWidget * FListView;
  TTerminalQueueStatus * FQueueStatus;

  static TQueueItemProxy * QueueViewItemToQueueItem(QTreeWidgetItem * Item);
  void QueueViewDblClick();
  void DoChange();

  static void FillQueueViewItem(QTreeWidgetItem * Item,
    TQueueItemProxy * QueueItem, bool Detail, bool OnlyLine);
  QTreeWidgetItem * InsertItemFor(TQueueItemProxy * QueueItem, int Index);
  bool UseDetailsLine(int ItemIndex, TQueueItemProxy * QueueItem);
  static bool SimpleOperation(TQueueItemProxy * QueueItem);
};
//---------------------------------------------------------------------------
#endif
