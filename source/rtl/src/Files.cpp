// SysUtils: file names and files (POSIX semantics)
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <vector>
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
std::string FileNameToBytes(const System::UnicodeString & FileName)
{
  return EncodeUtf8(FileName.c_str(), FileName.Length());
}
//---------------------------------------------------------------------------
System::UnicodeString BytesToFileName(const char * Bytes)
{
  return System::UnicodeString(DecodeUtf8(Bytes, strlen(Bytes)));
}
//---------------------------------------------------------------------------
} // namespace rtl
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
namespace
{
const THandle InvalidHandle = static_cast<THandle>(-1);

inline std::string Native(const UnicodeString & FileName)
{
  return rtl::FileNameToBytes(FileName);
}

inline int HandleToFd(THandle Handle)
{
  return static_cast<int>(static_cast<intptr_t>(Handle));
}

inline THandle FdToHandle(int Fd)
{
  return (Fd < 0) ? InvalidHandle : static_cast<THandle>(Fd);
}

// 100ns intervals between 1601-01-01 and 1970-01-01
const long long FileTimeUnixDelta = 116444736000000000LL;

FILETIME ToFileTime(const timespec & Time)
{
  long long Value = static_cast<long long>(Time.tv_sec) * 10000000LL + Time.tv_nsec / 100 + FileTimeUnixDelta;
  FILETIME Result;
  Result.dwLowDateTime = static_cast<DWORD>(Value & 0xFFFFFFFF);
  Result.dwHighDateTime = static_cast<DWORD>(static_cast<unsigned long long>(Value) >> 32);
  return Result;
}

int DosDateOf(time_t Time)
{
  tm Local;
  if (localtime_r(&Time, &Local) == nullptr)
  {
    return 0;
  }
  int Year = Local.tm_year + 1900;
  if ((Year < 1980) || (Year > 2107))
  {
    return 0;
  }
  unsigned int Date = ((Year - 1980) << 9) | ((Local.tm_mon + 1) << 5) | Local.tm_mday;
  unsigned int Time2 = (Local.tm_hour << 11) | (Local.tm_min << 5) | (Local.tm_sec / 2);
  return static_cast<int>((Date << 16) | Time2);
}

time_t TimeOfDosDate(int Age)
{
  TDateTime DateTime = FileDateToDateTime(Age);
  Word Year, Month, Day, Hour, Min, Sec, MSec;
  DecodeDate(DateTime, Year, Month, Day);
  DecodeTime(DateTime, Hour, Min, Sec, MSec);
  tm Local = {};
  Local.tm_year = Year - 1900;
  Local.tm_mon = Month - 1;
  Local.tm_mday = Day;
  Local.tm_hour = Hour;
  Local.tm_min = Min;
  Local.tm_sec = Sec;
  Local.tm_isdst = -1;
  return mktime(&Local);
}

bool IsHiddenName(const UnicodeString & FileName)
{
  UnicodeString Name = ExtractFileName(FileName);
  return !Name.IsEmpty() && (Name[1] == L'.') && (Name != L".") && (Name != L"..");
}

// Attributes from the entry (lstat) and, for links, from the target (stat)
int AttributesOf(const std::string & Path, const UnicodeString & Name, bool FollowLink, struct stat * Info)
{
  struct stat LinkInfo;
  if (lstat(Path.c_str(), &LinkInfo) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return -1;
  }
  struct stat TargetInfo = LinkInfo;
  bool IsLink = S_ISLNK(LinkInfo.st_mode);
  bool Dangling = false;
  if (IsLink && (stat(Path.c_str(), &TargetInfo) != 0))
  {
    TargetInfo = LinkInfo;
    Dangling = true;
  }
  const struct stat & Effective = (FollowLink || !IsLink) ? TargetInfo : LinkInfo;
  if (Info != nullptr)
  {
    *Info = Effective;
  }

  int Attr = 0;
  if (S_ISDIR(TargetInfo.st_mode))
  {
    Attr |= faDirectory;
  }
  else if (!S_ISREG(TargetInfo.st_mode) && !(IsLink && Dangling))
  {
    Attr |= faSysFile;
  }
  if (IsLink)
  {
    Attr |= faSymLink;
  }
  if (IsHiddenName(Name))
  {
    Attr |= faHidden;
  }
  if (Dangling || (faccessat(AT_FDCWD, Path.c_str(), W_OK, AT_EACCESS) != 0))
  {
    Attr |= faReadOnly;
  }
  return Attr;
}

bool HasWildcards(const UnicodeString & Pattern)
{
  return (Pattern.Pos(L"*") > 0) || (Pattern.Pos(L"?") > 0);
}

// Windows FindFirst wildcards: * and ? only, "*.*" matches everything
bool MatchesPattern(const wchar_t * Name, const wchar_t * Pattern)
{
  while (*Pattern != L'\0')
  {
    if (*Pattern == L'*')
    {
      while (*Pattern == L'*')
      {
        Pattern++;
      }
      if (*Pattern == L'\0')
      {
        return true;
      }
      for (const wchar_t * P = Name; ; P++)
      {
        if (MatchesPattern(P, Pattern))
        {
          return true;
        }
        if (*P == L'\0')
        {
          return false;
        }
      }
    }
    if (*Name == L'\0')
    {
      return false;
    }
    if ((*Pattern != L'?') && (*Pattern != *Name))
    {
      return false;
    }
    Pattern++;
    Name++;
  }
  return (*Name == L'\0');
}

struct TFindState
{
  DIR * Dir = nullptr;
  bool Single = false;
  bool SingleDone = false;
};

void FillSearchRec(TSearchRec & F, const UnicodeString & Name, int Attr, const struct stat & Info)
{
  F.Name = Name;
  F.Attr = Attr;
  F.Size = ((Attr & faDirectory) != 0) ? 0 : static_cast<__int64>(Info.st_size);
  F.Time = DosDateOf(Info.st_mtim.tv_sec);
  memset(&F.FindData, 0, sizeof(F.FindData));
  F.FindData.dwFileAttributes = static_cast<DWORD>(Attr);
  F.FindData.ftCreationTime = ToFileTime(Info.st_ctim);
  F.FindData.ftLastAccessTime = ToFileTime(Info.st_atim);
  F.FindData.ftLastWriteTime = ToFileTime(Info.st_mtim);
  F.FindData.nFileSizeHigh = static_cast<DWORD>(static_cast<unsigned long long>(F.Size) >> 32);
  F.FindData.nFileSizeLow = static_cast<DWORD>(F.Size & 0xFFFFFFFF);
  int Len = std::min(Name.Length(), static_cast<int>(MAX_PATH) - 1);
  wmemcpy(F.FindData.cFileName, Name.c_str(), Len);
  F.FindData.cFileName[Len] = L'\0';
}

// Looks for the next entry; 0 on success, a Windows error code otherwise
int FindMatchingFile(TSearchRec & F)
{
  TFindState * State = reinterpret_cast<TFindState *>(F.FindHandle);
  if (State == nullptr)
  {
    return ERROR_INVALID_HANDLE;
  }
  if (State->Single)
  {
    if (State->SingleDone)
    {
      return ERROR_NO_MORE_FILES;
    }
    State->SingleDone = true;
    struct stat Info;
    int Attr = AttributesOf(Native(F.PathOnly + F.Pattern), F.Pattern, true, &Info);
    if ((Attr < 0) || ((Attr & F.ExcludeAttr) != 0))
    {
      return ERROR_FILE_NOT_FOUND;
    }
    FillSearchRec(F, F.Pattern, Attr, Info);
    return 0;
  }
  for (;;)
  {
    errno = 0;
    dirent * Entry = readdir(State->Dir);
    if (Entry == nullptr)
    {
      return (errno != 0) ? static_cast<int>(rtl::ErrnoToWinError(errno)) : ERROR_NO_MORE_FILES;
    }
    UnicodeString Name = rtl::BytesToFileName(Entry->d_name);
    if (!MatchesPattern(Name.c_str(), F.Pattern.c_str()))
    {
      continue;
    }
    struct stat Info;
    int Attr = AttributesOf(Native(F.PathOnly + Name), Name, true, &Info);
    if ((Attr < 0) || ((Attr & F.ExcludeAttr) != 0))
    {
      continue;
    }
    FillSearchRec(F, Name, Attr, Info);
    return 0;
  }
}
} // namespace
//---------------------------------------------------------------------------
// File names
//---------------------------------------------------------------------------
UnicodeString IncludeTrailingPathDelimiter(const UnicodeString S)
{
  return IsPathDelimiter(S, S.Length()) ? S : (S + PathDelim);
}
//---------------------------------------------------------------------------
UnicodeString ExcludeTrailingPathDelimiter(const UnicodeString S)
{
  UnicodeString Result = S;
  if (IsPathDelimiter(Result, Result.Length()))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString IncludeTrailingBackslash(const UnicodeString S)
{
  return IncludeTrailingPathDelimiter(S);
}
//---------------------------------------------------------------------------
UnicodeString ExcludeTrailingBackslash(const UnicodeString S)
{
  return ExcludeTrailingPathDelimiter(S);
}
//---------------------------------------------------------------------------
UnicodeString ExtractFilePath(const UnicodeString FileName)
{
  return FileName.SubString(1, LastDelimiter(PathDelim, FileName));
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileDir(const UnicodeString FileName)
{
  int I = LastDelimiter(PathDelim, FileName);
  if ((I > 1) && (FileName[I] == PathDelim) && !IsDelimiter(PathDelim, FileName, I - 1))
  {
    I--;
  }
  return FileName.SubString(1, I);
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileName(const UnicodeString FileName)
{
  return FileName.SubString(LastDelimiter(PathDelim, FileName) + 1, MaxInt);
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileExt(const UnicodeString FileName)
{
  int I = LastDelimiter(UnicodeString(L".") + PathDelim, FileName);
  return ((I > 0) && (FileName[I] == L'.')) ? FileName.SubString(I, MaxInt) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString ExtractFileDrive(const UnicodeString)
{
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString ChangeFileExt(const UnicodeString FileName, const UnicodeString Extension)
{
  int I = LastDelimiter(UnicodeString(L".") + PathDelim, FileName);
  if ((I == 0) || (FileName[I] != L'.'))
  {
    I = MaxInt;
  }
  return FileName.SubString(1, I - 1) + Extension;
}
//---------------------------------------------------------------------------
UnicodeString ChangeFilePath(const UnicodeString FileName, const UnicodeString Path)
{
  return IncludeTrailingPathDelimiter(Path) + ExtractFileName(FileName);
}
//---------------------------------------------------------------------------
bool IsRelativePath(const UnicodeString Path)
{
  return Path.IsEmpty() || (Path[1] != PathDelim);
}
//---------------------------------------------------------------------------
UnicodeString ExpandFileName(const UnicodeString FileName)
{
  if (FileName.IsEmpty())
  {
    return UnicodeString();
  }
  UnicodeString Path = IsRelativePath(FileName) ? (IncludeTrailingPathDelimiter(GetCurrentDir()) + FileName) : FileName;
  // Lexical normalization; symbolic links are not resolved
  std::vector<UnicodeString> Parts;
  int Start = 1;
  int Len = Path.Length();
  for (int I = 1; I <= Len + 1; I++)
  {
    if ((I > Len) || (Path[I] == PathDelim))
    {
      UnicodeString Part = Path.SubString(Start, I - Start);
      if (Part == L"..")
      {
        if (!Parts.empty())
        {
          Parts.pop_back();
        }
      }
      else if (!Part.IsEmpty() && (Part != L"."))
      {
        Parts.push_back(Part);
      }
      Start = I + 1;
    }
  }
  UnicodeString Result;
  for (const UnicodeString & Part : Parts)
  {
    Result += PathDelim;
    Result += Part;
  }
  if (Result.IsEmpty() || IsPathDelimiter(FileName, FileName.Length()))
  {
    Result += PathDelim;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString ExpandUNCFileName(const UnicodeString FileName)
{
  return ExpandFileName(FileName);
}
//---------------------------------------------------------------------------
UnicodeString ExtractShortPathName(const UnicodeString FileName)
{
  return FileName;
}
//---------------------------------------------------------------------------
UnicodeString ExtractRelativePath(const UnicodeString BaseName, const UnicodeString DestName)
{
  UnicodeString BasePath = ExtractFilePath(BaseName);
  UnicodeString DestPath = ExtractFilePath(DestName);
  auto Split = [](const UnicodeString & Path)
    {
      std::vector<UnicodeString> Result;
      int Start = 1;
      for (int I = 1; I <= Path.Length(); I++)
      {
        if (Path[I] == PathDelim)
        {
          if (I > Start)
          {
            Result.push_back(Path.SubString(Start, I - Start));
          }
          Start = I + 1;
        }
      }
      if (Start <= Path.Length())
      {
        Result.push_back(Path.SubString(Start, MaxInt));
      }
      return Result;
    };
  std::vector<UnicodeString> BaseParts = Split(BasePath);
  std::vector<UnicodeString> DestParts = Split(DestPath);
  size_t Common = 0;
  while ((Common < BaseParts.size()) && (Common < DestParts.size()) && SameFileName(BaseParts[Common], DestParts[Common]))
  {
    Common++;
  }
  UnicodeString Result;
  for (size_t Index = Common; Index < BaseParts.size(); Index++)
  {
    Result += L"../";
  }
  for (size_t Index = Common; Index < DestParts.size(); Index++)
  {
    Result += DestParts[Index] + PathDelim;
  }
  return Result + ExtractFileName(DestName);
}
//---------------------------------------------------------------------------
UnicodeString GetCurrentDir()
{
  std::vector<char> Buffer(4096);
  while (getcwd(Buffer.data(), Buffer.size()) == nullptr)
  {
    if (errno != ERANGE)
    {
      rtl::SetLastErrorFromErrno();
      return UnicodeString();
    }
    Buffer.resize(Buffer.size() * 2);
  }
  return rtl::BytesToFileName(Buffer.data());
}
//---------------------------------------------------------------------------
bool SetCurrentDir(const UnicodeString Dir)
{
  if (chdir(Native(Dir).c_str()) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------
UnicodeString GetEnvironmentVariable(const UnicodeString Name)
{
  const char * Value = getenv(Native(Name).c_str());
  return (Value != nullptr) ? rtl::BytesToFileName(Value) : UnicodeString();
}
//---------------------------------------------------------------------------
namespace
{
// The numbers of the kernel release, e.g. 6.12.3 of "6.12.3-arch1-1"
const std::vector<int> & KernelVersion()
{
  static const std::vector<int> Version = []()
    {
      std::vector<int> Result;
      struct utsname Name;
      if (uname(&Name) == 0)
      {
        const char * P = Name.release;
        while ((Result.size() < 3) && isdigit(static_cast<unsigned char>(*P)))
        {
          char * End;
          Result.push_back(static_cast<int>(strtol(P, &End, 10)));
          P = (*End == '.') ? End + 1 : End;
          if (End == P)
          {
            break;
          }
        }
      }
      Result.resize(3, 0);
      return Result;
    }();
  return Version;
}
} // namespace
//---------------------------------------------------------------------------
int Win32MajorVersion()
{
  return KernelVersion()[0];
}
//---------------------------------------------------------------------------
int Win32MinorVersion()
{
  return KernelVersion()[1];
}
//---------------------------------------------------------------------------
int Win32BuildNumber()
{
  return KernelVersion()[2];
}
//---------------------------------------------------------------------------
UnicodeString Win32CSDVersion()
{
  return UnicodeString();
}
//---------------------------------------------------------------------------
bool CheckWin32Version(int AMajor, int AMinor)
{
  return (Win32MajorVersion() > AMajor) || ((Win32MajorVersion() == AMajor) && (Win32MinorVersion() >= AMinor));
}
//---------------------------------------------------------------------------
// Files
//---------------------------------------------------------------------------
bool FileExists(const UnicodeString FileName, bool FollowLink)
{
  struct stat Info;
  std::string Path = Native(FileName);
  int Result = FollowLink ? stat(Path.c_str(), &Info) : lstat(Path.c_str(), &Info);
  if (Result != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return !S_ISDIR(Info.st_mode);
}
//---------------------------------------------------------------------------
bool DirectoryExists(const UnicodeString Directory, bool FollowLink)
{
  struct stat Info;
  std::string Path = Native(Directory);
  int Result = FollowLink ? stat(Path.c_str(), &Info) : lstat(Path.c_str(), &Info);
  if (Result != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return S_ISDIR(Info.st_mode);
}
//---------------------------------------------------------------------------
bool CreateDir(const UnicodeString Dir)
{
  if (mkdir(Native(Dir).c_str(), 0777) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------
bool ForceDirectories(UnicodeString Dir)
{
  if (Dir.IsEmpty())
  {
    throw EInOutError(L"Unable to create directory");
  }
  Dir = ExcludeTrailingPathDelimiter(Dir);
  if (Dir.IsEmpty() || DirectoryExists(Dir))
  {
    return true;
  }
  UnicodeString Parent = ExtractFilePath(Dir);
  if (!Parent.IsEmpty() && (Parent != Dir) && !ForceDirectories(Parent))
  {
    return false;
  }
  return CreateDir(Dir) || DirectoryExists(Dir);
}
//---------------------------------------------------------------------------
bool RemoveDir(const UnicodeString Dir)
{
  if (rmdir(Native(Dir).c_str()) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------
bool DeleteFile(const UnicodeString FileName)
{
  if (unlink(Native(FileName).c_str()) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------
bool RenameFile(const UnicodeString OldName, const UnicodeString NewName)
{
  // Like Windows' MoveFile, an existing target is not replaced
  std::string From = Native(OldName);
  std::string To = Native(NewName);
  long Result = syscall(SYS_renameat2, AT_FDCWD, From.c_str(), AT_FDCWD, To.c_str(), 1 /* RENAME_NOREPLACE */);
  if ((Result != 0) && ((errno == EINVAL) || (errno == ENOSYS)))
  {
    // File system without RENAME_NOREPLACE support
    struct stat Info;
    if (lstat(To.c_str(), &Info) == 0)
    {
      errno = EEXIST;
    }
    else
    {
      Result = rename(From.c_str(), To.c_str());
    }
  }
  if (Result != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  return true;
}
//---------------------------------------------------------------------------
int FileGetAttr(const UnicodeString FileName, bool FollowLink)
{
  return AttributesOf(Native(FileName), FileName, FollowLink, nullptr);
}
//---------------------------------------------------------------------------
int FileSetAttr(const UnicodeString FileName, int Attr, bool FollowLink)
{
  std::string Path = Native(FileName);
  struct stat Info;
  if ((FollowLink ? stat(Path.c_str(), &Info) : lstat(Path.c_str(), &Info)) != 0)
  {
    return static_cast<int>(rtl::SetLastErrorFromErrno());
  }
  if (S_ISLNK(Info.st_mode))
  {
    // Permissions of symbolic links cannot be changed on Linux
    return 0;
  }
  mode_t Mode = Info.st_mode & 07777;
  if ((Attr & faReadOnly) != 0)
  {
    Mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
  }
  else if ((Mode & (S_IWUSR | S_IWGRP | S_IWOTH)) == 0)
  {
    Mode |= S_IWUSR;
  }
  if ((Mode != (Info.st_mode & 07777)) && (chmod(Path.c_str(), Mode) != 0))
  {
    return static_cast<int>(rtl::SetLastErrorFromErrno());
  }
  return 0;
}
//---------------------------------------------------------------------------
bool FileIsReadOnly(const UnicodeString FileName)
{
  return (faccessat(AT_FDCWD, Native(FileName).c_str(), W_OK, AT_EACCESS) != 0);
}
//---------------------------------------------------------------------------
bool FileSetReadOnly(const UnicodeString FileName, bool ReadOnly)
{
  int Attr = FileGetAttr(FileName);
  if (Attr < 0)
  {
    return false;
  }
  return (FileSetAttr(FileName, ReadOnly ? (Attr | faReadOnly) : (Attr & ~faReadOnly)) == 0);
}
//---------------------------------------------------------------------------
bool FileGetSymLinkTarget(const UnicodeString FileName, UnicodeString & TargetName)
{
  std::string Path = Native(FileName);
  std::vector<char> Buffer(4096);
  for (;;)
  {
    ssize_t Len = readlink(Path.c_str(), Buffer.data(), Buffer.size());
    if (Len < 0)
    {
      rtl::SetLastErrorFromErrno();
      return false;
    }
    if (static_cast<size_t>(Len) < Buffer.size())
    {
      TargetName = UnicodeString(rtl::DecodeUtf8(Buffer.data(), Len));
      return true;
    }
    Buffer.resize(Buffer.size() * 2);
  }
}
//---------------------------------------------------------------------------
int FileAge(const UnicodeString FileName)
{
  struct stat Info;
  if (stat(Native(FileName).c_str(), &Info) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return -1;
  }
  return DosDateOf(Info.st_mtim.tv_sec);
}
//---------------------------------------------------------------------------
bool FileAge(const UnicodeString FileName, TDateTime & FileDateTime, bool FollowLink)
{
  struct stat Info;
  std::string Path = Native(FileName);
  if ((FollowLink ? stat(Path.c_str(), &Info) : lstat(Path.c_str(), &Info)) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return false;
  }
  FileDateTime = rtl::DateTimeFromUnixTime(Info.st_mtim.tv_sec, Info.st_mtim.tv_nsec, true);
  return true;
}
//---------------------------------------------------------------------------
int FileGetDate(THandle Handle)
{
  struct stat Info;
  if (fstat(HandleToFd(Handle), &Info) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return -1;
  }
  return DosDateOf(Info.st_mtim.tv_sec);
}
//---------------------------------------------------------------------------
int FileSetDate(const UnicodeString FileName, int Age)
{
  utimbuf Times;
  Times.actime = Times.modtime = TimeOfDosDate(Age);
  if (utime(Native(FileName).c_str(), &Times) != 0)
  {
    return static_cast<int>(rtl::SetLastErrorFromErrno());
  }
  return 0;
}
//---------------------------------------------------------------------------
int FileSetDate(THandle Handle, int Age)
{
  timespec Times[2];
  Times[0].tv_sec = Times[1].tv_sec = TimeOfDosDate(Age);
  Times[0].tv_nsec = Times[1].tv_nsec = 0;
  if (futimens(HandleToFd(Handle), Times) != 0)
  {
    return static_cast<int>(rtl::SetLastErrorFromErrno());
  }
  return 0;
}
//---------------------------------------------------------------------------
THandle FileOpen(const UnicodeString FileName, unsigned int Mode)
{
  int Flags;
  switch (Mode & 3)
  {
    case fmOpenWrite: Flags = O_WRONLY; break;
    case fmOpenReadWrite: Flags = O_RDWR; break;
    default: Flags = O_RDONLY; break;
  }
  int Fd = open(Native(FileName).c_str(), Flags | O_CLOEXEC);
  if (Fd < 0)
  {
    rtl::SetLastErrorFromErrno();
  }
  return FdToHandle(Fd);
}
//---------------------------------------------------------------------------
THandle FileCreate(const UnicodeString FileName, unsigned int Mode, int Rights)
{
  int Flags = O_CREAT | O_TRUNC | O_CLOEXEC | (((Mode & 3) == fmOpenWrite) ? O_WRONLY : O_RDWR);
  if ((Mode & fmExclusive) != 0)
  {
    Flags |= O_EXCL;
  }
  int Fd = open(Native(FileName).c_str(), Flags, static_cast<mode_t>(Rights));
  if (Fd < 0)
  {
    rtl::SetLastErrorFromErrno();
  }
  return FdToHandle(Fd);
}
//---------------------------------------------------------------------------
THandle FileCreate(const UnicodeString FileName, int Rights)
{
  return FileCreate(FileName, fmOpenReadWrite, Rights);
}
//---------------------------------------------------------------------------
THandle FileCreate(const UnicodeString FileName)
{
  return FileCreate(FileName, fmOpenReadWrite, 0666);
}
//---------------------------------------------------------------------------
void FileClose(THandle Handle)
{
  if (Handle != InvalidHandle)
  {
    close(HandleToFd(Handle));
  }
}
//---------------------------------------------------------------------------
int FileRead(THandle Handle, void * Buffer, unsigned int Count)
{
  // Like Windows' ReadFile, reads until the count or the end of the file
  unsigned int Done = 0;
  while (Done < Count)
  {
    ssize_t Read = read(HandleToFd(Handle), static_cast<char *>(Buffer) + Done, Count - Done);
    if (Read < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      rtl::SetLastErrorFromErrno();
      return (Done > 0) ? static_cast<int>(Done) : -1;
    }
    if (Read == 0)
    {
      break;
    }
    Done += static_cast<unsigned int>(Read);
  }
  return static_cast<int>(Done);
}
//---------------------------------------------------------------------------
int FileWrite(THandle Handle, const void * Buffer, unsigned int Count)
{
  unsigned int Done = 0;
  while (Done < Count)
  {
    ssize_t Written = write(HandleToFd(Handle), static_cast<const char *>(Buffer) + Done, Count - Done);
    if (Written < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      rtl::SetLastErrorFromErrno();
      return -1;
    }
    Done += static_cast<unsigned int>(Written);
  }
  return static_cast<int>(Done);
}
//---------------------------------------------------------------------------
int FileRead(THandle Handle, TBytes & Buffer, unsigned int Offset, unsigned int Count)
{
  if (static_cast<unsigned long long>(Offset) + Count > static_cast<unsigned long long>(Buffer.Length))
  {
    rtl::ThrowRangeError(static_cast<int>(Offset + Count), Buffer.Length);
  }
  return FileRead(Handle, Buffer.data() + Offset, Count);
}
//---------------------------------------------------------------------------
int FileWrite(THandle Handle, const TBytes Buffer, unsigned int Offset, unsigned int Count)
{
  if (static_cast<unsigned long long>(Offset) + Count > static_cast<unsigned long long>(Buffer.Length))
  {
    rtl::ThrowRangeError(static_cast<int>(Offset + Count), Buffer.Length);
  }
  return FileWrite(Handle, Buffer.data() + Offset, Count);
}
//---------------------------------------------------------------------------
__int64 FileSeek(THandle Handle, const __int64 Offset, int Origin)
{
  int Whence = (Origin == 1) ? SEEK_CUR : ((Origin == 2) ? SEEK_END : SEEK_SET);
  off_t Result = lseek(HandleToFd(Handle), static_cast<off_t>(Offset), Whence);
  if (Result < 0)
  {
    rtl::SetLastErrorFromErrno();
  }
  return static_cast<__int64>(Result);
}
//---------------------------------------------------------------------------
int FileSeek(THandle Handle, int Offset, int Origin)
{
  return static_cast<int>(FileSeek(Handle, static_cast<__int64>(Offset), Origin));
}
//---------------------------------------------------------------------------
// FindFirst
//---------------------------------------------------------------------------
TDateTime TSearchRec::GetTimeStamp() const
{
  long long Value =
    ((static_cast<long long>(FindData.ftLastWriteTime.dwHighDateTime) << 32) | FindData.ftLastWriteTime.dwLowDateTime) -
    FileTimeUnixDelta;
  long long Seconds = Value / 10000000LL;
  long Nanoseconds = static_cast<long>((Value % 10000000LL) * 100);
  if (Nanoseconds < 0)
  {
    Seconds--;
    Nanoseconds += 1000000000L;
  }
  return rtl::DateTimeFromUnixTime(Seconds, Nanoseconds, true);
}
//---------------------------------------------------------------------------
int FindFirst(const UnicodeString Path, int Attr, TSearchRec & F)
{
  F.FindHandle = 0;
  F.ExcludeAttr = ~Attr & (faHidden | faSysFile | faVolumeID | faDirectory);
  F.PathOnly = ExtractFilePath(Path);
  F.Pattern = ExtractFileName(Path);
  if (F.Pattern == L"*.*")
  {
    F.Pattern = L"*";
  }

  std::unique_ptr<TFindState> State(new TFindState());
  if (F.Pattern.IsEmpty())
  {
    SetLastError(ERROR_FILE_NOT_FOUND);
    return ERROR_FILE_NOT_FOUND;
  }
  if (!HasWildcards(F.Pattern))
  {
    State->Single = true;
  }
  else
  {
    std::string Dir = Native(F.PathOnly.IsEmpty() ? UnicodeString(L".") : F.PathOnly);
    State->Dir = opendir(Dir.c_str());
    if (State->Dir == nullptr)
    {
      int Result = (errno == ENOENT) ? ERROR_PATH_NOT_FOUND : static_cast<int>(rtl::ErrnoToWinError(errno));
      SetLastError(Result);
      return Result;
    }
  }
  F.FindHandle = reinterpret_cast<THandle>(State.release());
  int Result = FindMatchingFile(F);
  if (Result != 0)
  {
    FindClose(F);
    if (Result == ERROR_NO_MORE_FILES)
    {
      Result = ERROR_FILE_NOT_FOUND;
    }
    SetLastError(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
int FindNext(TSearchRec & F)
{
  int Result = FindMatchingFile(F);
  if (Result != 0)
  {
    SetLastError(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
void FindClose(TSearchRec & F)
{
  TFindState * State = reinterpret_cast<TFindState *>(F.FindHandle);
  if (State != nullptr)
  {
    if (State->Dir != nullptr)
    {
      closedir(State->Dir);
    }
    delete State;
    F.FindHandle = 0;
  }
}
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
