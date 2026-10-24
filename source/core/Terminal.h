//---------------------------------------------------------------------------
#ifndef TerminalH
#define TerminalH

#include <Classes.hpp>

#include "SessionInfo.h"
#include "Interface.h"
#include "FileOperationProgress.h"
#include "FileMasks.h"
#include "RemoteFiles.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
class TCopyParamType;
class TFileOperationProgressType;
class TCustomFileSystem;
class TTunnelThread;
class TSecureShell;
struct TCalculateSizeParams;
struct TOverwriteFileParams;
struct TSynchronizeData;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
struct TCalculateSizeStats;
struct TFileSystemInfo;
struct TSpaceAvailable;
struct TFilesFindParams;
class TTunnelUI;
class TCallbackGuard;
class TParallelOperation;
class TCollectedFileList;
struct TLocalFileHandle;
struct TNeonCertificateData;
class TQueueItem;
class TTerminalUI;
struct TSynchronizeFileData;
typedef std::vector<__int64> TCalculatedSizes;
//---------------------------------------------------------------------------
typedef TMethodPtr<void(TObject * Sender, const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
   const TQueryParams * Params, unsigned int & Answer, TQueryType QueryType, void * Arg)>
 TQueryUserEvent;
typedef TMethodPtr<void(TTerminal * Terminal, TPromptKind Kind, UnicodeString Name, UnicodeString Instructions,
   TStrings * Prompts, TStrings * Results, bool & Result, void * Arg)>
 TPromptUserEvent;
typedef TMethodPtr<void(TTerminal * Terminal, UnicodeString SessionName, const UnicodeString & Banner,
   bool & NeverShowAgain, int Options, unsigned int & Params)>
 TDisplayBannerEvent;
typedef TMethodPtr<void(TTerminal * Terminal, Exception * E, void * Arg)>
 TExtendedExceptionEvent;
typedef TMethodPtr<void(System::TObject * Sender, Boolean ReloadOnly)> TReadDirectoryEvent;
typedef TMethodPtr<void(
  System::TObject* Sender, int Progress, int ResolvedLinks, bool & Cancel)> TReadDirectoryProgressEvent;
typedef TMethodPtr<void(const UnicodeString FileName, const TRemoteFile * File, void * Param)>
 TProcessFileEvent;
typedef TMethodPtr<void(const UnicodeString FileName, const TRemoteFile * File, void * Param, int Index)>
 TProcessFileEventEx;
typedef TMethodPtr<int(void * Param1, void * Param2)>
 TFileOperationEvent;
typedef TMethodPtr<void(const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
   bool & Continue, bool Collect, const TSynchronizeOptions * Options)>
 TSynchronizeDirectory;
typedef TMethodPtr<void(
  const TSynchronizeChecklist::TItemList & Items)> TUpdatedSynchronizationChecklistItems;
typedef TMethodPtr<void(
  const UnicodeString FileName, bool Alternative, int & Deleted)> TDeleteLocalFileEvent;
typedef TMethodPtr<int(TTerminal * Terminal, const UnicodeString Directory, bool SubDirs)>
 TDirectoryModifiedEvent;
typedef TMethodPtr<void(TTerminal * Terminal, const UnicodeString & Str, int Phase, const UnicodeString & Additional)>
 TInformationEvent;
typedef TMethodPtr<void(TTerminal * Terminal, const UnicodeString & Command, bool & Handled)>
 TCustomCommandEvent;
//---------------------------------------------------------------------------
const unsigned int folNone = 0x00;
const unsigned int folAllowSkip = 0x01;
const unsigned int folRetryOnFatal = 0x02;

/* TODO : Better user interface (query to user) */
#define FILE_OPERATION_LOOP_BEGIN \
  { \
    bool DoRepeat; \
    do \
    { \
      DoRepeat = false; \
      try \

#define FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, HELPKEYWORD) \
      catch (Exception & E) \
      { \
        FILE_OPERATION_LOOP_TERMINAL->FileOperationLoopEnd(E, OperationProgress, MESSAGE, FLAGS, L"", HELPKEYWORD); \
        DoRepeat = true; \
      } \
    } while (DoRepeat); \
  }

#define FILE_OPERATION_LOOP_END_EX(MESSAGE, FLAGS) \
  FILE_OPERATION_LOOP_END_CUSTOM(MESSAGE, FLAGS, L"")
#define FILE_OPERATION_LOOP_END(MESSAGE) \
  FILE_OPERATION_LOOP_END_EX(MESSAGE, folAllowSkip)
