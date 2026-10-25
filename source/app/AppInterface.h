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
class TProgramParams;
bool __fastcall CheckSafe(TProgramParams * Params);
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
// The desktop session is KDE Plasma (XDG_CURRENT_DESKTOP)
bool IsKdeDesktop();
typedef int __fastcall (*TCalculateWidth)(UnicodeString Text, void * Arg);
// Aligns the parts of lines after a tab
void __fastcall ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg);
//---------------------------------------------------------------------------
#endif
