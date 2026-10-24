// System.IniFiles
//---------------------------------------------------------------------------
#include <rtl/IniFiles.h>
#include <rtl/StrUtils.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Inifiles
{
//---------------------------------------------------------------------------
// TCustomIniFile
//---------------------------------------------------------------------------
bool TCustomIniFile::SectionExists(const UnicodeString Section)
{
  TStringList Strings;
  ReadSection(Section, &Strings);
  return (Strings.Count > 0);
}
//---------------------------------------------------------------------------
int TCustomIniFile::ReadInteger(const UnicodeString Section, const UnicodeString Ident, int Default)
{
  UnicodeString IntStr = ReadString(Section, Ident, UnicodeString());
  if ((IntStr.Length() > 2) && (IntStr[1] == L'0') && ((IntStr[2] == L'X') || (IntStr[2] == L'x')))
  {
    IntStr = L"$" + IntStr.SubString(3, MaxInt);
  }
  return StrToIntDef(IntStr, Default);
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteInteger(const UnicodeString Section, const UnicodeString Ident, int Value)
{
  WriteString(Section, Ident, IntToStr(Value));
}
//---------------------------------------------------------------------------
__int64 TCustomIniFile::ReadInt64(const UnicodeString Section, const UnicodeString Ident, __int64 Default)
{
  UnicodeString IntStr = ReadString(Section, Ident, UnicodeString());
  if ((IntStr.Length() > 2) && (IntStr[1] == L'0') && ((IntStr[2] == L'X') || (IntStr[2] == L'x')))
  {
    IntStr = L"$" + IntStr.SubString(3, MaxInt);
  }
  return StrToInt64Def(IntStr, Default);
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteInt64(const UnicodeString Section, const UnicodeString Ident, __int64 Value)
{
  WriteString(Section, Ident, IntToStr(Value));
}
//---------------------------------------------------------------------------
bool TCustomIniFile::ReadBool(const UnicodeString Section, const UnicodeString Ident, bool Default)
{
  return (ReadInteger(Section, Ident, Default ? 1 : 0) != 0);
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteBool(const UnicodeString Section, const UnicodeString Ident, bool Value)
{
  WriteString(Section, Ident, Value ? L"1" : L"0");
}
//---------------------------------------------------------------------------
int TCustomIniFile::ReadBinaryStream(const UnicodeString Section, const UnicodeString Name, TStream * Value)
{
  UnicodeString Text = ReadString(Section, Name, UnicodeString());
  if (Text.IsEmpty())
  {
    return 0;
  }
  std::string Bytes;
  for (int I = 1; I + 1 <= Text.Length(); I += 2)
  {
    int Byte;
    if (!TryStrToInt(L"$" + Text.SubString(I, 2), Byte))
    {
      break;
    }
    Bytes.push_back(static_cast<char>(Byte));
  }
  __int64 Start = Value->Position;
  Value->WriteBuffer(Bytes.data(), static_cast<int>(Bytes.size()));
  Value->Position = Start;
  return static_cast<int>(Bytes.size());
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteBinaryStream(const UnicodeString Section, const UnicodeString Name, TStream * Value)
{
  __int64 Size = Value->Size - Value->Position;
  std::string Bytes(static_cast<size_t>(Size), '\0');
  if (Size > 0)
  {
    Value->ReadBuffer(Bytes.data(), static_cast<int>(Size));
  }
  UnicodeString Text;
  for (char Ch : Bytes)
  {
    Text += IntToHex(static_cast<unsigned char>(Ch), 2);
  }
  WriteString(Section, Name, Text);
}
//---------------------------------------------------------------------------
TDateTime TCustomIniFile::ReadDate(const UnicodeString Section, const UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  return TryStrToDate(ReadString(Section, Name, UnicodeString()), Result) ? Result : Default;
}
//---------------------------------------------------------------------------
TDateTime TCustomIniFile::ReadDateTime(const UnicodeString Section, const UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  return TryStrToDateTime(ReadString(Section, Name, UnicodeString()), Result) ? Result : Default;
}
//---------------------------------------------------------------------------
double TCustomIniFile::ReadFloat(const UnicodeString Section, const UnicodeString Name, double Default)
{
  double Result;
  return TryStrToFloat(ReadString(Section, Name, UnicodeString()), Result) ? Result : Default;
}
//---------------------------------------------------------------------------
TDateTime TCustomIniFile::ReadTime(const UnicodeString Section, const UnicodeString Name, TDateTime Default)
{
  TDateTime Result;
  return TryStrToTime(ReadString(Section, Name, UnicodeString()), Result) ? Result : Default;
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteDate(const UnicodeString Section, const UnicodeString Name, TDateTime Value)
{
  WriteString(Section, Name, DateToStr(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteDateTime(const UnicodeString Section, const UnicodeString Name, TDateTime Value)
{
  WriteString(Section, Name, DateTimeToStr(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteFloat(const UnicodeString Section, const UnicodeString Name, double Value)
{
  WriteString(Section, Name, FloatToStr(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFile::WriteTime(const UnicodeString Section, const UnicodeString Name, TDateTime Value)
{
  WriteString(Section, Name, TimeToStr(Value));
}
//---------------------------------------------------------------------------
void TCustomIniFile::ReadSections(const UnicodeString Section, TStrings * Strings)
{
  // Sections below Section, separated by backslashes
  TStringList AllSections;
  ReadSections(&AllSections);
  UnicodeString Prefix = Section.IsEmpty() ? UnicodeString() : (Section + L"\\");
  Strings->BeginUpdate();
  Strings->Clear();
  for (int I = 0; I < AllSections.Count; I++)
  {
    UnicodeString Name = AllSections.Strings[I];
    if (Prefix.IsEmpty() || StartsText(Prefix, Name))
    {
      Strings->Add(Name);
    }
  }
  Strings->EndUpdate();
}
//---------------------------------------------------------------------------
void TCustomIniFile::ReadSubSections(const UnicodeString Section, TStrings * Strings, bool Recurse)
{
  TStringList AllSections;
  ReadSections(&AllSections);
  UnicodeString Prefix = Section.IsEmpty() ? UnicodeString() : (Section + L"\\");
  Strings->BeginUpdate();
  Strings->Clear();
  for (int I = 0; I < AllSections.Count; I++)
  {
    UnicodeString Name = AllSections.Strings[I];
    if (Prefix.IsEmpty() || ((Name.Length() > Prefix.Length()) && AnsiSameText(Name.SubString(1, Prefix.Length()), Prefix)))
    {
      UnicodeString Rest = Name.SubString(Prefix.Length() + 1, MaxInt);
      int P = Rest.Pos(L"\\");
      if (P > 0)
      {
        if (!Recurse)
        {
          Rest = Rest.SubString(1, P - 1);
        }
      }
      if (Strings->IndexOf(Rest) < 0)
      {
        Strings->Add(Rest);
      }
    }
  }
  Strings->EndUpdate();
}
//---------------------------------------------------------------------------
bool TCustomIniFile::ValueExists(const UnicodeString Section, const UnicodeString Ident)
{
  TStringList Strings;
  ReadSection(Section, &Strings);
  return (Strings.IndexOf(Ident) >= 0);
}
//---------------------------------------------------------------------------
// TMemIniFile
//---------------------------------------------------------------------------
TMemIniFile::TMemIniFile(const UnicodeString FileName) :
  TMemIniFile(FileName, nullptr, false)
{
}
//---------------------------------------------------------------------------
TMemIniFile::TMemIniFile(const UnicodeString FileName, TEncoding * AEncoding) :
  TMemIniFile(FileName, AEncoding, false)
{
}
//---------------------------------------------------------------------------
TMemIniFile::TMemIniFile(const UnicodeString FileName, TEncoding * AEncoding, bool CaseSensitive) :
  TCustomIniFile(FileName),
  FSections(new TStringList()),
  FEncoding(AEncoding)
{
  FSections->CaseSensitive = CaseSensitive;
  try
  {
    LoadValues();
  }
  catch (...)
  {
    Clear();
    delete FSections;
    throw;
  }
}
//---------------------------------------------------------------------------
TMemIniFile::~TMemIniFile()
{
  Clear();
  delete FSections;
}
//---------------------------------------------------------------------------
void TMemIniFile::LoadValues()
{
  if (!FFileName.IsEmpty() && FileExists(FFileName))
  {
    TStringList List;
    List.LoadFromFile(FFileName, FEncoding);
    FEncoding = List.Encoding;
    SetStrings(&List);
  }
  else
  {
    Clear();
  }
}
//---------------------------------------------------------------------------
TStringList * TMemIniFile::AddSection(const UnicodeString Section)
{
  TStringList * Result = new TStringList();
  Result->CaseSensitive = FSections->CaseSensitive;
  FSections->AddObject(Section, Result);
  return Result;
}
//---------------------------------------------------------------------------
TStringList * TMemIniFile::FindSection(const UnicodeString Section)
{
  int I = FSections->IndexOf(Section);
  return (I >= 0) ? static_cast<TStringList *>(FSections->Objects[I]) : nullptr;
}
//---------------------------------------------------------------------------
void TMemIniFile::Clear()
{
  for (int I = 0; I < FSections->Count; I++)
  {
    delete FSections->Objects[I];
  }
  FSections->Clear();
}
//---------------------------------------------------------------------------
void TMemIniFile::DeleteKey(const UnicodeString Section, const UnicodeString Ident)
{
  TStringList * Strings = FindSection(Section);
  if (Strings != nullptr)
  {
    int J = Strings->IndexOfName(Ident);
    if (J >= 0)
    {
      Strings->Delete(J);
    }
  }
}
//---------------------------------------------------------------------------
void TMemIniFile::EraseSection(const UnicodeString Section)
{
  int I = FSections->IndexOf(Section);
  if (I >= 0)
  {
    delete FSections->Objects[I];
    FSections->Delete(I);
  }
}
//---------------------------------------------------------------------------
void TMemIniFile::GetStrings(TStrings * List)
{
  List->BeginUpdate();
  try
  {
    for (int I = 0; I < FSections->Count; I++)
    {
      List->Add(L"[" + FSections->Strings[I] + L"]");
      TStrings * Strings = static_cast<TStrings *>(FSections->Objects[I]);
      for (int J = 0; J < Strings->Count; J++)
      {
        List->Add(Strings->Strings[J]);
      }
      List->Add(UnicodeString());
    }
  }
  catch (...)
  {
    List->EndUpdate();
    throw;
  }
  List->EndUpdate();
}
//---------------------------------------------------------------------------
void TMemIniFile::ReadSection(const UnicodeString Section, TStrings * Strings)
{
  Strings->BeginUpdate();
  try
  {
    Strings->Clear();
    TStringList * SectionStrings = FindSection(Section);
    if (SectionStrings != nullptr)
    {
      for (int I = 0; I < SectionStrings->Count; I++)
      {
        Strings->Add(SectionStrings->Names[I]);
      }
    }
  }
  catch (...)
  {
    Strings->EndUpdate();
    throw;
  }
  Strings->EndUpdate();
}
//---------------------------------------------------------------------------
void TMemIniFile::ReadSections(TStrings * Strings)
{
  Strings->BeginUpdate();
  try
  {
    Strings->Clear();
    for (int I = 0; I < FSections->Count; I++)
    {
      Strings->Add(FSections->Strings[I]);
    }
  }
  catch (...)
  {
    Strings->EndUpdate();
    throw;
  }
  Strings->EndUpdate();
}
//---------------------------------------------------------------------------
void TMemIniFile::ReadSectionValues(const UnicodeString Section, TStrings * Strings)
{
  Strings->BeginUpdate();
  try
  {
    Strings->Clear();
    TStringList * SectionStrings = FindSection(Section);
    if (SectionStrings != nullptr)
    {
      Strings->AddStrings(SectionStrings);
    }
  }
  catch (...)
  {
    Strings->EndUpdate();
    throw;
  }
  Strings->EndUpdate();
}
//---------------------------------------------------------------------------
UnicodeString TMemIniFile::ReadString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Default)
{
  TStringList * Strings = FindSection(Section);
  if (Strings != nullptr)
  {
    int I = Strings->IndexOfName(Ident);
    if (I >= 0)
    {
      return Strings->Strings[I].SubString(Ident.Length() + 2, MaxInt);
    }
  }
  return Default;
}
//---------------------------------------------------------------------------
void TMemIniFile::Rename(const UnicodeString FileName, bool Reload)
{
  FFileName = FileName;
  if (Reload)
  {
    LoadValues();
  }
}
//---------------------------------------------------------------------------
void TMemIniFile::SetStrings(TStrings * List)
{
  Clear();
  TStringList * Strings = nullptr;
  for (int I = 0; I < List->Count; I++)
  {
    UnicodeString S = Trim(List->Strings[I]);
    if (!S.IsEmpty() && (S[1] != L';'))
    {
      if ((S[1] == L'[') && (S[S.Length()] == L']'))
      {
        Strings = AddSection(Trim(S.SubString(2, S.Length() - 2)));
      }
      else if (Strings != nullptr)
      {
        int J = S.Pos(L"=");
        if (J > 0)
        {
          // Remove spaces around the separator
          Strings->Add(Trim(S.SubString(1, J - 1)) + L"=" + Trim(S.SubString(J + 1, MaxInt)));
        }
        else
        {
          Strings->Add(S);
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
void TMemIniFile::UpdateFile()
{
  TStringList List;
  GetStrings(&List);
  List.SaveToFile(FFileName, FEncoding);
}
//---------------------------------------------------------------------------
void TMemIniFile::WriteString(const UnicodeString Section, const UnicodeString Ident, const UnicodeString Value)
{
  TStringList * Strings = FindSection(Section);
  if (Strings == nullptr)
  {
    Strings = AddSection(Section);
  }
  UnicodeString S = Ident + L"=" + Value;
  int I = Strings->IndexOfName(Ident);
  if (I >= 0)
  {
    Strings->Strings[I] = S;
  }
  else
  {
    Strings->Add(S);
  }
}
//---------------------------------------------------------------------------
void TMemIniFile::SetCaseSensitive(bool Value)
{
  if (Value != FSections->CaseSensitive)
  {
    FSections->CaseSensitive = Value;
    for (int I = 0; I < FSections->Count; I++)
    {
      static_cast<TStringList *>(FSections->Objects[I])->CaseSensitive = Value;
    }
  }
}
//---------------------------------------------------------------------------
} // namespace Inifiles
} // namespace System