//---------------------------------------------------------------------------
enum TCurrentFSProtocol : int { cfsUnknown, cfsSCP, cfsSFTP, cfsFTP, cfsWebDAV, cfsS3 };
//---------------------------------------------------------------------------
const int cpDelete = 0x01;
const int cpTemporary = 0x04;
const int cpNoConfirmation = 0x08;
const int cpAppend = 0x20;
const int cpResume = 0x40;
const int cpNoRecurse = 0x80;
//---------------------------------------------------------------------------
const int ccApplyToDirectories = 0x01;
const int ccRecursive = 0x02;
const int ccUser = 0x100;
//---------------------------------------------------------------------------
const int csIgnoreErrors = 0x01;
const int csStopOnFirstFile = 0x02;
const int csDisallowTemporaryTransferFiles = 0x04;
//---------------------------------------------------------------------------
const int ropNoReadDirectory = 0x02;
//---------------------------------------------------------------------------
const int boDisableNeverShowAgain = 0x01;
const int bpMonospacedFont = 0x01;
//---------------------------------------------------------------------------
const int tfNone = 0x00;
const int tfFirstLevel = 0x01;
const int tfNewDirectory = 0x02;
const int tfAutoResume = 0x04;
const int tfPreCreateDir = 0x08;
const int tfUseFileTransferAny = 0x10;
//---------------------------------------------------------------------------
class TTerminal : public TObject
{
public:
  // TScript::SynchronizeProc relies on the order
  enum TSynchronizeMode : int { smRemote, smLocal, smBoth };
  static const int spDelete = 0x01; // cannot be combined with spTimestamp
  static const int spNoConfirmation = 0x02; // has no effect for spTimestamp
  static const int spExistingOnly = 0x04; // is implicit for spTimestamp
  static const int spNoRecurse = 0x08;
  static const int spUseCache = 0x10; // cannot be combined with spTimestamp
  static const int spDelayProgress = 0x20; // cannot be combined with spTimestamp
  static const int spPreviewChanges = 0x40; // not used by core
  static const int spSubDirs = 0x80; // cannot be combined with spTimestamp
  static const int spTimestamp = 0x100;
  static const int spNotByTime = 0x200; // cannot be combined with spTimestamp and smBoth
  static const int spBySize = 0x400; // cannot be combined with smBoth, has opposite meaning for spTimestamp
  static const int spSelectedOnly = 0x800; // not used by core
  static const int spMirror = 0x1000;
  static const int spCaseSensitive = 0x2000;
  static const int spByChecksum = 0x4000; // cannot be combined with spTimestamp and smBoth
  static const int spLocalLocal = 0x8000; // internal only
  static const int spDefault = TTerminal::spNoConfirmation | TTerminal::spPreviewChanges;

// for ReactOnCommand()
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;
friend class TS3FileSystem;
friend class TTunnelUI;
friend class TCallbackGuard;
friend class TSecondaryTerminal;
friend class TRetryOperationLoop;
friend class TParallelOperation;
friend class TTerminalUI;

private:
  TSessionData * FSessionData;
  UnicodeString FPasswordEncryptionKey;
  TSessionLog * FLog;
  TActionLog * FActionLog;
  bool FActionLogOwned;
  TConfiguration * FConfiguration;
  UnicodeString FCurrentDirectory;
  Integer FExceptionOnFail;
  TRemoteDirectory * FFiles;
  int FInTransaction;
  bool FSuspendTransaction;
  TNotifyEvent FOnChangeDirectory;
  TReadDirectoryEvent FOnReadDirectory;
  TNotifyEvent FOnStartReadDirectory;
  TReadDirectoryProgressEvent FOnReadDirectoryProgress;
  TDeleteLocalFileEvent FOnDeleteLocalFile;
  TNotifyEvent FOnInitializeLog;
  TRemoteTokenList FMembership;
  TRemoteTokenList FGroups;
  TRemoteTokenList FUsers;
  bool FUsersGroupsLookedup;
  TFileOperationProgressEvent FOnProgress;
  TFileOperationFinished FOnFinished;
  TFileOperationProgressType * FOperationProgress;
  bool FUseBusyCursor;
  TRemoteDirectoryCache * FDirectoryCache;
  TRemoteDirectoryChangesCache * FDirectoryChangesCache;
  TSecureShell * FSecureShell;
  UnicodeString FLastDirectoryChange;
  TCurrentFSProtocol FFSProtocol;
  TTerminal * FCommandSession;
  bool FAutoReadDirectory;
  bool FReadingCurrentDirectory;
  bool * FClosedOnCompletion;
  TSessionStatus FStatus;
  int FOpening;
  RawByteString FRememberedPassword;
  TPromptKind FRememberedPasswordKind;
  RawByteString FRememberedTunnelPassword;
  bool FRememberedPasswordUsed;
  TTunnelThread * FTunnelThread;
  TSecureShell * FTunnel;
  TSessionData * FTunnelData;
  TSessionLog * FTunnelLog;
  TTunnelUI * FTunnelUI;
  int FTunnelLocalPortNumber;
  UnicodeString FTunnelError;
  TQueryUserEvent FOnQueryUser;
  TPromptUserEvent FOnPromptUser;
  TDisplayBannerEvent FOnDisplayBanner;
  TExtendedExceptionEvent FOnShowExtendedException;
  TInformationEvent FOnInformation;
  TCustomCommandEvent FOnCustomCommand;
  TNotifyEvent FOnClose;
  TCallbackGuard * FCallbackGuard;
  TFindingFileEvent FOnFindingFile;
  std::unique_ptr<TStrings> FShellChecksumAlgDefs;
  bool FEnableSecureShellUsage;
  bool FCollectFileSystemUsage;
  bool FRememberedPasswordTried;
  bool FRememberedTunnelPasswordTried;
  int FNesting;
  UnicodeString FFingerprintScannedSHA256;
  UnicodeString FFingerprintScannedSHA1;
  UnicodeString FFingerprintScannedMD5;
  DWORD FLastProgressLogged;
  UnicodeString FDestFileName;
  bool FMultipleDestinationFiles;
  bool FFileTransferAny;
  typedef std::map<UnicodeString, UnicodeString> TEncryptedFileNames;
  TEncryptedFileNames FEncryptedFileNames;
  std::set<UnicodeString> FFoldersScannedForEncryptedFiles;
  RawByteString FEncryptKey;
  TFileOperationProgressType::TPersistence * FOperationProgressPersistence;
  TOnceDoneOperation FOperationProgressOnceDoneOperation;
  UnicodeString FCollectedCalculatedChecksum;
  TTerminalUI * FTerminalUI;

  void __fastcall CommandError(Exception * E, const UnicodeString Msg);
  unsigned int __fastcall CommandError(Exception * E, const UnicodeString Msg,
    unsigned int Answers, const UnicodeString HelpKeyword = L"");
  UnicodeString __fastcall GetCurrentDirectory();
  bool __fastcall GetExceptionOnFail() const;
  const TRemoteTokenList * __fastcall GetGroups();
  const TRemoteTokenList * __fastcall GetUsers();
  const TRemoteTokenList * __fastcall GetMembership();
  void __fastcall SetCurrentDirectory(UnicodeString value);
  void __fastcall SetExceptionOnFail(bool value);
  void __fastcall ReactOnCommand(int /*TFSCommand*/ Cmd);
  UnicodeString __fastcall GetUserName() const;
  bool __fastcall GetAreCachesEmpty() const;
  void __fastcall ClearCachedFileList(const UnicodeString Path, bool SubDirs);
  void __fastcall AddCachedFileList(TRemoteFileList * FileList);
  bool __fastcall GetCommandSessionOpened();
  TTerminal * __fastcall GetCommandSession();
  bool __fastcall GetResolvingSymlinks();
  bool __fastcall GetActive();
  UnicodeString __fastcall GetPassword();
  UnicodeString __fastcall GetRememberedPassword();
  UnicodeString __fastcall GetRememberedTunnelPassword();
  bool __fastcall GetStoredCredentialsTried();
  inline bool __fastcall InTransaction();
  void __fastcall SaveCapabilities(TFileSystemInfo & FileSystemInfo);
  void LogAndInformation(const UnicodeString & S);
  static UnicodeString __fastcall SynchronizeModeStr(TSynchronizeMode Mode);
  static UnicodeString __fastcall SynchronizeParamsStr(int Params);
  bool VerifyCertificateAgainstSessionData(
    const UnicodeString & FingerprintSHA1, const UnicodeString & FingerprintSHA256,
    const UnicodeString & CertificateSubject);

protected:
  bool FReadCurrentDirectoryPending;
  bool FReadDirectoryPending;
  bool FTunnelOpening;
  TCustomFileSystem * FFileSystem;
  int FSecondaryTerminals;

