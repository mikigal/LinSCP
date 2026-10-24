//---------------------------------------------------------------------------
#ifndef CopyParamH
#define CopyParamH

#include "FileMasks.h"
#include "FileBuffer.h"
#include "RemoteFiles.h"
#include "Exceptions.h"
//---------------------------------------------------------------------------
// When adding new options, mind TCopyParamType::GetLogStr()
enum TOperationSide : int { osLocal, osRemote, osCurrent };
enum TFileNameCase : int { ncNoChange, ncUpperCase, ncLowerCase, ncFirstUpperCase, ncLowerCaseShort };
// TScript::OptionProc depend on the order
enum TTransferMode : int { tmBinary, tmAscii, tmAutomatic };
enum TResumeSupport : int { rsOn, rsSmart, rsOff };
class THierarchicalStorage;
const int cpaIncludeMaskOnly = 0x01;
const int cpaNoTransferMode =  0x02;
const int cpaNoIncludeMask =   0x04;
const int cpaNoClearArchive =  0x08;
const int cpaNoPreserveTime =  0x10;
const int cpaNoRights =        0x20;
const int cpaNoPreserveReadOnly = 0x40;
const int cpaNoIgnorePermErrors = 0x80;
const int cpaNoNewerOnly        = 0x100;
const int cpaNoRemoveCtrlZ      = 0x200;
const int cpaNoRemoveBOM        = 0x400;
const int cpaNoPreserveTimeDirs = 0x800;
const int cpaNoResumeSupport    = 0x1000;
const int cpaNoEncryptNewFiles  = 0x2000;
const int cpaNoCalculateSize    = 0x4000;
//---------------------------------------------------------------------------
struct TUsableCopyParamAttrs
{
  int General;
  int Upload;
  int Download;
};
//---------------------------------------------------------------------------
class TCopyParamType
{
private:
  TFileMasks FAsciiFileMask;
  TFileNameCase FFileNameCase;
  bool FPreserveReadOnly;
  bool FPreserveTime;
  bool FPreserveTimeDirs;
  TRights FRights;
  TTransferMode FTransferMode;
  bool FAddXToDirectories;
  bool FPreserveRights;
  bool FIgnorePermErrors;
  TResumeSupport FResumeSupport;
  __int64 FResumeThreshold;
  UnicodeString __fastcall GetLogStr() const;
  wchar_t FInvalidCharsReplacement;
  UnicodeString FLocalInvalidChars;
  UnicodeString FTokenizibleChars;
  bool FCalculateSize;
  UnicodeString FFileMask;
  TFileMasks FIncludeFileMask;
  std::unique_ptr<TStringList> FTransferSkipList;
  UnicodeString FTransferResumeFile;
  bool FClearArchive;
  bool FRemoveCtrlZ;
  bool FRemoveBOM;
  unsigned int FCPSLimit;
  bool FNewerOnly;
  bool FEncryptNewFiles;
  bool FExcludeHiddenFiles;
  bool FExcludeEmptyDirectories;
  __int64 FSize;
  __int64 FPartOffset;
  __int64 FPartSize;
  TOnceDoneOperation FOnceDoneOperation;
  TTransferOutEvent FOnTransferOut;
  TTransferInEvent FOnTransferIn;

