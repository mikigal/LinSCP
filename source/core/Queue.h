//---------------------------------------------------------------------------
#ifndef QueueH
#define QueueH
//---------------------------------------------------------------------------
#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TSimpleThread
{
public:
  __fastcall TSimpleThread();
  virtual __fastcall ~TSimpleThread();

  virtual void __fastcall Start();
  void __fastcall WaitFor(unsigned int Milliseconds = INFINITE);
  virtual void __fastcall Terminate() = 0;
  void __fastcall Close();
  bool __fastcall IsFinished();

protected:
  HANDLE FThread;
  TThreadID FThreadId;
  bool FFinished;

  virtual void __fastcall Execute() = 0;
  virtual bool __fastcall Finished();

  static int __fastcall ThreadProc(void * Thread);
};
//---------------------------------------------------------------------------
class TSignalThread : public TSimpleThread
{
public:
  virtual void __fastcall Start();
  virtual void __fastcall Terminate();
  void __fastcall TriggerEvent();

protected:
  HANDLE FEvent;
  bool FTerminated;

  __fastcall TSignalThread(bool LowPriority, HANDLE Event = NULL);
  virtual __fastcall ~TSignalThread();

  virtual bool __fastcall WaitForEvent();
  int __fastcall WaitForEvent(unsigned int Timeout);
  virtual void __fastcall Execute();
  virtual void __fastcall ProcessEvent() = 0;
};
//---------------------------------------------------------------------------
class TTerminal;
class TQueueItem;
class TTerminalQueue;
class TQueueItemProxy;
class TTerminalQueueStatus;
class TQueueFileList;
class TTerminalItem;
//---------------------------------------------------------------------------
typedef TMethodPtr<void(TTerminalQueue * Queue)>
 TQueueListUpdate;
typedef TMethodPtr<void(TTerminalQueue * Queue, TQueueItem * Item)>
 TQueueItemUpdateEvent;
enum TQueueEvent : int { qeEmpty, qeEmptyButMonitored, qePendingUserAction };
typedef TMethodPtr<void(TTerminalQueue * Queue, TQueueEvent Event)>
 TQueueEventEvent;
//---------------------------------------------------------------------------
class TTerminalQueue : public TSignalThread
{
friend class TQueueItem;
friend class TQueueItemProxy;
friend class TTransferQueueItem;
friend class TParallelTransferQueueItem;

public:
  __fastcall TTerminalQueue(TTerminal * Terminal, TConfiguration * Configuration);
  virtual __fastcall ~TTerminalQueue();

  void __fastcall AddItem(TQueueItem * Item);
  TTerminalQueueStatus * __fastcall CreateStatus(TTerminalQueueStatus * Current);
  void __fastcall Idle();

