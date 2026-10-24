//---------------------------------------------------------------------------
#ifndef RemoteFilesH
#define RemoteFilesH
//---------------------------------------------------------------------------
#include <vector>
#include <map>
//---------------------------------------------------------------------------
enum TModificationFmt { mfNone, mfMDHM, mfYMDHM, mfMDY, mfFull };
//---------------------------------------------------------------------------
#define SYMLINKSTR L" -> "
#define ROOTDIRECTORY L"/"
#define FILETYPE_DEFAULT L'-'
#define FILETYPE_SYMLINK L'L'
#define FILETYPE_DIRECTORY L'D'
extern const UnicodeString PartialExt;
//---------------------------------------------------------------------------
class TTerminal;
class TRights;
class TRemoteFileList;
class THierarchicalStorage;
//---------------------------------------------------------------------------
class TRemoteToken
{
public:
  __fastcall TRemoteToken();
  TRemoteToken(const TRemoteToken &) = default;
  explicit __fastcall TRemoteToken(const UnicodeString & Name);

  void __fastcall Clear();

  bool __fastcall operator ==(const TRemoteToken & rht) const;
  bool __fastcall operator !=(const TRemoteToken & rht) const;
  TRemoteToken & __fastcall operator =(const TRemoteToken & rht);

  int __fastcall Compare(const TRemoteToken & rht) const;

  UnicodeString __property_get_Name() const { return FName; } void __property_set_Name(UnicodeString Value) { FName = Value; } __declspec(property(get = __property_get_Name, put = __property_set_Name)) UnicodeString Name;
  bool __property_get_NameValid() const { return ::rtl::Mutable(this)->GetNameValid(); } __declspec(property(get = __property_get_NameValid)) bool NameValid;
  unsigned int __property_get_ID() const { return FID; } void __property_set_ID(unsigned int Value) { SetID(Value); } __declspec(property(get = __property_get_ID, put = __property_set_ID)) unsigned int ID;
  bool __property_get_IDValid() const { return FIDValid; } __declspec(property(get = __property_get_IDValid)) bool IDValid;
  bool __property_get_IsSet() const { return ::rtl::Mutable(this)->GetIsSet(); } __declspec(property(get = __property_get_IsSet)) bool IsSet;
  UnicodeString __property_get_LogText() const { return ::rtl::Mutable(this)->GetLogText(); } __declspec(property(get = __property_get_LogText)) UnicodeString LogText;
  UnicodeString __property_get_DisplayText() const { return ::rtl::Mutable(this)->GetDisplayText(); } __declspec(property(get = __property_get_DisplayText)) UnicodeString DisplayText;

private:
  UnicodeString FName;
  unsigned int FID;
  bool FIDValid;

  void __fastcall SetID(unsigned int value);
  bool __fastcall GetNameValid() const;
  bool __fastcall GetIsSet() const;
  UnicodeString __fastcall GetDisplayText() const;
  UnicodeString __fastcall GetLogText() const;
};
//---------------------------------------------------------------------------
class TRemoteTokenList
{
public:
  TRemoteTokenList * __fastcall Duplicate() const;
  void __fastcall Clear();
  void __fastcall Add(const TRemoteToken & Token);
  void __fastcall AddUnique(const TRemoteToken & Token);
  bool __fastcall Exists(const UnicodeString & Name) const;
  const TRemoteToken * Find(unsigned int ID) const;
  const TRemoteToken * Find(const UnicodeString & Name) const;
  void __fastcall Log(TTerminal * Terminal, const wchar_t * Title);

  int __fastcall Count() const;
  const TRemoteToken * __fastcall Token(int Index) const;

private:
  typedef std::vector<TRemoteToken> TTokens;
  typedef std::map<UnicodeString, size_t> TNameMap;
  typedef std::map<unsigned int, size_t> TIDMap;
  TTokens FTokens;
  TNameMap FNameMap;
  TIDMap FIDMap;
};
//---------------------------------------------------------------------------
class TRemoteFile : public TPersistent
{
private:
  TRemoteFileList * FDirectory;
  TRemoteToken FOwner;
  TModificationFmt FModificationFmt;
  __int64 FSize;
  __int64 FCalculatedSize;
  UnicodeString FFileName;
  UnicodeString FDisplayName;
  Integer FINodeBlocks;
  TDateTime FModification;
  TDateTime FLastAccess;
  TRemoteToken FGroup;
  Integer FIconIndex;
  Boolean FIsSymLink;
  TRemoteFile * FLinkedFile;
  TRemoteFile * FLinkedByFile;
  UnicodeString FLinkTo;
  TRights *FRights;
  UnicodeString FHumanRights;
  TTerminal *FTerminal;
  wchar_t FType;
  UnicodeString FTags;
  bool FCyclicLink;
  UnicodeString FFullFileName;
  int FIsHidden;
  UnicodeString FTypeName;
  bool FIsEncrypted;
  int __fastcall GetAttr();
  bool __fastcall GetBrokenLink();
  bool __fastcall GetIsDirectory() const;
  const TRemoteFile * __fastcall GetLinkedFile() const;
  UnicodeString __fastcall GetModificationStr();
  void __fastcall SetModification(const TDateTime & value);
  void __fastcall SetListingStr(UnicodeString value);
  UnicodeString __fastcall GetListingStr();
  UnicodeString __fastcall GetRightsStr();
  wchar_t __fastcall GetType() const;
  void __fastcall SetType(wchar_t AType);
  void __fastcall SetTerminal(TTerminal * value);
  void __fastcall SetRights(TRights * value);
  UnicodeString __fastcall GetFullFileName() const;
  bool __fastcall GetHaveFullFileName() const;
  int __fastcall GetIconIndex() const;
  UnicodeString __fastcall GetTypeName();
  bool __fastcall GetIsHidden() const;
  void __fastcall SetIsHidden(bool value);
  bool __fastcall GetIsParentDirectory() const;
  bool __fastcall GetIsThisDirectory() const;
  bool __fastcall GetIsInaccessibleDirectory() const;
  UnicodeString __fastcall GetExtension();
  UnicodeString __fastcall GetUserModificationStr();
  void __fastcall LoadTypeInfo();
  __int64 __fastcall GetSize() const;

protected:
  void __fastcall FindLinkedFile();

public:
  __fastcall TRemoteFile(TRemoteFile * ALinkedByFile = NULL);
  virtual __fastcall ~TRemoteFile();
  TRemoteFile * __fastcall Duplicate(bool Standalone = true) const;

