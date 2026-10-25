// Declarations of the Qt front end shared by its windows (upstream has them
// in WinInterface.h).
//---------------------------------------------------------------------------
#ifndef QtInterfaceH
#define QtInterfaceH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <FileMasks.h>
#include <SessionInfo.h>
#include <SynchronizeController.h>
#include <Terminal.h>

#include <functional>
#include <map>
#include <vector>
//---------------------------------------------------------------------------
class TCopyParamList;
class TCustomCommandList;
class TCustomCommandType;
class TFileMasks;
class TRemoteTokenList;
class TTerminal;
class QPoint;
class QWidget;
struct TCopyParamRuleData;
struct TCalculateSizeStats;
class TRemoteProperties;
//---------------------------------------------------------------------------
const int mpNeverAskAgainCheck =   0x01;
const int mpAllowContinueOnError = 0x02;
//---------------------------------------------------------------------------
struct TMessageParams
{
  TMessageParams(unsigned int AParams = 0);
  TMessageParams(const TQueryParams * AParams);

  const TQueryButtonAlias * Aliases;
  unsigned int AliasesCount;
  unsigned int Params;
  unsigned int Timer;
  TQueryParamsTimerEvent TimerEvent;
  UnicodeString TimerMessage;
  unsigned int TimerAnswers;
  TQueryType TimerQueryType;
  unsigned int Timeout;
  unsigned int TimeoutAnswer;
  unsigned int TimeoutResponse;
  UnicodeString NeverAskAgainTitle;
  unsigned int NeverAskAgainAnswer;
  bool NeverAskAgainCheckedInitially;
  UnicodeString CustomCaption;

private:
  void Reset();
};
//---------------------------------------------------------------------------
// dialogs/MessageDialog.cpp
unsigned int MessageDialog(const UnicodeString & Msg, TQueryType Type,
  unsigned int Answers, const TMessageParams * Params = NULL);
unsigned int MessageDialog(int Ident, TQueryType Type,
  unsigned int Answers, const TMessageParams * Params = NULL);
unsigned int SimpleErrorDialog(const UnicodeString & Msg, const UnicodeString & MoreMessages = L"");
unsigned int MoreMessageDialog(const UnicodeString & Message,
  TStrings * MoreMessages, TQueryType Type, unsigned int Answers,
  const TMessageParams * Params = NULL);
unsigned int ExceptionMessageDialog(Exception * E, TQueryType Type,
  const UnicodeString & MessageFormat = L"", unsigned int Answers = qaOK,
  const TMessageParams * Params = NULL);
unsigned int FatalExceptionMessageDialog(
  Exception * E, TQueryType Type, const UnicodeString & MessageFormat, unsigned int Answers,
  const TMessageParams * Params);

// WhileIdle: not of an operation (a lost connection of an idle session
// may be closed silently)
void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E, bool WhileIdle = false);

// dialogs/CustomDialog.cpp
bool InputDialog(const UnicodeString & ACaption,
  const UnicodeString & APrompt, UnicodeString & Value,
  TStrings * History = NULL, bool PathInput = false, bool Echo = true);
// Saves the session as a site, asking for its name and whether to save
// its password unless ForceDialog is false and the answers are known;
// returns the new site or NULL when cancelled
TSessionData * DoSaveSession(TSessionData * SessionData,
  TSessionData * OriginalSession, bool ForceDialog, TStrings * AdditionalFolders);
// Asks for the name of a workspace to save the sessions as, whether to
// save their passwords (unless SavePasswords is NULL), to create a desktop
// shortcut and to save it automatically
bool DoSaveWorkspaceDialog(UnicodeString & WorkspaceName,
  bool * SavePasswords, bool NotRecommendedSavingPasswords,
  bool & CreateShortcut, bool & EnableAutoSave);
// Checks that a site can be saved under the name (or overwrite a site)
void SessionNameValidate(const UnicodeString & Text, const UnicodeString & OriginalName);
// Asks for the current master password
bool DoMasterPasswordDialog();
// Asks for the current master password (if any) and a new one
bool DoChangeMasterPasswordDialog(UnicodeString & NewPassword);
// Adds or edits a trusted host certification authority
bool DoSshHostCADialog(bool Add, TSshHostCA & SshHostCA);

