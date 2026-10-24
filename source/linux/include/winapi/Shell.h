// Shell functions of the Windows API (shlobj.h, shellapi.h, shlwapi.h) on
// the freedesktop.org conventions.
//---------------------------------------------------------------------------
#ifndef WinApiShellH
#define WinApiShellH
//---------------------------------------------------------------------------
#include <winapi/Handles.h>
//---------------------------------------------------------------------------
// Folders, mapped to the XDG base and user directories:
// CSIDL_APPDATA: $XDG_CONFIG_HOME, CSIDL_LOCAL_APPDATA: $XDG_DATA_HOME,
// CSIDL_PROFILE: $HOME, CSIDL_PERSONAL: XDG_DOCUMENTS_DIR,
// CSIDL_DESKTOPDIRECTORY: XDG_DESKTOP_DIR (from user-dirs.dirs).
#define CSIDL_DESKTOP 0x0000
#define CSIDL_PERSONAL 0x0005
#define CSIDL_DESKTOPDIRECTORY 0x0010
#define CSIDL_APPDATA 0x001a
#define CSIDL_LOCAL_APPDATA 0x001c
#define CSIDL_COMMON_APPDATA 0x0023
#define CSIDL_PROFILE 0x0028
#define CSIDL_FLAG_CREATE 0x8000
#define SHGFP_TYPE_CURRENT 0
#define SHGFP_TYPE_DEFAULT 1
HRESULT SHGetFolderPath(HWND Owner, int Folder, HANDLE Token, DWORD Flags, LPWSTR Path);
BOOL SHGetSpecialFolderPath(HWND Owner, LPWSTR Path, int Folder, BOOL Create);
//---------------------------------------------------------------------------
// File operations: FO_DELETE only, to the trash (freedesktop.org trash
// specification, the home trash) with FOF_ALLOWUNDO, permanently otherwise
#define FO_MOVE 0x0001
#define FO_COPY 0x0002
#define FO_DELETE 0x0003
#define FO_RENAME 0x0004
#define FOF_MULTIDESTFILES 0x0001
#define FOF_SILENT 0x0004
#define FOF_RENAMEONCOLLISION 0x0008
#define FOF_NOCONFIRMATION 0x0010
#define FOF_ALLOWUNDO 0x0040
#define FOF_FILESONLY 0x0080
#define FOF_NOCONFIRMMKDIR 0x0200
#define FOF_NOERRORUI 0x0400
typedef WORD FILEOP_FLAGS;
struct SHFILEOPSTRUCT
{
  HWND hwnd;
  UINT wFunc;
  // Lists of NUL-terminated names, terminated by an empty name
  LPCWSTR pFrom;
  LPCWSTR pTo;
  FILEOP_FLAGS fFlags;
  BOOL fAnyOperationsAborted;
  LPVOID hNameMappings;
  LPCWSTR lpszProgressTitle;
};
typedef SHFILEOPSTRUCT * LPSHFILEOPSTRUCT;
// Returns 0 or a Windows error code
int SHFileOperation(LPSHFILEOPSTRUCT FileOp);
//---------------------------------------------------------------------------
// File type information: the type name (as the Windows default: "<EXT>
// File", "File folder") and icon 0 (there is no system image list)
#define SHGFI_ICON 0x000000100
#define SHGFI_DISPLAYNAME 0x000000200
#define SHGFI_TYPENAME 0x000000400
#define SHGFI_ATTRIBUTES 0x000000800
#define SHGFI_SYSICONINDEX 0x000004000
#define SHGFI_SMALLICON 0x000000001
#define SHGFI_USEFILEATTRIBUTES 0x000000010
struct SHFILEINFOW
{
  HICON hIcon;
  int iIcon;
  DWORD dwAttributes;
  WCHAR szDisplayName[MAX_PATH];
  WCHAR szTypeName[80];
};
typedef SHFILEINFOW SHFILEINFO;
// Delphi's name (Winapi.ShellAPI)
typedef SHFILEINFOW TSHFileInfoW;
typedef SHFILEINFOW TSHFileInfo;
DWORD_PTR SHGetFileInfo(LPCWSTR Path, DWORD FileAttributes, SHFILEINFOW * Info, UINT InfoSize, UINT Flags);
//---------------------------------------------------------------------------
// Windows path syntax (drive letters, UNC, backslashes)
BOOL PathIsRelative(LPCWSTR Path);
LPCWSTR PathSkipRoot(LPCWSTR Path);
// Natural ("logical") order: case-insensitive, digit runs compare as numbers
int StrCmpLogicalW(LPCWSTR String1, LPCWSTR String2);
//---------------------------------------------------------------------------
// The MIME type of a file name (urlmon.h), from the freedesktop.org shared
// MIME-info database (globs2 in $XDG_DATA_HOME/mime and $XDG_DATA_DIRS/mime);
// only FMFD_URLASFILENAME (no content sniffing). The type is allocated with
// CoTaskMemAlloc.
#define FMFD_DEFAULT 0x00000000
#define FMFD_URLASFILENAME 0x00000001
struct IBindCtx;
HRESULT FindMimeFromData(
  IBindCtx * BindContext, LPCWSTR Url, LPVOID Buffer, DWORD Size, LPCWSTR MimeProposed, DWORD MimeFlags,
  LPWSTR * MimeOut, DWORD Reserved);
//---------------------------------------------------------------------------
#endif