  bool __property_get_IsEmpty() const { return ::rtl::Mutable(this)->GetIsEmpty(); } __declspec(property(get = __property_get_IsEmpty)) bool IsEmpty;
  int __property_get_TransfersLimit() const { return FTransfersLimit; } void __property_set_TransfersLimit(int Value) { SetTransfersLimit(Value); } __declspec(property(get = __property_get_TransfersLimit, put = __property_set_TransfersLimit)) int TransfersLimit;
  int __property_get_KeepDoneItemsFor() const { return FKeepDoneItemsFor; } void __property_set_KeepDoneItemsFor(int Value) { SetKeepDoneItemsFor(Value); } __declspec(property(get = __property_get_KeepDoneItemsFor, put = __property_set_KeepDoneItemsFor)) int KeepDoneItemsFor;
  int __property_get_ParallelDurationThreshold() const { return ::rtl::Mutable(this)->GetParallelDurationThreshold(); } __declspec(property(get = __property_get_ParallelDurationThreshold)) int ParallelDurationThreshold;
  bool __property_get_Enabled() const { return FEnabled; } void __property_set_Enabled(bool Value) { SetEnabled(Value); } __declspec(property(get = __property_get_Enabled, put = __property_set_Enabled)) bool Enabled;
  TQueryUserEvent __property_get_OnQueryUser() const { return FOnQueryUser; } void __property_set_OnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; } __declspec(property(get = __property_get_OnQueryUser, put = __property_set_OnQueryUser)) TQueryUserEvent OnQueryUser;
  TPromptUserEvent __property_get_OnPromptUser() const { return FOnPromptUser; } void __property_set_OnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; } __declspec(property(get = __property_get_OnPromptUser, put = __property_set_OnPromptUser)) TPromptUserEvent OnPromptUser;
  TExtendedExceptionEvent __property_get_OnShowExtendedException() const { return FOnShowExtendedException; } void __property_set_OnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; } __declspec(property(get = __property_get_OnShowExtendedException, put = __property_set_OnShowExtendedException)) TExtendedExceptionEvent OnShowExtendedException;
  TQueueListUpdate __property_get_OnListUpdate() const { return FOnListUpdate; } void __property_set_OnListUpdate(TQueueListUpdate Value) { FOnListUpdate = Value; } __declspec(property(get = __property_get_OnListUpdate, put = __property_set_OnListUpdate)) TQueueListUpdate OnListUpdate;
  TQueueItemUpdateEvent __property_get_OnQueueItemUpdate() const { return FOnQueueItemUpdate; } void __property_set_OnQueueItemUpdate(TQueueItemUpdateEvent Value) { FOnQueueItemUpdate = Value; } __declspec(property(get = __property_get_OnQueueItemUpdate, put = __property_set_OnQueueItemUpdate)) TQueueItemUpdateEvent OnQueueItemUpdate;
  TQueueEventEvent __property_get_OnEvent() const { return FOnEvent; } void __property_set_OnEvent(TQueueEventEvent Value) { FOnEvent = Value; } __declspec(property(get = __property_get_OnEvent, put = __property_set_OnEvent)) TQueueEventEvent OnEvent;

protected:
  friend class TTerminalItem;
  friend class TQueryUserAction;
  friend class TPromptUserAction;
  friend class TShowExtendedExceptionAction;

  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TQueueItemUpdateEvent FOnQueueItemUpdate;
  TQueueListUpdate FOnListUpdate;
  TQueueEventEvent FOnEvent;
  TTerminal * FTerminal;
  TConfiguration * FConfiguration;
  TSessionData * FSessionData;
  TList * FItems;
  TList * FDoneItems;
  int FItemsInProcess;
  TCriticalSection * FItemsSection;
  int FFreeTerminals;
  TList * FTerminals;
  TList * FForcedItems;
  int FTemporaryTerminals;
  int FOverallTerminals;
  int FTransfersLimit;
  int FKeepDoneItemsFor;
  bool FEnabled;
  TDateTime FIdleInterval;
  TDateTime FLastIdle;

  inline static TQueueItem * __fastcall GetItem(TList * List, int Index);
  inline TQueueItem * __fastcall GetItem(int Index);
  void __fastcall FreeItemsList(TList * List);
  void __fastcall UpdateStatusForList(
    TTerminalQueueStatus * Status, TList * List, TTerminalQueueStatus * Current);
  bool __fastcall ItemGetData(TQueueItem * Item, TQueueItemProxy * Proxy, TQueueFileList * FileList);
  bool __fastcall ItemProcessUserAction(TQueueItem * Item, void * Arg);
  bool __fastcall ItemMove(TQueueItem * Item, TQueueItem * BeforeItem);
  bool __fastcall ItemExecuteNow(TQueueItem * Item);
  bool __fastcall ItemDelete(TQueueItem * Item);
  bool __fastcall ItemPause(TQueueItem * Item, bool Pause);
  bool __fastcall ItemSetCPSLimit(TQueueItem * Item, unsigned int CPSLimit);
  bool __fastcall ItemGetCPSLimit(TQueueItem * Item, unsigned int & CPSLimit);

  void __fastcall RetryItem(TQueueItem * Item);
  void __fastcall DeleteItem(TQueueItem * Item, bool CanKeep);

  virtual bool __fastcall WaitForEvent();
  virtual void __fastcall ProcessEvent();
  void __fastcall TerminalFinished(TTerminalItem * TerminalItem);
  bool __fastcall TerminalFree(TTerminalItem * TerminalItem);
  int __fastcall GetParallelDurationThreshold();

  void __fastcall DoQueueItemUpdate(TQueueItem * Item);
  void __fastcall DoListUpdate();
  void __fastcall DoEvent(TQueueEvent Event);

  void __fastcall SetTransfersLimit(int value);
  void __fastcall SetKeepDoneItemsFor(int value);
  void __fastcall SetEnabled(bool value);
  bool __fastcall GetIsEmpty();

  bool __fastcall TryAddParallelOperation(TQueueItem * Item, bool Force);
  bool __fastcall ContinueParallelOperation();
};
//---------------------------------------------------------------------------
class TQueueItem
{
friend class TTerminalQueue;
friend class TTerminalItem;
friend class TParallelTransferQueueItem;

public:
  enum TStatus : int {
    qsPending, qsConnecting, qsProcessing, qsPrompt, qsQuery, qsError,
    qsPaused, qsDone };
  struct TInfo
  {
    TFileOperation Operation;
    TOperationSide Side;
    UnicodeString Source;
    UnicodeString Destination;
    UnicodeString ModifiedLocal;
    UnicodeString ModifiedRemote;
    bool SingleFile;
    bool Primary;
    void * GroupToken;
  };

