// System.Classes: TPersistent, TList, TStrings, TStringList
//---------------------------------------------------------------------------
#include <rtl/Classes.h>

#include <algorithm>
//---------------------------------------------------------------------------
namespace System
{
namespace Classes
{
//---------------------------------------------------------------------------
namespace
{
const wchar_t * const SListIndexError = L"List index out of bounds (%d)";
const wchar_t * const SListCapacityError = L"List capacity out of bounds (%d)";
const wchar_t * const SListCountError = L"List count out of bounds (%d)";
const wchar_t * const SDuplicateString = L"String list does not allow duplicates";
const wchar_t * const SSortedListError = L"Operation not allowed on sorted list";
} // namespace
//---------------------------------------------------------------------------
// TPersistent
//---------------------------------------------------------------------------
void TPersistent::Assign(TPersistent * Source)
{
  if (Source != nullptr)
  {
    Source->AssignTo(this);
  }
  else
  {
    AssignError(nullptr);
  }
}
//---------------------------------------------------------------------------
void TPersistent::AssignTo(TPersistent * Dest)
{
  Dest->AssignError(this);
}
//---------------------------------------------------------------------------
void TPersistent::AssignError(TPersistent * Source)
{
  UnicodeString SourceName = (Source != nullptr) ? Source->ClassName() : UnicodeString(L"nil");
  throw EConvertError(L"Cannot assign a %s to a %s", ARRAYOFCONST((SourceName, ClassName())));
}
//---------------------------------------------------------------------------
UnicodeString TPersistent::GetNamePath()
{
  return ClassName();
}
//---------------------------------------------------------------------------
// TComponent
//---------------------------------------------------------------------------
TComponent::TComponent(TComponent * AOwner) :
  FOwner(AOwner)
{
  if (FOwner != nullptr)
  {
    FOwner->FComponents.push_back(this);
  }
}
//---------------------------------------------------------------------------
TComponent::~TComponent()
{
  // The owned components remove themselves from the list
  while (!FComponents.empty())
  {
    delete FComponents.back();
  }
  if (FOwner != nullptr)
  {
    std::vector<TComponent *> & Siblings = FOwner->FComponents;
    Siblings.erase(std::find(Siblings.begin(), Siblings.end(), this));
  }
}
//---------------------------------------------------------------------------
TComponent * TComponent::GetComponent(int Index) const
{
  if ((Index < 0) || (Index >= ComponentCount))
  {
    throw EListError(SListIndexError, ARRAYOFCONST((Index)));
  }
  return FComponents[Index];
}
//---------------------------------------------------------------------------
// TList
//---------------------------------------------------------------------------
TList::~TList()
{
  Clear();
}
//---------------------------------------------------------------------------
void TList::Error(const UnicodeString Msg, NativeInt Data)
{
  throw EListError(Msg, ARRAYOFCONST((static_cast<__int64>(Data))));
}
//---------------------------------------------------------------------------
int TList::Add(void * Item)
{
  int Result = GetCount();
  FList.push_back(Item);
  if (Item != nullptr)
  {
    Notify(Item, lnAdded);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TList::Clear()
{
  SetCount(0);
  SetCapacity(0);
}
//---------------------------------------------------------------------------
void TList::Delete(int Index)
{
  if ((Index < 0) || (Index >= GetCount()))
  {
    Error(SListIndexError, Index);
  }
  void * Temp = FList[Index];
  FList.erase(FList.begin() + Index);
  if (Temp != nullptr)
  {
    Notify(Temp, lnDeleted);
  }
}
//---------------------------------------------------------------------------
void TList::Exchange(int Index1, int Index2)
{
  if ((Index1 < 0) || (Index1 >= GetCount()))
  {
    Error(SListIndexError, Index1);
  }
  if ((Index2 < 0) || (Index2 >= GetCount()))
  {
    Error(SListIndexError, Index2);
  }
  std::swap(FList[Index1], FList[Index2]);
}
//---------------------------------------------------------------------------
TList * TList::Expand()
{
  if (FList.size() == FList.capacity())
  {
    FList.reserve(FList.capacity() + ((FList.capacity() > 64) ? FList.capacity() / 4 : 16));
  }
  return this;
}
//---------------------------------------------------------------------------
void * TList::Extract(void * Item)
{
  int I = IndexOf(Item);
  if (I < 0)
  {
    return nullptr;
  }
  FList[I] = nullptr;
  Delete(I);
  Notify(Item, lnExtracted);
  return Item;
}
//---------------------------------------------------------------------------
void * TList::First()
{
  return Get(0);
}
//---------------------------------------------------------------------------
void * TList::Last()
{
  return Get(GetCount() - 1);
}
//---------------------------------------------------------------------------
int TList::IndexOf(void * Item)
{
  for (size_t Index = 0; Index < FList.size(); Index++)
  {
    if (FList[Index] == Item)
    {
      return static_cast<int>(Index);
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TList::Insert(int Index, void * Item)
{
  if ((Index < 0) || (Index > GetCount()))
  {
    Error(SListIndexError, Index);
  }
  FList.insert(FList.begin() + Index, Item);
  if (Item != nullptr)
  {
    Notify(Item, lnAdded);
  }
}
//---------------------------------------------------------------------------
void TList::Move(int CurIndex, int NewIndex)
{
  if (CurIndex != NewIndex)
  {
    if ((NewIndex < 0) || (NewIndex >= GetCount()))
    {
      Error(SListIndexError, NewIndex);
    }
    void * Item = Get(CurIndex);
    FList.erase(FList.begin() + CurIndex);
    FList.insert(FList.begin() + NewIndex, Item);
  }
}
//---------------------------------------------------------------------------
int TList::Remove(void * Item)
{
  int Result = IndexOf(Item);
  if (Result >= 0)
  {
    Delete(Result);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TList::Pack()
{
  for (int Index = GetCount() - 1; Index >= 0; Index--)
  {
    if (FList[Index] == nullptr)
    {
      FList.erase(FList.begin() + Index);
    }
  }
}
//---------------------------------------------------------------------------
void TList::Sort(TListSortCompare Compare)
{
  if (FList.size() < 2)
  {
    return;
  }
  // Delphi's quicksort, with guards against inconsistent comparisons
  std::function<void(int, int)> QuickSort = [&](int L, int R)
    {
      int I;
      do
      {
        I = L;
        int J = R;
        void * P = FList[(L + R) / 2];
        do
        {
          while ((I < R) && (Compare(FList[I], P) < 0))
          {
            I++;
          }
          while ((J > L) && (Compare(FList[J], P) > 0))
          {
            J--;
          }
          if (I <= J)
          {
            if (I != J)
            {
              std::swap(FList[I], FList[J]);
            }
            I++;
            J--;
          }
        }
        while (I <= J);
        if (L < J)
        {
          QuickSort(L, J);
        }
        L = I;
      }
      while (I < R);
    };
  QuickSort(0, GetCount() - 1);
}
//---------------------------------------------------------------------------
void TList::Assign(TList * ListA, TListAssignOp AOperator, TList * ListB)
{
  TList * Source = ListA;
  if (ListB != nullptr)
  {
    Assign(ListA, laCopy);
    Source = ListB;
  }
  switch (AOperator)
  {
    case laCopy:
      Clear();
      FList.reserve(Source->GetCount());
      for (int I = 0; I < Source->GetCount(); I++)
      {
        Add(Source->Get(I));
      }
      break;
    case laAnd:
      for (int I = GetCount() - 1; I >= 0; I--)
      {
        if (Source->IndexOf(Get(I)) < 0)
        {
          Delete(I);
        }
      }
      break;
    case laOr:
      for (int I = 0; I < Source->GetCount(); I++)
      {
        if (IndexOf(Source->Get(I)) < 0)
        {
          Add(Source->Get(I));
        }
      }
      break;
    case laXor:
      {
        TList Temp;
        for (int I = 0; I < Source->GetCount(); I++)
        {
          if (IndexOf(Source->Get(I)) < 0)
          {
            Temp.Add(Source->Get(I));
          }
        }
        for (int I = GetCount() - 1; I >= 0; I--)
        {
          if (Source->IndexOf(Get(I)) >= 0)
          {
            Delete(I);
          }
        }
        for (int I = 0; I < Temp.GetCount(); I++)
        {
          Add(Temp.Get(I));
        }
      }
      break;
    case laSrcUnique:
      for (int I = GetCount() - 1; I >= 0; I--)
      {
        if (Source->IndexOf(Get(I)) >= 0)
        {
          Delete(I);
        }
      }
      break;
    case laDestUnique:
      {
        TList Temp;
        for (int I = Source->GetCount() - 1; I >= 0; I--)
        {
          if (IndexOf(Source->Get(I)) < 0)
          {
            Temp.Add(Source->Get(I));
          }
        }
        Assign(&Temp);
      }
      break;
  }
}
//---------------------------------------------------------------------------
void TList::SetCapacity(int NewCapacity)
{
  if (NewCapacity < GetCount())
  {
    Error(SListCapacityError, NewCapacity);
  }
  if (NewCapacity == 0)
  {
    std::vector<void *>().swap(FList);
  }
  else
  {
    FList.reserve(NewCapacity);
  }
}
//---------------------------------------------------------------------------
void TList::SetCount(int NewCount)
{
  if (NewCount < 0)
  {
    Error(SListCountError, NewCount);
  }
  if (NewCount > GetCount())
  {
    FList.resize(NewCount, nullptr);
  }
  else
  {
    for (int Index = GetCount() - 1; Index >= NewCount; Index--)
    {
      Delete(Index);
    }
  }
}
//---------------------------------------------------------------------------
void * TList::Get(int Index)
{
  if ((Index < 0) || (Index >= GetCount()))
  {
    Error(SListIndexError, Index);
  }
  return FList[Index];
}
//---------------------------------------------------------------------------
void TList::Put(int Index, void * Item)
{
  if ((Index < 0) || (Index >= GetCount()))
  {
    Error(SListIndexError, Index);
  }
  if (Item != FList[Index])
  {
    void * Temp = FList[Index];
    FList[Index] = Item;
    if (Temp != nullptr)
    {
      Notify(Temp, lnDeleted);
    }
    if (Item != nullptr)
    {
      Notify(Item, lnAdded);
    }
  }
}
//---------------------------------------------------------------------------
void TList::Notify(void *, TListNotification)
{
}
//---------------------------------------------------------------------------
// TStrings
//---------------------------------------------------------------------------
TStrings::TStrings() :
  FDelimiter(L','),
  FQuoteChar(L'"'),
  FNameValueSeparator(L'='),
  FStrictDelimiter(false),
  FTrailingLineBreak(true),
  FWriteBOM(true),
  FLineBreak(sLineBreak),
  FEncoding(nullptr),
  FDefaultEncoding(TEncoding::Default),
  FUpdateCount(0)
{
}
//---------------------------------------------------------------------------
void TStrings::Error(const UnicodeString Msg, int Data)
{
  throw EStringListError(Msg, ARRAYOFCONST((Data)));
}
//---------------------------------------------------------------------------
int TStrings::Add(const UnicodeString S)
{
  int Result = GetCount();
  Insert(Result, S);
  return Result;
}
//---------------------------------------------------------------------------
int TStrings::AddObject(const UnicodeString S, TObject * AObject)
{
  int Result = Add(S);
  PutObject(Result, AObject);
  return Result;
}
//---------------------------------------------------------------------------
TStrings * TStrings::AddPair(const UnicodeString Name, const UnicodeString Value)
{
  Add(Name + FNameValueSeparator + Value);
  return this;
}
//---------------------------------------------------------------------------
void TStrings::Append(const UnicodeString S)
{
  Add(S);
}
//---------------------------------------------------------------------------
void TStrings::AddStrings(TStrings * Strings)
{
  BeginUpdate();
  try
  {
    for (int I = 0; I < Strings->GetCount(); I++)
    {
      AddObject(Strings->Get(I), Strings->GetObject(I));
    }
  }
  catch (...)
  {
    EndUpdate();
    throw;
  }
  EndUpdate();
}
//---------------------------------------------------------------------------
void TStrings::Assign(TPersistent * Source)
{
  TStrings * Strings = dynamic_cast<TStrings *>(Source);
  if (Strings == nullptr)
  {
    TPersistent::Assign(Source);
    return;
  }
  BeginUpdate();
  try
  {
    Clear();
    FDefaultEncoding = Strings->FDefaultEncoding;
    FEncoding = Strings->FEncoding;
    FLineBreak = Strings->FLineBreak;
    FDelimiter = Strings->FDelimiter;
    FQuoteChar = Strings->FQuoteChar;
    FNameValueSeparator = Strings->FNameValueSeparator;
    FStrictDelimiter = Strings->FStrictDelimiter;
    FTrailingLineBreak = Strings->FTrailingLineBreak;
    FWriteBOM = Strings->FWriteBOM;
    AddStrings(Strings);
  }
  catch (...)
  {
    EndUpdate();
    throw;
  }
  EndUpdate();
}
//---------------------------------------------------------------------------
void TStrings::BeginUpdate()
{
  if (FUpdateCount == 0)
  {
    SetUpdateState(true);
  }
  FUpdateCount++;
}
//---------------------------------------------------------------------------
void TStrings::EndUpdate()
{
  FUpdateCount--;
  if (FUpdateCount == 0)
  {
    SetUpdateState(false);
  }
}
//---------------------------------------------------------------------------
void TStrings::SetUpdateState(bool)
{
}
//---------------------------------------------------------------------------
bool TStrings::Equals(TStrings * Strings)
{
  int Count = GetCount();
  if (Count != Strings->GetCount())
  {
    return false;
  }
  for (int I = 0; I < Count; I++)
  {
    if (Get(I) != Strings->Get(I))
    {
      return false;
    }
  }
  return true;
}
//---------------------------------------------------------------------------
void TStrings::Exchange(int Index1, int Index2)
{
  BeginUpdate();
  try
  {
    UnicodeString TempString = Get(Index1);
    TObject * TempObject = GetObject(Index1);
    Put(Index1, Get(Index2));
    PutObject(Index1, GetObject(Index2));
    Put(Index2, TempString);
    PutObject(Index2, TempObject);
  }
  catch (...)
  {
    EndUpdate();
    throw;
  }
  EndUpdate();
}
//---------------------------------------------------------------------------
int TStrings::CompareStrings(const UnicodeString S1, const UnicodeString S2)
{
  return AnsiCompareText(S1, S2);
}
//---------------------------------------------------------------------------
int TStrings::IndexOf(const UnicodeString S)
{
  for (int I = 0; I < GetCount(); I++)
  {
    if (CompareStrings(Get(I), S) == 0)
    {
      return I;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
int TStrings::IndexOfName(const UnicodeString Name)
{
  for (int I = 0; I < GetCount(); I++)
  {
    UnicodeString S = Get(I);
    int P = S.Pos(FNameValueSeparator);
    if ((P != 0) && (CompareStrings(S.SubString(1, P - 1), Name) == 0))
    {
      return I;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
int TStrings::IndexOfObject(TObject * AObject)
{
  for (int I = 0; I < GetCount(); I++)
  {
    if (GetObject(I) == AObject)
    {
      return I;
    }
  }
  return -1;
}
//---------------------------------------------------------------------------
void TStrings::InsertObject(int Index, const UnicodeString S, TObject * AObject)
{
  Insert(Index, S);
  PutObject(Index, AObject);
}
//---------------------------------------------------------------------------
void TStrings::LoadFromFile(const UnicodeString FileName)
{
  LoadFromFile(FileName, nullptr);
}
//---------------------------------------------------------------------------
void TStrings::LoadFromFile(const UnicodeString FileName, TEncoding * Encoding)
{
  TFileStream Stream(FileName, static_cast<Word>(fmOpenRead | fmShareDenyWrite));
  LoadFromStream(&Stream, Encoding);
}
//---------------------------------------------------------------------------
void TStrings::LoadFromStream(TStream * Stream)
{
  LoadFromStream(Stream, nullptr);
}
//---------------------------------------------------------------------------
void TStrings::LoadFromStream(TStream * Stream, TEncoding * Encoding)
{
  BeginUpdate();
  try
  {
    __int64 Size = Stream->Size - Stream->Position;
    TBytes Buffer;
    Buffer.Length = static_cast<int>(Size);
    Stream->Read(Buffer, 0, static_cast<int>(Size));
    int PreambleSize = TEncoding::GetBufferEncoding(Buffer, Encoding, FDefaultEncoding);
    FEncoding = Encoding;
    SetTextStr(Encoding->GetString(Buffer, PreambleSize, Buffer.Length - PreambleSize));
  }
  catch (...)
  {
    EndUpdate();
    throw;
  }
  EndUpdate();
}
//---------------------------------------------------------------------------
void TStrings::Move(int CurIndex, int NewIndex)
{
  if (CurIndex != NewIndex)
  {
    BeginUpdate();
    try
    {
      UnicodeString TempString = Get(CurIndex);
      TObject * TempObject = GetObject(CurIndex);
      PutObject(CurIndex, nullptr);
      Delete(CurIndex);
      InsertObject(NewIndex, TempString, TempObject);
    }
    catch (...)
    {
      EndUpdate();
      throw;
    }
    EndUpdate();
  }
}
//---------------------------------------------------------------------------
void TStrings::SaveToFile(const UnicodeString FileName)
{
  SaveToFile(FileName, nullptr);
}
//---------------------------------------------------------------------------
void TStrings::SaveToFile(const UnicodeString FileName, TEncoding * Encoding)
{
  TFileStream Stream(FileName, fmCreate);
  SaveToStream(&Stream, Encoding);
}
//---------------------------------------------------------------------------
void TStrings::SaveToStream(TStream * Stream)
{
  SaveToStream(Stream, nullptr);
}
//---------------------------------------------------------------------------
void TStrings::SaveToStream(TStream * Stream, TEncoding * Encoding)
{
  if (Encoding == nullptr)
  {
    Encoding = (FEncoding != nullptr) ? FEncoding : FDefaultEncoding;
  }
  TBytes Buffer = Encoding->GetBytes(GetTextStr());
  if (FWriteBOM)
  {
    TBytes Preamble = Encoding->GetPreamble();
    if (Preamble.Length > 0)
    {
      Stream->WriteBuffer(Preamble, Preamble.Length);
    }
  }
  Stream->WriteBuffer(Buffer, Buffer.Length);
}
//---------------------------------------------------------------------------
void TStrings::SetText(const wchar_t * Text)
{
  SetTextStr(UnicodeString(Text));
}
//---------------------------------------------------------------------------
TStringDynArray TStrings::ToStringArray()
{
  TStringDynArray Result;
  Result.Length = GetCount();
  for (int I = 0; I < GetCount(); I++)
  {
    Result[I] = Get(I);
  }
  return Result;
}
//---------------------------------------------------------------------------
TObject * TStrings::GetObject(int)
{
  return nullptr;
}
//---------------------------------------------------------------------------
void TStrings::Put(int Index, const UnicodeString S)
{
  TObject * TempObject = GetObject(Index);
  Delete(Index);
  InsertObject(Index, S, TempObject);
}
//---------------------------------------------------------------------------
void TStrings::PutObject(int, TObject *)
{
}
//---------------------------------------------------------------------------
int TStrings::GetCapacity()
{
  return GetCount();
}
//---------------------------------------------------------------------------
void TStrings::SetCapacity(int)
{
}
//---------------------------------------------------------------------------
UnicodeString TStrings::GetTextStr()
{
  std::wstring Result;
  int Count = GetCount();
  for (int I = 0; I < Count; I++)
  {
    Result += Get(I).Std();
    if (FTrailingLineBreak || (I < Count - 1))
    {
      Result += FLineBreak.Std();
    }
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
void TStrings::SetTextStr(const UnicodeString Value)
{
  BeginUpdate();
  try
  {
    Clear();
    const wchar_t * P = Value.c_str();
    const wchar_t * End = P + Value.Length();
    if (FLineBreak == sLineBreak)
    {
      // Any of CR, LF and CRLF ends a line; like Delphi, stop at a NUL
      while ((P < End) && (*P != L'\0'))
      {
        const wchar_t * Start = P;
        while ((P < End) && (*P != L'\0') && (*P != L'\n') && (*P != L'\r'))
        {
          P++;
        }
        Add(UnicodeString(Start, static_cast<int>(P - Start)));
        if ((P < End) && (*P == L'\r'))
        {
          P++;
        }
        if ((P < End) && (*P == L'\n'))
        {
          P++;
        }
      }
    }
    else
    {
      const std::wstring & Data = Value.Std();
      const std::wstring & Break = FLineBreak.Std();
      size_t Start = 0;
      while (Start < Data.size())
      {
        size_t Found = Break.empty() ? std::wstring::npos : Data.find(Break, Start);
        if (Found == std::wstring::npos)
        {
          Add(UnicodeString(Data.substr(Start)));
          break;
        }
        Add(UnicodeString(Data.substr(Start, Found - Start)));
        Start = Found + Break.size();
      }
    }
  }
  catch (...)
  {
    EndUpdate();
    throw;
  }
  EndUpdate();
}
//---------------------------------------------------------------------------
UnicodeString TStrings::GetDelimitedText()
{
  int Count = GetCount();
  if ((Count == 1) && Get(0).IsEmpty())
  {
    return UnicodeString(FQuoteChar) + FQuoteChar;
  }
  std::wstring Result;
  for (int I = 0; I < Count; I++)
  {
    UnicodeString S = Get(I);
    bool NeedsQuoting = false;
    for (wchar_t Ch : S)
    {
      // As Delphi scans a PChar, a NUL ends the scan
      if (Ch == L'\0')
      {
        break;
      }
      if ((Ch == FQuoteChar) || (Ch == FDelimiter) || (!FStrictDelimiter && (Ch <= L' ')))
      {
        NeedsQuoting = true;
        break;
      }
    }
    if (NeedsQuoting)
    {
      S = AnsiQuotedStr(S, FQuoteChar);
    }
    if (I > 0)
    {
      Result += FDelimiter;
    }
    Result += S.Std();
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
void TStrings::SetDelimitedText(const UnicodeString Value)
{
  BeginUpdate();
  try
  {
    Clear();
    const wchar_t * P = Value.c_str();
    auto IsBlank = [](wchar_t Ch) { return (Ch >= 1) && (Ch <= L' '); };
    if (!FStrictDelimiter)
    {
      while (IsBlank(*P))
      {
        P++;
      }
    }
    while (*P != L'\0')
    {
      UnicodeString S;
      if (*P == FQuoteChar)
      {
        S = AnsiExtractQuotedStr(P, FQuoteChar);
      }
      else
      {
        const wchar_t * Start = P;
        while (((!FStrictDelimiter && (*P > L' ')) || (FStrictDelimiter && (*P != L'\0'))) && (*P != FDelimiter))
        {
          P++;
        }
        S = UnicodeString(Start, static_cast<int>(P - Start));
      }
      Add(S);
      if (!FStrictDelimiter)
      {
        while (IsBlank(*P))
        {
          P++;
        }
      }
      if (*P == FDelimiter)
      {
        if (P[1] == L'\0')
        {
          Add(UnicodeString());
        }
        do
        {
          P++;
        }
        while (!FStrictDelimiter && IsBlank(*P));
      }
    }
  }
  catch (...)
  {
    EndUpdate();
    throw;
  }
  EndUpdate();
}
//---------------------------------------------------------------------------
UnicodeString TStrings::GetCommaText()
{
  wchar_t OldDelimiter = FDelimiter;
  wchar_t OldQuoteChar = FQuoteChar;
  bool OldStrictDelimiter = FStrictDelimiter;
  FDelimiter = L',';
  FQuoteChar = L'"';
  FStrictDelimiter = false;
  UnicodeString Result;
  try
  {
    Result = GetDelimitedText();
  }
  catch (...)
  {
    FDelimiter = OldDelimiter;
    FQuoteChar = OldQuoteChar;
    FStrictDelimiter = OldStrictDelimiter;
    throw;
  }
  FDelimiter = OldDelimiter;
  FQuoteChar = OldQuoteChar;
  FStrictDelimiter = OldStrictDelimiter;
  return Result;
}
//---------------------------------------------------------------------------
void TStrings::SetCommaText(const UnicodeString Value)
{
  wchar_t OldDelimiter = FDelimiter;
  wchar_t OldQuoteChar = FQuoteChar;
  bool OldStrictDelimiter = FStrictDelimiter;
  FDelimiter = L',';
  FQuoteChar = L'"';
  FStrictDelimiter = false;
  try
  {
    SetDelimitedText(Value);
  }
  catch (...)
  {
    FDelimiter = OldDelimiter;
    FQuoteChar = OldQuoteChar;
    FStrictDelimiter = OldStrictDelimiter;
    throw;
  }
  FDelimiter = OldDelimiter;
  FQuoteChar = OldQuoteChar;
  FStrictDelimiter = OldStrictDelimiter;
}
//---------------------------------------------------------------------------
UnicodeString TStrings::ExtractName(const UnicodeString S)
{
  int P = S.Pos(FNameValueSeparator);
  return (P != 0) ? S.SubString(1, P - 1) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString TStrings::GetName(int Index)
{
  return ExtractName(Get(Index));
}
//---------------------------------------------------------------------------
UnicodeString TStrings::GetValue(const UnicodeString Name)
{
  int I = IndexOfName(Name);
  return (I >= 0) ? Get(I).SubString(Name.Length() + 2, MaxInt) : UnicodeString();
}
//---------------------------------------------------------------------------
void TStrings::SetValue(const UnicodeString Name, const UnicodeString Value)
{
  int I = IndexOfName(Name);
  if (!Value.IsEmpty())
  {
    if (I < 0)
    {
      I = Add(UnicodeString());
    }
    Put(I, Name + FNameValueSeparator + Value);
  }
  else if (I >= 0)
  {
    Delete(I);
  }
}
//---------------------------------------------------------------------------
UnicodeString TStrings::GetValueFromIndex(int Index)
{
  if (Index < 0)
  {
    return UnicodeString();
  }
  UnicodeString S = Get(Index);
  int P = S.Pos(FNameValueSeparator);
  return (P != 0) ? S.SubString(P + 1, MaxInt) : UnicodeString();
}
//---------------------------------------------------------------------------
void TStrings::SetValueFromIndex(int Index, const UnicodeString Value)
{
  if (!Value.IsEmpty())
  {
    if (Index < 0)
    {
      Index = Add(UnicodeString());
    }
    Put(Index, GetName(Index) + FNameValueSeparator + Value);
  }
  else if (Index >= 0)
  {
    Delete(Index);
  }
}
//---------------------------------------------------------------------------
// TStringList
//---------------------------------------------------------------------------
TStringList::TStringList()
{
}
//---------------------------------------------------------------------------
TStringList::TStringList(bool OwnsObjects) :
  FOwnsObjects(OwnsObjects)
{
}
//---------------------------------------------------------------------------
TStringList::TStringList(wchar_t QuoteChar, wchar_t Delimiter)
{
  SetQuoteChar(QuoteChar);
  SetDelimiter(Delimiter);
}
//---------------------------------------------------------------------------
TStringList::TStringList(TDuplicates Duplicates, bool Sorted, bool CaseSensitive) :
  FSorted(Sorted),
  FDuplicates(Duplicates),
  FCaseSensitive(CaseSensitive)
{
}
//---------------------------------------------------------------------------
TStringList::~TStringList()
{
  FOnChange = nullptr;
  FOnChanging = nullptr;
  ClearItems();
}
//---------------------------------------------------------------------------
void TStringList::ClearItems()
{
  std::vector<TStringItem> Items;
  Items.swap(FList);
  if (FOwnsObjects)
  {
    for (TStringItem & Item : Items)
    {
      delete Item.FObject;
    }
  }
}
//---------------------------------------------------------------------------
void TStringList::CheckIndex(int Index)
{
  if ((Index < 0) || (Index >= GetCount()))
  {
    Error(SListIndexError, Index);
  }
}
//---------------------------------------------------------------------------
void TStringList::Changed()
{
  if ((GetUpdateCount() == 0) && FOnChange)
  {
    FOnChange(this);
  }
}
//---------------------------------------------------------------------------
void TStringList::Changing()
{
  if ((GetUpdateCount() == 0) && FOnChanging)
  {
    FOnChanging(this);
  }
}
//---------------------------------------------------------------------------
void TStringList::SetUpdateState(bool Updating)
{
  if (Updating)
  {
    Changing();
  }
  else
  {
    Changed();
  }
}
//---------------------------------------------------------------------------
int TStringList::Add(const UnicodeString S)
{
  return AddObject(S, nullptr);
}
//---------------------------------------------------------------------------
int TStringList::AddObject(const UnicodeString S, TObject * AObject)
{
  int Result;
  if (!FSorted)
  {
    Result = GetCount();
  }
  else if (Find(S, Result))
  {
    switch (FDuplicates)
    {
      case dupIgnore:
        return Result;
      case dupError:
        throw EStringListError(SDuplicateString);
      default:
        break;
    }
  }
  InsertItem(Result, S, AObject);
  return Result;
}
//---------------------------------------------------------------------------
void TStringList::Assign(TPersistent * Source)
{
  TStrings::Assign(Source);
  TStringList * List = dynamic_cast<TStringList *>(Source);
  if (List != nullptr)
  {
    FCaseSensitive = List->FCaseSensitive;
    FDuplicates = List->FDuplicates;
    FSorted = List->FSorted;
  }
}
//---------------------------------------------------------------------------
void TStringList::Clear()
{
  if (!FList.empty())
  {
    Changing();
    ClearItems();
    Changed();
  }
}
//---------------------------------------------------------------------------
void TStringList::Delete(int Index)
{
  CheckIndex(Index);
  Changing();
  TObject * Obj = FList[Index].FObject;
  FList.erase(FList.begin() + Index);
  if (FOwnsObjects)
  {
    delete Obj;
  }
  Changed();
}
//---------------------------------------------------------------------------
void TStringList::Exchange(int Index1, int Index2)
{
  CheckIndex(Index1);
  CheckIndex(Index2);
  Changing();
  ExchangeItems(Index1, Index2);
  Changed();
}
//---------------------------------------------------------------------------
void TStringList::ExchangeItems(int Index1, int Index2)
{
  std::swap(FList[Index1], FList[Index2]);
}
//---------------------------------------------------------------------------
bool TStringList::Find(const UnicodeString S, int & Index)
{
  bool Result = false;
  int L = 0;
  int H = GetCount() - 1;
  while (L <= H)
  {
    int I = (L + H) >> 1;
    int C = CompareStrings(FList[I].FString, S);
    if (C < 0)
    {
      L = I + 1;
    }
    else
    {
      H = I - 1;
      if (C == 0)
      {
        Result = true;
        if (FDuplicates != dupAccept)
        {
          L = I;
        }
      }
    }
  }
  Index = L;
  return Result;
}
//---------------------------------------------------------------------------
int TStringList::IndexOf(const UnicodeString S)
{
  if (!FSorted)
  {
    return TStrings::IndexOf(S);
  }
  int Result;
  return Find(S, Result) ? Result : -1;
}
//---------------------------------------------------------------------------
void TStringList::Insert(int Index, const UnicodeString S)
{
  InsertObject(Index, S, nullptr);
}
//---------------------------------------------------------------------------
void TStringList::InsertObject(int Index, const UnicodeString S, TObject * AObject)
{
  if (FSorted)
  {
    throw EStringListError(SSortedListError);
  }
  if ((Index < 0) || (Index > GetCount()))
  {
    Error(SListIndexError, Index);
  }
  InsertItem(Index, S, AObject);
}
//---------------------------------------------------------------------------
void TStringList::InsertItem(int Index, const UnicodeString S, TObject * AObject)
{
  Changing();
  FList.insert(FList.begin() + Index, TStringItem{S, AObject});
  Changed();
}
//---------------------------------------------------------------------------
int TStringList::DefaultCompare(TStringList * List, int Index1, int Index2)
{
  return List->CompareStrings(List->FList[Index1].FString, List->FList[Index2].FString);
}
//---------------------------------------------------------------------------
void TStringList::Sort()
{
  CustomSort(DefaultCompare);
}
//---------------------------------------------------------------------------
void TStringList::CustomSort(TStringListSortCompare Compare)
{
  if (!FSorted && (GetCount() > 1))
  {
    Changing();
    QuickSort(0, GetCount() - 1, Compare);
    Changed();
  }
}
//---------------------------------------------------------------------------
void TStringList::QuickSort(int L, int R, TStringListSortCompare SCompare)
{
  // Delphi's algorithm: the comparison works on indexes, so the pivot index is
  // tracked as items are exchanged
  while (L < R)
  {
    if ((R - L) == 1)
    {
      if (SCompare(this, L, R) > 0)
      {
        ExchangeItems(L, R);
      }
      break;
    }
    int I = L;
    int J = R;
    int P = (L + R) >> 1;
    do
    {
      while ((I != P) && (SCompare(this, I, P) < 0))
      {
        I++;
      }
      while ((J != P) && (SCompare(this, J, P) > 0))
      {
        J--;
      }
      if (I <= J)
      {
        if (I != J)
        {
          ExchangeItems(I, J);
        }
        if (P == I)
        {
          P = J;
        }
        else if (P == J)
        {
          P = I;
        }
        I++;
        J--;
      }
    }
    while (I <= J);
    if ((J - L) > (R - I))
    {
      if (I < R)
      {
        QuickSort(I, R, SCompare);
      }
      R = J;
    }
    else
    {
      if (L < J)
      {
        QuickSort(L, J, SCompare);
      }
      L = I;
    }
  }
}
//---------------------------------------------------------------------------
int TStringList::CompareStrings(const UnicodeString S1, const UnicodeString S2)
{
  return FCaseSensitive ? AnsiCompareStr(S1, S2) : AnsiCompareText(S1, S2);
}
//---------------------------------------------------------------------------
UnicodeString TStringList::Get(int Index)
{
  CheckIndex(Index);
  return FList[Index].FString;
}
//---------------------------------------------------------------------------
TObject * TStringList::GetObject(int Index)
{
  CheckIndex(Index);
  return FList[Index].FObject;
}
//---------------------------------------------------------------------------
void TStringList::Put(int Index, const UnicodeString S)
{
  if (FSorted)
  {
    throw EStringListError(SSortedListError);
  }
  CheckIndex(Index);
  Changing();
  FList[Index].FString = S;
  Changed();
}
//---------------------------------------------------------------------------
void TStringList::PutObject(int Index, TObject * AObject)
{
  CheckIndex(Index);
  Changing();
  FList[Index].FObject = AObject;
  Changed();
}
//---------------------------------------------------------------------------
void TStringList::SetCapacity(int NewCapacity)
{
  if (NewCapacity < GetCount())
  {
    Error(SListCapacityError, NewCapacity);
  }
  FList.reserve(NewCapacity);
}
//---------------------------------------------------------------------------
void TStringList::SetSorted(bool Value)
{
  if (FSorted != Value)
  {
    if (Value)
    {
      Sort();
    }
    FSorted = Value;
  }
}
//---------------------------------------------------------------------------
void TStringList::SetCaseSensitive(bool Value)
{
  if (Value != FCaseSensitive)
  {
    FCaseSensitive = Value;
    if (FSorted)
    {
      FSorted = false;
      SetSorted(true);
    }
  }
}
//---------------------------------------------------------------------------
} // namespace Classes
} // namespace System
