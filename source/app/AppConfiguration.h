// The configuration of the application layer: the configuration of the
// core and of the transfer settings (TGUIConfiguration), stored in an INI
// file in the format of WinSCP.ini, the master password and the settings
// of the GUI (upstream has them in TWinConfiguration, under the same keys).
//---------------------------------------------------------------------------
#ifndef AppConfigurationH
#define AppConfigurationH
//---------------------------------------------------------------------------
#include "GUIConfiguration.h"
//---------------------------------------------------------------------------
typedef TMethodPtr<void()> TMasterPasswordPromptEvent;
// When the queue list is shown (upstream's WinConfiguration.h)
enum TQueueViewShow : int { qvShow, qvHideWhenEmpty, qvHide };
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
  bool GetShowHiddenFiles() const { return FShowHiddenFiles; }
  void SetShowHiddenFiles(bool value);
  UnicodeString GetLastStoredSession() const { return FLastStoredSession; }
  void SetLastStoredSession(UnicodeString value);
  UnicodeString GetOpenedStoredSessionFolders() const { return FOpenedStoredSessionFolders; }
  void SetOpenedStoredSessionFolders(UnicodeString value);
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
  __declspec(property(get = GetShowHiddenFiles, put = SetShowHiddenFiles)) bool ShowHiddenFiles;
  // The site or site folder (with a trailing slash) last selected in the
  // site manager
  __declspec(property(get = GetLastStoredSession, put = SetLastStoredSession)) UnicodeString LastStoredSession;
  // The expanded site folders of the site manager (comma text)
  __declspec(property(get = GetOpenedStoredSessionFolders, put = SetOpenedStoredSessionFolders))
    UnicodeString OpenedStoredSessionFolders;
  __declspec(property(get = GetOnMasterPasswordPrompt, put = SetOnMasterPasswordPrompt))
    TMasterPasswordPromptEvent OnMasterPasswordPrompt;

protected:
  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
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
  bool FShowHiddenFiles;
  // The histories (their names, the strings with the "modified" flag as the
  // objects of the names)
  TStringList * FHistory;
  TStringList * FEmptyHistory;
  UnicodeString FLastStoredSession;
  UnicodeString FOpenedStoredSessionFolders;

  void __fastcall AskForMasterPassword();
  void ClearHistory();
  void DefaultHistory();
};
//---------------------------------------------------------------------------
extern TAppConfiguration * AppConfiguration;
//---------------------------------------------------------------------------
#endif