  void __fastcall ShiftTimeInSeconds(__int64 Seconds);
  bool __fastcall IsTimeShiftingApplicable();
  void __fastcall Complete();
  void __fastcall SetEncrypted();
  const TRemoteFile * __fastcall Resolve() const;

  static bool __fastcall IsTimeShiftingApplicable(TModificationFmt ModificationFmt);
  static void __fastcall ShiftTimeInSeconds(TDateTime & DateTime, TModificationFmt ModificationFmt, __int64 Seconds);

  int __property_get_Attr() const { return ::rtl::Mutable(this)->GetAttr(); } __declspec(property(get = __property_get_Attr)) int Attr;
  bool __property_get_BrokenLink() const { return ::rtl::Mutable(this)->GetBrokenLink(); } __declspec(property(get = __property_get_BrokenLink)) bool BrokenLink;
  TRemoteFileList * __property_get_Directory() const { return FDirectory; } void __property_set_Directory(TRemoteFileList * Value) { FDirectory = Value; } __declspec(property(get = __property_get_Directory, put = __property_set_Directory)) TRemoteFileList * Directory;
  UnicodeString __property_get_RightsStr() const { return ::rtl::Mutable(this)->GetRightsStr(); } __declspec(property(get = __property_get_RightsStr)) UnicodeString RightsStr;
  __int64 __property_get_Size() const { return ::rtl::Mutable(this)->GetSize(); } void __property_set_Size(__int64 Value) { FSize = Value; } __declspec(property(get = __property_get_Size, put = __property_set_Size)) __int64 Size;
  __int64 __property_get_CalculatedSize() const { return FCalculatedSize; } void __property_set_CalculatedSize(__int64 Value) { FCalculatedSize = Value; } __declspec(property(get = __property_get_CalculatedSize, put = __property_set_CalculatedSize)) __int64 CalculatedSize;
  TRemoteToken __property_get_Owner() const { return FOwner; } void __property_set_Owner(TRemoteToken Value) { FOwner = Value; } __declspec(property(get = __property_get_Owner, put = __property_set_Owner)) TRemoteToken Owner;
  TRemoteToken __property_get_Group() const { return FGroup; } void __property_set_Group(TRemoteToken Value) { FGroup = Value; } __declspec(property(get = __property_get_Group, put = __property_set_Group)) TRemoteToken Group;
  UnicodeString __property_get_FileName() const { return FFileName; } void __property_set_FileName(UnicodeString Value) { FFileName = Value; } __declspec(property(get = __property_get_FileName, put = __property_set_FileName)) UnicodeString FileName;
  UnicodeString __property_get_DisplayName() const { return FDisplayName; } void __property_set_DisplayName(UnicodeString Value) { FDisplayName = Value; } __declspec(property(get = __property_get_DisplayName, put = __property_set_DisplayName)) UnicodeString DisplayName;
  int __property_get_INodeBlocks() const { return FINodeBlocks; } __declspec(property(get = __property_get_INodeBlocks)) int INodeBlocks;
  TDateTime __property_get_Modification() const { return FModification; } void __property_set_Modification(TDateTime Value) { SetModification(Value); } __declspec(property(get = __property_get_Modification, put = __property_set_Modification)) TDateTime Modification;
  UnicodeString __property_get_ModificationStr() const { return ::rtl::Mutable(this)->GetModificationStr(); } __declspec(property(get = __property_get_ModificationStr)) UnicodeString ModificationStr;
  UnicodeString __property_get_UserModificationStr() const { return ::rtl::Mutable(this)->GetUserModificationStr(); } __declspec(property(get = __property_get_UserModificationStr)) UnicodeString UserModificationStr;
  TModificationFmt __property_get_ModificationFmt() const { return FModificationFmt; } void __property_set_ModificationFmt(TModificationFmt Value) { FModificationFmt = Value; } __declspec(property(get = __property_get_ModificationFmt, put = __property_set_ModificationFmt)) TModificationFmt ModificationFmt;
  TDateTime __property_get_LastAccess() const { return FLastAccess; } void __property_set_LastAccess(TDateTime Value) { FLastAccess = Value; } __declspec(property(get = __property_get_LastAccess, put = __property_set_LastAccess)) TDateTime LastAccess;
  bool __property_get_IsSymLink() const { return FIsSymLink; } __declspec(property(get = __property_get_IsSymLink)) bool IsSymLink;
  bool __property_get_IsDirectory() const { return ::rtl::Mutable(this)->GetIsDirectory(); } __declspec(property(get = __property_get_IsDirectory)) bool IsDirectory;
  const TRemoteFile * __property_get_LinkedFile() const { return ::rtl::Mutable(this)->GetLinkedFile(); } __declspec(property(get = __property_get_LinkedFile)) const TRemoteFile * LinkedFile;
  UnicodeString __property_get_LinkTo() const { return FLinkTo; } void __property_set_LinkTo(UnicodeString Value) { FLinkTo = Value; } __declspec(property(get = __property_get_LinkTo, put = __property_set_LinkTo)) UnicodeString LinkTo;
  UnicodeString __property_get_ListingStr() const { return ::rtl::Mutable(this)->GetListingStr(); } void __property_set_ListingStr(UnicodeString Value) { SetListingStr(Value); } __declspec(property(get = __property_get_ListingStr, put = __property_set_ListingStr)) UnicodeString ListingStr;
  TRights * __property_get_Rights() const { return FRights; } void __property_set_Rights(TRights * Value) { SetRights(Value); } __declspec(property(get = __property_get_Rights, put = __property_set_Rights)) TRights * Rights;
  UnicodeString __property_get_HumanRights() const { return FHumanRights; } void __property_set_HumanRights(UnicodeString Value) { FHumanRights = Value; } __declspec(property(get = __property_get_HumanRights, put = __property_set_HumanRights)) UnicodeString HumanRights;
  TTerminal * __property_get_Terminal() const { return FTerminal; } void __property_set_Terminal(TTerminal * Value) { SetTerminal(Value); } __declspec(property(get = __property_get_Terminal, put = __property_set_Terminal)) TTerminal * Terminal;
  wchar_t __property_get_Type() const { return ::rtl::Mutable(this)->GetType(); } void __property_set_Type(wchar_t Value) { SetType(Value); } __declspec(property(get = __property_get_Type, put = __property_set_Type)) wchar_t Type;
  UnicodeString __property_get_FullFileName() const { return ::rtl::Mutable(this)->GetFullFileName(); } void __property_set_FullFileName(UnicodeString Value) { FFullFileName = Value; } __declspec(property(get = __property_get_FullFileName, put = __property_set_FullFileName)) UnicodeString FullFileName;
  bool __property_get_HaveFullFileName() const { return ::rtl::Mutable(this)->GetHaveFullFileName(); } __declspec(property(get = __property_get_HaveFullFileName)) bool HaveFullFileName;
  int __property_get_IconIndex() const { return ::rtl::Mutable(this)->GetIconIndex(); } __declspec(property(get = __property_get_IconIndex)) int IconIndex;
  UnicodeString __property_get_TypeName() const { return ::rtl::Mutable(this)->GetTypeName(); } __declspec(property(get = __property_get_TypeName)) UnicodeString TypeName;
  UnicodeString __property_get_Tags() const { return FTags; } void __property_set_Tags(UnicodeString Value) { FTags = Value; } __declspec(property(get = __property_get_Tags, put = __property_set_Tags)) UnicodeString Tags;
  bool __property_get_IsHidden() const { return ::rtl::Mutable(this)->GetIsHidden(); } void __property_set_IsHidden(bool Value) { SetIsHidden(Value); } __declspec(property(get = __property_get_IsHidden, put = __property_set_IsHidden)) bool IsHidden;
  bool __property_get_IsParentDirectory() const { return ::rtl::Mutable(this)->GetIsParentDirectory(); } __declspec(property(get = __property_get_IsParentDirectory)) bool IsParentDirectory;
  bool __property_get_IsThisDirectory() const { return ::rtl::Mutable(this)->GetIsThisDirectory(); } __declspec(property(get = __property_get_IsThisDirectory)) bool IsThisDirectory;
  bool __property_get_IsInaccesibleDirectory() const { return ::rtl::Mutable(this)->GetIsInaccessibleDirectory(); } __declspec(property(get = __property_get_IsInaccesibleDirectory)) bool IsInaccesibleDirectory;
  UnicodeString __property_get_Extension() const { return ::rtl::Mutable(this)->GetExtension(); } __declspec(property(get = __property_get_Extension)) UnicodeString Extension;
  bool __property_get_IsEncrypted() const { return FIsEncrypted; } __declspec(property(get = __property_get_IsEncrypted)) bool IsEncrypted;
};
//---------------------------------------------------------------------------
class TRemoteDirectoryFile : public TRemoteFile
{
public:
  __fastcall TRemoteDirectoryFile();
};
//---------------------------------------------------------------------------
class TRemoteParentDirectory : public TRemoteDirectoryFile
{
public:
  __fastcall TRemoteParentDirectory(TTerminal * Terminal);
};
//---------------------------------------------------------------------------
class TRemoteFileList : public TObjectList
{
friend class TSCPFileSystem;
friend class TSFTPFileSystem;
friend class TFTPFileSystem;
friend class TWebDAVFileSystem;
friend class TS3FileSystem;
protected:
  UnicodeString FDirectory;
  TDateTime FTimestamp;
  TRemoteFile * __fastcall GetFiles(Integer Index);
  void SetDirectory(const UnicodeString & value);
  UnicodeString __fastcall GetFullDirectory();
  Boolean __fastcall GetIsRoot();
  TRemoteFile * __fastcall GetParentDirectory();
  UnicodeString __fastcall GetParentPath();
  __int64 __fastcall GetTotalSize();
public:
  __fastcall TRemoteFileList();
  virtual void __fastcall Reset();
  TRemoteFile * __fastcall FindFile(const UnicodeString &FileName);
  virtual void __fastcall DuplicateTo(TRemoteFileList * Copy);
  virtual bool AddFile(TRemoteFile * File);
  virtual void ExtractFile(TRemoteFile * File);

