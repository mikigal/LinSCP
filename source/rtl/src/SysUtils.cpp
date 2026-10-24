// SysUtils: exceptions, resource strings, integer and string routines
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <cerrno>
#include <cstring>
#include <cwchar>
#include <limits>
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
const UnicodeString EmptyStr __attribute__((init_priority(200))) = UnicodeString();
//---------------------------------------------------------------------------
// Exceptions
//---------------------------------------------------------------------------
Exception::Exception(const UnicodeString Msg) :
  FMessage(Msg)
{
}
//---------------------------------------------------------------------------
Exception::Exception(const UnicodeString Msg, const TVarRec * Args, const int Args_High) :
  FMessage(Format(Msg, Args, Args_High))
{
}
//---------------------------------------------------------------------------
Exception::Exception(NativeUInt Ident) :
  FMessage(LoadStr(static_cast<int>(Ident)))
{
}
//---------------------------------------------------------------------------
Exception::Exception(PResStringRec ResStringRec) :
  FMessage(LoadResString(ResStringRec))
{
}
//---------------------------------------------------------------------------
Exception::Exception(NativeUInt Ident, const TVarRec * Args, const int Args_High) :
  FMessage(FmtLoadStr(static_cast<int>(Ident), Args, Args_High))
{
}
//---------------------------------------------------------------------------
Exception::Exception(PResStringRec ResStringRec, const TVarRec * Args, const int Args_High) :
  FMessage(Format(LoadResString(ResStringRec), Args, Args_High))
{
}
//---------------------------------------------------------------------------
Exception::Exception(const UnicodeString Msg, int AHelpContext) :
  FMessage(Msg),
  FHelpContext(AHelpContext)
{
}
//---------------------------------------------------------------------------
Exception::Exception(const UnicodeString Msg, const TVarRec * Args, const int Args_High, int AHelpContext) :
  FMessage(Format(Msg, Args, Args_High)),
  FHelpContext(AHelpContext)
{
}
//---------------------------------------------------------------------------
Exception::Exception(NativeUInt Ident, int AHelpContext) :
  FMessage(LoadStr(static_cast<int>(Ident))),
  FHelpContext(AHelpContext)
{
}
//---------------------------------------------------------------------------
Exception::Exception(PResStringRec ResStringRec, int AHelpContext) :
  FMessage(LoadResString(ResStringRec)),
  FHelpContext(AHelpContext)
{
}
//---------------------------------------------------------------------------
Exception::Exception(PResStringRec ResStringRec, const TVarRec * Args, const int Args_High, int AHelpContext) :
  FMessage(Format(LoadResString(ResStringRec), Args, Args_High)),
  FHelpContext(AHelpContext)
{
}
//---------------------------------------------------------------------------
Exception::Exception(NativeUInt Ident, const TVarRec * Args, const int Args_High, int AHelpContext) :
  FMessage(FmtLoadStr(static_cast<int>(Ident), Args, Args_High)),
  FHelpContext(AHelpContext)
{
}
//---------------------------------------------------------------------------
Exception::~Exception()
{
}
//---------------------------------------------------------------------------
UnicodeString Exception::ToString()
{
  return FMessage;
}
//---------------------------------------------------------------------------
void Abort()
{
  throw EAbort(L"Operation aborted");
}
//---------------------------------------------------------------------------
UnicodeString SysErrorMessage(unsigned int ErrorCode)
{
  // Windows errors without an errno equivalent
  switch (ErrorCode)
  {
    case ERROR_SUCCESS: return L"The operation completed successfully";
    case ERROR_INVALID_FUNCTION: return L"Incorrect function";
    case ERROR_PATH_NOT_FOUND: return L"The system cannot find the path specified";
    case ERROR_INVALID_DATA: return L"The data is invalid";
    case ERROR_NO_MORE_FILES: return L"There are no more files";
    case ERROR_HANDLE_EOF: return L"Reached the end of the file";
    case ERROR_INSUFFICIENT_BUFFER: return L"The data area passed to a system call is too small";
    case ERROR_INVALID_NAME: return L"The file name, directory name, or volume label syntax is incorrect";
    case ERROR_MORE_DATA: return L"More data is available";
    case WAIT_TIMEOUT: return L"The wait operation timed out";
    case ERROR_NO_MORE_ITEMS: return L"No more data is available";
    case ERROR_DIRECTORY: return L"The directory name is invalid";
    case ERROR_CANCELLED: return L"The operation was canceled by the user";
  }
  int Errno = rtl::WinErrorToErrno(ErrorCode);
  if (Errno == 0)
  {
    return Format(L"Unknown error %d", ARRAYOFCONST((static_cast<int>(ErrorCode))));
  }
  char Buffer[512];
  // GNU strerror_r returns the message, possibly not in the buffer
  const char * Message = strerror_r(Errno, Buffer, sizeof(Buffer));
  return UnicodeString(Message);
}
//---------------------------------------------------------------------------
void RaiseLastOSError(int LastError, const UnicodeString AdditionalInfo)
{
  UnicodeString Message;
  if (LastError != 0)
  {
    Message = Format(L"System Error.  Code: %d.\n%s", ARRAYOFCONST((LastError, SysErrorMessage(LastError))));
  }
  else
  {
    Message = L"A call to an OS function failed";
  }
  Message += AdditionalInfo;
  EOSError Error(Message);
  Error.ErrorCode = static_cast<unsigned int>(LastError);
  throw Error;
}
//---------------------------------------------------------------------------
void RaiseLastOSError(int LastError)
{
  RaiseLastOSError(LastError, UnicodeString());
}
//---------------------------------------------------------------------------
void RaiseLastOSError()
{
  RaiseLastOSError(static_cast<int>(GetLastError()));
}
//---------------------------------------------------------------------------
// Resource strings
//---------------------------------------------------------------------------
UnicodeString LoadStr(int Ident)
{
  return rtl::FindResourceString(Ident);
}
//---------------------------------------------------------------------------
UnicodeString FmtLoadStr(int Ident, const TVarRec * Args, const int Args_High)
{
  return Format(LoadStr(Ident), Args, Args_High);
}
//---------------------------------------------------------------------------
// Integers
//---------------------------------------------------------------------------
namespace
{
// Delphi's Val() for integers: leading blanks, a sign, decimal digits or
// hexadecimal after $, 0x or x. Hexadecimal values may use all bits.
bool ValInteger(const UnicodeString & S, bool Signed, int Bits, unsigned long long & Magnitude, bool & Negative)
{
  int Len = S.Length();
  int I = 1;
  while ((I <= Len) && (S[I] == L' '))
  {
    I++;
  }
  Negative = false;
  if ((I <= Len) && ((S[I] == L'-') || (S[I] == L'+')))
  {
    Negative = (S[I] == L'-');
    I++;
  }
  bool Hex = false;
  if ((I <= Len) && (S[I] == L'$'))
  {
    Hex = true;
    I++;
  }
  else if ((I <= Len) && ((S[I] == L'x') || (S[I] == L'X')))
  {
    Hex = true;
    I++;
  }
  else if ((I < Len) && (S[I] == L'0') && ((S[I + 1] == L'x') || (S[I + 1] == L'X')))
  {
    Hex = true;
    I += 2;
  }
  if (I > Len)
  {
    return false;
  }
  unsigned long long Limit = (Bits == 64) ? std::numeric_limits<unsigned long long>::max() : ((1ULL << Bits) - 1);
  unsigned long long Value = 0;
  for (; I <= Len; I++)
  {
    wchar_t Ch = S[I];
    unsigned int Digit;
    if ((Ch >= L'0') && (Ch <= L'9'))
    {
      Digit = Ch - L'0';
    }
    else if (Hex && (Ch >= L'a') && (Ch <= L'f'))
    {
      Digit = Ch - L'a' + 10;
    }
    else if (Hex && (Ch >= L'A') && (Ch <= L'F'))
    {
      Digit = Ch - L'A' + 10;
    }
    else
    {
      return false;
    }
    unsigned int Base = Hex ? 16 : 10;
    if (Value > (Limit - Digit) / Base)
    {
      return false;
    }
    Value = Value * Base + Digit;
  }
  if (!Hex && Signed)
  {
    unsigned long long Max = (1ULL << (Bits - 1)) - 1;
    if (Value > Max + (Negative ? 1 : 0))
    {
      return false;
    }
  }
  if (!Signed && Negative && (Value != 0))
  {
    return false;
  }
  Magnitude = Value;
  return true;
}

template <typename T>
bool TryStrToSigned(const UnicodeString & S, T & Value)
{
  unsigned long long Magnitude;
  bool Negative;
  if (!ValInteger(S, true, sizeof(T) * 8, Magnitude, Negative))
  {
    return false;
  }
  typedef std::make_unsigned_t<T> TUnsigned;
  TUnsigned Bits = static_cast<TUnsigned>(Magnitude);
  if (Negative)
  {
    Bits = static_cast<TUnsigned>(0 - Bits);
  }
  Value = static_cast<T>(Bits);
  return true;
}

[[noreturn]] void InvalidInteger(const UnicodeString & S)
{
  throw EConvertError(L"'" + S + L"' is not a valid integer value");
}

UnicodeString HexOf(unsigned long long Value, int Digits)
{
  static const wchar_t HexDigits[] = L"0123456789ABCDEF";
  std::wstring Result;
  do
  {
    Result.insert(Result.begin(), HexDigits[Value & 0xF]);
    Value >>= 4;
  }
  while (Value != 0);
  if (static_cast<int>(Result.size()) < Digits)
  {
    Result.insert(0, Digits - Result.size(), L'0');
  }
  return UnicodeString(Result);
}
} // namespace
//---------------------------------------------------------------------------
UnicodeString IntToStr(int Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString IntToStr(long Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString IntToStr(long long Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString IntToStr(unsigned int Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString IntToStr(unsigned long Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString IntToStr(unsigned long long Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString UIntToStr(unsigned int Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString UIntToStr(unsigned long Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString UIntToStr(unsigned long long Value) { return UnicodeString(std::to_wstring(Value)); }
UnicodeString IntToHex(int Value, int Digits) { return HexOf(static_cast<unsigned int>(Value), Digits); }
UnicodeString IntToHex(unsigned int Value, int Digits) { return HexOf(Value, Digits); }
UnicodeString IntToHex(long Value, int Digits) { return HexOf(static_cast<unsigned long>(Value), Digits); }
UnicodeString IntToHex(unsigned long Value, int Digits) { return HexOf(Value, Digits); }
UnicodeString IntToHex(long long Value, int Digits) { return HexOf(static_cast<unsigned long long>(Value), Digits); }
UnicodeString IntToHex(unsigned long long Value, int Digits) { return HexOf(Value, Digits); }
//---------------------------------------------------------------------------
bool TryStrToInt(const UnicodeString S, int & Value)
{
  return TryStrToSigned(S, Value);
}
//---------------------------------------------------------------------------
int StrToInt(const UnicodeString S)
{
  int Result;
  if (!TryStrToInt(S, Result))
  {
    InvalidInteger(S);
  }
  return Result;
}
//---------------------------------------------------------------------------
int StrToIntDef(const UnicodeString S, int Default)
{
  int Result;
  return TryStrToInt(S, Result) ? Result : Default;
}
//---------------------------------------------------------------------------
bool TryStrToInt64(const UnicodeString S, __int64 & Value)
{
  return TryStrToSigned(S, Value);
}
//---------------------------------------------------------------------------
__int64 StrToInt64(const UnicodeString S)
{
  __int64 Result;
  if (!TryStrToInt64(S, Result))
  {
    InvalidInteger(S);
  }
  return Result;
}
//---------------------------------------------------------------------------
__int64 StrToInt64Def(const UnicodeString S, const __int64 Default)
{
  __int64 Result;
  return TryStrToInt64(S, Result) ? Result : Default;
}
//---------------------------------------------------------------------------
bool TryStrToUInt64(const UnicodeString S, UInt64 & Value)
{
  unsigned long long Magnitude;
  bool Negative;
  if (!ValInteger(S, false, 64, Magnitude, Negative))
  {
    return false;
  }
  Value = Magnitude;
  return true;
}
//---------------------------------------------------------------------------
UInt64 StrToUInt64(const UnicodeString S)
{
  UInt64 Result;
  if (!TryStrToUInt64(S, Result))
  {
    InvalidInteger(S);
  }
  return Result;
}
//---------------------------------------------------------------------------
UInt64 StrToUInt64Def(const UnicodeString S, const UInt64 Default)
{
  UInt64 Result;
  return TryStrToUInt64(S, Result) ? Result : Default;
}
//---------------------------------------------------------------------------
// Booleans
//---------------------------------------------------------------------------
UnicodeString BoolToStr(bool B, bool UseBoolStrs)
{
  if (UseBoolStrs)
  {
    return B ? L"True" : L"False";
  }
  return B ? L"-1" : L"0";
}
//---------------------------------------------------------------------------
bool TryStrToBool(const UnicodeString S, bool & Value)
{
  if (SameText(S, L"True"))
  {
    Value = true;
    return true;
  }
  if (SameText(S, L"False"))
  {
    Value = false;
    return true;
  }
  Extended Number;
  if (TryStrToFloat(S, Number))
  {
    Value = (Number != 0);
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
bool StrToBool(const UnicodeString S)
{
  bool Result;
  if (!TryStrToBool(S, Result))
  {
    throw EConvertError(L"'" + S + L"' is not a valid boolean value");
  }
  return Result;
}
//---------------------------------------------------------------------------
bool StrToBoolDef(const UnicodeString S, const bool Default)
{
  bool Result;
  return TryStrToBool(S, Result) ? Result : Default;
}
//---------------------------------------------------------------------------
// Strings
//---------------------------------------------------------------------------
UnicodeString Trim(const UnicodeString S)
{
  const std::wstring & Data = S.Std();
  size_t Start = 0;
  size_t End = Data.size();
  while ((Start < End) && (Data[Start] <= L' '))
  {
    Start++;
  }
  while ((End > Start) && (Data[End - 1] <= L' '))
  {
    End--;
  }
  return UnicodeString(Data.substr(Start, End - Start));
}
//---------------------------------------------------------------------------
UnicodeString TrimLeft(const UnicodeString S)
{
  const std::wstring & Data = S.Std();
  size_t Start = 0;
  while ((Start < Data.size()) && (Data[Start] <= L' '))
  {
    Start++;
  }
  return UnicodeString(Data.substr(Start));
}
//---------------------------------------------------------------------------
UnicodeString TrimRight(const UnicodeString S)
{
  const std::wstring & Data = S.Std();
  size_t End = Data.size();
  while ((End > 0) && (Data[End - 1] <= L' '))
  {
    End--;
  }
  return UnicodeString(Data.substr(0, End));
}
//---------------------------------------------------------------------------
UnicodeString UpperCase(const UnicodeString S)
{
  std::wstring Result = S.Std();
  for (wchar_t & Ch : Result)
  {
    if ((Ch >= L'a') && (Ch <= L'z'))
    {
      Ch = static_cast<wchar_t>(Ch - (L'a' - L'A'));
    }
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
UnicodeString LowerCase(const UnicodeString S)
{
  std::wstring Result = S.Std();
  for (wchar_t & Ch : Result)
  {
    if ((Ch >= L'A') && (Ch <= L'Z'))
    {
      Ch = static_cast<wchar_t>(Ch + (L'a' - L'A'));
    }
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
UnicodeString AnsiUpperCase(const UnicodeString S)
{
  std::wstring Result = S.Std();
  for (wchar_t & Ch : Result)
  {
    Ch = rtl::UpperChar(Ch);
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
UnicodeString AnsiLowerCase(const UnicodeString S)
{
  std::wstring Result = S.Std();
  for (wchar_t & Ch : Result)
  {
    Ch = rtl::LowerChar(Ch);
  }
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
int CompareStr(const UnicodeString S1, const UnicodeString S2)
{
  const std::wstring & A = S1.Std();
  const std::wstring & B = S2.Std();
  size_t Len = std::min(A.size(), B.size());
  for (size_t Index = 0; Index < Len; Index++)
  {
    if (A[Index] != B[Index])
    {
      return static_cast<int>(A[Index]) - static_cast<int>(B[Index]);
    }
  }
  return static_cast<int>(A.size()) - static_cast<int>(B.size());
}
//---------------------------------------------------------------------------
bool SameStr(const UnicodeString S1, const UnicodeString S2)
{
  return (S1 == S2);
}
//---------------------------------------------------------------------------
int CompareText(const UnicodeString S1, const UnicodeString S2)
{
  const std::wstring & A = S1.Std();
  const std::wstring & B = S2.Std();
  size_t Len = std::min(A.size(), B.size());
  for (size_t Index = 0; Index < Len; Index++)
  {
    wchar_t ChA = A[Index];
    wchar_t ChB = B[Index];
    if ((ChA >= L'a') && (ChA <= L'z'))
    {
      ChA = static_cast<wchar_t>(ChA - (L'a' - L'A'));
    }
    if ((ChB >= L'a') && (ChB <= L'z'))
    {
      ChB = static_cast<wchar_t>(ChB - (L'a' - L'A'));
    }
    if (ChA != ChB)
    {
      return static_cast<int>(ChA) - static_cast<int>(ChB);
    }
  }
  return static_cast<int>(A.size()) - static_cast<int>(B.size());
}
//---------------------------------------------------------------------------
bool SameText(const UnicodeString S1, const UnicodeString S2)
{
  return (S1.Length() == S2.Length()) && (CompareText(S1, S2) == 0);
}
//---------------------------------------------------------------------------
namespace
{
int Sign(long long Value)
{
  return (Value > 0) ? 1 : ((Value < 0) ? -1 : 0);
}

int CompareFolded(const UnicodeString & S1, const UnicodeString & S2)
{
  const std::wstring & A = S1.Std();
  const std::wstring & B = S2.Std();
  size_t Len = std::min(A.size(), B.size());
  for (size_t Index = 0; Index < Len; Index++)
  {
    wchar_t ChA = rtl::UpperChar(A[Index]);
    wchar_t ChB = rtl::UpperChar(B[Index]);
    if (ChA != ChB)
    {
      return Sign(static_cast<long long>(ChA) - static_cast<long long>(ChB));
    }
  }
  return Sign(static_cast<long long>(A.size()) - static_cast<long long>(B.size()));
}
} // namespace
//---------------------------------------------------------------------------
int AnsiCompareStr(const UnicodeString S1, const UnicodeString S2)
{
  int Result = CompareFolded(S1, S2);
  if (Result == 0)
  {
    // Equal ignoring case: lowercase first, like Windows' linguistic order
    const std::wstring & A = S1.Std();
    const std::wstring & B = S2.Std();
    for (size_t Index = 0; Index < A.size(); Index++)
    {
      if (A[Index] != B[Index])
      {
        bool LowerA = (rtl::UpperChar(A[Index]) != A[Index]);
        bool LowerB = (rtl::UpperChar(B[Index]) != B[Index]);
        if (LowerA != LowerB)
        {
          return LowerA ? -1 : 1;
        }
        return Sign(static_cast<long long>(A[Index]) - static_cast<long long>(B[Index]));
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int AnsiCompareText(const UnicodeString S1, const UnicodeString S2)
{
  return CompareFolded(S1, S2);
}
//---------------------------------------------------------------------------
bool AnsiSameStr(const UnicodeString S1, const UnicodeString S2)
{
  return (S1 == S2);
}
//---------------------------------------------------------------------------
bool AnsiSameText(const UnicodeString S1, const UnicodeString S2)
{
  return (AnsiCompareText(S1, S2) == 0);
}
//---------------------------------------------------------------------------
int AnsiCompareFileName(const UnicodeString S1, const UnicodeString S2)
{
  return Sign(CompareStr(S1, S2));
}
//---------------------------------------------------------------------------
bool SameFileName(const UnicodeString S1, const UnicodeString S2)
{
  return (S1 == S2);
}
//---------------------------------------------------------------------------
int AnsiPos(const UnicodeString Substr, const UnicodeString S)
{
  return S.Pos(Substr);
}
//---------------------------------------------------------------------------
UnicodeString AnsiQuotedStr(const UnicodeString S, wchar_t Quote)
{
  std::wstring Result(1, Quote);
  for (wchar_t Ch : S)
  {
    Result.push_back(Ch);
    if (Ch == Quote)
    {
      Result.push_back(Ch);
    }
  }
  Result.push_back(Quote);
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
UnicodeString QuotedStr(const UnicodeString S)
{
  return AnsiQuotedStr(S, L'\'');
}
//---------------------------------------------------------------------------
UnicodeString AnsiExtractQuotedStr(const wchar_t *& Src, wchar_t Quote)
{
  if ((Src == nullptr) || (*Src != Quote))
  {
    return UnicodeString();
  }
  std::wstring Result;
  const wchar_t * P = Src + 1;
  while (*P != L'\0')
  {
    if (*P == Quote)
    {
      if (P[1] == Quote)
      {
        Result.push_back(Quote);
        P += 2;
        continue;
      }
      P++;
      break;
    }
    Result.push_back(*P);
    P++;
  }
  Src = P;
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
UnicodeString AnsiExtractQuotedStr(wchar_t *& Src, wchar_t Quote)
{
  const wchar_t * P = Src;
  UnicodeString Result = AnsiExtractQuotedStr(P, Quote);
  Src = const_cast<wchar_t *>(P);
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString AnsiDequotedStr(const UnicodeString S, wchar_t AQuote)
{
  const wchar_t * P = S.c_str();
  UnicodeString Result = AnsiExtractQuotedStr(P, AQuote);
  if (Result.IsEmpty() || (*P != L'\0'))
  {
    // Not a properly quoted string
    return S;
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString DequotedStr(const UnicodeString S, wchar_t AQuote)
{
  return AnsiDequotedStr(S, AQuote);
}
//---------------------------------------------------------------------------
UnicodeString StringReplace(const UnicodeString S, const UnicodeString OldPattern,
  const UnicodeString NewPattern, TReplaceFlags Flags)
{
  if (OldPattern.IsEmpty())
  {
    return S;
  }
  bool IgnoreCase = Flags.Contains(rfIgnoreCase);
  const std::wstring & Source = S.Std();
  std::wstring Searched = IgnoreCase ? AnsiUpperCase(S).Std() : Source;
  std::wstring Pattern = IgnoreCase ? AnsiUpperCase(OldPattern).Std() : OldPattern.Std();
  std::wstring Result;
  size_t From = 0;
  for (;;)
  {
    size_t P = Searched.find(Pattern, From);
    if (P == std::wstring::npos)
    {
      break;
    }
    Result.append(Source, From, P - From);
    Result += NewPattern.Std();
    From = P + Pattern.size();
    if (!Flags.Contains(rfReplaceAll))
    {
      break;
    }
  }
  Result.append(Source, From, std::wstring::npos);
  return UnicodeString(std::move(Result));
}
//---------------------------------------------------------------------------
bool IsDelimiter(const UnicodeString Delimiters, const UnicodeString S, int Index)
{
  if ((Index < 1) || (Index > S.Length()))
  {
    return false;
  }
  return (Delimiters.Std().find(S.Std()[Index - 1]) != std::wstring::npos);
}
//---------------------------------------------------------------------------
int FindDelimiter(const UnicodeString Delimiters, const UnicodeString S, int StartIdx)
{
  const std::wstring & Data = S.Std();
  for (int Index = std::max(StartIdx, 1); Index <= static_cast<int>(Data.size()); Index++)
  {
    wchar_t Ch = Data[Index - 1];
    if ((Ch != L'\0') && (Delimiters.Std().find(Ch) != std::wstring::npos))
    {
      return Index;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
int LastDelimiter(const UnicodeString Delimiters, const UnicodeString S)
{
  const std::wstring & Data = S.Std();
  for (int Index = static_cast<int>(Data.size()); Index >= 1; Index--)
  {
    wchar_t Ch = Data[Index - 1];
    if ((Ch != L'\0') && (Delimiters.Std().find(Ch) != std::wstring::npos))
    {
      return Index;
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
bool IsPathDelimiter(const UnicodeString S, int Index)
{
  return (Index >= 1) && (Index <= S.Length()) && (S.Std()[Index - 1] == PathDelim);
}
//---------------------------------------------------------------------------
// C strings
//---------------------------------------------------------------------------
size_t StrLen(const wchar_t * Str)
{
  return wcslen(Str);
}
//---------------------------------------------------------------------------
wchar_t * StrEnd(const wchar_t * Str)
{
  return const_cast<wchar_t *>(Str + wcslen(Str));
}
//---------------------------------------------------------------------------
wchar_t * StrCopy(wchar_t * Dest, const wchar_t * Source)
{
  return wcscpy(Dest, Source);
}
//---------------------------------------------------------------------------
wchar_t * StrLCopy(wchar_t * Dest, const wchar_t * Source, size_t MaxLen)
{
  size_t Index = 0;
  while ((Index < MaxLen) && (Source[Index] != L'\0'))
  {
    Dest[Index] = Source[Index];
    Index++;
  }
  Dest[Index] = L'\0';
  return Dest;
}
//---------------------------------------------------------------------------
wchar_t * StrPCopy(wchar_t * Dest, const UnicodeString Source)
{
  return StrLCopy(Dest, Source.c_str(), Source.Length());
}
//---------------------------------------------------------------------------
wchar_t * StrPLCopy(wchar_t * Dest, const UnicodeString Source, size_t MaxLen)
{
  return StrLCopy(Dest, Source.c_str(), MaxLen);
}
//---------------------------------------------------------------------------
int StrComp(const wchar_t * Str1, const wchar_t * Str2)
{
  return StrLComp(Str1, Str2, std::numeric_limits<size_t>::max());
}
//---------------------------------------------------------------------------
int StrLComp(const wchar_t * Str1, const wchar_t * Str2, size_t MaxLen)
{
  for (size_t Index = 0; Index < MaxLen; Index++)
  {
    if ((Str1[Index] != Str2[Index]) || (Str1[Index] == L'\0'))
    {
      return static_cast<int>(Str1[Index]) - static_cast<int>(Str2[Index]);
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
int StrIComp(const wchar_t * Str1, const wchar_t * Str2)
{
  return StrLIComp(Str1, Str2, std::numeric_limits<size_t>::max());
}
//---------------------------------------------------------------------------
int StrLIComp(const wchar_t * Str1, const wchar_t * Str2, size_t MaxLen)
{
  for (size_t Index = 0; Index < MaxLen; Index++)
  {
    wchar_t Ch1 = rtl::UpperChar(Str1[Index]);
    wchar_t Ch2 = rtl::UpperChar(Str2[Index]);
    if ((Ch1 != Ch2) || (Ch1 == L'\0'))
    {
      return static_cast<int>(Ch1) - static_cast<int>(Ch2);
    }
  }
  return 0;
}
//---------------------------------------------------------------------------
wchar_t * StrScan(const wchar_t * Str, wchar_t Chr)
{
  // Like Delphi, finds the terminator when looking for #0
  return const_cast<wchar_t *>(wcschr(Str, Chr));
}
//---------------------------------------------------------------------------
wchar_t * StrRScan(const wchar_t * Str, wchar_t Chr)
{
  return const_cast<wchar_t *>(wcsrchr(Str, Chr));
}
//---------------------------------------------------------------------------
wchar_t * StrPos(const wchar_t * Str1, const wchar_t * Str2)
{
  return const_cast<wchar_t *>(wcsstr(Str1, Str2));
}
//---------------------------------------------------------------------------
UnicodeString StrPas(const wchar_t * Str)
{
  return UnicodeString(Str);
}
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
