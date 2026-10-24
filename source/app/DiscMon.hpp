// Monitoring of local directories for changes: the TDiscMonitor component
// of upstream's packages/my/DiscMon.pas (its interface, for the ported
// SynchronizeController), implemented with inotify.
//
// Each directory is watched separately (no subtrees). A thread waits for
// the changes and, after ChangeDelay, reports each changed directory with
// OnChange in the main thread (through OnSynchronize, by default with
// TThread::Synchronize, so the main thread has to call CheckSynchronize).
// If the handler reports changed subdirectories, the subdirectories of the
// directory are scanned again.
//---------------------------------------------------------------------------
#ifndef DiscMonHPP
#define DiscMonHPP
//---------------------------------------------------------------------------
#include <Classes.hpp>

#include <atomic>
#include <map>
#include <thread>
//---------------------------------------------------------------------------
namespace Discmon
{
//---------------------------------------------------------------------------
// The signatures of upstream's handlers (strings passed by value)
typedef TMethodPtr<void(TObject * Sender, const UnicodeString Directory, bool & SubdirsChanged)> TDiscMonitorNotify;
typedef TMethodPtr<void(TObject * Sender, const UnicodeString Directory, const UnicodeString ErrorStr)> TDiscMonitorInvalid;
typedef TMethodPtr<void(TObject * Sender, TThreadMethod Method)> TDiscMonitorSynchronize;
typedef TMethodPtr<void(TObject * Sender, const UnicodeString DirectoryName, bool & Add)> TDiscMonitorFilter;
typedef TMethodPtr<void(TObject * Sender, int & MaxDirectories)> TDiscMonitorTooManyDirectories;
typedef TMethodPtr<void(TObject * Sender, int Directories)> TDiscMonitorDirectoriesChange;
//---------------------------------------------------------------------------
enum TMonitorFilter { moFilename, moDirName, moAttributes, moSize, moLastWrite, moSecurity };
typedef Set<TMonitorFilter, moFilename, moSecurity> TMonitorFilters;
//---------------------------------------------------------------------------
class TDiscMonitor : public TComponent
{
public:
  explicit TDiscMonitor(TComponent * AOwner);
  virtual ~TDiscMonitor();

  // Starts monitoring (in a thread)
  void Open();
  // Stops monitoring; can be called from the handlers (the thread finishes
  // asynchronously)
  void Close();
  // Adds the directory, and its subdirectories if SubDirs (with OnFilter
  // deciding which, and OnTooManyDirectories called whenever there are more
  // than MaxDirectories, unless it is negative)
  void AddDirectory(const UnicodeString & Directory, bool SubDirs);
  void SetDirectory(const UnicodeString & Directory);

  TStrings * GetDirectories() const { return FDirectories; }
  TMonitorFilters GetFilters() const { return FFilters; }
  void SetFilters(TMonitorFilters Value);
  bool GetSubTree() const { return FSubTree; }
  void SetSubTree(bool Value);
  bool GetActive() const { return FActive; }
  void SetActive(bool Value);
  int GetChangeDelay() const { return FChangeDelay; }
  void SetChangeDelay(int Value) { FChangeDelay = Value; }
  int GetMaxDirectories() const { return FMaxDirectories; }
  void SetMaxDirectories(int Value) { FMaxDirectories = Value; }

  TDiscMonitorNotify GetOnChange() const { return FOnChange; }
  void SetOnChange(TDiscMonitorNotify Value) { FOnChange = Value; }
  TDiscMonitorInvalid GetOnInvalid() const { return FOnInvalid; }
  void SetOnInvalid(TDiscMonitorInvalid Value) { FOnInvalid = Value; }
  TDiscMonitorSynchronize GetOnSynchronize() const { return FOnSynchronize; }
  void SetOnSynchronize(TDiscMonitorSynchronize Value) { FOnSynchronize = Value; }
  TDiscMonitorFilter GetOnFilter() const { return FOnFilter; }
  void SetOnFilter(TDiscMonitorFilter Value) { FOnFilter = Value; }
  TDiscMonitorTooManyDirectories GetOnTooManyDirectories() const { return FOnTooManyDirectories; }
  void SetOnTooManyDirectories(TDiscMonitorTooManyDirectories Value) { FOnTooManyDirectories = Value; }
  TDiscMonitorDirectoriesChange GetOnDirectoriesChange() const { return FOnDirectoriesChange; }
  void SetOnDirectoriesChange(TDiscMonitorDirectoriesChange Value) { FOnDirectoriesChange = Value; }

  __declspec(property(get = GetDirectories)) TStrings * Directories;
  __declspec(property(get = GetFilters, put = SetFilters)) TMonitorFilters Filters;
  __declspec(property(get = GetSubTree, put = SetSubTree)) bool SubTree;
  __declspec(property(get = GetActive, put = SetActive)) bool Active;
  __declspec(property(get = GetChangeDelay, put = SetChangeDelay)) int ChangeDelay;
  __declspec(property(get = GetMaxDirectories, put = SetMaxDirectories)) int MaxDirectories;
  __declspec(property(get = GetOnChange, put = SetOnChange)) TDiscMonitorNotify OnChange;
  __declspec(property(get = GetOnInvalid, put = SetOnInvalid)) TDiscMonitorInvalid OnInvalid;
  __declspec(property(get = GetOnSynchronize, put = SetOnSynchronize)) TDiscMonitorSynchronize OnSynchronize;
  __declspec(property(get = GetOnFilter, put = SetOnFilter)) TDiscMonitorFilter OnFilter;
  __declspec(property(get = GetOnTooManyDirectories, put = SetOnTooManyDirectories))
    TDiscMonitorTooManyDirectories OnTooManyDirectories;
  __declspec(property(get = GetOnDirectoriesChange, put = SetOnDirectoriesChange))
    TDiscMonitorDirectoriesChange OnDirectoriesChange;

private:
  TStringList * FDirectories;
  TMonitorFilters FFilters;
  bool FSubTree;
  bool FActive;
  int FChangeDelay;
  int FMaxDirectories;
  TDiscMonitorNotify FOnChange;
  TDiscMonitorInvalid FOnInvalid;
  TDiscMonitorSynchronize FOnSynchronize;
  TDiscMonitorFilter FOnFilter;
  TDiscMonitorTooManyDirectories FOnTooManyDirectories;
  TDiscMonitorDirectoriesChange FOnDirectoriesChange;

  std::thread FThread;
  int FInotify;
  int FStopEvent;
  std::atomic<bool> FStopping;
  std::atomic<bool> FFinished;
  std::map<int, UnicodeString> FWatches;
  // Of the requests to the main thread
  UnicodeString FNotifiedDirectory;
  bool FSubdirsChanged;
  UnicodeString FInvalidMessage;
  int FNotifiedDirectories;

  void Execute();
  bool Watch(const UnicodeString & Directory);
  void Unwatch(const UnicodeString & Directory);
  bool UpdateSubdirectories(const UnicodeString & Directory);
  void Invalid(const UnicodeString & Directory, int Error);
  void DoSynchronize(TThreadMethod Method);
  void InformChange();
  void InformInvalid();
  void InformDirectoriesChange();
  void Stop();
  void WaitForThread();
};
//---------------------------------------------------------------------------
} // namespace Discmon
//---------------------------------------------------------------------------
using namespace Discmon;
//---------------------------------------------------------------------------
#endif
