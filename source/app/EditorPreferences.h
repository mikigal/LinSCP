// The editors of files (upstream's TEditorData, TEditorPreferences and
// TEditorList of windows/WinConfiguration.h): which editor (the internal
// one, an external program or the application of the desktop) edits which
// files.
//---------------------------------------------------------------------------
#ifndef EditorPreferencesH
#define EditorPreferencesH
//---------------------------------------------------------------------------
#include <FileMasks.h>

#include <vector>
//---------------------------------------------------------------------------
class THierarchicalStorage;
//---------------------------------------------------------------------------
enum TEditor { edInternal, edExternal, edOpen };
//---------------------------------------------------------------------------
struct TEditorData
{
  TEditorData();
  TEditorData(const TEditorData & Source);
  TEditorData & operator =(const TEditorData &) = default;

  TFileMasks FileMask;
  TEditor Editor;
  UnicodeString ExternalEditor;
  bool ExternalEditorText;
  bool SDIExternalEditor;
  bool DetectMDIExternalEditor;

  bool operator ==(const TEditorData & rhd) const;
};
//---------------------------------------------------------------------------
class TEditorPreferences
{
public:
  TEditorPreferences();
  TEditorPreferences(const TEditorData & Data);
  bool Matches(const UnicodeString & FileName, bool Local, const TFileMasks::TParams & Params) const;
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString ExtractExternalEditorName() const;

  bool operator ==(const TEditorPreferences & rhp) const;

  const TEditorData * GetConstData() const { return &FData; }
  TEditorData * GetData();
  UnicodeString GetName() const;

  __declspec(property(get = GetConstData)) const TEditorData * Data;
  __declspec(property(get = GetName)) UnicodeString Name;

private:
  TEditorData FData;
  mutable UnicodeString FName;
};
//---------------------------------------------------------------------------
class TEditorList
{
public:
  TEditorList();
  ~TEditorList();

  const TEditorPreferences * Find(const UnicodeString & FileName,
    bool Local, const TFileMasks::TParams & Params) const;

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;

  TEditorList & operator=(const TEditorList & rhl);
  bool operator==(const TEditorList & rhl) const;

  void Clear();
  void Add(TEditorPreferences * Editor);
  void Insert(int Index, TEditorPreferences * Editor);
  void Change(int Index, TEditorPreferences * Editor);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);
  void Saved();

  int GetCount() const { return static_cast<int>(FEditors.size()); }
  const TEditorPreferences * GetEditor(int Index) const { return FEditors[Index]; }
  bool GetModified() const { return FModified; }

  __declspec(property(get = GetCount)) int Count;
  __declspec(property(get = GetEditor)) const TEditorPreferences * Editors[];
  __declspec(property(get = GetModified)) bool Modified;

private:
  std::vector<TEditorPreferences *> FEditors;
  bool FModified;

  void Modify();
};
//---------------------------------------------------------------------------
#endif
