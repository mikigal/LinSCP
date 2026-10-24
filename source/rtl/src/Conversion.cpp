#include <rtl/Conversion.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iconv.h>
#include <locale.h>
#include <wctype.h>
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
namespace
{
const wchar_t EscapeBase = 0xDC00;
const wchar_t ReplacementChar = 0xFFFD;

inline bool IsEscapedByte(uint32_t Ch)
{
  return (Ch >= 0xDC80) && (Ch <= 0xDCFF);
}

void AppendUtf8(std::string & Result, uint32_t Ch)
{
  if (Ch < 0x80)
  {
    Result.push_back(static_cast<char>(Ch));
  }
  else if (Ch < 0x800)
  {
    Result.push_back(static_cast<char>(0xC0 | (Ch >> 6)));
    Result.push_back(static_cast<char>(0x80 | (Ch & 0x3F)));
  }
  else if (Ch < 0x10000)
  {
    Result.push_back(static_cast<char>(0xE0 | (Ch >> 12)));
    Result.push_back(static_cast<char>(0x80 | ((Ch >> 6) & 0x3F)));
    Result.push_back(static_cast<char>(0x80 | (Ch & 0x3F)));
  }
  else
  {
    Result.push_back(static_cast<char>(0xF0 | (Ch >> 18)));
    Result.push_back(static_cast<char>(0x80 | ((Ch >> 12) & 0x3F)));
    Result.push_back(static_cast<char>(0x80 | ((Ch >> 6) & 0x3F)));
    Result.push_back(static_cast<char>(0x80 | (Ch & 0x3F)));
  }
}

class TIconv
{
public:
  TIconv(const char * To, const char * From)
  {
    FHandle = iconv_open(To, From);
  }

  ~TIconv()
  {
    if (IsValid())
    {
      iconv_close(FHandle);
    }
  }

  bool IsValid() const
  {
    return (FHandle != reinterpret_cast<iconv_t>(-1));
  }