  static bool __fastcall IsUserActionStatus(TStatus Status);

  TStatus __property_get_Status() const { return ::rtl::Mutable(this)->GetStatus(); } __declspec(property(get = __property_get_Status)) TStatus Status;
  HANDLE __property_get_CompleteEvent() const { return FCompleteEvent; } void __property_set_CompleteEvent(HANDLE Value) { FCompleteEvent = Value; } __declspec(property(get = __property_get_CompleteEvent, put = __property_set_CompleteEvent)) HANDLE CompleteEvent;

protected:
  TStatus FStatus;
  TCriticalSection * FSection;
  TTerminalItem * FTerminalItem;
  TFileOperationProgressType * FProgressData;
  TQueueItem::TInfo * FInfo;
  TTerminalQueue * FQueue;
  HANDLE FCompleteEvent;
  int FCPSLimit;
  TDateTime FDoneAt;

  __fastcall TQueueItem();
  virtual __fastcall ~TQueueItem();

  void __fastcall SetStatus(TStatus Status);
  TStatus __fastcall GetStatus();
  void __fastcall Execute();
  virtual void __fastcall DoExecute(TTerminal * Terminal) = 0;
  void __fastcall SetProgress(TFileOperationProgressType & ProgressData);
  void __fastcall GetData(TQueueItemProxy * Proxy);
  virtual bool __fastcall UpdateFileList(TQueueFileList * FileList);
  void __fastcall SetCPSLimit(unsigned int CPSLimit);
  unsigned int __fastcall GetCPSLimit();
  virtual unsigned int __fastcall DefaultCPSLimit();
  virtual UnicodeString __fastcall StartupDirectory() const;
  virtual void __fastcall ProgressUpdated();
  virtual TQueueItem * __fastcall CreateParallelOperation();
  virtual bool __fastcall Complete();
  bool IsExecutionCancelled();
};
//---------------------------------------------------------------------------
class TQueueItemProxy
{
friend class TQueueItem;
friend class TTerminalQueueStatus;
friend class TTerminalQueue;

public:
  bool __fastcall Update();
  bool __fastcall UpdateFileList(TQueueFileList * FileList);
  bool __fastcall ProcessUserAction();
  bool __fastcall Move(bool Sooner);
  bool __fastcall Move(TQueueItemProxy * BeforeItem);
  bool __fastcall ExecuteNow();
  bool __fastcall Delete();
  bool __fastcall Pause();
  bool __fastcall Resume();
  bool __fastcall SetCPSLimit(unsigned int CPSLimit);
  bool __fastcall GetCPSLimit(unsigned int & CPSLimit);

