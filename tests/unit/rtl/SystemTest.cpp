// The System unit: Set, DynamicArray, TObject, TMethodPtr, TVarRec, Variant, misc
//---------------------------------------------------------------------------
#include <System.hpp>
#include <SysUtils.hpp>
#include <Variants.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <vector>
//---------------------------------------------------------------------------
namespace
{
enum TColor { clRed, clGreen, clBlue, clYellow };
typedef Set<TColor, clRed, clYellow> TColors;

class TBase : public TObject
{
public:
  int Value = 0;
  void SetValue(int AValue) { Value = AValue; }
  int GetValue() const { return Value; }
  virtual int Kind() { return 1; }
};

class TDerived : public TBase
{
public:
  int Kind() override { return 2; }
  void Notify(TObject * Sender) { LastSender = Sender; }
  TObject * LastSender = nullptr;
};

class TOther
{
public:
  virtual ~TOther() {}
  int Other = 0;
};

class TMultiple : public TOther, public TBase
{
public:
  void Store(int AValue) { Other = AValue; }
};

typedef TMethodPtr<void(int)> TIntEvent;
typedef TMethodPtr<int()> TIntGetter;

int CallbackFunction(void * Data, int Value)
{
  return *static_cast<int *>(Data) + Value;
}

// Static objects of the core are constructed after the RTL's (init_priority)
const UnicodeString StaticInitProbe =
  EmptyStr + FormatDateTime(L"yyyy", EncodeDate(2026, 1, 1)) + L"/" + (UnicodeString(CmdLine).IsEmpty() ? L"-" : L"/") +
  TEncoding::UTF8->GetString(TEncoding::UTF8->GetBytes(L"x"));
} // namespace
//---------------------------------------------------------------------------
TEST(Set, Operations)
{
  TColors Colors;
  EXPECT_TRUE(Colors.Empty());
  Colors << clRed << clBlue;
  EXPECT_TRUE(Colors.Contains(clRed));
  EXPECT_FALSE(Colors.Contains(clGreen));
  EXPECT_TRUE(Colors.Contains(clBlue));
  Colors >> clRed;
  EXPECT_FALSE(Colors.Contains(clRed));
  TColors Other = TColors() << clBlue << clYellow;
  EXPECT_EQ((Colors + Other), (TColors() << clBlue << clYellow));
  EXPECT_EQ((Other - Colors), (TColors() << clYellow));
  EXPECT_EQ((Other * Colors), (TColors() << clBlue));
  EXPECT_NE(Colors, Other);
  EXPECT_EQ((TColors() << clRed << clGreen).ToInt(), 3u);
  EXPECT_TRUE(TColors::FromInt(4).Contains(clBlue));
  Colors.Clear();
  EXPECT_TRUE(Colors.Empty());
}
//---------------------------------------------------------------------------
TEST(DynamicArray, SharedSemantics)
{
  DynamicArray<int> A;
  EXPECT_EQ(A.Length, 0);
  A.Length = 3;
  EXPECT_EQ(A.Length, 3);
  EXPECT_EQ(A[0], 0);
  A[1] = 5;
  // Copies share the elements, as Delphi dynamic arrays
  DynamicArray<int> B = A;
  B[2] = 7;
  EXPECT_EQ(A[2], 7);
  EXPECT_TRUE(A == B);
  // Setting the length makes the array unique
  B.Length = 4;
  B[0] = 9;
  EXPECT_EQ(A[0], 0);
  EXPECT_FALSE(A == B);
  DynamicArray<int> C = A.Copy();
  C[1] = 1;
  EXPECT_EQ(A[1], 5);
  EXPECT_EQ(A.High, 2);
  EXPECT_EQ(A.Low, 0);
  EXPECT_THROW(A[3], ERangeError);
  EXPECT_THROW(A[-1], ERangeError);
  DynamicArray<int> D = A.CopyRange(1, 10);
  EXPECT_EQ(D.Length, 2);
  EXPECT_EQ(D[0], 5);
  TBytes Bytes{1, 2, 3};
  int Sum = 0;
  for (Byte Value : Bytes)
  {
    Sum += Value;
  }
  EXPECT_EQ(Sum, 6);
}
//---------------------------------------------------------------------------
TEST(TObject, ClassNames)
{
  TDerived Derived;
  TBase * Base = &Derived;
  EXPECT_EQ(Base->ClassName(), L"TDerived");
  EXPECT_TRUE(Base->InheritsFrom(__classid(TBase)));
  EXPECT_TRUE(Base->InheritsFrom(__classid(TDerived)));
  EXPECT_TRUE(Base->InheritsFrom(__classid(TObject)));
  EXPECT_FALSE(TBase().InheritsFrom(__classid(TDerived)));
  EXPECT_EQ(__classid(TDerived)->ClassName(), L"TDerived");
  EXPECT_TRUE(*Base->ClassType() == *__classid(TDerived));
  EXPECT_TRUE(Base->ClassType()->IsInstance(&Derived));
  EXPECT_EQ(Base->ToString(), L"TDerived");
  EXPECT_EQ(TObject().ClassName(), L"TObject");
  EXPECT_EQ(TObject().QualifiedClassName(), L"System.TObject");
}
//---------------------------------------------------------------------------
TEST(TMethodPtr, BindAndCall)
{
  TDerived Object;
  TIntEvent Event(&Object, &TBase::SetValue);
  ASSERT_TRUE(static_cast<bool>(Event));
  Event(42);
  EXPECT_EQ(Object.Value, 42);

  TIntGetter Getter(&Object, &TBase::GetValue);
  EXPECT_EQ(Getter(), 42);

  // Virtual methods dispatch on the object
  TIntGetter Kind(static_cast<TBase *>(&Object), &TBase::Kind);
  EXPECT_EQ(Kind(), 2);

  TMethodPtr<void(TObject *)> Notify(&Object, &TDerived::Notify);
  Notify(&Object);
  EXPECT_EQ(Object.LastSender, &Object);

  // Assignment with braces, as converted code does
  TIntEvent Assigned;
  Assigned = {&Object, &TBase::SetValue};
  Assigned(7);
  EXPECT_EQ(Object.Value, 7);
}
//---------------------------------------------------------------------------
TEST(TMethodPtr, NullAndComparison)
{
  TIntEvent Event;
  EXPECT_FALSE(static_cast<bool>(Event));
  EXPECT_TRUE(!Event);
  EXPECT_TRUE(Event == NULL);
  EXPECT_TRUE(Event == nullptr);
  TBase A;
  TBase B;
  TIntEvent ToA(&A, &TBase::SetValue);
  TIntEvent ToA2(&A, &TBase::SetValue);
  TIntEvent ToB(&B, &TBase::SetValue);
  EXPECT_TRUE(ToA != NULL);
  EXPECT_TRUE(ToA == ToA2);
  EXPECT_FALSE(ToA == ToB);
  ToA2 = NULL;
  EXPECT_TRUE(ToA2 == NULL);
  ToA2 = 0;
  EXPECT_FALSE(static_cast<bool>(ToA2));

  std::vector<TIntEvent> Handlers{ToA, ToB};
  EXPECT_EQ(std::find(Handlers.begin(), Handlers.end(), TIntEvent(&B, &TBase::SetValue)) - Handlers.begin(), 1);
}
//---------------------------------------------------------------------------
TEST(TMethodPtr, MultipleInheritanceAndFunctions)
{
  TMultiple Object;
  TIntEvent Store(&Object, &TMultiple::Store);
  Store(5);
  EXPECT_EQ(Object.Other, 5);
  TIntEvent Set(&Object, &TBase::SetValue);
  Set(6);
  EXPECT_EQ(Object.Value, 6);

  int Data = 10;
  TMethodPtr<int(int)> Function(&Data, &CallbackFunction);
  EXPECT_EQ(Function(5), 15);
  EXPECT_EQ(Function.GetData(), &Data);
}
//---------------------------------------------------------------------------
// The core stores closures as a TMethod (two pointers) and rebuilds them
TEST(TMethodPtr, TMethodLayout)
{
  static_assert(sizeof(TIntEvent) == sizeof(TMethod));
  TMultiple Object;
  TIntEvent Event(&Object, &TMultiple::Store);
  TMethod & Method = *reinterpret_cast<TMethod *>(&Event);
  EXPECT_EQ(Method.Data, static_cast<void *>(&Object));
  void * Code = Method.Code;
  void * Data = Method.Data;

  TMethod Rebuilt = { Code, Data };
  TIntEvent Copy = *reinterpret_cast<TIntEvent *>(&Rebuilt);
  EXPECT_TRUE(Copy == Event);
  Copy(7);
  EXPECT_EQ(Object.Other, 7);

  // The same method of another object has the same code
  TMultiple Other;
  TIntEvent OtherEvent(&Other, &TMultiple::Store);
  EXPECT_EQ(reinterpret_cast<TMethod *>(&OtherEvent)->Code, Code);
  EXPECT_FALSE(OtherEvent == Event);
}
//---------------------------------------------------------------------------
TEST(TVarRec, Types)
{
  TDerived Object;
  UnicodeString U = L"u";
  AnsiString A = "a";
  EXPECT_EQ(TVarRec(1).VType, vtInteger);
  EXPECT_EQ(TVarRec(true).VType, vtBoolean);
  EXPECT_EQ(TVarRec('c').VType, vtChar);
  EXPECT_EQ(TVarRec(L'c').VType, vtWideChar);
  EXPECT_EQ(TVarRec(1u).VType, vtInt64);
  EXPECT_EQ(TVarRec(1L).VType, vtInt64);
  EXPECT_EQ(TVarRec(18446744073709551615ULL).VType, vtUInt64);
  EXPECT_EQ(TVarRec(1.5).VType, vtExtended);
  EXPECT_EQ(TVarRec("s").VType, vtPChar);
  EXPECT_EQ(TVarRec(L"s").VType, vtPWideChar);
  EXPECT_EQ(TVarRec(static_cast<void *>(nullptr)).VType, vtPointer);
  EXPECT_EQ(TVarRec(&Object).VType, vtObject);
  EXPECT_EQ(TVarRec(U).VType, vtUnicodeString);
  EXPECT_EQ(TVarRec(U).VUnicodeString, &U);
  EXPECT_EQ(TVarRec(A).VType, vtAnsiString);
  EXPECT_EQ(TVarRec(clBlue).VType, vtInteger);
  EXPECT_EQ(TVarRec(clBlue).VInteger, 2);

  OpenArray<int> Ints(1, 2, 3);
  EXPECT_EQ(Ints.GetHigh(), 2);
  EXPECT_EQ(static_cast<const int *>(Ints)[1], 2);
  auto Sum = [](const int * Values, int High)
    {
      int Result = 0;
      for (int Index = 0; Index <= High; Index++)
      {
        Result += Values[Index];
      }
      return Result;
    };
  EXPECT_EQ(Sum(OPENARRAY(int, (4, 5, 6))), 15);
  EXPECT_EQ(Sum(OPENARRAY(int, ())), 0);
  EXPECT_EQ(OpenArrayCount<TVarRec>(1, 2).GetHigh(), 1);
}
//---------------------------------------------------------------------------
TEST(Variant, Arithmetic)
{
  Variant A = 5;
  Variant B = 3;
  EXPECT_EQ(static_cast<int>(A + B), 8);
  EXPECT_EQ((A + B).Type(), varInt64);
  EXPECT_DOUBLE_EQ(static_cast<double>(A * Variant(2.5)), 12.5);
  EXPECT_DOUBLE_EQ(static_cast<double>(A / B), 5.0 / 3);
  // Converting floats to integers rounds half to even, as Delphi
  EXPECT_EQ(static_cast<__int64>(Variant(2.5)), 2);
  EXPECT_EQ(static_cast<__int64>(Variant(3.5)), 4);
  EXPECT_EQ(static_cast<__int64>(Variant(-2.6)), -3);
  // The core uses this to round a time difference to seconds
  TDateTime Difference(1.0 / 86400 * 59.6);
  EXPECT_EQ(static_cast<__int64>(SecsPerDay * static_cast<Variant>(Difference)), 60);
  EXPECT_EQ(static_cast<int>(Variant(true)), -1);
  EXPECT_DOUBLE_EQ(static_cast<double>(Variant(L"12") + Variant(1)), 13.0);
  EXPECT_EQ(Variant(L"a") + Variant(L"b"), Variant(L"ab"));
  EXPECT_THROW((void)static_cast<int>(Variant(L"x")), EVariantError);
}
//---------------------------------------------------------------------------
TEST(Variant, NullAndStrings)
{
  Variant N = Null();
  EXPECT_TRUE(VarIsNull(N));
  EXPECT_TRUE(N.IsNull());
  EXPECT_TRUE(N == Null());
  EXPECT_FALSE(N == Variant(0));
  EXPECT_EQ(VarToStr(N), L"");
  EXPECT_THROW((void)static_cast<UnicodeString>(N), EVariantError);
  EXPECT_TRUE(VarIsNull(N + Variant(1)));
  Variant Empty;
  EXPECT_TRUE(VarIsEmpty(Empty));
  EXPECT_EQ(VarToStr(Empty), L"");
  EXPECT_EQ(static_cast<int>(Empty), 0);

  OleVariant S = UnicodeString(L"value");
  EXPECT_TRUE(S == UnicodeString(L"value"));
  EXPECT_FALSE(S == UnicodeString(L"other"));
  UnicodeString Str = S;
  EXPECT_EQ(Str, L"value");
  EXPECT_EQ(VarToStr(Variant(42)), L"42");
  EXPECT_EQ(VarToStr(Variant(1.5)), L"1.5");
  EXPECT_EQ(VarToStr(Variant(true)), L"True");
  EXPECT_TRUE(Variant(L"5") == Variant(5));
  EXPECT_TRUE(VarIsStr(S));
  EXPECT_TRUE(VarIsNumeric(Variant(1.5)));
  EXPECT_TRUE(VarIsOrdinal(Variant(1)));
}
//---------------------------------------------------------------------------
TEST(Variant, Arrays)
{
  Variant Array = VarArrayCreate(OPENARRAY(int, (0, 1)), varLongWord);
  EXPECT_TRUE(VarIsArray(Array));
  EXPECT_EQ(Array.ArrayLowBound(), 0);
  EXPECT_EQ(Array.ArrayHighBound(), 1);
  EXPECT_EQ(static_cast<unsigned int>(Array.GetElement(1)), 0u);
  Array.PutElement(5u, 1);
  EXPECT_EQ(static_cast<unsigned int>(Array.GetElement(1)), 5u);
  Variant Copy = Array;
  Array.PutElement(7u, 1);
  EXPECT_EQ(static_cast<unsigned int>(Copy.GetElement(1)), 5u);
  Array.ArrayRedim(10);
  EXPECT_EQ(Array.ArrayHighBound(), 10);
  EXPECT_EQ(static_cast<unsigned int>(Array.GetElement(1)), 7u);
  EXPECT_EQ(static_cast<unsigned int>(Array.GetElement(10)), 0u);
  EXPECT_THROW(Array.GetElement(11), EVariantError);
  EXPECT_THROW(Array.PutElement(-1, 0), EVariantError);
  EXPECT_EQ(VarArrayHighBound(Array, 1), 10);
}
//---------------------------------------------------------------------------
TEST(System, StaticInitialization)
{
  EXPECT_EQ(StaticInitProbe, L"2026//x");
}
//---------------------------------------------------------------------------
TEST(System, Random)
{
  int Saved = RandSeed;
  RandSeed = 0;
  EXPECT_EQ(Random(100), 0);
  EXPECT_EQ(Random(100), 3);
  RandSeed = 0;
  EXPECT_EQ(Random(100), 0);
  double Value = Random();
  EXPECT_GE(Value, 0.0);
  EXPECT_LT(Value, 1.0);
  Randomize();
  RandSeed = Saved;
}
//---------------------------------------------------------------------------
TEST(System, CommandLine)
{
  EXPECT_TRUE(ParamStr(0).Pos(L"openscp_rtl_tests") > 0) << ParamStr(0);
  EXPECT_GE(ParamCount(), 0);
  ASSERT_NE(CmdLine, nullptr);
  EXPECT_TRUE(UnicodeString(CmdLine).Pos(L"openscp_rtl_tests") > 0);

  const char * Args[] = { "/bin/prog", "a b", "c\"d", "" };
  rtl::SetCommandLine(4, Args);
  EXPECT_EQ(ParamCount(), 3);
  EXPECT_EQ(ParamStr(1), L"a b");
  EXPECT_EQ(ParamStr(2), L"c\"d");
  EXPECT_EQ(ParamStr(3), L"");
  EXPECT_EQ(ParamStr(4), L"");
  EXPECT_EQ(UnicodeString(CmdLine), L"/bin/prog \"a b\" \"c\\\"d\" \"\"");
}
//---------------------------------------------------------------------------
TEST(System, StringRoutines)
{
  EXPECT_EQ(UTF8ToString(RawByteString("za\xC5\xBC")), L"za\u017C");
  EXPECT_EQ(UTF8ToString("\xC5\xBC"), L"\u017C");
  EXPECT_EQ(UTF8Encode(L"\u017C").Std(), "\xC5\xBC");
  EXPECT_EQ(StringOfChar(L'x', 2), L"xx");
  char Buffer[4] = { 'a', 'b', 'c', 'd' };
  Move(Buffer, Buffer + 1, 2);
  EXPECT_EQ(std::string(Buffer, 4), "aabd");
  FillChar(Buffer, 4, 'z');
  EXPECT_EQ(std::string(Buffer, 4), "zzzz");
}
