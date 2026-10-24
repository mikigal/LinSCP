// The System.Classes unit: TPersistent, TList, TStrings, TStringList, streams.
//
// Delphi calls virtual methods of the most derived class even from destructors;
// C++ does not, so the destructors of the derived classes here do their own
// cleanup (e.g. TStringList frees owned objects itself).
//---------------------------------------------------------------------------
#ifndef RtlClassesH
#define RtlClassesH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
#include <rtl/Types.h>

#include <vector>
//---------------------------------------------------------------------------
namespace System
{
namespace Classes
{
//---------------------------------------------------------------------------
typedef TMethodPtr<void(TObject * Sender)> TNotifyEvent;

enum TShiftStateEnum { ssShift, ssAlt, ssCtrl, ssLeft, ssRight, ssMiddle, ssDouble, ssTouch, ssPen, ssCommand, ssHorizontal };
typedef Set<TShiftStateEnum, ssShift, ssHorizontal> TShiftState;
typedef Word TShortCut;

using System::Types::TDuplicates;
using System::Types::dupIgnore;
using System::Types::dupAccept;
using System::Types::dupError;

enum TSeekOrigin { soBeginning, soCurrent, soEnd };
const Word soFromBeginning = 0;
const Word soFromCurrent = 1;
const Word soFromEnd = 2;
const Word fmCreate = 0xFF00;

enum TListNotification { lnAdded, lnExtracted, lnDeleted };
enum TListAssignOp { laCopy, laAnd, laOr, laXor, laSrcUnique, laDestUnique };
//---------------------------------------------------------------------------
RTL_DECLARE_EXCEPTION(EStreamError, Exception)
RTL_DECLARE_EXCEPTION(EFileStreamError, EStreamError)
RTL_DECLARE_EXCEPTION(EFCreateError, EFileStreamError)
RTL_DECLARE_EXCEPTION(EFOpenError, EFileStreamError)
RTL_DECLARE_EXCEPTION(EFilerError, EStreamError)
RTL_DECLARE_EXCEPTION(EReadError, EFilerError)
RTL_DECLARE_EXCEPTION(EWriteError, EFilerError)
RTL_DECLARE_EXCEPTION(EClassNotFound, EFilerError)
RTL_DECLARE_EXCEPTION(EListError, Exception)
RTL_DECLARE_EXCEPTION(EStringListError, Exception)
RTL_DECLARE_EXCEPTION(EBitsError, Exception)
RTL_DECLARE_EXCEPTION(EInvalidOperation, Exception)
RTL_DECLARE_EXCEPTION(EComponentError, Exception)
RTL_DECLARE_EXCEPTION(EOutOfResources, Exception)
//---------------------------------------------------------------------------
class TPersistent : public TObject
{
public:
  virtual ~TPersistent() {}
  virtual void Assign(TPersistent * Source);
  virtual UnicodeString GetNamePath();

protected:
  virtual void AssignTo(TPersistent * Dest);
  void AssignError(TPersistent * Source);
};
//---------------------------------------------------------------------------
typedef int (*TListSortCompare)(void * Item1, void * Item2);
//---------------------------------------------------------------------------
class TList : public TObject
{
public:
  TList() {}
  virtual ~TList();

  int Add(void * Item);
  virtual void Clear();
  void Delete(int Index);
  [[noreturn]] static void Error(const UnicodeString Msg, NativeInt Data);
  void Exchange(int Index1, int Index2);
  TList * Expand();
  void * Extract(void * Item);
  void * First();
  int IndexOf(void * Item);
  void Insert(int Index, void * Item);
  void * Last();
  void Move(int CurIndex, int NewIndex);
  int Remove(void * Item);
  void Pack();
  void Sort(TListSortCompare Compare);
  void Assign(TList * ListA, TListAssignOp AOperator = laCopy, TList * ListB = nullptr);

  int GetCapacity() const { return static_cast<int>(FList.capacity()); }
  void SetCapacity(int NewCapacity);
  int GetCount() const { return static_cast<int>(FList.size()); }
  void SetCount(int NewCount);
  void * Get(int Index);
  void Put(int Index, void * Item);
  void ** GetList() { return FList.data(); }