  TFileOperationProgressType * __property_get_ProgressData() const { return ::rtl::Mutable(this)->GetProgressData(); } __declspec(property(get = __property_get_ProgressData)) TFileOperationProgressType * ProgressData;
  __int64 __property_get_TotalTransferred() const { return ::rtl::Mutable(this)->GetTotalTransferred(); } __declspec(property(get = __property_get_TotalTransferred)) __int64 TotalTransferred;
  TQueueItem::TInfo * __property_get_Info() const { return FInfo; } __declspec(property(get = __property_get_Info)) TQueueItem::TInfo * Info;
  TQueueItem::TStatus __property_get_Status() const { return FStatus; } __declspec(property(get = __property_get_Status)) TQueueItem::TStatus Status;
  bool __property_get_ProcessingUserAction() const { return FProcessingUserAction; } __declspec(property(get = __property_get_ProcessingUserAction)) bool ProcessingUserAction;
  int __property_get_Index() const { return ::rtl::Mutable(this)->GetIndex(); } __declspec(property(get = __property_get_Index)) int Index;
  // Clang warns on property backed by private field which is never used
  void * UserData;

private:
  TFileOperationProgressType * FProgressData;
  TQueueItem::TStatus FStatus;
  TTerminalQueue * FQueue;
  TQueueItem * FQueueItem;
  TTerminalQueueStatus * FQueueStatus;
  TQueueItem::TInfo * FInfo;
  bool FProcessingUserAction;

  __fastcall TQueueItemProxy(TTerminalQueue * Queue, TQueueItem * QueueItem);
  virtual __fastcall ~TQueueItemProxy();
  int __fastcall GetIndex();
  TFileOperationProgressType * __fastcall GetProgressData();
  __int64 __fastcall GetTotalTransferred();
};
//---------------------------------------------------------------------------
class TTerminalQueueStatus
{
friend class TTerminalQueue;
friend class TQueueItemProxy;

public:
  virtual __fastcall ~TTerminalQueueStatus();

  TQueueItemProxy * __fastcall FindByQueueItem(TQueueItem * QueueItem);

  int __property_get_Count() const { return ::rtl::Mutable(this)->GetCount(); } __declspec(property(get = __property_get_Count)) int Count;
  int __property_get_DoneCount() const { return FDoneCount; } __declspec(property(get = __property_get_DoneCount)) int DoneCount;
  int __property_get_ActiveCount() const { return ::rtl::Mutable(this)->GetActiveCount(); } __declspec(property(get = __property_get_ActiveCount)) int ActiveCount;
  int __property_get_DoneAndActiveCount() const { return ::rtl::Mutable(this)->GetDoneAndActiveCount(); } __declspec(property(get = __property_get_DoneAndActiveCount)) int DoneAndActiveCount;
  int __property_get_ActivePrimaryCount() const { return ::rtl::Mutable(this)->GetActivePrimaryCount(); } __declspec(property(get = __property_get_ActivePrimaryCount)) int ActivePrimaryCount;
  int __property_get_ActiveAndPendingPrimaryCount() const { return ::rtl::Mutable(this)->GetActiveAndPendingPrimaryCount(); } __declspec(property(get = __property_get_ActiveAndPendingPrimaryCount)) int ActiveAndPendingPrimaryCount;
  TQueueItemProxy * __property_get_Items(int Index) const { return ::rtl::Mutable(this)->GetItem(Index); } __declspec(property(get = __property_get_Items)) TQueueItemProxy * Items[];

  bool __fastcall IsOnlyOneActiveAndNoPending();

  bool __fastcall UpdateFileList(TQueueItemProxy * ItemProxy, TQueueFileList * FileList);

protected:
  __fastcall TTerminalQueueStatus();

  void __fastcall Add(TQueueItemProxy * ItemProxy);
  void __fastcall Delete(TQueueItemProxy * ItemProxy);
  void __fastcall ResetStats();
  void __fastcall NeedStats();

private:
  TList * FList;
  int FDoneCount;
  int FActiveCount;
  int FActivePrimaryCount;
  int FActiveAndPendingPrimaryCount;