  void __fastcall SetLocalInvalidChars(UnicodeString value);
  bool __fastcall GetReplaceInvalidChars() const;
  void __fastcall SetReplaceInvalidChars(bool value);
  UnicodeString __fastcall RestoreChars(UnicodeString FileName) const;
  void __fastcall DoGetInfoStr(UnicodeString Separator, int Attrs,
    UnicodeString & Result, bool & SomeAttrIncluded, const UnicodeString & Link, UnicodeString & ScriptArgs,
    TAssemblyLanguage Language, UnicodeString & AssemblyCode) const;
  TStrings * __fastcall GetTransferSkipList() const;
  void __fastcall SetTransferSkipList(TStrings * value);

public:
  __fastcall TCopyParamType();
  __fastcall TCopyParamType(const TCopyParamType & Source);
  virtual __fastcall ~TCopyParamType();
  TCopyParamType & __fastcall operator =(const TCopyParamType & rhp);
  virtual void __fastcall Assign(const TCopyParamType * Source);
  virtual void __fastcall Default();
  UnicodeString __fastcall ChangeFileName(UnicodeString FileName,
    TOperationSide Side, bool FirstLevel) const;
  int __fastcall LocalFileAttrs(const TRights & Rights) const;
  TRights __fastcall RemoteFileRights(int Attrs) const;
  bool __fastcall UseAsciiTransfer(UnicodeString FileName, TOperationSide Side,
    const TFileMasks::TParams & Params) const;
  bool __fastcall AllowResume(__int64 Size, const UnicodeString & FileName) const;
  bool __fastcall ResumeTransfer(UnicodeString FileName) const;
  UnicodeString __fastcall ValidLocalFileName(UnicodeString FileName) const;
  UnicodeString __fastcall ValidLocalPath(UnicodeString Path) const;
  bool __fastcall AllowAnyTransfer() const;
  bool __fastcall AllowTransfer(UnicodeString FileName, TOperationSide Side,
    bool Directory, const TFileMasks::TParams & Params, bool Hidden) const;
  bool __fastcall SkipTransfer(UnicodeString FileName, bool Directory) const;

  virtual void __fastcall Load(THierarchicalStorage * Storage);
  virtual void __fastcall Save(THierarchicalStorage * Storage, const TCopyParamType * Defaults = NULL) const;
  UnicodeString __fastcall GetInfoStr(UnicodeString Separator, int Attrs) const;
  bool __fastcall AnyUsableCopyParam(int Attrs) const;
  UnicodeString __fastcall GenerateTransferCommandArgs(int Attrs, const UnicodeString & Link) const;
  UnicodeString __fastcall GenerateAssemblyCode(TAssemblyLanguage Language, int Attrs) const;

  bool __fastcall operator==(const TCopyParamType & rhp) const;

