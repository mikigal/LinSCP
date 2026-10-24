// System.Win.Registry. The registry does not exist on Linux: TRegistry works
// on an empty registry that cannot be written (keys do not open, values do
// not exist, writes fail with ERegistryException). The configuration is
// stored in an INI file instead, see docs/port/PLAN.md, section 3.3.
//---------------------------------------------------------------------------
#ifndef RtlRegistryH
#define RtlRegistryH
//---------------------------------------------------------------------------
#include <rtl/IniFiles.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Win
{
namespace Registry
{
//---------------------------------------------------------------------------
RTL_DECLARE_EXCEPTION(ERegistryException, Exception)
//---------------------------------------------------------------------------
enum TRegDataType { rdUnknown, rdString, rdExpandString, rdInteger, rdBinary };
struct TRegDataInfo
{
  TRegDataType RegData;
  int DataSize;
};
//---------------------------------------------------------------------------
class TRegistry : public TObject
{
public:
  TRegistry();
  explicit TRegistry(unsigned int AAccess);
  virtual ~TRegistry();

  void CloseKey();
  bool CreateKey(const UnicodeString Key);
  bool DeleteKey(const UnicodeString Key);
  bool DeleteValue(const UnicodeString Name);
  bool GetDataInfo(const UnicodeString ValueName, TRegDataInfo & Value);
  int GetDataSize(const UnicodeString ValueName);
  TRegDataType GetDataType(const UnicodeString ValueName);
  void GetKeyNames(System::Classes::TStrings * Strings);
  void GetValueNames(System::Classes::TStrings * Strings);
  bool HasSubKeys();
  bool KeyExists(const UnicodeString Key);
  bool OpenKey(const UnicodeString Key, bool CanCreate);
  bool OpenKeyReadOnly(const UnicodeString Key);
  bool ValueExists(const UnicodeString Name);

  int ReadBinaryData(const UnicodeString Name, void * Buffer, int BufSize);
  bool ReadBool(const UnicodeString Name);
  TDateTime ReadDateTime(const UnicodeString Name);
  double ReadFloat(const UnicodeString Name);
  int ReadInteger(const UnicodeString Name);
  UnicodeString ReadString(const UnicodeString Name);
  void WriteBinaryData(const UnicodeString Name, void * Buffer, int BufSize);
  void WriteBool(const UnicodeString Name, bool Value);
  void WriteDateTime(const UnicodeString Name, TDateTime Value);
  void WriteExpandString(const UnicodeString Name, const UnicodeString Value);
  void WriteFloat(const UnicodeString Name, double Value);
  void WriteInteger(const UnicodeString Name, int Value);
  void WriteString(const UnicodeString Name, const UnicodeString Value);

  HKEY GetCurrentKey() const { return FCurrentKey; }
  UnicodeString GetCurrentPath() const { return FCurrentPath; }
  HKEY GetRootKey() const { return FRootKey; }
  void SetRootKey(HKEY Value) { CloseKey(); FRootKey = Value; }
  unsigned int GetAccess() const { return FAccess; }
  void SetAccess(unsigned int Value) { FAccess = Value; }
  bool GetLazyWrite() const { return FLazyWrite; }
  void SetLazyWrite(bool Value) { FLazyWrite = Value; }

  RTL_PROPERTY_GET(CurrentKey, GetCurrentKey)
  __declspec(property(get = __property_get_CurrentKey)) HKEY CurrentKey;
  RTL_PROPERTY_GET(CurrentPath, GetCurrentPath)
  __declspec(property(get = __property_get_CurrentPath)) UnicodeString CurrentPath;
  RTL_PROPERTY_GET(RootKey, GetRootKey) RTL_PROPERTY_PUT(RootKey, SetRootKey)
  __declspec(property(get = __property_get_RootKey, put = __property_set_RootKey)) HKEY RootKey;
  RTL_PROPERTY_GET(Access, GetAccess) RTL_PROPERTY_PUT(Access, SetAccess)
  __declspec(property(get = __property_get_Access, put = __property_set_Access)) unsigned int Access;
  RTL_PROPERTY_GET(LazyWrite, GetLazyWrite) RTL_PROPERTY_PUT(LazyWrite, SetLazyWrite)
  __declspec(property(get = __property_get_LazyWrite, put = __property_set_LazyWrite)) bool LazyWrite;

private:
  [[noreturn]] void ReadError(const UnicodeString & Name);
  [[noreturn]] void WriteError(const UnicodeString & Name);

  HKEY FRootKey;
  HKEY FCurrentKey;
  UnicodeString FCurrentPath;
  unsigned int FAccess;
  bool FLazyWrite;
};
//---------------------------------------------------------------------------
} // namespace Registry
} // namespace Win
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Win::Registry;
//---------------------------------------------------------------------------
#endif
