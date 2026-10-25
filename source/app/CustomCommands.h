// The custom commands (upstream's TCustomCommandType and TCustomCommandList
// of windows/WinConfiguration.h, TLocalCustomCommand of windows/GUITools.h).
// The extensions (.WinSCPextension files, PowerShell scripts with options)
// are not supported.
//---------------------------------------------------------------------------
#ifndef CustomCommandsH
#define CustomCommandsH
//---------------------------------------------------------------------------
#include <FileMasks.h>
//---------------------------------------------------------------------------
class THierarchicalStorage;
class TShortCuts;
//---------------------------------------------------------------------------
class TCustomCommandType
{
public:
  TCustomCommandType();
  TCustomCommandType(const TCustomCommandType & Other);

  TCustomCommandType & operator=(const TCustomCommandType & Other);
  bool Equals(const TCustomCommandType * Other) const;

  UnicodeString GetName() const { return FName; }
  void SetName(const UnicodeString & Value) { FName = Value; }
  UnicodeString GetCommand() const { return FCommand; }
  void SetCommand(const UnicodeString & Value) { FCommand = Value; }
  int GetParams() const { return FParams; }
  void SetParams(int Value) { FParams = Value; }
  TShortCut GetShortCut() const { return FShortCut; }
  void SetShortCut(TShortCut Value) { FShortCut = Value; }

  __declspec(property(get = GetName, put = SetName)) UnicodeString Name;
  __declspec(property(get = GetCommand, put = SetCommand)) UnicodeString Command;
  __declspec(property(get = GetParams, put = SetParams)) int Params;
  __declspec(property(get = GetShortCut, put = SetShortCut)) TShortCut ShortCut;

private:
  UnicodeString FName;
  UnicodeString FCommand;
  int FParams;
  TShortCut FShortCut;
};
//---------------------------------------------------------------------------
class TCustomCommandList
{
public:
  TCustomCommandList();
  ~TCustomCommandList();

  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage);
  void Reset();
  void Modify();

  void Clear();
  void Add(const UnicodeString & Name, const UnicodeString & Command, int Params);
  void Add(TCustomCommandType * Command);
  void Insert(int Index, TCustomCommandType * Command);
  void Change(int Index, TCustomCommandType * Command);
  void Move(int CurIndex, int NewIndex);
  void Delete(int Index);

  const TCustomCommandType * Find(const UnicodeString & Name) const;
  const TCustomCommandType * Find(TShortCut ShortCut) const;

  bool Equals(const TCustomCommandList * Other) const;
  void Assign(const TCustomCommandList * Other);

  void ShortCuts(TShortCuts & ShortCuts) const;

  bool GetModified() const { return FModified; }
  int GetCount() const;
  const TCustomCommandType * GetCommand(int Index) const;

  __declspec(property(get = GetModified)) bool Modified;
  __declspec(property(get = GetCount)) int Count;
  __declspec(property(get = GetCommand)) const TCustomCommandType * Commands[];

private:
  bool FModified;
  std::vector<TCustomCommandType *> FCommands;
};
//---------------------------------------------------------------------------
// The patterns of the local commands: !\ is the local directory, !^! the
// file of the local panel
class TLocalCustomCommand : public TFileCustomCommand
{
public:
  TLocalCustomCommand();
  TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath);
  TLocalCustomCommand(
    const TCustomCommandData & Data, const UnicodeString & RemotePath, const UnicodeString & LocalPath,
    const UnicodeString & FileName, const UnicodeString & LocalFileName,
    const UnicodeString & FileList);

  virtual bool __fastcall IsFileCommand(const UnicodeString & Command);
  bool HasLocalFileName(const UnicodeString & Command);

protected:
  virtual int __fastcall PatternLen(const UnicodeString & Command, int Index);
  virtual bool __fastcall PatternReplacement(int Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit);
  virtual void __fastcall DelimitReplacement(UnicodeString & Replacement, wchar_t Quote);

private:
  UnicodeString FLocalPath;
  UnicodeString FLocalFileName;
};
//---------------------------------------------------------------------------
#endif