// dialogs/PreferencesDialog.cpp
// The page the preferences open on
enum TPreferencesMode { pmDefault, pmEditor, pmCustomCommands,
    pmQueue, pmLogging, pmUpdates, pmPresets, pmEditors, pmCommander,
    pmEditorInternal, pmFileColors };
struct TPreferencesDialogData
{
  // The rule data for new presets ("current" in the preset dialog)
  TCopyParamRuleData * CopyParamRuleData;
};
bool DoPreferencesDialog(TPreferencesMode APreferencesMode, TPreferencesDialogData * DialogData = NULL);

// dialogs/EditorPreferencesDialog.cpp
struct TEditorData;
enum TEditorPreferencesMode { epmAdd, epmEdit, epmAdHoc };
// An editor of the list (with Remember for an ad hoc one)
bool DoEditorPreferencesDialog(TEditorData * Editor, bool & Remember, TEditorPreferencesMode Mode, bool MayRemote);

enum TTransferDirection { tdToRemote, tdToLocal };
// The operation on a file finished (an empty name: on all files)
typedef std::function<void(TOperationSide Side, const UnicodeString & FileName, bool Success, bool NotCancelled)>
  TFileOperationFinishedEvent;
enum TTransferType { ttCopy, ttMove };

// dialogs/CopyDialog.cpp
const int coTemp                = 0x001;
const int coDisableQueue        = 0x002;
const int coDoNotShowAgain      = 0x020;
const int coAllFiles            = 0x1000;
const int cooDoNotShowAgain     = 0x01;
const int cooSaveSettings       = 0x04;
// Confirms a transfer of the files to the target directory, with the
// transfer settings
bool DoCopyDialog(
  bool ToRemote, bool Move, TStrings * FileList, UnicodeString & TargetDirectory,
  TGUICopyParamType * Params, int Options, int CopyParamAttrs, int * OutputOptions);
bool DoCreateDirectoryDialog(UnicodeString & Directory);

// dialogs/CopyParamDialogs.cpp
bool DoCopyParamCustomDialog(TCopyParamType & CopyParam, int CopyParamAttrs);
enum TCopyParamPresetMode { cpmAdd, cpmAddCurrent, cpmEdit, cpmDuplicate };
bool DoCopyParamPresetDialog(TCopyParamList * CopyParamList,
  int & Index, TCopyParamPresetMode Mode, TCopyParamRuleData * CurrentRuleData,
  const TCopyParamType & DefaultCopyParams);
const int cplNone =             0x00;
const int cplCustomizeDefault = 0x02;
const int cplSaveSettings =     0x04;
// The menu of the transfer settings presets at the position (upstream's
// CopyParamListPopup and CopyParamListPopupClick); returns 1 when the
// settings changed (a preset was selected or the settings customized)
int CopyParamListPopup(const QPoint & Pos, TCopyParamType & Param, UnicodeString & Preset,
  int Options, int CopyParamAttrs, bool * SaveSettings);

// dialogs/FileFindDialog.cpp
typedef std::function<void(TTerminal * Terminal, UnicodeString Directory, const TFileMasks & FileMask,
  TFileFoundEvent OnFileFound, TFindingFileEvent OnFindingFile)> TFindEvent;
typedef std::function<void(TTerminal * Terminal, const UnicodeString & Path)> TFocusFileEvent;
typedef std::function<void(TTerminal * Terminal, TStrings * FileList,
  TFileOperationFinishedEvent OnFileOperationFinished)> TFileListOperationEvent;
// The modeless window of the file find; OnEditFiles is optional
void ShowFileFindDialog(
  TTerminal * Terminal, UnicodeString Directory, TFindEvent OnFind, TFocusFileEvent OnFocusFile,
  TFileListOperationEvent OnDeleteFiles, TFileListOperationEvent OnDownloadFiles,
  TFileListOperationEvent OnEditFiles);
void HideFileFindDialog();

// dialogs/EditMaskDialog.cpp
bool DoEditMaskDialog(TFileMasks & Mask);

// dialogs/ImportSessionsDialog.cpp: the sites of other applications
// imported, the new sites are added to the list
bool DoImportSessionsDialog(TList * Imported);

