// System.Win.Registry on an empty registry, see rtl/Registry.h.
//---------------------------------------------------------------------------
#include <rtl/Registry.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Win
{
namespace Registry
{
//---------------------------------------------------------------------------
TRegistry::TRegistry() :
  TRegistry(KEY_ALL_ACCESS)
{
}
//---------------------------------------------------------------------------
TRegistry::TRegistry(unsigned int AAccess) :
  FRootKey(HKEY_CURRENT_USER),
  FCurrentKey(nullptr),
  FAccess(AAccess),
  FLazyWrite(true)
{
}
//---------------------------------------------------------------------------
TRegistry::~TRegistry()
{
  CloseKey();
}
//---------------------------------------------------------------------------
void TRegistry::CloseKey()
{
  FCurrentKey = nullptr;
  FCurrentPath = UnicodeString();
}
//---------------------------------------------------------------------------
bool TRegistry::CreateKey(const UnicodeString)
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::DeleteKey(const UnicodeString)
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::DeleteValue(const UnicodeString)
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::GetDataInfo(const UnicodeString, TRegDataInfo & Value)
{
  Value.RegData = rdUnknown;
  Value.DataSize = 0;
  return false;
}
//---------------------------------------------------------------------------
int TRegistry::GetDataSize(const UnicodeString)
{
  return -1;
}
//---------------------------------------------------------------------------
TRegDataType TRegistry::GetDataType(const UnicodeString)
{
  return rdUnknown;
}
//---------------------------------------------------------------------------
void TRegistry::GetKeyNames(System::Classes::TStrings * Strings)
{
  Strings->Clear();
}
//---------------------------------------------------------------------------
void TRegistry::GetValueNames(System::Classes::TStrings * Strings)
{
  Strings->Clear();
}
//---------------------------------------------------------------------------
bool TRegistry::HasSubKeys()
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::KeyExists(const UnicodeString)
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::OpenKey(const UnicodeString, bool)
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::OpenKeyReadOnly(const UnicodeString)
{
  return false;
}
//---------------------------------------------------------------------------
bool TRegistry::ValueExists(const UnicodeString)
{
  return false;
}
//---------------------------------------------------------------------------
void TRegistry::ReadError(const UnicodeString & Name)
{
  throw ERegistryException(L"Invalid data type for '" + Name + L"'");
}
//---------------------------------------------------------------------------
void TRegistry::WriteError(const UnicodeString & Name)
{
  throw ERegistryException(L"Failed to set data for '" + Name + L"'");
}
//---------------------------------------------------------------------------
int TRegistry::ReadBinaryData(const UnicodeString Name, void *, int)
{
  ReadError(Name);
}
//---------------------------------------------------------------------------
bool TRegistry::ReadBool(const UnicodeString Name)
{
  ReadError(Name);
}
//---------------------------------------------------------------------------
TDateTime TRegistry::ReadDateTime(const UnicodeString Name)
{
  ReadError(Name);
}
//---------------------------------------------------------------------------
double TRegistry::ReadFloat(const UnicodeString Name)
{
  ReadError(Name);
}
//---------------------------------------------------------------------------
int TRegistry::ReadInteger(const UnicodeString Name)
{
  ReadError(Name);
}
//---------------------------------------------------------------------------
UnicodeString TRegistry::ReadString(const UnicodeString Name)
{
  ReadError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteBinaryData(const UnicodeString Name, void *, int)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteBool(const UnicodeString Name, bool)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteDateTime(const UnicodeString Name, TDateTime)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteExpandString(const UnicodeString Name, const UnicodeString)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteFloat(const UnicodeString Name, double)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteInteger(const UnicodeString Name, int)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
void TRegistry::WriteString(const UnicodeString Name, const UnicodeString)
{
  WriteError(Name);
}
//---------------------------------------------------------------------------
} // namespace Registry
} // namespace Win
} // namespace System
//---------------------------------------------------------------------------
