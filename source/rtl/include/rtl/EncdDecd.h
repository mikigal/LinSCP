// Soap.EncdDecd (Base64) and System.WideStrUtils (UTF-8 detection).
//---------------------------------------------------------------------------
#ifndef RtlEncdDecdH
#define RtlEncdDecdH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
//---------------------------------------------------------------------------
namespace Soap
{
namespace Encddecd
{
//---------------------------------------------------------------------------
// Lines of 76 characters separated by sLineBreak, as Delphi's encoder
System::UnicodeString EncodeBase64(const void * Input, int Size);
// Characters outside of the Base64 alphabet (line breaks) are skipped
System::TBytes DecodeBase64(const System::UnicodeString Input);
// The UTF-8 encoding of the string
System::UnicodeString EncodeString(const System::UnicodeString Input);
System::UnicodeString DecodeString(const System::UnicodeString Input);
//---------------------------------------------------------------------------
} // namespace Encddecd
} // namespace Soap
//---------------------------------------------------------------------------
namespace System
{
namespace Widestrutils
{
//---------------------------------------------------------------------------
enum TEncodeType { etUSASCII, etUTF8, etANSI };
TEncodeType DetectUTF8Encoding(const RawByteString & S);
bool IsUTF8String(const RawByteString & S);
bool HasExtendCharacter(const RawByteString & S);
//---------------------------------------------------------------------------
} // namespace Widestrutils
} // namespace System
//---------------------------------------------------------------------------
using namespace Soap::Encddecd;
using namespace System::Widestrutils;
//---------------------------------------------------------------------------
#endif
