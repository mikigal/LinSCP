// The editors of files.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <HierarchicalStorage.h>

#include "EditorPreferences.h"
//---------------------------------------------------------------------------
TEditorData::TEditorData() :
  FileMask(AnyMask),
  Editor(edInternal),
  ExternalEditor(L""),
  ExternalEditorText(false),
  SDIExternalEditor(false),
  DetectMDIExternalEditor(false)
{
}
//---------------------------------------------------------------------------
TEditorData::TEditorData(const TEditorData & Source) :
  FileMask(Source.FileMask),
  Editor(Source.Editor),
  ExternalEditor(Source.ExternalEditor),
  ExternalEditorText(Source.ExternalEditorText),
  SDIExternalEditor(Source.SDIExternalEditor),
  DetectMDIExternalEditor(Source.DetectMDIExternalEditor)
{
}
//---------------------------------------------------------------------------
#define C(Property) (Property == rhd.Property)
bool TEditorData::operator==(const TEditorData & rhd) const
{
  return
    C(FileMask) &&
    C(Editor) &&
    C(ExternalEditor) &&
    C(ExternalEditorText) &&
    C(SDIExternalEditor) &&
    C(DetectMDIExternalEditor) &&
    true;
}
#undef C
//---------------------------------------------------------------------------
TEditorPreferences::TEditorPreferences()
{
}
//---------------------------------------------------------------------------
TEditorPreferences::TEditorPreferences(const TEditorData & Data) :
  FData(Data)
{
}
//---------------------------------------------------------------------------
bool TEditorPreferences::operator==(const TEditorPreferences & rhp) const
{
  return (FData == rhp.FData);
}
//---------------------------------------------------------------------------
bool TEditorPreferences::Matches(const UnicodeString & FileName,
  bool Local, const TFileMasks::TParams & Params) const
{
  return FData.FileMask.Matches(FileName, Local, false, &Params);
}
//---------------------------------------------------------------------------
UnicodeString TEditorPreferences::ExtractExternalEditorName() const
{
  DebugAssert(FData.Editor == edExternal);
  UnicodeString ExternalEditor = FData.ExternalEditor;
  ReformatFileNameCommand(ExternalEditor);
  // Trim is a workaround for unknown problem with "notepad  " (2 trailing spaces)
  return ExtractProgramName(ExternalEditor).Trim();
}
//---------------------------------------------------------------------------
void TEditorPreferences::Load(THierarchicalStorage * Storage)
{
  FData.FileMask = Storage->ReadString(L"FileMask", FData.FileMask.Masks);
  FData.Editor = static_cast<TEditor>(Storage->ReadInteger(L"Editor", FData.Editor));
  FData.ExternalEditor = Storage->ReadString(L"ExternalEditor", FData.ExternalEditor);
  FData.ExternalEditorText = Storage->ReadBool(L"ExternalEditorText", FData.ExternalEditorText);
  FData.SDIExternalEditor = Storage->ReadBool(L"SDIExternalEditor", FData.SDIExternalEditor);
  FData.DetectMDIExternalEditor = Storage->ReadBool(L"DetectMDIExternalEditor", FData.DetectMDIExternalEditor);
}
//---------------------------------------------------------------------------
void TEditorPreferences::Save(THierarchicalStorage * Storage) const
{
  Storage->WriteString(L"FileMask", FData.FileMask.Masks);
  Storage->WriteInteger(L"Editor", FData.Editor);
  Storage->WriteString(L"ExternalEditor", FData.ExternalEditor);
  Storage->WriteBool(L"ExternalEditorText", FData.ExternalEditorText);
  Storage->WriteBool(L"SDIExternalEditor", FData.SDIExternalEditor);
  Storage->WriteBool(L"DetectMDIExternalEditor", FData.DetectMDIExternalEditor);
}
//---------------------------------------------------------------------------
TEditorData * TEditorPreferences::GetData()
{
  // returning non-const data, possible data change, invalidate cached name
  FName = L"";
  return &FData;
}
//---------------------------------------------------------------------------
UnicodeString TEditorPreferences::GetName() const
{
  if (FName.IsEmpty())
  {
    if (FData.Editor == edInternal)
    {
      // StripHotkey is relic from times when INTERNAL_EDITOR_NAME was used
      // also for the menu item caption
      FName = StripHotkey(LoadStr(INTERNAL_EDITOR_NAME));
    }
    else if (FData.Editor == edOpen)
    {
      FName = StripHotkey(LoadStr(OPEN_EDITOR_NAME));
    }
    else
    {
      UnicodeString Program, Params, Dir;
      UnicodeString ExternalEditor = FData.ExternalEditor;
      ReformatFileNameCommand(ExternalEditor);
      SplitCommand(ExternalEditor, Program, Params, Dir);
      FName = ExtractFileName(Program);
      int P = FName.LastDelimiter(L".");
      if (P > 0)
      {
        FName.SetLength(P - 1);
      }

      if (FName.UpperCase() == FName)
      {
        FName = FName.LowerCase();
      }

      if (FName.LowerCase() == FName)
      {
        FName = FName.SubString(1, 1).UpperCase() +
          FName.SubString(2, FName.Length() - 1);
      }
    }
  }

  return FName;
}
//---------------------------------------------------------------------------
TEditorList::TEditorList()
{
  FModified = false;
}
//---------------------------------------------------------------------------
TEditorList::~TEditorList()
{
  Clear();
}
//---------------------------------------------------------------------------
void TEditorList::Modify()
{
  FModified = true;
}
//---------------------------------------------------------------------------
void TEditorList::Saved()
{
  FModified = false;
}
//---------------------------------------------------------------------------
TEditorList & TEditorList::operator=(const TEditorList & rhl)
{
  Clear();

  for (int Index = 0; Index < rhl.Count; Index++)
  {
    Add(new TEditorPreferences(*rhl.Editors[Index]));
  }
  // there should be comparison of with the assigned list, but we rely on caller
  // to do it instead (TAppConfiguration::SetEditorList)
  Modify();
  return *this;
}
//---------------------------------------------------------------------------
bool TEditorList::operator==(const TEditorList & rhl) const
{
  bool Result = (Count == rhl.Count);
  for (int Index = 0; Result && (Index < Count); Index++)
  {
    Result = (*Editors[Index]) == (*rhl.Editors[Index]);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TEditorList::Clear()
{
  for (TEditorPreferences * Editor : FEditors)
  {
    delete Editor;
  }
  FEditors.clear();
}
//---------------------------------------------------------------------------
void TEditorList::Add(TEditorPreferences * Editor)
{
  Insert(Count, Editor);
}
//---------------------------------------------------------------------------
void TEditorList::Insert(int Index, TEditorPreferences * Editor)
{
  FEditors.insert(FEditors.begin() + Index, Editor);
  Modify();
}
//---------------------------------------------------------------------------
void TEditorList::Change(int Index, TEditorPreferences * Editor)
{
  if (!((*Editors[Index]) == *Editor))
  {
    delete FEditors[Index];
    FEditors[Index] = Editor;
    Modify();
  }
  else
  {
    delete Editor;
  }
}
//---------------------------------------------------------------------------
void TEditorList::Move(int CurIndex, int NewIndex)
{
  if (CurIndex != NewIndex)
  {
    TEditorPreferences * Editor = FEditors[CurIndex];
    FEditors.erase(FEditors.begin() + CurIndex);
    FEditors.insert(FEditors.begin() + NewIndex, Editor);
    Modify();
  }
}
//---------------------------------------------------------------------------
void TEditorList::Delete(int Index)
{
  DebugAssert((Index >= 0) && (Index < Count));
  delete FEditors[Index];
  FEditors.erase(FEditors.begin() + Index);
  Modify();
}
//---------------------------------------------------------------------------
const TEditorPreferences * TEditorList::Find(
  const UnicodeString & FileName, bool Local, const TFileMasks::TParams & Params) const
{
  for (const TEditorPreferences * Editor : FEditors)
  {
    if (Editor->Matches(FileName, Local, Params))
    {
      return Editor;
    }
  }
  return NULL;
}
//---------------------------------------------------------------------------
void TEditorList::Load(THierarchicalStorage * Storage)
{
  int Index = 0;
  bool Next;

  do
  {
    UnicodeString Name = IntToStr(Index);
    Next = Storage->OpenSubKey(Name, false);
    if (Next)
    {
      std::unique_ptr<TEditorPreferences> Editor(new TEditorPreferences());
      {
        ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
        Editor->Load(Storage);
      }
      FEditors.push_back(Editor.release());
    }

    Index++;
  }
  while (Next);

  FModified = false;
}
//---------------------------------------------------------------------------
void TEditorList::Save(THierarchicalStorage * Storage) const
{
  Storage->ClearSubKeys();
  for (int Index = 0; Index < Count; Index++)
  {
    if (Storage->OpenSubKey(IntToStr(Index), true))
    {
      ::rtl::TFinally Finally([&]() { Storage->CloseSubKey(); });
      Editors[Index]->Save(Storage);
    }
  }
}
//---------------------------------------------------------------------------