  static TStrings * __fastcall CloneStrings(TStrings * List);
  static bool AnyDirectory(TStrings * List);

  UnicodeString __property_get_Directory() const { return FDirectory; } void __property_set_Directory(UnicodeString Value) { SetDirectory(Value); } __declspec(property(get = __property_get_Directory, put = __property_set_Directory)) UnicodeString Directory;
  TRemoteFile * __property_get_Files(Integer Index) const { return ::rtl::Mutable(this)->GetFiles(Index); } __declspec(property(get = __property_get_Files)) TRemoteFile * Files[];
  UnicodeString __property_get_FullDirectory() const { return ::rtl::Mutable(this)->GetFullDirectory(); } __declspec(property(get = __property_get_FullDirectory)) UnicodeString FullDirectory;
  Boolean __property_get_IsRoot() const { return ::rtl::Mutable(this)->GetIsRoot(); } __declspec(property(get = __property_get_IsRoot)) Boolean IsRoot;
  UnicodeString __property_get_ParentPath() const { return ::rtl::Mutable(this)->GetParentPath(); } __declspec(property(get = __property_get_ParentPath)) UnicodeString ParentPath;
  __int64 __property_get_TotalSize() const { return ::rtl::Mutable(this)->GetTotalSize(); } __declspec(property(get = __property_get_TotalSize)) __int64 TotalSize;
  TDateTime __property_get_Timestamp() const { return FTimestamp; } __declspec(property(get = __property_get_Timestamp)) TDateTime Timestamp;
};
//---------------------------------------------------------------------------
class TRemoteDirectory : public TRemoteFileList
{
private:
  Boolean FIncludeParentDirectory;
  TTerminal * FTerminal;
  TRemoteFile * FParentDirectory;
  Boolean __fastcall GetLoaded();
  void __fastcall SetIncludeParentDirectory(Boolean value);
  void __fastcall ReleaseRelativeDirectories();
public:
  __fastcall TRemoteDirectory(TTerminal * aTerminal, TRemoteDirectory * Template = NULL);
  virtual __fastcall ~TRemoteDirectory();
  virtual bool AddFile(TRemoteFile * File);
  virtual void __fastcall DuplicateTo(TRemoteFileList * Copy);
  virtual void __fastcall Reset();
  TTerminal * __property_get_Terminal() const { return FTerminal; } __declspec(property(get = __property_get_Terminal)) TTerminal * Terminal;
  Boolean __property_get_IncludeParentDirectory() const { return FIncludeParentDirectory; } void __property_set_IncludeParentDirectory(Boolean Value) { SetIncludeParentDirectory(Value); } __declspec(property(get = __property_get_IncludeParentDirectory, put = __property_set_IncludeParentDirectory)) Boolean IncludeParentDirectory;
  Boolean __property_get_Loaded() const { return ::rtl::Mutable(this)->GetLoaded(); } __declspec(property(get = __property_get_Loaded)) Boolean Loaded;
};
//---------------------------------------------------------------------------
class TRemoteDirectoryCache : public TStringList
{
public:
  __fastcall TRemoteDirectoryCache();
  virtual __fastcall ~TRemoteDirectoryCache();
  bool __fastcall HasFileList(const UnicodeString Directory);
  bool __fastcall HasNewerFileList(const UnicodeString Directory, TDateTime Timestamp);
  bool __fastcall GetFileList(const UnicodeString Directory,
    TRemoteFileList * FileList);
  void __fastcall AddFileList(TRemoteFileList * FileList);
  void __fastcall ClearFileList(UnicodeString Directory, bool SubDirs);
  void __fastcall Clear();

