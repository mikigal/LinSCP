//---------------------------------------------------------------------------
#ifndef HierarchicalStorageH
#define HierarchicalStorageH

#include <Registry.hpp>
#include <memory>
#include <map>
//---------------------------------------------------------------------------
enum TStorage : int { stDetect, stRegistry, stIniFile, stNul };
enum TStorageAccessMode : int { smRead, smReadWrite };
typedef std::map<UnicodeString, int> TIntMapping;
//---------------------------------------------------------------------------
class THierarchicalStorage
{
friend class TCustomIniFileStorage;

public:
  __fastcall THierarchicalStorage(const UnicodeString & AStorage);
  virtual __fastcall ~THierarchicalStorage();
  void __fastcall ConfigureForPutty();
  bool __fastcall OpenRootKey(bool CanCreate);
  virtual bool __fastcall OpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void __fastcall CloseSubKey();
  void __fastcall CloseAll();
  bool __fastcall OpenSubKeyPath(const UnicodeString & KeyPath, bool CanCreate);
  void __fastcall CloseSubKeyPath();
  void __fastcall GetSubKeyNames(TStrings * Strings);
  void __fastcall GetValueNames(TStrings * Strings);
  bool __fastcall HasSubKeys();
  bool __fastcall KeyExists(const UnicodeString & SubKey);
  bool __fastcall ValueExists(const UnicodeString & Value);
  bool DeleteSubKey(const UnicodeString & Key, bool Recursive = false);
  bool RecursiveDeleteSubKey(const UnicodeString & Key);
  virtual void __fastcall ClearSubKeys();
  virtual void __fastcall ReadValues(TStrings* Strings, bool MaintainKeys = false);
  virtual void __fastcall WriteValues(TStrings* Strings, bool MaintainKeys = false);
  virtual void __fastcall ClearValues();
  bool __fastcall DeleteValue(const UnicodeString & Name);

  bool __fastcall ReadBool(const UnicodeString & Name, bool Default);
  template<typename T>
  T __fastcall ReadEnum(
    const UnicodeString & Name, const T & Default, const TIntMapping & Mapping = TIntMapping());
  int __fastcall ReadInteger(const UnicodeString & Name, int Default);
  __int64 __fastcall ReadInt64(const UnicodeString & Name, __int64 Default);
  TDateTime __fastcall ReadDateTime(const UnicodeString & Name, TDateTime Default);
  double __fastcall ReadFloat(const UnicodeString & Name, double Default);
  UnicodeString __fastcall ReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  size_t __fastcall ReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

  virtual UnicodeString __fastcall ReadString(const UnicodeString & Name, const UnicodeString & Default);
  RawByteString __fastcall ReadBinaryData(const UnicodeString & Name);
  RawByteString __fastcall ReadStringAsBinaryData(const UnicodeString & Name, const RawByteString & Default);

  void __fastcall WriteBool(const UnicodeString & Name, bool Value);
  void __fastcall WriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  void __fastcall WriteInteger(const UnicodeString & Name, int Value);
  void __fastcall WriteEnum(const UnicodeString & Name, int Value);
  void __fastcall WriteInt64(const UnicodeString & Name, __int64 Value);
  void __fastcall WriteDateTime(const UnicodeString & Name, TDateTime Value);
  void __fastcall WriteFloat(const UnicodeString & Name, double Value);
  void __fastcall WriteBinaryData(const UnicodeString & Name, const void * Buffer, int Size);
  void __fastcall WriteString(const UnicodeString & Name, const UnicodeString & Value);
  void __fastcall WriteBinaryData(const UnicodeString & Name, const RawByteString & Value);
  void __fastcall WriteBinaryDataAsString(const UnicodeString & Name, const RawByteString & Value);

  virtual void __fastcall Flush();

