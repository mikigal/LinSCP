// The custom commands.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <Configuration.h>
#include <HierarchicalStorage.h>

#include "CustomCommands.h"
//---------------------------------------------------------------------------
TCustomCommandType::TCustomCommandType() :
  FParams(0), FShortCut(0)
{
}
//---------------------------------------------------------------------------
TCustomCommandType::TCustomCommandType(const TCustomCommandType & Other) :
  FName(Other.FName),
  FCommand(Other.FCommand),
  FParams(Other.FParams),
  FShortCut(Other.FShortCut)
{
}
//---------------------------------------------------------------------------
TCustomCommandType & TCustomCommandType::operator=(const TCustomCommandType & Other)
{
  FName = Other.FName;
  FCommand = Other.FCommand;
  FParams = Other.FParams;
  FShortCut = Other.FShortCut;
  return *this;
}
//---------------------------------------------------------------------------
bool TCustomCommandType::Equals(const TCustomCommandType * Other) const
{
  return
    (FName == Other->FName) &&
    (FCommand == Other->FCommand) &&
    (FParams == Other->FParams) &&
    (FShortCut == Other->FShortCut);
}
//---------------------------------------------------------------------------
TCustomCommandList::TCustomCommandList()
{
  FModified = false;
}
//---------------------------------------------------------------------------
TCustomCommandList::~TCustomCommandList()
{
  Clear();
}
//---------------------------------------------------------------------------
void TCustomCommandList::Reset()
{
  FModified = false;
}
//---------------------------------------------------------------------------
void TCustomCommandList::Modify()
{
  FModified = true;
}
//---------------------------------------------------------------------------
void TCustomCommandList::Load(THierarchicalStorage * Storage)
{
  Clear();

  if (Storage->OpenSubKey(L"CustomCommands", false))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    std::unique_ptr<TStrings> Names(new TStringList());
    Storage->ReadValues(Names.get(), true);
    for (int Index = 0; Index < Names->Count; Index++)
    {
      TCustomCommandType * Command = new TCustomCommandType();
      Command->Name = Names->Names[Index];
      Command->Command = Names->Values[Names->Names[Index]];
      FCommands.push_back(Command);
    }
  }

  if (Storage->OpenSubKey(L"CustomCommandsParams", false))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    for (TCustomCommandType * Command : FCommands)
    {
      Command->Params = Storage->ReadInteger(Command->Name, Command->Params);
    }
  }

  if (Storage->OpenSubKey(L"CustomCommandsShortCuts", false))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    for (TCustomCommandType * Command : FCommands)
    {
      Command->ShortCut = static_cast<Word>(Storage->ReadInteger(Command->Name, Command->ShortCut));
    }
  }
  Reset();
}
//---------------------------------------------------------------------------
void TCustomCommandList::Save(THierarchicalStorage * Storage)
{
  if (Storage->OpenSubKey(L"CustomCommands", true))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    Storage->ClearValues();
    for (const TCustomCommandType * Command : FCommands)
    {
      Storage->WriteString(Command->Name, Command->Command);
    }
  }
  if (Storage->OpenSubKey(L"CustomCommandsParams", true))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    Storage->ClearValues();
    for (const TCustomCommandType * Command : FCommands)
    {
      Storage->WriteInteger(Command->Name, Command->Params);
    }
  }
  if (Storage->OpenSubKey(L"CustomCommandsShortCuts", true))
  {
    ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
    Storage->ClearValues();
    for (const TCustomCommandType * Command : FCommands)
    {
      if (Command->ShortCut != 0)
      {
        Storage->WriteInteger(Command->Name, Command->ShortCut);
      }
    }
  }
}
//---------------------------------------------------------------------------
void TCustomCommandList::Clear()
{
  for (TCustomCommandType * Command : FCommands)
  {
    delete Command;
  }
  FCommands.clear();
}
//---------------------------------------------------------------------------
void TCustomCommandList::Add(const UnicodeString & Name, const UnicodeString & ACommand, int Params)
{
  TCustomCommandType * Command = new TCustomCommandType();
  Command->Name = Name;
  Command->Command = ACommand;
  Command->Params = Params;
  Add(Command);
}
//---------------------------------------------------------------------------
void TCustomCommandList::Add(TCustomCommandType * Command)
{
  Insert(Count, Command);
}
//---------------------------------------------------------------------------
void TCustomCommandList::Insert(int Index, TCustomCommandType * Command)
{
  FCommands.insert(FCommands.begin() + Index, Command);
  Modify();
}
//---------------------------------------------------------------------------
void TCustomCommandList::Change(int Index, TCustomCommandType * ACommand)
{
  TCustomCommandType * Command = FCommands[Index];
  if (!Command->Equals(ACommand))
  {
    delete Command;
    FCommands[Index] = ACommand;
    Modify();
  }
  else
  {
    delete ACommand;
  }
}
//---------------------------------------------------------------------------
void TCustomCommandList::Move(int CurIndex, int NewIndex)
{
  if (CurIndex != NewIndex)
  {
    TCustomCommandType * Command = FCommands[CurIndex];
    FCommands.erase(FCommands.begin() + CurIndex);
    FCommands.insert(FCommands.begin() + NewIndex, Command);
    Modify();
  }
}
//---------------------------------------------------------------------------
void TCustomCommandList::Delete(int Index)
{
  DebugAssert((Index >= 0) && (Index < Count));
  delete FCommands[Index];
  FCommands.erase(FCommands.begin() + Index);
  Modify();
}
//---------------------------------------------------------------------------
int TCustomCommandList::GetCount() const
{
  return static_cast<int>(FCommands.size());
}
//---------------------------------------------------------------------------
const TCustomCommandType * TCustomCommandList::GetCommand(int Index) const
{
  return FCommands[Index];
}
//---------------------------------------------------------------------------
bool TCustomCommandList::Equals(const TCustomCommandList * Other) const
{
  bool Result = (Count == Other->Count);
  for (int Index = 0; Result && (Index < Count); Index++)
  {
    Result = Commands[Index]->Equals(Other->Commands[Index]);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TCustomCommandList::Assign(const TCustomCommandList * Other)
{
  Clear();
  for (int Index = 0; Index < Other->Count; Index++)
  {
    Add(new TCustomCommandType(*Other->Commands[Index]));
  }
  // there should be comparison of with the assigned list, be we rely on caller
  // to do it instead (TGUIConfiguration::SetCopyParamList)
  Modify();
}
//---------------------------------------------------------------------------
const TCustomCommandType * TCustomCommandList::Find(const UnicodeString & Name) const
{
  for (const TCustomCommandType * Command : FCommands)
  {
    if (Command->Name == Name)
    {
      return Command;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
const TCustomCommandType * TCustomCommandList::Find(TShortCut ShortCut) const
{
  for (const TCustomCommandType * Command : FCommands)
  {
    if (Command->ShortCut == ShortCut)
    {
      return Command;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
void TCustomCommandList::ShortCuts(TShortCuts & ShortCuts) const
{
  for (const TCustomCommandType * Command : FCommands)
  {
    if (Command->ShortCut != 0)
    {
      ShortCuts.Add(Command->ShortCut);
    }
  }
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand()
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath) :
  TFileCustomCommand(Data, RemotePath)
{
  FLocalPath = LocalPath;
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & RemotePath, const UnicodeString & LocalPath, const UnicodeString & FileName,
  const UnicodeString & LocalFileName, const UnicodeString & FileList) :
  TFileCustomCommand(Data, RemotePath, FileName, FileList)
{
  FLocalPath = LocalPath;
  FLocalFileName = LocalFileName;
}
//---------------------------------------------------------------------------
int __fastcall TLocalCustomCommand::PatternLen(const UnicodeString & Command, int Index)
{
  int Len;
  if ((Index < Command.Length()) && (Command[Index + 1] == L'\\'))
  {
    Len = 2;
  }
  else if ((Index < Command.Length()) && (Command[Index + 1] == L'^'))
  {
    Len = 3;
  }
  else
  {
    Len = TFileCustomCommand::PatternLen(Command, Index);
  }
  return Len;
}
//---------------------------------------------------------------------------
bool __fastcall TLocalCustomCommand::PatternReplacement(
  int Index, const UnicodeString & Pattern, UnicodeString & Replacement, bool & Delimit)
{
  bool Result;
  if (Pattern == L"!\\")
  {
    // When used as "!\" in an argument to PowerShell, the trailing \ would escape the ",
    // so we exclude it
    Replacement = ExcludeTrailingBackslash(FLocalPath);
    Result = true;
  }
  else if (Pattern == L"!^!")
  {
    Replacement = FLocalFileName;
    Result = true;
  }
  else
  {
    Result = TFileCustomCommand::PatternReplacement(Index, Pattern, Replacement, Delimit);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::HasLocalFileName(const UnicodeString & Command)
{
  return FindPattern(Command, L'^');
}
//---------------------------------------------------------------------------
bool __fastcall TLocalCustomCommand::IsFileCommand(const UnicodeString & Command)
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}
//---------------------------------------------------------------------------