  bool __property_get_IsEmpty() const { return ::rtl::Mutable(this)->GetIsEmpty(); } __declspec(property(get = __property_get_IsEmpty)) bool IsEmpty;
protected:
  virtual void __fastcall Delete(int Index);
private:
  TCriticalSection * FSection;
  bool __fastcall GetIsEmpty() const;
  void __fastcall DoClearFileList(UnicodeString Directory, bool SubDirs);
};
//---------------------------------------------------------------------------
class TRemoteDirectoryChangesCache : public TStringList
{
public:
  __fastcall TRemoteDirectoryChangesCache(int MaxSize);

  void __fastcall AddDirectoryChange(const UnicodeString SourceDir,
    const UnicodeString Change, const UnicodeString TargetDir);
  void __fastcall ClearDirectoryChange(UnicodeString SourceDir);
  void __fastcall ClearDirectoryChangeTarget(UnicodeString TargetDir);
  bool __fastcall GetDirectoryChange(const UnicodeString SourceDir,
    const UnicodeString Change, UnicodeString & TargetDir);
  void __fastcall Clear();

  void __fastcall Serialize(UnicodeString & Data);
  void __fastcall Deserialize(const UnicodeString Data);

  bool __property_get_IsEmpty() const { return ::rtl::Mutable(this)->GetIsEmpty(); } __declspec(property(get = __property_get_IsEmpty)) bool IsEmpty;

private:
  static bool __fastcall DirectoryChangeKey(const UnicodeString SourceDir,
    const UnicodeString Change, UnicodeString & Key);
  bool __fastcall GetIsEmpty() const;
  void __fastcall SetValue(const UnicodeString & Name, const UnicodeString & Value);
  UnicodeString __fastcall GetValue(const UnicodeString & Name);