  UnicodeString __property_get_Storage() const { return FStorage; } __declspec(property(get = __property_get_Storage)) UnicodeString Storage;
  UnicodeString __property_get_CurrentSubKey() const { return ::rtl::Mutable(this)->GetCurrentSubKey(); } __declspec(property(get = __property_get_CurrentSubKey)) UnicodeString CurrentSubKey;
  TStorageAccessMode __property_get_AccessMode() const { return FAccessMode; } void __property_set_AccessMode(TStorageAccessMode Value) { SetAccessMode(Value); } __declspec(property(get = __property_get_AccessMode, put = __property_set_AccessMode)) TStorageAccessMode AccessMode;
  bool __property_get_Explicit() const { return FExplicit; } void __property_set_Explicit(bool Value) { FExplicit = Value; } __declspec(property(get = __property_get_Explicit, put = __property_set_Explicit)) bool Explicit;
  bool __property_get_ForceSave() const { return FForceSave; } void __property_set_ForceSave(bool Value) { FForceSave = Value; } __declspec(property(get = __property_get_ForceSave, put = __property_set_ForceSave)) bool ForceSave;
  bool __property_get_MungeStringValues() const { return FMungeStringValues; } void __property_set_MungeStringValues(bool Value) { FMungeStringValues = Value; } __declspec(property(get = __property_get_MungeStringValues, put = __property_set_MungeStringValues)) bool MungeStringValues;
  UnicodeString __property_get_Source() const { return ::rtl::Mutable(this)->GetSource(); } __declspec(property(get = __property_get_Source)) UnicodeString Source;
  bool __property_get_Temporary() const { return ::rtl::Mutable(this)->GetTemporary(); } __declspec(property(get = __property_get_Temporary)) bool Temporary;
  UnicodeString __property_get_UnmungedRoot() const { return FUnmungedRoot; } void __property_set_UnmungedRoot(UnicodeString Value) { FUnmungedRoot = Value; } __declspec(property(get = __property_get_UnmungedRoot, put = __property_set_UnmungedRoot)) UnicodeString UnmungedRoot;

protected:
  enum THierarchicalStorageAccess : int { hsaRead = 0x01, hsaWrite = 0x02 };
  struct TKeyEntry
  {
    UnicodeString Key;
    int Levels;
    unsigned int Access;
  };

  UnicodeString FStorage;
  std::vector<TKeyEntry> FKeyHistory;
  TStorageAccessMode FAccessMode;
  bool FExplicit;
  bool FForceSave;
  bool FMungeStringValues;
  bool FForceAnsi;
  int FFakeReadOnlyOpens;
  int FRootAccess;
  UnicodeString FUnmungedRoot;

  bool __property_get_ForceAnsi() const { return FForceAnsi; } void __property_set_ForceAnsi(bool Value) { FForceAnsi = Value; } __declspec(property(get = __property_get_ForceAnsi, put = __property_set_ForceAnsi)) bool ForceAnsi;

  UnicodeString __fastcall GetCurrentSubKey();
  UnicodeString __fastcall GetCurrentSubKeyMunged();
  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
  virtual bool __fastcall DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi) = 0;
  virtual bool __fastcall DoValueExists(const UnicodeString & Value, bool ForceAnsi) = 0;
  static UnicodeString __fastcall IncludeTrailingBackslash(const UnicodeString & S);
  static UnicodeString __fastcall ExcludeTrailingBackslash(const UnicodeString & S);
  virtual bool __fastcall DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate) = 0;
  virtual void __fastcall DoCloseSubKey() = 0;
  bool MungingKeyName(const UnicodeString & Key);
  UnicodeString __fastcall MungeKeyName(const UnicodeString & Key);
  UnicodeString MungeIniName(const UnicodeString & Str);
  virtual UnicodeString __fastcall GetSource() = 0;
  virtual bool __fastcall GetTemporary();
  virtual bool __fastcall DoDeleteSubKey(const UnicodeString & SubKey) = 0;
  virtual bool __fastcall DoDeleteValue(const UnicodeString & Name) = 0;

  virtual void __fastcall DoGetSubKeyNames(TStrings * Strings) = 0;
  virtual void __fastcall DoGetValueNames(TStrings * Strings) = 0;

  virtual void __fastcall DoWriteBool(const UnicodeString & Name, bool Value) = 0;
  virtual void __fastcall DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value) = 0;
  virtual void __fastcall DoWriteInteger(const UnicodeString & Name, int Value) = 0;
  virtual void __fastcall DoWriteInt64(const UnicodeString & Name, __int64 Value) = 0;
  virtual void __fastcall DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int Size) = 0;

  virtual bool __fastcall DoReadBool(const UnicodeString & Name, bool Default) = 0;
  virtual UnicodeString __fastcall DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default) = 0;
  int ReadIntegerWithMapping(const UnicodeString & Name, int Default, const TIntMapping * Mapping);
  virtual int __fastcall DoReadInteger(const UnicodeString & Name, int Default, const TIntMapping * Mapping) = 0;
  virtual __int64 __fastcall DoReadInt64(const UnicodeString & Name, __int64 Default) = 0;
  virtual TDateTime __fastcall DoReadDateTime(const UnicodeString & Name, TDateTime Default) = 0;
  virtual double __fastcall DoReadFloat(const UnicodeString & Name, double Default) = 0;
  virtual size_t __fastcall DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size) = 0;

  virtual size_t __fastcall DoBinaryDataSize(const UnicodeString & Name) = 0;

  virtual UnicodeString __fastcall DoReadRootAccessString();

  size_t __fastcall BinaryDataSize(const UnicodeString & Name);
  UnicodeString __fastcall ReadAccessString();
  unsigned int __fastcall ReadAccess(unsigned int CurrentAccess);
  virtual bool __fastcall HasAccess(unsigned int Access);
  inline bool __fastcall CanRead();
  inline bool __fastcall CanWrite();
  virtual unsigned int __fastcall GetCurrentAccess();
};
//---------------------------------------------------------------------------
extern TIntMapping AutoSwitchMapping;
extern TIntMapping AutoSwitchReversedMapping;
//---------------------------------------------------------------------------
template<typename T>
T __fastcall THierarchicalStorage::ReadEnum(const UnicodeString & Name, const T & Default, const TIntMapping & Mapping)
{
  const TIntMapping * AMapping = (Mapping == TIntMapping()) ? NULL : &Mapping;
  return T(ReadIntegerWithMapping(Name, int(Default), AMapping));
}
//---------------------------------------------------------------------------
class TRegistryStorage : public THierarchicalStorage
{
public:
  __fastcall TRegistryStorage(const UnicodeString & AStorage, HKEY ARootKey, REGSAM WowMode = 0);
  __fastcall TRegistryStorage(const UnicodeString & AStorage);
  virtual __fastcall ~TRegistryStorage();

