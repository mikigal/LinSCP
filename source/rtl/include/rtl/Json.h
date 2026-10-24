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
  RTL_PROPERTY_GET(AsInt, GetAsInt)
  __declspec(property(get = __property_get_AsInt)) int AsInt;
  RTL_PROPERTY_GET(AsInt64, GetAsInt64)
  __declspec(property(get = __property_get_AsInt64)) __int64 AsInt64;
  RTL_PROPERTY_GET(AsDouble, GetAsDouble)
  __declspec(property(get = __property_get_AsDouble)) double AsDouble;
};
//---------------------------------------------------------------------------
class TJSONBool : public TJSONValue
{
public:
  explicit TJSONBool(bool AValue) : FValue(AValue) {}
  virtual UnicodeString Value() override { return FValue ? L"true" : L"false"; }
  virtual UnicodeString ToJSON() override { return Value(); }
  bool GetAsBoolean() const { return FValue; }
  RTL_PROPERTY_GET(AsBoolean, GetAsBoolean)
  __declspec(property(get = __property_get_AsBoolean)) bool AsBoolean;

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
  RTL_PROPERTY_GET(Count, GetCount)
  __declspec(property(get = __property_get_Count)) int Count;
  RTL_PROPERTY_GET(Items, GetItem)
  __declspec(property(get = __property_get_Items)) TJSONValue * Items[];

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
  RTL_PROPERTY_GET(JsonString, GetJsonString)
  __declspec(property(get = __property_get_JsonString)) TJSONString * JsonString;
  RTL_PROPERTY_GET(JsonValue, GetJsonValue)
  __declspec(property(get = __property_get_JsonValue)) TJSONValue * JsonValue;

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
    RTL_PROPERTY_GET(Current, GetCurrent)
    __declspec(property(get = __property_get_Current)) TJSONPair * Current;

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
  RTL_PROPERTY_GET(Count, GetCount)
  __declspec(property(get = __property_get_Count)) int Count;
  RTL_PROPERTY_GET(Pairs, GetPair)
  __declspec(property(get = __property_get_Pairs)) TJSONPair * Pairs[];
  RTL_PROPERTY_GET(Values, GetValue)
  __declspec(property(get = __property_get_Values)) TJSONValue * Values[];

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
