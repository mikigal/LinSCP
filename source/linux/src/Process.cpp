// Process information functions of the Windows API, see winapi/Process.h.
//---------------------------------------------------------------------------
#include <winapi/Process.h>

#include <System.hpp>
#include <SysUtils.hpp>

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <dirent.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
struct TProcessInfo
{
  DWORD Id;
  DWORD ParentId;
  DWORD Threads;
  UnicodeString Name;
};
//---------------------------------------------------------------------------
class TSnapshot : public winapi::TKernelObject
{
public:
  std::vector<TProcessInfo> Processes;
  size_t Next = 0;
};
//---------------------------------------------------------------------------
class TProcess : public winapi::TKernelObject
{
public:
  explicit TProcess(DWORD Id) : FId(Id) {}
  DWORD Id() const { return FId; }

private:
  DWORD FId;
};
//---------------------------------------------------------------------------
bool ReadProcess(DWORD Id, TProcessInfo & Info)
{
  // /proc/<pid>/stat: pid (comm) state ppid ...; comm may contain anything
  std::ifstream Stat("/proc/" + std::to_string(Id) + "/stat");
  std::string Line;
  if (!std::getline(Stat, Line))
  {
    return false;
  }
  size_t Open = Line.find('(');
  size_t Close = Line.rfind(')');
  if ((Open == std::string::npos) || (Close == std::string::npos) || (Close < Open))
  {
    return false;
  }
  std::istringstream Fields(Line.substr(Close + 1));
  std::string State;
  unsigned long ParentId = 0;
  Fields >> State >> ParentId;
  // Field 20 is num_threads: skip 15 fields after ppid
  std::string Skip;
  for (int Index = 0; Index < 15; Index++)
  {
    Fields >> Skip;
  }
  unsigned long Threads = 0;
  Fields >> Threads;
  Info.Id = Id;
  Info.ParentId = static_cast<DWORD>(ParentId);
  Info.Threads = static_cast<DWORD>(Threads);
  Info.Name = rtl::BytesToFileName(Line.substr(Open + 1, Close - Open - 1).c_str());
  return true;
}
//---------------------------------------------------------------------------
BOOL FillEntry(HANDLE Snapshot, LPPROCESSENTRY32 Entry, bool First)
{
  std::shared_ptr<TSnapshot> Object = winapi::FindObject<TSnapshot>(Snapshot);
  if (!Object)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  if (First)
  {
    Object->Next = 0;
  }
  if (Object->Next >= Object->Processes.size())
  {
    SetLastError(ERROR_NO_MORE_FILES);
    return FALSE;
  }
  const TProcessInfo & Info = Object->Processes[Object->Next++];
  DWORD Size = Entry->dwSize;
  memset(Entry, 0, sizeof(*Entry));
  Entry->dwSize = Size;
  Entry->th32ProcessID = Info.Id;
  Entry->th32ParentProcessID = Info.ParentId;
  Entry->cntThreads = Info.Threads;
  wcsncpy(Entry->szExeFile, Info.Name.c_str(), MAX_PATH - 1);
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD ProcessIdOf(HANDLE Process)
{
  if (Process == GetCurrentProcess())
  {
    return GetCurrentProcessId();
  }
  std::shared_ptr<TProcess> Object = winapi::FindObject<TProcess>(Process);
  return Object ? Object->Id() : 0;
}
//---------------------------------------------------------------------------
std::string ExecutableOf(DWORD Id)
{
  char Buffer[4096];
  ssize_t Length = readlink(("/proc/" + std::to_string(Id) + "/exe").c_str(), Buffer, sizeof(Buffer) - 1);
  return (Length > 0) ? std::string(Buffer, Length) : std::string();
}
//---------------------------------------------------------------------------
// The executable, then the shared libraries in the order of their mappings
std::vector<std::string> ModulesOf(DWORD Id)
{
  std::vector<std::string> Result;
  std::string Executable = ExecutableOf(Id);
  if (Executable.empty())
  {
    return Result;
  }
  Result.push_back(Executable);
  std::ifstream Maps("/proc/" + std::to_string(Id) + "/maps");
  std::string Line;
  while (std::getline(Maps, Line))
  {
    size_t Path = Line.find('/');
    if (Path == std::string::npos)
    {
      continue;
    }
    std::string Name = Line.substr(Path);
    bool Library = (Name.find(".so") != std::string::npos);
    bool Known = false;
    for (const std::string & Module : Result)
    {
      Known = Known || (Module == Name);
    }
    if (Library && !Known)
    {
      Result.push_back(Name);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
HANDLE CreateToolhelp32Snapshot(DWORD Flags, DWORD)
{
  if ((Flags & TH32CS_SNAPPROCESS) == 0)
  {
    SetLastError(ERROR_NOT_SUPPORTED);
    return INVALID_HANDLE_VALUE;
  }
  std::shared_ptr<TSnapshot> Snapshot = std::make_shared<TSnapshot>();
  DIR * Directory = opendir("/proc");
  if (Directory == nullptr)
  {
    rtl::SetLastErrorFromErrno();
    return INVALID_HANDLE_VALUE;
  }
  while (struct dirent * Entry = readdir(Directory))
  {
    char * End;
    unsigned long Id = strtoul(Entry->d_name, &End, 10);
    TProcessInfo Info;
    if ((*End == '\0') && (End != Entry->d_name) && ReadProcess(static_cast<DWORD>(Id), Info))
    {
      Snapshot->Processes.push_back(Info);
    }
  }
  closedir(Directory);
  return winapi::RegisterObject(Snapshot);
}
//---------------------------------------------------------------------------
BOOL Process32First(HANDLE Snapshot, LPPROCESSENTRY32 Entry)
{
  return FillEntry(Snapshot, Entry, true);
}
//---------------------------------------------------------------------------
BOOL Process32Next(HANDLE Snapshot, LPPROCESSENTRY32 Entry)
{
  return FillEntry(Snapshot, Entry, false);
}
//---------------------------------------------------------------------------
HANDLE OpenProcess(DWORD, BOOL, DWORD ProcessId)
{
  if (access(("/proc/" + std::to_string(ProcessId)).c_str(), F_OK) != 0)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }
  return winapi::RegisterObject(std::make_shared<TProcess>(ProcessId));
}
//---------------------------------------------------------------------------
BOOL EnumProcessModules(HANDLE Process, HMODULE * Modules, DWORD Size, LPDWORD Needed)
{
  DWORD Id = ProcessIdOf(Process);
  std::vector<std::string> List = (Id != 0) ? ModulesOf(Id) : std::vector<std::string>();
  if (List.empty())
  {
    SetLastError(ERROR_ACCESS_DENIED);
    return FALSE;
  }
  *Needed = static_cast<DWORD>(List.size() * sizeof(HMODULE));
  for (size_t Index = 0; (Index < List.size()) && ((Index + 1) * sizeof(HMODULE) <= Size); Index++)
  {
    // Module handles are 1-based indexes into the list
    Modules[Index] = reinterpret_cast<HMODULE>(Index + 1);
  }
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetModuleFileNameEx(HANDLE Process, HMODULE Module, LPWSTR FileName, DWORD Size)
{
  DWORD Id = ProcessIdOf(Process);
  size_t Index = reinterpret_cast<size_t>(Module);
  std::string Name;
  if (Id != 0)
  {
    if (Index <= 1)
    {
      Name = ExecutableOf(Id);
    }
    else
    {
      std::vector<std::string> List = ModulesOf(Id);
      Name = (Index <= List.size()) ? List[Index - 1] : std::string();
    }
  }
  if (Name.empty() || (Size == 0))
  {
    SetLastError(ERROR_ACCESS_DENIED);
    return 0;
  }
  UnicodeString Value = rtl::BytesToFileName(Name.c_str());
  DWORD Length = std::min(static_cast<DWORD>(Value.Length()), Size - 1);
  wmemcpy(FileName, Value.c_str(), Length);
  FileName[Length] = L'\0';
  return Length;
}
//---------------------------------------------------------------------------
