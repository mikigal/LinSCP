// The files opened or edited from the remote panel (upstream's
// TEditorManager of windows/EditorManager.h): their temporary copies are
// uploaded once they change, and deleted once their editors close. The
// internal editors announce their saves and closing; for external editors
// the files are watched (and their processes, where they are known).
//---------------------------------------------------------------------------
#ifndef EditorManagerH
#define EditorManagerH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <QtGlobal>

#include <functional>
#include <mutex>
#include <vector>
//---------------------------------------------------------------------------
class TSessionData;
class TTerminal;
//---------------------------------------------------------------------------
struct TEditedFileData
{
  TEditedFileData();
  ~TEditedFileData();

  UnicodeString LocalRootDirectory;
  UnicodeString RemoteDirectory;
  bool ForceText;
  // Nulled when the session is closed
  TTerminal * Terminal;
  TSessionData * SessionData;
  UnicodeString SessionName;
  UnicodeString OriginalFileName;
  UnicodeString Command;
  TDateTime SourceTimestamp;
};
//---------------------------------------------------------------------------
typedef std::function<void(const UnicodeString & FileName, const TDateTime & Timestamp, TEditedFileData * Data,
  HANDLE CompleteEvent, bool & Retry)> TEditedFileChangedEvent;
typedef std::function<void(const UnicodeString & FileName, TEditedFileData * Data)> TEditedFileReloadEvent;
typedef std::function<void(const TEditedFileData * Data, bool & KeepOpen)> TEditedFileEarlyClosedEvent;
typedef std::function<void(void * Token)> TEditedFileUploadComplete;
typedef std::function<void(const UnicodeString & FileName, TEditedFileData * Data, void * Token)>
  TEditedFileProcessEvent;
//---------------------------------------------------------------------------
class TEditorManager
{
public:
  TEditorManager();
  ~TEditorManager();

  bool Empty(bool IgnoreClosed);
  bool CanAddFile(const UnicodeString & RemoteDirectory,
    const UnicodeString & OriginalFileName, const UnicodeString & SessionName,
    void *& Token, UnicodeString & ExistingLocalRootDirectory,
    UnicodeString & ExistingLocalDirectory);
  bool CloseInternalEditors(std::function<void(void * Token)> CloseCallback);
  bool CloseExternalFilesWithoutProcess();

  // The internal editor is the token
  void AddFileInternal(const UnicodeString & FileName, TEditedFileData * Data, void * Token);
  // The process of the editor is watched (unless 0, when not known)
  void AddFileExternal(const UnicodeString & FileName, TEditedFileData * Data, qint64 Process);

  void Check();

  void FileChanged(void * Token);
  void FileReload(void * Token);
  void FileClosed(void * Token, bool Forced);

  void ProcessFiles(TEditedFileProcessEvent Callback);
  TEditedFileData * FindByUploadCompleteEvent(HANDLE UploadCompleteEvent);

  TEditedFileChangedEvent OnFileChange;
  TEditedFileReloadEvent OnFileReload;
  TEditedFileEarlyClosedEvent OnFileEarlyClosed;
  TEditedFileUploadComplete OnFileUploadComplete;
  // The queue threads look up the files too
  std::recursive_mutex Section;

private:
  struct TFileData
  {
    UnicodeString FileName;
    bool External;
    // The detached process of the external editor, or 0
    qint64 Process;
    void * Token;
    TDateTime Timestamp;
    TEditedFileData * Data;
    bool Closed;
    HANDLE UploadCompleteEvent;
    TDateTime Opened;
    bool Reupload;
    bool Reloading;
    unsigned int Saves;
  };

  std::vector<TFileData> FFiles;

  void AddFile(TFileData & FileData, TEditedFileData * Data);
  void UploadComplete(int Index, bool Retry);
  bool CloseFile(int Index, bool IgnoreErrors, bool Delete);
  void CloseProcess(int Index);
  bool EarlyClose(int Index);
  bool HasFileChanged(int Index, TDateTime & NewTimestamp);
  void CheckFileChange(int Index, bool Force);
  int FindFile(const void * Token);
  void ReleaseFile(int Index);
  TDateTime NormalizeTimestamp(const TDateTime & Timestamp);
  bool GetFileTimestamp(const UnicodeString & FileName, TDateTime & Timestamp);
  int FindFinishedProcess();
  int FindCompletedUpload();
};
//---------------------------------------------------------------------------
#endif