  bool __fastcall Copy(TRegistryStorage * Storage);

protected:
  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
  virtual bool __fastcall DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool __fastcall DoValueExists(const UnicodeString & Value, bool ForceAnsi);
  virtual bool __fastcall DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void __fastcall DoCloseSubKey();
  virtual UnicodeString __fastcall GetSource();
  virtual size_t __fastcall DoBinaryDataSize(const UnicodeString & Name);
  virtual bool __fastcall DoDeleteSubKey(const UnicodeString & SubKey);
  virtual bool __fastcall DoDeleteValue(const UnicodeString & Name);

  virtual void __fastcall DoGetSubKeyNames(TStrings * Strings);
  virtual void __fastcall DoGetValueNames(TStrings* Strings);

  virtual void __fastcall DoWriteBool(const UnicodeString & Name, bool Value);
  virtual void __fastcall DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void __fastcall DoWriteInteger(const UnicodeString & Name, int Value);
  virtual void __fastcall DoWriteInt64(const UnicodeString & Name, __int64 Value);
  virtual void __fastcall DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int Size);

  virtual bool __fastcall DoReadBool(const UnicodeString & Name, bool Default);
  virtual int __fastcall DoReadInteger(const UnicodeString & Name, int Default, const TIntMapping * Mapping);
  virtual __int64 __fastcall DoReadInt64(const UnicodeString & Name, __int64 Default);
  virtual TDateTime __fastcall DoReadDateTime(const UnicodeString & Name, TDateTime Default);
  virtual double __fastcall DoReadFloat(const UnicodeString & Name, double Default);
  virtual UnicodeString __fastcall DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  virtual size_t __fastcall DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

private:
  TRegistry * FRegistry;
  REGSAM FWowMode;

  void __fastcall Init();
};
//---------------------------------------------------------------------------
class TCustomIniFileStorage : public THierarchicalStorage
{
public:
  __fastcall TCustomIniFileStorage(const UnicodeString & Storage, TCustomIniFile * IniFile);
  virtual __fastcall ~TCustomIniFileStorage();

  virtual bool __fastcall OpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void __fastcall CloseSubKey();

private:
  UnicodeString __fastcall GetCurrentSection();
  inline bool __fastcall HandleByMasterStorage();
  inline bool __fastcall HandleReadByMasterStorage(const UnicodeString & Name);
  inline bool __fastcall DoValueExistsInternal(const UnicodeString & Value, bool ForceAnsi);
  void __fastcall DoWriteStringRawInternal(const UnicodeString & Name, const UnicodeString & Value);
  int __fastcall DoReadIntegerWithMapping(const UnicodeString & Name, int Default, const TIntMapping * Mapping);

protected:
  TCustomIniFile * FIniFile;
  std::unique_ptr<TStringList> FSections;
  std::unique_ptr<THierarchicalStorage> FMasterStorage;
  int FMasterStorageOpenFailures;
  bool FOpeningSubKey;

