// The configuration of the application layer.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <Cryptography.h>
#include <Security.h>

#include "AppConfiguration.h"
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

  FUseMasterPassword = false;
  FPlainMasterPasswordEncrypt = L"";
  FPlainMasterPasswordDecrypt = L"";
  FMasterPasswordVerifier = L"";

  FConfirmDeleting = true;
  FConfirmTransferring = true;
  FCopyParamAutoSelectNotice = true;
  FShowHiddenFiles = false;
  FLastStoredSession = L"";
  FOpenedStoredSessionFolders = L"";

  DefaultHistory();
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
    KEY(Bool,    FShowHiddenFiles); \
    KEY(String,  FLastStoredSession); \
    KEY(String,  FOpenedStoredSessionFolders); \
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