  __declspec(property(get = GetCapacity, put = SetCapacity)) int Capacity;
  __declspec(property(get = GetCount, put = SetCount)) int Count;
  __declspec(property(get = Get, put = Put)) void * Items[];
  __declspec(property(get = GetList)) void ** List;
  void * operator [](int Index) { return Get(Index); }

protected:
  virtual void Notify(void * Ptr, TListNotification Action);

private:
  std::vector<void *> FList;
};
//---------------------------------------------------------------------------
class TStream;
class TStringList;
typedef int (*TStringListSortCompare)(TStringList * List, int Index1, int Index2);
//---------------------------------------------------------------------------
class TStrings : public TPersistent
{
public:
  TStrings();
  virtual ~TStrings() {}

  virtual int Add(const UnicodeString S);
  virtual int AddObject(const UnicodeString S, TObject * AObject);
  TStrings * AddPair(const UnicodeString Name, const UnicodeString Value);
  void Append(const UnicodeString S);
  virtual void AddStrings(TStrings * Strings);
  virtual void Assign(TPersistent * Source) override;
  void BeginUpdate();
  void EndUpdate();
  virtual void Clear() = 0;
  virtual void Delete(int Index) = 0;
  bool Equals(TStrings * Strings);
  using TObject::Equals;
  virtual void Exchange(int Index1, int Index2);
  virtual int IndexOf(const UnicodeString S);
  virtual int IndexOfName(const UnicodeString Name);
  virtual int IndexOfObject(TObject * AObject);
  virtual void Insert(int Index, const UnicodeString S) = 0;
  virtual void InsertObject(int Index, const UnicodeString S, TObject * AObject);
  virtual void LoadFromFile(const UnicodeString FileName);
  virtual void LoadFromFile(const UnicodeString FileName, TEncoding * Encoding);
  virtual void LoadFromStream(TStream * Stream);
  virtual void LoadFromStream(TStream * Stream, TEncoding * Encoding);
  virtual void Move(int CurIndex, int NewIndex);
  virtual void SaveToFile(const UnicodeString FileName);
  virtual void SaveToFile(const UnicodeString FileName, TEncoding * Encoding);
  virtual void SaveToStream(TStream * Stream);
  virtual void SaveToStream(TStream * Stream, TEncoding * Encoding);
  virtual void SetText(const wchar_t * Text);
  TStringDynArray ToStringArray();

  // Accessors of the properties below
  virtual UnicodeString Get(int Index) = 0;
  virtual int GetCount() = 0;
  virtual TObject * GetObject(int Index);
  virtual void Put(int Index, const UnicodeString S);
  virtual void PutObject(int Index, TObject * AObject);
  virtual int GetCapacity();
  virtual void SetCapacity(int NewCapacity);
  virtual UnicodeString GetTextStr();
  virtual void SetTextStr(const UnicodeString Value);
  UnicodeString GetCommaText();
  void SetCommaText(const UnicodeString Value);
  UnicodeString GetDelimitedText();
  void SetDelimitedText(const UnicodeString Value);
  UnicodeString GetName(int Index);
  UnicodeString GetValue(const UnicodeString Name);
  void SetValue(const UnicodeString Name, const UnicodeString Value);
  UnicodeString GetValueFromIndex(int Index);
  void SetValueFromIndex(int Index, const UnicodeString Value);
  wchar_t GetDelimiter() const { return FDelimiter; }
  void SetDelimiter(wchar_t Value) { FDelimiter = Value; }
  wchar_t GetQuoteChar() const { return FQuoteChar; }
  void SetQuoteChar(wchar_t Value) { FQuoteChar = Value; }
  wchar_t GetNameValueSeparator() const { return FNameValueSeparator; }
  void SetNameValueSeparator(wchar_t Value) { FNameValueSeparator = Value; }
  bool GetStrictDelimiter() const { return FStrictDelimiter; }
  void SetStrictDelimiter(bool Value) { FStrictDelimiter = Value; }
  UnicodeString GetLineBreak() const { return FLineBreak; }
  void SetLineBreak(const UnicodeString Value) { FLineBreak = Value; }
  bool GetTrailingLineBreak() const { return FTrailingLineBreak; }
  void SetTrailingLineBreak(bool Value) { FTrailingLineBreak = Value; }
  bool GetWriteBOM() const { return FWriteBOM; }
  void SetWriteBOM(bool Value) { FWriteBOM = Value; }
  TEncoding * GetEncoding() const { return FEncoding; }
  TEncoding * GetDefaultEncoding() const { return FDefaultEncoding; }
  void SetDefaultEncoding(TEncoding * Value) { FDefaultEncoding = (Value != nullptr) ? Value : TEncoding::Default; }
  int GetUpdateCount() const { return FUpdateCount; }

