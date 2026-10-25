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
  bool GetShowHiddenFiles() const { return FShowHiddenFiles; }
  void SetShowHiddenFiles(bool value);
  UnicodeString GetLastStoredSession() const { return FLastStoredSession; }
  void SetLastStoredSession(UnicodeString value);
  UnicodeString GetOpenedStoredSessionFolders() const { return FOpenedStoredSessionFolders; }
  void SetOpenedStoredSessionFolders(UnicodeString value);
  TMasterPasswordPromptEvent GetOnMasterPasswordPrompt() const { return FOnMasterPasswordPrompt; }
  void SetOnMasterPasswordPrompt(TMasterPasswordPromptEvent Value) { FOnMasterPasswordPrompt = Value; }

  __declspec(property(get = GetUseMasterPassword)) bool UseMasterPassword;
  __declspec(property(get = GetConfirmDeleting, put = SetConfirmDeleting)) bool ConfirmDeleting;
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
  bool FShowHiddenFiles;
  UnicodeString FLastStoredSession;
  UnicodeString FOpenedStoredSessionFolders;

  void __fastcall AskForMasterPassword();
};
//---------------------------------------------------------------------------
extern TAppConfiguration * AppConfiguration;
//---------------------------------------------------------------------------
#endif
