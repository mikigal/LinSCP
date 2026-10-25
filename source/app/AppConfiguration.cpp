// The configuration of the application layer.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <Bookmarks.h>
#include <Cryptography.h>
#include <Terminal.h>
#include <Security.h>

#include "AppConfiguration.h"
#include "CustomCommands.h"
#include "EditorPreferences.h"
//---------------------------------------------------------------------------
TAppConfiguration * AppConfiguration = NULL;
//---------------------------------------------------------------------------
class THistoryStrings : public TStringList
{
public:
  THistoryStrings() : TStringList()
  {
    Modified = false;
  }

  bool Modified;
};
//---------------------------------------------------------------------------
__fastcall TAppConfiguration::TAppConfiguration() :
  TGUIConfiguration()
{
  FUseMasterPassword = false;
  FOnMasterPasswordPrompt = NULL;
  FHistory = new TStringList();
  FEmptyHistory = new TStringList();
  FBookmarks = new TBookmarks();
  FCustomCommandList = new TCustomCommandList();
  FCustomCommandsDefaults = true;
  FEditorList = new TEditorList();
  Default();
}
//---------------------------------------------------------------------------
__fastcall TAppConfiguration::~TAppConfiguration()
{
  Shred(FPlainMasterPasswordEncrypt);
  Shred(FPlainMasterPasswordDecrypt);
  ClearHistory();
  delete FHistory;
  delete FEmptyHistory;
  delete FBookmarks;
  delete FCustomCommandList;
  delete FEditorList;
}
//---------------------------------------------------------------------------
void TAppConfiguration::ClearHistory()
{
  for (int Index = 0; Index < FHistory->Count; Index++)
  {
    delete FHistory->Objects[Index];
  }
  FHistory->Clear();
}
//---------------------------------------------------------------------------
void TAppConfiguration::DefaultHistory()
{
  ClearHistory();

  // Defaults for speed limits.
  std::unique_ptr<THistoryStrings> Strings(new THistoryStrings());
  Strings->Add(LoadStr(SPEED_UNLIMITED));
  unsigned long Speed = MaxSpeed;
  while (Speed >= MinSpeed)
  {
    Strings->Add(SetSpeedLimit(Speed));
    Speed = Speed / 2;
  }
  FHistory->AddObject(L"SpeedLimit", Strings.release());
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::Default()
{
  TGUIConfiguration::Default();
  // OpenSCP: PuTTY is rarely installed, ssh in a terminal emulator then
  PuttyPath = ::DefaultPuttyPath();

  FUseMasterPassword = false;
  FPlainMasterPasswordEncrypt = L"";
  FPlainMasterPasswordDecrypt = L"";
  FMasterPasswordVerifier = L"";

  FConfirmDeleting = true;
  FConfirmTransferring = true;
  FCopyParamAutoSelectNotice = true;
  FEnableQueueByDefault = true;
  FConfirmExitOnCompletion = true;
  FQueueViewShow = qvHideWhenEmpty;
  FQueueViewHeight = 140;
  FSelectMask = AnyMask;
  FSynchronizeSummary = true;
  FShowHiddenFiles = false;
  FLastStoredSession = L"";
  FOpenedStoredSessionFolders = L"";
  FUseLocationProfiles = false;
  FUseSharedBookmarks = false;
  FDDTransferConfirmation = asAuto;
  // OpenSCP: the drop target is known from KDE's protocols only
  FDDFakeFile = IsKdeDesktop();
  FDDDeleteDelay = 120;
  FExternalSessionInExistingInstance = true;
  FBalloonNotifications = true;
  FNotificationsTimeout = 10;
  FMinimizeToTray = false;
  FFlashTaskbar = true;
  FMaxSessions = 100;
  FAutoOpenInPutty = false;
  FWorkspaceConnectAll = false;
  FAutoSaveWorkspace = false;
  FAutoSaveWorkspacePasswords = false;
  FAutoWorkspace = L"";
  FLastWorkspace = L"";
  FSessionTabNameFormat = stnfShortPathTrunc;
  FPreservePanelState = true;
  FConfirmClosingSession = true;
  FShowLoginWhenNoSession = true;
  FKeepOpenWhenNoSession = true;
  FPreserveLocalDirectory = false;
  FSessionsTabs = true;
  FSessionTabCaptionTruncation = true;
  FPathInCaption = picShort;
  FConfirmRecycling = true;
  FDeleteToRecycleBin = true;
  FDDWarnLackOfTempSpace = true;
  FDDWarnLackOfTempSpaceRatio = 1.1;
  FDDTemporaryDirectory = L"";
  FTemporaryDirectoryAppendSession = false;
  FTemporaryDirectoryAppendPath = true;
  FTemporaryDirectoryDeterministic = false;
  FTemporaryDirectoryCleanup = true;
  FConfirmTemporaryDirectoryCleanup = true;
  FDefaultDirIsHome = true;
  FRenameWholeName = false;
  FShowInaccesibleDirectories = true;
  FDoubleClickAction = dcaEdit;
  FCopyOnDoubleClickConfirmation = false;
  FNaturalOrderNumericalSorting = true;
  FAlwaysSortDirectoriesByName = false;
  FFormatSizeBytes = fbKilobytes;
  FPanelSearch = isNameStartOnly;
  FRefreshRemotePanel = false;
  FRefreshRemotePanelInterval = TDateTime(0, 1, 0, 0);
  FPanelFont = TFontConfiguration();
  FEditorCheckNotModified = false;
  FSelectDirectories = false;
  FFileColors = L"";
  FSessionColors = L"";
  FFontColors = L"";
  FSwappedPanels = false;
  FNortonLikeMode = nlKeyboard;
  FCompareByTime = true;
  FCompareBySize = false;
  FExplorerKeyboardShortcuts = false;
  FTreeOnLeft = false;
  FLocalPanel = TPanelConfiguration();
  FRemotePanel = TPanelConfiguration();

  FBookmarks->Clear();
  FCustomCommandsDefaults = true;

  FEditor.FontName = L"";
  FEditor.FontSize = 0;
  FEditor.WordWrap = false;
  FEditor.FindText = L"";
  FEditor.ReplaceText = L"";
  FEditor.FindMatchCase = false;
  FEditor.FindWholeWord = false;
  FEditor.FindDown = true;
  FEditor.TabSize = 8;
  FEditor.MaxEditors = 500;
  FEditor.EarlyClose = 2; // seconds
  FEditor.SDIShellEditor = false;
  FEditor.Encoding = CP_UTF8;
  FEditor.WarnOnEncodingFallback = true;
  FEditor.WarnOrLargeFileSize = true;
  FEditor.LargeFileSize = 10*1024;
  FEditor.FontColor = TColor(0);
  FEditor.BackgroundColor = TColor(0);
  // OpenSCP: upstream adds Notepad as an external editor of the list
  FEditorList->Clear();
  FEditorList->Add(new TEditorPreferences());
  FEditorList->Saved();

  DefaultHistory();
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::DefaultLocalized()
{
  TGUIConfiguration::DefaultLocalized();

  if (FCustomCommandsDefaults)
  {
    FCustomCommandList->Clear();
    FCustomCommandList->Add(LoadStr(CUSTOM_COMMAND_EXECUTE), L"\"./!\"", 0);
    FCustomCommandList->Add(LoadStr(CUSTOM_COMMAND_TOUCH), L"touch \"!\"", ccApplyToDirectories | ccRecursive);
    FCustomCommandList->Add(LoadStr(CUSTOM_COMMAND_TAR),
      FORMAT(L"tar -cz  -f \"!?%s?archive.tgz!\" !&",
        (LoadStr(CUSTOM_COMMAND_TAR_ARCHIVE))), ccApplyToDirectories);
    FCustomCommandList->Add(LoadStr(CUSTOM_COMMAND_UNTAR),
      FORMAT(L"tar -xz --directory=\"!?%s?.!\" -f \"!\"",
        (LoadStr(CUSTOM_COMMAND_UNTAR_DIRECTORY))), 0);
    FCustomCommandList->Add(LoadStr(CUSTOM_COMMAND_GREP),
      FORMAT(L"grep \"!?%s?!\" !&", (LoadStr(CUSTOM_COMMAND_GREP_PATTERN))),
      ccShowResults);
    // OpenSCP: the printing of upstream ("notepad.exe /p") with CUPS
    FCustomCommandList->Add(LoadStr(CUSTOM_COMMAND_PRINT), L"lp \"!\"", ccLocal);
    FCustomCommandList->Reset();
    FCustomCommandsDefaults = true;
  }
}
//---------------------------------------------------------------------------
UnicodeString TAppConfiguration::ExpandedTemporaryDirectory()
{
  UnicodeString Result =
    ExpandFileName(ExpandEnvironmentVariables(DDTemporaryDirectory));
  if (Result.IsEmpty())
  {
    Result = SystemTemporaryDirectory();
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TAppConfiguration::TemporaryDir(bool Mask)
{
  return UniqTempDir(ExpandedTemporaryDirectory(), L"scp", Mask);
}
//---------------------------------------------------------------------------
TStrings * TAppConfiguration::FindTemporaryFolders()
{
  std::unique_ptr<TStrings> Result(new TStringList());
  TSearchRecOwned SRec;
  UnicodeString Mask = TemporaryDir(true);
  if (FindFirstUnchecked(Mask, faDirectory | faHidden, SRec) == 0)
  {
    do
    {
      if (SRec.IsDirectory())
      {
        Result->Add(SRec.GetFilePath());
      }
    }
    while (FindNextChecked(SRec) == 0);
  }

  if (Result->Count == 0)
  {
    Result.reset(NULL);
  }

  return Result.release();
}
//---------------------------------------------------------------------------
void TAppConfiguration::CleanupTemporaryFolders(TStrings * Folders)
{
  if (DebugAlwaysTrue(Folders->Count > 0))
  {
    Usage->Inc(L"TemporaryDirectoryCleanups");
  }

  UnicodeString ErrorList;
  for (int i = 0; i < Folders->Count; i++)
  {
    if (!RecursiveDeleteFile(Folders->Strings[i]))
    {
      if (!ErrorList.IsEmpty())
      {
        ErrorList += L"\n";
      }
      ErrorList += Folders->Strings[i];
    }
  }

  if (!ErrorList.IsEmpty())
  {
    throw ExtException(LoadStr(CLEANUP_TEMP_ERROR), ErrorList);
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TAppConfiguration::DefaultIniFileStorageName()
{
  return
    IncludeTrailingBackslash(IncludeTrailingBackslash(GetShellFolderPath(CSIDL_APPDATA)) + L"openscp") +
    L"openscp.ini";
}
//---------------------------------------------------------------------------
THierarchicalStorage * TAppConfiguration::CreateScpStorage(bool & SessionList)
{
  // The directory of the default configuration file is created on demand
  // (as Windows has the application data folder)
  if ((Storage == stIniFile) && SameStr(IniFileStorageName, DefaultIniFileStorageName()))
  {
    ForceDirectories(ApiPath(ExtractFileDir(IniFileStorageName)));
  }
  return TGUIConfiguration::CreateScpStorage(SessionList);
}
//---------------------------------------------------------------------------
// The keys of TWinConfiguration
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKeyPath(KEY, CANCREATE)) \
  { \
    try { BLOCK } catch (...) { Storage->CloseSubKeyPath(); throw; } \
    Storage->CloseSubKeyPath(); \
  }
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, PropertyToKey(TEXT(#VAR)))
#define REGCONFIG(CANCREATE) \
  BLOCK(L"Interface", CANCREATE, \
    KEY(Bool,    FConfirmDeleting); \
    KEY(Bool,    FConfirmTransferring); \
    KEY(Bool,    FCopyParamAutoSelectNotice); \
    KEY(Bool,    FEnableQueueByDefault); \
    KEY(Bool,    FConfirmExitOnCompletion); \
    KEY(String,  FSelectMask); \
    KEY(Bool,    FSynchronizeSummary); \
    KEY(Bool,    FShowHiddenFiles); \
    KEY(String,  FLastStoredSession); \
    KEY(String,  FOpenedStoredSessionFolders); \
    KEY(Bool,    FUseLocationProfiles); \
    KEY(Bool,    FUseSharedBookmarks); \
    KEYEX(Enum,  FDDTransferConfirmation, L"DDTransferConfirmation2"); \
    KEY(Bool,    FDDFakeFile); \
    KEY(Bool,    FExternalSessionInExistingInstance); \
    KEY(Bool,    FBalloonNotifications); \
    KEY(Integer, FNotificationsTimeout); \
    KEY(Bool,    FMinimizeToTray); \
    KEY(Bool,    FFlashTaskbar); \
    KEY(Integer, FMaxSessions); \
    KEY(Bool,    FAutoOpenInPutty); \
    KEY(Bool,    FWorkspaceConnectAll); \
    KEY(Bool,    FAutoSaveWorkspace); \
    KEY(Bool,    FAutoSaveWorkspacePasswords); \
    KEY(String,  FAutoWorkspace); \
    KEY(Enum,    FSessionTabNameFormat); \
    KEY(Bool,    FPreservePanelState); \
    KEY(Bool,    FConfirmClosingSession); \
    KEY(Bool,    FShowLoginWhenNoSession); \
    KEY(Bool,    FKeepOpenWhenNoSession); \
    KEY(Bool,    FSessionTabCaptionTruncation); \
    KEY(Enum,    FPathInCaption); \
    KEY(Bool,    FConfirmRecycling); \
    KEY(Bool,    FDeleteToRecycleBin); \
    KEY(Bool,    FDDWarnLackOfTempSpace); \
    KEY(Float,   FDDWarnLackOfTempSpaceRatio); \
    KEY(String,  FDDTemporaryDirectory); \
    KEY(Bool,    FTemporaryDirectoryAppendSession); \
    KEY(Bool,    FTemporaryDirectoryAppendPath); \
    KEY(Bool,    FTemporaryDirectoryDeterministic); \
    KEY(Bool,    FTemporaryDirectoryCleanup); \
    KEY(Bool,    FConfirmTemporaryDirectoryCleanup); \
    KEY(Bool,    FDefaultDirIsHome); \
    KEY(Bool,    FRenameWholeName); \
    KEY(Bool,    FShowInaccesibleDirectories); \
    KEY(Enum,    FDoubleClickAction); \
    KEY(Bool,    FCopyOnDoubleClickConfirmation); \
    KEY(Bool,    FNaturalOrderNumericalSorting); \
    KEY(Bool,    FAlwaysSortDirectoriesByName); \
    KEY(Enum,    FFormatSizeBytes); \
    KEY(Enum,    FPanelSearch); \
    KEY(Bool,    FRefreshRemotePanel); \
    KEY(DateTime, FRefreshRemotePanelInterval); \
    KEYEX(String, FPanelFont.FontName, L"PanelFontName"); \
    KEYEX(Integer, FPanelFont.FontSize, L"PanelFontSize"); \
    KEYEX(Integer, FPanelFont.FontStyle, L"PanelFontStyle"); \
    KEYEX(Integer, FPanelFont.FontCharset, L"PanelFontCharset"); \
    KEY(Bool,    FEditorCheckNotModified); \
    KEY(Bool,    FSelectDirectories); \
    KEY(String,  FFileColors); \
    KEY(String,  FSessionColors); \
    KEY(String,  FFontColors); \
  ); \
  BLOCK(L"Interface\\Commander", CANCREATE, \
    KEY(Bool,    FPreserveLocalDirectory); \
    KEY(Bool,    FSessionsTabs); \
    KEY(Bool,    FSwappedPanels); \
    KEYEX(Enum,  FNortonLikeMode, L"ExplorerStyleSelection"); \
    KEY(Bool,    FCompareByTime); \
    KEY(Bool,    FCompareBySize); \
    KEY(Bool,    FExplorerKeyboardShortcuts); \
    KEY(Bool,    FTreeOnLeft); \
  ); \
  BLOCK(L"Interface\\Commander\\LocalPanel", CANCREATE, \
    KEYEX(String,  FLocalPanel.DirViewParams, L"DirViewParams"); \
    KEYEX(Bool,    FLocalPanel.StatusBar, L"StatusBar"); \
    KEYEX(Bool,    FLocalPanel.DriveView, L"DriveView"); \
    KEYEX(Integer, FLocalPanel.DriveViewHeight, L"DriveViewHeight"); \
    KEYEX(Integer, FLocalPanel.DriveViewWidth, L"DriveViewWidth"); \
  ); \
  BLOCK(L"Interface\\Commander\\RemotePanel", CANCREATE, \
    KEYEX(String,  FRemotePanel.DirViewParams, L"DirViewParams"); \
    KEYEX(Bool,    FRemotePanel.StatusBar, L"StatusBar"); \
    KEYEX(Bool,    FRemotePanel.DriveView, L"DriveView"); \
    KEYEX(Integer, FRemotePanel.DriveViewHeight, L"DriveViewHeight"); \
    KEYEX(Integer, FRemotePanel.DriveViewWidth, L"DriveViewWidth"); \
  ); \
  BLOCK(L"Interface\\Editor", CANCREATE, \
    KEYEX(String,  FEditor.FontName, L"FontName2"); \
    KEY(Integer,   FEditor.FontSize); \
    KEY(Bool,      FEditor.WordWrap); \
    KEY(String,    FEditor.FindText); \
    KEY(String,    FEditor.ReplaceText); \
    KEY(Bool,      FEditor.FindMatchCase); \
    KEY(Bool,      FEditor.FindWholeWord); \
    KEY(Bool,      FEditor.FindDown); \
    KEY(Integer,   FEditor.TabSize); \
    KEY(Integer,   FEditor.MaxEditors); \
    KEY(Integer,   FEditor.EarlyClose); \
    KEY(Bool,      FEditor.SDIShellEditor); \
    KEY(Integer,   FEditor.Encoding); \
    KEY(Bool,      FEditor.WarnOnEncodingFallback); \
    KEY(Bool,      FEditor.WarnOrLargeFileSize); \
    KEY(Integer,   FEditor.LargeFileSize); \
    KEY(Enum,      FEditor.FontColor); \
    KEY(Enum,      FEditor.BackgroundColor); \
  ); \
  BLOCK(L"Interface\\QueueView", CANCREATE, \
    KEYEX(Integer, FQueueViewHeight, L"Height"); \
    KEYEX(Integer, FQueueViewShow, L"Show"); \
  ); \
  BLOCK(L"Security", CANCREATE, \
    KEY(Bool,    FUseMasterPassword); \
    KEY(String,  FMasterPasswordVerifier); \
  );
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::SaveData(THierarchicalStorage * Storage, bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  #define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(NAME, VAR)
  REGCONFIG(true);
  #undef KEYEX

  if (Storage->OpenSubKey(L"Bookmarks", true))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    FBookmarks->Save(Storage, All);
  }
  if ((All || FEditorList->Modified) &&
      Storage->OpenSubKeyPath(L"Interface\\Editor", true))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKeyPath(); });
    FEditorList->Save(Storage);
  }
  if ((All && !FCustomCommandsDefaults) || FCustomCommandList->Modified)
  {
    FCustomCommandList->Save(Storage);

    if (FCustomCommandList->Count == 0)
    {
      Storage->WriteBool(L"CustomCommandsNone", true);
    }
    else
    {
      Storage->DeleteValue(L"CustomCommandsNone");
    }
  }

  // As upstream's TCustomWinConfiguration
  if ((FHistory->Count > 0) && Storage->OpenSubKey(L"History", true))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    for (int Index = 0; Index < FHistory->Count; Index++)
    {
      THistoryStrings * HistoryStrings = DebugNotNull(dynamic_cast<THistoryStrings *>(FHistory->Objects[Index]));
      if ((All || HistoryStrings->Modified) && Storage->OpenSubKey(FHistory->Strings[Index], true))
      {
        ::rtl::TFinally SubKeyFinally([&]() { Storage->CloseSubKey(); });
        Storage->WriteValues(HistoryStrings);
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadData(Storage);

  #define KEYEX(TYPE, VAR, NAME) VAR = Storage->Read ## TYPE(NAME, VAR)
  REGCONFIG(false);
  #undef KEYEX

  if (Storage->OpenSubKey(L"Bookmarks", false))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    FBookmarks->Load(Storage);
  }
  if (Storage->OpenSubKeyPath(L"Interface\\Editor", false))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKeyPath(); });
    // The defaults stay, unless there are any editors
    if (Storage->KeyExists(L"0"))
    {
      FEditorList->Clear();
      FEditorList->Load(Storage);
    }
  }
  if (Storage->KeyExists(L"CustomCommands") ||
      Storage->ReadBool(L"CustomCommandsNone", false))
  {
    FCustomCommandList->Load(Storage);
    FCustomCommandsDefaults = false;
  }
  FCustomCommandList->Reset();

  DefaultHistory();
  if (Storage->OpenSubKey(L"History", false))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    std::unique_ptr<TStrings> Names(new TStringList());
    Storage->GetSubKeyNames(Names.get());
    for (int Index = 0; Index < Names->Count; Index++)
    {
      if (Storage->OpenSubKey(Names->Strings[Index], false))
      {
        ::rtl::TFinally SubKeyFinally([&]() { Storage->CloseSubKey(); });
        // remove defaults, if any
        int HIndex = FHistory->IndexOf(Names->Strings[Index]);
        if (HIndex >= 0)
        {
          delete FHistory->Objects[HIndex];
          FHistory->Delete(HIndex);
        }

        std::unique_ptr<THistoryStrings> HistoryStrings(new THistoryStrings());
        Storage->ReadValues(HistoryStrings.get());
        FHistory->AddObject(Names->Strings[Index], HistoryStrings.release());
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::Saved()
{
  TGUIConfiguration::Saved();
  FBookmarks->ModifyAll(false);
  FCustomCommandList->Reset();
  FEditorList->Saved();
}
//---------------------------------------------------------------------------
RawByteString __fastcall TAppConfiguration::StronglyRecryptPassword(RawByteString Password, UnicodeString Key)
{
  RawByteString Dummy;
  RawByteString Result;
  if (GetExternalEncryptedPassword(Password, Dummy) ||
      !FUseMasterPassword)
  {
    // already-strongly encrypted
    // or no master password set, so we cannot strongly-encrypt it
    Result = Password;
  }
  else
  {
    UnicodeString PasswordText = TGUIConfiguration::DecryptPassword(Password, Key);
    if (!PasswordText.IsEmpty())
    {
      AskForMasterPasswordIfNotSet();
      Password = ScramblePassword(PasswordText);
      AES256EncyptWithMAC(Password, FPlainMasterPasswordEncrypt, Result);
      Result = SetExternalEncryptedPassword(Result);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TAppConfiguration::DecryptPassword(RawByteString Password, UnicodeString Key)
{
  UnicodeString Result;
  RawByteString Buf;
  if (GetExternalEncryptedPassword(Password, Buf))
  {
    if (FPlainMasterPasswordDecrypt.IsEmpty())
    {
      AskForMasterPassword();
    }
    if (!AES256DecryptWithMAC(Buf, FPlainMasterPasswordDecrypt, Buf) ||
        !UnscramblePassword(Buf, Result))
    {
      throw Exception(LoadStr(DECRYPT_PASSWORD_ERROR));
    }
  }
  else
  {
    Result = TGUIConfiguration::DecryptPassword(Password, Key);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::SetMasterPassword(UnicodeString value)
{
  // Stores the plain-text version of the password. With the master password
  // off, this is a stray encrypted password (e.g. of a manually imported
  // site); the encrypt password is not set then, not to start encrypting
  // new passwords.
  if (!FUseMasterPassword)
  {
    FPlainMasterPasswordDecrypt = value;
  }
  else if (DebugAlwaysTrue(ValidateMasterPassword(value)))
  {
    FPlainMasterPasswordEncrypt = value;
    FPlainMasterPasswordDecrypt = value;
  }
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::ChangeMasterPassword(
  UnicodeString value, TStrings * RecryptPasswordErrors)
{
  RawByteString Verifier;
  AES256CreateVerifier(value, Verifier);
  FMasterPasswordVerifier = BytesToHex(Verifier);
  FPlainMasterPasswordEncrypt = value;
  FUseMasterPassword = true;
  ::rtl::TFinally Finally([&]() { FPlainMasterPasswordDecrypt = value; });
  RecryptPasswords(RecryptPasswordErrors);
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::ClearMasterPassword(TStrings * RecryptPasswordErrors)
{
  FMasterPasswordVerifier = L"";
  FUseMasterPassword = false;
  Shred(FPlainMasterPasswordEncrypt);
  ::rtl::TFinally Finally([&]() { Shred(FPlainMasterPasswordDecrypt); });
  RecryptPasswords(RecryptPasswordErrors);
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::RecryptPasswords(TStrings * RecryptPasswordErrors)
{
  StoredSessions->RecryptPasswords(RecryptPasswordErrors);

  if (OnMasterPasswordRecrypt != nullptr)
  {
    try
    {
      OnMasterPasswordRecrypt();
    }
    catch (Exception & E)
    {
      UnicodeString Message;
      if (ExceptionMessage(&E, Message))
      {
        // we do not expect this really to happen,
        // so we do not bother providing context
        RecryptPasswordErrors->Add(Message);
      }
    }
  }
}
//---------------------------------------------------------------------------
bool __fastcall TAppConfiguration::ValidateMasterPassword(UnicodeString value)
{
  DebugAssert(UseMasterPassword);
  DebugAssert(!FMasterPasswordVerifier.IsEmpty());
  return AES256Verify(value, HexToBytes(FMasterPasswordVerifier));
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::AskForMasterPassword()
{
  if (FOnMasterPasswordPrompt == NULL)
  {
    throw Exception(L"Master password handler not set");
  }
  FOnMasterPasswordPrompt();
  DebugAssert(!FPlainMasterPasswordDecrypt.IsEmpty());
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::AskForMasterPasswordIfNotSet()
{
  if (FPlainMasterPasswordEncrypt.IsEmpty())
  {
    AskForMasterPassword();
  }
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetConfirmDeleting(bool value)
{
  SET_CONFIG_PROPERTY(ConfirmDeleting);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetConfirmTransferring(bool value)
{
  SET_CONFIG_PROPERTY(ConfirmTransferring);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetCopyParamAutoSelectNotice(bool value)
{
  SET_CONFIG_PROPERTY(CopyParamAutoSelectNotice);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetEnableQueueByDefault(bool value)
{
  SET_CONFIG_PROPERTY(EnableQueueByDefault);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetConfirmExitOnCompletion(bool value)
{
  SET_CONFIG_PROPERTY(ConfirmExitOnCompletion);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetQueueViewShow(TQueueViewShow value)
{
  SET_CONFIG_PROPERTY(QueueViewShow);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetQueueViewHeight(int value)
{
  SET_CONFIG_PROPERTY(QueueViewHeight);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetSelectMask(UnicodeString value)
{
  SET_CONFIG_PROPERTY(SelectMask);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetSynchronizeSummary(bool value)
{
  SET_CONFIG_PROPERTY(SynchronizeSummary);
}
//---------------------------------------------------------------------------
TStrings * TAppConfiguration::GetHistory(const UnicodeString Index)
{
  int I = FHistory->IndexOf(Index);
  return (I >= 0) ? dynamic_cast<TStrings *>(FHistory->Objects[I]) : FEmptyHistory;
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetHistory(const UnicodeString Index, TStrings * value)
{
  int I = FHistory->IndexOf(Index);
  bool NonDefaultChange;
  if (I >= 0)
  {
    THistoryStrings * HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[I]);
    NonDefaultChange = !HistoryStrings->Equals(value);
    if (NonDefaultChange)
    {
      HistoryStrings->Assign(value);
    }
  }
  else
  {
    THistoryStrings * HistoryStrings = new THistoryStrings();
    HistoryStrings->Assign(value);
    FHistory->AddObject(Index, HistoryStrings);
    I = FHistory->Count - 1;
    NonDefaultChange = true;
  }

  if (NonDefaultChange)
  {
    dynamic_cast<THistoryStrings *>(FHistory->Objects[I])->Modified = true;
    Changed();
  }
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetShowHiddenFiles(bool value)
{
  SET_CONFIG_PROPERTY(ShowHiddenFiles);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetLastStoredSession(UnicodeString value)
{
  SET_CONFIG_PROPERTY(LastStoredSession);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetOpenedStoredSessionFolders(UnicodeString value)
{
  SET_CONFIG_PROPERTY(OpenedStoredSessionFolders);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void TAppConfiguration::SetUseLocationProfiles(bool value)
{
  SET_CONFIG_PROPERTY(UseLocationProfiles);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetUseSharedBookmarks(bool value)
{
  SET_CONFIG_PROPERTY(UseSharedBookmarks);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetDDTransferConfirmation(TAutoSwitch value)
{
  SET_CONFIG_PROPERTY(DDTransferConfirmation);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetDDFakeFile(bool value)
{
  SET_CONFIG_PROPERTY(DDFakeFile);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TAppConfiguration::GetDefaultKeyFile()
{
  return FDefaultKeyFile;
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetExternalSessionInExistingInstance(bool value)
{
  SET_CONFIG_PROPERTY(ExternalSessionInExistingInstance);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetBalloonNotifications(bool value)
{
  SET_CONFIG_PROPERTY(BalloonNotifications);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetNotificationsTimeout(int value)
{
  SET_CONFIG_PROPERTY(NotificationsTimeout);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetMinimizeToTray(bool value)
{
  SET_CONFIG_PROPERTY(MinimizeToTray);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetFlashTaskbar(bool value)
{
  SET_CONFIG_PROPERTY(FlashTaskbar);
}
//---------------------------------------------------------------------------
TBookmarkList * TAppConfiguration::GetBookmarks(UnicodeString Key)
{
  return FBookmarks->Bookmarks[Key];
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetBookmarks(UnicodeString Key, TBookmarkList * value)
{
  FBookmarks->Bookmarks[Key] = value;
  Changed();
}
//---------------------------------------------------------------------------
TBookmarkList * TAppConfiguration::GetSharedBookmarks()
{
  return FBookmarks->SharedBookmarks;
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetSharedBookmarks(TBookmarkList * value)
{
  FBookmarks->SharedBookmarks = value;
  Changed();
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetCustomCommandList(TCustomCommandList * value)
{
  if (!FCustomCommandList->Equals(value))
  {
    FCustomCommandList->Assign(value);
    FCustomCommandsDefaults = false;
    Changed();
  }
}
//---------------------------------------------------------------------------
void TAppConfiguration::CustomCommandShortCuts(TShortCuts & ShortCuts) const
{
  FCustomCommandList->ShortCuts(ShortCuts);
}
//---------------------------------------------------------------------------
UnicodeString TAppConfiguration::GetValidHistoryKey(UnicodeString Key)
{
  for (int Index = 1; Index <= Key.Length(); Index++)
  {
    if (!IsLetter(Key[Index]) && !IsDigit(Key[Index]))
    {
      Key[Index] = L'_';
    }
  }

  while (!Key.IsEmpty() && (Key[1] == L'_'))
  {
    Key.Delete(1, 1);
  }

  while (!Key.IsEmpty() && (Key[Key.Length()] == L'_'))
  {
    Key.Delete(Key.Length(), 1);
  }

  int P;
  while ((P = Key.Pos(L"__")) > 0)
  {
    Key.Delete(P, 1);
  }

  return Key;
}
//---------------------------------------------------------------------------
bool TEditorConfiguration::operator !=(const TEditorConfiguration & rhc) const
{
  #define C(Property) (Property != rhc.Property) ||
  return
    C(FontName) C(FontSize) C(WordWrap) C(FindText) C(ReplaceText)
    C(FindMatchCase) C(FindWholeWord) C(FindDown) C(TabSize)
    C(MaxEditors) C(EarlyClose) C(SDIShellEditor)
    C(Encoding) C(WarnOnEncodingFallback) C(WarnOrLargeFileSize) C(LargeFileSize)
    C(FontColor) C(BackgroundColor) false;
  #undef C
}
//---------------------------------------------------------------------------
bool TPanelConfiguration::operator !=(const TPanelConfiguration & rhc) const
{
  return
    (DirViewParams != rhc.DirViewParams) || (StatusBar != rhc.StatusBar) || (DriveView != rhc.DriveView) ||
    (DriveViewHeight != rhc.DriveViewHeight) || (DriveViewWidth != rhc.DriveViewWidth);
}
//---------------------------------------------------------------------------
bool TFontConfiguration::operator !=(const TFontConfiguration & rhc) const
{
  return
    (FontName != rhc.FontName) || (FontSize != rhc.FontSize) ||
    (FontCharset != rhc.FontCharset) || (FontStyle != rhc.FontStyle);
}
//---------------------------------------------------------------------------
static const wchar_t FileColorDataSeparator = L':';
TFileColorData::TFileColorData() :
  Color(TColor())
{
}
//---------------------------------------------------------------------------
void TFileColorData::Load(const UnicodeString & S)
{
  UnicodeString Buf(S);
  Color = RestoreColor(CutToChar(Buf, FileColorDataSeparator, true));
  FileMask = Buf;
}
//---------------------------------------------------------------------------
UnicodeString TFileColorData::Save() const
{
  UnicodeString Result = StoreColor(Color) + FileColorDataSeparator + FileMask.Masks;
  return Result;
}
//---------------------------------------------------------------------------
void TFileColorData::LoadList(const UnicodeString & S, TList & List)
{
  std::unique_ptr<TStringList> Strings(CommaTextToStringList(S));

  List.clear();
  for (int Index = 0; Index < Strings->Count; Index++)
  {
    TFileColorData FileColorData;
    FileColorData.Load(Strings->Strings[Index]);
    List.push_back(FileColorData);
  }
}
//---------------------------------------------------------------------------
UnicodeString TFileColorData::SaveList(const TList & List)
{
  std::unique_ptr<TStringList> Strings(new TStringList());
  for (TFileColorData::TList::const_iterator Iter = List.begin(); Iter != List.end(); Iter++)
  {
    Strings->Add((*Iter).Save());
  }
  return Strings->CommaText;
}
//---------------------------------------------------------------------------
TColor RestoreColor(const UnicodeString & CStr)
{
  return TColor(StrToInt(UnicodeString(L"$") + CStr));
}
//---------------------------------------------------------------------------
UnicodeString StoreColor(TColor Color)
{
  return IntToHex(Color, 6);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetEditor(const TEditorConfiguration & value)
{
  SET_CONFIG_PROPERTY(Editor);
}
//---------------------------------------------------------------------------
void TAppConfiguration::SetEditorList(const TEditorList * value)
{
  if (!(*FEditorList == *value))
  {
    *FEditorList = *value;
    Changed();
  }
}
//---------------------------------------------------------------------------
const TEditorPreferences * TAppConfiguration::DefaultEditorForFile(
  const UnicodeString & FileName, bool Local, const TFileMasks::TParams & MaskParams)
{
  return FEditorList->Find(FileName, Local, MaskParams);
}