  int FMaxSize;
};
//---------------------------------------------------------------------------
class TRights
{
public:
  static const int TextLen = 9;
  static const wchar_t UndefSymbol = L'$';
  static const wchar_t UnsetSymbol = L'-';
  // Used by Win32-OpenSSH for permissions that are not applicable on Windows.
  // See strmode() in contrib\win32\win32compat\misc.c
  static const wchar_t UnsetSymbolWin = L'*';
  static const wchar_t BasicSymbols[];
  static const wchar_t CombinedSymbols[];
  static const wchar_t ExtendedSymbols[];
  static const wchar_t ModeGroups[];
  enum TRightLevel {
    rlNone = -1,
    rlRead, rlWrite, rlExec, rlSpecial,
    rlFirst = rlRead, rlLastNormal = rlExec, rlLastWithSpecial = rlSpecial,
    rlS3Read = rlRead, rlS3Write = rlWrite, rlS3ReadACP = rlExec, rlS3WriteACP = rlSpecial, rlLastAcl = rlLastWithSpecial,
  };
  enum TRightGroup {
    rgUser, rgGroup, rgOther,
    rgFirst = rgUser, rgLast = rgOther,
    rgS3AllAwsUsers = rgGroup, rgS3AllUsers = rgOther,
  };
  enum TRight {
    rrUserIDExec, rrGroupIDExec, rrStickyBit,
    rrUserRead, rrUserWrite, rrUserExec,
    rrGroupRead, rrGroupWrite, rrGroupExec,
    rrOtherRead, rrOtherWrite, rrOtherExec,
    rrFirst = rrUserIDExec, rrLast = rrOtherExec };
  enum TFlag {
    rfSetUID =    04000, rfSetGID =      02000, rfStickyBit = 01000,
    rfUserRead =  00400, rfUserWrite =   00200, rfUserExec =  00100,
    rfGroupRead = 00040, rfGroupWrite =  00020, rfGroupExec = 00010,
    rfOtherRead = 00004, rfOtherWrite =  00002, rfOtherExec = 00001,
    rfRead =      00444, rfWrite =       00222, rfExec =      00111,
    rfNo =        00000, rfDefault =     00644, rfAll =       00777,
    rfSpecials =  07000, rfAllSpecials = 07777,
    rfS3Read = rfOtherRead, rfS3Write = rfOtherWrite, rfS3ReadACP = rfOtherExec, rfS3WriteACP = rfStickyBit,
     };
  enum TUnsupportedFlag {
    rfDirectory  = 040000 };
  enum TState { rsNo, rsYes, rsUndef };

public:
  static TFlag __fastcall RightToFlag(TRight Right);
  static TRight CalculateRight(TRightGroup Group, TRightLevel Level);
  static TFlag CalculateFlag(TRightGroup Group, TRightLevel Level);
  static unsigned short CalculatePermissions(TRightGroup Group, TRightLevel Level, TRightLevel Level2 = rlNone, TRightLevel Level3 = rlNone);

  __fastcall TRights();
  __fastcall TRights(const TRights & Source);
  __fastcall TRights(unsigned short Number);
  void __fastcall Assign(const TRights * Source);
  void __fastcall AddExecute();
  void __fastcall AllUndef();
  TRights Combine(const TRights & Other) const;
  void SetTextOverride(const UnicodeString & value);

