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

  RTL_PROPERTY_GET(Capacity, GetCapacity) RTL_PROPERTY_PUT(Capacity, SetCapacity, int)
  __declspec(property(get = __property_get_Capacity, put = __property_set_Capacity)) int Capacity;
  RTL_PROPERTY_GET(Count, GetCount) RTL_PROPERTY_PUT(Count, SetCount, int)
  __declspec(property(get = __property_get_Count, put = __property_set_Count)) int Count;
  RTL_PROPERTY_GET(Items, Get) RTL_PROPERTY_PUT_INDEXED(Items, Put, int, void *)
  __declspec(property(get = __property_get_Items, put = __property_set_Items)) void * Items[];
  RTL_PROPERTY_GET(List, GetList)
  __declspec(property(get = __property_get_List)) void ** List;
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

  RTL_PROPERTY_GET(Capacity, GetCapacity) RTL_PROPERTY_PUT(Capacity, SetCapacity, int)
  __declspec(property(get = __property_get_Capacity, put = __property_set_Capacity)) int Capacity;
  RTL_PROPERTY_GET(CommaText, GetCommaText) RTL_PROPERTY_PUT(CommaText, SetCommaText, UnicodeString)
  __declspec(property(get = __property_get_CommaText, put = __property_set_CommaText)) UnicodeString CommaText;
  RTL_PROPERTY_GET(Count, GetCount)
  __declspec(property(get = __property_get_Count)) int Count;
  RTL_PROPERTY_GET(Delimiter, GetDelimiter) RTL_PROPERTY_PUT(Delimiter, SetDelimiter, wchar_t)
  __declspec(property(get = __property_get_Delimiter, put = __property_set_Delimiter)) wchar_t Delimiter;
  RTL_PROPERTY_GET(DelimitedText, GetDelimitedText) RTL_PROPERTY_PUT(DelimitedText, SetDelimitedText, UnicodeString)
  __declspec(property(get = __property_get_DelimitedText, put = __property_set_DelimitedText)) UnicodeString DelimitedText;
  RTL_PROPERTY_GET(Encoding, GetEncoding)
  __declspec(property(get = __property_get_Encoding)) TEncoding * Encoding;
  RTL_PROPERTY_GET(DefaultEncoding, GetDefaultEncoding) RTL_PROPERTY_PUT(DefaultEncoding, SetDefaultEncoding, TEncoding *)
  __declspec(property(get = __property_get_DefaultEncoding, put = __property_set_DefaultEncoding)) TEncoding * DefaultEncoding;
  RTL_PROPERTY_GET(LineBreak, GetLineBreak) RTL_PROPERTY_PUT(LineBreak, SetLineBreak, UnicodeString)
  __declspec(property(get = __property_get_LineBreak, put = __property_set_LineBreak)) UnicodeString LineBreak;
  RTL_PROPERTY_GET(Names, GetName)
  __declspec(property(get = __property_get_Names)) UnicodeString Names[];
  RTL_PROPERTY_GET(NameValueSeparator, GetNameValueSeparator) RTL_PROPERTY_PUT(NameValueSeparator, SetNameValueSeparator, wchar_t)
  __declspec(property(get = __property_get_NameValueSeparator, put = __property_set_NameValueSeparator)) wchar_t NameValueSeparator;
  RTL_PROPERTY_GET(Objects, GetObject) RTL_PROPERTY_PUT_INDEXED(Objects, PutObject, int, TObject *)
  __declspec(property(get = __property_get_Objects, put = __property_set_Objects)) TObject * Objects[];
  RTL_PROPERTY_GET(QuoteChar, GetQuoteChar) RTL_PROPERTY_PUT(QuoteChar, SetQuoteChar, wchar_t)
  __declspec(property(get = __property_get_QuoteChar, put = __property_set_QuoteChar)) wchar_t QuoteChar;
  RTL_PROPERTY_GET(StrictDelimiter, GetStrictDelimiter) RTL_PROPERTY_PUT(StrictDelimiter, SetStrictDelimiter, bool)
  __declspec(property(get = __property_get_StrictDelimiter, put = __property_set_StrictDelimiter)) bool StrictDelimiter;
  RTL_PROPERTY_GET(Strings, Get) RTL_PROPERTY_PUT_INDEXED(Strings, Put, int, UnicodeString)
  __declspec(property(get = __property_get_Strings, put = __property_set_Strings)) UnicodeString Strings[];
  RTL_PROPERTY_GET(Text, GetTextStr) RTL_PROPERTY_PUT(Text, SetTextStr, UnicodeString)
  __declspec(property(get = __property_get_Text, put = __property_set_Text)) UnicodeString Text;
  RTL_PROPERTY_GET(TrailingLineBreak, GetTrailingLineBreak) RTL_PROPERTY_PUT(TrailingLineBreak, SetTrailingLineBreak, bool)
  __declspec(property(get = __property_get_TrailingLineBreak, put = __property_set_TrailingLineBreak)) bool TrailingLineBreak;
  RTL_PROPERTY_GET(UpdateCount, GetUpdateCount)
  __declspec(property(get = __property_get_UpdateCount)) int UpdateCount;
  RTL_PROPERTY_GET(Values, GetValue) RTL_PROPERTY_PUT_INDEXED(Values, SetValue, const UnicodeString, UnicodeString)
  __declspec(property(get = __property_get_Values, put = __property_set_Values)) UnicodeString Values[];
  RTL_PROPERTY_GET(ValueFromIndex, GetValueFromIndex) RTL_PROPERTY_PUT_INDEXED(ValueFromIndex, SetValueFromIndex, int, UnicodeString)
  __declspec(property(get = __property_get_ValueFromIndex, put = __property_set_ValueFromIndex)) UnicodeString ValueFromIndex[];
  RTL_PROPERTY_GET(WriteBOM, GetWriteBOM) RTL_PROPERTY_PUT(WriteBOM, SetWriteBOM, bool)
  __declspec(property(get = __property_get_WriteBOM, put = __property_set_WriteBOM)) bool WriteBOM;
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

  RTL_PROPERTY_GET(Sorted, GetSorted) RTL_PROPERTY_PUT(Sorted, SetSorted, bool)
  __declspec(property(get = __property_get_Sorted, put = __property_set_Sorted)) bool Sorted;
  RTL_PROPERTY_GET(Duplicates, GetDuplicates) RTL_PROPERTY_PUT(Duplicates, SetDuplicates, TDuplicates)
  __declspec(property(get = __property_get_Duplicates, put = __property_set_Duplicates)) TDuplicates Duplicates;
  RTL_PROPERTY_GET(CaseSensitive, GetCaseSensitive) RTL_PROPERTY_PUT(CaseSensitive, SetCaseSensitive, bool)
  __declspec(property(get = __property_get_CaseSensitive, put = __property_set_CaseSensitive)) bool CaseSensitive;
  RTL_PROPERTY_GET(OwnsObjects, GetOwnsObjects) RTL_PROPERTY_PUT(OwnsObjects, SetOwnsObjects, bool)
  __declspec(property(get = __property_get_OwnsObjects, put = __property_set_OwnsObjects)) bool OwnsObjects;
  RTL_PROPERTY_GET(OnChange, GetOnChange) RTL_PROPERTY_PUT(OnChange, SetOnChange, TNotifyEvent)
  __declspec(property(get = __property_get_OnChange, put = __property_set_OnChange)) TNotifyEvent OnChange;
  RTL_PROPERTY_GET(OnChanging, GetOnChanging) RTL_PROPERTY_PUT(OnChanging, SetOnChanging, TNotifyEvent)
  __declspec(property(get = __property_get_OnChanging, put = __property_set_OnChanging)) TNotifyEvent OnChanging;

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
  RTL_PROPERTY_GET(Position, GetPosition) RTL_PROPERTY_PUT(Position, SetPosition, __int64)
  __declspec(property(get = __property_get_Position, put = __property_set_Position)) __int64 Position;
  RTL_PROPERTY_GET(Size, GetSize) RTL_PROPERTY_PUT(Size, SetSize64, __int64)
  __declspec(property(get = __property_get_Size, put = __property_set_Size)) __int64 Size;

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
  RTL_PROPERTY_GET(Handle, GetHandle)
  __declspec(property(get = __property_get_Handle)) THandle Handle;

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
  RTL_PROPERTY_GET(FileName, GetFileName)
  __declspec(property(get = __property_get_FileName)) UnicodeString FileName;

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
  RTL_PROPERTY_GET(Memory, GetMemory)
  __declspec(property(get = __property_get_Memory)) void * Memory;

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
  RTL_PROPERTY_GET(Capacity, GetCapacity) RTL_PROPERTY_PUT(Capacity, SetCapacity, NativeInt)
  __declspec(property(get = __property_get_Capacity, put = __property_set_Capacity)) NativeInt Capacity;

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
  RTL_PROPERTY_GET(Bytes, GetBytes)
  __declspec(property(get = __property_get_Bytes)) TBytes Bytes;
};
//---------------------------------------------------------------------------
} // namespace Classes
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Classes;
//---------------------------------------------------------------------------
#endif
