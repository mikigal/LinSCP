// Monitoring of local directories for changes, with inotify.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include "DiscMon.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <vector>

#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace Discmon
{
//---------------------------------------------------------------------------
namespace
{
// Tags of the directories while rescanning
TObject * const TagKept = reinterpret_cast<TObject *>(0);
TObject * const TagRemoved = reinterpret_cast<TObject *>(1);
TObject * const TagAdded = reinterpret_cast<TObject *>(2);
//---------------------------------------------------------------------------
// Adds the subdirectories of the directory, recursively. With Tag, the
// directories that are already in the list are tagged as kept, the new ones
// as added.
void ScanDirectory(
  TStrings * Dirs, const UnicodeString & ADirectory, int & MaxDirectories, TDiscMonitorFilter OnFilter,
  TDiscMonitorTooManyDirectories OnTooManyDirectories, bool Tag)
{
  UnicodeString Directory = IncludeTrailingBackslash(ADirectory);
  TSearchRec SearchRec;
  int FindAttrs = faReadOnly | faHidden | faSysFile | faDirectory | faArchive;
  bool Found = (FindFirst(Directory + L"*", FindAttrs, SearchRec) == 0);
  if (Found)
  {
    try
    {
      while (Found)
      {
        // Links to directories are not followed (they may make cycles)
        if ((SearchRec.Name != L".") && (SearchRec.Name != L"..") &&
            FLAGSET(SearchRec.Attr, faDirectory) && FLAGCLEAR(SearchRec.Attr, faSymLink))
        {
          UnicodeString FileName = Directory + SearchRec.Name;
          bool Add = true;
          if (OnFilter != nullptr)
          {
            OnFilter(nullptr, FileName, Add);
          }
          if (Add)
          {
            int Index = Tag ? Dirs->IndexOf(FileName) : -1;
            if (Index >= 0)
            {
              DebugAssert(Dirs->Objects[Index] == TagRemoved);
              Dirs->Objects[Index] = TagKept;
            }
            else
            {
              Dirs->AddObject(FileName, Tag ? TagAdded : TagKept);
              if ((MaxDirectories >= 0) && (Dirs->Count > MaxDirectories) &&
                  (OnTooManyDirectories != nullptr))
              {
                OnTooManyDirectories(nullptr, MaxDirectories);
              }
            }
            ScanDirectory(Dirs, FileName, MaxDirectories, OnFilter, OnTooManyDirectories, Tag);
          }
        }
        Found = (FindNext(SearchRec) == 0);
      }
    }
    catch (...)
    {
      FindClose(SearchRec);
      throw;
    }
    FindClose(SearchRec);
  }
}
} // namespace
//---------------------------------------------------------------------------
TDiscMonitor::TDiscMonitor(TComponent * AOwner) :
  TComponent(AOwner),
  FFilters(TMonitorFilters() << moFilename),
  FSubTree(true),
  FActive(false),
  FChangeDelay(500),
  FMaxDirectories(0),
  FInotify(-1),
  FStopEvent(-1),
  FStopping(false),
  FFinished(true),
  FSubdirsChanged(false),
  FNotifiedDirectories(0)
{
  FDirectories = new TStringList();
  FDirectories->CaseSensitive = true;
  FDirectories->Sorted = true;
}
//---------------------------------------------------------------------------
TDiscMonitor::~TDiscMonitor()
{
  Stop();
  WaitForThread();
  delete FDirectories;
}
//---------------------------------------------------------------------------
void TDiscMonitor::SetFilters(TMonitorFilters Value)
{
  if (Value.Empty())
  {
    throw Exception(L"Some filter condition must be set.");
  }
  FFilters = Value;
}
//---------------------------------------------------------------------------
void TDiscMonitor::SetSubTree(bool Value)
{
  // Each directory is watched separately
  DebugAssert(!Value);
  FSubTree = Value;
}
//---------------------------------------------------------------------------
void TDiscMonitor::SetActive(bool Value)
{
  if (Value != FActive)
  {
    if (Value)
    {
      // A previous thread may be finishing after Close
      WaitForThread();
      FInotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
      FStopEvent = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
      if ((FInotify < 0) || (FStopEvent < 0))
      {
        DWORD Error = rtl::ErrnoToWinError(errno);
        WaitForThread();
        RaiseLastOSError(Error);
      }
      FStopping = false;
      FFinished = false;
      FActive = true;
      FThread = std::thread(&TDiscMonitor::Execute, this);
    }
    else
    {
      FActive = false;
      Stop();
    }
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::Open()
{
  Active = true;
}
//---------------------------------------------------------------------------
void TDiscMonitor::Close()
{
  Active = false;
}
//---------------------------------------------------------------------------
void TDiscMonitor::AddDirectory(const UnicodeString & Directory, bool SubDirs)
{
  DebugAssert(!FActive);
  if (!Directory.IsEmpty())
  {
    std::unique_ptr<TStringList> Dirs(new TStringList());
    Dirs->Assign(FDirectories);
    Dirs->Add(Directory);
    if (SubDirs)
    {
      ScanDirectory(Dirs.get(), Directory, FMaxDirectories, FOnFilter, FOnTooManyDirectories, false);
    }
    FDirectories->Assign(Dirs.get());
  }
  else
  {
    FDirectories->Clear();
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::SetDirectory(const UnicodeString & Directory)
{
  FDirectories->Clear();
  AddDirectory(Directory, false);
}
//---------------------------------------------------------------------------
void TDiscMonitor::Stop()
{
  FStopping = true;
  if (FStopEvent >= 0)
  {
    eventfd_write(FStopEvent, 1);
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::WaitForThread()
{
  if (FThread.joinable())
  {
    // The thread may wait for the main thread to run its request
    while (!FFinished)
    {
      CheckSynchronize(10);
    }
    FThread.join();
  }
  if (FInotify >= 0)
  {
    close(FInotify);
    FInotify = -1;
  }
  if (FStopEvent >= 0)
  {
    close(FStopEvent);
    FStopEvent = -1;
  }
  FWatches.clear();
}
//---------------------------------------------------------------------------
bool TDiscMonitor::Watch(const UnicodeString & Directory)
{
  uint32_t Mask = IN_ONLYDIR;
  if (FFilters.Contains(moFilename) || FFilters.Contains(moDirName))
  {
    Mask |= IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO;
  }
  if (FFilters.Contains(moAttributes) || FFilters.Contains(moSecurity))
  {
    Mask |= IN_ATTRIB;
  }
  if (FFilters.Contains(moSize) || FFilters.Contains(moLastWrite))
  {
    Mask |= IN_MODIFY | IN_CLOSE_WRITE;
  }
  int Descriptor = inotify_add_watch(FInotify, rtl::FileNameToBytes(ApiPath(Directory)).c_str(), Mask);
  bool Result = (Descriptor >= 0);
  if (Result)
  {
    FWatches[Descriptor] = Directory;
  }
  return Result;
}
//---------------------------------------------------------------------------
void TDiscMonitor::Unwatch(const UnicodeString & Directory)
{
  for (auto I = FWatches.begin(); I != FWatches.end(); ++I)
  {
    if (I->second == Directory)
    {
      inotify_rm_watch(FInotify, I->first);
      FWatches.erase(I);
      break;
    }
  }
}
//---------------------------------------------------------------------------
bool TDiscMonitor::UpdateSubdirectories(const UnicodeString & Directory)
{
  UnicodeString Path = ExcludeTrailingBackslash(Directory);
  UnicodeString Prefix = IncludeTrailingBackslash(Path);
  for (int Index = 0; Index < FDirectories->Count; Index++)
  {
    if (StartsStr(Prefix, FDirectories->Strings[Index]))
    {
      FDirectories->Objects[Index] = TagRemoved;
    }
  }

  int MaxDirectories = -1;
  ScanDirectory(FDirectories, Path, MaxDirectories, FOnFilter, nullptr, true);

  bool Changed = false;
  int Index = 0;
  while (Index < FDirectories->Count)
  {
    UnicodeString Subdirectory = FDirectories->Strings[Index];
    if (FDirectories->Objects[Index] == TagRemoved)
    {
      Unwatch(Subdirectory);
      FDirectories->Delete(Index);
      Changed = true;
    }
    else
    {
      if (FDirectories->Objects[Index] == TagAdded)
      {
        if (!Watch(Subdirectory))
        {
          Invalid(Subdirectory, errno);
          return false;
        }
        FDirectories->Objects[Index] = TagKept;
        Changed = true;
      }
      Index++;
    }
  }

  if (Changed && (FOnDirectoriesChange != nullptr))
  {
    FNotifiedDirectories = FDirectories->Count;
    DoSynchronize(::rtl::MethodPtr(this, &TDiscMonitor::InformDirectoriesChange));
  }
  return true;
}
//---------------------------------------------------------------------------
void TDiscMonitor::Invalid(const UnicodeString & Directory, int Error)
{
  FNotifiedDirectory = Directory;
  FInvalidMessage = SysErrorMessage(rtl::ErrnoToWinError(Error));
  DoSynchronize(::rtl::MethodPtr(this, &TDiscMonitor::InformInvalid));
}
//---------------------------------------------------------------------------
void TDiscMonitor::DoSynchronize(TThreadMethod Method)
{
  if (FOnSynchronize != nullptr)
  {
    FOnSynchronize(this, Method);
  }
  else
  {
    TThread::Synchronize(nullptr, Method);
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::InformChange()
{
  if (!FStopping && (FOnChange != nullptr))
  {
    FOnChange(this, FNotifiedDirectory, FSubdirsChanged);
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::InformInvalid()
{
  if (!FStopping && (FOnInvalid != nullptr))
  {
    FOnInvalid(this, FNotifiedDirectory, FInvalidMessage);
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::InformDirectoriesChange()
{
  if (!FStopping && (FOnDirectoriesChange != nullptr))
  {
    FOnDirectoriesChange(this, FNotifiedDirectories);
  }
}
//---------------------------------------------------------------------------
void TDiscMonitor::Execute()
{
  try
  {
    bool Valid = true;
    for (int Index = 0; Valid && (Index < FDirectories->Count); Index++)
    {
      Valid = Watch(FDirectories->Strings[Index]);
      if (!Valid)
      {
        Invalid(FDirectories->Strings[Index], errno);
      }
    }

    pollfd Fds[2];
    Fds[0].fd = FStopEvent;
    Fds[0].events = POLLIN;
    Fds[1].fd = FInotify;
    Fds[1].events = POLLIN;
    while (Valid && !FStopping)
    {
      if (poll(Fds, 2, -1) < 0)
      {
        if (errno != EINTR)
        {
          Invalid(UnicodeString(), errno);
          Valid = false;
        }
        continue;
      }
      if (FStopping || FLAGSET(Fds[0].revents, POLLIN))
      {
        break;
      }

      // Let the changes settle; the changes made meanwhile are reported together
      if ((FChangeDelay > 0) && (poll(Fds, 1, FChangeDelay) > 0))
      {
        break;
      }

      std::vector<UnicodeString> Changed;
      bool AllChanged = false;
      alignas(inotify_event) char Buffer[16 * 1024];
      ssize_t Length;
      while ((Length = read(FInotify, Buffer, sizeof(Buffer))) > 0)
      {
        for (char * P = Buffer; P < Buffer + Length; )
        {
          const inotify_event * Event = reinterpret_cast<const inotify_event *>(P);
          if (FLAGSET(Event->mask, IN_Q_OVERFLOW))
          {
            AllChanged = true;
          }
          else if (FLAGSET(Event->mask, IN_IGNORED))
          {
            // The directory is gone; its parent reports it
            FWatches.erase(Event->wd);
          }
          else
          {
            auto I = FWatches.find(Event->wd);
            if ((I != FWatches.end()) &&
                (std::find(Changed.begin(), Changed.end(), I->second) == Changed.end()))
            {
              Changed.push_back(I->second);
            }
          }
          P += sizeof(inotify_event) + Event->len;
        }
      }

      if (AllChanged)
      {
        Changed.clear();
        for (int Index = 0; Index < FDirectories->Count; Index++)
        {
          Changed.push_back(FDirectories->Strings[Index]);
        }
      }

      for (size_t Index = 0; Valid && !FStopping && (Index < Changed.size()); Index++)
      {
        // When deleting a tree, the change of a directory may be reported
        // before its deletion
        if (DirectoryExists(ApiPath(Changed[Index])))
        {
          FNotifiedDirectory = Changed[Index];
          FSubdirsChanged = false;
          DoSynchronize(::rtl::MethodPtr(this, &TDiscMonitor::InformChange));
          if (FSubdirsChanged && !FStopping)
          {
            Valid = UpdateSubdirectories(Changed[Index]);
          }
        }
      }
    }
  }
  catch (...)
  {
    // An exception of a handler ends the monitoring, as in Delphi's TThread
  }
  FFinished = true;
}
//---------------------------------------------------------------------------
} // namespace Discmon
//---------------------------------------------------------------------------
