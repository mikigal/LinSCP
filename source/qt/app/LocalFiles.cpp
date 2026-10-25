// Operations on the local files of the GUI.
//---------------------------------------------------------------------------
#include "LocalFiles.h"

#include <RemoteFiles.h>
#include <Terminal.h>

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>
//---------------------------------------------------------------------------
static TRemoteToken UserToken(uid_t Uid)
{
  TRemoteToken Result;
  Result.ID = Uid;
  std::vector<char> Buffer(16384);
  struct passwd Entry;
  struct passwd * Found = NULL;
  if ((getpwuid_r(Uid, &Entry, Buffer.data(), Buffer.size(), &Found) == 0) && (Found != NULL))
  {
    Result.Name = UTF8ToString(Found->pw_name);
  }
  else
  {
    Result.Name = IntToStr(static_cast<int>(Uid));
  }
  return Result;
}
//---------------------------------------------------------------------------
static TRemoteToken GroupToken(gid_t Gid)
{
  TRemoteToken Result;
  Result.ID = Gid;
  std::vector<char> Buffer(16384);
  struct group Entry;
  struct group * Found = NULL;
  if ((getgrgid_r(Gid, &Entry, Buffer.data(), Buffer.size(), &Found) == 0) && (Found != NULL))
  {
    Result.Name = UTF8ToString(Found->gr_name);
  }
  else
  {
    Result.Name = IntToStr(static_cast<int>(Gid));
  }
  return Result;
}
//---------------------------------------------------------------------------
static void RaiseFileError(int Error, const UnicodeString & Message)
{
  throw ExtException(Message, UnicodeString(strerror(Error)));
}
//---------------------------------------------------------------------------
TStrings * CreateLocalFileList(TStrings * FileNames)
{
  std::unique_ptr<TStringList> Result(new TStringList());
  Result->OwnsObjects = true;
  for (int Index = 0; Index < FileNames->Count; Index++)
  {
    UnicodeString FileName = FileNames->Strings[Index];
    UTF8String Path = UTF8Encode(FileName);
    struct stat Stat;
    if (lstat(Path.c_str(), &Stat) != 0)
    {
      RaiseFileError(errno, FMTLOAD(FILE_NOT_EXISTS, (FileName)));
    }
    std::unique_ptr<TRemoteFile> File(new TRemoteFile());
    File->FileName = ExtractFileName(FileName);
    File->FullFileName = FileName;
    if (S_ISLNK(Stat.st_mode))
    {
      File->Type = FILETYPE_SYMLINK;
      char Target[PATH_MAX];
      ssize_t Length = readlink(Path.c_str(), Target, sizeof(Target) - 1);
      if (Length >= 0)
      {
        Target[Length] = '\0';
        File->LinkTo = UTF8ToString(Target);
      }
      // The properties of the target, as the local panel shows them
      struct stat TargetStat;
      if (stat(Path.c_str(), &TargetStat) == 0)
      {
        Stat = TargetStat;
      }
    }
    else
    {
      File->Type = S_ISDIR(Stat.st_mode) ? FILETYPE_DIRECTORY : FILETYPE_DEFAULT;
    }
    File->Size = S_ISDIR(Stat.st_mode) ? 0 : Stat.st_size;
    File->Rights->Number = static_cast<unsigned short>(Stat.st_mode & TRights::rfAllSpecials);
    File->Owner = UserToken(Stat.st_uid);
    File->Group = GroupToken(Stat.st_gid);
    File->Modification = UnixToDateTime(Stat.st_mtime, dstmUnix);
    Result->AddObject(FileName, File.release());
  }
  return Result.release();
}
//---------------------------------------------------------------------------
static void CalculateLocalFileSize(const std::string & Path, bool TopLevel, __int64 & Size, TCalculateSizeStats & Stats)
{
  struct stat Stat;
  if (lstat(Path.c_str(), &Stat) != 0)
  {
    RaiseFileError(errno, FMTLOAD(FILE_NOT_EXISTS, (UTF8ToString(Path.c_str()))));
  }
  bool SymLink = S_ISLNK(Stat.st_mode);
  if (SymLink)
  {
    struct stat TargetStat;
    if (stat(Path.c_str(), &TargetStat) == 0)
    {
      Stat = TargetStat;
    }
    Stats.SymLinks++;
  }

  if (S_ISDIR(Stat.st_mode))
  {
    if (!SymLink)
    {
      DIR * Directory = opendir(Path.c_str());
      if (Directory == NULL)
      {
        RaiseFileError(errno, FMTLOAD(LIST_DIR_ERROR, (UTF8ToString(Path.c_str()))));
      }
      ::rtl::TFinally Finally([&]() { closedir(Directory); });
      struct dirent * Entry;
      while ((Entry = readdir(Directory)) != NULL)
      {
        if ((strcmp(Entry->d_name, ".") != 0) && (strcmp(Entry->d_name, "..") != 0))
        {
          CalculateLocalFileSize(Path + '/' + Entry->d_name, false, Size, Stats);
        }
      }
    }
    Stats.Directories++;
  }
  else
  {
    Size += Stat.st_size;
    Stats.Files++;
  }
  DebugUsedParam(TopLevel);
}
//---------------------------------------------------------------------------
void CalculateLocalFilesSize(TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats)
{
  Size = 0;
  for (int Index = 0; Index < FileList->Count; Index++)
  {
    CalculateLocalFileSize(UTF8Encode(FileList->Strings[Index]).c_str(), true, Size, Stats);
  }
}
//---------------------------------------------------------------------------
static void ChangeLocalFileRights(const std::string & Path, bool TopLevel, const TRemoteProperties & Properties)
{
  struct stat Stat;
  // The files given follow their links (as chmod does), the links found
  // in the directories are skipped
  if ((TopLevel ? stat(Path.c_str(), &Stat) : lstat(Path.c_str(), &Stat)) != 0)
  {
    RaiseFileError(errno, FMTLOAD(CANT_SET_ATTRS, (UTF8ToString(Path.c_str()))));
  }
  if (S_ISLNK(Stat.st_mode))
  {
    return;
  }
  bool IsDirectory = S_ISDIR(Stat.st_mode);
  TRights Rights =
    TRights(static_cast<unsigned short>(Stat.st_mode & TRights::rfAllSpecials)).Combine(Properties.Rights);
  if (IsDirectory && Properties.AddXToDirectories)
  {
    Rights.AddExecute();
  }
  mode_t Mode = static_cast<unsigned short>(Rights);
  if ((Mode != (Stat.st_mode & TRights::rfAllSpecials)) && (chmod(Path.c_str(), Mode) != 0))
  {
    RaiseFileError(errno, FMTLOAD(CANT_SET_ATTRS, (UTF8ToString(Path.c_str()))));
  }

  if (IsDirectory && Properties.Recursive)
  {
    DIR * Directory = opendir(Path.c_str());
    if (Directory == NULL)
    {
      RaiseFileError(errno, FMTLOAD(LIST_DIR_ERROR, (UTF8ToString(Path.c_str()))));
    }
    ::rtl::TFinally Finally([&]() { closedir(Directory); });
    struct dirent * Entry;
    while ((Entry = readdir(Directory)) != NULL)
    {
      if ((strcmp(Entry->d_name, ".") != 0) && (strcmp(Entry->d_name, "..") != 0))
      {
        ChangeLocalFileRights(Path + '/' + Entry->d_name, false, Properties);
      }
    }
  }
}
//---------------------------------------------------------------------------
void ChangeLocalFilesProperties(TStrings * FileList, const TRemoteProperties & Properties)
{
  if (Properties.Valid.Contains(vpRights))
  {
    for (int Index = 0; Index < FileList->Count; Index++)
    {
      ChangeLocalFileRights(UTF8Encode(FileList->Strings[Index]).c_str(), true, Properties);
    }
  }
}
//---------------------------------------------------------------------------
