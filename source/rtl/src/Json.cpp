// System.JSON
//---------------------------------------------------------------------------
#include <rtl/Json.h>
#include <rtl/Printf.h>

#include <cstdio>
//---------------------------------------------------------------------------
namespace System
{
namespace Json
{
//---------------------------------------------------------------------------
namespace
{
UnicodeString Quote(const UnicodeString & S)
{
  std::wstring Result = L"\"";
  for (wchar_t Ch : S)
  {
    switch (Ch)
    {
      case L'"': Result += L"\\\""; break;
      case L'\\': Result += L"\\\\"; break;
      case L'\b': Result += L"\\b"; break;
      case L'\f': Result += L"\\f"; break;
      case L'\n': Result += L"\\n"; break;
      case L'\r': Result += L"\\r"; break;
      case L'\t': Result += L"\\t"; break;
      default:
        if (Ch < 0x20)
        {
          Result += Format(L"\\u%.4x", ARRAYOFCONST((static_cast<int>(Ch)))).LowerCase().Std();
        }
        else
        {
          Result.push_back(Ch);
        }
        break;
    }
  }
  Result += L"\"";
  return UnicodeString(std::move(Result));
}

class TParser
{
public:
  explicit TParser(const UnicodeString & Data) : FP(Data.c_str()), FEnd(Data.c_str() + Data.Length()) {}

  TJSONValue * ParseDocument()
  {
    std::unique_ptr<TJSONValue> Result(ParseValue());
    SkipBlanks();
    return (FP == FEnd) ? Result.release() : nullptr;
  }

private:
  void SkipBlanks()
  {
    while ((FP < FEnd) && ((*FP == L' ') || (*FP == L'\t') || (*FP == L'\n') || (*FP == L'\r')))
    {
      FP++;
    }
  }

  [[noreturn]] void Fail()
  {
    throw EConvertError(L"Invalid JSON");
  }

  void Expect(wchar_t Ch)
  {
    SkipBlanks();
    if ((FP >= FEnd) || (*FP != Ch))
    {
      Fail();
    }
    FP++;
  }

  bool Literal(const wchar_t * Text)
  {
    const wchar_t * P = FP;
    while (*Text != L'\0')
    {
      if ((P >= FEnd) || (*P != *Text))
      {
        return false;
      }
      P++;
      Text++;
    }
    FP = P;
    return true;
  }

  unsigned int Hex4()
  {
    unsigned int Result = 0;
    for (int Index = 0; Index < 4; Index++)
    {
      if (FP >= FEnd)
      {
        Fail();
      }
      wchar_t Ch = *FP++;
      Result <<= 4;
      if ((Ch >= L'0') && (Ch <= L'9'))
      {
        Result |= Ch - L'0';
      }
      else if ((Ch >= L'a') && (Ch <= L'f'))
      {
        Result |= Ch - L'a' + 10;
      }
      else if ((Ch >= L'A') && (Ch <= L'F'))
      {
        Result |= Ch - L'A' + 10;
      }
      else
      {
        Fail();
      }
    }
    return Result;
  }

  UnicodeString ParseString()
  {
    Expect(L'"');
    std::wstring Result;
    for (;;)
    {
      if (FP >= FEnd)
      {
        Fail();
      }
      wchar_t Ch = *FP++;
      if (Ch == L'"')
      {
        break;
      }
      if (Ch != L'\\')
      {
        Result.push_back(Ch);
        continue;
      }
      if (FP >= FEnd)
      {
        Fail();
      }
      Ch = *FP++;
      switch (Ch)
      {
        case L'"': Result.push_back(L'"'); break;
        case L'\\': Result.push_back(L'\\'); break;
        case L'/': Result.push_back(L'/'); break;
        case L'b': Result.push_back(L'\b'); break;
        case L'f': Result.push_back(L'\f'); break;
        case L'n': Result.push_back(L'\n'); break;
        case L'r': Result.push_back(L'\r'); break;
        case L't': Result.push_back(L'\t'); break;
        case L'u':
          {
            unsigned int Unit = Hex4();
            if ((Unit >= 0xD800) && (Unit <= 0xDBFF) && (FEnd - FP >= 6) && (FP[0] == L'\\') && (FP[1] == L'u'))
            {
              const wchar_t * Saved = FP;
              FP += 2;
              unsigned int Low = Hex4();
              if ((Low >= 0xDC00) && (Low <= 0xDFFF))
              {
                Unit = 0x10000 + ((Unit - 0xD800) << 10) + (Low - 0xDC00);
              }
              else
              {
                FP = Saved;
              }
            }
            Result.push_back(static_cast<wchar_t>(Unit));
          }
          break;
        default:
          Fail();
      }
    }
    return UnicodeString(std::move(Result));
  }

  TJSONValue * ParseNumber()
  {
    const wchar_t * Start = FP;
    if ((FP < FEnd) && (*FP == L'-'))
    {
      FP++;
    }
    auto Digits = [&]()
      {
        const wchar_t * DigitStart = FP;
        while ((FP < FEnd) && (*FP >= L'0') && (*FP <= L'9'))
        {
          FP++;
        }
        if (FP == DigitStart)
        {
          Fail();
        }
      };
    Digits();
    if ((FP < FEnd) && (*FP == L'.'))
    {
      FP++;
      Digits();
    }
    if ((FP < FEnd) && ((*FP == L'e') || (*FP == L'E')))
    {
      FP++;
      if ((FP < FEnd) && ((*FP == L'+') || (*FP == L'-')))
      {
        FP++;
      }
      Digits();
    }
    return new TJSONNumber(UnicodeString(Start, static_cast<int>(FP - Start)));
  }

