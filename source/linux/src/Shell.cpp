// Shell functions of the Windows API, see winapi/Shell.h.
//---------------------------------------------------------------------------
#include <winapi/Shell.h>

#include <System.hpp>
#include <SysUtils.hpp>

#include <climits>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <string>
#include <vector>

#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <pwd.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
std::string HomeDirectory()
{
  const char * Home = getenv("HOME");
  if ((Home != nullptr) && (*Home == '/'))
  {
    return Home;
  }
  struct passwd Entry;
  struct passwd * Found = nullptr;
  std::vector<char> Buffer(16384);
  if ((getpwuid_r(geteuid(), &Entry, Buffer.data(), Buffer.size(), &Found) == 0) && (Found != nullptr))
  {
    return Found->pw_dir;
  }
  return "/";
}
//---------------------------------------------------------------------------
// An XDG base directory: the variable if absolute, the default under home otherwise
std::string BaseDirectory(const char * Variable, const char * Default)
{
  const char * Value = getenv(Variable);
  if ((Value != nullptr) && (*Value == '/'))
  {
    return Value;
  }
  return HomeDirectory() + "/" + Default;
}
//---------------------------------------------------------------------------
// An XDG user directory (user-dirs.dirs), home if not configured
std::string UserDirectory(const char * Name)
{
  std::ifstream File(BaseDirectory("XDG_CONFIG_HOME", ".config") + "/user-dirs.dirs");
  std::string Line;
  std::string Prefix = std::string(Name) + "=\"";
  while (std::getline(File, Line))
  {
    if ((Line.compare(0, Prefix.size(), Prefix) == 0) && (Line.size() > Prefix.size()) && (Line.back() == '"'))
    {
      std::string Value = Line.substr(Prefix.size(), Line.size() - Prefix.size() - 1);
      if (Value.compare(0, 6, "$HOME/") == 0)
      {
        return HomeDirectory() + Value.substr(5);
      }
      if (Value == "$HOME")
      {
        return HomeDirectory();
      }
      if (!Value.empty() && (Value[0] == '/'))
      {
        return Value;
      }
    }
  }
  return HomeDirectory();
}
//---------------------------------------------------------------------------
bool CreateDirectories(const std::string & Path)
{
  for (size_t Slash = Path.find('/', 1); ; Slash = Path.find('/', Slash + 1))
  {
    std::string Part = Path.substr(0, Slash);
    if ((mkdir(Part.c_str(), 0700) != 0) && (errno != EEXIST))
    {
      return false;
    }
    if (Slash == std::string::npos)
    {
      return true;
    }
  }
}
//---------------------------------------------------------------------------
int RemoveEntry(const char * Path, const struct stat *, int Type, struct FTW *)
{
  return ((Type == FTW_DP) ? rmdir(Path) : unlink(Path));
}
//---------------------------------------------------------------------------
DWORD DeletePermanently(const std::string & Path)
{
  struct stat Info;
  if (lstat(Path.c_str(), &Info) != 0)
  {
    return rtl::ErrnoToWinError(errno);
  }
  int Result = S_ISDIR(Info.st_mode) ? nftw(Path.c_str(), RemoveEntry, 32, FTW_DEPTH | FTW_PHYS) : unlink(Path.c_str());
  return (Result == 0) ? ERROR_SUCCESS : rtl::ErrnoToWinError(errno);
}
//---------------------------------------------------------------------------
std::string PercentEncode(const std::string & Path)
{
  static const char Hex[] = "0123456789ABCDEF";
  std::string Result;
  for (unsigned char Char : Path)
  {
    if (isalnum(Char) || (strchr("/-._~", Char) != nullptr))
    {
      Result += static_cast<char>(Char);
    }
    else
    {
      Result += '%';
      Result += Hex[Char >> 4];
      Result += Hex[Char & 15];
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
// Moves the file to the home trash (freedesktop.org trash specification)
DWORD MoveToTrash(const std::string & Path)
{
  std::string Trash = BaseDirectory("XDG_DATA_HOME", ".local/share") + "/Trash";
  if (!CreateDirectories(Trash + "/files") || !CreateDirectories(Trash + "/info"))
  {
    return rtl::ErrnoToWinError(errno);
  }
  std::string Absolute = Path;
  char Resolved[PATH_MAX];
  size_t Slash = Path.rfind('/');
  std::string Directory = (Slash == std::string::npos) ? std::string(".") : Path.substr(0, (Slash == 0) ? 1 : Slash);
  std::string Name = (Slash == std::string::npos) ? Path : Path.substr(Slash + 1);
  if (realpath(Directory.c_str(), Resolved) != nullptr)
  {
    Absolute = std::string(Resolved) + ((strcmp(Resolved, "/") == 0) ? "" : "/") + Name;
  }

  time_t Now = time(nullptr);
  struct tm Local;
  localtime_r(&Now, &Local);
  char Date[32];
  strftime(Date, sizeof(Date), "%Y-%m-%dT%H:%M:%S", &Local);
  std::string Info = "[Trash Info]\nPath=" + PercentEncode(Absolute) + "\nDeletionDate=" + Date + "\n";

  for (int Index = 1; Index < 10000; Index++)
  {
    std::string TrashName = (Index == 1) ? Name : (Name + "." + std::to_string(Index));
    std::string InfoPath = Trash + "/info/" + TrashName + ".trashinfo";
    // Creating the information file reserves the name
    int Fd = open(InfoPath.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (Fd < 0)
    {
      if (errno == EEXIST)
      {
        continue;
      }
      return rtl::ErrnoToWinError(errno);
    }
    bool Written = (write(Fd, Info.data(), Info.size()) == static_cast<ssize_t>(Info.size()));
    close(Fd);
    if (Written && (rename(Path.c_str(), (Trash + "/files/" + TrashName).c_str()) == 0))
    {
      return ERROR_SUCCESS;
    }
    int Error = errno;
    unlink(InfoPath.c_str());
    return rtl::ErrnoToWinError(Written ? Error : EIO);
  }
  return ERROR_ALREADY_EXISTS;
}
//---------------------------------------------------------------------------
bool IsSeparator(wchar_t Char)
{
  return (Char == L'\\') || (Char == L'/');
}
//---------------------------------------------------------------------------
bool IsDrive(const wchar_t * Path)
{
  return iswalpha(Path[0]) && (Path[1] == L':');
}
//---------------------------------------------------------------------------
// Skips \\server\share\ (or as much of it as there is)
LPCWSTR SkipUnc(LPCWSTR Path)
{
  for (int Part = 0; Part < 2; Part++)
  {
    while ((*Path != L'\0') && !IsSeparator(*Path))
    {
      Path++;
    }
    if (*Path != L'\0')
    {
      Path++;
    }
  }
  return Path;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
HRESULT SHGetFolderPath(HWND, int Folder, HANDLE, DWORD, LPWSTR Path)
{
  std::string Result;
  switch (Folder & ~CSIDL_FLAG_CREATE)
  {
    case CSIDL_APPDATA:
      Result = BaseDirectory("XDG_CONFIG_HOME", ".config");
      break;
    case CSIDL_LOCAL_APPDATA:
      Result = BaseDirectory("XDG_DATA_HOME", ".local/share");
      break;
    case CSIDL_COMMON_APPDATA:
      Result = "/etc/xdg";
      break;
    case CSIDL_PROFILE:
      Result = HomeDirectory();
      break;
    case CSIDL_PERSONAL:
      Result = UserDirectory("XDG_DOCUMENTS_DIR");
      break;
    case CSIDL_DESKTOP:
    case CSIDL_DESKTOPDIRECTORY:
      Result = UserDirectory("XDG_DESKTOP_DIR");
      break;
    default:
      return E_FAIL;
  }
  if (((Folder & CSIDL_FLAG_CREATE) != 0) && !CreateDirectories(Result))
  {
    return E_FAIL;
  }
  UnicodeString Value = rtl::BytesToFileName(Result.c_str());
  if (Value.Length() >= MAX_PATH)
  {
    return E_FAIL;
  }
  wmemcpy(Path, Value.c_str(), Value.Length() + 1);
  return S_OK;
}
//---------------------------------------------------------------------------
BOOL SHGetSpecialFolderPath(HWND Owner, LPWSTR Path, int Folder, BOOL Create)
{
  return SUCCEEDED(SHGetFolderPath(Owner, Folder | (Create ? CSIDL_FLAG_CREATE : 0), NULL, SHGFP_TYPE_CURRENT, Path));
}
//---------------------------------------------------------------------------
int SHFileOperation(LPSHFILEOPSTRUCT FileOp)
{
  FileOp->fAnyOperationsAborted = FALSE;
  if (FileOp->wFunc != FO_DELETE)
  {
    return ERROR_NOT_SUPPORTED;
  }
  for (LPCWSTR Name = FileOp->pFrom; *Name != L'\0'; Name += wcslen(Name) + 1)
  {
    std::string Path = rtl::FileNameToBytes(Name);
    DWORD Error = ((FileOp->fFlags & FOF_ALLOWUNDO) != 0) ? MoveToTrash(Path) : DeletePermanently(Path);
    if (Error != ERROR_SUCCESS)
    {
      return static_cast<int>(Error);
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
DWORD_PTR SHGetFileInfo(LPCWSTR Path, DWORD FileAttributes, SHFILEINFOW * Info, UINT InfoSize, UINT Flags)
{
  if ((Info == NULL) || (InfoSize < sizeof(SHFILEINFOW)))
  {
    return 0;
  }
  memset(Info, 0, sizeof(*Info));
  if ((Flags & SHGFI_USEFILEATTRIBUTES) == 0)
  {
    DWORD Attributes = static_cast<DWORD>(FileGetAttr(Path));
    if (static_cast<int>(Attributes) < 0)
    {
      return 0;
    }
    FileAttributes = Attributes;
  }
  if ((Flags & SHGFI_TYPENAME) != 0)
  {
    UnicodeString TypeName;
    if ((FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
    {
      TypeName = L"File folder";
    }
    else
    {
      UnicodeString Ext = ExtractFileExt(UnicodeString(Path));
      TypeName = (Ext.Length() > 1) ? (Ext.SubString(2, Ext.Length() - 1).UpperCase() + L" File") : UnicodeString(L"File");
    }
    wcsncpy(Info->szTypeName, TypeName.c_str(), 79);
  }
  if ((Flags & SHGFI_DISPLAYNAME) != 0)
  {
    wcsncpy(Info->szDisplayName, ExtractFileName(UnicodeString(Path)).c_str(), MAX_PATH - 1);
  }
  Info->dwAttributes = FileAttributes;
  return 1;
}
//---------------------------------------------------------------------------
BOOL PathIsRelative(LPCWSTR Path)
{
  return !IsSeparator(Path[0]) && !IsDrive(Path);
}
//---------------------------------------------------------------------------
LPCWSTR PathSkipRoot(LPCWSTR Path)
{
  if (IsSeparator(Path[0]) && IsSeparator(Path[1]))
  {
    if ((Path[2] == L'?') && IsSeparator(Path[3]))
    {
      LPCWSTR Rest = Path + 4;
      if ((wcsncasecmp(Rest, L"UNC", 3) == 0) && IsSeparator(Rest[3]))
      {
        return SkipUnc(Rest + 4);
      }
      if (IsDrive(Rest))
      {
        return Rest + (IsSeparator(Rest[2]) ? 3 : 2);
      }
      return NULL;
    }
    return SkipUnc(Path + 2);
  }
  if (IsDrive(Path))
  {
    return Path + (IsSeparator(Path[2]) ? 3 : 2);
  }
  if (IsSeparator(Path[0]))
  {
    return Path + 1;
  }
  return NULL;
}
//---------------------------------------------------------------------------
int StrCmpLogicalW(LPCWSTR String1, LPCWSTR String2)
{
  const wchar_t * P1 = String1;
  const wchar_t * P2 = String2;
  while ((*P1 != L'\0') && (*P2 != L'\0'))
  {
    if (iswdigit(*P1) && iswdigit(*P2))
    {
      while (*P1 == L'0')
      {
        P1++;
      }
      while (*P2 == L'0')
      {
        P2++;
      }
      const wchar_t * End1 = P1;
      const wchar_t * End2 = P2;
      while (iswdigit(*End1))
      {
        End1++;
      }
      while (iswdigit(*End2))
      {
        End2++;
      }
      if ((End1 - P1) != (End2 - P2))
      {
        return ((End1 - P1) < (End2 - P2)) ? -1 : 1;
      }
      for (; P1 < End1; P1++, P2++)
      {
        if (*P1 != *P2)
        {
          return (*P1 < *P2) ? -1 : 1;
        }
      }
    }
    else
    {
      wchar_t C1 = rtl::LowerChar(*P1);
      wchar_t C2 = rtl::LowerChar(*P2);
      if (C1 != C2)
      {
        return (C1 < C2) ? -1 : 1;
      }
      P1++;
      P2++;
    }
  }
  return (*P1 == L'\0') ? ((*P2 == L'\0') ? 0 : -1) : 1;
}
//---------------------------------------------------------------------------
