// Declarations of the application layer shared by the front ends, for the
// ported files of upstream's source/windows (which has them in
// WinInterface.h, VCLCommon.h and GUITools.h) and for the front ends.
//---------------------------------------------------------------------------
#ifndef AppInterfaceH
#define AppInterfaceH
//---------------------------------------------------------------------------
#include <Interface.h>
//---------------------------------------------------------------------------
// Command line switches (WinInterface.h)
#define UPLOAD_IF_ANY_SWITCH L"UploadIfAny"
#define UPLOAD_SWITCH L"Upload"
#define SYNCHRONIZE_SWITCH L"Synchronize"
#define KEEP_UP_TO_DATE_SWITCH L"KeepUpToDate"
#define UNSAFE_SWITCH L"Unsafe"
#define DEFAULTS_SWITCH L"Defaults"
#define NEWINSTANCE_SWICH L"NewInstance"
#define KEYGEN_SWITCH L"KeyGen"
#define KEYGEN_OUTPUT_SWITCH L"Output"
#define KEYGEN_COMMENT_SWITCH L"Comment"
#define KEYGEN_CHANGE_PASSPHRASE_SWITCH L"ChangePassphrase"
#define KEYGEN_CERTIFICATE_SWITCH L"Certificate"
#define LOG_SWITCH L"Log"
#define LOGSIZE_SWITCH L"LogSize"
#define LOGSIZE_SEPARATOR L"*"
#define INI_SWITCH L"Ini"
#define RAW_CONFIG_SWITCH L"RawConfig"
#define FINGERPRINTSCAN_SWITCH L"FingerprintScan"
#define INFO_SWITCH L"Info"
#define EXPLORE_SWITCH L"Explore"
#define EXPLORE_OLD_SWITCH L"Browse"
#define NOINTERACTIVEINPUT_SWITCH L"NoInteractiveInput"
#define STDOUT_SWITCH L"StdOut"
#define STDIN_SWITCH L"StdIn"
#define STDINOUT_BINARY_VALUE L"binary"
#define STDINOUT_CHUNKED_VALUE L"chunked"
#define COPYID_SWITCH L"CopyId"
#define IDENTITY_SWITCH L"Identity"
#define WORKSPACE_SWITCH L"Workspace"
//---------------------------------------------------------------------------
extern const UnicodeString AppName;
//---------------------------------------------------------------------------
// Sizes as upstream's file panels format them (packages/filemng/BaseUtils.pas)
enum TFormatBytesStyle : int { fbNone, fbKilobytes, fbShort };
// A color of the VCL (0x00BBGGRR); 0 is the default color in the
// configuration
typedef int TColor;
//---------------------------------------------------------------------------
class TOptions;
class TProgramParams;
bool __fastcall CheckSafe(TOptions * Params);
void __fastcall CheckLogParam(TProgramParams * Params);
bool __fastcall CheckXmlLogParam(TProgramParams * Params);
// Applies the switches that configure logging and the default transfer
// settings (/loglevel, /logsize, /rawtransfersettings, /privatekey), as
// upstream's Execute does before it runs a front end
void __fastcall ApplyCommandLineConfiguration(TProgramParams * Params);
//---------------------------------------------------------------------------
UnicodeString __fastcall FormatMainFormCaption(
  const UnicodeString & Caption, const UnicodeString & SessionName = UnicodeString());
UnicodeString __fastcall UniqTempDir(const UnicodeString BaseDir,
  const UnicodeString Identity, bool Mask = false);
// Files of the user's own on a system of several users, which Windows keeps
// in folders per user: the configuration (with the passwords) and the
// temporary directories (with the downloaded files).
// Removes the permissions of the group and of others from a file or
// directory of the user (following links); false when it does not exist, is
// another user's or cannot be changed
bool MakePrivate(const UnicodeString & Path);
// Creates a new directory accessible to the user only; false (with errno)
// when it exists already or cannot be created
bool CreatePrivateDirectory(const UnicodeString & Path);
// The path is a directory of the user, not a link
bool IsOwnDirectory(const UnicodeString & Path);
// Makes sure that the directory exists and is private: creates it so (with
// its parents), or makes an existing directory of the user private; false
// when it is another user's or a link (in a shared directory, such as /tmp)
bool ForcePrivateDirectory(const UnicodeString & Path);
// The desktop session is KDE Plasma (XDG_CURRENT_DESKTOP)
bool IsKdeDesktop();
// The SSH client to open the sessions in (upstream's PuttyPath, putty.exe):
// PuTTY when installed, else OpenSSH's ssh in a terminal emulator
UnicodeString DefaultPuttyPath();
typedef int __fastcall (*TCalculateWidth)(UnicodeString Text, void * Arg);
// Aligns the parts of lines after a tab
void __fastcall ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg);
//---------------------------------------------------------------------------
#endif
