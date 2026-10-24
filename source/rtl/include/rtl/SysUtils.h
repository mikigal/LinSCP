// The SysUtils unit (C++Builder's System.SysUtils.hpp): exceptions, string
// conversion and formatting, file names, files, dates.
//
// Semantics follow Delphi; where Delphi distinguishes Windows from POSIX
// (paths, file attributes), the POSIX behaviour is implemented.
//---------------------------------------------------------------------------
#ifndef RtlSysUtilsH
#define RtlSysUtilsH
//---------------------------------------------------------------------------
#include <rtl/System.h>
//---------------------------------------------------------------------------
namespace System
{
namespace Sysutils
{
//---------------------------------------------------------------------------
// Exceptions
//---------------------------------------------------------------------------
class Exception : public TObject
{
public:
  Exception(const UnicodeString Msg);
  Exception(const UnicodeString Msg, const TVarRec * Args, const int Args_High);
  Exception(NativeUInt Ident);
  Exception(PResStringRec ResStringRec);
  Exception(NativeUInt Ident, const TVarRec * Args, const int Args_High);
  Exception(PResStringRec ResStringRec, const TVarRec * Args, const int Args_High);
  Exception(const UnicodeString Msg, int AHelpContext);
  Exception(const UnicodeString Msg, const TVarRec * Args, const int Args_High, int AHelpContext);
  Exception(NativeUInt Ident, int AHelpContext);
  Exception(PResStringRec ResStringRec, int AHelpContext);
  Exception(PResStringRec ResStringRec, const TVarRec * Args, const int Args_High, int AHelpContext);
  Exception(NativeUInt Ident, const TVarRec * Args, const int Args_High, int AHelpContext);
  Exception(const Exception & Src) = default;
  Exception & operator =(const Exception & Src) = default;
  virtual ~Exception();

  virtual UnicodeString ToString() override;

  UnicodeString GetMessage() const { return FMessage; }
  void SetMessage(const UnicodeString & Value) { FMessage = Value; }
  int GetHelpContext() const { return FHelpContext; }
  void SetHelpContext(int Value) { FHelpContext = Value; }
  Exception * GetBaseException() { return this; }
  Exception * GetInnerException() const { return nullptr; }
  UnicodeString GetStackTrace() const { return UnicodeString(); }