  TFileMasks __property_get_AsciiFileMask() const { return FAsciiFileMask; } void __property_set_AsciiFileMask(TFileMasks Value) { FAsciiFileMask = Value; } __declspec(property(get = __property_get_AsciiFileMask, put = __property_set_AsciiFileMask)) TFileMasks AsciiFileMask;
  TFileNameCase __property_get_FileNameCase() const { return FFileNameCase; } void __property_set_FileNameCase(TFileNameCase Value) { FFileNameCase = Value; } __declspec(property(get = __property_get_FileNameCase, put = __property_set_FileNameCase)) TFileNameCase FileNameCase;
  bool __property_get_PreserveReadOnly() const { return FPreserveReadOnly; } void __property_set_PreserveReadOnly(bool Value) { FPreserveReadOnly = Value; } __declspec(property(get = __property_get_PreserveReadOnly, put = __property_set_PreserveReadOnly)) bool PreserveReadOnly;
  bool __property_get_PreserveTime() const { return FPreserveTime; } void __property_set_PreserveTime(bool Value) { FPreserveTime = Value; } __declspec(property(get = __property_get_PreserveTime, put = __property_set_PreserveTime)) bool PreserveTime;
  bool __property_get_PreserveTimeDirs() const { return FPreserveTimeDirs; } void __property_set_PreserveTimeDirs(bool Value) { FPreserveTimeDirs = Value; } __declspec(property(get = __property_get_PreserveTimeDirs, put = __property_set_PreserveTimeDirs)) bool PreserveTimeDirs;
  TRights __property_get_Rights() const { return FRights; } void __property_set_Rights(TRights Value) { FRights = Value; } __declspec(property(get = __property_get_Rights, put = __property_set_Rights)) TRights Rights;
  TTransferMode __property_get_TransferMode() const { return FTransferMode; } void __property_set_TransferMode(TTransferMode Value) { FTransferMode = Value; } __declspec(property(get = __property_get_TransferMode, put = __property_set_TransferMode)) TTransferMode TransferMode;
  UnicodeString __property_get_LogStr() const { return ::rtl::Mutable(this)->GetLogStr(); } __declspec(property(get = __property_get_LogStr)) UnicodeString LogStr;
  bool __property_get_AddXToDirectories() const { return FAddXToDirectories; } void __property_set_AddXToDirectories(bool Value) { FAddXToDirectories = Value; } __declspec(property(get = __property_get_AddXToDirectories, put = __property_set_AddXToDirectories)) bool AddXToDirectories;
  bool __property_get_PreserveRights() const { return FPreserveRights; } void __property_set_PreserveRights(bool Value) { FPreserveRights = Value; } __declspec(property(get = __property_get_PreserveRights, put = __property_set_PreserveRights)) bool PreserveRights;
  bool __property_get_IgnorePermErrors() const { return FIgnorePermErrors; } void __property_set_IgnorePermErrors(bool Value) { FIgnorePermErrors = Value; } __declspec(property(get = __property_get_IgnorePermErrors, put = __property_set_IgnorePermErrors)) bool IgnorePermErrors;
  TResumeSupport __property_get_ResumeSupport() const { return FResumeSupport; } void __property_set_ResumeSupport(TResumeSupport Value) { FResumeSupport = Value; } __declspec(property(get = __property_get_ResumeSupport, put = __property_set_ResumeSupport)) TResumeSupport ResumeSupport;
  __int64 __property_get_ResumeThreshold() const { return FResumeThreshold; } void __property_set_ResumeThreshold(__int64 Value) { FResumeThreshold = Value; } __declspec(property(get = __property_get_ResumeThreshold, put = __property_set_ResumeThreshold)) __int64 ResumeThreshold;
  wchar_t __property_get_InvalidCharsReplacement() const { return FInvalidCharsReplacement; } void __property_set_InvalidCharsReplacement(wchar_t Value) { FInvalidCharsReplacement = Value; } __declspec(property(get = __property_get_InvalidCharsReplacement, put = __property_set_InvalidCharsReplacement)) wchar_t InvalidCharsReplacement;
  bool __property_get_ReplaceInvalidChars() const { return ::rtl::Mutable(this)->GetReplaceInvalidChars(); } void __property_set_ReplaceInvalidChars(bool Value) { SetReplaceInvalidChars(Value); } __declspec(property(get = __property_get_ReplaceInvalidChars, put = __property_set_ReplaceInvalidChars)) bool ReplaceInvalidChars;
  UnicodeString __property_get_LocalInvalidChars() const { return FLocalInvalidChars; } void __property_set_LocalInvalidChars(UnicodeString Value) { SetLocalInvalidChars(Value); } __declspec(property(get = __property_get_LocalInvalidChars, put = __property_set_LocalInvalidChars)) UnicodeString LocalInvalidChars;
  bool __property_get_CalculateSize() const { return FCalculateSize; } void __property_set_CalculateSize(bool Value) { FCalculateSize = Value; } __declspec(property(get = __property_get_CalculateSize, put = __property_set_CalculateSize)) bool CalculateSize;
  UnicodeString __property_get_FileMask() const { return FFileMask; } void __property_set_FileMask(UnicodeString Value) { FFileMask = Value; } __declspec(property(get = __property_get_FileMask, put = __property_set_FileMask)) UnicodeString FileMask;
  TFileMasks __property_get_IncludeFileMask() const { return FIncludeFileMask; } void __property_set_IncludeFileMask(TFileMasks Value) { FIncludeFileMask = Value; } __declspec(property(get = __property_get_IncludeFileMask, put = __property_set_IncludeFileMask)) TFileMasks IncludeFileMask;
  TStrings * __property_get_TransferSkipList() const { return ::rtl::Mutable(this)->GetTransferSkipList(); } void __property_set_TransferSkipList(TStrings * Value) { SetTransferSkipList(Value); } __declspec(property(get = __property_get_TransferSkipList, put = __property_set_TransferSkipList)) TStrings * TransferSkipList;
  UnicodeString __property_get_TransferResumeFile() const { return FTransferResumeFile; } void __property_set_TransferResumeFile(UnicodeString Value) { FTransferResumeFile = Value; } __declspec(property(get = __property_get_TransferResumeFile, put = __property_set_TransferResumeFile)) UnicodeString TransferResumeFile;
  bool __property_get_ClearArchive() const { return FClearArchive; } void __property_set_ClearArchive(bool Value) { FClearArchive = Value; } __declspec(property(get = __property_get_ClearArchive, put = __property_set_ClearArchive)) bool ClearArchive;
  bool __property_get_RemoveCtrlZ() const { return FRemoveCtrlZ; } void __property_set_RemoveCtrlZ(bool Value) { FRemoveCtrlZ = Value; } __declspec(property(get = __property_get_RemoveCtrlZ, put = __property_set_RemoveCtrlZ)) bool RemoveCtrlZ;
  bool __property_get_RemoveBOM() const { return FRemoveBOM; } void __property_set_RemoveBOM(bool Value) { FRemoveBOM = Value; } __declspec(property(get = __property_get_RemoveBOM, put = __property_set_RemoveBOM)) bool RemoveBOM;
  unsigned int __property_get_CPSLimit() const { return FCPSLimit; } void __property_set_CPSLimit(unsigned int Value) { FCPSLimit = Value; } __declspec(property(get = __property_get_CPSLimit, put = __property_set_CPSLimit)) unsigned int CPSLimit;
  bool __property_get_NewerOnly() const { return FNewerOnly; } void __property_set_NewerOnly(bool Value) { FNewerOnly = Value; } __declspec(property(get = __property_get_NewerOnly, put = __property_set_NewerOnly)) bool NewerOnly;
  bool __property_get_EncryptNewFiles() const { return FEncryptNewFiles; } void __property_set_EncryptNewFiles(bool Value) { FEncryptNewFiles = Value; } __declspec(property(get = __property_get_EncryptNewFiles, put = __property_set_EncryptNewFiles)) bool EncryptNewFiles;
  bool __property_get_ExcludeHiddenFiles() const { return FExcludeHiddenFiles; } void __property_set_ExcludeHiddenFiles(bool Value) { FExcludeHiddenFiles = Value; } __declspec(property(get = __property_get_ExcludeHiddenFiles, put = __property_set_ExcludeHiddenFiles)) bool ExcludeHiddenFiles;
  bool __property_get_ExcludeEmptyDirectories() const { return FExcludeEmptyDirectories; } void __property_set_ExcludeEmptyDirectories(bool Value) { FExcludeEmptyDirectories = Value; } __declspec(property(get = __property_get_ExcludeEmptyDirectories, put = __property_set_ExcludeEmptyDirectories)) bool ExcludeEmptyDirectories;
  __int64 __property_get_Size() const { return FSize; } void __property_set_Size(__int64 Value) { FSize = Value; } __declspec(property(get = __property_get_Size, put = __property_set_Size)) __int64 Size;
  __int64 __property_get_PartSize() const { return FPartSize; } void __property_set_PartSize(__int64 Value) { FPartSize = Value; } __declspec(property(get = __property_get_PartSize, put = __property_set_PartSize)) __int64 PartSize;
  __int64 __property_get_PartOffset() const { return FPartOffset; } void __property_set_PartOffset(__int64 Value) { FPartOffset = Value; } __declspec(property(get = __property_get_PartOffset, put = __property_set_PartOffset)) __int64 PartOffset;
  TOnceDoneOperation __property_get_OnceDoneOperation() const { return FOnceDoneOperation; } void __property_set_OnceDoneOperation(TOnceDoneOperation Value) { FOnceDoneOperation = Value; } __declspec(property(get = __property_get_OnceDoneOperation, put = __property_set_OnceDoneOperation)) TOnceDoneOperation OnceDoneOperation;
  TTransferOutEvent __property_get_OnTransferOut() const { return FOnTransferOut; } void __property_set_OnTransferOut(TTransferOutEvent Value) { FOnTransferOut = Value; } __declspec(property(get = __property_get_OnTransferOut, put = __property_set_OnTransferOut)) TTransferOutEvent OnTransferOut;
  TTransferInEvent __property_get_OnTransferIn() const { return FOnTransferIn; } void __property_set_OnTransferIn(TTransferInEvent Value) { FOnTransferIn = Value; } __declspec(property(get = __property_get_OnTransferIn, put = __property_set_OnTransferIn)) TTransferInEvent OnTransferIn;
};
//---------------------------------------------------------------------------
unsigned int __fastcall GetSpeedLimit(const UnicodeString & Text);
UnicodeString __fastcall SetSpeedLimit(unsigned int Limit);
void __fastcall CopySpeedLimits(TStrings * Source, TStrings * Dest);
TOperationSide ReverseOperationSide(TOperationSide Side);
extern const unsigned int MinSpeed;
extern const unsigned int MaxSpeed;
//---------------------------------------------------------------------------
#endif