  TJSONValue * ParseValue()
  {
    SkipBlanks();
    if (FP >= FEnd)
    {
      Fail();
    }
    switch (*FP)
    {
      case L'{':
        {
          FP++;
          std::unique_ptr<TJSONObject> Result(new TJSONObject());
          SkipBlanks();
          if ((FP < FEnd) && (*FP == L'}'))
          {
            FP++;
            return Result.release();
          }
          for (;;)
          {
            UnicodeString Name = ParseString();
            Expect(L':');
            TJSONValue * Value = ParseValue();
            Result->AddPair(Name, Value);
            SkipBlanks();
            if ((FP < FEnd) && (*FP == L','))
            {
              FP++;
              continue;
            }
            Expect(L'}');
            return Result.release();
          }
        }
      case L'[':
        {
          FP++;
          std::unique_ptr<TJSONArray> Result(new TJSONArray());
          SkipBlanks();
          if ((FP < FEnd) && (*FP == L']'))
          {
            FP++;
            return Result.release();
          }
          for (;;)
          {
            Result->Add(ParseValue());
            SkipBlanks();
            if ((FP < FEnd) && (*FP == L','))
            {
              FP++;
              continue;
            }
            Expect(L']');
            return Result.release();
          }
        }
      case L'"':
        return new TJSONString(ParseString());
      default:
        if (Literal(L"true"))
        {
          return new TJSONTrue();
        }
        if (Literal(L"false"))
        {
          return new TJSONFalse();
        }
        if (Literal(L"null"))
        {
          return new TJSONNull();
        }
        return ParseNumber();
    }
  }

  const wchar_t * FP;
  const wchar_t * FEnd;
};
} // namespace
//---------------------------------------------------------------------------
UnicodeString TJSONString::ToJSON()
{
  return Quote(FValue);
}
//---------------------------------------------------------------------------
TJSONNumber::TJSONNumber(double AValue) :
  TJSONString(UnicodeString(rtl::WithCLocale([AValue]()
    {
      char Buffer[64];
      snprintf(Buffer, sizeof(Buffer), "%.17g", AValue);
      return std::string(Buffer);
    }).c_str()))
{
}
//---------------------------------------------------------------------------
double TJSONNumber::GetAsDouble()
{
  TFormatSettings Settings = TFormatSettings::Invariant();
  return static_cast<double>(StrToFloat(Value(), Settings));
}
//---------------------------------------------------------------------------
TJSONArray::~TJSONArray()
{
  for (TJSONValue * Element : FElements)
  {
    delete Element;
  }
}
//---------------------------------------------------------------------------
TJSONArray * TJSONArray::Add(TJSONValue * Element)
{
  FElements.push_back(Element);
  return this;
}
//---------------------------------------------------------------------------
TJSONValue * TJSONArray::GetItem(int Index)
{
  return ((Index >= 0) && (Index < GetCount())) ? FElements[Index] : nullptr;
}
//---------------------------------------------------------------------------
UnicodeString TJSONArray::ToJSON()
{
  UnicodeString Result = L"[";
  for (size_t Index = 0; Index < FElements.size(); Index++)
  {
    if (Index > 0)
    {
      Result += L",";
    }
    Result += FElements[Index]->ToJSON();
  }
  return Result + L"]";
}
//---------------------------------------------------------------------------
TJSONObject::~TJSONObject()
{
  for (TJSONPair * Pair : FPairs)
  {
    delete Pair;
  }
}
//---------------------------------------------------------------------------
TJSONValue * TJSONObject::ParseJSONValue(const UnicodeString Data)
{
  try
  {
    TParser Parser(Data);
    return Parser.ParseDocument();
  }
  catch (EConvertError &)
  {
    return nullptr;
  }
}
//---------------------------------------------------------------------------
UnicodeString TJSONObject::ToJSON()
{
  UnicodeString Result = L"{";
  for (size_t Index = 0; Index < FPairs.size(); Index++)
  {
    if (Index > 0)
    {
      Result += L",";
    }
    Result += FPairs[Index]->JsonString->ToJSON() + L":" + FPairs[Index]->JsonValue->ToJSON();
  }
  return Result + L"}";
}
//---------------------------------------------------------------------------
TJSONObject * TJSONObject::AddPair(TJSONPair * Pair)
{
  FPairs.push_back(Pair);
  return this;
}
//---------------------------------------------------------------------------
TJSONObject * TJSONObject::AddPair(const UnicodeString Str, TJSONValue * Val)
{
  return AddPair(new TJSONPair(Str, Val));
}
//---------------------------------------------------------------------------
TJSONObject * TJSONObject::AddPair(const UnicodeString Str, const UnicodeString Val)
{
  return AddPair(new TJSONPair(Str, new TJSONString(Val)));
}
//---------------------------------------------------------------------------
TJSONPair * TJSONObject::Get(const UnicodeString Name)
{
  for (TJSONPair * Pair : FPairs)
  {
    if (Pair->JsonString->Value() == Name)
    {
      return Pair;
    }
  }
  return nullptr;
}
//---------------------------------------------------------------------------
TJSONPair * TJSONObject::GetPair(int Index)
{
  return ((Index >= 0) && (Index < GetCount())) ? FPairs[Index] : nullptr;
}
//---------------------------------------------------------------------------
TJSONValue * TJSONObject::GetValue(const UnicodeString Name)
{
  TJSONPair * Pair = Get(Name);
  return (Pair != nullptr) ? Pair->JsonValue : nullptr;
}
//---------------------------------------------------------------------------
} // namespace Json
} // namespace System
