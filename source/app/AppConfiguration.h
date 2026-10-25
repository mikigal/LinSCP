// The configuration of the application layer: the configuration of the
// core and of the transfer settings (TGUIConfiguration), stored in an INI
// file in the format of WinSCP.ini, the master password and the settings
// of the GUI (upstream has them in TWinConfiguration, under the same keys).
//---------------------------------------------------------------------------
#ifndef AppConfigurationH
#define AppConfigurationH
//---------------------------------------------------------------------------
#include "GUIConfiguration.h"

#include <FileMasks.h>

#include <vector>
//---------------------------------------------------------------------------
class TBookmarks;
class TBookmarkList;
class TCustomCommandList;
class TEditorList;
class TEditorPreferences;
typedef TMethodPtr<void()> TMasterPasswordPromptEvent;
// The passwords are encrypted with another master password (or without it)
typedef std::function<void()> TMasterPasswordRecryptEvent;
// When the queue list is shown (upstream's WinConfiguration.h)
enum TQueueViewShow : int { qvShow, qvHideWhenEmpty, qvHide };
// How the tabs of sessions with the same name are told apart
enum TSessionTabNameFormat : int { stnfNone, stnfShortPath, stnfShortPathTrunc };
// The path of the current panel in the title of the window
enum TPathInCaption : int { picShort, picFull, picNone };
// What double-click does on a file (upstream's WinConfiguration.h)
enum TDoubleClickAction : int { dcaOpen = 0, dcaCopy = 1, dcaEdit = 2 };
// How a panel searches as letters are typed (CustomWinConfiguration.h)
enum TIncrementalSearch : int { isOff = -1, isNameStartOnly, isName, isAll };
// How the panels select files (upstream's TNortonLikeMode, the inverse of
// its "Explorer-style selection"): the keyboard (nlKeyboard) or also the
// mouse (nlOn) only focus files, Insert and Space select them
enum TNortonLikeMode : int { nlOn, nlOff, nlKeyboard };
// The page of the dialog of the generated URL and code, the format of the
// script (sfBatchFile is a shell script on Linux)
enum TGenerateUrlCodeTarget : int { guctUrl, guctScript, guctAssembly };
enum TScriptFormat : int { sfScriptFile, sfBatchFile, sfCommandLine, sfPowerShell };
// The style bits of fonts (VCL's TFontStyles)
const int fsBold = 0x01;
const int fsItalic = 0x02;
const int fsUnderline = 0x04;
const int fsStrikeOut = 0x08;
// A font of the configuration, the default one when the name is empty
struct TFontConfiguration
{
  UnicodeString FontName;
  int FontSize = 0;
  int FontCharset = 1; // DEFAULT_CHARSET
  int FontStyle = 0;

