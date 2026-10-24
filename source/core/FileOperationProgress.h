//---------------------------------------------------------------------------
#ifndef FileOperationProgressH
#define FileOperationProgressH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
#include "Exceptions.h"
#include <vector>
//---------------------------------------------------------------------------
class TFileOperationProgressType;
enum TFileOperation : int { foNone, foCopy, foMove, foDelete, foSetProperties,
  foRename, foCustomCommand, foCalculateSize, foRemoteMove, foRemoteCopy,
  foGetProperties, foCalculateChecksum, foLock, foUnlock };
// csCancelTransfer and csRemoteAbort are used with SCP only
enum TCancelStatus : int { csContinue = 0, csCancelFile, csCancel, csCancelTransfer, csRemoteAbort };
enum TBatchOverwrite : int { boNo, boAll, boNone, boOlder, boAlternateResume, boAppend, boResume };
typedef TMethodPtr<void(TFileOperationProgressType & ProgressData)>
 TFileOperationProgressEvent;
typedef TMethodPtr<void(TFileOperation Operation, TOperationSide Side, bool Temp,
   const UnicodeString & FileName, bool Success, bool NotCancelled, TOnceDoneOperation & OnceDoneOperation)>
 TFileOperationFinished;
//---------------------------------------------------------------------------
class TFileOperationStatistics
{
public:
  TFileOperationStatistics();

  int FilesUploaded;
  int FilesDownloaded;
  int FilesDeletedLocal;
  int FilesDeletedRemote;
  __int64 TotalUploaded;
  __int64 TotalDownloaded;
};
//---------------------------------------------------------------------------
class TFileOperationProgressType
{
public:
  class TPersistence
  {
  friend class TFileOperationProgressType;
  public:
    TPersistence();
    TFileOperationStatistics * __property_get_Statistics() const { return FStatistics; } void __property_set_Statistics(TFileOperationStatistics * Value) { FStatistics = Value; } __declspec(property(get = __property_get_Statistics, put = __property_set_Statistics)) TFileOperationStatistics * Statistics;

  private:
    void Clear(bool Batch, bool Speed);

    TDateTime StartTime;
    TBatchOverwrite BatchOverwrite;
    bool SkipToAll;
    unsigned int CPSLimit;
    bool CounterSet;
    std::vector<unsigned int> Ticks;
    std::vector<__int64> TotalTransferredThen;
    TOperationSide Side;
    __int64 TotalTransferred;
    TFileOperationStatistics * FStatistics;
  };

private:
  TFileOperation FOperation;
  UnicodeString FFileName;
  UnicodeString FFullFileName;
  UnicodeString FDirectory;
  bool FAsciiTransfer;
  bool FTransferringFile;
  bool FTemp;
  __int64 FLocalSize;
  __int64 FLocallyUsed;
  __int64 FTransferSize;
  __int64 FTransferredSize;
  __int64 FSkippedSize;
  bool FInProgress;
  bool FDone;
  bool FFileInProgress;
  TCancelStatus FCancel;
  int FCount;
  __int64 FTotalTransferBase;
  __int64 FTotalSkipped;
  __int64 FTotalSize;
  bool FTotalSizeSet;
  bool FSuspended;
  bool FRestored;
  TFileOperationProgressType * FParent;

  // when it was last time suspended (to calculate suspend time in Resume())
  unsigned int FSuspendTime;
  // when current file was started being transferred
  TDateTime FFileStartTime;
  int FFilesFinished;
  int FFilesFinishedSuccessfully;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinished FOnFinished;
  bool FReset;
  unsigned int FLastSecond;
  unsigned int FRemainingCPS;
  TOnceDoneOperation FInitialOnceDoneOperation;
  TPersistence FPersistence;
  TCriticalSection * FSection;
  TCriticalSection * FUserSelectionsSection;

