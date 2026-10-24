// System information, environment, locale and module functions of the
// Windows API, see winapi/System.h.
//---------------------------------------------------------------------------
#include <winapi/System.h>
#include <winapi/TimeApi.h>

#include <System.hpp>
#include <StrUtils.hpp>
#include <SysUtils.hpp>

#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <pwd.h>
#include <unistd.h>
//---------------------------------------------------------------------------
#ifndef OPENSCP_CORE_VERSION
#error OPENSCP_CORE_VERSION must be defined (the version of the upstream WinSCP core)
#endif
#ifndef OPENSCP_RELEASE_TYPE
#define OPENSCP_RELEASE_TYPE "stable"
#endif
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// Copies a string to a buffer of Size characters the Windows way: the length
// without the terminator on success, the size needed with it otherwise
DWORD CopyResult(const UnicodeString & Value, LPWSTR Buffer, DWORD Size)
{
  DWORD Length = static_cast<DWORD>(Value.Length());
  if ((Buffer == NULL) || (Length + 1 > Size))
  {
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return Length + 1;
  }
  wmemcpy(Buffer, Value.c_str(), Length + 1);
  return Length;
}
//---------------------------------------------------------------------------
char Kernel32Module;
//---------------------------------------------------------------------------
UnicodeString LoginName()
{
  struct passwd Entry;
  struct passwd * Found = nullptr;
  std::vector<char> Buffer(16384);
  if ((getpwuid_r(geteuid(), &Entry, Buffer.data(), Buffer.size(), &Found) == 0) && (Found != nullptr))
  {
    return rtl::BytesToFileName(Found->pw_name);
  }
  const char * Name = getenv("USER");
  return (Name != nullptr) ? rtl::BytesToFileName(Name) : UnicodeString();
}
//---------------------------------------------------------------------------
std::string ExecutablePath()
{
  char Buffer[4096];
  ssize_t Length = readlink("/proc/self/exe", Buffer, sizeof(Buffer) - 1);
  return (Length > 0) ? std::string(Buffer, Length) : std::string();
}
//---------------------------------------------------------------------------
// The version information block: the fixed information, one translation and
// the strings, as consecutive NUL-terminated names and values
struct TVersionBlockHeader
{
  DWORD Magic;
  DWORD Size;
  VS_FIXEDFILEINFO Fixed;
  DWORD Translation;
  DWORD StringsLength;
};
const DWORD VersionBlockMagic = 0x4F53434E; // "NCSO"
//---------------------------------------------------------------------------
const std::vector<char> & VersionBlock()
{
  static const std::vector<char> Block = []()
    {
      UnicodeString Version = OPENSCP_CORE_VERSION;
      unsigned int Parts[4] = { 0, 0, 0, 0 };
      UnicodeString Rest = Version;
      for (int Index = 0; (Index < 4) && !Rest.IsEmpty(); Index++)
      {
        int P = Rest.Pos(L".");
        UnicodeString Part = (P > 0) ? Rest.SubString(1, P - 1) : Rest;
        Rest = (P > 0) ? Rest.SubString(P + 1, Rest.Length() - P) : UnicodeString();
        Parts[Index] = static_cast<unsigned int>(StrToIntDef(Part, 0));
      }
      TVersionBlockHeader Header = {};
      Header.Magic = VersionBlockMagic;
      Header.Fixed.dwSignature = 0xFEEF04BD;
      Header.Fixed.dwStrucVersion = 0x00010000;
      Header.Fixed.dwFileVersionMS = (Parts[0] << 16) | Parts[1];
      Header.Fixed.dwFileVersionLS = (Parts[2] << 16) | Parts[3];
      Header.Fixed.dwProductVersionMS = Header.Fixed.dwFileVersionMS;
      Header.Fixed.dwProductVersionLS = Header.Fixed.dwFileVersionLS;
      // en-US, Unicode
      Header.Translation = 0x04B00409;

      const wchar_t * Strings[][2] = {
        { L"CompanyName", L"OpenSCP" },
        { L"FileDescription", L"OpenSCP: SFTP, FTP, WebDAV, S3 and SCP client" },
        { L"FileVersion", Version.c_str() },
        { L"InternalName", L"openscp" },
        { L"LegalCopyright", L"(c) 2000-2026 Martin Prikryl; OpenSCP contributors" },
        { L"OriginalFilename", L"openscp" },
        { L"ProductName", L"OpenSCP" },
        { L"ProductVersion", Version.c_str() },
        { L"ReleaseType", L"" OPENSCP_RELEASE_TYPE },
        { L"WWW", L"https://winscp.net/" },
      };
      std::wstring StringsData;
      for (auto & String : Strings)
      {
        StringsData += String[0];
        StringsData += L'\0';
        StringsData += String[1];
        StringsData += L'\0';
      }
      Header.StringsLength = static_cast<DWORD>(StringsData.size());
      Header.Size = static_cast<DWORD>(sizeof(Header) + StringsData.size() * sizeof(wchar_t));
      std::vector<char> Result(Header.Size);
      memcpy(Result.data(), &Header, sizeof(Header));
      memcpy(Result.data() + sizeof(Header), StringsData.data(), StringsData.size() * sizeof(wchar_t));
      return Result;
    }();
  return Block;
}
//---------------------------------------------------------------------------
bool IsExecutable(LPCWSTR FileName)
{
  std::string Executable = ExecutablePath();
  std::string Name = rtl::FileNameToBytes(FileName);
  if (Executable.empty() || Name.empty())
  {
    return false;
  }
  char Resolved[4096];
  return (realpath(Name.c_str(), Resolved) != nullptr) && (Executable == Resolved);
}
//---------------------------------------------------------------------------
struct TMapping
{
  uintptr_t Start;
  uintptr_t End;
  bool Private;
};
//---------------------------------------------------------------------------
std::vector<TMapping> ReadMappings()
{
  std::vector<TMapping> Result;
  std::ifstream Maps("/proc/self/maps");
  std::string Line;
  while (std::getline(Maps, Line))
  {
    std::istringstream Stream(Line);
    std::string Range;
    std::string Permissions;
    Stream >> Range >> Permissions;
    size_t Dash = Range.find('-');
    if ((Dash == std::string::npos) || (Permissions.size() < 4))
    {
      continue;
    }
    TMapping Mapping;
    Mapping.Start = std::stoull(Range.substr(0, Dash), nullptr, 16);
    Mapping.End = std::stoull(Range.substr(Dash + 1), nullptr, 16);
    Mapping.Private = (Permissions[3] == 'p');
    Result.push_back(Mapping);
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
DWORD GetTempPath(DWORD BufferLength, LPWSTR Buffer)
{
  const char * Directory = getenv("TMPDIR");
  UnicodeString Path = ((Directory != nullptr) && (*Directory != '\0')) ? rtl::BytesToFileName(Directory) : UnicodeString(L"/tmp");
  return CopyResult(IncludeTrailingPathDelimiter(Path), Buffer, BufferLength);
}
//---------------------------------------------------------------------------
DWORD ExpandEnvironmentStrings(LPCWSTR Source, LPWSTR Destination, DWORD Size)
{
  UnicodeString Result;
  const wchar_t * P = Source;
  while (*P != L'\0')
  {
    const wchar_t * End = (*P == L'%') ? wcschr(P + 1, L'%') : nullptr;
    if ((End != nullptr) && (End > P + 1))
    {
      UnicodeString Name(P + 1, static_cast<int>(End - P - 1));
      const char * Value = getenv(rtl::FileNameToBytes(Name).c_str());
      if (Value != nullptr)
      {
        Result += rtl::BytesToFileName(Value);
        P = End + 1;
        continue;
      }
    }
    Result += *P;
    P++;
  }
  // Unlike other functions, the result includes the terminator
  DWORD Length = static_cast<DWORD>(Result.Length()) + 1;
  if ((Destination != NULL) && (Length <= Size))
  {
    wmemcpy(Destination, Result.c_str(), Length);
  }
  return Length;
}
//---------------------------------------------------------------------------
LCID GetUserDefaultLCID()
{
  return 1033;
}
//---------------------------------------------------------------------------
LCID GetSystemDefaultLCID()
{
  return 1033;
}
//---------------------------------------------------------------------------
LANGID GetUserDefaultUILanguage()
{
  return 1033;
}
//---------------------------------------------------------------------------
UINT GetACP()
{
  return CP_UTF8;
}
//---------------------------------------------------------------------------
UINT GetOEMCP()
{
  return CP_UTF8;
}
//---------------------------------------------------------------------------
BOOL GetCPInfoEx(UINT CodePage, DWORD, LPCPINFOEX CPInfoEx)
{
  if ((CodePage == CP_ACP) || (CodePage == CP_OEMCP))
  {
    CodePage = CP_UTF8;
  }
  memset(CPInfoEx, 0, sizeof(*CPInfoEx));
  CPInfoEx->CodePage = CodePage;
  CPInfoEx->DefaultChar[0] = '?';
  CPInfoEx->UnicodeDefaultChar = L'?';
  if (CodePage == CP_UTF8)
  {
    CPInfoEx->MaxCharSize = 4;
    wcscpy(CPInfoEx->CodePageName, L"65001  (UTF-8)");
    return TRUE;
  }
  std::string Name = rtl::CodePageName(CodePage);
  if (Name.empty())
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  CPInfoEx->MaxCharSize = 1;
  UnicodeString Description = UnicodeString(static_cast<int>(CodePage)) + L"  (" + UnicodeString(Name.c_str()) + L")";
  wcsncpy(CPInfoEx->CodePageName, Description.c_str(), MAX_PATH - 1);
  return TRUE;
}
//---------------------------------------------------------------------------
int lstrcmp(LPCWSTR String1, LPCWSTR String2)
{
  return AnsiCompareStr(String1, String2);
}
//---------------------------------------------------------------------------
int lstrcmpi(LPCWSTR String1, LPCWSTR String2)
{
  return AnsiCompareText(String1, String2);
}
//---------------------------------------------------------------------------
int lstrlen(LPCWSTR String)
{
  return (String != NULL) ? static_cast<int>(wcslen(String)) : 0;
}
//---------------------------------------------------------------------------
FILE * _wfopen(const wchar_t * FileName, const wchar_t * Mode)
{
  std::string ModeBytes;
  for (const wchar_t * P = Mode; *P != L'\0'; P++)
  {
    // "t" (text) and "ccs=" are Windows extensions; "b" is accepted by POSIX
    if (*P == L',')
    {
      break;
    }
    if (*P != L't')
    {
      ModeBytes += static_cast<char>(*P);
    }
  }
  return fopen(rtl::FileNameToBytes(FileName).c_str(), ModeBytes.c_str());
}
//---------------------------------------------------------------------------
wchar_t * _wcsdup(const wchar_t * String)
{
  return wcsdup(String);
}
//---------------------------------------------------------------------------
HMODULE LoadLibrary(LPCWSTR)
{
  SetLastError(ERROR_MOD_NOT_FOUND);
  return NULL;
}
//---------------------------------------------------------------------------
HMODULE LoadLibraryEx(LPCWSTR FileName, HANDLE, DWORD)
{
  return LoadLibrary(FileName);
}
//---------------------------------------------------------------------------
BOOL FreeLibrary(HMODULE)
{
  return TRUE;
}
//---------------------------------------------------------------------------
HMODULE GetModuleHandle(LPCWSTR ModuleName)
{
  if (ModuleName == NULL)
  {
    return HInstance;
  }
  if (SameText(ModuleName, kernel32) || SameText(ModuleName, L"kernel32"))
  {
    return reinterpret_cast<HMODULE>(&Kernel32Module);
  }
  SetLastError(ERROR_MOD_NOT_FOUND);
  return NULL;
}
//---------------------------------------------------------------------------
FARPROC GetProcAddress(HMODULE Module, LPCSTR ProcName)
{
  if ((Module == reinterpret_cast<HMODULE>(&Kernel32Module)) &&
      (strcmp(ProcName, "GetTimeZoneInformationForYear") == 0))
  {
    // Through void (*)(), the generic function pointer type
    return reinterpret_cast<FARPROC>(reinterpret_cast<void (*)()>(&GetTimeZoneInformationForYear));
  }
  SetLastError(ERROR_PROC_NOT_FOUND);
  return NULL;
}
//---------------------------------------------------------------------------
DWORD GetModuleFileName(HMODULE Module, LPWSTR FileName, DWORD Size)
{
  if (Module != HInstance)
  {
    SetLastError(ERROR_MOD_NOT_FOUND);
    return 0;
  }
  DWORD Result = CopyResult(rtl::BytesToFileName(ExecutablePath().c_str()), FileName, Size);
  // Windows truncates and returns the size of the buffer
  if (Result >= Size)
  {
    if (Size > 0)
    {
      UnicodeString Path = rtl::BytesToFileName(ExecutablePath().c_str());
      wmemcpy(FileName, Path.c_str(), Size - 1);
      FileName[Size - 1] = L'\0';
    }
    return Size;
  }
  return Result;
}
//---------------------------------------------------------------------------
int LoadString(HINSTANCE, UINT Id, LPWSTR Buffer, int BufferMax)
{
  UnicodeString Value = rtl::FindResourceString(static_cast<int>(Id));
  if (Value.IsEmpty() || (BufferMax <= 0))
  {
    SetLastError(ERROR_RESOURCE_NAME_NOT_FOUND);
    return 0;
  }
  int Length = std::min(Value.Length(), BufferMax - 1);
  wmemcpy(Buffer, Value.c_str(), Length);
  Buffer[Length] = L'\0';
  return Length;
}
//---------------------------------------------------------------------------
HGLOBAL GlobalFree(HGLOBAL Memory)
{
  free(Memory);
  return NULL;
}
//---------------------------------------------------------------------------
HLOCAL LocalFree(HLOCAL Memory)
{
  free(Memory);
  return NULL;
}
//---------------------------------------------------------------------------
SIZE_T VirtualQuery(LPCVOID Address, PMEMORY_BASIC_INFORMATION Buffer, SIZE_T Length)
{
  if (Length < sizeof(MEMORY_BASIC_INFORMATION))
  {
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return 0;
  }
  // Callers walk the address space upwards: read the mappings once per walk
  thread_local std::vector<TMapping> Mappings;
  thread_local uintptr_t LastAddress = 0;
  uintptr_t Value = reinterpret_cast<uintptr_t>(Address);
  if (Mappings.empty() || (Value <= LastAddress))
  {
    Mappings = ReadMappings();
  }
  LastAddress = Value;
  memset(Buffer, 0, sizeof(*Buffer));
  uintptr_t FreeStart = 0;
  for (const TMapping & Mapping : Mappings)
  {
    if (Value < Mapping.Start)
    {
      Buffer->BaseAddress = reinterpret_cast<PVOID>(Value);
      Buffer->AllocationBase = reinterpret_cast<PVOID>(FreeStart);
      Buffer->RegionSize = Mapping.Start - Value;
      Buffer->State = MEM_FREE;
      return sizeof(MEMORY_BASIC_INFORMATION);
    }
    if (Value < Mapping.End)
    {
      Buffer->BaseAddress = reinterpret_cast<PVOID>(Value);
      Buffer->AllocationBase = reinterpret_cast<PVOID>(Mapping.Start);
      Buffer->RegionSize = Mapping.End - Value;
      Buffer->State = MEM_COMMIT;
      Buffer->Type = Mapping.Private ? MEM_PRIVATE : MEM_MAPPED;
      return sizeof(MEMORY_BASIC_INFORMATION);
    }
    FreeStart = Mapping.End;
  }
  SetLastError(ERROR_INVALID_PARAMETER);
  return 0;
}
//---------------------------------------------------------------------------
BOOLEAN GetUserNameEx(EXTENDED_NAME_FORMAT, LPWSTR NameBuffer, PULONG Size)
{
  UnicodeString Name = LoginName();
  DWORD Result = CopyResult(Name, NameBuffer, *Size);
  if (Result >= *Size)
  {
    *Size = Result;
    return FALSE;
  }
  *Size = Result;
  return TRUE;
}
//---------------------------------------------------------------------------
BOOLEAN GetUserNameEx(EXTENDED_NAME_FORMAT NameFormat, LPWSTR NameBuffer, unsigned long * Size)
{
  ULONG Value = static_cast<ULONG>(*Size);
  BOOLEAN Result = GetUserNameEx(NameFormat, NameBuffer, &Value);
  *Size = Value;
  return Result;
}
//---------------------------------------------------------------------------
BOOL GetUserName(LPWSTR Buffer, LPDWORD Size)
{
  UnicodeString Name = LoginName();
  DWORD Result = CopyResult(Name, Buffer, *Size);
  if (Result >= *Size)
  {
    *Size = Result;
    return FALSE;
  }
  // Including the terminator
  *Size = Result + 1;
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL GetComputerName(LPWSTR Buffer, LPDWORD Size)
{
  char Name[256];
  if (gethostname(Name, sizeof(Name)) != 0)
  {
    rtl::SetLastErrorFromErrno();
    return FALSE;
  }
  Name[sizeof(Name) - 1] = '\0';
  DWORD Result = CopyResult(rtl::BytesToFileName(Name), Buffer, *Size);
  if (Result >= *Size)
  {
    SetLastError(ERROR_BUFFER_OVERFLOW);
    *Size = Result;
    return FALSE;
  }
  *Size = Result;
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL IsWow64Process(HANDLE, PBOOL Wow64Process)
{
  *Wow64Process = (sizeof(void *) == 8) ? TRUE : FALSE;
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL GetProductInfo(DWORD, DWORD, DWORD, DWORD, PDWORD ReturnedProductType)
{
  *ReturnedProductType = PRODUCT_UNDEFINED;
  return TRUE;
}
//---------------------------------------------------------------------------
DWORD GetFileVersionInfoSize(LPCWSTR FileName, LPDWORD Handle)
{
  if (Handle != NULL)
  {
    *Handle = 0;
  }
  if (!IsExecutable(FileName))
  {
    SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
    return 0;
  }
  return static_cast<DWORD>(VersionBlock().size());
}
//---------------------------------------------------------------------------
BOOL GetFileVersionInfo(LPCWSTR FileName, DWORD, DWORD Length, LPVOID Data)
{
  const std::vector<char> & Block = VersionBlock();
  if (!IsExecutable(FileName))
  {
    SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
    return FALSE;
  }
  if (Length < Block.size())
  {
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
    return FALSE;
  }
  memcpy(Data, Block.data(), Block.size());
  return TRUE;
}
//---------------------------------------------------------------------------
BOOL VerQueryValue(LPCVOID Block, LPCWSTR SubBlock, LPVOID * Buffer, PUINT Length)
{
  const TVersionBlockHeader * Header = static_cast<const TVersionBlockHeader *>(Block);
  if ((Header == NULL) || (Header->Magic != VersionBlockMagic))
  {
    SetLastError(ERROR_INVALID_DATA);
    return FALSE;
  }
  UnicodeString Path = SubBlock;
  if (Path == L"\\")
  {
    *Buffer = const_cast<VS_FIXEDFILEINFO *>(&Header->Fixed);
    *Length = sizeof(VS_FIXEDFILEINFO);
    return TRUE;
  }
  if (SameText(Path, L"\\VarFileInfo\\Translation"))
  {
    *Buffer = const_cast<DWORD *>(&Header->Translation);
    *Length = sizeof(DWORD);
    return TRUE;
  }
  const UnicodeString Prefix = L"\\StringFileInfo\\";
  if (StartsText(Prefix, Path))
  {
    UnicodeString Name = Path.SubString(Prefix.Length() + 1, Path.Length() - Prefix.Length());
    int P = Name.Pos(L"\\");
    // Translation of the strings, the only one there is
    Name = (P > 0) ? Name.SubString(P + 1, Name.Length() - P) : UnicodeString();
    const wchar_t * Strings = reinterpret_cast<const wchar_t *>(Header + 1);
    const wchar_t * End = Strings + Header->StringsLength;
    while (Strings < End)
    {
      const wchar_t * Value = Strings + wcslen(Strings) + 1;
      if (SameText(Name, Strings))
      {
        *Buffer = const_cast<wchar_t *>(Value);
        *Length = static_cast<UINT>(wcslen(Value) + 1);
        return TRUE;
      }
      Strings = Value + wcslen(Value) + 1;
    }
  }
  SetLastError(ERROR_RESOURCE_TYPE_NOT_FOUND);
  return FALSE;
}
//---------------------------------------------------------------------------
int MessageBox(HWND, LPCWSTR Text, LPCWSTR Caption, UINT)
{
  fprintf(stderr, "%s: %s\n",
    rtl::FileNameToBytes((Caption != NULL) ? Caption : L"").c_str(),
    rtl::FileNameToBytes((Text != NULL) ? Text : L"").c_str());
  return IDOK;
}
//---------------------------------------------------------------------------
BOOL IsBadWritePtr(LPVOID Pointer, UINT_PTR Size)
{
  return ((Pointer == NULL) && (Size > 0)) ? TRUE : FALSE;
}
//---------------------------------------------------------------------------