// dialogs/FileSystemInfoDialog.cpp: the information about the server and
// the protocol of the session; OnGetSpaceAvailable is NULL when the space
// available cannot be checked (Close set closes the dialog, upstream's
// TGetSpaceAvailable)
typedef std::function<void(const UnicodeString & Path, TSpaceAvailable & ASpaceAvailable, bool & Close)>
  TGetSpaceAvailableEvent;
void DoFileSystemInfoDialog(const TSessionInfo & SessionInfo, const TFileSystemInfo & FileSystemInfo,
  const UnicodeString & SpaceAvailablePath, TGetSpaceAvailableEvent OnGetSpaceAvailable);

// dialogs/SiteAdvancedDialog.cpp
bool DoSiteAdvancedDialog(TSessionData * SessionData);
// dialogs/CustomDialog.cpp: the settings of the site that differ from the
// defaults, as raw settings (upstream's TSiteRawDialog)
void DoSiteRawDialog(TSessionData * Data);

// dialogs/SelectMaskDialog.cpp
// The files of a panel selected by a mask (upstream's TFileFilter of
// its directory views)
struct TFileFilter
{
  UnicodeString Masks;
  bool Directories = false;
};
struct TFileColorData;
// The mask of the files to select or unselect (the last one, SelectMask)
bool DoSelectMaskDialog(QWidget * Parent, bool Select, TFileFilter & Filter);
// The mask of the files shown in a panel
bool DoFilterMaskDialog(QWidget * Parent, UnicodeString & Mask);
bool DoFileColorDialog(TFileColorData & FileColorData);

// app/KeyTools.cpp: the private keys and the certificates of the sites
// The key is usable (of PuTTY), else its error is shown and EAbort thrown
// (unless the error can be and is ignored); a key of OpenSSH is converted
// to a key of PuTTY when confirmed (VerifyAndConvertKey)
void VerifyAndConvertKey(UnicodeString & FileName, bool CanIgnore);
void VerifyKey(const UnicodeString & FileName);
void VerifyCertificate(const UnicodeString & FileName);
struct TPrivateKey;
// Adds the certificate found next to the key to it, returns its file
UnicodeString AddMatchingKeyCertificate(TPrivateKey * PrivateKey, const UnicodeString & FileName);
// The proxy of the environment (https_proxy, ...)
bool AutodetectProxy(UnicodeString & HostName, int & PortNumber);

// dialogs/ColorMenu.cpp
// The menu of the colors at the position: the default one (0), the custom
// colors picked before, the standard palette (light colors of the sessions
// or the colors of the fonts), More Colors; false when no color was picked
bool ColorMenuPopup(const QPoint & Pos, TColor & Color, bool SessionColors);

// dialogs/FullSynchronizeDialog.cpp
enum TSynchronizeMode { smRemote, smLocal, smBoth };
const int fsoDisableTimestamp = 0x01;
const int fsoDoNotUsePresets = 0x02;
const int fsoAllowSelectedOnly = 0x04;
const int fsoDisableByChecksum = 0x08;
const int fsoLocalLocal = 0x10;
bool DoFullSynchronizeDialog(TSynchronizeMode & Mode, int & Params,
  UnicodeString & Directory1, UnicodeString & Directory2,
  TCopyParamType * CopyParams, bool & SaveSettings, bool & SaveMode, int Options,
  const TUsableCopyParamAttrs & CopyParamAttrs);

// app/OpenScpApplication.cpp: shows an exception that escaped an event
// handler (as the VCL's Application->OnException). The handlers that run
// operations catch their exceptions and pass them here: an exception must
// not propagate through Qt's code, which keeps state on the stack (guards
// of the objects of the signals), Qt is not built for exceptions
void HandleApplicationException(Exception & E);
// Runs a handler of an event (an action, a button), its exception goes to
// HandleApplicationException
void ExecuteHandler(const std::function<void()> & Handler);

// app/QtInterface.cpp: minimizing the whole application (upstream's
// ApplicationMinimize and ApplicationRestore), to the system tray when
// asked or configured; done by the main window
void MinimizeApplication(bool ToTray = false);
void RestoreApplication();
// The desktop has a system tray (a StatusNotifierWatcher)
bool IsSystemTrayAvailable();

