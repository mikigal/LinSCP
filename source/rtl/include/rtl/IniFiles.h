// The System.IniFiles unit: TCustomIniFile and TMemIniFile.
//---------------------------------------------------------------------------
#ifndef RtlIniFilesH
#define RtlIniFilesH
//---------------------------------------------------------------------------
#include <rtl/Classes.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Inifiles
{
//---------------------------------------------------------------------------
RTL_DECLARE_EXCEPTION(EIniFileException, Exception)
//---------------------------------------------------------------------------
class TCustomIniFile : public TObject
{
public:
  explicit TCustomIniFile(const UnicodeString FileName) : FFileName(FileName) {}
  virtual ~TCustomIniFile() {}

  bool SectionExists(const UnicodeString Section);
  virtual UnicodeString ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default) = 0;
  virtual void WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value) = 0;
  virtual int ReadInteger(const UnicodeString Section, const UnicodeString Ident, int Default);
  virtual void WriteInteger(const UnicodeString Section, const UnicodeString Ident, int Value);
  virtual __int64 ReadInt64(const UnicodeString Section, const UnicodeString Ident, __int64 Default);
  virtual void WriteInt64(const UnicodeString Section, const UnicodeString Ident, __int64 Value);
  virtual bool ReadBool(const UnicodeString Section, const UnicodeString Ident, bool Default);
  virtual void WriteBool(const UnicodeString Section, const UnicodeString Ident, bool Value);
  virtual int ReadBinaryStream(const UnicodeString Section, const UnicodeString Name, TStream * Value);
  virtual void WriteBinaryStream(const UnicodeString Section, const UnicodeString Name, TStream * Value);
  virtual TDateTime ReadDate(const UnicodeString Section, const UnicodeString Name, TDateTime Default);
  virtual TDateTime ReadDateTime(const UnicodeString Section, const UnicodeString Name, TDateTime Default);
  virtual double ReadFloat(const UnicodeString Section, const UnicodeString Name, double Default);
  virtual TDateTime ReadTime(const UnicodeString Section, const UnicodeString Name, TDateTime Default);
  virtual void WriteDate(const UnicodeString Section, const UnicodeString Name, TDateTime Value);
  virtual void WriteDateTime(const UnicodeString Section, const UnicodeString Name, TDateTime Value);
  virtual void WriteFloat(const UnicodeString Section, const UnicodeString Name, double Value);
  virtual void WriteTime(const UnicodeString Section, const UnicodeString Name, TDateTime Value);
  virtual void ReadSection(const UnicodeString Section, TStrings * Strings) = 0;
  virtual void ReadSections(TStrings * Strings) = 0;
  virtual void ReadSections(const UnicodeString Section, TStrings * Strings);
  virtual void ReadSubSections(const UnicodeString Section, TStrings * Strings, bool Recurse = false);
  virtual void ReadSectionValues(const UnicodeString Section, TStrings * Strings) = 0;
  virtual void EraseSection(const UnicodeString Section) = 0;
  virtual void DeleteKey(const UnicodeString Section, const UnicodeString Ident) = 0;
  virtual void UpdateFile() = 0;
  virtual bool ValueExists(const UnicodeString Section, const UnicodeString Ident);

  UnicodeString GetFileName() const { return FFileName; }
  __declspec(property(get = GetFileName)) UnicodeString FileName;

protected:
  UnicodeString FFileName;
};
//---------------------------------------------------------------------------
// The whole file in memory; UpdateFile writes it back
class TMemIniFile : public TCustomIniFile
{
public:
  explicit TMemIniFile(const UnicodeString FileName);
  TMemIniFile(const UnicodeString FileName, TEncoding * AEncoding);
  TMemIniFile(const UnicodeString FileName, TEncoding * AEncoding, bool CaseSensitive);
  virtual ~TMemIniFile();

  void Clear();
  virtual void DeleteKey(const UnicodeString Section, const UnicodeString Ident) override;
  virtual void EraseSection(const UnicodeString Section) override;
  void GetStrings(TStrings * List);
  virtual void ReadSection(const UnicodeString Section, TStrings * Strings) override;
  using TCustomIniFile::ReadSections;
  virtual void ReadSections(TStrings * Strings) override;
  virtual void ReadSectionValues(const UnicodeString Section, TStrings * Strings) override;
  virtual UnicodeString ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default) override;
  void Rename(const UnicodeString FileName, bool Reload);
  void SetStrings(TStrings * List);
  virtual void UpdateFile() override;
  virtual void WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value) override;

  bool GetCaseSensitive() const { return FSections->CaseSensitive; }
  void SetCaseSensitive(bool Value);
  TEncoding * GetEncoding() const { return FEncoding; }
  void SetEncoding(TEncoding * Value) { FEncoding = Value; }
  __declspec(property(get = GetCaseSensitive, put = SetCaseSensitive)) bool CaseSensitive;
  __declspec(property(get = GetEncoding, put = SetEncoding)) TEncoding * Encoding;

private:
  TStringList * AddSection(const UnicodeString Section);
  TStringList * FindSection(const UnicodeString Section);
  void LoadValues();

  TStringList * FSections;
  TEncoding * FEncoding;
};
//---------------------------------------------------------------------------
} // namespace Inifiles
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Inifiles;
//---------------------------------------------------------------------------
#endif