  __declspec(property(get = GetCapacity, put = SetCapacity)) int Capacity;
  __declspec(property(get = GetCommaText, put = SetCommaText)) UnicodeString CommaText;
  __declspec(property(get = GetCount)) int Count;
  __declspec(property(get = GetDelimiter, put = SetDelimiter)) wchar_t Delimiter;
  __declspec(property(get = GetDelimitedText, put = SetDelimitedText)) UnicodeString DelimitedText;
  __declspec(property(get = GetEncoding)) TEncoding * Encoding;
  __declspec(property(get = GetDefaultEncoding, put = SetDefaultEncoding)) TEncoding * DefaultEncoding;
  __declspec(property(get = GetLineBreak, put = SetLineBreak)) UnicodeString LineBreak;
  __declspec(property(get = GetName)) UnicodeString Names[];
  __declspec(property(get = GetNameValueSeparator, put = SetNameValueSeparator)) wchar_t NameValueSeparator;
  __declspec(property(get = GetObject, put = PutObject)) TObject * Objects[];
  __declspec(property(get = GetQuoteChar, put = SetQuoteChar)) wchar_t QuoteChar;
  __declspec(property(get = GetStrictDelimiter, put = SetStrictDelimiter)) bool StrictDelimiter;
  __declspec(property(get = Get, put = Put)) UnicodeString Strings[];
  __declspec(property(get = GetTextStr, put = SetTextStr)) UnicodeString Text;
  __declspec(property(get = GetTrailingLineBreak, put = SetTrailingLineBreak)) bool TrailingLineBreak;
  __declspec(property(get = GetUpdateCount)) int UpdateCount;
  __declspec(property(get = GetValue, put = SetValue)) UnicodeString Values[];
  __declspec(property(get = GetValueFromIndex, put = SetValueFromIndex)) UnicodeString ValueFromIndex[];
  __declspec(property(get = GetWriteBOM, put = SetWriteBOM)) bool WriteBOM;
  UnicodeString operator [](int Index) { return Get(Index); }

protected:
  virtual void SetUpdateState(bool Updating);
  virtual int CompareStrings(const UnicodeString S1, const UnicodeString S2);
  [[noreturn]] void Error(const UnicodeString Msg, int Data);
  UnicodeString ExtractName(const UnicodeString S);

private:
  wchar_t FDelimiter;
  wchar_t FQuoteChar;
  wchar_t FNameValueSeparator;
  bool FStrictDelimiter;
  bool FTrailingLineBreak;
  bool FWriteBOM;
  UnicodeString FLineBreak;
  TEncoding * FEncoding;
  TEncoding * FDefaultEncoding;
  int FUpdateCount;
};
//---------------------------------------------------------------------------
class TStringList : public TStrings
{
public:
  TStringList();
  explicit TStringList(bool OwnsObjects);
  TStringList(wchar_t QuoteChar, wchar_t Delimiter);
  TStringList(TDuplicates Duplicates, bool Sorted, bool CaseSensitive);
  virtual ~TStringList();