  RTL_PROPERTY_GET(Message, GetMessage) RTL_PROPERTY_PUT(Message, SetMessage, UnicodeString)
  __declspec(property(get = __property_get_Message, put = __property_set_Message)) UnicodeString Message;
  RTL_PROPERTY_GET(HelpContext, GetHelpContext) RTL_PROPERTY_PUT(HelpContext, SetHelpContext, int)
  __declspec(property(get = __property_get_HelpContext, put = __property_set_HelpContext)) int HelpContext;
  RTL_PROPERTY_GET(BaseException, GetBaseException)
  __declspec(property(get = __property_get_BaseException)) Exception * BaseException;
  RTL_PROPERTY_GET(InnerException, GetInnerException)
  __declspec(property(get = __property_get_InnerException)) Exception * InnerException;
  RTL_PROPERTY_GET(StackTrace, GetStackTrace)
  __declspec(property(get = __property_get_StackTrace)) UnicodeString StackTrace;

private:
  UnicodeString FMessage;
  int FHelpContext = 0;
};
//---------------------------------------------------------------------------
#define RTL_DECLARE_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
    typedef BASE TBase; \
  public: \
    using TBase::TBase; \
  };
//---------------------------------------------------------------------------
RTL_DECLARE_EXCEPTION(EAbort, Exception)
RTL_DECLARE_EXCEPTION(EHeapException, Exception)
RTL_DECLARE_EXCEPTION(EOutOfMemory, EHeapException)
RTL_DECLARE_EXCEPTION(EInvalidPointer, EHeapException)
RTL_DECLARE_EXCEPTION(EExternal, Exception)
RTL_DECLARE_EXCEPTION(EExternalException, EExternal)
RTL_DECLARE_EXCEPTION(EIntError, EExternal)
RTL_DECLARE_EXCEPTION(EDivByZero, EIntError)
RTL_DECLARE_EXCEPTION(ERangeError, EIntError)
RTL_DECLARE_EXCEPTION(EIntOverflow, EIntError)
RTL_DECLARE_EXCEPTION(EMathError, EExternal)
RTL_DECLARE_EXCEPTION(EInvalidOp, EMathError)
RTL_DECLARE_EXCEPTION(EZeroDivide, EMathError)
RTL_DECLARE_EXCEPTION(EOverflow, EMathError)
RTL_DECLARE_EXCEPTION(EUnderflow, EMathError)
RTL_DECLARE_EXCEPTION(EAccessViolation, EExternal)
RTL_DECLARE_EXCEPTION(EPrivilege, EExternal)
RTL_DECLARE_EXCEPTION(EStackOverflow, EExternal)
RTL_DECLARE_EXCEPTION(EControlC, EExternal)
RTL_DECLARE_EXCEPTION(EInvalidCast, Exception)
RTL_DECLARE_EXCEPTION(EConvertError, Exception)
RTL_DECLARE_EXCEPTION(EVariantError, Exception)
RTL_DECLARE_EXCEPTION(EAssertionFailed, Exception)
RTL_DECLARE_EXCEPTION(EAbstractError, Exception)
RTL_DECLARE_EXCEPTION(EIntfCastError, Exception)
RTL_DECLARE_EXCEPTION(ESafecallException, Exception)
RTL_DECLARE_EXCEPTION(EMonitor, Exception)
RTL_DECLARE_EXCEPTION(ENoMonitorSupportException, Exception)
RTL_DECLARE_EXCEPTION(EProgrammerNotFound, Exception)
RTL_DECLARE_EXCEPTION(ENotImplemented, Exception)
RTL_DECLARE_EXCEPTION(EObjectDisposed, Exception)
RTL_DECLARE_EXCEPTION(EArgumentException, Exception)
RTL_DECLARE_EXCEPTION(EArgumentOutOfRangeException, EArgumentException)
RTL_DECLARE_EXCEPTION(EArgumentNilException, EArgumentException)
RTL_DECLARE_EXCEPTION(EPathTooLongException, Exception)
RTL_DECLARE_EXCEPTION(ENotSupportedException, Exception)
RTL_DECLARE_EXCEPTION(EDirectoryNotFoundException, Exception)
RTL_DECLARE_EXCEPTION(EFileNotFoundException, Exception)
RTL_DECLARE_EXCEPTION(EPathNotFoundException, Exception)
RTL_DECLARE_EXCEPTION(EInvalidOpException, Exception)
RTL_DECLARE_EXCEPTION(ENoConstructException, Exception)
RTL_DECLARE_EXCEPTION(EOperationCancelled, Exception)
RTL_DECLARE_EXCEPTION(EEncodingError, Exception)
//---------------------------------------------------------------------------
class EInOutError : public Exception
{
public:
  using Exception::Exception;
  int ErrorCode = 0;
};
//---------------------------------------------------------------------------
class EOSError : public Exception
{
public:
  using Exception::Exception;
  unsigned int GetErrorCode() const { return FErrorCode; }
  void SetErrorCode(unsigned int Value) { FErrorCode = Value; }
  RTL_PROPERTY_GET(ErrorCode, GetErrorCode) RTL_PROPERTY_PUT(ErrorCode, SetErrorCode, unsigned int)
  __declspec(property(get = __property_get_ErrorCode, put = __property_set_ErrorCode)) unsigned int ErrorCode;

private:
  unsigned int FErrorCode = 0;
};
//---------------------------------------------------------------------------
[[noreturn]] void Abort();
// The error codes are Windows codes (rtl/WinError.h); without an argument the
// thread's last error (GetLastError) is used
[[noreturn]] void RaiseLastOSError();
[[noreturn]] void RaiseLastOSError(int LastError);
[[noreturn]] void RaiseLastOSError(int LastError, const UnicodeString AdditionalInfo);
UnicodeString SysErrorMessage(unsigned int ErrorCode);
//---------------------------------------------------------------------------
// Format settings
//---------------------------------------------------------------------------
// A Windows locale identifier (LCID)
typedef unsigned int TLocaleID;
//---------------------------------------------------------------------------
struct TFormatSettings
{
  Byte CurrencyFormat;
  Byte NegCurrFormat;
  WideChar ThousandSeparator;
  WideChar DecimalSeparator;
  Byte CurrencyDecimals;
  WideChar DateSeparator;
  WideChar TimeSeparator;
  WideChar ListSeparator;
  UnicodeString CurrencyString;
  UnicodeString ShortDateFormat;
  UnicodeString LongDateFormat;
  UnicodeString TimeAMString;
  UnicodeString TimePMString;
  UnicodeString ShortTimeFormat;
  UnicodeString LongTimeFormat;
  StaticArray<UnicodeString, 12> ShortMonthNames;
  StaticArray<UnicodeString, 12> LongMonthNames;
  StaticArray<UnicodeString, 7> ShortDayNames;
  StaticArray<UnicodeString, 7> LongDayNames;
  Word TwoDigitYearCenturyWindow;