  __int64 __fastcall GetTotalTransferred();
  __int64 __fastcall GetOperationTransferred() const;
  __int64 __fastcall GetTotalSize();
  unsigned int __fastcall GetCPSLimit();
  TBatchOverwrite __fastcall GetBatchOverwrite();
  bool __fastcall GetSkipToAll();
  TDateTime __fastcall GetStartTime() const { return FPersistence.StartTime; }
  TOperationSide __fastcall GetSide() const { return FPersistence.Side; }

protected:
  void __fastcall ClearTransfer();
  inline void __fastcall DoProgress();
  int __fastcall OperationProgress() const;
  void __fastcall AddTransferredToTotals(__int64 ASize);
  void __fastcall AddSkipped(__int64 ASize);
  void __fastcall AddTotalSize(__int64 ASize);
  void __fastcall RollbackTransferFromTotals(__int64 ATransferredSize, __int64 ASkippedSize);
  unsigned int __fastcall GetCPS();
  void __fastcall Init();
  static bool __fastcall PassCancelToParent(TCancelStatus ACancel);
  void __fastcall DoClear(bool Batch, bool Speed);

public:
  // common data
  TFileOperation __property_get_Operation() const { return FOperation; } __declspec(property(get = __property_get_Operation)) TFileOperation Operation;
  // on what side if operation being processed (local/remote), source of copy
  TOperationSide __property_get_Side() const { return ::rtl::Mutable(this)->GetSide(); } __declspec(property(get = __property_get_Side)) TOperationSide Side;
  int __property_get_Count() const { return FCount; } __declspec(property(get = __property_get_Count)) int Count;
  UnicodeString __property_get_FileName() const { return FFileName; } __declspec(property(get = __property_get_FileName)) UnicodeString FileName;
  UnicodeString __property_get_FullFileName() const { return FFullFileName; } __declspec(property(get = __property_get_FullFileName)) UnicodeString FullFileName;
  UnicodeString __property_get_Directory() const { return FDirectory; } __declspec(property(get = __property_get_Directory)) UnicodeString Directory;
  bool __property_get_AsciiTransfer() const { return FAsciiTransfer; } __declspec(property(get = __property_get_AsciiTransfer)) bool AsciiTransfer;
  // Can be true with SCP protocol only
  bool __property_get_TransferringFile() const { return FTransferringFile; } __declspec(property(get = __property_get_TransferringFile)) bool TransferringFile;
  bool __property_get_Temp() const { return FTemp; } __declspec(property(get = __property_get_Temp)) bool Temp;

  // file size to read/write
  __int64 __property_get_LocalSize() const { return FLocalSize; } __declspec(property(get = __property_get_LocalSize)) __int64 LocalSize;
  __int64 __property_get_LocallyUsed() const { return FLocallyUsed; } __declspec(property(get = __property_get_LocallyUsed)) __int64 LocallyUsed;
  __int64 __property_get_TransferSize() const { return FTransferSize; } __declspec(property(get = __property_get_TransferSize)) __int64 TransferSize;
  __int64 __property_get_TransferredSize() const { return FTransferredSize; } __declspec(property(get = __property_get_TransferredSize)) __int64 TransferredSize;
  __int64 __property_get_SkippedSize() const { return FSkippedSize; } __declspec(property(get = __property_get_SkippedSize)) __int64 SkippedSize;
  bool __property_get_InProgress() const { return FInProgress; } __declspec(property(get = __property_get_InProgress)) bool InProgress;
  bool __property_get_Done() const { return FDone; } __declspec(property(get = __property_get_Done)) bool Done;
  bool __property_get_FileInProgress() const { return FFileInProgress; } __declspec(property(get = __property_get_FileInProgress)) bool FileInProgress;
  TCancelStatus __property_get_Cancel() const { return ::rtl::Mutable(this)->GetCancel(); } __declspec(property(get = __property_get_Cancel)) TCancelStatus Cancel;
  // when operation started
  TDateTime __property_get_StartTime() const { return ::rtl::Mutable(this)->GetStartTime(); } __declspec(property(get = __property_get_StartTime)) TDateTime StartTime;
  // bytes transferred
  __int64 __property_get_TotalTransferred() const { return ::rtl::Mutable(this)->GetTotalTransferred(); } __declspec(property(get = __property_get_TotalTransferred)) __int64 TotalTransferred;
  __int64 __property_get_OperationTransferred() const { return ::rtl::Mutable(this)->GetOperationTransferred(); } __declspec(property(get = __property_get_OperationTransferred)) __int64 OperationTransferred;
  __int64 __property_get_TotalSize() const { return ::rtl::Mutable(this)->GetTotalSize(); } __declspec(property(get = __property_get_TotalSize)) __int64 TotalSize;
  int __property_get_FilesFinishedSuccessfully() const { return FFilesFinishedSuccessfully; } __declspec(property(get = __property_get_FilesFinishedSuccessfully)) int FilesFinishedSuccessfully;
  TOnceDoneOperation __property_get_InitialOnceDoneOperation() const { return FInitialOnceDoneOperation; } __declspec(property(get = __property_get_InitialOnceDoneOperation)) TOnceDoneOperation InitialOnceDoneOperation;

  TBatchOverwrite __property_get_BatchOverwrite() const { return ::rtl::Mutable(this)->GetBatchOverwrite(); } __declspec(property(get = __property_get_BatchOverwrite)) TBatchOverwrite BatchOverwrite;
  bool __property_get_SkipToAll() const { return ::rtl::Mutable(this)->GetSkipToAll(); } __declspec(property(get = __property_get_SkipToAll)) bool SkipToAll;
  unsigned int __property_get_CPSLimit() const { return ::rtl::Mutable(this)->GetCPSLimit(); } __declspec(property(get = __property_get_CPSLimit)) unsigned int CPSLimit;

