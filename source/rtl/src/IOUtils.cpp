// System.IOUtils
//---------------------------------------------------------------------------
#include <rtl/IOUtils.h>

#include <cerrno>
#include <fcntl.h>
#include <filesystem>
#include <random>
#include <sys/stat.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Ioutils
{
//---------------------------------------------------------------------------
namespace
{
[[noreturn]] void RaiseIOError(const UnicodeString & Path)
{
  DWORD Error = GetLastError();
  if ((Error == ERROR_FILE_NOT_FOUND) || (Error == ERROR_PATH_NOT_FOUND))
  {
    throw EFileNotFoundException(L"File or directory not found: %s", ARRAYOFCONST((Path)));
  }
  throw EInOutError(L"%s: %s", ARRAYOFCONST((Path, SysErrorMessage(Error))));
}

void CheckValidPath(const UnicodeString & Path)
{
  if (!TPath::HasValidPathChars(Path, false))
  {
    throw EArgumentException(L"Invalid characters in path");
  }
}
} // namespace
//---------------------------------------------------------------------------
// TPath
//---------------------------------------------------------------------------
UnicodeString TPath::ChangeExtension(const UnicodeString Path, const UnicodeString Extension)
{
  UnicodeString Ext = Extension;
  if (!Ext.IsEmpty() && (Ext[1] != L'.'))
  {
    Ext = L"." + Ext;
  }
  return ChangeFileExt(Path, Ext);
}
//---------------------------------------------------------------------------
UnicodeString TPath::Combine(const UnicodeString Path1, const UnicodeString Path2)
{
  CheckValidPath(Path1);
  CheckValidPath(Path2);
  if (Path1.IsEmpty() || IsPathRooted(Path2))
  {
    return Path2;
  }
  if (Path2.IsEmpty())
  {
    return Path1;
  }
  return IncludeTrailingPathDelimiter(Path1) + Path2;
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetDirectoryName(const UnicodeString FileName)
{
  return ExcludeTrailingPathDelimiter(ExtractFilePath(FileName));
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetExtension(const UnicodeString FileName)
{
  UnicodeString Result = ExtractFileExt(FileName);
  return (Result == L".") ? UnicodeString() : Result;
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetFileName(const UnicodeString FileName)
{
  return ExtractFileName(FileName);
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetFileNameWithoutExtension(const UnicodeString FileName)
{
  return ChangeFileExt(ExtractFileName(FileName), UnicodeString());
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetFullPath(const UnicodeString Path)
{
  CheckValidPath(Path);
  return ExpandFileName(Path);
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetHomePath()
{
  return GetEnvironmentVariable(L"HOME");
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetDocumentsPath()
{
  return GetHomePath();
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetTempPath()
{
  UnicodeString Result = GetEnvironmentVariable(L"TMPDIR");
  if (Result.IsEmpty())
  {
    Result = L"/tmp";
  }
  return IncludeTrailingPathDelimiter(Result);
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetRandomFileName()
{
  static const wchar_t Chars[] = L"abcdefghijklmnopqrstuvwxyz0123456789";
  std::random_device Device;
  std::mt19937 Generator(Device());
  std::uniform_int_distribution<int> Distribution(0, 35);
  UnicodeString Result;
  // 8.3, as Delphi
  for (int I = 0; I < 11; I++)
  {
    if (I == 8)
    {
      Result += L'.';
    }
    Result += Chars[Distribution(Generator)];
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetTempFileName()
{
  std::string Template = rtl::FileNameToBytes(GetTempPath()) + "tmpXXXXXX";
  std::vector<char> Buffer(Template.begin(), Template.end());
  Buffer.push_back('\0');
  int Fd = mkstemp(Buffer.data());
  if (Fd < 0)
  {
    rtl::SetLastErrorFromErrno();
    RaiseIOError(GetTempPath());
  }
  close(Fd);
  return rtl::BytesToFileName(Buffer.data());
}
//---------------------------------------------------------------------------
UnicodeString TPath::GetPathRoot(const UnicodeString Path)
{
  return IsPathRooted(Path) ? UnicodeString(L"/") : UnicodeString();
}
//---------------------------------------------------------------------------
bool TPath::HasExtension(const UnicodeString Path)
{
  return !GetExtension(Path).IsEmpty();
}
//---------------------------------------------------------------------------
bool TPath::HasValidPathChars(const UnicodeString Path, const bool)
{
  // NUL is the only character a POSIX path cannot contain
  return (Path.Std().find(L'\0') == std::wstring::npos);
}
//---------------------------------------------------------------------------
bool TPath::HasValidFileNameChars(const UnicodeString FileName, const bool UseWildcards)
{
  return HasValidPathChars(FileName, UseWildcards) && (FileName.Pos(L"/") == 0);
}
//---------------------------------------------------------------------------
bool TPath::IsPathRooted(const UnicodeString Path)
{
  return !Path.IsEmpty() && (Path[1] == L'/');
}
//---------------------------------------------------------------------------
// TFile
//---------------------------------------------------------------------------
bool TFile::Exists(const UnicodeString Path, bool FollowLink)
{
  return FileExists(Path, FollowLink);
}
//---------------------------------------------------------------------------
void TFile::Delete(const UnicodeString Path)
{
  CheckValidPath(Path);
  if (!DeleteFile(Path) && (GetLastError() != ERROR_FILE_NOT_FOUND))
  {
    RaiseIOError(Path);
  }
}
//---------------------------------------------------------------------------
void TFile::Copy(const UnicodeString SourceFileName, const UnicodeString DestFileName, bool Overwrite)
{
  std::error_code Error;
  std::filesystem::copy_options Options =
    Overwrite ? std::filesystem::copy_options::overwrite_existing : std::filesystem::copy_options::none;
  if (!std::filesystem::copy_file(rtl::FileNameToBytes(SourceFileName), rtl::FileNameToBytes(DestFileName), Options, Error))
  {
    SetLastError(rtl::ErrnoToWinError(Error ? Error.value() : EEXIST));
    RaiseIOError(DestFileName);
  }
}
//---------------------------------------------------------------------------
void TFile::Move(const UnicodeString SourceFileName, const UnicodeString DestFileName)
{
  if (!RenameFile(SourceFileName, DestFileName))
  {
    RaiseIOError(SourceFileName);
  }
}
//---------------------------------------------------------------------------
TBytes TFile::ReadAllBytes(const UnicodeString Path)
{
  TFileStream Stream(Path, static_cast<Word>(fmOpenRead | fmShareDenyWrite));
  TBytes Result;
  Result.Length = static_cast<int>(Stream.Size);
  if (Result.Length > 0)
  {
    Stream.ReadBuffer(Result, Result.Length);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TFile::ReadAllText(const UnicodeString Path)
{
  TBytes Buffer = ReadAllBytes(Path);
  TEncoding * Encoding = nullptr;
  int PreambleLength = TEncoding::GetBufferEncoding(Buffer, Encoding);
  return Encoding->GetString(Buffer, PreambleLength, Buffer.Length - PreambleLength);
}
//---------------------------------------------------------------------------
UnicodeString TFile::ReadAllText(const UnicodeString Path, TEncoding * Encoding)
{
  TBytes Buffer = ReadAllBytes(Path);
  int PreambleLength = TEncoding::GetBufferEncoding(Buffer, Encoding);
  return Encoding->GetString(Buffer, PreambleLength, Buffer.Length - PreambleLength);
}
//---------------------------------------------------------------------------
void TFile::WriteAllBytes(const UnicodeString Path, const TBytes & Bytes)
{
  TFileStream Stream(Path, fmCreate);
  if (Bytes.Length > 0)
  {
    Stream.WriteBuffer(Bytes, Bytes.Length);
  }
}
//---------------------------------------------------------------------------
void TFile::WriteAllText(const UnicodeString Path, const UnicodeString Contents)
{
  TFileStream Stream(Path, fmCreate);
  TBytes Bytes = TEncoding::UTF8->GetBytes(Contents);
  if (Bytes.Length > 0)
  {
    Stream.WriteBuffer(Bytes, Bytes.Length);
  }
}
//---------------------------------------------------------------------------
void TFile::WriteAllText(const UnicodeString Path, const UnicodeString Contents, TEncoding * Encoding)
{
  TFileStream Stream(Path, fmCreate);
  TBytes Preamble = Encoding->GetPreamble();
  if (Preamble.Length > 0)
  {
    Stream.WriteBuffer(Preamble, Preamble.Length);
  }
  TBytes Bytes = Encoding->GetBytes(Contents);
  if (Bytes.Length > 0)
  {
    Stream.WriteBuffer(Bytes, Bytes.Length);
  }
}
//---------------------------------------------------------------------------
void TFile::AppendAllText(const UnicodeString Path, const UnicodeString Contents)
{
  THandle Handle = FileExists(Path) ? FileOpen(Path, fmOpenWrite) : FileCreate(Path);
  if (Handle == static_cast<THandle>(-1))
  {
    RaiseIOError(Path);
  }
  FileSeek(Handle, static_cast<__int64>(0), 2);
  TBytes Bytes = TEncoding::UTF8->GetBytes(Contents);
  int Written = (Bytes.Length > 0) ? FileWrite(Handle, Bytes, 0, Bytes.Length) : 0;
  FileClose(Handle);
  if (Written != Bytes.Length)
  {
    RaiseIOError(Path);
  }
}
//---------------------------------------------------------------------------
TDateTime TFile::GetLastWriteTime(const UnicodeString Path)
{
  TDateTime Result;
  if (!FileAge(Path, Result))
  {
    RaiseIOError(Path);
  }
  return Result;
}
//---------------------------------------------------------------------------
TDateTime TFile::GetLastWriteTimeUtc(const UnicodeString Path)
{
  struct stat Info;
  if (stat(rtl::FileNameToBytes(Path).c_str(), &Info) != 0)
  {
    rtl::SetLastErrorFromErrno();
    RaiseIOError(Path);
  }
  return rtl::DateTimeFromUnixTime(Info.st_mtim.tv_sec, Info.st_mtim.tv_nsec, false);
}
//---------------------------------------------------------------------------
__int64 TFile::GetSize(const UnicodeString Path)
{
  struct stat Info;
  if (stat(rtl::FileNameToBytes(Path).c_str(), &Info) != 0)
  {
    rtl::SetLastErrorFromErrno();
    RaiseIOError(Path);
  }
  return Info.st_size;
}
//---------------------------------------------------------------------------
// TDirectory
//---------------------------------------------------------------------------
bool TDirectory::Exists(const UnicodeString Path, bool FollowLink)
{
  return DirectoryExists(Path, FollowLink);
}
//---------------------------------------------------------------------------
void TDirectory::CreateDirectory(const UnicodeString Path)
{
  CheckValidPath(Path);
  if (!ForceDirectories(Path))
  {
    RaiseIOError(Path);
  }
}
//---------------------------------------------------------------------------
void TDirectory::Delete(const UnicodeString Path, const bool Recursive)
{
  std::error_code Error;
  std::string Native = rtl::FileNameToBytes(Path);
  bool Success = Recursive ? (std::filesystem::remove_all(Native, Error) != static_cast<std::uintmax_t>(-1)) : RemoveDir(Path);
  if (!Success || Error)
  {
    if (Error)
    {
      SetLastError(rtl::ErrnoToWinError(Error.value()));
    }
    RaiseIOError(Path);
  }
}
//---------------------------------------------------------------------------
bool TDirectory::IsEmpty(const UnicodeString Path)
{
  std::error_code Error;
  return std::filesystem::is_empty(rtl::FileNameToBytes(Path), Error);
}
//---------------------------------------------------------------------------
namespace
{
TStringDynArray FindEntries(const UnicodeString & Path, const UnicodeString & Pattern, bool Directories)
{
  std::vector<UnicodeString> Found;
  TSearchRec Rec;
  int Error = FindFirst(IncludeTrailingPathDelimiter(Path) + Pattern, faAnyFile, Rec);
  while (Error == 0)
  {
    bool IsDirectory = ((Rec.Attr & faDirectory) != 0);
    if ((IsDirectory == Directories) && (Rec.Name != L".") && (Rec.Name != L".."))
    {
      Found.push_back(IncludeTrailingPathDelimiter(Path) + Rec.Name);
    }
    Error = FindNext(Rec);
  }
  FindClose(Rec);
  TStringDynArray Result;
  Result.Length = static_cast<int>(Found.size());
  for (size_t Index = 0; Index < Found.size(); Index++)
  {
    Result[static_cast<int>(Index)] = Found[Index];
  }
  return Result;
}
} // namespace
//---------------------------------------------------------------------------
TStringDynArray TDirectory::GetFiles(const UnicodeString Path, const UnicodeString SearchPattern)
{
  return FindEntries(Path, SearchPattern, false);
}
//---------------------------------------------------------------------------
TStringDynArray TDirectory::GetDirectories(const UnicodeString Path, const UnicodeString SearchPattern)
{
  return FindEntries(Path, SearchPattern, true);
}
//---------------------------------------------------------------------------
void TDirectory::SetCurrentDirectory(const UnicodeString Path)
{
  if (!SetCurrentDir(Path))
  {
    RaiseIOError(Path);
  }
}
//---------------------------------------------------------------------------
} // namespace Ioutils
} // namespace System