  // Settings of the process locale (LC_TIME, LC_NUMERIC, LC_MONETARY)
  static TFormatSettings Create();
  // Settings of a Windows locale. Locales other than 1033 (en-US) and 0
  // (invariant) fall back to the process locale.
  static TFormatSettings Create(LCID Locale);
  // A POSIX locale name, e.g. "de_DE.UTF-8"
  static TFormatSettings Create(const UnicodeString & LocaleName);
  static TFormatSettings Invariant();
};
extern TFormatSettings FormatSettings;
//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------
extern const UnicodeString EmptyStr;
const WideChar PathDelim = L'/';
const WideChar PathSep = L':';

const int HoursPerDay = 24;
const int MinsPerHour = 60;
const int SecsPerMin = 60;
const int MSecsPerSec = 1000;
const int MinsPerDay = HoursPerDay * MinsPerHour;
const int SecsPerHour = SecsPerMin * MinsPerHour;
const int SecsPerDay = MinsPerDay * SecsPerMin;
const int MSecsPerDay = SecsPerDay * MSecsPerSec;
// Days between 1/1/0001 and 12/31/1899
const int DateDelta = 693594;
// Days between TDateTime basis (12/31/1899) and Unix time_t basis (1/1/1970)
const int UnixDateDelta = 25569;
const double MinDateTime = -657434.0;
const double MaxDateTime = 2958465.99999999;

const int faReadOnly = 0x00000001;
const int faHidden = 0x00000002;
const int faSysFile = 0x00000004;
const int faVolumeID = 0x00000008;
const int faDirectory = 0x00000010;
const int faArchive = 0x00000020;
const int faNormal = 0x00000080;
const int faTemporary = 0x00000100;
const int faSymLink = 0x00000400;
const int faCompressed = 0x00000800;
const int faEncrypted = 0x00004000;
const int faVirtual = 0x00010000;
const int faAnyFile = 0x000001FF;

const unsigned int fmOpenRead = 0x0000;
const unsigned int fmOpenWrite = 0x0001;
const unsigned int fmOpenReadWrite = 0x0002;
const unsigned int fmExclusive = 0x0004;
const unsigned int fmShareCompat = 0x0000;
const unsigned int fmShareExclusive = 0x0010;
const unsigned int fmShareDenyWrite = 0x0020;
const unsigned int fmShareDenyRead = 0x0030;
const unsigned int fmShareDenyNone = 0x0040;
//---------------------------------------------------------------------------
// Formatting
//---------------------------------------------------------------------------
UnicodeString Format(const UnicodeString Format, const TVarRec * Args, const int Args_High);
UnicodeString Format(const UnicodeString Format, const TVarRec * Args, const int Args_High,
  const TFormatSettings & AFormatSettings);
void FmtStr(UnicodeString & Result, const UnicodeString Format, const TVarRec * Args, const int Args_High);
UnicodeString LoadStr(int Ident);
UnicodeString FmtLoadStr(int Ident, const TVarRec * Args, const int Args_High);
//---------------------------------------------------------------------------
enum TFloatFormat { ffGeneral, ffExponent, ffFixed, ffNumber, ffCurrency };
enum TFloatValue { fvExtended, fvCurrency };

UnicodeString FloatToStr(Extended Value);
UnicodeString FloatToStr(Extended Value, const TFormatSettings & AFormatSettings);
UnicodeString FloatToStrF(Extended Value, TFloatFormat Format, int Precision, int Digits);
UnicodeString FloatToStrF(Extended Value, TFloatFormat Format, int Precision, int Digits,
  const TFormatSettings & AFormatSettings);
UnicodeString FormatFloat(const UnicodeString Format, Extended Value);
UnicodeString FormatFloat(const UnicodeString Format, Extended Value, const TFormatSettings & AFormatSettings);
Extended StrToFloat(const UnicodeString S);
Extended StrToFloat(const UnicodeString S, const TFormatSettings & AFormatSettings);
Extended StrToFloatDef(const UnicodeString S, const Extended Default);
Extended StrToFloatDef(const UnicodeString S, const Extended Default, const TFormatSettings & AFormatSettings);
bool TryStrToFloat(const UnicodeString S, Extended & Value);
bool TryStrToFloat(const UnicodeString S, double & Value);
bool TryStrToFloat(const UnicodeString S, Extended & Value, const TFormatSettings & AFormatSettings);
bool TryStrToFloat(const UnicodeString S, double & Value, const TFormatSettings & AFormatSettings);
//---------------------------------------------------------------------------
// Integers
//---------------------------------------------------------------------------
UnicodeString IntToStr(int Value);
UnicodeString IntToStr(long Value);
UnicodeString IntToStr(long long Value);
UnicodeString IntToStr(unsigned int Value);
UnicodeString IntToStr(unsigned long Value);
UnicodeString IntToStr(unsigned long long Value);
UnicodeString UIntToStr(unsigned int Value);
UnicodeString UIntToStr(unsigned long Value);
UnicodeString UIntToStr(unsigned long long Value);
// Two's complement of the value's own width: IntToHex(-1, 2) is "FFFFFFFF"
UnicodeString IntToHex(int Value, int Digits);
UnicodeString IntToHex(unsigned int Value, int Digits);
UnicodeString IntToHex(long Value, int Digits);
UnicodeString IntToHex(unsigned long Value, int Digits);
UnicodeString IntToHex(long long Value, int Digits);
UnicodeString IntToHex(unsigned long long Value, int Digits);
// With the digits of the value's width (SizeOf(Value) * 2)
inline UnicodeString IntToHex(int Value) { return IntToHex(Value, 8); }
inline UnicodeString IntToHex(unsigned int Value) { return IntToHex(Value, 8); }
inline UnicodeString IntToHex(long Value) { return IntToHex(Value, static_cast<int>(sizeof(long) * 2)); }
inline UnicodeString IntToHex(unsigned long Value) { return IntToHex(Value, static_cast<int>(sizeof(long) * 2)); }
inline UnicodeString IntToHex(long long Value) { return IntToHex(Value, 16); }
inline UnicodeString IntToHex(unsigned long long Value) { return IntToHex(Value, 16); }
int StrToInt(const UnicodeString S);
int StrToIntDef(const UnicodeString S, int Default);
bool TryStrToInt(const UnicodeString S, int & Value);
__int64 StrToInt64(const UnicodeString S);
__int64 StrToInt64Def(const UnicodeString S, const __int64 Default);
bool TryStrToInt64(const UnicodeString S, __int64 & Value);
UInt64 StrToUInt64(const UnicodeString S);
UInt64 StrToUInt64Def(const UnicodeString S, const UInt64 Default);
bool TryStrToUInt64(const UnicodeString S, UInt64 & Value);
//---------------------------------------------------------------------------
UnicodeString BoolToStr(bool B, bool UseBoolStrs = false);
bool StrToBool(const UnicodeString S);
bool StrToBoolDef(const UnicodeString S, const bool Default);
bool TryStrToBool(const UnicodeString S, bool & Value);
//---------------------------------------------------------------------------
// Strings
//---------------------------------------------------------------------------
UnicodeString Trim(const UnicodeString S);
UnicodeString TrimLeft(const UnicodeString S);
UnicodeString TrimRight(const UnicodeString S);
// ASCII only
UnicodeString UpperCase(const UnicodeString S);
UnicodeString LowerCase(const UnicodeString S);
// Full Unicode
UnicodeString AnsiUpperCase(const UnicodeString S);
UnicodeString AnsiLowerCase(const UnicodeString S);
// Ordinal
int CompareStr(const UnicodeString S1, const UnicodeString S2);
bool SameStr(const UnicodeString S1, const UnicodeString S2);
// Ordinal, ASCII case-insensitive
int CompareText(const UnicodeString S1, const UnicodeString S2);
bool SameText(const UnicodeString S1, const UnicodeString S2);
// Unicode case-insensitive; AnsiCompareStr sorts lowercase before uppercase
// among otherwise equal strings, like Windows' CompareString
int AnsiCompareStr(const UnicodeString S1, const UnicodeString S2);
int AnsiCompareText(const UnicodeString S1, const UnicodeString S2);
bool AnsiSameStr(const UnicodeString S1, const UnicodeString S2);
bool AnsiSameText(const UnicodeString S1, const UnicodeString S2);
// File names are case-sensitive on Linux
int AnsiCompareFileName(const UnicodeString S1, const UnicodeString S2);
bool SameFileName(const UnicodeString S1, const UnicodeString S2);
int AnsiPos(const UnicodeString Substr, const UnicodeString S);
UnicodeString QuotedStr(const UnicodeString S);
UnicodeString AnsiQuotedStr(const UnicodeString S, wchar_t Quote);
UnicodeString AnsiExtractQuotedStr(const wchar_t *& Src, wchar_t Quote);
UnicodeString AnsiExtractQuotedStr(wchar_t *& Src, wchar_t Quote);
UnicodeString AnsiDequotedStr(const UnicodeString S, wchar_t AQuote);
UnicodeString DequotedStr(const UnicodeString S, wchar_t AQuote = L'\'');

enum TReplaceFlag { rfReplaceAll, rfIgnoreCase };
typedef Set<TReplaceFlag, rfReplaceAll, rfIgnoreCase> TReplaceFlags;
UnicodeString StringReplace(const UnicodeString S, const UnicodeString OldPattern,
  const UnicodeString NewPattern, TReplaceFlags Flags);

bool IsDelimiter(const UnicodeString Delimiters, const UnicodeString S, int Index);
int LastDelimiter(const UnicodeString Delimiters, const UnicodeString S);
bool IsPathDelimiter(const UnicodeString S, int Index);

// C-string functions
size_t StrLen(const wchar_t * Str);
wchar_t * StrEnd(const wchar_t * Str);
wchar_t * StrCopy(wchar_t * Dest, const wchar_t * Source);
wchar_t * StrLCopy(wchar_t * Dest, const wchar_t * Source, size_t MaxLen);
wchar_t * StrPCopy(wchar_t * Dest, const UnicodeString Source);
wchar_t * StrPLCopy(wchar_t * Dest, const UnicodeString Source, size_t MaxLen);
int StrComp(const wchar_t * Str1, const wchar_t * Str2);
int StrIComp(const wchar_t * Str1, const wchar_t * Str2);
int StrLComp(const wchar_t * Str1, const wchar_t * Str2, size_t MaxLen);
int StrLIComp(const wchar_t * Str1, const wchar_t * Str2, size_t MaxLen);
wchar_t * StrScan(const wchar_t * Str, wchar_t Chr);
wchar_t * StrRScan(const wchar_t * Str, wchar_t Chr);
wchar_t * StrPos(const wchar_t * Str1, const wchar_t * Str2);
UnicodeString StrPas(const wchar_t * Str);
//---------------------------------------------------------------------------
// File names (POSIX: '/' is the only delimiter, there are no drives)
//---------------------------------------------------------------------------
UnicodeString IncludeTrailingBackslash(const UnicodeString S);
UnicodeString ExcludeTrailingBackslash(const UnicodeString S);
UnicodeString IncludeTrailingPathDelimiter(const UnicodeString S);
UnicodeString ExcludeTrailingPathDelimiter(const UnicodeString S);
UnicodeString ExtractFilePath(const UnicodeString FileName);
UnicodeString ExtractFileDir(const UnicodeString FileName);
UnicodeString ExtractFileName(const UnicodeString FileName);
UnicodeString ExtractFileExt(const UnicodeString FileName);
UnicodeString ExtractFileDrive(const UnicodeString FileName);
UnicodeString ExtractRelativePath(const UnicodeString BaseName, const UnicodeString DestName);
UnicodeString ExtractShortPathName(const UnicodeString FileName);
UnicodeString ExpandFileName(const UnicodeString FileName);
UnicodeString ExpandUNCFileName(const UnicodeString FileName);
UnicodeString ChangeFileExt(const UnicodeString FileName, const UnicodeString Extension);
UnicodeString ChangeFilePath(const UnicodeString FileName, const UnicodeString Path);
bool IsRelativePath(const UnicodeString Path);
UnicodeString GetCurrentDir();
bool SetCurrentDir(const UnicodeString Dir);
UnicodeString GetEnvironmentVariable(const UnicodeString Name);
// The version of the operating system: the Linux kernel release
// (major.minor.patch, the patch as the build number)
int Win32MajorVersion();
int Win32MinorVersion();
int Win32BuildNumber();
UnicodeString Win32CSDVersion();
bool CheckWin32Version(int AMajor, int AMinor = 0);
//---------------------------------------------------------------------------
// Files
//---------------------------------------------------------------------------
bool FileExists(const UnicodeString FileName, bool FollowLink = true);
bool DirectoryExists(const UnicodeString Directory, bool FollowLink = true);
bool ForceDirectories(UnicodeString Dir);
bool CreateDir(const UnicodeString Dir);
bool RemoveDir(const UnicodeString Dir);
bool DeleteFile(const UnicodeString FileName);
bool RenameFile(const UnicodeString OldName, const UnicodeString NewName);
// Attributes as in Delphi on POSIX: faDirectory, faReadOnly (not writable by
// the process), faHidden (name starts with a dot), faSymLink, faSysFile (not a
// regular file or directory). -1 on error.
int FileGetAttr(const UnicodeString FileName, bool FollowLink = true);
// Supports faReadOnly only (clears or sets the write permission bits)
int FileSetAttr(const UnicodeString FileName, int Attr, bool FollowLink = true);
bool FileIsReadOnly(const UnicodeString FileName);
bool FileSetReadOnly(const UnicodeString FileName, bool ReadOnly);
bool FileGetSymLinkTarget(const UnicodeString FileName, UnicodeString & TargetName);
int FileAge(const UnicodeString FileName);
bool FileAge(const UnicodeString FileName, TDateTime & FileDateTime, bool FollowLink = true);
int FileGetDate(THandle Handle);
int FileSetDate(const UnicodeString FileName, int Age);
int FileSetDate(THandle Handle, int Age);
// File handles are file descriptors; INVALID_HANDLE_VALUE on failure
THandle FileOpen(const UnicodeString FileName, unsigned int Mode);
THandle FileCreate(const UnicodeString FileName);
THandle FileCreate(const UnicodeString FileName, int Rights);
THandle FileCreate(const UnicodeString FileName, unsigned int Mode, int Rights);
void FileClose(THandle Handle);
int FileRead(THandle Handle, void * Buffer, unsigned int Count);
int FileWrite(THandle Handle, const void * Buffer, unsigned int Count);
int FileRead(THandle Handle, TBytes & Buffer, unsigned int Offset, unsigned int Count);
int FileWrite(THandle Handle, const TBytes Buffer, unsigned int Offset, unsigned int Count);
int FileSeek(THandle Handle, int Offset, int Origin);
__int64 FileSeek(THandle Handle, const __int64 Offset, int Origin);
//---------------------------------------------------------------------------
struct TSearchRec
{
  int Time;
  __int64 Size;
  int Attr;
  TFileName Name;
  int ExcludeAttr;
  THandle FindHandle;
  // Windows-compatible copy of the attributes, size and times
  TWin32FindData FindData;
  // Private to FindFirst/FindNext
  UnicodeString PathOnly;
  UnicodeString Pattern;

