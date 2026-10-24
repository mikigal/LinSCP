//---------------------------------------------------------------------------
#ifndef ScriptH
#define ScriptH
//---------------------------------------------------------------------------
#include <time.h>
#include "Option.h"
#include "Terminal.h"
#include "FileOperationProgress.h"
//---------------------------------------------------------------------------
class TTerminal;
class TScript;
class TScriptCommands;
class TStoredSessionList;
class TTerminalList;
//---------------------------------------------------------------------------
class TScriptProgress
{
public:
  TFileOperation Operation;
  TOperationSide Side;
  UnicodeString FileName;
  UnicodeString Directory;
  unsigned int OverallProgress;
  unsigned int FileProgress;
  unsigned int CPS;
  bool Cancel;
};
//---------------------------------------------------------------------------
typedef TMethodPtr<void(TScript * Script, const UnicodeString Str, bool Error)> TScriptPrintEvent;
typedef TMethodPtr<void(TScript * Script,
  const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
  const TCopyParamType & CopyParam, int SynchronizeParams)> TScriptSynchronizeStartStop;
typedef TMethodPtr<void(TScript * Script, TScriptProgress & Progress)> TScriptProgressEvent;
//---------------------------------------------------------------------------
class TScriptProcParams : public TOptions
{
public:
  __fastcall TScriptProcParams(const UnicodeString & FullCommand, const UnicodeString & ParamsStr);

  UnicodeString __property_get_ParamsStr() const { return FParamsStr; } __declspec(property(get = __property_get_ParamsStr)) UnicodeString ParamsStr;
  UnicodeString __property_get_FullCommand() const { return FFullCommand; } __declspec(property(get = __property_get_FullCommand)) UnicodeString FullCommand;

private:
  UnicodeString FParamsStr;
  UnicodeString FFullCommand;
};
//---------------------------------------------------------------------------
class TScript
{
public:
  enum TBatchMode : int { BatchOff, BatchOn, BatchAbort, BatchContinue };

  __fastcall TScript(bool LimitedOutput);
  virtual __fastcall ~TScript();

  void __fastcall Command(UnicodeString Cmd);
  void __fastcall Log(TLogLineType Type, const UnicodeString & Str, TTerminal * ATerminal = NULL);
  void __fastcall PrintLine(const UnicodeString Str, bool Error = false, TTerminal * ATerminal = NULL);
  void __fastcall StartInteractive();

  void __fastcall Synchronize(const UnicodeString LocalDirectory,
    const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
    int SynchronizeParams, TSynchronizeChecklist ** Checklist);

  static void __fastcall RequireParams(TScriptProcParams * Parameters, int MinParams);