  int __fastcall GetCount();
  int __fastcall GetActiveCount();
  int __fastcall GetDoneAndActiveCount();
  int __fastcall GetActivePrimaryCount();
  int __fastcall GetActiveAndPendingPrimaryCount();
  void __fastcall SetDoneCount(int Value);
  TQueueItemProxy * __fastcall GetItem(int Index);
};
//---------------------------------------------------------------------------
class TBootstrapQueueItem : public TQueueItem
{
public:
  __fastcall TBootstrapQueueItem();

protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);
  virtual bool __fastcall Complete();
};
//---------------------------------------------------------------------------
class TLocatedQueueItem : public TQueueItem
{
protected:
  __fastcall TLocatedQueueItem(TTerminal * Terminal);
  __fastcall TLocatedQueueItem(const TLocatedQueueItem & Source);

  virtual void __fastcall DoExecute(TTerminal * Terminal);
  virtual UnicodeString __fastcall StartupDirectory() const;

private:
  UnicodeString FCurrentDir;
};
//---------------------------------------------------------------------------
class TTransferQueueItem : public TLocatedQueueItem
{
public:
  __fastcall TTransferQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, TOperationSide Side,
    bool SingleFile, bool Parallel);
  virtual __fastcall ~TTransferQueueItem();

protected:
  TStrings * FFilesToCopy;
  UnicodeString FTargetDir;
  TCopyParamType * FCopyParam;
  int FParams;
  bool FParallel;
  DWORD FLastParallelOperationAdded;
  std::unique_ptr<TParallelOperation> FParallelOperation;

  virtual unsigned int __fastcall DefaultCPSLimit();
  virtual void __fastcall DoExecute(TTerminal * Terminal);
  virtual void __fastcall DoTransferExecute(TTerminal * Terminal, TParallelOperation * ParallelOperation) = 0;
  virtual void __fastcall ProgressUpdated();
  virtual TQueueItem * __fastcall CreateParallelOperation();
  virtual bool __fastcall UpdateFileList(TQueueFileList * FileList);
};
//---------------------------------------------------------------------------
class TUploadQueueItem : public TTransferQueueItem
{
public:
  __fastcall TUploadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, bool Parallel);

protected:
  virtual void __fastcall DoTransferExecute(TTerminal * Terminal, TParallelOperation * ParallelOperation);
};
//---------------------------------------------------------------------------
class TDownloadQueueItem : public TTransferQueueItem
{
public:
  __fastcall TDownloadQueueItem(TTerminal * Terminal,
    TStrings * FilesToCopy, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, bool Parallel);

protected:
  virtual void __fastcall DoTransferExecute(TTerminal * Terminal, TParallelOperation * ParallelOperation);
};
//---------------------------------------------------------------------------
class TRemoteDeleteQueueItem : public TLocatedQueueItem
{
public:
  TRemoteDeleteQueueItem(TTerminal * Terminal, TStrings * FilesToDelete, int Params);

protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);

private:
  std::unique_ptr<TStrings> FFilesToDelete;
  int FParams;
};
//---------------------------------------------------------------------------
class TLocalDeleteQueueItem : public TQueueItem
{
public:
  TLocalDeleteQueueItem(TStrings * FilesToDelete, int Params);

protected:
  virtual void __fastcall DoExecute(TTerminal * Terminal);

private:
  std::unique_ptr<TStrings> FFilesToDelete;
  int FParams;
};
//---------------------------------------------------------------------------
class TUserAction;
class TInformationUserAction;
enum TTerminalReopenResult : int { trrPending, trrSucceeded, trrFailed, trrNeedsInteraction };
class TTerminalThread : public TSignalThread
{
public:
  __fastcall TTerminalThread(TTerminal * Terminal);
  virtual __fastcall ~TTerminalThread();

  void __fastcall TerminalOpen();
  void __fastcall TerminalReopen();
  void StartTerminalReopenNonInteractive();
  TTerminalReopenResult IsTerminalReopenComplete();
  void ContinueTerminalReopenInteractive();
  bool Abandoned();

  void __fastcall Cancel();
  bool __fastcall Release();
  void __fastcall Idle();