  bool __fastcall operator ==(const TRights & rhr) const;
  bool __fastcall operator ==(unsigned short rhr) const;
  bool __fastcall operator !=(const TRights & rhr) const;
  TRights & __fastcall operator =(const TRights & rhr);
  TRights & __fastcall operator =(unsigned short rhr);
  TRights __fastcall operator ~() const;
  TRights __fastcall operator &(unsigned short rhr) const;
  TRights __fastcall operator &(const TRights & rhr) const;
  TRights & __fastcall operator &=(unsigned short rhr);
  TRights & __fastcall operator &=(const TRights & rhr);
  TRights __fastcall operator |(unsigned short rhr) const;
  TRights __fastcall operator |(const TRights & rhr) const;
  TRights & __fastcall operator |=(unsigned short rhr);
  TRights & __fastcall operator |=(const TRights & rhr);
  __fastcall operator unsigned short() const;
  __fastcall operator unsigned long() const;

  UnicodeString __fastcall GetChmodStr(int Directory) const;

  bool __property_get_AllowUndef() const { return FAllowUndef; } void __property_set_AllowUndef(bool Value) { SetAllowUndef(Value); } __declspec(property(get = __property_get_AllowUndef, put = __property_set_AllowUndef)) bool AllowUndef;
  bool __property_get_IsUndef() const { return ::rtl::Mutable(this)->GetIsUndef(); } __declspec(property(get = __property_get_IsUndef)) bool IsUndef;
  UnicodeString __property_get_ModeStr() const { return ::rtl::Mutable(this)->GetModeStr(); } __declspec(property(get = __property_get_ModeStr)) UnicodeString ModeStr;
  UnicodeString __property_get_Octal() const { return ::rtl::Mutable(this)->GetOctal(); } void __property_set_Octal(UnicodeString Value) { SetOctal(Value); } __declspec(property(get = __property_get_Octal, put = __property_set_Octal)) UnicodeString Octal;
  unsigned short __property_get_Number() const { return ::rtl::Mutable(this)->GetNumber(); } void __property_set_Number(unsigned short Value) { SetNumber(Value); } __declspec(property(get = __property_get_Number, put = __property_set_Number)) unsigned short Number;
  unsigned short __property_get_NumberSet() const { return FSet; } __declspec(property(get = __property_get_NumberSet)) unsigned short NumberSet;
  unsigned short __property_get_NumberUnset() const { return FUnset; } __declspec(property(get = __property_get_NumberUnset)) unsigned short NumberUnset;
  unsigned long __property_get_NumberDecadic() const { return ::rtl::Mutable(this)->GetNumberDecadic(); } __declspec(property(get = __property_get_NumberDecadic)) unsigned long NumberDecadic;
  bool __property_get_ReadOnly() const { return ::rtl::Mutable(this)->GetReadOnly(); } void __property_set_ReadOnly(bool Value) { SetReadOnly(Value); } __declspec(property(get = __property_get_ReadOnly, put = __property_set_ReadOnly)) bool ReadOnly;
  bool __property_get_Right(TRight Right) const { return ::rtl::Mutable(this)->GetRight(Right); } void __property_set_Right(TRight Right, bool Value) { SetRight(Right, Value); } __declspec(property(get = __property_get_Right, put = __property_set_Right)) bool Right[];
  TState __property_get_RightUndef(TRight Right) const { return ::rtl::Mutable(this)->GetRightUndef(Right); } void __property_set_RightUndef(TRight Right, TState Value) { SetRightUndef(Right, Value); } __declspec(property(get = __property_get_RightUndef, put = __property_set_RightUndef)) TState RightUndef[];
  UnicodeString __property_get_Text() const { return ::rtl::Mutable(this)->GetText(); } void __property_set_Text(UnicodeString Value) { SetText(Value); } __declspec(property(get = __property_get_Text, put = __property_set_Text)) UnicodeString Text;
  bool __property_get_Unknown() const { return FUnknown; } __declspec(property(get = __property_get_Unknown)) bool Unknown;

private:
  bool FAllowUndef;
  unsigned short FSet;
  unsigned short FUnset;
  UnicodeString FText;
  bool FUnknown;

  bool __fastcall GetIsUndef() const;
  UnicodeString __fastcall GetModeStr() const;
  void __fastcall SetNumber(unsigned short value);
  UnicodeString __fastcall GetText() const;
  void __fastcall SetText(const UnicodeString & value);
  void __fastcall SetOctal(UnicodeString value);
  unsigned short __fastcall GetNumber() const;
  unsigned short __fastcall GetNumberSet() const;
  unsigned short __fastcall GetNumberUnset() const;
  unsigned long __fastcall GetNumberDecadic() const;
  UnicodeString __fastcall GetOctal() const;
  bool __fastcall GetReadOnly();
  bool __fastcall GetRight(TRight Right) const;
  TState __fastcall GetRightUndef(TRight Right) const;
  void __fastcall SetAllowUndef(bool value);
  void __fastcall SetReadOnly(bool value);
  void __fastcall SetRight(TRight Right, bool value);
  void __fastcall SetRightUndef(TRight Right, TState value);
};
//---------------------------------------------------------------------------
enum TValidProperty { vpRights, vpGroup, vpOwner, vpModification, vpLastAccess, vpEncrypt, vpTags };
typedef Set<TValidProperty, vpRights, vpTags> TValidProperties;
class TRemoteProperties
{
public:
  TValidProperties Valid;
  bool Recursive;
  TRights Rights;
  bool AddXToDirectories;
  TRemoteToken Group;
  TRemoteToken Owner;
  __int64 Modification; // unix time
  __int64 LastAccess; // unix time
  bool Encrypt;
  UnicodeString Tags;