// dialogs/ConsoleDialog.cpp
void DoConsoleDialog(TTerminal * Terminal, const UnicodeString & Command = UnicodeString(),
  const TStrings * Log = NULL);

// app/LocalProcesses.cpp (the commands are run by /bin/sh)
void ExecuteShellChecked(const UnicodeString & Command);
void ExecuteShellCheckedAndWait(const UnicodeString & Command);
void ExecuteProcessAndReadOutput(const UnicodeString & Command, UnicodeString & Output, int & ExitCode, bool ReadStdErr);
// With the output, the command fails when it exits with an error
void ExecuteProcessChecked(const UnicodeString & Command, UnicodeString * Output);
void ExecuteProcessCheckedAndWait(const UnicodeString & Command, UnicodeString * Output);

// dialogs/CustomCommandDialog.cpp
enum TCustomCommandsMode { ccmAdd, ccmEdit, ccmAdHoc };
const int ccoDisableRemote = 0x01;
const int ccoDisableRemoteFiles = 0x02;
typedef std::function<void(const TCustomCommandType & Command)> TCustomCommandValidate;
bool DoCustomCommandDialog(TCustomCommandType & Command, const TCustomCommandList * CustomCommandList,
  TCustomCommandsMode Mode, int Options, TCustomCommandValidate OnValidate, const TShortCuts * ShortCuts);
// The prompts of the custom commands in one dialog, the output of local
// commands
class TWinInteractiveCustomCommand : public TInteractiveCustomCommand
{
public:
  TWinInteractiveCustomCommand(TCustomCommand * ChildCustomCommand, const UnicodeString & CustomCommandName);

protected:
  virtual void __fastcall Prompt(int Index, const UnicodeString & Prompt, UnicodeString & Value) override;
  virtual void __fastcall Execute(const UnicodeString & Command, UnicodeString & Value) override;
  virtual void __fastcall PatternHint(int Index, const UnicodeString & Pattern) override;

private:
  UnicodeString FCustomCommandName;
  std::map<int, size_t> FIndexes;
  std::vector<UnicodeString> FPrompts;
  std::vector<UnicodeString> FDefaults;
  std::vector<UnicodeString> FValues;
};
// Which of the custom commands are listed
enum TCustomCommandListType { ccltAll, ccltFile, ccltNonFile, ccltBoth };

// dialogs/EditorForm.cpp
typedef std::function<void(QWidget * Editor)> TEditorEvent;
typedef std::function<void(QWidget * Editor, bool Forced)> TEditorClosedEvent;
typedef std::function<void(QWidget * Editor, bool & Modified)> TAnyModifiedEvent;
// The internal editor, a window of its own; saving is disabled without
// OnSaveAll, a save is announced by OnFileChanged
QWidget * ShowEditorForm(const UnicodeString & FileName, QWidget * ParentForm,
  TEditorEvent OnFileChanged, TEditorEvent OnFileReload, TEditorClosedEvent OnClose,
  TEditorEvent OnSaveAll, TAnyModifiedEvent OnAnyModified,
  const UnicodeString & Caption, bool NewFile);
void EditorFormFileUploadComplete(QWidget * Form);
void EditorFormFileSave(QWidget * Form);
bool IsEditorFormModified(QWidget * Form);

// dialogs/OpenDirectoryDialog.cpp
enum TOpenDirectoryMode { odBrowse, odAddBookmark };
bool DoOpenDirectoryDialog(TOpenDirectoryMode Mode, TOperationSide Side, UnicodeString & Directory,
  TStrings * Directories, TTerminal * Terminal, bool AllowSwitch);

// dialogs/LocationProfilesDialog.cpp
bool LocationProfilesDialog(TOpenDirectoryMode Mode, TOperationSide Side,
  UnicodeString & LocalDirectory, UnicodeString & RemoteDirectory,
  TStrings * LocalDirectories, TStrings * RemoteDirectories, TTerminal * Terminal);

// dialogs/SynchronizeDialog.cpp
const int soDoNotUsePresets =  0x01;
const int soNoMinimize =       0x02;
const int soAllowSelectedOnly = 0x04;
typedef std::function<void(int Params, TSynchronizeOptions & Options)> TGetSynchronizeOptionsEvent;
typedef std::function<void(const UnicodeString & Message)> TSynchronizeSessionLog;
typedef std::function<void(const UnicodeString & Message, TStrings * MoreMessages, TQueryType Type)>
  TFeedSynchronizeError;
