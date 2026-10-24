//---------------------------------------------------------------------------
#ifndef ConfigurationH
#define ConfigurationH

#include <set>
#include <list>
#include "RemoteFiles.h"
#include "FileBuffer.h"
#include "HierarchicalStorage.h"
#include "Usage.h"
//---------------------------------------------------------------------------
#define SET_CONFIG_PROPERTY_EX(PROPERTY, APPLY) \
  if (PROPERTY != value) { F ## PROPERTY = value; Changed(); APPLY; }
#define SET_CONFIG_PROPERTY(PROPERTY) \
  SET_CONFIG_PROPERTY_EX(PROPERTY, )
//---------------------------------------------------------------------------
extern const wchar_t * AutoSwitchNames;
extern const wchar_t * NotAutoSwitchNames;
enum TAutoSwitch : int { asOn, asOff, asAuto }; // Has to match PuTTY FORCE_ON, FORCE_OFF, AUTO
//---------------------------------------------------------------------------
class TStoredSessionList;
class TCopyParamType;
//---------------------------------------------------------------------------
class TSshHostCA
{
public:
  TSshHostCA();
  void Save(THierarchicalStorage * Storage) const;
  bool Load(THierarchicalStorage * Storage);

  UnicodeString Name;
  RawByteString PublicKey;
  UnicodeString ValidityExpression;
  bool PermitRsaSha1;
  bool PermitRsaSha256;
  bool PermitRsaSha512;

  typedef std::vector<TSshHostCA> TList;
};
//---------------------------------------------------------------------------
class TSshHostCAList
{
public:
  TSshHostCAList();
  TSshHostCAList(const TSshHostCA::TList & List);
  TSshHostCAList & operator =(const TSshHostCAList & other);
  void Default();
  const TSshHostCA::TList & GetList() const;
  int GetCount() const;
  const TSshHostCA * Get(int Index) const;
  const TSshHostCA * Find(const UnicodeString & Name) const;

  void Save(THierarchicalStorage * Storage);
  void Load(THierarchicalStorage * Storage);

private:
  TSshHostCA::TList FList;
};
//---------------------------------------------------------------------------
class TConfiguration : public TObject
{
private:
  bool FDontSave;
  bool FForceSave;
  bool FChanged;
  int FUpdating;
  TNotifyEvent FOnChange;

  void * FApplicationInfo;
  TUsage * FUsage;
  bool FLogging;
  bool FPermanentLogging;
  UnicodeString FLogFileName;
  UnicodeString FPermanentLogFileName;
  bool FLogFileAppend;
  bool FLogSensitive;
  bool FPermanentLogSensitive;
  __int64 FLogMaxSize;
  __int64 FPermanentLogMaxSize;
  int FLogMaxCount;
  int FPermanentLogMaxCount;
  int FLogProtocol;
  int FPermanentLogProtocol;
  int FActualLogProtocol;
  bool FLogActions;
  bool FPermanentLogActions;
  bool FLogActionsRequired;
  UnicodeString FActionsLogFileName;
  UnicodeString FPermanentActionsLogFileName;
  bool FConfirmOverwriting;
  bool FConfirmResume;
  bool FAutoReadDirectoryAfterOp;
  int FSessionReopenAuto;
  int FSessionReopenBackground;
  int FSessionReopenTimeout;
  int FSessionReopenAutoStall;
  UnicodeString FCustomIniFileStorageName;
  UnicodeString FIniFileStorageName;
  UnicodeString FVirtualIniFileStorageName;
  std::unique_ptr<TStrings> FOptionsStorage;
  int FProgramIniPathWritable;
  int FTunnelLocalPortNumberLow;
  int FTunnelLocalPortNumberHigh;
  int FCacheDirectoryChangesMaxSize;
  bool FShowFtpWelcomeMessage;
  UnicodeString FDefaultRandomSeedFile;
  UnicodeString FRandomSeedFile;
  UnicodeString FPuttyRegistryStorageKey;
  UnicodeString FExternalIpAddress;
  int FLocalPortNumberMin;
  int FLocalPortNumberMax;
  bool FTryFtpWhenSshFails;
  int FParallelDurationThreshold;
  bool FScripting;
  UnicodeString FMimeTypes;
  int FDontReloadMoreThanSessions;
  int FScriptProgressFileNameLimit;
  int FKeyVersion;
  int FQueueTransfersLimit;
  int FParallelTransferThreshold;
  UnicodeString FCertificateStorage;
  UnicodeString FAWSAPI;
  UnicodeString FChecksumCommands;
  std::unique_ptr<TSshHostCAList> FSshHostCAList;
  std::unique_ptr<TSshHostCAList> FPuttySshHostCAList;
  bool FSshHostCAsFromPuTTY;
  int FHttpsCertificateValidation;
  UnicodeString FSynchronizationChecksumAlgs;

  bool FDisablePasswordStoring;
  bool FForceBanners;
  bool FDisableAcceptingHostKeys;
  bool FDefaultCollectUsage;

  TVSFixedFileInfo *__fastcall GetFixedApplicationInfo();
  void * __fastcall GetApplicationInfo();
  virtual UnicodeString __fastcall GetVersionStr();
  virtual UnicodeString __fastcall GetVersion();
  UnicodeString __fastcall GetProductVersion();
  UnicodeString __fastcall GetProductName();
  UnicodeString __fastcall GetCompanyName();
  UnicodeString __fastcall GetFileVersion(TVSFixedFileInfo * Info);
  UnicodeString __fastcall GetStoredSessionsSubKey();
  UnicodeString __fastcall DoGetPuttySessionsKey();
  UnicodeString __fastcall GetPuttySessionsSubKey();
  void __fastcall SetRandomSeedFile(UnicodeString value);
  UnicodeString __fastcall GetRandomSeedFileName();
  void __fastcall SetPuttyRegistryStorageKey(UnicodeString value);
  UnicodeString __fastcall GetSshHostKeysSubKey();
  UnicodeString __fastcall GetRootKeyStr();
  UnicodeString __fastcall GetConfigurationSubKey();
  TEOLType __fastcall GetLocalEOLType();
  void __fastcall SetLogging(bool value);
  bool __fastcall GetLogging();
  void __fastcall SetLogFileName(UnicodeString value);
  UnicodeString __fastcall GetLogFileName();
  bool __fastcall GetLogToFile();
  void __fastcall SetLogFileAppend(bool value);
  void __fastcall SetLogSensitive(bool value);
  void __fastcall SetLogMaxSize(__int64 value);
  __int64 __fastcall GetLogMaxSize();
  void __fastcall SetLogMaxCount(int value);
  int __fastcall GetLogMaxCount();
  void __fastcall SetLogProtocol(int value);
  void __fastcall SetLogActions(bool value);
  bool __fastcall GetLogActions();
  void __fastcall SetActionsLogFileName(UnicodeString value);
  UnicodeString __fastcall GetPermanentActionsLogFileName();
  UnicodeString __fastcall GetActionsLogFileName();
  UnicodeString __fastcall GetDefaultLogFileName();
  UnicodeString __fastcall GetTimeFormat();
  UnicodeString __fastcall GetRegistryStorageKey();
  UnicodeString __fastcall GetIniFileStorageNameForReadingWriting();
  UnicodeString __fastcall GetIniFileStorageNameForReading();
  UnicodeString __fastcall GetIniFileStorageName(bool ReadingOnly);
  void __fastcall SetOptionsStorage(TStrings * value);
  TStrings * __fastcall GetOptionsStorage();
  UnicodeString __fastcall GetFileInfoString(const UnicodeString Key);
  void __fastcall SetSessionReopenAuto(int value);
  void __fastcall SetSessionReopenBackground(int value);
  void __fastcall SetSessionReopenTimeout(int value);
  void __fastcall SetSessionReopenAutoStall(int value);
  void __fastcall SetTunnelLocalPortNumberLow(int value);
  void __fastcall SetTunnelLocalPortNumberHigh(int value);
  void __fastcall SetCacheDirectoryChangesMaxSize(int value);
  void __fastcall SetShowFtpWelcomeMessage(bool value);
  int __fastcall GetCompoundVersion();
  void __fastcall UpdateActualLogProtocol();
  void __fastcall SetExternalIpAddress(UnicodeString value);
  void __fastcall SetTryFtpWhenSshFails(bool value);
  void __fastcall SetParallelDurationThreshold(int value);
  void __fastcall SetMimeTypes(UnicodeString value);
  void SetCertificateStorage(const UnicodeString & value);
  UnicodeString GetCertificateStorageExpanded();
  void SetAWSAPI(const UnicodeString & value);
  bool __fastcall GetCollectUsage();
  void __fastcall SetCollectUsage(bool value);
  bool __fastcall GetIsUnofficial();
  bool __fastcall GetPersistent();
  void SetLocalPortNumberMin(int value);
  void SetLocalPortNumberMax(int value);
  void SetQueueTransfersLimit(int value);
  const TSshHostCAList * GetSshHostCAList();
  void SetSshHostCAList(const TSshHostCAList * value);
  const TSshHostCAList * GetPuttySshHostCAList();
  const TSshHostCAList * GetActiveSshHostCAList();
  int GetAuthAgent();
  void SetAuthAgent(int value);

protected:
  TStorage FStorage;
  TCriticalSection * FCriticalSection;

  virtual TStorage __fastcall GetStorage();
  virtual void __fastcall Changed();
  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
  virtual void __fastcall LoadFrom(THierarchicalStorage * Storage);
  virtual void __fastcall CopyData(THierarchicalStorage * Source, THierarchicalStorage * Target);
  virtual void __fastcall LoadAdmin(THierarchicalStorage * Storage);
  void LoadSshHostCAList(TSshHostCAList * SshHostCAList, THierarchicalStorage * Storage);
  virtual UnicodeString __fastcall GetDefaultKeyFile();
  virtual void __fastcall Saved();
  void __fastcall CleanupRegistry(const UnicodeString & RegistryPath);
  void __fastcall CopyAllStringsInSubKey(
    THierarchicalStorage * Source, THierarchicalStorage * Target, const UnicodeString & Name);
  bool __fastcall CopySubKey(THierarchicalStorage * Source, THierarchicalStorage * Target, const UnicodeString & Name);
  UnicodeString __fastcall BannerHash(const UnicodeString & Banner);
  void __fastcall SetBannerData(const UnicodeString & SessionKey, const UnicodeString & BannerHash, unsigned int Params);
  void __fastcall GetBannerData(const UnicodeString & SessionKey, UnicodeString & BannerHash, unsigned int & Params);
  static UnicodeString __fastcall PropertyToKey(const UnicodeString & Property);
  void DoSave(THierarchicalStorage * AStorage, bool All);
  virtual void __fastcall DoSave(bool All, bool Explicit);
  UnicodeString __fastcall FormatFingerprintKey(const UnicodeString & SiteKey, const UnicodeString & FingerprintType);
  THierarchicalStorage * OpenDirectoryStatisticsCache(bool CanCreate);
  UnicodeString __fastcall GetDirectoryStatisticsCacheKey(
    const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam);
  void SelectSessionsToImportIfAny(
    TStoredSessionList * ImportSessionList, TStoredSessionList * Sessions,
    UnicodeString & Error, const UnicodeString & NoSessionsError);

  virtual bool __fastcall GetConfirmOverwriting();
  virtual void __fastcall SetConfirmOverwriting(bool value);
  bool __fastcall GetConfirmResume();
  void __fastcall SetConfirmResume(bool value);
  bool __fastcall GetAutoReadDirectoryAfterOp();
  void __fastcall SetAutoReadDirectoryAfterOp(bool value);
  virtual bool __fastcall GetRememberPassword();
  UnicodeString __fastcall GetReleaseType();
  UnicodeString __fastcall LoadCustomIniFileStorageName();
  void __fastcall SaveCustomIniFileStorageName();
  UnicodeString __fastcall GetRegistryStorageOverrideKey();
  TStrings * GetCaches();
  UnicodeString GetFullVersion();

  virtual UnicodeString __fastcall ModuleFileName();

  UnicodeString GetFileFileInfoString(
    const UnicodeString & Key, const UnicodeString & FileName, bool AllowEmpty = false);
  void * GetFileApplicationInfo(const UnicodeString & FileName);
  UnicodeString GetFileProductVersion(const UnicodeString & FileName, bool AllowEmpty = false);
  UnicodeString GetFileProductName(const UnicodeString & FileName, bool AllowEmpty = false);
  UnicodeString GetFileCompanyName(const UnicodeString & FileName);

  bool __property_get_PermanentLogging() const { return ::rtl::Mutable(this)->GetLogging(); } void __property_set_PermanentLogging(bool Value) { SetLogging(Value); } __declspec(property(get = __property_get_PermanentLogging, put = __property_set_PermanentLogging)) bool PermanentLogging;
  UnicodeString __property_get_PermanentLogFileName() const { return ::rtl::Mutable(this)->GetLogFileName(); } void __property_set_PermanentLogFileName(UnicodeString Value) { SetLogFileName(Value); } __declspec(property(get = __property_get_PermanentLogFileName, put = __property_set_PermanentLogFileName)) UnicodeString PermanentLogFileName;
  bool __property_get_PermanentLogActions() const { return ::rtl::Mutable(this)->GetLogActions(); } void __property_set_PermanentLogActions(bool Value) { SetLogActions(Value); } __declspec(property(get = __property_get_PermanentLogActions, put = __property_set_PermanentLogActions)) bool PermanentLogActions;
  UnicodeString __property_get_PermanentActionsLogFileName() const { return ::rtl::Mutable(this)->GetPermanentActionsLogFileName(); } void __property_set_PermanentActionsLogFileName(UnicodeString Value) { SetActionsLogFileName(Value); } __declspec(property(get = __property_get_PermanentActionsLogFileName, put = __property_set_PermanentActionsLogFileName)) UnicodeString PermanentActionsLogFileName;
  int __property_get_PermanentLogProtocol() const { return FPermanentLogProtocol; } void __property_set_PermanentLogProtocol(int Value) { SetLogProtocol(Value); } __declspec(property(get = __property_get_PermanentLogProtocol, put = __property_set_PermanentLogProtocol)) int PermanentLogProtocol;
  bool __property_get_PermanentLogSensitive() const { return FPermanentLogSensitive; } void __property_set_PermanentLogSensitive(bool Value) { SetLogSensitive(Value); } __declspec(property(get = __property_get_PermanentLogSensitive, put = __property_set_PermanentLogSensitive)) bool PermanentLogSensitive;
  __int64 __property_get_PermanentLogMaxSize() const { return ::rtl::Mutable(this)->GetLogMaxSize(); } void __property_set_PermanentLogMaxSize(__int64 Value) { SetLogMaxSize(Value); } __declspec(property(get = __property_get_PermanentLogMaxSize, put = __property_set_PermanentLogMaxSize)) __int64 PermanentLogMaxSize;
  int __property_get_PermanentLogMaxCount() const { return ::rtl::Mutable(this)->GetLogMaxCount(); } void __property_set_PermanentLogMaxCount(int Value) { SetLogMaxCount(Value); } __declspec(property(get = __property_get_PermanentLogMaxCount, put = __property_set_PermanentLogMaxCount)) int PermanentLogMaxCount;

public:
  __fastcall TConfiguration();
  virtual __fastcall ~TConfiguration();
  virtual void __fastcall Default();
  virtual void __fastcall UpdateStaticUsage();
  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save();
  void __fastcall SaveExplicit();
  void __fastcall MoveStorage(TStorage AStorage, const UnicodeString & ACustomIniFileStorageName);
  void __fastcall ScheduleCustomIniFileStorageUse(const UnicodeString & ACustomIniFileStorageName);
  void __fastcall SetExplicitIniFileStorageName(const UnicodeString & FileName);
  void __fastcall SetNulStorage();
  UnicodeString __fastcall GetAutomaticIniFileStorageName(bool ReadingOnly);
  UnicodeString __fastcall GetDefaultIniFileExportPath();
  UnicodeString __fastcall GetIniFileParamValue();
  void __fastcall Export(const UnicodeString & FileName);
  void __fastcall Import(const UnicodeString & FileName);
  void __fastcall CleanupConfiguration();
  void __fastcall CleanupIniFile();
  void __fastcall CleanupCaches();
  bool __fastcall HasAnyCache();
  void __fastcall CleanupRandomSeedFile();
  void __fastcall BeginUpdate();
  void __fastcall EndUpdate();
  void __fastcall DontSave();
  void __fastcall LoadDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  void __fastcall SaveDirectoryChangesCache(const UnicodeString SessionKey,
    TRemoteDirectoryChangesCache * DirectoryChangesCache);
  TStrings * __fastcall LoadDirectoryStatisticsCache(
    const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam);
  void __fastcall SaveDirectoryStatisticsCache(
    const UnicodeString & SessionKey, const UnicodeString & Path, const TCopyParamType & CopyParam, TStrings * DataList);
  bool __fastcall ShowBanner(const UnicodeString & SessionKey, const UnicodeString & Banner, unsigned int & Params);
  void __fastcall NeverShowBanner(const UnicodeString & SessionKey, const UnicodeString & Banner);
  void __fastcall SetBannerParams(const UnicodeString & SessionKey, unsigned int Params);
  void __fastcall RememberLastFingerprint(const UnicodeString & SiteKey, const UnicodeString & FingerprintType, const UnicodeString & Fingerprint);
  UnicodeString __fastcall LastFingerprint(const UnicodeString & SiteKey, const UnicodeString & FingerprintType);
  THierarchicalStorage * CreateConfigStorage();
  THierarchicalStorage * CreateConfigRegistryStorage();
  virtual THierarchicalStorage * CreateScpStorage(bool & SessionList);
  void __fastcall TemporaryLogging(const UnicodeString ALogFileName);
  void __fastcall TemporaryActionsLogging(const UnicodeString ALogFileName);
  void __fastcall TemporaryLogProtocol(int ALogProtocol);
  void __fastcall TemporaryLogSensitive(bool ALogSensitive);
  void __fastcall TemporaryLogMaxSize(__int64 ALogMaxSize);
  void __fastcall TemporaryLogMaxCount(int ALogMaxCount);
  virtual RawByteString __fastcall EncryptPassword(UnicodeString Password, UnicodeString Key);
  virtual UnicodeString __fastcall DecryptPassword(RawByteString Password, UnicodeString Key);
  virtual RawByteString __fastcall StronglyRecryptPassword(RawByteString Password, UnicodeString Key);
  UnicodeString __fastcall GetFileDescription(const UnicodeString & FileName);
  UnicodeString __fastcall GetFileVersion(const UnicodeString & FileName);
  UnicodeString __fastcall GetFileMimeType(const UnicodeString & FileName);
  bool RegistryPathExists(const UnicodeString & RegistryPath);
  bool HasLocalPortNumberLimits();
  virtual UnicodeString TemporaryDir(bool Mask = false) = 0;
  UnicodeString GetVersionStrHuman();

  TStoredSessionList * __fastcall SelectFilezillaSessionsForImport(
    TStoredSessionList * Sessions, UnicodeString & Error);
  bool __fastcall AnyFilezillaSessionForImport(TStoredSessionList * Sessions);
  TStoredSessionList * __fastcall SelectKnownHostsSessionsForImport(
    TStoredSessionList * Sessions, UnicodeString & Error);
  TStoredSessionList * __fastcall SelectKnownHostsSessionsForImport(
    TStrings * Lines, TStoredSessionList * Sessions, UnicodeString & Error);
  TStoredSessionList * SelectOpensshSessionsForImport(TStoredSessionList * Sessions, UnicodeString & Error);
  TStoredSessionList * SelectSessionsForImport(TStoredSessionList * Sessions, const UnicodeString & FileName, UnicodeString & Error);
  UnicodeString GetPuttySessionsKey(const UnicodeString & RootKey);
  void RefreshPuttySshHostCAList();

  TVSFixedFileInfo * __property_get_FixedApplicationInfo() const { return ::rtl::Mutable(this)->GetFixedApplicationInfo(); } __declspec(property(get = __property_get_FixedApplicationInfo)) TVSFixedFileInfo * FixedApplicationInfo;
  void * __property_get_ApplicationInfo() const { return ::rtl::Mutable(this)->GetApplicationInfo(); } __declspec(property(get = __property_get_ApplicationInfo)) void * ApplicationInfo;
  TUsage * __property_get_Usage() const { return FUsage; } __declspec(property(get = __property_get_Usage)) TUsage * Usage;
  bool __property_get_CollectUsage() const { return ::rtl::Mutable(this)->GetCollectUsage(); } void __property_set_CollectUsage(bool Value) { SetCollectUsage(Value); } __declspec(property(get = __property_get_CollectUsage, put = __property_set_CollectUsage)) bool CollectUsage;
  UnicodeString __property_get_StoredSessionsSubKey() const { return ::rtl::Mutable(this)->GetStoredSessionsSubKey(); } __declspec(property(get = __property_get_StoredSessionsSubKey)) UnicodeString StoredSessionsSubKey;
  UnicodeString __property_get_PuttyRegistryStorageKey() const { return FPuttyRegistryStorageKey; } void __property_set_PuttyRegistryStorageKey(UnicodeString Value) { SetPuttyRegistryStorageKey(Value); } __declspec(property(get = __property_get_PuttyRegistryStorageKey, put = __property_set_PuttyRegistryStorageKey)) UnicodeString PuttyRegistryStorageKey;
  UnicodeString __property_get_PuttySessionsKey() const { return ::rtl::Mutable(this)->DoGetPuttySessionsKey(); } __declspec(property(get = __property_get_PuttySessionsKey)) UnicodeString PuttySessionsKey;
  UnicodeString __property_get_PuttySessionsSubKey() const { return ::rtl::Mutable(this)->GetPuttySessionsSubKey(); } __declspec(property(get = __property_get_PuttySessionsSubKey)) UnicodeString PuttySessionsSubKey;
  UnicodeString __property_get_RandomSeedFile() const { return FRandomSeedFile; } void __property_set_RandomSeedFile(UnicodeString Value) { SetRandomSeedFile(Value); } __declspec(property(get = __property_get_RandomSeedFile, put = __property_set_RandomSeedFile)) UnicodeString RandomSeedFile;
  UnicodeString __property_get_RandomSeedFileName() const { return ::rtl::Mutable(this)->GetRandomSeedFileName(); } __declspec(property(get = __property_get_RandomSeedFileName)) UnicodeString RandomSeedFileName;
  UnicodeString __property_get_SshHostKeysSubKey() const { return ::rtl::Mutable(this)->GetSshHostKeysSubKey(); } __declspec(property(get = __property_get_SshHostKeysSubKey)) UnicodeString SshHostKeysSubKey;
  UnicodeString __property_get_RootKeyStr() const { return ::rtl::Mutable(this)->GetRootKeyStr(); } __declspec(property(get = __property_get_RootKeyStr)) UnicodeString RootKeyStr;
  UnicodeString __property_get_ConfigurationSubKey() const { return ::rtl::Mutable(this)->GetConfigurationSubKey(); } __declspec(property(get = __property_get_ConfigurationSubKey)) UnicodeString ConfigurationSubKey;
  TEOLType __property_get_LocalEOLType() const { return ::rtl::Mutable(this)->GetLocalEOLType(); } __declspec(property(get = __property_get_LocalEOLType)) TEOLType LocalEOLType;
  UnicodeString __property_get_VersionStr() const { return ::rtl::Mutable(this)->GetVersionStr(); } __declspec(property(get = __property_get_VersionStr)) UnicodeString VersionStr;
  UnicodeString __property_get_Version() const { return ::rtl::Mutable(this)->GetVersion(); } __declspec(property(get = __property_get_Version)) UnicodeString Version;
  int __property_get_CompoundVersion() const { return ::rtl::Mutable(this)->GetCompoundVersion(); } __declspec(property(get = __property_get_CompoundVersion)) int CompoundVersion;
  UnicodeString __property_get_ProductVersion() const { return ::rtl::Mutable(this)->GetProductVersion(); } __declspec(property(get = __property_get_ProductVersion)) UnicodeString ProductVersion;
  UnicodeString __property_get_ProductName() const { return ::rtl::Mutable(this)->GetProductName(); } __declspec(property(get = __property_get_ProductName)) UnicodeString ProductName;
  UnicodeString __property_get_CompanyName() const { return ::rtl::Mutable(this)->GetCompanyName(); } __declspec(property(get = __property_get_CompanyName)) UnicodeString CompanyName;
  bool __property_get_IsUnofficial() const { return ::rtl::Mutable(this)->GetIsUnofficial(); } __declspec(property(get = __property_get_IsUnofficial)) bool IsUnofficial;
  bool __property_get_Logging() const { return FLogging; } void __property_set_Logging(bool Value) { SetLogging(Value); } __declspec(property(get = __property_get_Logging, put = __property_set_Logging)) bool Logging;
  UnicodeString __property_get_LogFileName() const { return FLogFileName; } void __property_set_LogFileName(UnicodeString Value) { SetLogFileName(Value); } __declspec(property(get = __property_get_LogFileName, put = __property_set_LogFileName)) UnicodeString LogFileName;
  bool __property_get_LogToFile() const { return ::rtl::Mutable(this)->GetLogToFile(); } __declspec(property(get = __property_get_LogToFile)) bool LogToFile;
  bool __property_get_LogFileAppend() const { return FLogFileAppend; } void __property_set_LogFileAppend(bool Value) { SetLogFileAppend(Value); } __declspec(property(get = __property_get_LogFileAppend, put = __property_set_LogFileAppend)) bool LogFileAppend;
  bool __property_get_LogSensitive() const { return FLogSensitive; } void __property_set_LogSensitive(bool Value) { SetLogSensitive(Value); } __declspec(property(get = __property_get_LogSensitive, put = __property_set_LogSensitive)) bool LogSensitive;
  __int64 __property_get_LogMaxSize() const { return FLogMaxSize; } void __property_set_LogMaxSize(__int64 Value) { SetLogMaxSize(Value); } __declspec(property(get = __property_get_LogMaxSize, put = __property_set_LogMaxSize)) __int64 LogMaxSize;
  int __property_get_LogMaxCount() const { return FLogMaxCount; } void __property_set_LogMaxCount(int Value) { SetLogMaxCount(Value); } __declspec(property(get = __property_get_LogMaxCount, put = __property_set_LogMaxCount)) int LogMaxCount;
  int __property_get_LogProtocol() const { return FLogProtocol; } void __property_set_LogProtocol(int Value) { SetLogProtocol(Value); } __declspec(property(get = __property_get_LogProtocol, put = __property_set_LogProtocol)) int LogProtocol;
  int __property_get_ActualLogProtocol() const { return FActualLogProtocol; } __declspec(property(get = __property_get_ActualLogProtocol)) int ActualLogProtocol;
  bool __property_get_LogActions() const { return FLogActions; } void __property_set_LogActions(bool Value) { SetLogActions(Value); } __declspec(property(get = __property_get_LogActions, put = __property_set_LogActions)) bool LogActions;
  bool __property_get_LogActionsRequired() const { return FLogActionsRequired; } void __property_set_LogActionsRequired(bool Value) { FLogActionsRequired = Value; } __declspec(property(get = __property_get_LogActionsRequired, put = __property_set_LogActionsRequired)) bool LogActionsRequired;
  UnicodeString __property_get_ActionsLogFileName() const { return ::rtl::Mutable(this)->GetActionsLogFileName(); } void __property_set_ActionsLogFileName(UnicodeString Value) { SetActionsLogFileName(Value); } __declspec(property(get = __property_get_ActionsLogFileName, put = __property_set_ActionsLogFileName)) UnicodeString ActionsLogFileName;
  UnicodeString __property_get_DefaultLogFileName() const { return ::rtl::Mutable(this)->GetDefaultLogFileName(); } __declspec(property(get = __property_get_DefaultLogFileName)) UnicodeString DefaultLogFileName;
  TNotifyEvent __property_get_OnChange() const { return FOnChange; } void __property_set_OnChange(TNotifyEvent Value) { FOnChange = Value; } __declspec(property(get = __property_get_OnChange, put = __property_set_OnChange)) TNotifyEvent OnChange;
  bool __property_get_ConfirmOverwriting() const { return ::rtl::Mutable(this)->GetConfirmOverwriting(); } void __property_set_ConfirmOverwriting(bool Value) { SetConfirmOverwriting(Value); } __declspec(property(get = __property_get_ConfirmOverwriting, put = __property_set_ConfirmOverwriting)) bool ConfirmOverwriting;
  bool __property_get_ConfirmResume() const { return ::rtl::Mutable(this)->GetConfirmResume(); } void __property_set_ConfirmResume(bool Value) { SetConfirmResume(Value); } __declspec(property(get = __property_get_ConfirmResume, put = __property_set_ConfirmResume)) bool ConfirmResume;
  bool __property_get_AutoReadDirectoryAfterOp() const { return ::rtl::Mutable(this)->GetAutoReadDirectoryAfterOp(); } void __property_set_AutoReadDirectoryAfterOp(bool Value) { SetAutoReadDirectoryAfterOp(Value); } __declspec(property(get = __property_get_AutoReadDirectoryAfterOp, put = __property_set_AutoReadDirectoryAfterOp)) bool AutoReadDirectoryAfterOp;
  bool __property_get_RememberPassword() const { return ::rtl::Mutable(this)->GetRememberPassword(); } __declspec(property(get = __property_get_RememberPassword)) bool RememberPassword;
  int __property_get_SessionReopenAuto() const { return FSessionReopenAuto; } void __property_set_SessionReopenAuto(int Value) { SetSessionReopenAuto(Value); } __declspec(property(get = __property_get_SessionReopenAuto, put = __property_set_SessionReopenAuto)) int SessionReopenAuto;
  int __property_get_SessionReopenBackground() const { return FSessionReopenBackground; } void __property_set_SessionReopenBackground(int Value) { SetSessionReopenBackground(Value); } __declspec(property(get = __property_get_SessionReopenBackground, put = __property_set_SessionReopenBackground)) int SessionReopenBackground;
  int __property_get_SessionReopenTimeout() const { return FSessionReopenTimeout; } void __property_set_SessionReopenTimeout(int Value) { SetSessionReopenTimeout(Value); } __declspec(property(get = __property_get_SessionReopenTimeout, put = __property_set_SessionReopenTimeout)) int SessionReopenTimeout;
  int __property_get_SessionReopenAutoStall() const { return FSessionReopenAutoStall; } void __property_set_SessionReopenAutoStall(int Value) { SetSessionReopenAutoStall(Value); } __declspec(property(get = __property_get_SessionReopenAutoStall, put = __property_set_SessionReopenAutoStall)) int SessionReopenAutoStall;
  int __property_get_TunnelLocalPortNumberLow() const { return FTunnelLocalPortNumberLow; } void __property_set_TunnelLocalPortNumberLow(int Value) { SetTunnelLocalPortNumberLow(Value); } __declspec(property(get = __property_get_TunnelLocalPortNumberLow, put = __property_set_TunnelLocalPortNumberLow)) int TunnelLocalPortNumberLow;
  int __property_get_TunnelLocalPortNumberHigh() const { return FTunnelLocalPortNumberHigh; } void __property_set_TunnelLocalPortNumberHigh(int Value) { SetTunnelLocalPortNumberHigh(Value); } __declspec(property(get = __property_get_TunnelLocalPortNumberHigh, put = __property_set_TunnelLocalPortNumberHigh)) int TunnelLocalPortNumberHigh;
  int __property_get_CacheDirectoryChangesMaxSize() const { return FCacheDirectoryChangesMaxSize; } void __property_set_CacheDirectoryChangesMaxSize(int Value) { SetCacheDirectoryChangesMaxSize(Value); } __declspec(property(get = __property_get_CacheDirectoryChangesMaxSize, put = __property_set_CacheDirectoryChangesMaxSize)) int CacheDirectoryChangesMaxSize;
  bool __property_get_ShowFtpWelcomeMessage() const { return FShowFtpWelcomeMessage; } void __property_set_ShowFtpWelcomeMessage(bool Value) { SetShowFtpWelcomeMessage(Value); } __declspec(property(get = __property_get_ShowFtpWelcomeMessage, put = __property_set_ShowFtpWelcomeMessage)) bool ShowFtpWelcomeMessage;
  UnicodeString __property_get_ExternalIpAddress() const { return FExternalIpAddress; } void __property_set_ExternalIpAddress(UnicodeString Value) { SetExternalIpAddress(Value); } __declspec(property(get = __property_get_ExternalIpAddress, put = __property_set_ExternalIpAddress)) UnicodeString ExternalIpAddress;
  UnicodeString __property_get_CertificateStorage() const { return FCertificateStorage; } void __property_set_CertificateStorage(UnicodeString Value) { SetCertificateStorage(Value); } __declspec(property(get = __property_get_CertificateStorage, put = __property_set_CertificateStorage)) UnicodeString CertificateStorage;
  UnicodeString __property_get_CertificateStorageExpanded() const { return ::rtl::Mutable(this)->GetCertificateStorageExpanded(); } __declspec(property(get = __property_get_CertificateStorageExpanded)) UnicodeString CertificateStorageExpanded;
  UnicodeString __property_get_AWSAPI() const { return FAWSAPI; } void __property_set_AWSAPI(UnicodeString Value) { SetAWSAPI(Value); } __declspec(property(get = __property_get_AWSAPI, put = __property_set_AWSAPI)) UnicodeString AWSAPI;
  UnicodeString __property_get_ChecksumCommands() const { return FChecksumCommands; } void __property_set_ChecksumCommands(UnicodeString Value) { FChecksumCommands = Value; } __declspec(property(get = __property_get_ChecksumCommands, put = __property_set_ChecksumCommands)) UnicodeString ChecksumCommands;
  int __property_get_LocalPortNumberMin() const { return FLocalPortNumberMin; } void __property_set_LocalPortNumberMin(int Value) { SetLocalPortNumberMin(Value); } __declspec(property(get = __property_get_LocalPortNumberMin, put = __property_set_LocalPortNumberMin)) int LocalPortNumberMin;
  int __property_get_LocalPortNumberMax() const { return FLocalPortNumberMax; } void __property_set_LocalPortNumberMax(int Value) { SetLocalPortNumberMax(Value); } __declspec(property(get = __property_get_LocalPortNumberMax, put = __property_set_LocalPortNumberMax)) int LocalPortNumberMax;
  bool __property_get_TryFtpWhenSshFails() const { return FTryFtpWhenSshFails; } void __property_set_TryFtpWhenSshFails(bool Value) { SetTryFtpWhenSshFails(Value); } __declspec(property(get = __property_get_TryFtpWhenSshFails, put = __property_set_TryFtpWhenSshFails)) bool TryFtpWhenSshFails;
  int __property_get_ParallelDurationThreshold() const { return FParallelDurationThreshold; } void __property_set_ParallelDurationThreshold(int Value) { SetParallelDurationThreshold(Value); } __declspec(property(get = __property_get_ParallelDurationThreshold, put = __property_set_ParallelDurationThreshold)) int ParallelDurationThreshold;
  UnicodeString __property_get_MimeTypes() const { return FMimeTypes; } void __property_set_MimeTypes(UnicodeString Value) { SetMimeTypes(Value); } __declspec(property(get = __property_get_MimeTypes, put = __property_set_MimeTypes)) UnicodeString MimeTypes;
  int __property_get_DontReloadMoreThanSessions() const { return FDontReloadMoreThanSessions; } void __property_set_DontReloadMoreThanSessions(int Value) { FDontReloadMoreThanSessions = Value; } __declspec(property(get = __property_get_DontReloadMoreThanSessions, put = __property_set_DontReloadMoreThanSessions)) int DontReloadMoreThanSessions;
  int __property_get_ScriptProgressFileNameLimit() const { return FScriptProgressFileNameLimit; } void __property_set_ScriptProgressFileNameLimit(int Value) { FScriptProgressFileNameLimit = Value; } __declspec(property(get = __property_get_ScriptProgressFileNameLimit, put = __property_set_ScriptProgressFileNameLimit)) int ScriptProgressFileNameLimit;
  int __property_get_QueueTransfersLimit() const { return FQueueTransfersLimit; } void __property_set_QueueTransfersLimit(int Value) { SetQueueTransfersLimit(Value); } __declspec(property(get = __property_get_QueueTransfersLimit, put = __property_set_QueueTransfersLimit)) int QueueTransfersLimit;
  int __property_get_ParallelTransferThreshold() const { return FParallelTransferThreshold; } void __property_set_ParallelTransferThreshold(int Value) { FParallelTransferThreshold = Value; } __declspec(property(get = __property_get_ParallelTransferThreshold, put = __property_set_ParallelTransferThreshold)) int ParallelTransferThreshold;
  int __property_get_KeyVersion() const { return FKeyVersion; } void __property_set_KeyVersion(int Value) { FKeyVersion = Value; } __declspec(property(get = __property_get_KeyVersion, put = __property_set_KeyVersion)) int KeyVersion;
  const TSshHostCAList * __property_get_SshHostCAList() const { return ::rtl::Mutable(this)->GetSshHostCAList(); } void __property_set_SshHostCAList(TSshHostCAList * Value) { SetSshHostCAList(Value); } __declspec(property(get = __property_get_SshHostCAList, put = __property_set_SshHostCAList)) TSshHostCAList * SshHostCAList;
  const TSshHostCAList * __property_get_PuttySshHostCAList() const { return ::rtl::Mutable(this)->GetPuttySshHostCAList(); } __declspec(property(get = __property_get_PuttySshHostCAList)) TSshHostCAList * PuttySshHostCAList;
  const TSshHostCAList * __property_get_ActiveSshHostCAList() const { return ::rtl::Mutable(this)->GetActiveSshHostCAList(); } __declspec(property(get = __property_get_ActiveSshHostCAList)) TSshHostCAList * ActiveSshHostCAList;
  bool __property_get_SshHostCAsFromPuTTY() const { return FSshHostCAsFromPuTTY; } void __property_set_SshHostCAsFromPuTTY(bool Value) { FSshHostCAsFromPuTTY = Value; } __declspec(property(get = __property_get_SshHostCAsFromPuTTY, put = __property_set_SshHostCAsFromPuTTY)) bool SshHostCAsFromPuTTY;
  int __property_get_HttpsCertificateValidation() const { return FHttpsCertificateValidation; } void __property_set_HttpsCertificateValidation(int Value) { FHttpsCertificateValidation = Value; } __declspec(property(get = __property_get_HttpsCertificateValidation, put = __property_set_HttpsCertificateValidation)) int HttpsCertificateValidation;
  UnicodeString __property_get_SynchronizationChecksumAlgs() const { return FSynchronizationChecksumAlgs; } void __property_set_SynchronizationChecksumAlgs(UnicodeString Value) { FSynchronizationChecksumAlgs = Value; } __declspec(property(get = __property_get_SynchronizationChecksumAlgs, put = __property_set_SynchronizationChecksumAlgs)) UnicodeString SynchronizationChecksumAlgs;
  int __property_get_AuthAgent() const { return ::rtl::Mutable(this)->GetAuthAgent(); } void __property_set_AuthAgent(int Value) { SetAuthAgent(Value); } __declspec(property(get = __property_get_AuthAgent, put = __property_set_AuthAgent)) int AuthAgent;

  UnicodeString __property_get_TimeFormat() const { return ::rtl::Mutable(this)->GetTimeFormat(); } __declspec(property(get = __property_get_TimeFormat)) UnicodeString TimeFormat;
  TStorage __property_get_Storage() const { return ::rtl::Mutable(this)->GetStorage(); } __declspec(property(get = __property_get_Storage)) TStorage Storage;
  UnicodeString __property_get_RegistryStorageKey() const { return ::rtl::Mutable(this)->GetRegistryStorageKey(); } __declspec(property(get = __property_get_RegistryStorageKey)) UnicodeString RegistryStorageKey;
  UnicodeString __property_get_CustomIniFileStorageName() const { return FCustomIniFileStorageName; } __declspec(property(get = __property_get_CustomIniFileStorageName)) UnicodeString CustomIniFileStorageName;
  UnicodeString __property_get_ExplicitIniFileStorageName() const { return FIniFileStorageName; } __declspec(property(get = __property_get_ExplicitIniFileStorageName)) UnicodeString ExplicitIniFileStorageName;
  UnicodeString __property_get_IniFileStorageName() const { return ::rtl::Mutable(this)->GetIniFileStorageNameForReadingWriting(); } __declspec(property(get = __property_get_IniFileStorageName)) UnicodeString IniFileStorageName;
  UnicodeString __property_get_IniFileStorageNameForReading() const { return ::rtl::Mutable(this)->GetIniFileStorageNameForReading(); } __declspec(property(get = __property_get_IniFileStorageNameForReading)) UnicodeString IniFileStorageNameForReading;
  TStrings * __property_get_OptionsStorage() const { return ::rtl::Mutable(this)->GetOptionsStorage(); } void __property_set_OptionsStorage(TStrings * Value) { SetOptionsStorage(Value); } __declspec(property(get = __property_get_OptionsStorage, put = __property_set_OptionsStorage)) TStrings * OptionsStorage;
  bool __property_get_Persistent() const { return ::rtl::Mutable(this)->GetPersistent(); } __declspec(property(get = __property_get_Persistent)) bool Persistent;
  bool __property_get_ForceSave() const { return FForceSave; } void __property_set_ForceSave(bool Value) { FForceSave = Value; } __declspec(property(get = __property_get_ForceSave, put = __property_set_ForceSave)) bool ForceSave;
  bool __property_get_Scripting() const { return FScripting; } void __property_set_Scripting(bool Value) { FScripting = Value; } __declspec(property(get = __property_get_Scripting, put = __property_set_Scripting)) bool Scripting;

  UnicodeString __property_get_DefaultKeyFile() const { return ::rtl::Mutable(this)->GetDefaultKeyFile(); } __declspec(property(get = __property_get_DefaultKeyFile)) UnicodeString DefaultKeyFile;

  bool __property_get_DisablePasswordStoring() const { return FDisablePasswordStoring; } __declspec(property(get = __property_get_DisablePasswordStoring)) bool DisablePasswordStoring;
  bool __property_get_ForceBanners() const { return FForceBanners; } __declspec(property(get = __property_get_ForceBanners)) bool ForceBanners;
  bool __property_get_DisableAcceptingHostKeys() const { return FDisableAcceptingHostKeys; } __declspec(property(get = __property_get_DisableAcceptingHostKeys)) bool DisableAcceptingHostKeys;
};
//---------------------------------------------------------------------------
class TShortCuts
{
public:
  void __fastcall Add(TShortCut ShortCut);
  bool __fastcall Has(TShortCut ShortCut) const;

private:
  std::set<TShortCut> FShortCuts;
};
//---------------------------------------------------------------------------
extern const UnicodeString OriginalPuttyRegistryStorageKey;
extern const UnicodeString KittyRegistryStorageKey;
extern const UnicodeString OriginalPuttyExecutable;
extern const UnicodeString KittyExecutable;
//---------------------------------------------------------------------------
extern const UnicodeString Sha1ChecksumAlg;
extern const UnicodeString Sha224ChecksumAlg;
extern const UnicodeString Sha256ChecksumAlg;
extern const UnicodeString Sha384ChecksumAlg;
extern const UnicodeString Sha512ChecksumAlg;
extern const UnicodeString Md5ChecksumAlg;
extern const UnicodeString Crc32ChecksumAlg;
//---------------------------------------------------------------------------
extern const UnicodeString SshFingerprintType;
extern const UnicodeString TlsFingerprintType;
//---------------------------------------------------------------------------
extern const UnicodeString FtpsCertificateStorageKey;
extern const UnicodeString HttpsCertificateStorageKey;
//---------------------------------------------------------------------------
extern const int BelowNormalLogLevels;
//---------------------------------------------------------------------------
extern const UnicodeString OpensshFolderName;
extern const UnicodeString OpensshAuthorizedKeysFileName;
//---------------------------------------------------------------------------
#endif