  virtual int Add(const UnicodeString S) override;
  virtual int AddObject(const UnicodeString S, TObject * AObject) override;
  virtual void Assign(TPersistent * Source) override;
  virtual void Clear() override;
  virtual void Delete(int Index) override;
  virtual void Exchange(int Index1, int Index2) override;
  virtual bool Find(const UnicodeString S, int & Index);
  virtual int IndexOf(const UnicodeString S) override;
  virtual void Insert(int Index, const UnicodeString S) override;
  virtual void InsertObject(int Index, const UnicodeString S, TObject * AObject) override;
  virtual void Sort();
  virtual void CustomSort(TStringListSortCompare Compare);

  virtual UnicodeString Get(int Index) override;
  virtual int GetCount() override { return static_cast<int>(FList.size()); }
  virtual TObject * GetObject(int Index) override;
  virtual void Put(int Index, const UnicodeString S) override;
  virtual void PutObject(int Index, TObject * AObject) override;
  virtual int GetCapacity() override { return static_cast<int>(FList.capacity()); }
  virtual void SetCapacity(int NewCapacity) override;
  bool GetSorted() const { return FSorted; }
  void SetSorted(bool Value);
  TDuplicates GetDuplicates() const { return FDuplicates; }
  void SetDuplicates(TDuplicates Value) { FDuplicates = Value; }
  bool GetCaseSensitive() const { return FCaseSensitive; }
  void SetCaseSensitive(bool Value);
  bool GetOwnsObjects() const { return FOwnsObjects; }
  void SetOwnsObjects(bool Value) { FOwnsObjects = Value; }
  TNotifyEvent GetOnChange() const { return FOnChange; }
  void SetOnChange(TNotifyEvent Value) { FOnChange = Value; }
  TNotifyEvent GetOnChanging() const { return FOnChanging; }
  void SetOnChanging(TNotifyEvent Value) { FOnChanging = Value; }

  __declspec(property(get = GetSorted, put = SetSorted)) bool Sorted;
  __declspec(property(get = GetDuplicates, put = SetDuplicates)) TDuplicates Duplicates;
  __declspec(property(get = GetCaseSensitive, put = SetCaseSensitive)) bool CaseSensitive;
  __declspec(property(get = GetOwnsObjects, put = SetOwnsObjects)) bool OwnsObjects;
  __declspec(property(get = GetOnChange, put = SetOnChange)) TNotifyEvent OnChange;
  __declspec(property(get = GetOnChanging, put = SetOnChanging)) TNotifyEvent OnChanging;

protected:
  virtual void Changed();
  virtual void Changing();
  virtual void SetUpdateState(bool Updating) override;
  virtual int CompareStrings(const UnicodeString S1, const UnicodeString S2) override;
  virtual void InsertItem(int Index, const UnicodeString S, TObject * AObject);

private:
  struct TStringItem
  {
    UnicodeString FString;
    TObject * FObject;
  };

  static int DefaultCompare(TStringList * List, int Index1, int Index2);
  void CheckIndex(int Index);
  void ExchangeItems(int Index1, int Index2);
  void QuickSort(int L, int R, TStringListSortCompare SCompare);
  void ClearItems();

  std::vector<TStringItem> FList;
  bool FSorted = false;
  TDuplicates FDuplicates = dupIgnore;
  bool FCaseSensitive = false;
  bool FOwnsObjects = false;
  TNotifyEvent FOnChange;
  TNotifyEvent FOnChanging;
};
//---------------------------------------------------------------------------
// Streams
//---------------------------------------------------------------------------
class TStream : public TObject
{
public:
  virtual ~TStream() {}

  virtual int Read(void * Buffer, int Count);
  virtual int Write(const void * Buffer, int Count);
  virtual int Read(DynamicArray<Byte> Buffer, int Offset, int Count);
  virtual int Write(const DynamicArray<Byte> Buffer, int Offset, int Count);
  int Read(DynamicArray<Byte> & Buffer, int Count);
  int Write(const DynamicArray<Byte> Buffer, int Count);
  virtual int Seek(int Offset, Word Origin);
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin);
  __int64 Seek(const __int64 Offset, Word Origin);
  void ReadBuffer(void * Buffer, int Count);
  void ReadBuffer(DynamicArray<Byte> & Buffer, int Count);
  void ReadBuffer(DynamicArray<Byte> & Buffer, int Offset, int Count);
  void WriteBuffer(const void * Buffer, int Count);
  void WriteBuffer(const DynamicArray<Byte> Buffer, int Count);
  void WriteBuffer(const DynamicArray<Byte> Buffer, int Offset, int Count);
  __int64 CopyFrom(TStream * Source, __int64 Count, int BufferSize = 0);