  bool operator !=(const TFontConfiguration & rhc) const;
};
// An option with its field, accessors and property; its key is its name
#define APP_CONFIG_PROPERTY(TYPE, NAME) \
  public: \
    TYPE Get ## NAME() const { return F ## NAME; } \
    void Set ## NAME(TYPE value) { SET_CONFIG_PROPERTY(NAME); } \
    __declspec(property(get = Get ## NAME, put = Set ## NAME)) TYPE NAME; \
  private: \
    TYPE F ## NAME;
// The settings of a panel of the Commander (upstream's
// TScpCommanderPanelConfiguration): its columns, its status bar, its tree
struct TPanelConfiguration
{
  UnicodeString DirViewParams;
  bool StatusBar = true;
  bool DriveView = false;
  int DriveViewHeight = 100;
  int DriveViewWidth = 100;

  bool operator !=(const TPanelConfiguration & rhc) const;
};
// The settings of the editors (upstream's TEditorConfiguration, without the
// colors and the window placement)
struct TEditorConfiguration
{
  // The fixed width font of the desktop when empty
  UnicodeString FontName;
  int FontSize;
  bool WordWrap;
  UnicodeString FindText;
  UnicodeString ReplaceText;
  bool FindMatchCase;
  bool FindWholeWord;
  bool FindDown;
  int TabSize;
  int MaxEditors;
  // Seconds; an external editor closed sooner is probably not the one that edits the file
  int EarlyClose;
  bool SDIShellEditor;
  // CP_UTF8 or CP_ACP (the other encoding)
  int Encoding;
  bool WarnOnEncodingFallback;
  bool WarnOrLargeFileSize;
  // KiB
  int LargeFileSize;
  // The colors of the text and the background, 0 for the default
  TColor FontColor;
  TColor BackgroundColor;

  bool operator !=(const TEditorConfiguration & rhc) const;
};
// The color of the files matching a mask (upstream's TFileColorData)
struct TFileColorData
{
  TFileColorData();

  TFileMasks FileMask;
  TColor Color;

  void Load(const UnicodeString & S);
  UnicodeString Save() const;
  typedef std::vector<TFileColorData> TList;
  static void LoadList(const UnicodeString & S, TList & List);
  static UnicodeString SaveList(const TList & List);
};
// The colors in the configuration (BGR, in hexadecimal)
TColor RestoreColor(const UnicodeString & CStr);
UnicodeString StoreColor(TColor Color);
//---------------------------------------------------------------------------
class TAppConfiguration : public TGUIConfiguration
{
public:
  __fastcall TAppConfiguration();
  virtual __fastcall ~TAppConfiguration();

  virtual void __fastcall Default();
  virtual UnicodeString TemporaryDir(bool Mask = false);

  void __fastcall SetMasterPassword(UnicodeString value);
  bool __fastcall ValidateMasterPassword(UnicodeString value);
  void __fastcall AskForMasterPasswordIfNotSet();
  // The passwords of the sites are encrypted with the new master password,
  // or not at all; the errors are collected
  void __fastcall ChangeMasterPassword(UnicodeString value, TStrings * RecryptPasswordErrors);
  void __fastcall ClearMasterPassword(TStrings * RecryptPasswordErrors);
  // The directory for the temporary directories, $TMPDIR unless
  // DDTemporaryDirectory
  UnicodeString ExpandedTemporaryDirectory();
  // The temporary directories left by previous runs
  TStrings * FindTemporaryFolders();
  void CleanupTemporaryFolders(TStrings * Folders);

  // $XDG_CONFIG_HOME/openscp/openscp.ini, used unless /ini= says otherwise
  static UnicodeString __fastcall DefaultIniFileStorageName();

  bool GetUseMasterPassword() const { return FUseMasterPassword; }
  bool GetConfirmDeleting() const { return FConfirmDeleting; }
  void SetConfirmDeleting(bool value);
  bool GetConfirmTransferring() const { return FConfirmTransferring; }
  void SetConfirmTransferring(bool value);
  bool GetCopyParamAutoSelectNotice() const { return FCopyParamAutoSelectNotice; }
  void SetCopyParamAutoSelectNotice(bool value);
  bool GetEnableQueueByDefault() const { return FEnableQueueByDefault; }
  void SetEnableQueueByDefault(bool value);
  bool GetConfirmExitOnCompletion() const { return FConfirmExitOnCompletion; }
  void SetConfirmExitOnCompletion(bool value);
  TQueueViewShow GetQueueViewShow() const { return static_cast<TQueueViewShow>(FQueueViewShow); }
  void SetQueueViewShow(TQueueViewShow value);
  int GetQueueViewHeight() const { return FQueueViewHeight; }
  void SetQueueViewHeight(int value);
  UnicodeString GetSelectMask() const { return FSelectMask; }
  void SetSelectMask(UnicodeString value);
  bool GetSynchronizeSummary() const { return FSynchronizeSummary; }
  void SetSynchronizeSummary(bool value);
  bool GetShowHiddenFiles() const { return FShowHiddenFiles; }
  void SetShowHiddenFiles(bool value);
  UnicodeString GetLastStoredSession() const { return FLastStoredSession; }
  void SetLastStoredSession(UnicodeString value);
  UnicodeString GetOpenedStoredSessionFolders() const { return FOpenedStoredSessionFolders; }
  void SetOpenedStoredSessionFolders(UnicodeString value);
  bool GetUseLocationProfiles() const { return FUseLocationProfiles; }
  void SetUseLocationProfiles(bool value);
  bool GetUseSharedBookmarks() const { return FUseSharedBookmarks; }
  void SetUseSharedBookmarks(bool value);
  TAutoSwitch GetDDTransferConfirmation() const { return FDDTransferConfirmation; }
  void SetDDTransferConfirmation(TAutoSwitch value);
  bool GetDDFakeFile() const { return FDDFakeFile; }
  void SetDDFakeFile(bool value);
  int GetDDDeleteDelay() const { return FDDDeleteDelay; }
  // The key of the sessions without one (/privatekey), not stored
  void SetDefaultKeyFile(const UnicodeString & value) { FDefaultKeyFile = value; }
  bool GetExternalSessionInExistingInstance() const { return FExternalSessionInExistingInstance; }
  void SetExternalSessionInExistingInstance(bool value);
  bool GetBalloonNotifications() const { return FBalloonNotifications; }
  void SetBalloonNotifications(bool value);
  int GetNotificationsTimeout() const { return FNotificationsTimeout; }
  void SetNotificationsTimeout(int value);
  bool GetMinimizeToTray() const { return FMinimizeToTray; }
  void SetMinimizeToTray(bool value);
  bool GetFlashTaskbar() const { return FFlashTaskbar; }
  void SetFlashTaskbar(bool value);
  TBookmarkList * GetBookmarks(UnicodeString Key);
  void SetBookmarks(UnicodeString Key, TBookmarkList * value);
  TBookmarkList * GetSharedBookmarks();
  void SetSharedBookmarks(TBookmarkList * value);
  TCustomCommandList * GetCustomCommandList() const { return FCustomCommandList; }
  const TEditorConfiguration & GetEditor() const { return FEditor; }
  void SetEditor(const TEditorConfiguration & value);
  const TEditorList * GetEditorList() const { return FEditorList; }
  void SetEditorList(const TEditorList * value);
  const TEditorPreferences * DefaultEditorForFile(
    const UnicodeString & FileName, bool Local, const TFileMasks::TParams & MaskParams);
  void SetCustomCommandList(TCustomCommandList * value);
  // The shortcuts used by the custom commands
  void CustomCommandShortCuts(TShortCuts & ShortCuts) const;
  // A name of a history from any text (letters and digits only)
  UnicodeString GetValidHistoryKey(UnicodeString Key);
  // The history of a combo box (upstream's TCustomWinConfiguration::History)
  TStrings * GetHistory(const UnicodeString Index);
  void SetHistory(const UnicodeString Index, TStrings * value);
  TMasterPasswordPromptEvent GetOnMasterPasswordPrompt() const { return FOnMasterPasswordPrompt; }
  void SetOnMasterPasswordPrompt(TMasterPasswordPromptEvent Value) { FOnMasterPasswordPrompt = Value; }

  __declspec(property(get = GetUseMasterPassword)) bool UseMasterPassword;
  __declspec(property(get = GetConfirmDeleting, put = SetConfirmDeleting)) bool ConfirmDeleting;
  // The copy dialog is shown before transfers
  __declspec(property(get = GetConfirmTransferring, put = SetConfirmTransferring)) bool ConfirmTransferring;
  __declspec(property(get = GetHistory, put = SetHistory)) TStrings * History[];
  // A transfer settings preset selected by its rule is announced
  __declspec(property(get = GetCopyParamAutoSelectNotice, put = SetCopyParamAutoSelectNotice))
    bool CopyParamAutoSelectNotice;
  // The queues of new sessions process their items
  __declspec(property(get = GetEnableQueueByDefault, put = SetEnableQueueByDefault)) bool EnableQueueByDefault;
  // The application is closed once the queue gets empty only when confirmed
  __declspec(property(get = GetConfirmExitOnCompletion, put = SetConfirmExitOnCompletion)) bool ConfirmExitOnCompletion;
  __declspec(property(get = GetQueueViewShow, put = SetQueueViewShow)) TQueueViewShow QueueViewShow;
  __declspec(property(get = GetQueueViewHeight, put = SetQueueViewHeight)) int QueueViewHeight;
  // The last file mask of the select and find dialogs
  __declspec(property(get = GetSelectMask, put = SetSelectMask)) UnicodeString SelectMask;
  // The summary of a synchronization is shown
  __declspec(property(get = GetSynchronizeSummary, put = SetSynchronizeSummary)) bool SynchronizeSummary;
  __declspec(property(get = GetShowHiddenFiles, put = SetShowHiddenFiles)) bool ShowHiddenFiles;
  // The site or site folder (with a trailing slash) last selected in the
  // site manager
  __declspec(property(get = GetLastStoredSession, put = SetLastStoredSession)) UnicodeString LastStoredSession;
  // The expanded site folders of the site manager (comma text)
  __declspec(property(get = GetOpenedStoredSessionFolders, put = SetOpenedStoredSessionFolders))
    UnicodeString OpenedStoredSessionFolders;
  // The location profiles are opened instead of the bookmarks (both
  // directories at once)
  __declspec(property(get = GetUseLocationProfiles, put = SetUseLocationProfiles)) bool UseLocationProfiles;
  // The shared bookmarks page was the last one selected
  __declspec(property(get = GetUseSharedBookmarks, put = SetUseSharedBookmarks)) bool UseSharedBookmarks;
  // The copy dialog is shown before transfers by drag and drop (asAuto:
  // shown with "do not show again" checked)
  __declspec(property(get = GetDDTransferConfirmation, put = SetDDTransferConfirmation))
    TAutoSwitch DDTransferConfirmation;
  // Files dragged or copied from the remote panel to the file manager are
  // downloaded to the directory they are dropped or pasted to, which the
  // file manager tells (KDE: Ark's drop protocol, the notifications of
  // KIO); otherwise they are downloaded to a temporary directory once the
  // file manager asks for them. On by default on KDE Plasma.
  __declspec(property(get = GetDDFakeFile, put = SetDDFakeFile)) bool DDFakeFile;
  // Seconds before the temporary directory of a drag and drop is deleted
  __declspec(property(get = GetDDDeleteDelay)) int DDDeleteDelay;
  // A session of the command line is opened by an instance that runs
  // already, if it can
  __declspec(property(get = GetExternalSessionInExistingInstance, put = SetExternalSessionInExistingInstance))
    bool ExternalSessionInExistingInstance;
  // The notifications of the desktop are shown (upstream's balloons of the
  // tray icon), for the seconds
  __declspec(property(get = GetBalloonNotifications, put = SetBalloonNotifications)) bool BalloonNotifications;
  __declspec(property(get = GetNotificationsTimeout, put = SetNotificationsTimeout)) int NotificationsTimeout;
  // Minimizing the application (by its own commands) hides it to the
  // system tray
  __declspec(property(get = GetMinimizeToTray, put = SetMinimizeToTray)) bool MinimizeToTray;
  // The button of the task manager asks for attention when the application
  // needs the user in the background
  __declspec(property(get = GetFlashTaskbar, put = SetFlashTaskbar)) bool FlashTaskbar;
  // The bookmarks of a site (by its session key), copied when set
  __declspec(property(get = GetBookmarks, put = SetBookmarks)) TBookmarkList * Bookmarks[];
  __declspec(property(get = GetSharedBookmarks, put = SetSharedBookmarks)) TBookmarkList * SharedBookmarks;
  // Copied when set
  __declspec(property(get = GetCustomCommandList, put = SetCustomCommandList)) TCustomCommandList * CustomCommandList;
  __declspec(property(get = GetEditor, put = SetEditor)) TEditorConfiguration Editor;
  // Copied when set
  __declspec(property(get = GetEditorList, put = SetEditorList)) const TEditorList * EditorList;
  __declspec(property(get = GetOnMasterPasswordPrompt, put = SetOnMasterPasswordPrompt))
    TMasterPasswordPromptEvent OnMasterPasswordPrompt;
  // The copies of the sites (of the site manager) are recrypted
  TMasterPasswordRecryptEvent OnMasterPasswordRecrypt;

  // The sessions of PuTTY to import (upstream's SelectPuttySessionsForImport
  // of its registry, here the files of PuTTY's directory) and their host
  // keys
  TStoredSessionList * SelectPuttySessionsForImport(TStoredSessionList * Sessions, UnicodeString & Error);
  void ImportPuttyHostKeys(TStoredSessionList * Sessions);
  // The directory of PuTTY's configuration
  static UnicodeString GetPuttyFolder();

  // The sessions and their tabs
  // The number of sessions above which opening another is confirmed
  APP_CONFIG_PROPERTY(int, MaxSessions)
  // A connected session is opened in the SSH client too
  APP_CONFIG_PROPERTY(bool, AutoOpenInPutty)
  // All the sessions of a workspace are connected (the inactive ones in the
  // background), not only the first one
  APP_CONFIG_PROPERTY(bool, WorkspaceConnectAll)
  // The sessions are saved as the workspace AutoWorkspace when the
  // application closes (with their passwords)
  APP_CONFIG_PROPERTY(bool, AutoSaveWorkspace)
  APP_CONFIG_PROPERTY(bool, AutoSaveWorkspacePasswords)
  APP_CONFIG_PROPERTY(UnicodeString, AutoWorkspace)
  // The workspace last saved (offered when saving again)
  APP_CONFIG_PROPERTY(UnicodeString, LastWorkspace)
  APP_CONFIG_PROPERTY(TSessionTabNameFormat, SessionTabNameFormat)
  // The panels of a session are shown as they were when it is activated
  // again (the history, the focused file)
  APP_CONFIG_PROPERTY(bool, PreservePanelState)
  // Closing the application with connected sessions is confirmed
  APP_CONFIG_PROPERTY(bool, ConfirmClosingSession)
  // The site manager is shown once the last session is closed, and the
  // application keeps running without sessions
  APP_CONFIG_PROPERTY(bool, ShowLoginWhenNoSession)
  APP_CONFIG_PROPERTY(bool, KeepOpenWhenNoSession)
  // The local panel keeps its directory when another session is activated
  // (Interface\Commander)
  APP_CONFIG_PROPERTY(bool, PreserveLocalDirectory)
  // The tabs of the sessions are shown (Interface\Commander)
  APP_CONFIG_PROPERTY(bool, SessionsTabs)
  APP_CONFIG_PROPERTY(bool, SessionTabCaptionTruncation)
  APP_CONFIG_PROPERTY(TPathInCaption, PathInCaption)

  // Moving local files to the trash is confirmed, Delete moves them there
  APP_CONFIG_PROPERTY(bool, ConfirmRecycling)
  APP_CONFIG_PROPERTY(bool, DeleteToRecycleBin)
  // A download to the temporary directory warns when there is not enough
  // space (the size of the files by the ratio)
  APP_CONFIG_PROPERTY(bool, DDWarnLackOfTempSpace)
  APP_CONFIG_PROPERTY(double, DDWarnLackOfTempSpaceRatio)
  // The temporary directory (edited and dragged files), the system one when
  // empty
  APP_CONFIG_PROPERTY(UnicodeString, DDTemporaryDirectory)
  // The temporary copies of edited remote files are in a directory of the
  // session, of the remote path, in a directory that does not change
  APP_CONFIG_PROPERTY(bool, TemporaryDirectoryAppendSession)
  APP_CONFIG_PROPERTY(bool, TemporaryDirectoryAppendPath)
  APP_CONFIG_PROPERTY(bool, TemporaryDirectoryDeterministic)
  // The temporary directories of previous runs are deleted on startup,
  // when confirmed
  APP_CONFIG_PROPERTY(bool, TemporaryDirectoryCleanup)
  APP_CONFIG_PROPERTY(bool, ConfirmTemporaryDirectoryCleanup)

  // The panels
  // The home directory of the local panel is the local directory of the
  // site
  APP_CONFIG_PROPERTY(bool, DefaultDirIsHome)
  // Renaming selects the extension too
  APP_CONFIG_PROPERTY(bool, RenameWholeName)
  // Remote directories that cannot be read are listed
  APP_CONFIG_PROPERTY(bool, ShowInaccesibleDirectories)
  APP_CONFIG_PROPERTY(TDoubleClickAction, DoubleClickAction)
  APP_CONFIG_PROPERTY(bool, CopyOnDoubleClickConfirmation)
  // Numbers in names sort by their values; directories sort by name
  // whatever the column
  APP_CONFIG_PROPERTY(bool, NaturalOrderNumericalSorting)
  APP_CONFIG_PROPERTY(bool, AlwaysSortDirectoriesByName)
  APP_CONFIG_PROPERTY(TFormatBytesStyle, FormatSizeBytes)
  APP_CONFIG_PROPERTY(TIncrementalSearch, PanelSearch)
  // The remote directory is reloaded after the interval
  APP_CONFIG_PROPERTY(bool, RefreshRemotePanel)
  APP_CONFIG_PROPERTY(TDateTime, RefreshRemotePanelInterval)
  APP_CONFIG_PROPERTY(TFontConfiguration, PanelFont)
  // An edited remote file is uploaded only when it did not change on the
  // server meanwhile (confirmed otherwise)
  APP_CONFIG_PROPERTY(bool, EditorCheckNotModified)
  // Selecting by a mask selects the matching directories too
  APP_CONFIG_PROPERTY(bool, SelectDirectories)
  // The dialog of the generated URL and code: the components of the URL
  // (-1 for the default ones), the page, the format of the script; WebDAV
  // URLs with http(s)://
  APP_CONFIG_PROPERTY(int, GenerateUrlComponents)
  APP_CONFIG_PROPERTY(TGenerateUrlCodeTarget, GenerateUrlCodeTarget)
  APP_CONFIG_PROPERTY(TScriptFormat, GenerateUrlScriptFormat)
  APP_CONFIG_PROPERTY(bool, HttpForWebDAV)
  // The code of the translation (upstream's, translations/*.ini), empty for
  // the language of the system, EN for English
  APP_CONFIG_PROPERTY(UnicodeString, Language)
  // The colors of the files in the panels (TFileColorData::SaveList)
  APP_CONFIG_PROPERTY(UnicodeString, FileColors)
  // The custom colors picked for the sessions and for the files (comma
  // separated, StoreColor), offered in the color menus
  APP_CONFIG_PROPERTY(UnicodeString, SessionColors)
  APP_CONFIG_PROPERTY(UnicodeString, FontColors)

  // The Commander (Interface\Commander)
  // The remote panel is on the left
  APP_CONFIG_PROPERTY(bool, SwappedPanels)
  APP_CONFIG_PROPERTY(TNortonLikeMode, NortonLikeMode)
  // Comparing the directories of the panels marks newer files, and files
  // of the same time with other sizes
  APP_CONFIG_PROPERTY(bool, CompareByTime)
  APP_CONFIG_PROPERTY(bool, CompareBySize)
  // The keyboard shortcuts of the Explorer interface (F5 refreshes, ...)
  APP_CONFIG_PROPERTY(bool, ExplorerKeyboardShortcuts)
  // The Commander or the Explorer
  APP_CONFIG_PROPERTY(TInterface, Interface)
  // The directory trees are left of the files, not above them
  APP_CONFIG_PROPERTY(bool, TreeOnLeft)
  APP_CONFIG_PROPERTY(TPanelConfiguration, LocalPanel)
  APP_CONFIG_PROPERTY(TPanelConfiguration, RemotePanel)
  // The toolbars of the Commander in upstream's format (ToolbarsLayout2:
  // Name=Visible:Dock:Row+Pos), their hidden buttons (Name=Button:0,...;),
  // the status bar of the window
  // The queue view (Interface\QueueView): its toolbar, the setting the
  // queue was shown by before the user toggled it
  APP_CONFIG_PROPERTY(bool, QueueViewToolBar)
  APP_CONFIG_PROPERTY(TQueueViewShow, QueueViewLastHideShow)
  APP_CONFIG_PROPERTY(UnicodeString, CommanderToolbarsLayout)
  APP_CONFIG_PROPERTY(UnicodeString, CommanderToolbarsButtons)
  APP_CONFIG_PROPERTY(bool, CommanderStatusBar)
  // The toolbars of both interfaces (Interface): fixed in their places,
  // the text of the important buttons shown, the size of the icons
  // (0 normal, 1 large, 2 very large)
  APP_CONFIG_PROPERTY(bool, LockToolbars)
  APP_CONFIG_PROPERTY(bool, SelectiveToolbarText)
  APP_CONFIG_PROPERTY(int, LargerToolbar)

  // The Explorer (Interface\Explorer): its panel, the local directory of
  // the downloads and the synchronizations, the full path in the address
  // bar, the tabs of the sessions
  APP_CONFIG_PROPERTY(TPanelConfiguration, ExplorerPanel)
  APP_CONFIG_PROPERTY(UnicodeString, ExplorerLastLocalTargetDirectory)
  APP_CONFIG_PROPERTY(bool, ExplorerShowFullAddress)
  APP_CONFIG_PROPERTY(bool, ExplorerSessionsTabs)
  APP_CONFIG_PROPERTY(UnicodeString, ExplorerToolbarsLayout)
  APP_CONFIG_PROPERTY(UnicodeString, ExplorerToolbarsButtons)

protected:
  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
  virtual void __fastcall Saved();
  virtual void __fastcall DefaultLocalized();
  virtual UnicodeString __fastcall GetDefaultKeyFile();
  virtual THierarchicalStorage * CreateScpStorage(bool & SessionList);
  virtual UnicodeString __fastcall DecryptPassword(RawByteString Password, UnicodeString Key);
  virtual RawByteString __fastcall StronglyRecryptPassword(RawByteString Password, UnicodeString Key);

private:
  bool FUseMasterPassword;
  UnicodeString FPlainMasterPasswordEncrypt;
  UnicodeString FPlainMasterPasswordDecrypt;
  UnicodeString FMasterPasswordVerifier;
  TMasterPasswordPromptEvent FOnMasterPasswordPrompt;
  bool FConfirmDeleting;
  bool FConfirmTransferring;
  bool FCopyParamAutoSelectNotice;
  bool FEnableQueueByDefault;
  bool FConfirmExitOnCompletion;
  // TQueueViewShow
  int FQueueViewShow;
  int FQueueViewHeight;
  UnicodeString FSelectMask;
  bool FSynchronizeSummary;
  bool FShowHiddenFiles;
  // The histories (their names, the strings with the "modified" flag as the
  // objects of the names)
  TStringList * FHistory;
  TStringList * FEmptyHistory;
  UnicodeString FLastStoredSession;
  UnicodeString FOpenedStoredSessionFolders;
  bool FUseLocationProfiles;
  bool FUseSharedBookmarks;
  TAutoSwitch FDDTransferConfirmation;
  bool FDDFakeFile;
  int FDDDeleteDelay;
  bool FExternalSessionInExistingInstance;
  bool FBalloonNotifications;
  int FNotificationsTimeout;
  bool FMinimizeToTray;
  bool FFlashTaskbar;
  UnicodeString FDefaultKeyFile;
  TBookmarks * FBookmarks;
  TCustomCommandList * FCustomCommandList;
  TEditorConfiguration FEditor;
  TEditorList * FEditorList;
  // The default commands, not stored
  bool FCustomCommandsDefaults;

  void __fastcall AskForMasterPassword();
  void __fastcall RecryptPasswords(TStrings * RecryptPasswordErrors);
  void ClearHistory();
  void DefaultHistory();
};
//---------------------------------------------------------------------------
extern TAppConfiguration * AppConfiguration;
//---------------------------------------------------------------------------
#endif