  void __fastcall DoStartReadDirectory();
  void __fastcall DoReadDirectoryProgress(int Progress, int ResolvedLinks, bool & Cancel);
  void __fastcall DoReadDirectory(bool ReloadOnly);
  void DoReadDirectoryFinish(TRemoteDirectory * Files, bool ReloadOnly);
  void __fastcall DoCreateDirectory(const UnicodeString & DirName, bool Encrypt);
  void __fastcall DoDeleteFile(
    TCustomFileSystem * FileSystem, const UnicodeString & FileName, const TRemoteFile * File, int Params);
  void __fastcall DoCustomCommandOnFile(UnicodeString FileName,
    const TRemoteFile * File, UnicodeString Command, int Params, TCaptureOutputEvent OutputEvent);
  bool DoRenameOrCopyFile(
    bool Rename, const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & NewName,
    bool Move, bool DontOverwrite, bool IsBatchOperation);
  bool __fastcall DoRenameFile(
    const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & NewName, bool Move, bool DontOverwrite);
  bool __fastcall DoMoveFile(const UnicodeString & FileName, const TRemoteFile * File, /*const TMoveFileParams*/ void * Param);
  void __fastcall DoCopyFile(
    const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & NewName, bool DontOverwrite);
  void __fastcall DoChangeFileProperties(const UnicodeString FileName,
    const TRemoteFile * File, const TRemoteProperties * Properties);
  void __fastcall DoChangeDirectory();
  void __fastcall DoInitializeLog();
  void __fastcall EnsureNonExistence(const UnicodeString FileName);
  void __fastcall LookupUsersGroups();
  void __fastcall FileModified(const TRemoteFile * File,
    const UnicodeString FileName, bool ClearDirectoryChange = false);
  int __fastcall FileOperationLoop(TFileOperationEvent CallBackFunc,
    TFileOperationProgressType * OperationProgress, unsigned int Flags,
    const UnicodeString Message, void * Param1 = NULL, void * Param2 = NULL);
  bool __fastcall GetIsCapable(TFSCapability Capability) const;
  bool __fastcall ProcessFiles(TStrings * FileList, TFileOperation Operation,
    TProcessFileEvent ProcessFile, void * Param = NULL, TOperationSide Side = osRemote,
    bool Ex = false);
  bool __fastcall ProcessFilesEx(TStrings * FileList, TFileOperation Operation,
    TProcessFileEventEx ProcessFile, void * Param = NULL, TOperationSide Side = osRemote);
  void __fastcall ProcessDirectory(const UnicodeString DirName,
    TProcessFileEvent CallBackFunc, void * Param = NULL, bool UseCache = false,
    bool IgnoreErrors = false);
  bool __fastcall DeleteContentsIfDirectory(
    const UnicodeString & FileName, const TRemoteFile * File, int Params, TRmSessionAction & Action);
  void __fastcall AnnounceFileListOperation();
  void __fastcall ReadDirectory(TRemoteFileList * FileList);
  void __fastcall CustomReadDirectory(TRemoteFileList * FileList);
  void __fastcall DoCreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  bool __fastcall CreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void __fastcall OpenLocalFile(const UnicodeString FileName, unsigned int Access,
    int * Attrs, HANDLE * Handle, __int64 * ACTime, __int64 * MTime,
    __int64 * ATime, __int64 * Size, bool TryWriteReadOnly = true);
  void __fastcall OpenLocalFile(
    const UnicodeString & FileName, unsigned int Access, TLocalFileHandle & Handle, bool TryWriteReadOnly = true);
  bool __fastcall AllowLocalFileTransfer(
    const UnicodeString & FileName, const TSearchRecSmart * SearchRec,
    const TCopyParamType * CopyParam, TFileOperationProgressType * OperationProgress);
  bool __fastcall HandleException(Exception * E);
  void __fastcall CalculateFileSize(UnicodeString FileName,
    const TRemoteFile * File, /*TCalculateSizeParams*/ void * Size);
  void __fastcall DoCalculateFileSize(UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  bool __fastcall DoCalculateDirectorySize(const UnicodeString & FileName, TCalculateSizeParams * Params);
  void __fastcall CalculateLocalFileSize(
    const UnicodeString & FileName, const TSearchRecSmart & Rec, /*__int64*/ void * Size);
  TBatchOverwrite __fastcall EffectiveBatchOverwrite(
    const UnicodeString & SourceFullFileName, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, bool Special);
  bool __fastcall CheckRemoteFile(
    const UnicodeString & FileName, const TCopyParamType * CopyParam,
    int Params, TFileOperationProgressType * OperationProgress);
  unsigned int __fastcall ConfirmFileOverwrite(
    const UnicodeString & SourceFullFileName, const UnicodeString & TargetFileName,
    const TOverwriteFileParams * FileParams, unsigned int Answers, TQueryParams * QueryParams,
    TOperationSide Side, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, UnicodeString Message = L"");
  void DoSynchronizeCollectDirectory(TSynchronizeData Data);
  bool __fastcall LocalFindFirstLoop(const UnicodeString & Directory, TSearchRecChecked & SearchRec);
  bool __fastcall LocalFindNextLoop(TSearchRecChecked & SearchRec);
  bool __fastcall DoAllowLocalFileTransfer(
    const UnicodeString & FileName, const TSearchRecSmart & SearchRec, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool __fastcall DoAllowRemoteFileTransfer(
    const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool __fastcall IsEmptyLocalDirectory(
    const UnicodeString & LocalDirectory, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  bool __fastcall IsEmptyRemoteDirectory(
    const TRemoteFile * File, const TCopyParamType * CopyParam, bool DisallowTemporaryTransferFiles);
  void __fastcall DoSynchronizeCollectFile(const UnicodeString FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void __fastcall SynchronizeCollectFile(const UnicodeString FileName,
    const TRemoteFile * File, /*TSynchronizeData*/ void * Param);
  void SynchronizeCollectLocalFile(
    const UnicodeString & FileName, const TSearchRecSmart & Rec, TSynchronizeData * Data);
  void DoSynchronizeCollectLocalFile(const UnicodeString & FileName, const TSearchRecSmart & SearchRec, TSynchronizeData * Data);
  bool SameFileChecksum(const UnicodeString & LeftFileName, const UnicodeString & RightFileName, const TRemoteFile * RightFile);
  UnicodeString CalculateLocalFileChecksum(const UnicodeString & FileName, const UnicodeString & Alg);
  void __fastcall CollectCalculatedChecksum(
    const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
  void __fastcall SynchronizeRemoteTimestamp(const UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void __fastcall SynchronizeLocalTimestamp(const UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  void __fastcall DoSynchronizeProgress(const TSynchronizeData & Data, bool Collect);
  void SynchronizedFileCheckModified(
    TSynchronizeData * Data, std::unique_ptr<TSynchronizeChecklist::TItem> & ChecklistItem,
    const UnicodeString & FullLeftFileName, TSynchronizeFileData * LocalData,
    const UnicodeString & FullRightFileName, const TRemoteFile * RightFile);
  void SynchronizedFileNew(
    TSynchronizeData * Data, std::unique_ptr<TSynchronizeChecklist::TItem> & ChecklistItem,
    const UnicodeString & FullRightFileName, const TRemoteFile * RightLinkedFile);
  void SynchronizedFileNewOrModified(
    TSynchronizeData * Data, std::unique_ptr<TSynchronizeChecklist::TItem> & ChecklistItem, const TRemoteFile * File, bool New);
  bool DoFilesMatch(const TSynchronizeFileData * LeftItem, TSynchronizeChecklist::TItem * RightItem);
  void __fastcall DeleteLocalFile(UnicodeString FileName,
    const TRemoteFile * File, void * Param);
  bool __fastcall RecycleFile(const UnicodeString & FileName, const TRemoteFile * File);
  TStrings * __fastcall GetFixedPaths();
  void __fastcall DoStartup();
  virtual bool __fastcall DoQueryReopen(Exception * E);
  virtual void __fastcall FatalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  void __fastcall ResetConnection();
  virtual bool __fastcall DoPromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Response);
  void __fastcall OpenTunnel();
  void __fastcall CloseTunnel();
  void __fastcall DoInformation(
    const UnicodeString & Str, int Phase = -1, const UnicodeString & Additional = UnicodeString());
  bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, UnicodeString Prompt, bool Echo,
    int MaxLen, UnicodeString & Result);
  void __fastcall FileFind(UnicodeString FileName, const TRemoteFile * File, void * Param);
  void __fastcall DoFilesFind(UnicodeString Directory, TFilesFindParams & Params, UnicodeString RealDirectory);
  bool __fastcall DoCreateLocalFile(const UnicodeString FileName,
    TFileOperationProgressType * OperationProgress, HANDLE * AHandle,
    bool NoConfirmation);
  void __fastcall LockFile(const UnicodeString FileName, const TRemoteFile * File, void * Param);
  void __fastcall UnlockFile(const UnicodeString FileName, const TRemoteFile * File, void * Param);
  void __fastcall DoLockFile(const UnicodeString & FileName, const TRemoteFile * File);
  void __fastcall DoUnlockFile(const UnicodeString & FileName, const TRemoteFile * File);
  void __fastcall OperationFinish(
    TFileOperationProgressType * Progress, const void * Item, const UnicodeString & FileName,
    bool Success, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int Count);
  void __fastcall OperationStart(
    TFileOperationProgressType & Progress, TFileOperation Operation, TOperationSide Side, int Count,
    bool Temp, const UnicodeString & Directory, unsigned long CPSLimit, TOnceDoneOperation OnceDoneOperation);
  void __fastcall OperationStop(TFileOperationProgressType & Progress);
  virtual void __fastcall Information(const UnicodeString & Str);
  virtual unsigned int __fastcall QueryUser(const UnicodeString Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual unsigned int __fastcall QueryUserException(const UnicodeString Query,
    Exception * E, unsigned int Answers, const TQueryParams * Params,
    TQueryType QueryType = qtConfirmation);
  virtual bool __fastcall PromptUser(TSessionData * Data, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts, TStrings * Results);
  virtual void __fastcall DisplayBanner(const UnicodeString & Banner);
  virtual void __fastcall Closed();
  virtual void __fastcall ProcessGUI();
  void __fastcall Progress(TFileOperationProgressType * OperationProgress);
  virtual void __fastcall HandleExtendedException(Exception * E);
  bool __fastcall IsListenerFree(unsigned int PortNumber);
  void __fastcall DoProgress(TFileOperationProgressType & ProgressData);
  void __fastcall DoFinished(TFileOperation Operation, TOperationSide Side, bool Temp,
    const UnicodeString & FileName, bool Success, bool NotCancelled, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall RollbackAction(TSessionAction & Action,
    TFileOperationProgressType * OperationProgress, Exception * E = NULL);
  void __fastcall DoAnyCommand(const UnicodeString Command, TCaptureOutputEvent OutputEvent,
    TCallSessionAction * Action);
  TRemoteFileList * __fastcall DoReadDirectoryListing(UnicodeString Directory, bool UseCache);
  RawByteString __fastcall EncryptPassword(const UnicodeString & Password);
  UnicodeString __fastcall DecryptPassword(const RawByteString & Password);
  UnicodeString __fastcall GetRemoteFileInfo(TRemoteFile * File);
  void __fastcall LogRemoteFile(TRemoteFile * File);
  UnicodeString __fastcall FormatFileDetailsForLog(
    const UnicodeString & FileName, TDateTime Modification, __int64 Size, const TRemoteFile * LinkedFile = NULL);
  void __fastcall LogFileDetails(const UnicodeString & FileName, TDateTime Modification, __int64 Size, const TRemoteFile * LinkedFile = NULL);
  void __fastcall LogFileDone(
    TFileOperationProgressType * OperationProgress, const UnicodeString & DestFileName,
    TTransferSessionAction & Action);
  void __fastcall LogTotalTransferDetails(
    const UnicodeString TargetDir, const TCopyParamType * CopyParam,
    TFileOperationProgressType * OperationProgress, bool Parallel, TStrings * Files);
  void __fastcall LogTotalTransferDone(TFileOperationProgressType * OperationProgress);
  virtual TTerminal * __fastcall GetPrimaryTerminal();
  void __fastcall DoEndTransaction(bool Inform);
  bool  __fastcall VerifyCertificate(
    const UnicodeString & CertificateStorageKey, const UnicodeString & SiteKey,
    const UnicodeString & FingerprintSHA1, const UnicodeString & FingerprintSHA256,
    const UnicodeString & CertificateSubject, int Failures);
  void __fastcall CacheCertificate(const UnicodeString & CertificateStorageKey,
    const UnicodeString & SiteKey, const UnicodeString & FingerprintSHA1, const UnicodeString & FingerprintSHA256,
    int Failures);
  bool ConfirmCertificate(TSessionInfo & SessionInfo, int Failures, const UnicodeString & CertificateStorageKey);
  bool VerifyOrConfirmHttpCertificate(
    const UnicodeString & AHostName, int APortNumber, const TNeonCertificateData & Data, TSessionInfo & SessionInfo);
  void __fastcall CollectTlsUsage(const UnicodeString & TlsVersionStr);
  bool __fastcall LoadTlsCertificate(X509 *& Certificate, EVP_PKEY *& PrivateKey);
  bool __fastcall TryStartOperationWithFile(
    const UnicodeString & FileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  void __fastcall StartOperationWithFile(
    const UnicodeString & FileName, TFileOperation Operation1, TFileOperation Operation2 = foNone);
  bool __fastcall CanRecurseToDirectory(const TRemoteFile * File);
  bool __fastcall DoOnCustomCommand(const UnicodeString & Command);
  bool __fastcall CanParallel(const TCopyParamType * CopyParam, int Params, TParallelOperation * ParallelOperation);
  void __fastcall CopyParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress);
  void __fastcall DoCopyToRemote(
    TStrings * FilesToCopy, const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall SourceRobust(
    const UnicodeString & FileName, const TSearchRecSmart * SearchRec,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void __fastcall Source(
    const UnicodeString & FileName, const TSearchRecSmart * SearchRec,
    const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags, TUploadSessionAction & Action, bool & ChildError);
  void __fastcall DirectorySource(
    const UnicodeString & DirectoryName, const UnicodeString & TargetDir, const UnicodeString & DestDirectoryName,
    int Attrs, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags);
  bool UseAsciiTransfer(
    const UnicodeString & BaseFileName, TOperationSide Side, const TCopyParamType * CopyParam,
    const TFileMasks::TParams & MaskParams);
  void __fastcall SelectTransferMode(
    const UnicodeString & BaseFileName, TOperationSide Side, const TCopyParamType * CopyParam,
    const TFileMasks::TParams & MaskParams);
  void __fastcall SelectSourceTransferMode(const TLocalFileHandle & Handle, const TCopyParamType * CopyParam);
  void DoDeleteLocalFile(const UnicodeString & FileName);
  void DoRenameLocalFileForce(const UnicodeString & OldName, const UnicodeString & NewName);
  void __fastcall UpdateSource(const TLocalFileHandle & Handle, const TCopyParamType * CopyParam, int Params);
  void __fastcall DoCopyToLocal(
    TStrings * FilesToCopy, const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * OperationProgress, unsigned int Flags, TOnceDoneOperation & OnceDoneOperation);
  void __fastcall SinkRobust(
    const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags);
  void __fastcall Sink(
    const UnicodeString & FileName, const TRemoteFile * File, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, int Params, TFileOperationProgressType * OperationProgress, unsigned int Flags,
    TDownloadSessionAction & Action);
  void __fastcall SinkFile(UnicodeString FileName, const TRemoteFile * File, void * Param);
  void __fastcall UpdateTargetAttrs(
    const UnicodeString & DestFullName, const TRemoteFile * File, const TCopyParamType * CopyParam, int Attrs);
  void __fastcall UpdateTargetTime(
    HANDLE Handle, TDateTime Modification, TModificationFmt ModificationFmt, TDSTMode DSTMode);
  void CheckParallelFileTransfer(
    const UnicodeString & TargetDir, TStringList * Files, const TCopyParamType * CopyParam, int Params,
    UnicodeString & ParallelFileName, __int64 & ParallelFileSize, TFileOperationProgressType * OperationProgress);
  TRemoteFile * CheckRights(const UnicodeString & EntryType, const UnicodeString & FileName, bool & WrongRights);
  bool IsValidFile(TRemoteFile * File);
  void __fastcall CalculateSubFoldersChecksum(
    const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum,
    TFileOperationProgressType * OperationProgress, bool FirstLevel);
  void GetShellChecksumAlgs(TStrings * Algs);
  TStrings * GetShellChecksumAlgDefs();
  TStrings * ProcessFeatures(TStrings * Features);

  UnicodeString __fastcall EncryptFileName(const UnicodeString & Path, bool EncryptNewFiles);
  UnicodeString __fastcall DecryptFileName(const UnicodeString & Path, bool DecryptFullPath, bool DontCache);
  TEncryptedFileNames::const_iterator __fastcall GetEncryptedFileName(const UnicodeString & Path);
  bool __fastcall IsFileEncrypted(const UnicodeString & Path, bool EncryptNewFiles = false);

  TFileOperationProgressType * __property_get_OperationProgress() const { return FOperationProgress; } __declspec(property(get = __property_get_OperationProgress)) TFileOperationProgressType * OperationProgress;

public:
  __fastcall TTerminal(TSessionData * SessionData, TConfiguration * Configuration, TActionLog * ActionLog = NULL);
  __fastcall ~TTerminal();
  void __fastcall Open();
  void __fastcall Close();
  void __fastcall FingerprintScan(UnicodeString & SHA256, UnicodeString & SHA1, UnicodeString & MD5);
  void __fastcall Reopen(int Params);
  virtual void __fastcall DirectoryModified(const UnicodeString Path, bool SubDirs);
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  void __fastcall ShowExtendedException(Exception * E);
  void __fastcall Idle();
  void __fastcall RecryptPasswords();
  bool __fastcall AllowedAnyCommand(const UnicodeString Command);
  void __fastcall AnyCommand(const UnicodeString Command, TCaptureOutputEvent OutputEvent);
  void __fastcall CloseOnCompletion(
    TOnceDoneOperation Operation = odoDisconnect, const UnicodeString & Message = L"",
    const UnicodeString & TargetLocalPath = L"", const UnicodeString & DestLocalFileName = L"");
  UnicodeString __fastcall AbsolutePath(UnicodeString Path, bool Local);
  void __fastcall BeginTransaction();
  void __fastcall ReadCurrentDirectory();
  void __fastcall ReadDirectory(bool ReloadOnly, bool ForceCache = false);
  TRemoteFileList * __fastcall ReadDirectoryListing(UnicodeString Directory, const TFileMasks & Mask);
  TRemoteFileList * __fastcall CustomReadDirectoryListing(UnicodeString Directory, bool UseCache);
  TRemoteFile * __fastcall ReadFileListing(UnicodeString Path);
  TRemoteFile * ReadFile(const UnicodeString & FileName);
  TRemoteFile * TryReadFile(const UnicodeString & FileName);
  bool FileExists(const UnicodeString & FileName);
  bool DirectoryExists(const UnicodeString & FileName);
  void __fastcall ReadSymlink(TRemoteFile * SymlinkFile, TRemoteFile *& File);
  bool __fastcall CopyToLocal(
    TStrings * FilesToCopy, const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TParallelOperation * ParallelOperation);
  bool __fastcall CopyToRemote(
    TStrings * FilesToCopy, const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TParallelOperation * ParallelOperation);
  int __fastcall CopyToParallel(TParallelOperation * ParallelOperation, TFileOperationProgressType * OperationProgress);
  void __fastcall LogParallelTransfer(TParallelOperation * ParallelOperation);
  void __fastcall CreateDirectory(const UnicodeString & DirName, const TRemoteProperties * Properties);
  bool __fastcall CreateTargetDirectory(const UnicodeString & DirectoryPath, int Attrs, const TCopyParamType * CopyParam);
  void __fastcall CreateLink(const UnicodeString FileName, const UnicodeString PointTo, bool Symbolic);
  void __fastcall DeleteFile(UnicodeString FileName,
    const TRemoteFile * File = NULL, void * Params = NULL);
  bool __fastcall DeleteFiles(TStrings * FilesToDelete, int Params = 0);
  bool __fastcall DeleteLocalFiles(TStrings * FileList, int Params = 0);
  bool __fastcall IsRecycledFile(UnicodeString FileName);
  void __fastcall CustomCommandOnFile(UnicodeString FileName,
    const TRemoteFile * File, void * AParams);
  void __fastcall CustomCommandOnFiles(UnicodeString Command, int Params,
    TStrings * Files, TCaptureOutputEvent OutputEvent);
  void __fastcall ChangeDirectory(const UnicodeString Directory);
  void __fastcall EndTransaction();
  void __fastcall HomeDirectory();
  UnicodeString __fastcall GetHomeDirectory();
  void __fastcall ChangeFileProperties(UnicodeString FileName,
    const TRemoteFile * File, /*const TRemoteProperties */ void * Properties);
  void __fastcall ChangeFilesProperties(TStrings * FileList,
    const TRemoteProperties * Properties);
  bool __fastcall LoadFilesProperties(TStrings * FileList);
  NORETURN void __fastcall TerminalError(UnicodeString Msg);
  NORETURN void __fastcall TerminalError(Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  void __fastcall ReloadDirectory();
  void __fastcall RefreshDirectory();
  void __fastcall RenameFile(const TRemoteFile * File, const UnicodeString & NewName);
  void __fastcall MoveFile(const UnicodeString FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool __fastcall MoveFiles(
    TStrings * FileList, const UnicodeString & Target, const UnicodeString & FileMask, bool DontOverwrite);
  void __fastcall CopyFile(const UnicodeString FileName, const TRemoteFile * File,
    /*const TMoveFileParams*/ void * Param);
  bool __fastcall CopyFiles(
    TStrings * FileList, const UnicodeString & Target, const UnicodeString & FileMask, bool DontOverwrite);
  bool CalculateFilesSize(TStrings * FileList, __int64 & Size, TCalculateSizeParams & Params);
  bool __fastcall CalculateLocalFilesSize(TStrings * FileList, __int64 & Size,
    const TCopyParamType * CopyParam, bool AllowDirs, TStrings * Files, TCalculatedSizes * CalculatedSizes);
  void __fastcall CalculateFilesChecksum(
    const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum);
  void __fastcall ClearCaches();
  TSynchronizeChecklist * SynchronizeCollect(
    const UnicodeString & Directory1, const UnicodeString & Directory2,
    TSynchronizeMode Mode, const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory, TSynchronizeOptions * Options);
  void __fastcall SynchronizeApply(
    TSynchronizeChecklist * Checklist,
    const TCopyParamType * CopyParam, int Params,
    TSynchronizeDirectory OnSynchronizeDirectory, TProcessedSynchronizationChecklistItem OnProcessedItem,
    TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems, void * Token,
    TFileOperationStatistics * Statistics);
  TQueueItem * SynchronizeToQueue(
    const TSynchronizeChecklist::TItem * ChecklistItem, const TCopyParamType * CopyParam, int Params, bool Parallel);
  void __fastcall SynchronizeChecklistCalculateSize(
    TSynchronizeChecklist * Checklist, const TSynchronizeChecklist::TItemList & Items,
    const TCopyParamType * CopyParam, int Params);
  static TCopyParamType GetSynchronizeCopyParam(const TCopyParamType * CopyParam, int Params);
  static int GetSynchronizeCopyParams(int Params);
  void __fastcall FilesFind(UnicodeString Directory, const TFileMasks & FileMask,
    TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile);
  void __fastcall SpaceAvailable(const UnicodeString Path, TSpaceAvailable & ASpaceAvailable);
  void __fastcall LockFiles(TStrings * FileList);
  void __fastcall UnlockFiles(TStrings * FileList);
  TRemoteFileList * __fastcall DirectoryFileList(const UnicodeString Path, TDateTime Timestamp, bool CanLoad);
  void __fastcall MakeLocalFileList(
    const UnicodeString & FileName, const TSearchRecSmart & Rec, void * Param);
  bool __fastcall FileOperationLoopQuery(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString Message,
    unsigned int Flags, UnicodeString SpecialRetry = L"", UnicodeString HelpKeyword = L"");
  void __fastcall FileOperationLoopEnd(Exception & E,
    TFileOperationProgressType * OperationProgress, const UnicodeString & Message,
    unsigned int Flags, const UnicodeString & SpecialRetry, const UnicodeString & HelpKeyword);
  TUsableCopyParamAttrs __fastcall UsableCopyParamAttrs(int Params);
  bool __fastcall ContinueReopen(TDateTime Start);
  bool __fastcall QueryReopen(Exception * E, int Params,
    TFileOperationProgressType * OperationProgress);
  UnicodeString __fastcall PeekCurrentDirectory();
  void __fastcall FatalAbort();
  void __fastcall ReflectSettings();
  void __fastcall CollectUsage();
  TTerminal * __fastcall CreateSecondarySession(const UnicodeString & Name, TSessionData * SessionData);
  void __fastcall FillSessionDataForCode(TSessionData * Data);
  void __fastcall UpdateSessionCredentials(TSessionData * Data);
  UnicodeString UploadPublicKey(const UnicodeString & FileName);
  TCustomFileSystem * GetFileSystemForCapability(TFSCapability Capability, bool NeedCurrentDirectory = false);
  void PrepareCommandSession(bool NeedCurrentDirectory = false);

  const TSessionInfo & __fastcall GetSessionInfo();
  const TFileSystemInfo & __fastcall GetFileSystemInfo(bool Retrieve = false);
  void __fastcall LogEvent(const UnicodeString & Str);
  void __fastcall LogEvent(int Level, const UnicodeString & Str);
  void __fastcall GetSupportedChecksumAlgs(TStrings * Algs);
  UnicodeString __fastcall ChangeFileName(const TCopyParamType * CopyParam,
    UnicodeString FileName, TOperationSide Side, bool FirstLevel);
  UnicodeString __fastcall GetBaseFileName(UnicodeString FileName);
  bool __fastcall IsEncryptingFiles() const { return !FEncryptKey.IsEmpty(); }
  RawByteString __fastcall GetEncryptKey() { return FEncryptKey; }

  static UnicodeString __fastcall ExpandFileName(UnicodeString Path,
    const UnicodeString BasePath);

  TSessionData * __property_get_SessionData() const { return FSessionData; } __declspec(property(get = __property_get_SessionData)) TSessionData * SessionData;
  TSessionLog * __property_get_Log() const { return FLog; } __declspec(property(get = __property_get_Log)) TSessionLog * Log;
  TActionLog * __property_get_ActionLog() const { return FActionLog; } __declspec(property(get = __property_get_ActionLog)) TActionLog * ActionLog;
  TConfiguration * __property_get_Configuration() const { return FConfiguration; } __declspec(property(get = __property_get_Configuration)) TConfiguration * Configuration;
  bool __property_get_Active() const { return ::rtl::Mutable(this)->GetActive(); } __declspec(property(get = __property_get_Active)) bool Active;
  TSessionStatus __property_get_Status() const { return FStatus; } __declspec(property(get = __property_get_Status)) TSessionStatus Status;
  UnicodeString __property_get_CurrentDirectory() const { return ::rtl::Mutable(this)->GetCurrentDirectory(); } void __property_set_CurrentDirectory(UnicodeString Value) { SetCurrentDirectory(Value); } __declspec(property(get = __property_get_CurrentDirectory, put = __property_set_CurrentDirectory)) UnicodeString CurrentDirectory;
  bool __property_get_ExceptionOnFail() const { return ::rtl::Mutable(this)->GetExceptionOnFail(); } void __property_set_ExceptionOnFail(bool Value) { SetExceptionOnFail(Value); } __declspec(property(get = __property_get_ExceptionOnFail, put = __property_set_ExceptionOnFail)) bool ExceptionOnFail;
  TRemoteDirectory * __property_get_Files() const { return FFiles; } __declspec(property(get = __property_get_Files)) TRemoteDirectory * Files;
  TNotifyEvent __property_get_OnChangeDirectory() const { return FOnChangeDirectory; } void __property_set_OnChangeDirectory(TNotifyEvent Value) { FOnChangeDirectory = Value; } __declspec(property(get = __property_get_OnChangeDirectory, put = __property_set_OnChangeDirectory)) TNotifyEvent OnChangeDirectory;
  TReadDirectoryEvent __property_get_OnReadDirectory() const { return FOnReadDirectory; } void __property_set_OnReadDirectory(TReadDirectoryEvent Value) { FOnReadDirectory = Value; } __declspec(property(get = __property_get_OnReadDirectory, put = __property_set_OnReadDirectory)) TReadDirectoryEvent OnReadDirectory;
  TNotifyEvent __property_get_OnStartReadDirectory() const { return FOnStartReadDirectory; } void __property_set_OnStartReadDirectory(TNotifyEvent Value) { FOnStartReadDirectory = Value; } __declspec(property(get = __property_get_OnStartReadDirectory, put = __property_set_OnStartReadDirectory)) TNotifyEvent OnStartReadDirectory;
  TReadDirectoryProgressEvent __property_get_OnReadDirectoryProgress() const { return FOnReadDirectoryProgress; } void __property_set_OnReadDirectoryProgress(TReadDirectoryProgressEvent Value) { FOnReadDirectoryProgress = Value; } __declspec(property(get = __property_get_OnReadDirectoryProgress, put = __property_set_OnReadDirectoryProgress)) TReadDirectoryProgressEvent OnReadDirectoryProgress;
  TDeleteLocalFileEvent __property_get_OnDeleteLocalFile() const { return FOnDeleteLocalFile; } void __property_set_OnDeleteLocalFile(TDeleteLocalFileEvent Value) { FOnDeleteLocalFile = Value; } __declspec(property(get = __property_get_OnDeleteLocalFile, put = __property_set_OnDeleteLocalFile)) TDeleteLocalFileEvent OnDeleteLocalFile;
  TNotifyEvent __property_get_OnInitializeLog() const { return FOnInitializeLog; } void __property_set_OnInitializeLog(TNotifyEvent Value) { FOnInitializeLog = Value; } __declspec(property(get = __property_get_OnInitializeLog, put = __property_set_OnInitializeLog)) TNotifyEvent OnInitializeLog;
  const TRemoteTokenList * __property_get_Groups() const { return ::rtl::Mutable(this)->GetGroups(); } __declspec(property(get = __property_get_Groups)) const TRemoteTokenList * Groups;
  const TRemoteTokenList * __property_get_Users() const { return ::rtl::Mutable(this)->GetUsers(); } __declspec(property(get = __property_get_Users)) const TRemoteTokenList * Users;
  const TRemoteTokenList * __property_get_Membership() const { return ::rtl::Mutable(this)->GetMembership(); } __declspec(property(get = __property_get_Membership)) const TRemoteTokenList * Membership;
  TFileOperationProgressEvent __property_get_OnProgress() const { return FOnProgress; } void __property_set_OnProgress(TFileOperationProgressEvent Value) { FOnProgress = Value; } __declspec(property(get = __property_get_OnProgress, put = __property_set_OnProgress)) TFileOperationProgressEvent OnProgress;
  TFileOperationFinished __property_get_OnFinished() const { return FOnFinished; } void __property_set_OnFinished(TFileOperationFinished Value) { FOnFinished = Value; } __declspec(property(get = __property_get_OnFinished, put = __property_set_OnFinished)) TFileOperationFinished OnFinished;
  TCurrentFSProtocol __property_get_FSProtocol() const { return FFSProtocol; } __declspec(property(get = __property_get_FSProtocol)) TCurrentFSProtocol FSProtocol;
  bool __property_get_UseBusyCursor() const { return FUseBusyCursor; } void __property_set_UseBusyCursor(bool Value) { FUseBusyCursor = Value; } __declspec(property(get = __property_get_UseBusyCursor, put = __property_set_UseBusyCursor)) bool UseBusyCursor;
  UnicodeString __property_get_UserName() const { return ::rtl::Mutable(this)->GetUserName(); } __declspec(property(get = __property_get_UserName)) UnicodeString UserName;
  bool __property_get_IsCapable(TFSCapability Capability) const { return ::rtl::Mutable(this)->GetIsCapable(Capability); } __declspec(property(get = __property_get_IsCapable)) bool IsCapable[];
  bool __property_get_AreCachesEmpty() const { return ::rtl::Mutable(this)->GetAreCachesEmpty(); } __declspec(property(get = __property_get_AreCachesEmpty)) bool AreCachesEmpty;
  bool __property_get_CommandSessionOpened() const { return ::rtl::Mutable(this)->GetCommandSessionOpened(); } __declspec(property(get = __property_get_CommandSessionOpened)) bool CommandSessionOpened;
  TTerminal * __property_get_CommandSession() const { return ::rtl::Mutable(this)->GetCommandSession(); } __declspec(property(get = __property_get_CommandSession)) TTerminal * CommandSession;
  TTerminal * __property_get_PrimaryTerminal() const { return ::rtl::Mutable(this)->GetPrimaryTerminal(); } __declspec(property(get = __property_get_PrimaryTerminal)) TTerminal * PrimaryTerminal;
  bool __property_get_AutoReadDirectory() const { return FAutoReadDirectory; } void __property_set_AutoReadDirectory(bool Value) { FAutoReadDirectory = Value; } __declspec(property(get = __property_get_AutoReadDirectory, put = __property_set_AutoReadDirectory)) bool AutoReadDirectory;
  TStrings * __property_get_FixedPaths() const { return ::rtl::Mutable(this)->GetFixedPaths(); } __declspec(property(get = __property_get_FixedPaths)) TStrings * FixedPaths;
  bool __property_get_ResolvingSymlinks() const { return ::rtl::Mutable(this)->GetResolvingSymlinks(); } __declspec(property(get = __property_get_ResolvingSymlinks)) bool ResolvingSymlinks;
  UnicodeString __property_get_Password() const { return ::rtl::Mutable(this)->GetPassword(); } __declspec(property(get = __property_get_Password)) UnicodeString Password;
  bool __property_get_StoredCredentialsTried() const { return ::rtl::Mutable(this)->GetStoredCredentialsTried(); } __declspec(property(get = __property_get_StoredCredentialsTried)) bool StoredCredentialsTried;
  TQueryUserEvent __property_get_OnQueryUser() const { return FOnQueryUser; } void __property_set_OnQueryUser(TQueryUserEvent Value) { FOnQueryUser = Value; } __declspec(property(get = __property_get_OnQueryUser, put = __property_set_OnQueryUser)) TQueryUserEvent OnQueryUser;
  TPromptUserEvent __property_get_OnPromptUser() const { return FOnPromptUser; } void __property_set_OnPromptUser(TPromptUserEvent Value) { FOnPromptUser = Value; } __declspec(property(get = __property_get_OnPromptUser, put = __property_set_OnPromptUser)) TPromptUserEvent OnPromptUser;
  TDisplayBannerEvent __property_get_OnDisplayBanner() const { return FOnDisplayBanner; } void __property_set_OnDisplayBanner(TDisplayBannerEvent Value) { FOnDisplayBanner = Value; } __declspec(property(get = __property_get_OnDisplayBanner, put = __property_set_OnDisplayBanner)) TDisplayBannerEvent OnDisplayBanner;
  TExtendedExceptionEvent __property_get_OnShowExtendedException() const { return FOnShowExtendedException; } void __property_set_OnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; } __declspec(property(get = __property_get_OnShowExtendedException, put = __property_set_OnShowExtendedException)) TExtendedExceptionEvent OnShowExtendedException;
  TInformationEvent __property_get_OnInformation() const { return FOnInformation; } void __property_set_OnInformation(TInformationEvent Value) { FOnInformation = Value; } __declspec(property(get = __property_get_OnInformation, put = __property_set_OnInformation)) TInformationEvent OnInformation;
  TCustomCommandEvent __property_get_OnCustomCommand() const { return FOnCustomCommand; } void __property_set_OnCustomCommand(TCustomCommandEvent Value) { FOnCustomCommand = Value; } __declspec(property(get = __property_get_OnCustomCommand, put = __property_set_OnCustomCommand)) TCustomCommandEvent OnCustomCommand;
  TNotifyEvent __property_get_OnClose() const { return FOnClose; } void __property_set_OnClose(TNotifyEvent Value) { FOnClose = Value; } __declspec(property(get = __property_get_OnClose, put = __property_set_OnClose)) TNotifyEvent OnClose;
  int __property_get_TunnelLocalPortNumber() const { return FTunnelLocalPortNumber; } __declspec(property(get = __property_get_TunnelLocalPortNumber)) int TunnelLocalPortNumber;
};
//---------------------------------------------------------------------------
class TSecondaryTerminal : public TTerminal
{
public:
  __fastcall TSecondaryTerminal(
    TTerminal * MainTerminal, TSessionData * SessionData, TConfiguration * Configuration,
    const UnicodeString & Name, TActionLog * ActionLog);

  void __fastcall UpdateFromMain();

  TTerminal * __property_get_MainTerminal() const { return FMainTerminal; } __declspec(property(get = __property_get_MainTerminal)) TTerminal * MainTerminal;

protected:
  virtual void __fastcall DirectoryLoaded(TRemoteFileList * FileList);
  virtual void __fastcall DirectoryModified(const UnicodeString Path,
    bool SubDirs);
  virtual TTerminal * __fastcall GetPrimaryTerminal();

private:
  TTerminal * FMainTerminal;
};
//---------------------------------------------------------------------------
class TTerminalList : public TObjectList
{
public:
  __fastcall TTerminalList(TConfiguration * AConfiguration);
  __fastcall ~TTerminalList();

  virtual TTerminal * __fastcall NewTerminal(TSessionData * Data);
  virtual void __fastcall FreeTerminal(TTerminal * Terminal);
  void __fastcall FreeAndNullTerminal(TTerminal * & Terminal);
  void __fastcall RecryptPasswords();

  TTerminal * __property_get_Terminals(int Index) const { return ::rtl::Mutable(this)->GetTerminal(Index); } __declspec(property(get = __property_get_Terminals)) TTerminal * Terminals[];

protected:
  virtual TTerminal * __fastcall CreateTerminal(TSessionData * Data);

private:
  TConfiguration * FConfiguration;

  TTerminal * __fastcall GetTerminal(int Index);
};
//---------------------------------------------------------------------------
struct TCustomCommandParams
{
  UnicodeString Command;
  int Params;
  TCaptureOutputEvent OutputEvent;
};
//---------------------------------------------------------------------------
struct TCalculateSizeStats
{
  TCalculateSizeStats();

  int Files;
  int Directories;
  int SymLinks;
  TStrings * FoundFiles;
  TCalculatedSizes * CalculatedSizes;
};
//---------------------------------------------------------------------------
struct TCalculateSizeParams
{
friend class TTerminal;

public:
  TCalculateSizeParams();

  int Params;
  const TCopyParamType * CopyParam;
  TCalculateSizeStats * Stats;
  bool AllowDirs;
  bool UseCache;

private:
  TCollectedFileList * Files;
  UnicodeString LastDirPath;
  __int64 Size;
  bool Result;
};
//---------------------------------------------------------------------------
struct TOverwriteFileParams
{
  TOverwriteFileParams();

  __int64 SourceSize;
  __int64 DestSize;
  TDateTime SourceTimestamp;
  TDateTime DestTimestamp;
  TModificationFmt SourcePrecision;
  TModificationFmt DestPrecision;
};
//---------------------------------------------------------------------------
typedef std::vector<TDateTime> TDateTimes;
//---------------------------------------------------------------------------
struct TMakeLocalFileListParams
{
  TStrings * FileList;
  TDateTimes * FileTimes;
  bool IncludeDirs;
  bool Recursive;
};
//---------------------------------------------------------------------------
struct TSynchronizeOptions
{
  TSynchronizeOptions();
  ~TSynchronizeOptions();

  TStringList * Filter;
  int Files;

  bool __fastcall FilterFind(const UnicodeString & FileName);
  bool __fastcall MatchesFilter(const UnicodeString & FileName);
};
//---------------------------------------------------------------------------
struct TSpaceAvailable
{
  TSpaceAvailable();

  __int64 BytesOnDevice;
  __int64 UnusedBytesOnDevice;
  __int64 BytesAvailableToUser;
  __int64 UnusedBytesAvailableToUser;
  unsigned long BytesPerAllocationUnit;
};
//---------------------------------------------------------------------------
class TRobustOperationLoop
{
public:
  TRobustOperationLoop(TTerminal * Terminal, TFileOperationProgressType * OperationProgress, bool * AnyTransfer = NULL, bool CanRetry = true);
  ~TRobustOperationLoop();
  bool TryReopen(Exception & E);
  bool ShouldRetry();
  bool Retry();

private:
  TTerminal * FTerminal;
  TFileOperationProgressType * FOperationProgress;
  bool FRetry;
  bool * FAnyTransfer;
  bool FPrevAnyTransfer;
  TDateTime FStart;
  bool FCanRetry;
};
//---------------------------------------------------------------------------
class TCollectedFileList : public TObject
{
public:
  TCollectedFileList();
  virtual __fastcall ~TCollectedFileList();
  int Add(const UnicodeString & FileName, TObject * Object, bool Dir);
  void DidNotRecurse(int Index);
  void Delete(int Index);

  int Count() const;
  UnicodeString GetFileName(int Index) const;
  TObject * GetObject(int Index) const;
  bool IsDir(int Index) const;
  bool IsRecursed(int Index) const;
  int GetState(int Index) const;
  void SetState(int Index, int State);

private:
  void Deleting(int Index);

  struct TFileData
  {
    UnicodeString FileName;
    TObject * Object;
    bool Dir;
    bool Recursed;
    int State;
  };
  typedef std::vector<TFileData> TFileDataList;
  TFileDataList FList;
};
//---------------------------------------------------------------------------
class TQueueFileList;
//---------------------------------------------------------------------------
class TParallelOperation
{
public:
  TParallelOperation(TOperationSide Side);
  ~TParallelOperation();

  void Init(
    TStrings * AFiles, const UnicodeString & TargetDir, const TCopyParamType * CopyParam, int Params,
    TFileOperationProgressType * MainOperationProgress, const UnicodeString & MainName,
    __int64 ParallelFileSize);

  bool IsInitialized();
  void WaitFor();
  bool ShouldAddClient();
  void AddClient();
  void RemoveClient();
  int GetNext(
    TTerminal * Terminal, UnicodeString & FileName, TObject *& Object, UnicodeString & TargetDir, bool & Dir,
    bool & Recursed, TCopyParamType *& CustomCopyParam);
  void Done(
    const UnicodeString & FileName, bool Dir, bool Success, const UnicodeString & TargetDir,
    const TCopyParamType * CopyParam, TTerminal * Terminal);
  bool UpdateFileList(TQueueFileList * UpdateFileList);

  static bool GetOnlyFile(TStrings * FileList, UnicodeString & FileName, TObject *& Object);
  static TCollectedFileList * GetFileList(TStrings * FileList, int Index);
  static UnicodeString GetPartPrefix(const UnicodeString & FileName);
  TOperationSide __property_get_Side() const { return FSide; } __declspec(property(get = __property_get_Side)) TOperationSide Side;
  const TCopyParamType * __property_get_CopyParam() const { return FCopyParam; } __declspec(property(get = __property_get_CopyParam)) const TCopyParamType * CopyParam;
  int __property_get_Params() const { return FParams; } __declspec(property(get = __property_get_Params)) int Params;
  UnicodeString __property_get_TargetDir() const { return FTargetDir; } __declspec(property(get = __property_get_TargetDir)) UnicodeString TargetDir;
  TFileOperationProgressType * __property_get_MainOperationProgress() const { return FMainOperationProgress; } __declspec(property(get = __property_get_MainOperationProgress)) TFileOperationProgressType * MainOperationProgress;
  UnicodeString __property_get_MainName() const { return FMainName; } __declspec(property(get = __property_get_MainName)) UnicodeString MainName;
  bool __property_get_IsParallelFileTransfer() const { return FIsParallelFileTransfer; } __declspec(property(get = __property_get_IsParallelFileTransfer)) bool IsParallelFileTransfer;

private:
  struct TDirectoryData
  {
    UnicodeString OppositePath;
    bool Exists;
  };

  std::unique_ptr<TStrings> FFileList;
  int FListIndex;
  int FIndex;
  typedef std::map<UnicodeString, TDirectoryData> TDirectories;
  TDirectories FDirectories;
  UnicodeString FTargetDir;
  const TCopyParamType * FCopyParam;
  int FParams;
  bool FProbablyEmpty;
  int FClients;
  std::unique_ptr<TCriticalSection> FSection;
  TFileOperationProgressType * FMainOperationProgress;
  TOperationSide FSide;
  UnicodeString FMainName;
  int FVersion;
  bool FIsParallelFileTransfer;
  __int64 FParallelFileSize;
  __int64 FParallelFileOffset;
  int FParallelFileCount;
  UnicodeString FParallelFileTargetName;
  typedef std::vector<__int64> TParallelFileOffsets;
  TParallelFileOffsets FParallelFileOffsets;
  std::vector<bool> FParallelFileDones;
  bool FParallelFileMerging;
  int FParallelFileMerged;

  bool CheckEnd(TCollectedFileList * Files);
  TCollectedFileList * GetFileList(int Index);
};
//---------------------------------------------------------------------------
struct TLocalFileHandle
{
  TLocalFileHandle();
  ~TLocalFileHandle();

  void Dismiss();
  void Close();
  void Release();

  UnicodeString FileName;
  HANDLE Handle;
  int Attrs;
  bool Directory;
  TDateTime Modification;
  __int64 MTime;
  __int64 ATime;
  __int64 Size;
};
//---------------------------------------------------------------------------
class TLocalFile : public TObject
{
public:
  TSearchRecSmart SearchRec;
};
//---------------------------------------------------------------------------
#endif