  iconv_t Handle() const
  {
    return FHandle;
  }

private:
  iconv_t FHandle;
};

locale_t CaseLocale()
{
  static locale_t Locale = []()
  {
    const char * Names[] = { "C.UTF-8", "C.utf8", "en_US.UTF-8", "en_US.utf8" };
    for (const char * Name : Names)
    {
      locale_t Result = newlocale(LC_CTYPE_MASK, Name, static_cast<locale_t>(0));
      if (Result != static_cast<locale_t>(0))
      {
        return Result;
      }
    }
    return static_cast<locale_t>(0);
  }();
  return Locale;
}
} // namespace
//---------------------------------------------------------------------------
std::wstring DecodeUtf8(const char * Data, size_t Length)
{
  std::wstring Result;
  Result.reserve(Length);
  const unsigned char * P = reinterpret_cast<const unsigned char *>(Data);
  const unsigned char * End = P + Length;
  while (P < End)
  {
    unsigned char Lead = *P;
    if (Lead < 0x80)
    {
      Result.push_back(static_cast<wchar_t>(Lead));
      P++;
      continue;
    }

    int Need = 0;
    uint32_t Ch = 0;
    uint32_t Min = 0;
    if ((Lead >= 0xC2) && (Lead <= 0xDF))
    {
      Need = 1;
      Ch = Lead & 0x1F;
      Min = 0x80;
    }
    else if ((Lead >= 0xE0) && (Lead <= 0xEF))
    {
      Need = 2;
      Ch = Lead & 0x0F;
      Min = 0x800;
    }
    else if ((Lead >= 0xF0) && (Lead <= 0xF4))
    {
      Need = 3;
      Ch = Lead & 0x07;
      Min = 0x10000;
    }

    bool Valid = (Need > 0) && (End - P > Need);
    for (int Index = 1; Valid && (Index <= Need); Index++)
    {
      if ((P[Index] & 0xC0) != 0x80)
      {
        Valid = false;
      }
      else
      {
        Ch = (Ch << 6) | (P[Index] & 0x3F);
      }
    }
    if (Valid && ((Ch < Min) || (Ch > 0x10FFFF) || ((Ch >= 0xD800) && (Ch <= 0xDFFF))))
    {
      Valid = false;
    }

    if (Valid)
    {
      Result.push_back(static_cast<wchar_t>(Ch));
      P += Need + 1;
    }
    else
    {
      Result.push_back(static_cast<wchar_t>(EscapeBase + Lead));
      P++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::string EncodeUtf8(const wchar_t * Data, size_t Length)
{
  std::string Result;
  Result.reserve(Length);
  for (size_t Index = 0; Index < Length; Index++)
  {
    uint32_t Ch = static_cast<uint32_t>(Data[Index]);
    if (Ch < 0x80)
    {
      Result.push_back(static_cast<char>(Ch));
    }
    else if ((Ch >= 0xD800) && (Ch <= 0xDBFF) && (Index + 1 < Length) &&
             (static_cast<uint32_t>(Data[Index + 1]) >= 0xDC00) &&
             (static_cast<uint32_t>(Data[Index + 1]) <= 0xDFFF))
    {
      // A UTF-16 surrogate pair (escapes never follow a high surrogate)
      uint32_t Low = static_cast<uint32_t>(Data[Index + 1]);
      AppendUtf8(Result, 0x10000 + ((Ch - 0xD800) << 10) + (Low - 0xDC00));
      Index++;
    }
    else if (IsEscapedByte(Ch))
    {
      Result.push_back(static_cast<char>(Ch - EscapeBase));
    }
    else if (((Ch >= 0xD800) && (Ch <= 0xDFFF)) || (Ch > 0x10FFFF))
    {
      AppendUtf8(Result, ReplacementChar);
    }
    else
    {
      AppendUtf8(Result, Ch);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring DecodeUtf16(const char16_t * Data, size_t Length)
{
  std::wstring Result;
  Result.reserve(Length);
  for (size_t Index = 0; Index < Length; Index++)
  {
    uint32_t Unit = Data[Index];
    if ((Unit >= 0xD800) && (Unit <= 0xDBFF) && (Index + 1 < Length) &&
        (Data[Index + 1] >= 0xDC00) && (Data[Index + 1] <= 0xDFFF))
    {
      uint32_t Low = Data[Index + 1];
      Result.push_back(static_cast<wchar_t>(0x10000 + ((Unit - 0xD800) << 10) + (Low - 0xDC00)));
      Index++;
    }
    else
    {
      Result.push_back(static_cast<wchar_t>(Unit));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::u16string EncodeUtf16(const wchar_t * Data, size_t Length)
{
  std::u16string Result;
  Result.reserve(Length);
  for (size_t Index = 0; Index < Length; Index++)
  {
    uint32_t Ch = static_cast<uint32_t>(Data[Index]);
    if (Ch > 0x10FFFF)
    {
      Result.push_back(static_cast<char16_t>(ReplacementChar));
    }
    else if (Ch >= 0x10000)
    {
      Ch -= 0x10000;
      Result.push_back(static_cast<char16_t>(0xD800 + (Ch >> 10)));
      Result.push_back(static_cast<char16_t>(0xDC00 + (Ch & 0x3FF)));
    }
    else
    {
      Result.push_back(static_cast<char16_t>(Ch));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool IsUtf8CodePage(unsigned int CodePage)
{
  return (CodePage == 0) || (CodePage == RawByteCodePage) || (CodePage == 65001);
}
//---------------------------------------------------------------------------
std::string CodePageName(unsigned int CodePage)
{
  if (IsUtf8CodePage(CodePage))
  {
    return "UTF-8";
  }
  if ((CodePage >= 28591) && (CodePage <= 28606))
  {
    return "ISO-8859-" + std::to_string(CodePage - 28590);
  }
  switch (CodePage)
  {
    case 1200: return "UTF-16LE";
    case 1201: return "UTF-16BE";
    case 12000: return "UTF-32LE";
    case 12001: return "UTF-32BE";
    case 65000: return "UTF-7";
    case 20127: return "ASCII";
    case 20866: return "KOI8-R";
    case 21866: return "KOI8-U";
    case 10000: return "MACINTOSH";
    case 20932:
    case 51932: return "EUC-JP";
    case 50220:
    case 50221:
    case 50222: return "ISO-2022-JP";
    case 51949: return "EUC-KR";
    case 52936: return "HZ";
    case 54936: return "GB18030";
    case 936: return "GBK";
    case 950: return "BIG5";
  }
  return "CP" + std::to_string(CodePage);
}
//---------------------------------------------------------------------------
std::wstring DecodeCodePage(unsigned int CodePage, const char * Data, size_t Length)
{
  if (IsUtf8CodePage(CodePage))
  {
    return DecodeUtf8(Data, Length);
  }

  std::string Name = CodePageName(CodePage);
  TIconv Converter("WCHAR_T", Name.c_str());
  if (!Converter.IsValid())
  {
    return DecodeUtf8(Data, Length);
  }

  std::wstring Result;
  char * In = const_cast<char *>(Data);
  size_t InLeft = Length;
  wchar_t Buffer[256];
  while (InLeft > 0)
  {
    char * Out = reinterpret_cast<char *>(Buffer);
    size_t OutLeft = sizeof(Buffer);
    size_t Converted = iconv(Converter.Handle(), &In, &InLeft, &Out, &OutLeft);
    Result.append(Buffer, (sizeof(Buffer) - OutLeft) / sizeof(wchar_t));
    if (Converted == static_cast<size_t>(-1))
    {
      if (errno == E2BIG)
      {
        continue;
      }
      // Invalid or incomplete sequence: keep the byte, reversibly (see the header)
      Result.push_back(static_cast<wchar_t>(EscapeBase + static_cast<unsigned char>(*In)));
      In++;
      InLeft--;
      iconv(Converter.Handle(), nullptr, nullptr, nullptr, nullptr);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
std::string EncodeCodePage(unsigned int CodePage, const wchar_t * Data, size_t Length)
{
  if (IsUtf8CodePage(CodePage))
  {
    return EncodeUtf8(Data, Length);
  }

  std::string Name = CodePageName(CodePage);
  TIconv Converter(Name.c_str(), "WCHAR_T");
  if (!Converter.IsValid())
  {
    return EncodeUtf8(Data, Length);
  }

  std::string Result;
  char Buffer[1024];
  size_t Index = 0;
  while (Index < Length)
  {
    if (IsEscapedByte(static_cast<uint32_t>(Data[Index])))
    {
      Result.push_back(static_cast<char>(Data[Index] - EscapeBase));
      Index++;
      continue;
    }
    // Convert a run up to the next escaped byte
    size_t RunEnd = Index;
    while ((RunEnd < Length) && !IsEscapedByte(static_cast<uint32_t>(Data[RunEnd])))
    {
      RunEnd++;
    }
    char * In = reinterpret_cast<char *>(const_cast<wchar_t *>(Data + Index));
    size_t InLeft = (RunEnd - Index) * sizeof(wchar_t);
    while (InLeft > 0)
    {
      char * Out = Buffer;
      size_t OutLeft = sizeof(Buffer);
      size_t Converted = iconv(Converter.Handle(), &In, &InLeft, &Out, &OutLeft);
      Result.append(Buffer, sizeof(Buffer) - OutLeft);
      if (Converted == static_cast<size_t>(-1))
      {
        if (errno == E2BIG)
        {
          continue;
        }
        // Not representable in the code page
        Result.push_back('?');
        In += sizeof(wchar_t);
        InLeft -= sizeof(wchar_t);
        iconv(Converter.Handle(), nullptr, nullptr, nullptr, nullptr);
      }
    }
    // Flush any shift state
    char * Out = Buffer;
    size_t OutLeft = sizeof(Buffer);
    iconv(Converter.Handle(), nullptr, nullptr, &Out, &OutLeft);
    Result.append(Buffer, sizeof(Buffer) - OutLeft);
    Index = RunEnd;
  }
  return Result;
}
//---------------------------------------------------------------------------
wchar_t UpperChar(wchar_t Ch)
{
  if (Ch < 0x80)
  {
    return ((Ch >= L'a') && (Ch <= L'z')) ? static_cast<wchar_t>(Ch - (L'a' - L'A')) : Ch;
  }
  locale_t Locale = CaseLocale();
  return (Locale != static_cast<locale_t>(0)) ? static_cast<wchar_t>(towupper_l(Ch, Locale)) : Ch;
}
//---------------------------------------------------------------------------
wchar_t LowerChar(wchar_t Ch)
{
  if (Ch < 0x80)
  {
    return ((Ch >= L'A') && (Ch <= L'Z')) ? static_cast<wchar_t>(Ch + (L'a' - L'A')) : Ch;
  }
  locale_t Locale = CaseLocale();
  return (Locale != static_cast<locale_t>(0)) ? static_cast<wchar_t>(towlower_l(Ch, Locale)) : Ch;
}
//---------------------------------------------------------------------------
} // namespace rtl
