// The configuration of the application layer.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <Cryptography.h>
#include <Security.h>

#include "AppConfiguration.h"
//---------------------------------------------------------------------------
TAppConfiguration * AppConfiguration = NULL;
//---------------------------------------------------------------------------
__fastcall TAppConfiguration::TAppConfiguration() :
  TGUIConfiguration()
{
  FUseMasterPassword = false;
  FOnMasterPasswordPrompt = NULL;
  Default();
}
//---------------------------------------------------------------------------
__fastcall TAppConfiguration::~TAppConfiguration()
{
  Shred(FPlainMasterPasswordEncrypt);
  Shred(FPlainMasterPasswordDecrypt);
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::Default()
{
  TGUIConfiguration::Default();

  FUseMasterPassword = false;
  FPlainMasterPasswordEncrypt = L"";
  FPlainMasterPasswordDecrypt = L"";
  FMasterPasswordVerifier = L"";
}
//---------------------------------------------------------------------------
UnicodeString TAppConfiguration::TemporaryDir(bool Mask)
{
  return UniqTempDir(SystemTemporaryDirectory(), L"scp", Mask);
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
// The keys of TWinConfiguration's "Security" section
#define BLOCK(KEY, CANCREATE, BLOCK) \
  if (Storage->OpenSubKeyPath(KEY, CANCREATE)) \
  { \
    try { BLOCK } catch (...) { Storage->CloseSubKeyPath(); throw; } \
    Storage->CloseSubKeyPath(); \
  }
#define KEY(TYPE, VAR) KEYEX(TYPE, VAR, PropertyToKey(TEXT(#VAR)))
#define REGCONFIG(CANCREATE) \
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
}
//---------------------------------------------------------------------------
void __fastcall TAppConfiguration::LoadData(THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadData(Storage);

  #define KEYEX(TYPE, VAR, NAME) VAR = Storage->Read ## TYPE(NAME, VAR)
  REGCONFIG(false);
  #undef KEYEX
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
