//---------------------------------------------------------------------------
#ifndef SynchronizeControllerH
#define SynchronizeControllerH
//---------------------------------------------------------------------------
#include <CopyParam.h>
//---------------------------------------------------------------------------
struct TSynchronizeParamType
{
  UnicodeString LocalDirectory;
  UnicodeString RemoteDirectory;
  int Params;
  int Options;
};
//---------------------------------------------------------------------------
class TSynchronizeController;
struct TSynchronizeOptions;
class TSynchronizeChecklist;
typedef TMethodPtr<void(System::TObject * Sender, bool Close)>
 TSynchronizeAbortEvent;
typedef TMethodPtr<void(TObject* Sender, TThreadMethod Method)>
 TSynchronizeThreadsEvent;
enum TSynchronizeLogEntry : int { slScan, slStart, slChange, slUpload, slDelete, slDirChange, slContinuedError };
typedef TMethodPtr<void(TSynchronizeController * Controller, TSynchronizeLogEntry Entry, const UnicodeString Message)>
 TSynchronizeLog;
typedef TMethodPtr<void(System::TObject * Sender, bool Start, const TSynchronizeParamType & Params,
   const TCopyParamType & CopyParam, TSynchronizeOptions * Options,
   TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
   TSynchronizeLog OnSynchronizeLog)>
 TSynchronizeStartStopEvent;
typedef TMethodPtr<void(TSynchronizeController * Sender, const UnicodeString LocalDirectory,
   const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
   const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
   TSynchronizeOptions * Options, bool Full)>
 TSynchronizeEvent;
typedef TMethodPtr<void(TSynchronizeController * Sender, const UnicodeString Directory, const UnicodeString ErrorStr)>
 TSynchronizeInvalidEvent;
typedef TMethodPtr<void(TSynchronizeController * Sender, int & MaxDirectories)>
 TSynchronizeTooManyDirectories;
//---------------------------------------------------------------------------
namespace Discmon
{
class TDiscMonitor;
}
//---------------------------------------------------------------------------
enum TSynchronizeOperation : int { soUpload, soDelete };
//---------------------------------------------------------------------------
class TSynchronizeController
{
public:
  __fastcall TSynchronizeController(TSynchronizeEvent AOnSynchronize,
    TSynchronizeInvalidEvent AOnSynchronizeInvalid,
    TSynchronizeTooManyDirectories AOnTooManyDirectories);
  __fastcall ~TSynchronizeController();

  void __fastcall StartStop(TObject * Sender, bool Start,
    const TSynchronizeParamType & Params, const TCopyParamType & CopyParam,
    TSynchronizeOptions * Options,
    TSynchronizeAbortEvent OnAbort, TSynchronizeThreadsEvent OnSynchronizeThreads,
    TSynchronizeLog OnSynchronizeLog);
  void __fastcall LogOperation(TSynchronizeOperation Operation, const UnicodeString FileName);

private:
  TSynchronizeEvent FOnSynchronize;
  TSynchronizeParamType FSynchronizeParams;
  TSynchronizeOptions * FOptions;
  Discmon::TDiscMonitor * FSynchronizeMonitor;
  TSynchronizeAbortEvent FSynchronizeAbort;
  TSynchronizeInvalidEvent FOnSynchronizeInvalid;
  TSynchronizeTooManyDirectories FOnTooManyDirectories;
  TSynchronizeLog FSynchronizeLog;
  TCopyParamType FCopyParam;

  void __fastcall SynchronizeChange(TObject * Sender, const UnicodeString Directory,
    bool & SubdirsChanged);
  void __fastcall SynchronizeAbort(bool Close);
  void __fastcall SynchronizeLog(TSynchronizeLogEntry Entry, const UnicodeString Message);
  void __fastcall SynchronizeInvalid(TObject * Sender, const UnicodeString Directory,
    const UnicodeString ErrorStr);
  void __fastcall SynchronizeFilter(TObject * Sender, const UnicodeString DirectoryName,
    bool & Add);
  void __fastcall SynchronizeTooManyDirectories(TObject * Sender, int & MaxDirectories);
  void __fastcall SynchronizeDirectoriesChange(TObject * Sender, int Directories);
};
//---------------------------------------------------------------------------
void __fastcall LogSynchronizeEvent(TTerminal * Terminal, const UnicodeString & Message);
//---------------------------------------------------------------------------
#endif