  __int64 GetPosition() { return Seek(static_cast<__int64>(0), soCurrent); }
  void SetPosition(const __int64 Pos) { Seek(Pos, soBeginning); }
  virtual __int64 GetSize();
  void SetSize64(const __int64 NewSize) { SetSize(NewSize); }
  __declspec(property(get = GetPosition, put = SetPosition)) __int64 Position;
  __declspec(property(get = GetSize, put = SetSize64)) __int64 Size;

  virtual void SetSize(int NewSize);
  virtual void SetSize(const __int64 NewSize);
};
//---------------------------------------------------------------------------
class THandleStream : public TStream
{
public:
  explicit THandleStream(THandle AHandle) : FHandle(AHandle) {}
  using TStream::Read;
  using TStream::Write;
  using TStream::Seek;
  using TStream::SetSize;
  virtual int Read(void * Buffer, int Count) override;
  virtual int Write(const void * Buffer, int Count) override;
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin) override;
  virtual void SetSize(const __int64 NewSize) override;

  THandle GetHandle() const { return FHandle; }
  __declspec(property(get = GetHandle)) THandle Handle;

protected:
  THandle FHandle;
};
//---------------------------------------------------------------------------
class TFileStream : public THandleStream
{
public:
  TFileStream(const UnicodeString AFileName, Word Mode);
  TFileStream(const UnicodeString AFileName, Word Mode, unsigned int Rights);
  virtual ~TFileStream();

  UnicodeString GetFileName() const { return FFileName; }
  __declspec(property(get = GetFileName)) UnicodeString FileName;

private:
  UnicodeString FFileName;
};
//---------------------------------------------------------------------------
class TCustomMemoryStream : public TStream
{
public:
  using TStream::Read;
  using TStream::Seek;
  virtual int Read(void * Buffer, int Count) override;
  virtual __int64 Seek(const __int64 Offset, TSeekOrigin Origin) override;
  void SaveToStream(TStream * Stream);
  void SaveToFile(const UnicodeString FileName);

  void * GetMemory() const { return FMemory; }
  __declspec(property(get = GetMemory)) void * Memory;

protected:
  void SetPointer(void * Ptr, NativeInt Size);

  void * FMemory = nullptr;
  NativeInt FSize = 0;
  NativeInt FPosition = 0;
};
//---------------------------------------------------------------------------
class TMemoryStream : public TCustomMemoryStream
{
public:
  TMemoryStream() {}
  virtual ~TMemoryStream();
  using TStream::Write;
  using TStream::SetSize;
  void Clear();
  void LoadFromStream(TStream * Stream);
  void LoadFromFile(const UnicodeString FileName);
  virtual void SetSize(const __int64 NewSize) override;
  virtual int Write(const void * Buffer, int Count) override;

  NativeInt GetCapacity() const { return FCapacity; }
  void SetCapacity(NativeInt NewCapacity);
  __declspec(property(get = GetCapacity, put = SetCapacity)) NativeInt Capacity;

protected:
  virtual void * Realloc(NativeInt & NewCapacity);

private:
  NativeInt FCapacity = 0;
};
//---------------------------------------------------------------------------
class TBytesStream : public TMemoryStream
{
public:
  explicit TBytesStream(const TBytes & ABytes = TBytes());
  TBytes GetBytes();
  __declspec(property(get = GetBytes)) TBytes Bytes;
};
//---------------------------------------------------------------------------
} // namespace Classes
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Classes;
//---------------------------------------------------------------------------
#endif
