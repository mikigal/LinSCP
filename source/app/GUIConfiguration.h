//---------------------------------------------------------------------------
#ifndef GUIConfigurationH
#define GUIConfigurationH
//---------------------------------------------------------------------------
#include "Configuration.h"
#include "CopyParam.h"
//---------------------------------------------------------------------------
class TGUIConfiguration;
class TStoredSessionList;
enum TInterface : int { ifCommander, ifExplorer };
//---------------------------------------------------------------------------
extern const int ccLocal;
extern const int ccShowResults;
extern const int ccCopyResults;
extern const int ccSet;
extern const int ccRemoteFiles;
extern const int ccShowResultsInMsgBox;
extern const int SessionReopenAutoIdleDefault;
//---------------------------------------------------------------------------
const int soRecurse =         0x01;
const int soSynchronize =     0x02;
const int soSynchronizeAsk =  0x04;
const int soContinueOnError = 0x08;
//---------------------------------------------------------------------------
class TGUICopyParamType : public TCopyParamType
{
public:
  __fastcall TGUICopyParamType();
  __fastcall TGUICopyParamType(const TCopyParamType & Source);
  __fastcall TGUICopyParamType(const TGUICopyParamType & Source);

  virtual void __fastcall Load(THierarchicalStorage * Storage);
  virtual void __fastcall Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults = NULL) const;

  virtual void __fastcall Default();
  virtual void __fastcall Assign(const TCopyParamType * Source);
  TGUICopyParamType & __fastcall operator =(const TGUICopyParamType & rhp);
  TGUICopyParamType & __fastcall operator =(const TCopyParamType & rhp);

  bool __property_get_Queue() const { return FQueue; } void __property_set_Queue(bool Value) { FQueue = Value; } __declspec(property(get = __property_get_Queue, put = __property_set_Queue)) bool Queue;
  bool __property_get_QueueNoConfirmation() const { return FQueueNoConfirmation; } void __property_set_QueueNoConfirmation(bool Value) { FQueueNoConfirmation = Value; } __declspec(property(get = __property_get_QueueNoConfirmation, put = __property_set_QueueNoConfirmation)) bool QueueNoConfirmation;
  bool __property_get_QueueParallel() const { return FQueueParallel; } void __property_set_QueueParallel(bool Value) { FQueueParallel = Value; } __declspec(property(get = __property_get_QueueParallel, put = __property_set_QueueParallel)) bool QueueParallel;

protected:
  void __fastcall GUIDefault();
  void __fastcall GUIAssign(const TGUICopyParamType * Source);

private:
  bool FQueue;
  bool FQueueNoConfirmation;
  bool FQueueParallel;
};
//---------------------------------------------------------------------------
struct TCopyParamRuleData
{
  UnicodeString HostName;
  UnicodeString UserName;
  UnicodeString RemoteDirectory;
  UnicodeString LocalDirectory;

  void __fastcall Default();
};
//---------------------------------------------------------------------------
class TCopyParamRule
{
public:
  __fastcall TCopyParamRule();
  __fastcall TCopyParamRule(const TCopyParamRuleData & Data);
  __fastcall TCopyParamRule(const TCopyParamRule & Source);

  bool __fastcall Matches(const TCopyParamRuleData & Value) const;
  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage) const;

  UnicodeString __fastcall GetInfoStr(UnicodeString Separator) const;

  bool __fastcall operator ==(const TCopyParamRule & rhp) const;

  TCopyParamRuleData __property_get_Data() const { return FData; } void __property_set_Data(TCopyParamRuleData Value) { FData = Value; } __declspec(property(get = __property_get_Data, put = __property_set_Data)) TCopyParamRuleData Data;
  bool __property_get_IsEmpty() const { return ::rtl::Mutable(this)->GetEmpty(); } __declspec(property(get = __property_get_IsEmpty)) bool IsEmpty;

private:
  TCopyParamRuleData FData;

  inline bool __fastcall Match(const UnicodeString & Mask,
    const UnicodeString & Value, bool Path, bool Local, int ForceDirectoryMasks) const;
  bool __fastcall GetEmpty() const;
};
//---------------------------------------------------------------------------
class TLocaleInfo : public TObject
{
public:
  LCID Locale;
  UnicodeString Name;
  int Completeness;
};
//---------------------------------------------------------------------------
class TCopyParamList
{
friend class TGUIConfiguration;
public:
  __fastcall TCopyParamList();
  virtual __fastcall ~TCopyParamList();
  int __fastcall Find(const TCopyParamRuleData & Value) const;

