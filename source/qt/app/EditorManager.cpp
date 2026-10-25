// The files opened or edited from the remote panel.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <AppConfiguration.h>
#include <CoreMain.h>
#include <SessionData.h>

#include "EditorManager.h"

#include <cerrno>

#include <signal.h>
//---------------------------------------------------------------------------
TEditedFileData::TEditedFileData()
{
  ForceText = false;
  Terminal = NULL;
  SessionData = NULL;
}
//---------------------------------------------------------------------------
TEditedFileData::~TEditedFileData()
{
  delete SessionData;
}
//---------------------------------------------------------------------------
TEditorManager::TEditorManager()
{
}
//---------------------------------------------------------------------------
TEditorManager::~TEditorManager()
{
  for (size_t I = FFiles.size(); I > 0; I--)
  {
    int Index = static_cast<int>(I) - 1;
    TFileData * FileData = &FFiles[Index];

    // pending should be only external editors and files being uploaded
    DebugAssert(FileData->Closed || FileData->External);

    if (!FileData->Closed)
    {
      if (!CloseFile(Index, true, true))
      {
        ReleaseFile(Index);
      }
    }
    else
    {
      // OpenSCP: still being uploaded (upstream leaves them)
      if (FileData->UploadCompleteEvent != INVALID_HANDLE_VALUE)
      {
        CloseHandle(FileData->UploadCompleteEvent);
        FileData->UploadCompleteEvent = INVALID_HANDLE_VALUE;
      }
      ReleaseFile(Index);
    }
  }
}
//---------------------------------------------------------------------------
bool TEditorManager::Empty(bool IgnoreClosed)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  bool Result;

  if (!IgnoreClosed)
  {
    Result = FFiles.empty();
  }
  else
  {
    Result = true;
    for (const TFileData & FileData : FFiles)
    {
      if (!FileData.Closed)
      {
        Result = false;
        break;
      }
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TEditorManager::CanAddFile(const UnicodeString & RemoteDirectory,
  const UnicodeString & OriginalFileName, const UnicodeString & SessionName,
  void *& Token, UnicodeString & ExistingLocalRootDirectory,
  UnicodeString & ExistingLocalDirectory)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  bool Result = true;

  Token = NULL;
  for (size_t I = 0; I < FFiles.size(); I++)
  {
    TFileData * FileData = &FFiles[I];

    // include even "closed" (=being uploaded) files as it is nonsense
    // to download file being uploaded
    if ((FileData->Data->RemoteDirectory == RemoteDirectory) &&
        (FileData->Data->OriginalFileName == OriginalFileName) &&
        (FileData->Data->SessionName == SessionName))
    {
      if (!FileData->External)
      {
        Result = false;
        if (!FileData->Closed && (FileData->Token != NULL))
        {
          Token = FileData->Token;
        }
      }
      else
      {
        // MDI editor?
        if (FileData->Process == 0)
        {
          // file is just being uploaded, do not allow new editor instance
          if (FileData->Closed)
          {
            Result = false;
          }
          else
          {
            UnicodeString AExistingLocalDirectory = ExtractFilePath(FileData->FileName);
            // If the temporary directory was deleted, behave like the file was never opened
            if (DirectoryExists(ApiPath(AExistingLocalDirectory)))
            {
              // get directory where the file already is so we download it there again
              ExistingLocalRootDirectory = FileData->Data->LocalRootDirectory;
              ExistingLocalDirectory = AExistingLocalDirectory;
            }
            CloseFile(static_cast<int>(I), false, false); // do not delete file
            Result = true;
          }
        }
        else
        {
          Result = false;
        }
      }
      break;
    }
  }

  if (Result)
  {
    if (static_cast<int>(FFiles.size()) >= AppConfiguration->Editor.MaxEditors)
    {
      throw Exception(LoadStr(TOO_MANY_EDITORS));
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
TEditedFileData * TEditorManager::FindByUploadCompleteEvent(HANDLE UploadCompleteEvent)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  for (TFileData & FileData : FFiles)
  {
    if (FileData.UploadCompleteEvent == UploadCompleteEvent)
    {
      return FileData.Data;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
void TEditorManager::ProcessFiles(TEditedFileProcessEvent Callback)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  for (size_t I = 0; I < FFiles.size(); I++)
  {
    TFileData * FileData = &FFiles[I];
    Callback(FileData->FileName, FileData->Data, (FileData->Closed ? NULL : FileData->Token));
  }
}
//---------------------------------------------------------------------------
bool TEditorManager::CloseInternalEditors(std::function<void(void * Token)> CloseCallback)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  // Traverse from end, as closing internal editor causes deletion of
  // respective file vector element.
  void * PrevToken = NULL;
  for (size_t I = FFiles.size(); I > 0; I--)
  {
    // Note that element may be deleted by external cause (like if external editor
    // is closed while "save confirmation" message is displayed).
    if (I <= FFiles.size())
    {
      TFileData * FileData = &FFiles[I - 1];
      // PrevToken is simple check not to ask same editor twice, however
      // it does not solve all posibilities
      if (!FileData->Closed && (FileData->Token != NULL) &&
          (FileData->Token != PrevToken))
      {
        PrevToken = FileData->Token;
        CloseCallback(FileData->Token);
      }
    }
  }

  bool Result = true;
  for (const TFileData & FileData : FFiles)
  {
    if (!FileData.Closed && (FileData.Token != NULL))
    {
      Result = false;
      break;
    }
  }

  return Result;
}
//---------------------------------------------------------------------------
bool TEditorManager::CloseExternalFilesWithoutProcess()
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  for (size_t I = FFiles.size(); I > 0; I--)
  {
    TFileData * FileData = &FFiles[I - 1];

    if (!FileData->Closed && FileData->External && (FileData->Process == 0))
    {
      CloseFile(static_cast<int>(I) - 1, true, true);
    }
  }
  return true;
}
//---------------------------------------------------------------------------
void TEditorManager::AddFileInternal(const UnicodeString & FileName, TEditedFileData * AData, void * Token)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  std::unique_ptr<TEditedFileData> Data(AData);
  TFileData FileData;
  FileData.FileName = FileName;
  FileData.External = false;
  FileData.Process = 0;
  FileData.Token = Token;

  AddFile(FileData, Data.release());
}
//---------------------------------------------------------------------------
void TEditorManager::AddFileExternal(const UnicodeString & FileName, TEditedFileData * AData, qint64 Process)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  std::unique_ptr<TEditedFileData> Data(AData);
  TFileData FileData;
  FileData.FileName = FileName;
  FileData.External = true;
  FileData.Process = Process;
  FileData.Token = NULL;
  AddFile(FileData, Data.release());
}
//---------------------------------------------------------------------------
int TEditorManager::FindFinishedProcess()
{
  for (size_t I = 0; I < FFiles.size(); I++)
  {
    // The editor is not our child (detached), its end is when it is gone
    qint64 Process = FFiles[I].Process;
    if ((Process > 0) && (kill(static_cast<pid_t>(Process), 0) != 0) && (errno == ESRCH))
    {
      return static_cast<int>(I);
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
int TEditorManager::FindCompletedUpload()
{
  for (size_t I = 0; I < FFiles.size(); I++)
  {
    HANDLE Event = FFiles[I].UploadCompleteEvent;
    if ((Event != INVALID_HANDLE_VALUE) && (WaitForSingleObject(Event, 0) == WAIT_OBJECT_0))
    {
      return static_cast<int>(I);
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TEditorManager::Check()
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  int Index;

  for (Index = 0; Index < static_cast<int>(FFiles.size()); Index++)
  {
    TDateTime NewTimestamp;
    if (HasFileChanged(Index, NewTimestamp))
    {
      TDateTime N = NormalizeTimestamp(Now());
      // Let the editor finish writing to the file
      // (first to avoid uploading partially saved file, second
      // because the timestamp may change more than once during saving).
      // WORKAROUND WithinPastMilliSeconds seems buggy that it return true even if NewTimestamp is within future
      if ((NewTimestamp <= N) &&
          !WithinPastMilliSeconds(N, NewTimestamp, GUIConfiguration->KeepUpToDateChangeDelay))
      {
        CheckFileChange(Index, false);
      }
    }
  }

  while ((Index = FindFinishedProcess()) >= 0)
  {
    ::rtl::TFinally Finally([&]()
    {
      if (!EarlyClose(Index))
      {
        // CheckFileChange may fail,
        // but we want to close handles anyway
        CloseFile(Index, false, true);
      }
    });
    CheckFileChange(Index, false);
  }

  while ((Index = FindCompletedUpload()) >= 0)
  {
    UploadComplete(Index, false);
  }
}
//---------------------------------------------------------------------------
bool TEditorManager::EarlyClose(int Index)
{
  TFileData * FileData = &FFiles[Index];

  bool Result =
    (FileData->Process != 0) &&
    (Now() - FileData->Opened <=
      TDateTime(0, 0, static_cast<unsigned short>(AppConfiguration->Editor.EarlyClose), 0)) &&
    OnFileEarlyClosed;

  if (Result)
  {
    Result = false;
    OnFileEarlyClosed(FileData->Data, Result);
    if (Result)
    {
      // forget the associated process
      CloseProcess(Index);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void TEditorManager::FileChanged(void * Token)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  int Index = FindFile(Token);

  DebugAssert(Index >= 0);
  DebugAssert(!FFiles[Index].External);

  CheckFileChange(Index, true);
}
//---------------------------------------------------------------------------
void TEditorManager::FileReload(void * Token)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  int Index = FindFile(Token);

  DebugAssert(Index >= 0);
  TFileData * FileData = &FFiles[Index];
  DebugAssert(!FileData->External);
  TAutoFlag ReloadingFlag(FileData->Reloading);

  OnFileReload(FileData->FileName, FileData->Data);
  GetFileTimestamp(FileData->FileName, FileData->Timestamp);
}
//---------------------------------------------------------------------------
void TEditorManager::FileClosed(void * Token, bool Forced)
{
  std::lock_guard<std::recursive_mutex> Guard(Section);
  int Index = FindFile(Token);

  DebugAssert(Index >= 0);
  DebugAssert(!FFiles[Index].External);

  CheckFileChange(Index, false);
  CloseFile(Index, false, !Forced);
}
//---------------------------------------------------------------------------
void TEditorManager::AddFile(TFileData & FileData, TEditedFileData * AData)
{
  std::unique_ptr<TEditedFileData> Data(AData);

  GetFileTimestamp(FileData.FileName, FileData.Timestamp);
  FileData.Closed = false;
  FileData.UploadCompleteEvent = INVALID_HANDLE_VALUE;
  FileData.Opened = Now();
  FileData.Reupload = false;
  FileData.Reloading = false;
  FileData.Saves = 0;
  FileData.Data = Data.get();

  FFiles.push_back(FileData);

  Data.release(); // ownership passed
}
//---------------------------------------------------------------------------
void TEditorManager::UploadComplete(int Index, bool Retry)
{
  TFileData * FileData = &FFiles[Index];

  DebugAssert(FileData->UploadCompleteEvent != INVALID_HANDLE_VALUE);

  CloseHandle(FileData->UploadCompleteEvent);
  FileData->UploadCompleteEvent = INVALID_HANDLE_VALUE;

  if (FileData->Closed)
  {
    CloseFile(Index, false, true);
  }
  else
  {
    if (FileData->Reupload)
    {
      FileData->Reupload = false;
      CheckFileChange(Index, true);
    }
    // so far used only to signal to the internal editor that saving was complete,
    // so we do signal once an actual upload is done only
    else if ((FileData->Token != NULL) && OnFileUploadComplete && !Retry)
    {
      OnFileUploadComplete(FileData->Token);
    }
  }
}
//---------------------------------------------------------------------------
void TEditorManager::CloseProcess(int Index)
{
  TFileData * FileData = &FFiles[Index];

  // The editor keeps running, if it still does
  FileData->Process = 0;
}
//---------------------------------------------------------------------------
void TEditorManager::ReleaseFile(int Index)
{
  TFileData * FileData = &FFiles[Index];
  delete FileData->Data;
  FileData->Data = NULL;
}
//---------------------------------------------------------------------------
bool TEditorManager::CloseFile(int Index, bool IgnoreErrors, bool Delete)
{
  bool Result = false;
  TFileData * FileData = &FFiles[Index];

  if (FileData->Process != 0)
  {
    CloseProcess(Index);
  }

  if (FileData->UploadCompleteEvent != INVALID_HANDLE_VALUE)
  {
    FileData->Closed = true;
    AppLogFmt(L"Opened/edited file \"%s\" has been closed, but the file is still being uploaded.", (FileData->FileName));
  }
  else
  {
    UnicodeString LocalRootDirectory = FileData->Data->LocalRootDirectory;
    UnicodeString FileName = FileData->FileName; // Before it's released

    ReleaseFile(Index);
    FFiles.erase(FFiles.begin() + Index);
    Result = true;

    if (Delete && !LocalRootDirectory.IsEmpty())
    {
      if (!RecursiveDeleteFile(ExcludeTrailingBackslash(LocalRootDirectory)) &&
          !IgnoreErrors)
      {
        throw Exception(FMTLOAD(DELETE_TEMP_EXECUTE_FILE_ERROR, (LocalRootDirectory)));
      }
      AppLogFmt(L"Deleted opened/edited file [%s] folder \"%s\".", (FileName, LocalRootDirectory));
    }
    else
    {
      AppLogFmt(L"Opened/edited file \"%s\" has been closed.", (FileName));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TEditorManager::NormalizeTimestamp(const TDateTime & Timestamp)
{
  return TTimeZone::Local->ToUniversalTime(Timestamp);
}
//---------------------------------------------------------------------------
bool TEditorManager::GetFileTimestamp(const UnicodeString & FileName, TDateTime & Timestamp)
{
  TSearchRecSmart ASearchRec;
  bool Result = FileSearchRec(FileName, ASearchRec);
  if (Result)
  {
    Timestamp = NormalizeTimestamp(ASearchRec.GetLastWriteTime());
  }
  return Result;
}
//---------------------------------------------------------------------------
bool TEditorManager::HasFileChanged(int Index, TDateTime & NewTimestamp)
{
  TFileData * FileData = &FFiles[Index];
  bool Result;
  if (FileData->Reloading)
  {
    Result = false;
  }
  else
  {
    Result =
      GetFileTimestamp(FileData->FileName, NewTimestamp) &&
      (FileData->Timestamp != NewTimestamp);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TEditorManager::CheckFileChange(int Index, bool Force)
{
  TDateTime NewTimestamp;
  bool Changed = HasFileChanged(Index, NewTimestamp);
  if (Force || Changed)
  {
    TFileData * FileData = &FFiles[Index];
    if (FileData->UploadCompleteEvent != INVALID_HANDLE_VALUE)
    {
      AppLogFmt(L"Opened/edited file \"%s\" was changed, but it still being uploaded.", (FileData->FileName));
      FileData->Reupload = true;
    }
    else
    {
      if (Changed)
      {
        AppLogFmt(L"Opened/edited file \"%s\" was changed.", (FileData->FileName));
      }
      else
      {
        AppLogFmt(L"Forcing upload of opened/edited file \"%s\".", (FileData->FileName));
      }
      // OpenSCP: upstream delays the upload of a file locked by its editor;
      // files are not locked on Linux
      FileData->UploadCompleteEvent = CreateEvent(NULL, false, false, NULL);

      TDateTime PrevTimestamp = FileData->Timestamp;
      FileData->Timestamp = NewTimestamp;
      FileData->Saves++;
      if (FileData->Saves == 1)
      {
        Configuration->Usage->Inc(L"RemoteFilesSaved");
      }
      Configuration->Usage->Inc(L"RemoteFileSaves");

      try
      {
        DebugAssert(OnFileChange);
        bool Retry = false;
        AppLogFmt(L"Uploading opened/edited file \"%s\".", (FileData->FileName));
        OnFileChange(FileData->FileName, FileData->Timestamp, FileData->Data, FileData->UploadCompleteEvent, Retry);
        // The handler may have closed the file meanwhile
        FileData = &FFiles[Index];
        if (Retry)
        {
          AppLogFmt(L"Will retry uploading opened/edited file \"%s\".", (FileData->FileName));
          UploadComplete(Index, true);
          FileData->Timestamp = PrevTimestamp;
        }
      }
      catch (...)
      {
        AppLogFmt(L"Failed to upload opened/edited file \"%s\".", (FileData->FileName));
        // upload failed (was not even started)
        if (FileData->UploadCompleteEvent != INVALID_HANDLE_VALUE)
        {
          UploadComplete(Index, false);
        }
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
int TEditorManager::FindFile(const void * Token)
{
  for (size_t I = 0; I < FFiles.size(); I++)
  {
    if (!FFiles[I].Closed && (FFiles[I].Token == Token))
    {
      return static_cast<int>(I);
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