// Sets OnFeedSynchronizeError while watching, if errors are to be logged
// only (soContinueOnError), and OnSynchronizeAbort while the dialog shows
bool DoSynchronizeDialog(TSynchronizeParamType & Params,
  const TCopyParamType * CopyParams, TSynchronizeStartStopEvent OnStartStop,
  bool & SaveSettings, int Options, int CopyParamAttrs,
  TGetSynchronizeOptionsEvent OnGetOptions,
  TSynchronizeSessionLog OnSynchronizeSessionLog,
  TFeedSynchronizeError & OnFeedSynchronizeError,
  std::function<void()> & OnSynchronizeAbort);

// dialogs/SynchronizeChecklistDialog.cpp
typedef std::function<void(void * Token, TProcessedSynchronizationChecklistItem OnProcessedItem,
  TUpdatedSynchronizationChecklistItems OnUpdatedSynchronizationChecklistItems)> TFullSynchronizeEvent;
typedef std::function<void(void * Token)> TQueueSynchronizeEvent;
typedef std::function<void(TSynchronizeChecklist * Checklist,
  const TSynchronizeChecklist::TItemList & Items, void * Token)> TSynchronizeChecklistCalculateSize;
bool DoSynchronizeChecklistDialog(TSynchronizeChecklist * Checklist, TSynchronizeMode Mode, int Params,
  const UnicodeString & Directory1, const UnicodeString & Directory2,
  TFullSynchronizeEvent OnSynchronize, TQueueSynchronizeEvent OnQueueSynchronize,
  TSynchronizeChecklistCalculateSize OnSynchronizeChecklistCalculateSize, void * Token);

// dialogs/RemoteTransferDialog.cpp
enum TDirectRemoteCopy { drcDisallow, drcAllow, drcConfirmCommandSession, drcConfirmCommandSessionDirs };
typedef std::function<bool(void * Session, const UnicodeString & Directory)> TDirectoryExistsEvent;
bool DoRemoteCopyDialog(
  TStrings * Sessions, TStrings * Directories,
  TDirectRemoteCopy AllowDirectCopy, bool Multi, void *& Session, UnicodeString & Target, UnicodeString & FileMask,
  bool & DirectCopy, void * CurrentSession, TDirectoryExistsEvent OnDirectoryExists, bool TargetConfirmed);
// dialogs/CustomDialog.cpp
bool DoRemoteMoveDialog(
  bool Multi, UnicodeString & Target, UnicodeString & FileMask, TDirectoryExistsEvent OnDirectoryExists);

bool DoShortCutDialog(TShortCut & ShortCut, const TShortCuts & ShortCuts);
bool DoTagDialog(bool Add, TStrings * Tags, UnicodeString & Key, UnicodeString & Value);

// dialogs/PropertiesDialog.cpp
// The properties that can be changed
const int cpMode =  0x01;
const int cpOwner = 0x02;
const int cpGroup = 0x04;
const int cpAcl =   0x08;
// Owners and groups are identified by their IDs, the tags can be edited
const int poUserGroupByID = 0x01;
const int poTags =          0x02;
typedef std::function<void(TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats, bool & Close)>
  TCalculateSizeEvent;
typedef std::function<void(const UnicodeString & Alg, TStrings * FileList,
  TCalculatedChecksumEvent OnCalculatedChecksum, bool & Close)> TCalculateChecksumEvent;
// The files of FileList have their TRemoteFile objects (also local ones)
bool DoPropertiesDialog(TStrings * FileList,
  const UnicodeString & Directory, const TRemoteTokenList * GroupList,
  const TRemoteTokenList * UserList, TStrings * ChecksumAlgs,
  TRemoteProperties * Properties,
  int AllowedChanges, int Options, TCalculateSizeEvent OnCalculateSize,
  TCalculateChecksumEvent OnCalculateChecksum);

// dialogs/LoginDialog.cpp
// The site manager: fills DataList with the sessions to open
bool DoLoginDialog(TList * DataList, QWidget * Parent, bool NeedTerminal);
//---------------------------------------------------------------------------
#endif