  TScriptPrintEvent __property_get_OnPrint() const { return FOnPrint; } void __property_set_OnPrint(TScriptPrintEvent Value) { FOnPrint = Value; } __declspec(property(get = __property_get_OnPrint, put = __property_set_OnPrint)) TScriptPrintEvent OnPrint;
  TExtendedExceptionEvent __property_get_OnShowExtendedException() const { return FOnShowExtendedException; } void __property_set_OnShowExtendedException(TExtendedExceptionEvent Value) { FOnShowExtendedException = Value; } __declspec(property(get = __property_get_OnShowExtendedException, put = __property_set_OnShowExtendedException)) TExtendedExceptionEvent OnShowExtendedException;
  TSynchronizeDirectory __property_get_OnTerminalSynchronizeDirectory() const { return FOnTerminalSynchronizeDirectory; } void __property_set_OnTerminalSynchronizeDirectory(TSynchronizeDirectory Value) { FOnTerminalSynchronizeDirectory = Value; } __declspec(property(get = __property_get_OnTerminalSynchronizeDirectory, put = __property_set_OnTerminalSynchronizeDirectory)) TSynchronizeDirectory OnTerminalSynchronizeDirectory;
  TScriptSynchronizeStartStop __property_get_OnSynchronizeStartStop() const { return FOnSynchronizeStartStop; } void __property_set_OnSynchronizeStartStop(TScriptSynchronizeStartStop Value) { FOnSynchronizeStartStop = Value; } __declspec(property(get = __property_get_OnSynchronizeStartStop, put = __property_set_OnSynchronizeStartStop)) TScriptSynchronizeStartStop OnSynchronizeStartStop;
  TScriptProgressEvent __property_get_OnProgress() const { return FOnProgress; } void __property_set_OnProgress(TScriptProgressEvent Value) { FOnProgress = Value; } __declspec(property(get = __property_get_OnProgress, put = __property_set_OnProgress)) TScriptProgressEvent OnProgress;
  TCopyParamType __property_get_CopyParam() const { return FCopyParam; } void __property_set_CopyParam(TCopyParamType Value) { SetCopyParam(Value); } __declspec(property(get = __property_get_CopyParam, put = __property_set_CopyParam)) TCopyParamType CopyParam;
  int __property_get_SynchronizeParams() const { return FSynchronizeParams; } void __property_set_SynchronizeParams(int Value) { SetSynchronizeParams(Value); } __declspec(property(get = __property_get_SynchronizeParams, put = __property_set_SynchronizeParams)) int SynchronizeParams;
  TBatchMode __property_get_Batch() const { return FBatch; } __declspec(property(get = __property_get_Batch)) TBatchMode Batch;
  TTerminal * __property_get_Terminal() const { return FTerminal; } __declspec(property(get = __property_get_Terminal)) TTerminal * Terminal;
  bool __property_get_Groups() const { return FGroups; } void __property_set_Groups(bool Value) { FGroups = Value; } __declspec(property(get = __property_get_Groups, put = __property_set_Groups)) bool Groups;
  bool __property_get_WantsProgress() const { return FWantsProgress; } void __property_set_WantsProgress(bool Value) { FWantsProgress = Value; } __declspec(property(get = __property_get_WantsProgress, put = __property_set_WantsProgress)) bool WantsProgress;
  bool __property_get_UsageWarnings() const { return FUsageWarnings; } void __property_set_UsageWarnings(bool Value) { FUsageWarnings = Value; } __declspec(property(get = __property_get_UsageWarnings, put = __property_set_UsageWarnings)) bool UsageWarnings;
  TTransferOutEvent __property_get_OnTransferOut() const { return FOnTransferOut; } void __property_set_OnTransferOut(TTransferOutEvent Value) { FOnTransferOut = Value; } __declspec(property(get = __property_get_OnTransferOut, put = __property_set_OnTransferOut)) TTransferOutEvent OnTransferOut;
  TTransferInEvent __property_get_OnTransferIn() const { return FOnTransferIn; } void __property_set_OnTransferIn(TTransferInEvent Value) { FOnTransferIn = Value; } __declspec(property(get = __property_get_OnTransferIn, put = __property_set_OnTransferIn)) TTransferInEvent OnTransferIn;

protected:
  TTerminal * FTerminal;
  TTerminal * FLoggingTerminal;
  TScriptCommands * FCommands;
  TScriptPrintEvent FOnPrint;
  TExtendedExceptionEvent FOnShowExtendedException;
  TSynchronizeDirectory FOnTerminalSynchronizeDirectory;
  TScriptSynchronizeStartStop FOnSynchronizeStartStop;
  TScriptProgressEvent FOnProgress;
  TCopyParamType FCopyParam;
  bool FIncludeFileMaskOptionUsed;
  TBatchMode FBatch;
  TBatchMode FInteractiveBatch;
  bool FConfirm;
  bool FInteractiveConfirm;
  bool FEcho;
  bool FFailOnNoMatch;
  int FSynchronizeParams;
  int FSynchronizeMode;
  bool FKeepingUpToDate;
  UnicodeString FSynchronizeIntro;
  bool FLimitedOutput;
  int FSessionReopenTimeout;
  int FInteractiveSessionReopenTimeout;
  bool FGroups;
  bool FWantsProgress;
  bool FUsageWarnings;
  TTransferOutEvent FOnTransferOut;
  TTransferInEvent FOnTransferIn;
  TStrings * FPendingLogLines;
  bool FWarnNonDefaultCopyParam;
  bool FWarnNonDefaultSynchronizeParams;
  bool FPrintInformation;