  __fastcall TRemoteProperties();
  __fastcall TRemoteProperties(const TRemoteProperties & rhp);
  TRemoteProperties & operator =(const TRemoteProperties &) = default;
  bool __fastcall operator ==(const TRemoteProperties & rhp) const;
  bool __fastcall operator !=(const TRemoteProperties & rhp) const;
  void __fastcall Default();
  void __fastcall Load(THierarchicalStorage * Storage);
  void __fastcall Save(THierarchicalStorage * Storage) const;

  static TRemoteProperties __fastcall CommonProperties(TStrings * FileList);
  static TRemoteProperties __fastcall ChangedProperties(
    const TRemoteProperties & OriginalProperties, TRemoteProperties NewProperties);
};
//---------------------------------------------------------------------------
class TSynchronizeChecklist
{
friend class TTerminal;

public:
  enum TAction {
    saNone, saUploadNew, saDownloadNew, saUploadUpdate, saDownloadUpdate, saDeleteRemote, saDeleteLocal };
  static const int ActionCount = saDeleteLocal;

  class TItem
  {
  friend class TTerminal;
  friend class TSynchronizeChecklist;

  public:
    struct TFileInfo
    {
      UnicodeString FileName;
      UnicodeString Directory;
      TDateTime Modification;
      TModificationFmt ModificationFmt;
      __int64 Size;
    };

    TAction Action;
    bool IsDirectory;
    TFileInfo Info1;
    TFileInfo Info2;
    int ImageIndex;
    bool Checked;
    TRemoteFile * RemoteFile;

    const UnicodeString& GetFileName() const;
    bool IsRemoteOnly() const { return (Action == saDownloadNew) || (Action == saDeleteRemote); }
    bool IsLocalOnly() const { return (Action == saUploadNew) || (Action == saDeleteLocal); }
    bool HasSize() const { return !IsDirectory || FDirectoryHasSize; }
    __int64 __fastcall GetBaseSize() const;
    __int64 __fastcall GetSize() const;
    __int64 __fastcall GetSize(TAction AAction) const;
    UnicodeString GetLocalPath() const;
    UnicodeString GetLocalPath2() const;
    UnicodeString ForceGetLocalPath() const;
    UnicodeString ForceGetLocalPath2() const;
    // Contrary to RemoteFile->FullFileName, this does not include trailing slash for directories
    UnicodeString GetRemotePath() const;
    UnicodeString ForceGetRemotePath() const;
    UnicodeString GetLocalTarget() const;
    UnicodeString GetLocalTarget2() const;
    UnicodeString GetRemoteTarget() const;
    TStrings * GetFileList() const;

    ~TItem();

  private:
    FILETIME FLocalLastWriteTime;
    bool FDirectoryHasSize;

    TItem();
    __int64 __fastcall GetBaseSize(TAction AAction) const;
  };

  typedef std::vector<const TItem *> TItemList;

  ~TSynchronizeChecklist();

  void __fastcall Update(const TItem * Item, bool Check, TAction Action);
  void __fastcall UpdateDirectorySize(const TItem * Item, __int64 Size);
  void Delete(const TItem * Item);

  static TAction __fastcall Reverse(TAction Action);
  static bool __fastcall IsItemSizeIrrelevant(TAction Action);
  bool GetNextChecked(int & Index, const TItem *& Item) const;

  int __property_get_Count() const { return ::rtl::Mutable(this)->GetCount(); } __declspec(property(get = __property_get_Count)) int Count;
  int __property_get_CheckedCount() const { return ::rtl::Mutable(this)->GetCheckedCount(); } __declspec(property(get = __property_get_CheckedCount)) int CheckedCount;
  const TItem * __property_get_Item(int Index) const { return ::rtl::Mutable(this)->GetItem(Index); } __declspec(property(get = __property_get_Item)) const TItem * Item[];

  static int Compare(const TItem * Item1, const TItem * Item2);

protected:
  TSynchronizeChecklist();

  void Sort();
  void Add(TItem * Item);

  int GetCount() const;
  int GetCheckedCount() const;
  const TItem * GetItem(int Index) const;

private:
  TList * FList;

  static int __fastcall Compare(void * Item1, void * Item2);
};
//---------------------------------------------------------------------------
typedef TMethodPtr<void(
  void * Token, const TSynchronizeChecklist::TItem * Item)> TProcessedSynchronizationChecklistItem;
struct IFileOperation;
struct IShellItem;
class TFileOperationProgressSink;
//---------------------------------------------------------------------------
class TSynchronizeChecklistFileOperation
{
friend class TSynchronizeChecklist;
friend class TFileOperationProgressSink;
public:
  TSynchronizeChecklistFileOperation(
    const TSynchronizeChecklist * Checklist, TProcessedSynchronizationChecklistItem OnProcessedItem, void * Token);
  ~TSynchronizeChecklistFileOperation();

