// Soap.EncdDecd and System.WideStrUtils
//---------------------------------------------------------------------------
#include <rtl/EncdDecd.h>
//---------------------------------------------------------------------------
namespace Soap
{
namespace Encddecd
{
//---------------------------------------------------------------------------
namespace
{
const char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const int CharsPerLine = 76;
} // namespace
//---------------------------------------------------------------------------
System::UnicodeString EncodeBase64(const void * Input, int Size)
{
  const unsigned char * Data = static_cast<const unsigned char *>(Input);
  std::wstring Result;
  int LineLength = 0;
  auto Put = [&](wchar_t Ch)
    {
      if (LineLength == CharsPerLine)
      {
        Result += sLineBreak;
        LineLength = 0;
      }
      Result.push_back(Ch);
      LineLength++;
    };
  for (int Index = 0; Index < Size; Index += 3)
  {
    unsigned int Chunk = Data[Index] << 16;
    if (Index + 1 < Size)
    {
      Chunk |= Data[Index + 1] << 8;
    }
    if (Index + 2 < Size)
    {
      Chunk |= Data[Index + 2];
    }
    Put(Alphabet[(Chunk >> 18) & 0x3F]);
    Put(Alphabet[(Chunk >> 12) & 0x3F]);
    Put((Index + 1 < Size) ? Alphabet[(Chunk >> 6) & 0x3F] : L'=');
    Put((Index + 2 < Size) ? Alphabet[Chunk & 0x3F] : L'=');
  }
  return System::UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
System::TBytes DecodeBase64(const System::UnicodeString Input)
{
  std::string Bytes;
  unsigned int Chunk = 0;
  int Bits = 0;
  for (wchar_t Ch : Input)
  {
    int Value;
    if ((Ch >= L'A') && (Ch <= L'Z'))
    {
      Value = Ch - L'A';
    }
    else if ((Ch >= L'a') && (Ch <= L'z'))
    {
      Value = Ch - L'a' + 26;
    }
    else if ((Ch >= L'0') && (Ch <= L'9'))
    {
      Value = Ch - L'0' + 52;
    }
    else if (Ch == L'+')
    {
      Value = 62;
    }
    else if (Ch == L'/')
    {
      Value = 63;
    }
    else if (Ch == L'=')
    {
      break;
    }
    else
    {
      continue;
    }
    Chunk = (Chunk << 6) | static_cast<unsigned int>(Value);
    Bits += 6;
    if (Bits >= 8)
    {
      Bits -= 8;
      Bytes.push_back(static_cast<char>((Chunk >> Bits) & 0xFF));
    }
  }
  return System::Sysutils::BytesOf(Bytes.data(), static_cast<int>(Bytes.size()));
}
//---------------------------------------------------------------------------
System::UnicodeString EncodeString(const System::UnicodeString Input)
{
  System::UTF8String Utf8 = Input;
  return EncodeBase64(Utf8.c_str(), Utf8.Length());
}
//---------------------------------------------------------------------------
System::UnicodeString DecodeString(const System::UnicodeString Input)
{
  return System::Sysutils::TEncoding::UTF8->GetString(DecodeBase64(Input));
}
//---------------------------------------------------------------------------
} // namespace Encddecd
} // namespace Soap
//---------------------------------------------------------------------------
namespace System
{
namespace Widestrutils
{
//---------------------------------------------------------------------------
bool HasExtendCharacter(const RawByteString & S)
{
  for (char Ch : S)
  {
    if (static_cast<unsigned char>(Ch) >= 0x80)
    {
      return true;
    }
  }
  return false;
}
//---------------------------------------------------------------------------
bool IsUTF8String(const RawByteString & S)
{
  std::wstring Wide = rtl::DecodeUtf8(S.c_str(), S.Length());
  for (wchar_t Ch : Wide)
  {
    // Bytes that are not valid UTF-8 decode to escapes (see rtl/Conversion.h)
    if ((Ch >= 0xDC80) && (Ch <= 0xDCFF))
    {
      return false;
    }
  }
  return true;
}
//---------------------------------------------------------------------------
TEncodeType DetectUTF8Encoding(const RawByteString & S)
{
  if (!HasExtendCharacter(S))
  {
    return etUSASCII;
  }
  return IsUTF8String(S) ? etUTF8 : etANSI;
}
//---------------------------------------------------------------------------
} // namespace Widestrutils
} // namespace System
