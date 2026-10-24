// System.Classes, System.Contnrs, System.SyncObjs, TEncoding
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Contnrs.hpp>
#include <System.SyncObjs.hpp>
#include <Xml.XMLDoc.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <thread>
#include <vector>
//---------------------------------------------------------------------------
namespace
{
class TTracked : public TObject
{
public:
  explicit TTracked(int & Counter) : FCounter(Counter) { FCounter++; }
  virtual ~TTracked() { FCounter--; }

private:
  int & FCounter;
};

class TNotifiedList : public TList
{
public:
  std::vector<std::pair<void *, TListNotification>> Notifications;
  ~TNotifiedList() { Clear(); }

protected:
  void Notify(void * Ptr, TListNotification Action) override
  {
    Notifications.push_back(std::make_pair(Ptr, Action));
  }
};

class TChangeCounter
{
public:
  void Changed(TObject *) { Count++; }
  int Count = 0;
};

UnicodeString Joined(TStrings * Strings)
{
  UnicodeString Result;
  for (int I = 0; I < Strings->Count; I++)
  {
    Result += ((I > 0) ? L"|" : L"") + Strings->Strings[I];
  }
  return Result;
}

int CompareInts(void * Item1, void * Item2)
{
  return static_cast<int>(reinterpret_cast<intptr_t>(Item1) - reinterpret_cast<intptr_t>(Item2));
}

int CompareByLength(TStringList * List, int Index1, int Index2)
{
  return List->Strings[Index1].Length() - List->Strings[Index2].Length();
}

class TempDir
{
public:
  TempDir()
  {
    std::string Pattern = std::string((getenv("TMPDIR") != nullptr) ? getenv("TMPDIR") : "/tmp") + "/openscp-rtl-XXXXXX";
    std::vector<char> Buffer(Pattern.begin(), Pattern.end());
    Buffer.push_back('\0');
    FPath = mkdtemp(Buffer.data());
  }
  ~TempDir()
  {
    std::error_code Error;
    std::filesystem::remove_all(FPath, Error);
  }
  UnicodeString operator /(const wchar_t * Name) const { return UnicodeString(FPath.c_str()) + L"/" + Name; }

private:
  std::string FPath;
};

std::string ReadAll(const UnicodeString & FileName)
{
  TFileStream Stream(FileName, fmOpenRead);
  std::string Result(static_cast<size_t>(Stream.Size), '\0');
  Stream.ReadBuffer(Result.data(), static_cast<int>(Result.size()));
  return Result;
}
} // namespace
//---------------------------------------------------------------------------
TEST(TList, Basics)
{
  TList List;
  int A, B, C;
  EXPECT_EQ(List.Add(&A), 0);
  EXPECT_EQ(List.Add(&B), 1);
  List.Insert(0, &C);
  EXPECT_EQ(List.Count, 3);
  EXPECT_EQ(List.Items[0], &C);
  EXPECT_EQ(List[1], &A);
  EXPECT_EQ(List.IndexOf(&B), 2);
  EXPECT_EQ(List.IndexOf(nullptr), -1);
  List.Exchange(0, 2);
  EXPECT_EQ(List.First(), &B);
  EXPECT_EQ(List.Last(), &C);
  List.Move(0, 2);
  EXPECT_EQ(List.Last(), &B);
  EXPECT_EQ(List.Remove(&A), 0);
  EXPECT_EQ(List.Remove(&A), -1);
  EXPECT_THROW(List.Items[5], EListError);
  EXPECT_THROW(List.Delete(-1), EListError);
  EXPECT_THROW(List.Insert(10, &A), EListError);
  try
  {
    List.Get(7);
  }
  catch (EListError & E)
  {
    EXPECT_EQ(E.Message, L"List index out of bounds (7)");
  }
  List.Count = 4;
  EXPECT_EQ(List.Items[3], nullptr);
  List.Pack();
  EXPECT_EQ(List.Count, 2);
  List.Items[0] = &A;
  EXPECT_EQ(List.Items[0], &A);
  List.Clear();
  EXPECT_EQ(List.Count, 0);
  EXPECT_THROW(List.First(), EListError);
}
//---------------------------------------------------------------------------
TEST(TList, NotificationsAndSort)
{
  TNotifiedList List;
  int A, B;
  List.Add(&A);
  List.Add(nullptr);
  List.Add(&B);
  EXPECT_EQ(List.Extract(&A), &A);
  List.Delete(0);
  List.Put(0, &A);
  std::vector<std::pair<void *, TListNotification>> Expected =
    { {&A, lnAdded}, {&B, lnAdded}, {&A, lnExtracted}, {&B, lnDeleted}, {&A, lnAdded} };
  EXPECT_EQ(List.Notifications, Expected);

  TList Numbers;
  for (intptr_t Value : {5, 3, 9, 1, 7, 3})
  {
    Numbers.Add(reinterpret_cast<void *>(Value));
  }
  Numbers.Sort(CompareInts);
  std::vector<intptr_t> Sorted;
  for (int I = 0; I < Numbers.Count; I++)
  {
    Sorted.push_back(reinterpret_cast<intptr_t>(Numbers[I]));
  }
  EXPECT_EQ(Sorted, (std::vector<intptr_t>{1, 3, 3, 5, 7, 9}));
}
//---------------------------------------------------------------------------
TEST(TObjectList, Ownership)
{
  int Alive = 0;
  {
    TObjectList List;
    EXPECT_TRUE(List.OwnsObjects);
    List.Add(new TTracked(Alive));
    TTracked * Second = new TTracked(Alive);
    List.Add(Second);
    List.Add(new TTracked(Alive));
    EXPECT_EQ(Alive, 3);
    List.Delete(0);
    EXPECT_EQ(Alive, 2);
    EXPECT_EQ(List.Extract(Second), Second);
    EXPECT_EQ(Alive, 2);
    delete Second;
    EXPECT_EQ(List.FindInstanceOf(__classid(TTracked)), 0);
    EXPECT_EQ(List.FindInstanceOf(__classid(TObject), true), -1);
    EXPECT_EQ(List.FindInstanceOf(__classid(TObject), false), 0);
  }
  EXPECT_EQ(Alive, 0);
  TObjectList NotOwning(false);
  TTracked Object(Alive);
  NotOwning.Add(&Object);
  NotOwning.Clear();
  EXPECT_EQ(Alive, 1);
}
//---------------------------------------------------------------------------
TEST(TStringList, AddDeleteInsert)
{
  TStringList List;
  EXPECT_EQ(List.Add(L"b"), 0);
  EXPECT_EQ(List.Add(L"a"), 1);
  List.Insert(0, L"c");
  List.Append(L"d");
  EXPECT_EQ(Joined(&List), L"c|b|a|d");
  EXPECT_EQ(List[2], L"a");
  List.Delete(1);
  EXPECT_EQ(Joined(&List), L"c|a|d");
  List.Strings[0] = L"x";
  EXPECT_EQ(List.Strings[0], L"x");
  EXPECT_EQ(List.IndexOf(L"A"), 1);
  EXPECT_EQ(List.IndexOf(L"z"), -1);
  List.Exchange(0, 2);
  EXPECT_EQ(Joined(&List), L"d|a|x");
  List.Move(0, 2);
  EXPECT_EQ(Joined(&List), L"a|x|d");
  EXPECT_THROW(List.Strings[3], EStringListError);
  EXPECT_THROW(List.Delete(3), EStringListError);
  EXPECT_THROW(List.Insert(4, L"x"), EStringListError);
  List.Clear();
  EXPECT_EQ(List.Count, 0);
}
//---------------------------------------------------------------------------
TEST(TStringList, Objects)
{
  int Alive = 0;
  TStringList * List = new TStringList(true);
  List->AddObject(L"one", new TTracked(Alive));
  List->AddObject(L"two", new TTracked(Alive));
  List->AddObject(L"three", new TTracked(Alive));
  EXPECT_EQ(Alive, 3);
  EXPECT_EQ(List->IndexOfObject(List->Objects[1]), 1);
  List->Delete(0);
  EXPECT_EQ(Alive, 2);
  // Move keeps the object alive
  List->Move(0, 1);
  EXPECT_EQ(Alive, 2);
  EXPECT_NE(List->Objects[1], nullptr);
  delete List;
  EXPECT_EQ(Alive, 0);
}
//---------------------------------------------------------------------------
TEST(TStringList, Sorted)
{
  TStringList List;
  List.Sorted = true;
  List.Add(L"banana");
  List.Add(L"Apple");
  List.Add(L"cherry");
  EXPECT_EQ(List.Add(L"apple"), 0);
  EXPECT_EQ(Joined(&List), L"Apple|banana|cherry");
  int Index;
  EXPECT_TRUE(List.Find(L"BANANA", Index));
  EXPECT_EQ(Index, 1);
  EXPECT_FALSE(List.Find(L"blueberry", Index));
  EXPECT_EQ(Index, 2);
  EXPECT_EQ(List.IndexOf(L"CHERRY"), 2);
  EXPECT_THROW(List.Insert(0, L"x"), EStringListError);
  EXPECT_THROW(List.Strings[0] = L"x", EStringListError);

  List.Duplicates = dupError;
  EXPECT_THROW(List.Add(L"apple"), EStringListError);
  List.Duplicates = dupAccept;
  List.Add(L"apple");
  EXPECT_EQ(List.Count, 4);
  EXPECT_TRUE(List.Find(L"APPLE", Index));
  EXPECT_EQ(Index, 0);

  List.CaseSensitive = true;
  EXPECT_EQ(Joined(&List), L"apple|Apple|banana|cherry");
  EXPECT_EQ(List.IndexOf(L"APPLE"), -1);

  TStringList Unsorted;
  Unsorted.CommaText = L"c,a,b,A";
  Unsorted.Sort();
  EXPECT_EQ(Unsorted.Strings[0].LowerCase(), L"a");
  EXPECT_EQ(Unsorted.Strings[3], L"c");
  Unsorted.CommaText = L"ccc,a,bb";
  Unsorted.CustomSort(CompareByLength);
  EXPECT_EQ(Joined(&Unsorted), L"a|bb|ccc");
}
//---------------------------------------------------------------------------
TEST(TStringList, Text)
{
  TStringList List;
  List.Text = L"a\nb\r\nc\rd";
  EXPECT_EQ(Joined(&List), L"a|b|c|d");
  EXPECT_EQ(List.Text, L"a\nb\nc\nd\n");
  List.Text = L"a\n\nb\n";
  EXPECT_EQ(List.Count, 3);
  EXPECT_EQ(List.Strings[1], L"");
  List.Text = L"";
  EXPECT_EQ(List.Count, 0);
  EXPECT_EQ(List.Text, L"");
  List.Text = L"one";
  EXPECT_EQ(List.Text, L"one\n");
  List.TrailingLineBreak = false;
  List.Add(L"two");
  EXPECT_EQ(List.Text, L"one\ntwo");
  List.LineBreak = L"\r\n";
  EXPECT_EQ(List.Text, L"one\r\ntwo");
  List.Text = L"x\r\ny\nz";
  EXPECT_EQ(Joined(&List), L"x|y\nz");
}
//---------------------------------------------------------------------------
TEST(TStringList, CommaAndDelimitedText)
{
  TStringList List;
  List.CommaText = L"a,b c,\"d,e\",\"f\"\"g\",,h";
  EXPECT_EQ(Joined(&List), L"a|b|c|d,e|f\"g||h");
  EXPECT_EQ(List.CommaText, L"a,b,c,\"d,e\",\"f\"\"g\",,h");
  List.CommaText = L"a,";
  EXPECT_EQ(List.Count, 2);
  List.CommaText = L"  x  ,  y  ";
  EXPECT_EQ(Joined(&List), L"x|y");
  List.Clear();
  List.Add(L"");
  EXPECT_EQ(List.CommaText, L"\"\"");
  List.Add(L"with space");
  EXPECT_EQ(List.CommaText, L",\"with space\"");

  List.Delimiter = L';';
  List.StrictDelimiter = true;
  List.DelimitedText = L"a b;c";
  EXPECT_EQ(Joined(&List), L"a b|c");
  EXPECT_EQ(List.DelimitedText, L"a b;c");
  // CommaText always uses the defaults
  EXPECT_EQ(List.CommaText, L"\"a b\",c");
  EXPECT_EQ(List.Delimiter, L';');
}
//---------------------------------------------------------------------------
TEST(TStringList, NamesAndValues)
{
  TStringList List;
  List.Add(L"Host=example.com");
  List.Add(L"Port=22");
  List.Add(L"NoValue");
  List.Add(L"Empty=");
  EXPECT_EQ(List.Names[0], L"Host");
  EXPECT_EQ(List.Names[2], L"");
  EXPECT_EQ(List.Values[L"host"], L"example.com");
  EXPECT_EQ(List.Values[L"Missing"], L"");
  EXPECT_EQ(List.ValueFromIndex[1], L"22");
  EXPECT_EQ(List.ValueFromIndex[2], L"");
  EXPECT_EQ(List.IndexOfName(L"PORT"), 1);
  EXPECT_EQ(List.IndexOfName(L"NoValue"), -1);
  List.Values[L"Port"] = L"2222";
  EXPECT_EQ(List.Strings[1], L"Port=2222");
  List.Values[L"User"] = L"root";
  EXPECT_EQ(List.Strings[4], L"User=root");
  // Setting an empty value deletes the entry
  List.Values[L"Host"] = L"";
  EXPECT_EQ(List.IndexOfName(L"Host"), -1);
  List.ValueFromIndex[0] = L"23";
  EXPECT_EQ(List.Strings[0], L"Port=23");
  List.NameValueSeparator = L':';
  List.Add(L"a:b");
  EXPECT_EQ(List.Values[L"a"], L"b");
  List.AddPair(L"x", L"y");
  EXPECT_EQ(List.Strings[List.Count - 1], L"x:y");
}
//---------------------------------------------------------------------------
TEST(TStringList, AssignEqualsAndEvents)
{
  TStringList Source;
  Source.Sorted = true;
  Source.CaseSensitive = true;
  Source.Add(L"b");
  Source.Add(L"a");
  Source.Delimiter = L';';
  TStringList Target;
  Target.Assign(&Source);
  EXPECT_TRUE(Target.Equals(&Source));
  EXPECT_TRUE(Target.Sorted);
  EXPECT_TRUE(Target.CaseSensitive);
  EXPECT_EQ(Target.Delimiter, L';');
  Target.Sorted = false;
  Target.Add(L"c");
  EXPECT_FALSE(Target.Equals(&Source));
  EXPECT_THROW(Target.Assign(nullptr), EConvertError);
  try
  {
    TPersistent Persistent;
    Persistent.Assign(&Target);
  }
  catch (EConvertError & E)
  {
    EXPECT_EQ(E.Message, L"Cannot assign a TStringList to a TPersistent");
  }

  TChangeCounter Counter;
  TStringList List;
  List.OnChange = TNotifyEvent(&Counter, &TChangeCounter::Changed);
  List.Add(L"a");
  List.Add(L"b");
  EXPECT_EQ(Counter.Count, 2);
  List.BeginUpdate();
  List.Add(L"c");
  List.Delete(0);
  EXPECT_EQ(Counter.Count, 2);
  List.EndUpdate();
  EXPECT_EQ(Counter.Count, 3);
  List.Text = L"x\ny";
  EXPECT_EQ(Counter.Count, 4);
}
//---------------------------------------------------------------------------
TEST(TStringList, Files)
{
  TempDir Dir;
  TStringList List;
  List.Add(L"za\u017C\u00F3\u0142\u0107");
  List.Add(L"second");
  List.SaveToFile(Dir / L"ansi.txt");
  EXPECT_EQ(ReadAll(Dir / L"ansi.txt"), "za\xC5\xBC\xC3\xB3\xC5\x82\xC4\x87\nsecond\n");
  List.SaveToFile(Dir / L"utf8.txt", TEncoding::UTF8);
  EXPECT_EQ(ReadAll(Dir / L"utf8.txt"), "\xEF\xBB\xBFza\xC5\xBC\xC3\xB3\xC5\x82\xC4\x87\nsecond\n");
  List.SaveToFile(Dir / L"utf16.txt", TEncoding::Unicode);

  for (const wchar_t * Name : {L"ansi.txt", L"utf8.txt", L"utf16.txt"})
  {
    TStringList Loaded;
    Loaded.LoadFromFile(Dir / Name);
    EXPECT_TRUE(Loaded.Equals(&List)) << Name;
  }
  TStringList Loaded;
  Loaded.LoadFromFile(Dir / L"utf8.txt");
  EXPECT_EQ(Loaded.Encoding, TEncoding::UTF8);
  // Saved again in the encoding it was loaded with
  Loaded.SaveToFile(Dir / L"again.txt");
  EXPECT_EQ(ReadAll(Dir / L"again.txt"), ReadAll(Dir / L"utf8.txt"));
  List.WriteBOM = false;
  List.SaveToFile(Dir / L"nobom.txt", TEncoding::UTF8);
  EXPECT_EQ(ReadAll(Dir / L"nobom.txt"), ReadAll(Dir / L"ansi.txt"));
  EXPECT_THROW(Loaded.LoadFromFile(Dir / L"missing.txt"), EFOpenError);
}
//---------------------------------------------------------------------------
TEST(Streams, MemoryStream)
{
  TMemoryStream Stream;
  EXPECT_EQ(Stream.Size, 0);
  Stream.WriteBuffer("hello", 5);
  EXPECT_EQ(Stream.Size, 5);
  EXPECT_EQ(Stream.Position, 5);
  Stream.Position = 10;
  Stream.WriteBuffer("!", 1);
  EXPECT_EQ(Stream.Size, 11);
  EXPECT_EQ(static_cast<char *>(Stream.Memory)[7], '\0');
  Stream.Seek(0, soFromBeginning);
  char Buffer[16] = {};
  EXPECT_EQ(Stream.Read(Buffer, 3), 3);
  EXPECT_EQ(std::string(Buffer, 3), "hel");
  EXPECT_EQ(Stream.Seek(static_cast<__int64>(-1), soEnd), 10);
  EXPECT_EQ(Stream.Seek(static_cast<__int64>(2), soCurrent), 12);
  EXPECT_EQ(Stream.Read(Buffer, 1), 0);
  EXPECT_THROW(Stream.ReadBuffer(Buffer, 1), EReadError);
  Stream.Size = 2;
  EXPECT_EQ(Stream.Position, 2);
  Stream.Position = 0;
  TBytes Bytes;
  Bytes.Length = 2;
  Stream.ReadBuffer(Bytes, 2);
  EXPECT_EQ(Bytes[1], 'e');
  Stream.Clear();
  EXPECT_EQ(Stream.Size, 0);
  EXPECT_EQ(Stream.Memory, nullptr);
}
//---------------------------------------------------------------------------
TEST(Streams, FileStream)
{
  TempDir Dir;
  {
    TFileStream Stream(Dir / L"f.bin", fmCreate);
    EXPECT_EQ(Stream.FileName, Dir / L"f.bin");
    Stream.WriteBuffer("0123456789", 10);
    Stream.Size = 4;
    EXPECT_EQ(Stream.Size, 4);
  }
  {
    TFileStream Stream(Dir / L"f.bin", static_cast<Word>(fmOpenRead | fmShareDenyNone));
    EXPECT_EQ(Stream.Size, 4);
    TMemoryStream Memory;
    EXPECT_EQ(Memory.CopyFrom(&Stream, 0), 4);
    EXPECT_EQ(std::string(static_cast<char *>(Memory.Memory), 4), "0123");
    THandleStream Handle(Stream.Handle);
    Handle.Position = 1;
    char Ch;
    Handle.ReadBuffer(&Ch, 1);
    EXPECT_EQ(Ch, '1');
  }
  EXPECT_THROW(TFileStream(Dir / L"missing/f", fmCreate), EFCreateError);
  try
  {
    TFileStream Stream(Dir / L"missing", fmOpenRead);
  }
  catch (EFOpenError & E)
  {
    EXPECT_EQ(E.Message.Pos(L"Cannot open file \"" + (Dir / L"missing") + L"\". "), 1);
  }
  TMemoryStream Memory;
  Memory.LoadFromFile(Dir / L"f.bin");
  EXPECT_EQ(Memory.Size, 4);
  Memory.SaveToFile(Dir / L"g.bin");
  EXPECT_EQ(ReadAll(Dir / L"g.bin"), "0123");
  TBytesStream BytesStream(BytesOf(RawByteString("xyz")));
  EXPECT_EQ(BytesStream.Size, 3);
  EXPECT_EQ(BytesStream.Bytes.Length, 3);
}
//---------------------------------------------------------------------------
TEST(TEncoding, Standard)
{
  EXPECT_EQ(TEncoding::UTF8->GetString(BytesOf(RawByteString("\xC5\xBC"))), L"\u017C");
  EXPECT_EQ(TEncoding::UTF8->GetPreamble().Length, 3);
  EXPECT_EQ(TEncoding::ANSI->GetPreamble().Length, 0);
  EXPECT_EQ(TEncoding::Default, TEncoding::ANSI);
  TBytes Utf16 = TEncoding::Unicode->GetBytes(L"a\U0001F600");
  ASSERT_EQ(Utf16.Length, 6);
  EXPECT_EQ(Utf16[0], 'a');
  EXPECT_EQ(Utf16[1], 0);
  EXPECT_EQ(TEncoding::Unicode->GetString(Utf16), L"a\U0001F600");
  TBytes BigEndian = TEncoding::BigEndianUnicode->GetBytes(L"a");
  EXPECT_EQ(BigEndian[1], 'a');
  EXPECT_EQ(TEncoding::ASCII->GetBytes(L"\u017C")[0], '?');

  TEncoding * Encoding = nullptr;
  EXPECT_EQ(TEncoding::GetBufferEncoding(BytesOf(RawByteString("\xFF\xFE" "a")), Encoding), 2);
  EXPECT_EQ(Encoding, TEncoding::Unicode);
  Encoding = nullptr;
  EXPECT_EQ(TEncoding::GetBufferEncoding(BytesOf(RawByteString("abc")), Encoding, TEncoding::UTF8), 0);
  EXPECT_EQ(Encoding, TEncoding::UTF8);
  Encoding = TEncoding::UTF8;
  EXPECT_EQ(TEncoding::GetBufferEncoding(BytesOf(RawByteString("\xEF\xBB\xBFx")), Encoding), 3);

  TEncoding * Cp1250 = TEncoding::GetEncoding(1250);
  EXPECT_FALSE(TEncoding::IsStandardEncoding(Cp1250));
  EXPECT_EQ(Cp1250->GetBytes(L"\u0105")[0], 0xB9);
  EXPECT_EQ(Cp1250->GetString(Cp1250->GetBytes(L"\u0105\u0119")), L"\u0105\u0119");
  TEncoding::FreeEncoding(Cp1250);
  EXPECT_EQ(StringOf(BytesOf(UnicodeString(L"za\u017C"))), L"za\u017C");
}
//---------------------------------------------------------------------------
TEST(SyncObjs, CriticalSectionAndEvent)
{
  TCriticalSection Section;
  Section.Enter();
  // Recursive
  EXPECT_TRUE(Section.TryEnter());
  Section.Leave();
  Section.Leave();

  std::atomic<int> Counter(0);
  std::vector<std::thread> Threads;
  int Shared = 0;
  for (int T = 0; T < 4; T++)
  {
    Threads.emplace_back([&]()
      {
        for (int I = 0; I < 1000; I++)
        {
          Section.Acquire();
          Shared++;
          Section.Release();
        }
        Counter++;
      });
  }
  for (std::thread & Thread : Threads)
  {
    Thread.join();
  }
  EXPECT_EQ(Shared, 4000);

  TEvent Event(nullptr, false, false, L"");
  EXPECT_EQ(Event.WaitFor(10), wrTimeout);
  std::thread Setter([&]() { Event.SetEvent(); });
  EXPECT_EQ(Event.WaitFor(5000), wrSignaled);
  Setter.join();
  // Auto-reset
  EXPECT_EQ(Event.WaitFor(10), wrTimeout);
  TSimpleEvent Manual;
  Manual.SetEvent();
  EXPECT_EQ(Manual.WaitFor(0), wrSignaled);
  EXPECT_EQ(Manual.WaitFor(0), wrSignaled);
  Manual.ResetEvent();
  EXPECT_EQ(Manual.WaitFor(0), wrTimeout);
}
//---------------------------------------------------------------------------
namespace
{
// Like the core's TRemoteDirectoryChangesCache: methods named as the
// accessors of a base class property must not replace them
class THidingStringList : private TStringList
{
public:
  UnicodeString Read(const UnicodeString & Name) { return GetValue(Name); }
  void Write(const UnicodeString & Name, const UnicodeString & Value) { SetValue(Name, Value); }
  int Size() { return Count; }

private:
  UnicodeString GetValue(const UnicodeString & Name)
  {
    // Through the property, the base class accessor
    return Values[Name] + L"!";
  }
  void SetValue(const UnicodeString & Name, const UnicodeString & Value)
  {
    Values[Name] = Value;
  }
};
} // namespace
//---------------------------------------------------------------------------
TEST(ClassesTest, PropertyAccessorsAreNotHidden)
{
  THidingStringList List;
  List.Write(L"a", L"1");
  EXPECT_EQ(List.Read(L"a"), L"1!");
  EXPECT_EQ(List.Size(), 1);
}
//---------------------------------------------------------------------------
TEST(ClassesTest, Interfaces)
{
  TXMLDocument * Document = new TXMLDocument(static_cast<TObject *>(nullptr));
  _di_IXMLDocument Interface = interface_cast<IXMLDocument>(Document);
  IInterface * Unknown = nullptr;
  EXPECT_EQ(Interface->QueryInterface(IInterface::Iid, reinterpret_cast<void **>(&Unknown)), S_OK);
  ASSERT_NE(Unknown, nullptr);
  Unknown->Release();
  GUID Other = { 1, 2, 3, { 4, 5, 6, 7, 8, 9, 10, 11 } };
  EXPECT_NE(Interface->QueryInterface(Other, reinterpret_cast<void **>(&Unknown)), S_OK);
  EXPECT_EQ(Unknown, nullptr);
}
