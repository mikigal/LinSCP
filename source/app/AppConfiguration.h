// The configuration of the application layer: the configuration of the
// core and of the transfer settings (TGUIConfiguration), stored in an INI
// file in the format of WinSCP.ini, and the master password (upstream has
// it in TWinConfiguration).
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
  TMasterPasswordPromptEvent GetOnMasterPasswordPrompt() const { return FOnMasterPasswordPrompt; }
  void SetOnMasterPasswordPrompt(TMasterPasswordPromptEvent Value) { FOnMasterPasswordPrompt = Value; }

  __declspec(property(get = GetUseMasterPassword)) bool UseMasterPassword;
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

  void __fastcall AskForMasterPassword();
};
//---------------------------------------------------------------------------
extern TAppConfiguration * AppConfiguration;
//---------------------------------------------------------------------------
#endif