  virtual void __fastcall ResetTransfer();
  virtual void __fastcall ConnectTerminal(TTerminal * ATerminal);
  bool __fastcall EnsureCommandSessionFallback(
    TFSCapability Capability, TSessionAction * Action);
  void __fastcall Print(const UnicodeString Str, bool Error = false);
  void __fastcall CheckSession();
  void __fastcall CheckParams(TScriptProcParams * Parameters);
  void __fastcall CopyParamParams(TCopyParamType & CopyParam, TScriptProcParams * Parameters);
  void __fastcall TransferParamParams(int & Params, TScriptProcParams * Parameters);
  enum TFileListType : int
  {
    fltDefault =     0x00,
    fltDirectories = 0x01,
    fltQueryServer = 0x02,
    fltMask =        0x04,
    fltLatest =      0x08,
    fltOnlyFile =    0x10,
  };
  TStrings * __fastcall CreateFileList(TScriptProcParams * Parameters, int Start,
    int End, TFileListType ListType = fltDefault);
  TStrings * __fastcall CreateLocalFileList(TScriptProcParams * Parameters,
    int Start, int End, TFileListType ListType);
  void __fastcall FreeFiles(TStrings * FileList);
  void __fastcall FreeFileList(TStrings * FileList);
  void __fastcall LogPendingLines(TTerminal * ATerminal);

  void __fastcall HelpProc(TScriptProcParams * Parameters);
  void __fastcall CallProc(TScriptProcParams * Parameters);
  void __fastcall PwdProc(TScriptProcParams * Parameters);
  void __fastcall CdProc(TScriptProcParams * Parameters);
  void __fastcall LsProc(TScriptProcParams * Parameters);
  void __fastcall RmProc(TScriptProcParams * Parameters);
  void __fastcall RmDirProc(TScriptProcParams * Parameters);
  void __fastcall MvProc(TScriptProcParams * Parameters);
  void __fastcall CpProc(TScriptProcParams * Parameters);
  void __fastcall ChModProc(TScriptProcParams * Parameters);
  void __fastcall LnProc(TScriptProcParams * Parameters);
  void __fastcall MkDirProc(TScriptProcParams * Parameters);
  void __fastcall GetProc(TScriptProcParams * Parameters);
  void __fastcall PutProc(TScriptProcParams * Parameters);
  void __fastcall OptionProc(TScriptProcParams * Parameters);
  void __fastcall AsciiProc(TScriptProcParams * Parameters);
  void __fastcall BinaryProc(TScriptProcParams * Parameters);
  void __fastcall SynchronizeProc(TScriptProcParams * Parameters);
  void __fastcall KeepUpToDateProc(TScriptProcParams * Parameters);
  void __fastcall EchoProc(TScriptProcParams * Parameters);
  void __fastcall StatProc(TScriptProcParams * Parameters);
  void __fastcall ChecksumProc(TScriptProcParams * Parameters);
  void __fastcall CopyIdProc(TScriptProcParams * Parameters);