  bool __property_get_TotalSizeSet() const { return FTotalSizeSet; } __declspec(property(get = __property_get_TotalSizeSet)) bool TotalSizeSet;

  bool __property_get_Suspended() const { return FSuspended; } __declspec(property(get = __property_get_Suspended)) bool Suspended;

  __fastcall TFileOperationProgressType();
  __fastcall TFileOperationProgressType(
    TFileOperationProgressEvent AOnProgress, TFileOperationFinished AOnFinished,
    TFileOperationProgressType * Parent = NULL);
  __fastcall ~TFileOperationProgressType();
  TFileOperationProgressType & operator =(const TFileOperationProgressType &) = default;
  void __fastcall Assign(const TFileOperationProgressType & Other);
  void __fastcall AssignButKeepSuspendState(const TFileOperationProgressType & Other);
  void __fastcall AddLocallyUsed(__int64 ASize);
  void __fastcall AddTransferred(__int64 ASize, bool AddToTotals = true);
  void __fastcall AddResumed(__int64 ASize);
  void __fastcall AddSkippedFileSize(__int64 ASize);
  void __fastcall Clear();
  unsigned int __fastcall CPS();
  void __fastcall Finish(UnicodeString FileName, bool Success,
    TOnceDoneOperation & OnceDoneOperation);
  void __fastcall Succeeded(int Count = 1);
  void __fastcall Progress();
  unsigned int __fastcall LocalBlockSize();
  bool __fastcall IsLocallyDone();
  bool IsTransferDone();
  bool IsTransferDoneChecked();
  void __fastcall SetFile(UnicodeString AFileName, bool AFileInProgress = true);
  void __fastcall SetFileInProgress();
  unsigned int __fastcall TransferBlockSize();
  unsigned int __fastcall AdjustToCPSLimit(unsigned int Size);
  void __fastcall ThrottleToCPSLimit(unsigned int Size);
  static unsigned int __fastcall StaticBlockSize();
  void __fastcall Reset();
  void __fastcall Resume();
  void __fastcall SetLocalSize(__int64 ASize);
  void __fastcall SetAsciiTransfer(bool AAsciiTransfer);
  void __fastcall SetTransferSize(__int64 ASize);
  void __fastcall ChangeTransferSize(__int64 ASize);
  void __fastcall RollbackTransfer();
  void __fastcall SetTotalSize(__int64 ASize);
  void __fastcall Start(TFileOperation AOperation, TOperationSide ASide, int ACount);
  void __fastcall Start(TFileOperation AOperation,
    TOperationSide ASide, int ACount, bool ATemp, const UnicodeString ADirectory,
    unsigned int ACPSLimit, TOnceDoneOperation InitialOnceDoneOperation);
  void __fastcall Stop();
  void __fastcall SetDone();
  void __fastcall Suspend();
  void __fastcall LockUserSelections();
  void __fastcall UnlockUserSelections();
  // whole operation
  TDateTime __fastcall TimeElapsed();
  // only current file
  TDateTime __fastcall TimeExpected();
  TDateTime __fastcall TotalTimeLeft();
  int __fastcall TransferProgress();
  int __fastcall OverallProgress() const;
  int __fastcall TotalTransferProgress() const;
  void __fastcall SetSpeedCounters();
  void __fastcall SetTransferringFile(bool ATransferringFile);
  TCancelStatus __fastcall GetCancel();
  void __fastcall SetCancel(TCancelStatus ACancel);
  void __fastcall SetCancelAtLeast(TCancelStatus ACancel);
  bool __fastcall ClearCancelFile();
  void __fastcall SetCPSLimit(unsigned int ACPSLimit);
  void __fastcall SetBatchOverwrite(TBatchOverwrite ABatchOverwrite);
  void __fastcall SetSkipToAll();
  UnicodeString __fastcall GetLogStr(bool Done);
  void __fastcall Store(TPersistence & Persistence);
  void __fastcall Restore(TPersistence & Persistence);
  bool IsIndeterminate() const;
  bool IsTransfer() const;

  static bool IsIndeterminateOperation(TFileOperation Operation);
  static bool IsTransferOperation(TFileOperation Operation);
};
//---------------------------------------------------------------------------
class TSuspendFileOperationProgress
{
public:
  __fastcall TSuspendFileOperationProgress(TFileOperationProgressType * OperationProgress)
  {
    FOperationProgress = OperationProgress;
    if (FOperationProgress != NULL)
    {
      FOperationProgress->Suspend();
    }
  }

  __fastcall ~TSuspendFileOperationProgress()
  {
    if (FOperationProgress != NULL)
    {
      FOperationProgress->Resume();
    }
  }

private:
  TFileOperationProgressType * FOperationProgress;
};
//---------------------------------------------------------------------------
#endif
