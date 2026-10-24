// The System.JSON unit: a JSON parser with Delphi's TJSONValue classes.
//---------------------------------------------------------------------------
#ifndef RtlJsonH
#define RtlJsonH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <memory>
#include <vector>
//---------------------------------------------------------------------------
namespace System
{
namespace Json
{
//---------------------------------------------------------------------------
class TJSONPair;
//---------------------------------------------------------------------------
class TJSONValue : public TObject
{
public:
  virtual ~TJSONValue() {}
  // The value as a string: unescaped text of strings, the literal of numbers,
  // "true", "false" and "null"; empty for objects and arrays
  virtual UnicodeString Value() { return UnicodeString(); }
  virtual UnicodeString ToJSON() = 0;
  virtual UnicodeString ToString() override { return ToJSON(); }
  virtual bool Null() { return false; }
};
//---------------------------------------------------------------------------
class TJSONString : public TJSONValue
{
public:
  TJSONString() {}
  explicit TJSONString(const UnicodeString AValue) : FValue(AValue) {}
  virtual UnicodeString Value() override { return FValue; }
  virtual UnicodeString ToJSON() override;

private:
  UnicodeString FValue;
};
//---------------------------------------------------------------------------
class TJSONNumber : public TJSONString
{
public:
  explicit TJSONNumber(const UnicodeString ANumber) : TJSONString(ANumber) {}
  explicit TJSONNumber(int AValue) : TJSONString(IntToStr(AValue)) {}
  explicit TJSONNumber(__int64 AValue) : TJSONString(IntToStr(AValue)) {}
  explicit TJSONNumber(double AValue);
  virtual UnicodeString ToJSON() override { return Value(); }
  int GetAsInt() { return StrToInt(Value()); }
  __int64 GetAsInt64() { return StrToInt64(Value()); }
  double GetAsDouble();
  __declspec(property(get = GetAsInt)) int AsInt;
  __declspec(property(get = GetAsInt64)) __int64 AsInt64;
  __declspec(property(get = GetAsDouble)) double AsDouble;
};
//---------------------------------------------------------------------------
class TJSONBool : public TJSONValue
{
public:
  explicit TJSONBool(bool AValue) : FValue(AValue) {}
  virtual UnicodeString Value() override { return FValue ? L"true" : L"false"; }
  virtual UnicodeString ToJSON() override { return Value(); }
  bool GetAsBoolean() const { return FValue; }
  __declspec(property(get = GetAsBoolean)) bool AsBoolean;

private:
  bool FValue;
};
//---------------------------------------------------------------------------
class TJSONTrue : public TJSONBool
{
public:
  TJSONTrue() : TJSONBool(true) {}
};
//---------------------------------------------------------------------------
class TJSONFalse : public TJSONBool
{
public:
  TJSONFalse() : TJSONBool(false) {}
};
//---------------------------------------------------------------------------
class TJSONNull : public TJSONValue
{
public:
  virtual UnicodeString Value() override { return L"null"; }
  virtual UnicodeString ToJSON() override { return L"null"; }
  virtual bool Null() override { return true; }
};
//---------------------------------------------------------------------------
class TJSONArray : public TJSONValue
{
public:
  virtual ~TJSONArray();
  virtual UnicodeString ToJSON() override;
  TJSONArray * Add(TJSONValue * Element);
  int GetCount() const { return static_cast<int>(FElements.size()); }
  TJSONValue * GetItem(int Index);
  __declspec(property(get = GetCount)) int Count;
  __declspec(property(get = GetItem)) TJSONValue * Items[];

private:
  std::vector<TJSONValue *> FElements;
};
//---------------------------------------------------------------------------
class TJSONPair : public TObject
{
public:
  TJSONPair(TJSONString * AJsonString, TJSONValue * AJsonValue) : FJsonString(AJsonString), FJsonValue(AJsonValue) {}
  TJSONPair(const UnicodeString Str, TJSONValue * AJsonValue) : TJSONPair(new TJSONString(Str), AJsonValue) {}
  virtual ~TJSONPair() { delete FJsonString; delete FJsonValue; }
  TJSONString * GetJsonString() const { return FJsonString; }
  TJSONValue * GetJsonValue() const { return FJsonValue; }
  __declspec(property(get = GetJsonString)) TJSONString * JsonString;
  __declspec(property(get = GetJsonValue)) TJSONValue * JsonValue;

private:
  TJSONString * FJsonString;
  TJSONValue * FJsonValue;
};
//---------------------------------------------------------------------------
class TJSONObject : public TJSONValue
{
public:
  class TEnumerator
  {
  public:
    explicit TEnumerator(TJSONObject * AObject) : FObject(AObject) {}
    bool MoveNext() { return (++FIndex < FObject->GetCount()); }
    TJSONPair * GetCurrent() { return FObject->GetPair(FIndex); }
    __declspec(property(get = GetCurrent)) TJSONPair * Current;

  private:
    TJSONObject * FObject;
    int FIndex = -1;
  };

  virtual ~TJSONObject();
  // The caller owns the result; NULL if the text is not valid JSON
  static TJSONValue * ParseJSONValue(const UnicodeString Data);
  virtual UnicodeString ToJSON() override;
  TJSONObject * AddPair(TJSONPair * Pair);
  TJSONObject * AddPair(const UnicodeString Str, TJSONValue * Val);
  TJSONObject * AddPair(const UnicodeString Str, const UnicodeString Val);
  TJSONPair * Get(const UnicodeString Name);
  TEnumerator * GetEnumerator() { return new TEnumerator(this); }

  int GetCount() const { return static_cast<int>(FPairs.size()); }
  TJSONPair * GetPair(int Index);
  TJSONValue * GetValue(const UnicodeString Name);
  __declspec(property(get = GetCount)) int Count;
  __declspec(property(get = GetPair)) TJSONPair * Pairs[];
  __declspec(property(get = GetValue)) TJSONValue * Values[];

private:
  std::vector<TJSONPair *> FPairs;
};
//---------------------------------------------------------------------------
} // namespace Json
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Json;
//---------------------------------------------------------------------------
#endif