  void __fastcall OptionImpl(UnicodeString OptionName, UnicodeString ValueName);
  void __fastcall SynchronizeDirectories(TScriptProcParams * Parameters,
    UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory, int FirstParam);
  virtual bool __fastcall HandleExtendedException(Exception * E,
    TTerminal * Terminal = NULL);
  void __fastcall TerminalCaptureLog(const UnicodeString & AddedLine, TCaptureOutputType OutputType);
  virtual UnicodeString __fastcall GetLogCmd(const UnicodeString & FullCommand,
    const UnicodeString & Command, const UnicodeString & Params);
  void __fastcall SynchronizePreview(
    UnicodeString LocalDirectory, UnicodeString RemoteDirectory,
    TSynchronizeChecklist * Checklist);
  UnicodeString __fastcall SynchronizeFileRecord(
    const UnicodeString & RootDirectory, const TSynchronizeChecklist::TItem * Item,
    bool Local);
  UnicodeString __fastcall ListingSysErrorMessage();
  void __fastcall NoMatch(const UnicodeString & Mask, const UnicodeString & Error);
  void __fastcall NoMatch(const UnicodeString & Message);

private:
  void __fastcall Init();
  void __fastcall SetCopyParam(const TCopyParamType & value);
  void __fastcall SetSynchronizeParams(int value);
  TTransferMode __fastcall ParseTransferModeName(UnicodeString Name);
  inline bool __fastcall IsTerminalLogging(TTerminal * ATerminal);
  void __fastcall CheckDefaultCopyParam();
  bool __fastcall HasNonDefaultCopyParams();
  void __fastcall CheckDefaultSynchronizeParams();
  void __fastcall CheckMultiFilesToOne(TStrings * FileList, const UnicodeString & Target, bool Unix);
  void __fastcall LogOption(const UnicodeString & LogStr);
  void __fastcall DoMvOrCp(TScriptProcParams * Parameters, TFSCapability Capability, bool Cp);
  void __fastcall DoCalculatedChecksum(
    const UnicodeString & FileName, const UnicodeString & Alg, const UnicodeString & Hash);
};
//---------------------------------------------------------------------------
typedef TMethodPtr<void(TScript * Script, const UnicodeString Prompt, UnicodeString & Str)> TScriptInputEvent;
typedef TMethodPtr<void(TScript * Script, bool & Cancel)> TScriptQueryCancelEvent;
typedef TMethodPtr<void(TScript * Script, bool First, const UnicodeString Str)> TScriptPrintProgressEvent;
//---------------------------------------------------------------------------
class TManagementScript : public TScript
{
public:
  __fastcall TManagementScript(TStoredSessionList * StoredSessions, bool LimitedOutput);
  virtual __fastcall ~TManagementScript();

  void __fastcall Connect(const UnicodeString Session, TOptions * Options, bool CheckParams);
  void __fastcall ReflectSettings();

  void __fastcall MaskPasswordInCommandLine(UnicodeString & Command, bool Recurse);

  TScriptInputEvent __property_get_OnInput() const { return FOnInput; } void __property_set_OnInput(TScriptInputEvent Value) { FOnInput = Value; } __declspec(property(get = __property_get_OnInput, put = __property_set_OnInput)) TScriptInputEvent OnInput;
  TScriptQueryCancelEvent __property_get_OnQueryCancel() const { return FOnQueryCancel; } void __property_set_OnQueryCancel(TScriptQueryCancelEvent Value) { FOnQueryCancel = Value; } __declspec(property(get = __property_get_OnQueryCancel, put = __property_set_OnQueryCancel)) TScriptQueryCancelEvent OnQueryCancel;
  TPromptUserEvent __property_get_OnTerminalPromptUser() const { return FOnTerminalPromptUser; } void __property_set_OnTerminalPromptUser(TPromptUserEvent Value) { FOnTerminalPromptUser = Value; } __declspec(property(get = __property_get_OnTerminalPromptUser, put = __property_set_OnTerminalPromptUser)) TPromptUserEvent OnTerminalPromptUser;
  TQueryUserEvent __property_get_OnTerminalQueryUser() const { return FOnTerminalQueryUser; } void __property_set_OnTerminalQueryUser(TQueryUserEvent Value) { FOnTerminalQueryUser = Value; } __declspec(property(get = __property_get_OnTerminalQueryUser, put = __property_set_OnTerminalQueryUser)) TQueryUserEvent OnTerminalQueryUser;
  TScriptPrintProgressEvent __property_get_OnPrintProgress() const { return FOnPrintProgress; } void __property_set_OnPrintProgress(TScriptPrintProgressEvent Value) { FOnPrintProgress = Value; } __declspec(property(get = __property_get_OnPrintProgress, put = __property_set_OnPrintProgress)) TScriptPrintProgressEvent OnPrintProgress;
  bool __property_get_Continue() const { return FContinue; } __declspec(property(get = __property_get_Continue)) bool Continue;

protected:
  TScriptInputEvent FOnInput;
  TScriptQueryCancelEvent FOnQueryCancel;
  TPromptUserEvent FOnTerminalPromptUser;
  TQueryUserEvent FOnTerminalQueryUser;
  TScriptPrintProgressEvent FOnPrintProgress;
  TStoredSessionList * FStoredSessions;
  TTerminalList * FTerminalList;
  UnicodeString FLastProgressFile;
  UnicodeString FLastProgressMessage;
  time_t FLastProgressTime;
  time_t FLastProgressEventTime;
  UnicodeString FLastProgressEventDoneFileName;
  bool FLastProgressOverallDone;
  bool FContinue;

