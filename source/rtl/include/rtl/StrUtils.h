// The System.StrUtils unit.
//---------------------------------------------------------------------------
#ifndef RtlStrUtilsH
#define RtlStrUtilsH
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>
#include <rtl/Types.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Strutils
{
//---------------------------------------------------------------------------
bool ContainsStr(const UnicodeString AText, const UnicodeString ASubText);
bool ContainsText(const UnicodeString AText, const UnicodeString ASubText);
bool StartsStr(const UnicodeString ASubText, const UnicodeString AText);
bool StartsText(const UnicodeString ASubText, const UnicodeString AText);
bool EndsStr(const UnicodeString ASubText, const UnicodeString AText);
bool EndsText(const UnicodeString ASubText, const UnicodeString AText);
bool AnsiContainsStr(const UnicodeString AText, const UnicodeString ASubText);
bool AnsiContainsText(const UnicodeString AText, const UnicodeString ASubText);
bool AnsiStartsStr(const UnicodeString ASubText, const UnicodeString AText);
bool AnsiStartsText(const UnicodeString ASubText, const UnicodeString AText);
bool AnsiEndsStr(const UnicodeString ASubText, const UnicodeString AText);
bool AnsiEndsText(const UnicodeString ASubText, const UnicodeString AText);

UnicodeString ReplaceStr(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText);
UnicodeString ReplaceText(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText);
UnicodeString AnsiReplaceStr(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText);
UnicodeString AnsiReplaceText(const UnicodeString AText, const UnicodeString AFromText, const UnicodeString AToText);

UnicodeString LeftStr(const UnicodeString AText, const int ACount);
UnicodeString RightStr(const UnicodeString AText, const int ACount);
UnicodeString MidStr(const UnicodeString AText, const int AStart, const int ACount);
UnicodeString AnsiLeftStr(const UnicodeString AText, const int ACount);
UnicodeString AnsiRightStr(const UnicodeString AText, const int ACount);
UnicodeString AnsiMidStr(const UnicodeString AText, const int AStart, const int ACount);

int PosEx(const UnicodeString SubStr, const UnicodeString S, int Offset = 1);

// Open arrays: values and the index of the last one
int IndexStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
int IndexText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
bool MatchStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
bool MatchText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
int AnsiIndexStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
int AnsiIndexText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
bool AnsiMatchStr(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);
bool AnsiMatchText(const UnicodeString AText, const UnicodeString * AValues, const int AValues_High);

UnicodeString DupeString(const UnicodeString AText, int ACount);
UnicodeString ReverseString(const UnicodeString AText);
UnicodeString StuffString(const UnicodeString AText, int AStart, int ALength, const UnicodeString ASubText);
UnicodeString IfThen(bool AValue, const UnicodeString ATrue, const UnicodeString AFalse = UnicodeString());
TStringDynArray SplitString(const UnicodeString S, const UnicodeString Delimiters);
//---------------------------------------------------------------------------
} // namespace Strutils
} // namespace System
//---------------------------------------------------------------------------
using namespace System::Strutils;
//---------------------------------------------------------------------------
#endif