  UnicodeString __property_get_CurrentSection() const { return ::rtl::Mutable(this)->GetCurrentSection(); } __declspec(property(get = __property_get_CurrentSection)) UnicodeString CurrentSection;
  virtual void __fastcall SetAccessMode(TStorageAccessMode value);
  virtual bool __fastcall DoKeyExists(const UnicodeString & SubKey, bool ForceAnsi);
  virtual bool __fastcall DoValueExists(const UnicodeString & Value, bool ForceAnsi);
  virtual bool __fastcall DoOpenSubKey(const UnicodeString & SubKey, bool CanCreate);
  virtual void __fastcall DoCloseSubKey();
  virtual UnicodeString __fastcall GetSource();
  virtual size_t __fastcall DoBinaryDataSize(const UnicodeString & Name);
  virtual bool __fastcall DoDeleteSubKey(const UnicodeString & SubKey);
  virtual bool __fastcall DoDeleteValue(const UnicodeString & Name);

  virtual void __fastcall DoGetSubKeyNames(TStrings * Strings);
  virtual void __fastcall DoGetValueNames(TStrings * Strings);

  virtual void __fastcall DoWriteBool(const UnicodeString & Name, bool Value);
  virtual void __fastcall DoWriteStringRaw(const UnicodeString & Name, const UnicodeString & Value);
  virtual void __fastcall DoWriteInteger(const UnicodeString & Name, int Value);
  virtual void __fastcall DoWriteInt64(const UnicodeString & Name, __int64 Value);
  virtual void __fastcall DoWriteBinaryData(const UnicodeString & Name, const void * Buffer, int Size);

  virtual bool __fastcall DoReadBool(const UnicodeString & Name, bool Default);
  virtual int __fastcall DoReadInteger(const UnicodeString & Name, int Default, const TIntMapping * Mapping);
  virtual __int64 __fastcall DoReadInt64(const UnicodeString & Name, __int64 Default);
  virtual TDateTime __fastcall DoReadDateTime(const UnicodeString & Name, TDateTime Default);
  virtual double __fastcall DoReadFloat(const UnicodeString & Name, double Default);
  virtual UnicodeString __fastcall DoReadStringRaw(const UnicodeString & Name, const UnicodeString & Default);
  virtual size_t __fastcall DoReadBinaryData(const UnicodeString & Name, void * Buffer, size_t Size);

  virtual UnicodeString __fastcall DoReadRootAccessString();
  virtual unsigned int __fastcall GetCurrentAccess();
  virtual bool __fastcall HasAccess(unsigned int Access);

  void __fastcall CacheSections();
  void __fastcall ResetCache();
  bool __fastcall DoKeyExistsInternal(const UnicodeString & SubKey);
};
//---------------------------------------------------------------------------
class TIniFileStorage : public TCustomIniFileStorage
{
public:
  static TIniFileStorage * __fastcall CreateFromPath(const UnicodeString & AStorage);
  static TIniFileStorage * __fastcall CreateNul();
  virtual __fastcall ~TIniFileStorage();

  virtual void __fastcall Flush();

private:
  __fastcall TIniFileStorage(const UnicodeString & FileName, TCustomIniFile * IniFile);
  TStrings * FOriginal;
  void __fastcall ApplyOverrides();
};
//---------------------------------------------------------------------------
class TOptionsStorage : public TCustomIniFileStorage
{
public:
  __fastcall TOptionsStorage(TStrings * Options, bool AllowWrite);
  __fastcall TOptionsStorage(TStrings * Options, const UnicodeString & RootKey, THierarchicalStorage * MasterStorage);

protected:
  virtual bool __fastcall GetTemporary();
};
//---------------------------------------------------------------------------
UnicodeString __fastcall PuttyMungeStr(const UnicodeString & Str);
AnsiString PuttyStr(const UnicodeString & Str);
TIntMapping CreateIntMappingFromEnumNames(const UnicodeString & Names);
//---------------------------------------------------------------------------
#endif