  IFileOperation * __property_get_FileOperation() const { return ::rtl::Mutable(this)->GetFileOperation(); } __declspec(property(get = __property_get_FileOperation)) IFileOperation * FileOperation;
  bool __property_get_Any() const { return FAny; } __declspec(property(get = __property_get_Any)) bool Any;

protected:
  void ProcessedItem(IShellItem * ShellItem);
  IFileOperation * GetFileOperation() { return FFileOperation.Get(); }

private:
  TComPtr<IFileOperation> FFileOperation;
  TFileOperationProgressSink * FProgressSink;
  std::map<UnicodeString, const TSynchronizeChecklist::TItem *> FShellItems;
  TProcessedSynchronizationChecklistItem FOnProcessedItem;
  void * FToken;
  bool FAny;
};
//---------------------------------------------------------------------------
class TFileOperationProgressType;
//---------------------------------------------------------------------------
class TSynchronizeProgress
{
public:
  TSynchronizeProgress(const TSynchronizeChecklist * Checklist);

  void ItemProcessed(const TSynchronizeChecklist::TItem * ChecklistItem);
  int Progress(const TFileOperationProgressType * CurrentItemOperationProgress) const;
  TDateTime TimeLeft(const TFileOperationProgressType * CurrentItemOperationProgress) const;

private:
  const TSynchronizeChecklist * FChecklist;
  mutable __int64 FTotalSize;
  __int64 FProcessedSize;

  __int64 ItemSize(const TSynchronizeChecklist::TItem * ChecklistItem) const;
  __int64 GetProcessed(const TFileOperationProgressType * CurrentItemOperationProgress) const;
};
//---------------------------------------------------------------------------
bool __fastcall IsUnixStyleWindowsPath(const UnicodeString & Path);
bool __fastcall UnixIsAbsolutePath(const UnicodeString & Path);
UnicodeString __fastcall UnixIncludeTrailingBackslash(const UnicodeString & Path);
UnicodeString UniversalIncludeTrailingBackslash(bool Unix, const UnicodeString & Path);
UnicodeString __fastcall UnixExcludeTrailingBackslash(const UnicodeString & Path, bool Simple = false);
UnicodeString UniversalExcludeTrailingBackslash(bool Unix, const UnicodeString & Path);
UnicodeString __fastcall SimpleUnixExcludeTrailingBackslash(const UnicodeString & Path);
UnicodeString __fastcall UnixCombinePaths(const UnicodeString & Path1, const UnicodeString & Path2);
UnicodeString UnixCombinePathsForce(const UnicodeString & Path1, const UnicodeString & Path2);
UnicodeString UnixCombinePathsSmart(const UnicodeString & Path1, const UnicodeString & Path2);
UnicodeString UniversalCombinePaths(bool Unix, const UnicodeString & Path1, const UnicodeString & Path2);
UnicodeString __fastcall UnixExtractFileDir(const UnicodeString & Path);
UnicodeString __fastcall UnixExtractFilePath(const UnicodeString & Path);
UnicodeString __fastcall UnixExtractFileName(const UnicodeString & Path);
UnicodeString ExtractShortName(const UnicodeString & Path, bool Unix);
UnicodeString __fastcall UnixExtractFileExt(const UnicodeString & Path);
Boolean __fastcall UnixSamePath(const UnicodeString & Path1, const UnicodeString & Path2);
bool __fastcall UnixIsChildPath(const UnicodeString & Parent, const UnicodeString & Child);
bool __fastcall ExtractCommonPath(TStrings * Files, UnicodeString & Path);
bool __fastcall UnixExtractCommonPath(TStrings * Files, UnicodeString & Path);
UnicodeString __fastcall ExtractFileName(const UnicodeString & Path, bool Unix);
bool __fastcall IsUnixRootPath(const UnicodeString & Path);
bool __fastcall IsUnixHiddenFile(const UnicodeString & Path);
UnicodeString __fastcall AbsolutePath(const UnicodeString & Base, const UnicodeString & Path);
UnicodeString __fastcall FromUnixPath(const UnicodeString & Path);
UnicodeString __fastcall ToUnixPath(const UnicodeString & Path);
UnicodeString __fastcall MinimizeName(const UnicodeString & FileName, int MaxLen, bool Unix);
UnicodeString __fastcall MakeFileList(TStrings * FileList);
TDateTime __fastcall ReduceDateTimePrecision(TDateTime DateTime,
  TModificationFmt Precision);
TModificationFmt __fastcall LessDateTimePrecision(
  TModificationFmt Precision1, TModificationFmt Precision2);
UnicodeString __fastcall UserModificationStr(TDateTime DateTime,
  TModificationFmt Precision);
UnicodeString __fastcall ModificationStr(TDateTime DateTime,
  TModificationFmt Precision);
int GetPartialFileExtLen(const UnicodeString & FileName);
int __fastcall FakeFileImageIndex(UnicodeString FileName, unsigned long Attrs = 0,
  UnicodeString * TypeName = NULL);
bool __fastcall SameUserName(const UnicodeString & UserName1, const UnicodeString & UserName2);
UnicodeString __fastcall FormatMultiFilesToOneConfirmation(const UnicodeString & Target, bool Unix);
//---------------------------------------------------------------------------
#endif