  void __fastcall Load(THierarchicalStorage * Storage, int Count);
  void __fastcall Save(THierarchicalStorage * Storage) const;

  static void __fastcall ValidateName(const UnicodeString Name);

  TCopyParamList & __fastcall operator=(const TCopyParamList & rhl);
  bool __fastcall operator==(const TCopyParamList & rhl) const;

  void __fastcall Clear();
  void __fastcall Add(const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void __fastcall Insert(int Index, const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void __fastcall Change(int Index, const UnicodeString Name,
    TCopyParamType * CopyParam, TCopyParamRule * Rule);
  void __fastcall Move(int CurIndex, int NewIndex);
  void __fastcall Delete(int Index);
  int __fastcall IndexOfName(const UnicodeString Name) const;

  int __property_get_Count() const { return ::rtl::Mutable(this)->GetCount(); } __declspec(property(get = __property_get_Count)) int Count;
  UnicodeString __property_get_Names(int Index) const { return ::rtl::Mutable(this)->GetName(Index); } __declspec(property(get = __property_get_Names)) UnicodeString Names[];
  const TCopyParamRule * __property_get_Rules(int Index) const { return ::rtl::Mutable(this)->GetRule(Index); } __declspec(property(get = __property_get_Rules)) const TCopyParamRule * Rules[];
  const TCopyParamType * __property_get_CopyParams(int Index) const { return ::rtl::Mutable(this)->GetCopyParam(Index); } __declspec(property(get = __property_get_CopyParams)) const TCopyParamType * CopyParams[];
  bool __property_get_Modified() const { return FModified; } __declspec(property(get = __property_get_Modified)) bool Modified;
  TStrings * __property_get_NameList() const { return ::rtl::Mutable(this)->GetNameList(); } __declspec(property(get = __property_get_NameList)) TStrings * NameList;
  bool __property_get_AnyRule() const { return ::rtl::Mutable(this)->GetAnyRule(); } __declspec(property(get = __property_get_AnyRule)) bool AnyRule;

private:
  static UnicodeString FInvalidChars;
  TList * FRules;
  TList * FCopyParams;
  TStrings * FNames;
  mutable TStrings * FNameList;
  bool FModified;

  int __fastcall GetCount() const;
  const TCopyParamRule * __fastcall GetRule(int Index) const;
  const TCopyParamType * __fastcall GetCopyParam(int Index) const;
  UnicodeString __fastcall GetName(int Index) const;
  TStrings * __fastcall GetNameList() const;
  bool __fastcall GetAnyRule() const;

  void __fastcall Init();
  void __fastcall Reset();
  void __fastcall Modify();
  bool __fastcall CompareItem(int Index, const TCopyParamType * CopyParam,
    const TCopyParamRule * Rule) const;
};
//---------------------------------------------------------------------------
class TGUIConfiguration : public TConfiguration
{
private:
  TObjectList * FLocales;
  UnicodeString FLastLocalesExts;
  bool FContinueOnError;
  bool FConfirmCommandSession;
  UnicodeString FPuttyPath;
  TAutoSwitch FUsePuttyPwFile;
  bool FPuttyPassword;
  bool FTelnetForFtpInPutty;
  UnicodeString FPuttySession;
  int FSynchronizeParams;
  int FSynchronizeOptions;
  int FSynchronizeModeAuto;
  int FSynchronizeMode;
  int FMaxWatchDirectories;
  TDateTime FIgnoreCancelBeforeFinish;
  bool FQueueAutoPopup;
  bool FSessionRememberPassword;
  bool FQueueBootstrap;
  bool FQueueKeepDoneItems;
  int FQueueKeepDoneItemsFor;
  TGUICopyParamType FDefaultCopyParam;
  bool FBeepOnFinish;
  TDateTime FBeepOnFinishAfter;
  UnicodeString FBeepSound;
  UnicodeString FDefaultPuttyPathOnly;
  UnicodeString FDefaultPuttyPath;
  TCopyParamList * FCopyParamList;
  bool FCopyParamListDefaults;
  UnicodeString FCopyParamCurrent;
  TRemoteProperties FNewDirectoryProperties;
  int FKeepUpToDateChangeDelay;
  UnicodeString FChecksumAlg;
  int FSessionReopenAutoIdle;
  bool FSessionReopenAutoIdleOn;
  bool FSessionReopenAutoInactive;
  bool FSessionSilentDisconnect;
  LCID FAppliedLocale;
  // Corresponds to FAppliedLocale
  UnicodeString FLocaleModuleName;

protected:
  LCID FLocale;

  virtual void __fastcall SaveData(THierarchicalStorage * Storage, bool All);
  virtual void __fastcall LoadData(THierarchicalStorage * Storage);
  LCID __fastcall GetLocale();
  void __fastcall SetLocale(LCID value);
  void __fastcall SetLocaleSafe(LCID value);
  UnicodeString __fastcall GetAppliedLocaleHex();
  virtual HMODULE LoadNewResourceModule(LCID Locale, UnicodeString & FileName);
  void SetResourceModule(HMODULE Instance);
  TObjectList * __fastcall GetLocales();
  void __fastcall AddLocale(LCID Locale, const UnicodeString & Name);
  void FreeResourceModule(HINSTANCE Instance);
  void __fastcall SetDefaultCopyParam(const TGUICopyParamType & value);
  virtual bool __fastcall GetRememberPassword();
  const TCopyParamList * __fastcall GetCopyParamList();
  void __fastcall SetCopyParamList(const TCopyParamList * value);
  virtual void __fastcall DefaultLocalized();
  int __fastcall GetCopyParamIndex();
  TGUICopyParamType __fastcall GetCurrentCopyParam();
  TGUICopyParamType __fastcall GetCopyParamPreset(UnicodeString Name);
  bool __fastcall GetHasCopyParamPreset(UnicodeString Name);
  void __fastcall SetCopyParamIndex(int value);
  void __fastcall SetCopyParamCurrent(UnicodeString value);
  void __fastcall SetNewDirectoryProperties(const TRemoteProperties & value);
  virtual void __fastcall Saved();
  void __fastcall SetQueueBootstrap(bool value);
  void __fastcall SetQueueKeepDoneItems(bool value);
  void __fastcall SetQueueKeepDoneItemsFor(int value);
  void __fastcall SetLocaleInternal(LCID value, bool Safe, bool CompleteOnly);
  void __fastcall SetAppliedLocale(LCID AppliedLocale, const UnicodeString & LocaleModuleName);
  bool __fastcall GetCanApplyLocaleImmediately();
  UnicodeString __fastcall GetTranslationModule(const UnicodeString & Path);
  void __fastcall FindLocales(const UnicodeString & Path, TStrings * Exts, UnicodeString & LocalesExts);
  virtual int GetResourceModuleCompleteness(HMODULE Module);
  virtual bool IsTranslationComplete(HMODULE Module);
  static int __fastcall LocalesCompare(void * Item1, void * Item2);
  LCID __fastcall InternalLocale();
  bool __fastcall DoSaveCopyParam(THierarchicalStorage * Storage, const TCopyParamType * CopyParam, const TCopyParamType * Defaults);

public:
  __fastcall TGUIConfiguration();
  virtual __fastcall ~TGUIConfiguration();
  virtual void __fastcall Default();
  virtual void __fastcall UpdateStaticUsage();
  bool __fastcall LoadCopyParam(THierarchicalStorage * Storage, TCopyParamType * CopyParam);
  void __fastcall LoadDefaultCopyParam(THierarchicalStorage * Storage);

  HINSTANCE ChangeToDefaultResourceModule();
  HINSTANCE ChangeResourceModule(HINSTANCE Instance);
  bool __fastcall UsingInternalTranslation();
  UnicodeString __fastcall AppliedLocaleCopyright();
  UnicodeString __fastcall AppliedLocaleVersion();
  TStoredSessionList * __fastcall SelectPuttySessionsForImport(
    const UnicodeString & RootKey, const UnicodeString & Source, TStoredSessionList * Sessions, UnicodeString & Error);
  bool __fastcall AnyPuttySessionForImport(TStoredSessionList * Sessions);

  bool __property_get_ContinueOnError() const { return FContinueOnError; } void __property_set_ContinueOnError(bool Value) { FContinueOnError = Value; } __declspec(property(get = __property_get_ContinueOnError, put = __property_set_ContinueOnError)) bool ContinueOnError;
  bool __property_get_ConfirmCommandSession() const { return FConfirmCommandSession; } void __property_set_ConfirmCommandSession(bool Value) { FConfirmCommandSession = Value; } __declspec(property(get = __property_get_ConfirmCommandSession, put = __property_set_ConfirmCommandSession)) bool ConfirmCommandSession;
  int __property_get_SynchronizeParams() const { return FSynchronizeParams; } void __property_set_SynchronizeParams(int Value) { FSynchronizeParams = Value; } __declspec(property(get = __property_get_SynchronizeParams, put = __property_set_SynchronizeParams)) int SynchronizeParams;
  int __property_get_SynchronizeOptions() const { return FSynchronizeOptions; } void __property_set_SynchronizeOptions(int Value) { FSynchronizeOptions = Value; } __declspec(property(get = __property_get_SynchronizeOptions, put = __property_set_SynchronizeOptions)) int SynchronizeOptions;
  int __property_get_SynchronizeModeAuto() const { return FSynchronizeModeAuto; } void __property_set_SynchronizeModeAuto(int Value) { FSynchronizeModeAuto = Value; } __declspec(property(get = __property_get_SynchronizeModeAuto, put = __property_set_SynchronizeModeAuto)) int SynchronizeModeAuto;
  int __property_get_SynchronizeMode() const { return FSynchronizeMode; } void __property_set_SynchronizeMode(int Value) { FSynchronizeMode = Value; } __declspec(property(get = __property_get_SynchronizeMode, put = __property_set_SynchronizeMode)) int SynchronizeMode;
  int __property_get_MaxWatchDirectories() const { return FMaxWatchDirectories; } void __property_set_MaxWatchDirectories(int Value) { FMaxWatchDirectories = Value; } __declspec(property(get = __property_get_MaxWatchDirectories, put = __property_set_MaxWatchDirectories)) int MaxWatchDirectories;
  bool __property_get_QueueBootstrap() const { return FQueueBootstrap; } void __property_set_QueueBootstrap(bool Value) { SetQueueBootstrap(Value); } __declspec(property(get = __property_get_QueueBootstrap, put = __property_set_QueueBootstrap)) bool QueueBootstrap;
  bool __property_get_QueueKeepDoneItems() const { return FQueueKeepDoneItems; } void __property_set_QueueKeepDoneItems(bool Value) { SetQueueKeepDoneItems(Value); } __declspec(property(get = __property_get_QueueKeepDoneItems, put = __property_set_QueueKeepDoneItems)) bool QueueKeepDoneItems;
  int __property_get_QueueKeepDoneItemsFor() const { return FQueueKeepDoneItemsFor; } void __property_set_QueueKeepDoneItemsFor(int Value) { SetQueueKeepDoneItemsFor(Value); } __declspec(property(get = __property_get_QueueKeepDoneItemsFor, put = __property_set_QueueKeepDoneItemsFor)) int QueueKeepDoneItemsFor;
  bool __property_get_QueueAutoPopup() const { return FQueueAutoPopup; } void __property_set_QueueAutoPopup(bool Value) { FQueueAutoPopup = Value; } __declspec(property(get = __property_get_QueueAutoPopup, put = __property_set_QueueAutoPopup)) bool QueueAutoPopup;
  bool __property_get_SessionRememberPassword() const { return FSessionRememberPassword; } void __property_set_SessionRememberPassword(bool Value) { FSessionRememberPassword = Value; } __declspec(property(get = __property_get_SessionRememberPassword, put = __property_set_SessionRememberPassword)) bool SessionRememberPassword;
  LCID __property_get_Locale() const { return ::rtl::Mutable(this)->GetLocale(); } void __property_set_Locale(LCID Value) { SetLocale(Value); } __declspec(property(get = __property_get_Locale, put = __property_set_Locale)) LCID Locale;
  LCID __property_get_LocaleSafe() const { return ::rtl::Mutable(this)->GetLocale(); } void __property_set_LocaleSafe(LCID Value) { SetLocaleSafe(Value); } __declspec(property(get = __property_get_LocaleSafe, put = __property_set_LocaleSafe)) LCID LocaleSafe;
  UnicodeString __property_get_AppliedLocaleHex() const { return ::rtl::Mutable(this)->GetAppliedLocaleHex(); } __declspec(property(get = __property_get_AppliedLocaleHex)) UnicodeString AppliedLocaleHex;
  TObjectList * __property_get_Locales() const { return ::rtl::Mutable(this)->GetLocales(); } __declspec(property(get = __property_get_Locales)) TObjectList * Locales;
  UnicodeString __property_get_PuttyPath() const { return FPuttyPath; } void __property_set_PuttyPath(UnicodeString Value) { FPuttyPath = Value; } __declspec(property(get = __property_get_PuttyPath, put = __property_set_PuttyPath)) UnicodeString PuttyPath;
  TAutoSwitch __property_get_UsePuttyPwFile() const { return FUsePuttyPwFile; } void __property_set_UsePuttyPwFile(TAutoSwitch Value) { FUsePuttyPwFile = Value; } __declspec(property(get = __property_get_UsePuttyPwFile, put = __property_set_UsePuttyPwFile)) TAutoSwitch UsePuttyPwFile;
  UnicodeString __property_get_DefaultPuttyPath() const { return FDefaultPuttyPath; } __declspec(property(get = __property_get_DefaultPuttyPath)) UnicodeString DefaultPuttyPath;
  bool __property_get_PuttyPassword() const { return FPuttyPassword; } void __property_set_PuttyPassword(bool Value) { FPuttyPassword = Value; } __declspec(property(get = __property_get_PuttyPassword, put = __property_set_PuttyPassword)) bool PuttyPassword;
  bool __property_get_TelnetForFtpInPutty() const { return FTelnetForFtpInPutty; } void __property_set_TelnetForFtpInPutty(bool Value) { FTelnetForFtpInPutty = Value; } __declspec(property(get = __property_get_TelnetForFtpInPutty, put = __property_set_TelnetForFtpInPutty)) bool TelnetForFtpInPutty;
  UnicodeString __property_get_PuttySession() const { return FPuttySession; } void __property_set_PuttySession(UnicodeString Value) { FPuttySession = Value; } __declspec(property(get = __property_get_PuttySession, put = __property_set_PuttySession)) UnicodeString PuttySession;
  TDateTime __property_get_IgnoreCancelBeforeFinish() const { return FIgnoreCancelBeforeFinish; } void __property_set_IgnoreCancelBeforeFinish(TDateTime Value) { FIgnoreCancelBeforeFinish = Value; } __declspec(property(get = __property_get_IgnoreCancelBeforeFinish, put = __property_set_IgnoreCancelBeforeFinish)) TDateTime IgnoreCancelBeforeFinish;
  TGUICopyParamType __property_get_DefaultCopyParam() const { return FDefaultCopyParam; } void __property_set_DefaultCopyParam(TGUICopyParamType Value) { SetDefaultCopyParam(Value); } __declspec(property(get = __property_get_DefaultCopyParam, put = __property_set_DefaultCopyParam)) TGUICopyParamType DefaultCopyParam;
  bool __property_get_BeepOnFinish() const { return FBeepOnFinish; } void __property_set_BeepOnFinish(bool Value) { FBeepOnFinish = Value; } __declspec(property(get = __property_get_BeepOnFinish, put = __property_set_BeepOnFinish)) bool BeepOnFinish;
  TDateTime __property_get_BeepOnFinishAfter() const { return FBeepOnFinishAfter; } void __property_set_BeepOnFinishAfter(TDateTime Value) { FBeepOnFinishAfter = Value; } __declspec(property(get = __property_get_BeepOnFinishAfter, put = __property_set_BeepOnFinishAfter)) TDateTime BeepOnFinishAfter;
  UnicodeString __property_get_BeepSound() const { return FBeepSound; } void __property_set_BeepSound(UnicodeString Value) { FBeepSound = Value; } __declspec(property(get = __property_get_BeepSound, put = __property_set_BeepSound)) UnicodeString BeepSound;
  const TCopyParamList * __property_get_CopyParamList() const { return ::rtl::Mutable(this)->GetCopyParamList(); } void __property_set_CopyParamList(const TCopyParamList * Value) { SetCopyParamList(Value); } __declspec(property(get = __property_get_CopyParamList, put = __property_set_CopyParamList)) const TCopyParamList * CopyParamList;
  UnicodeString __property_get_CopyParamCurrent() const { return FCopyParamCurrent; } void __property_set_CopyParamCurrent(UnicodeString Value) { SetCopyParamCurrent(Value); } __declspec(property(get = __property_get_CopyParamCurrent, put = __property_set_CopyParamCurrent)) UnicodeString CopyParamCurrent;
  int __property_get_CopyParamIndex() const { return ::rtl::Mutable(this)->GetCopyParamIndex(); } void __property_set_CopyParamIndex(int Value) { SetCopyParamIndex(Value); } __declspec(property(get = __property_get_CopyParamIndex, put = __property_set_CopyParamIndex)) int CopyParamIndex;
  TGUICopyParamType __property_get_CurrentCopyParam() const { return ::rtl::Mutable(this)->GetCurrentCopyParam(); } __declspec(property(get = __property_get_CurrentCopyParam)) TGUICopyParamType CurrentCopyParam;
  TGUICopyParamType __property_get_CopyParamPreset(UnicodeString Name) const { return ::rtl::Mutable(this)->GetCopyParamPreset(Name); } __declspec(property(get = __property_get_CopyParamPreset)) TGUICopyParamType CopyParamPreset[];
  bool __property_get_HasCopyParamPreset(UnicodeString Name) const { return ::rtl::Mutable(this)->GetHasCopyParamPreset(Name); } __declspec(property(get = __property_get_HasCopyParamPreset)) bool HasCopyParamPreset[];
  TRemoteProperties __property_get_NewDirectoryProperties() const { return FNewDirectoryProperties; } void __property_set_NewDirectoryProperties(TRemoteProperties Value) { SetNewDirectoryProperties(Value); } __declspec(property(get = __property_get_NewDirectoryProperties, put = __property_set_NewDirectoryProperties)) TRemoteProperties NewDirectoryProperties;
  int __property_get_KeepUpToDateChangeDelay() const { return FKeepUpToDateChangeDelay; } void __property_set_KeepUpToDateChangeDelay(int Value) { FKeepUpToDateChangeDelay = Value; } __declspec(property(get = __property_get_KeepUpToDateChangeDelay, put = __property_set_KeepUpToDateChangeDelay)) int KeepUpToDateChangeDelay;
  UnicodeString __property_get_ChecksumAlg() const { return FChecksumAlg; } void __property_set_ChecksumAlg(UnicodeString Value) { FChecksumAlg = Value; } __declspec(property(get = __property_get_ChecksumAlg, put = __property_set_ChecksumAlg)) UnicodeString ChecksumAlg;
  int __property_get_SessionReopenAutoIdle() const { return FSessionReopenAutoIdle; } void __property_set_SessionReopenAutoIdle(int Value) { FSessionReopenAutoIdle = Value; } __declspec(property(get = __property_get_SessionReopenAutoIdle, put = __property_set_SessionReopenAutoIdle)) int SessionReopenAutoIdle;
  bool __property_get_SessionReopenAutoIdleOn() const { return FSessionReopenAutoIdleOn; } void __property_set_SessionReopenAutoIdleOn(bool Value) { FSessionReopenAutoIdleOn = Value; } __declspec(property(get = __property_get_SessionReopenAutoIdleOn, put = __property_set_SessionReopenAutoIdleOn)) bool SessionReopenAutoIdleOn;
  bool __property_get_SessionReopenAutoInactive() const { return FSessionReopenAutoInactive; } void __property_set_SessionReopenAutoInactive(bool Value) { FSessionReopenAutoInactive = Value; } __declspec(property(get = __property_get_SessionReopenAutoInactive, put = __property_set_SessionReopenAutoInactive)) bool SessionReopenAutoInactive;
  bool __property_get_SessionSilentDisconnect() const { return FSessionSilentDisconnect; } void __property_set_SessionSilentDisconnect(bool Value) { FSessionSilentDisconnect = Value; } __declspec(property(get = __property_get_SessionSilentDisconnect, put = __property_set_SessionSilentDisconnect)) bool SessionSilentDisconnect;
  bool __property_get_CanApplyLocaleImmediately() const { return ::rtl::Mutable(this)->GetCanApplyLocaleImmediately(); } __declspec(property(get = __property_get_CanApplyLocaleImmediately)) bool CanApplyLocaleImmediately;
  LCID __property_get_AppliedLocale() const { return FAppliedLocale; } __declspec(property(get = __property_get_AppliedLocale)) LCID AppliedLocale;
};
//---------------------------------------------------------------------------
extern TGUIConfiguration * GUIConfiguration;
//---------------------------------------------------------------------------
#endif