  TNotifyEvent __property_get_OnIdle() const { return FOnIdle; } void __property_set_OnIdle(TNotifyEvent Value) { FOnIdle = Value; } __declspec(property(get = __property_get_OnIdle, put = __property_set_OnIdle)) TNotifyEvent OnIdle;
  bool __property_get_Cancelling() const { return FCancel; } __declspec(property(get = __property_get_Cancelling)) bool Cancelling;
  bool __property_get_AllowAbandon() const { return FAllowAbandon; } void __property_set_AllowAbandon(bool Value) { FAllowAbandon = Value; } __declspec(property(get = __property_get_AllowAbandon, put = __property_set_AllowAbandon)) bool AllowAbandon;


protected:
  virtual void __fastcall ProcessEvent();
  virtual bool __fastcall Finished();

private:
  TTerminal * FTerminal;

  TInformationEvent FOnInformation;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TExtendedExceptionEvent FOnShowExtendedException;
  TDisplayBannerEvent FOnDisplayBanner;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TNotifyEvent FOnInitializeLog;

  TNotifyEvent FOnIdle;

  TNotifyEvent FAction;
  HANDLE FActionEvent;
  TUserAction * FUserAction;

  Exception * FException;
  Exception * FIdleException;
  bool FCancel;
  TDateTime FCancelAfter;
  bool FAbandoned;
  bool FCancelled;
  bool FPendingIdle;
  bool FAllowAbandon;
  bool FNonInteractive;
  bool FNeedsInteraction;
  typedef std::list<TInformationUserAction *> TInformationList;
  TInformationList FNonInteractiveInformation;

  DWORD FMainThread;
  TCriticalSection * FSection;

  void __fastcall WaitForUserAction(TUserAction * UserAction);
  void StartAction(TNotifyEvent Action);
  void RunAction(TNotifyEvent Action);

  static void __fastcall SaveException(Exception & E, Exception *& Exception);
  static void __fastcall Rethrow(Exception *& Exception);
  void __fastcall FatalAbort();
  void __fastcall CheckCancel();

  void __fastcall TerminalOpenEvent(TObject * Sender);
  void __fastcall TerminalReopenEvent(TObject * Sender);

  void __fastcall TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, int Phase, const UnicodeString & Additional);
  void __fastcall TerminalQueryUser(TObject * Sender,
    const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
    const TQueryParams * Params, unsigned int & Answer, TQueryType Type, void * Arg);
  void __fastcall TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions,
    TStrings * Prompts, TStrings * Results, bool & Result, void * Arg);
  void __fastcall TerminalShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void __fastcall TerminalDisplayBanner(TTerminal * Terminal,
    UnicodeString SessionName, const UnicodeString & Banner,
    bool & NeverShowAgain, int Options, unsigned int & Params);
  void __fastcall TerminalChangeDirectory(TObject * Sender);
  void __fastcall TerminalReadDirectory(TObject * Sender, Boolean ReloadOnly);
  void __fastcall TerminalStartReadDirectory(TObject * Sender);
  void __fastcall TerminalReadDirectoryProgress(TObject * Sender, int Progress, int ResolvedLinks, bool & Cancel);
  void __fastcall TerminalInitializeLog(TObject * Sender);
  void DiscardException();
  bool WaitForActionEvent(unsigned int Wait);
  void CompleteAction();
  void NeedsInteraction();
};
//---------------------------------------------------------------------------
enum TQueueFileState : int { qfsQueued = 0, qfsProcessed = 1 };
//---------------------------------------------------------------------------
class TQueueFileList
{
friend class TParallelOperation;
public:
  TQueueFileList();
  void Clear();
  void Add(const UnicodeString & FileName, int State);
  UnicodeString GetFileName(int Index) const;
  int GetState(int Index) const;
  void SetState(int Index, int State);
  int GetCount() const;
private:
  std::unique_ptr<TStrings> FList;
  TParallelOperation * FLastParallelOperation;
  int FLastParallelOperationVersion;
};
//---------------------------------------------------------------------------
#endif