  virtual void __fastcall ResetTransfer();
  void __fastcall Input(const UnicodeString Prompt, UnicodeString & Str, bool AllowEmpty);
  void __fastcall TerminalInformation(
    TTerminal * Terminal, const UnicodeString & Str, int Phase, const UnicodeString & Additional);
  void __fastcall TerminalOperationProgress(TFileOperationProgressType & ProgressData);
  void __fastcall TerminalOperationFinished(TFileOperation Operation, TOperationSide Side,
    bool Temp, const UnicodeString & FileName, bool Success, bool NotCancelled,
    TOnceDoneOperation & OnceDoneOperation);

  void __fastcall PrintActiveSession();
  TTerminal * __fastcall FindSession(const UnicodeString Index);
  void __fastcall FreeTerminal(TTerminal * Terminal);
  void __fastcall PrintProgress(bool First, const UnicodeString Str);
  bool __fastcall QueryCancel();
  void __fastcall TerminalSynchronizeDirectory(
    const UnicodeString & LocalDirectory, const UnicodeString & RemoteDirectory,
    bool & Continue, bool Collect, const TSynchronizeOptions * Options);
  void __fastcall DoChangeLocalDirectory(UnicodeString Directory);
  void __fastcall DoClose(TTerminal * Terminal);
  virtual bool __fastcall HandleExtendedException(Exception * E,
    TTerminal * Terminal = NULL);
  void __fastcall TerminalPromptUser(TTerminal * Terminal, TPromptKind Kind,
    UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results, bool & Result, void * Arg);
  void __fastcall TerminalInitializeLog(TObject * Sender);
  inline bool __fastcall Synchronizing();
  inline void __fastcall ShowPendingProgress();
  virtual UnicodeString __fastcall GetLogCmd(const UnicodeString & FullCommand,
    const UnicodeString & Command, const UnicodeString & Params);
  UnicodeString __fastcall MaskPasswordInCommand(const UnicodeString & FullCommand,
    const UnicodeString & Command);

  void __fastcall ExitProc(TScriptProcParams * Parameters);
  void __fastcall OpenProc(TScriptProcParams * Parameters);
  void __fastcall CloseProc(TScriptProcParams * Parameters);
  void __fastcall SessionProc(TScriptProcParams * Parameters);
  void __fastcall LPwdProc(TScriptProcParams * Parameters);
  void __fastcall LCdProc(TScriptProcParams * Parameters);
  void __fastcall LLsProc(TScriptProcParams * Parameters);
};
//---------------------------------------------------------------------------
#define COPYID_COMMAND L"copyid"
#define EXIT_COMMAND L"exit"
//---------------------------------------------------------------------------
#endif