  TDateTime GetTimeStamp() const;
  RTL_PROPERTY_GET(TimeStamp, GetTimeStamp)
  __declspec(property(get = __property_get_TimeStamp)) TDateTime TimeStamp;
};
// Returns 0 or an errno value. The pattern of the last path component may use
// * and ?; "*.*" matches everything, like on Windows. Directories, hidden and
// system entries are only found when included in Attr.
int FindFirst(const UnicodeString Path, int Attr, TSearchRec & F);
int FindNext(TSearchRec & F);
void FindClose(TSearchRec & F);
//---------------------------------------------------------------------------
// Dates and times
//---------------------------------------------------------------------------
struct TTimeStamp
{
  int Time; // milliseconds since midnight
  int Date; // days since 1/1/0001, plus one
};

TDateTime Now();
TDateTime Date();
TDateTime Time();
TDateTime GetTime();
TDateTime EncodeDate(Word Year, Word Month, Word Day);
TDateTime EncodeTime(Word Hour, Word Min, Word Sec, Word MSec);
bool TryEncodeDate(Word Year, Word Month, Word Day, TDateTime & Date);
bool TryEncodeTime(Word Hour, Word Min, Word Sec, Word MSec, TDateTime & Time);
void DecodeDate(const TDateTime DateTime, Word & Year, Word & Month, Word & Day);
bool DecodeDateFully(const TDateTime DateTime, Word & Year, Word & Month, Word & Day, Word & DOW);
void DecodeTime(const TDateTime DateTime, Word & Hour, Word & Min, Word & Sec, Word & MSec);
// 1 = Sunday
int DayOfWeek(const TDateTime DateTime);
bool IsLeapYear(Word Year);
TDateTime IncMonth(const TDateTime DateTime, int NumberOfMonths = 1);
void ReplaceTime(TDateTime & DateTime, const TDateTime NewTime);
void ReplaceDate(TDateTime & DateTime, const TDateTime NewDate);
TTimeStamp DateTimeToTimeStamp(TDateTime DateTime);
TDateTime TimeStampToDateTime(const TTimeStamp & TimeStamp);
Comp TimeStampToMSecs(const TTimeStamp & TimeStamp);
TTimeStamp MSecsToTimeStamp(Comp MSecs);
void DateTimeToSystemTime(const TDateTime DateTime, SYSTEMTIME & SystemTime);
TDateTime SystemTimeToDateTime(const SYSTEMTIME & SystemTime);
// DOS date and time, as in TSearchRec::Time
TDateTime FileDateToDateTime(int FileDate);
int DateTimeToFileDate(TDateTime DateTime);

UnicodeString FormatDateTime(const UnicodeString Format, TDateTime DateTime);
UnicodeString FormatDateTime(const UnicodeString Format, TDateTime DateTime, const TFormatSettings & AFormatSettings);
void DateTimeToString(UnicodeString & Result, const UnicodeString Format, TDateTime DateTime);
void DateTimeToString(UnicodeString & Result, const UnicodeString Format, TDateTime DateTime,
  const TFormatSettings & AFormatSettings);
UnicodeString DateToStr(const TDateTime DateTime);
UnicodeString DateToStr(const TDateTime DateTime, const TFormatSettings & AFormatSettings);
UnicodeString TimeToStr(const TDateTime DateTime);
UnicodeString TimeToStr(const TDateTime DateTime, const TFormatSettings & AFormatSettings);
UnicodeString DateTimeToStr(const TDateTime DateTime);
UnicodeString DateTimeToStr(const TDateTime DateTime, const TFormatSettings & AFormatSettings);
TDateTime StrToDate(const UnicodeString S);
TDateTime StrToDate(const UnicodeString S, const TFormatSettings & AFormatSettings);
TDateTime StrToTime(const UnicodeString S);
TDateTime StrToTime(const UnicodeString S, const TFormatSettings & AFormatSettings);
TDateTime StrToDateTime(const UnicodeString S);
TDateTime StrToDateTime(const UnicodeString S, const TFormatSettings & AFormatSettings);
bool TryStrToDate(const UnicodeString S, TDateTime & Value);
bool TryStrToDate(const UnicodeString S, TDateTime & Value, const TFormatSettings & AFormatSettings);
bool TryStrToTime(const UnicodeString S, TDateTime & Value);
bool TryStrToTime(const UnicodeString S, TDateTime & Value, const TFormatSettings & AFormatSettings);
bool TryStrToDateTime(const UnicodeString S, TDateTime & Value);
bool TryStrToDateTime(const UnicodeString S, TDateTime & Value, const TFormatSettings & AFormatSettings);
//---------------------------------------------------------------------------
// Miscellaneous
//---------------------------------------------------------------------------
template <class T>
void FreeAndNil(T *& Obj)
{
  T * Temp = Obj;
  Obj = nullptr;
  delete Temp;
}
//---------------------------------------------------------------------------
} // namespace Sysutils
} // namespace System
//---------------------------------------------------------------------------
// Resource strings of System.SysConst (which SysUtils includes) used by the core
#define System_Sysconst_SOSError (::System::UnicodeString(L"System Error.  Code: %d.\n%s%s"))
//---------------------------------------------------------------------------
namespace rtl
{
// Unicode string of a file name passed in from the file system and back.
// Invalid UTF-8 is preserved (see rtl/Conversion.h).
std::string FileNameToBytes(const System::UnicodeString & FileName);
System::UnicodeString BytesToFileName(const char * Bytes);
// The system time zone, see DateUtils for conversions
System::TDateTime DateTimeFromUnixTime(long long Seconds, long Nanoseconds, bool Local);
} // namespace rtl
//---------------------------------------------------------------------------
#include <rtl/Encoding.h>
//---------------------------------------------------------------------------
using namespace System::Sysutils;
//---------------------------------------------------------------------------
#endif
