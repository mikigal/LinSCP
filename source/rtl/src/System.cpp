// The System unit: TObject, resource strings, command line, random numbers
//---------------------------------------------------------------------------
#include <rtl/SysUtils.h>

#include <cxxabi.h>
#include <fstream>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <vector>
//---------------------------------------------------------------------------
namespace System
{
//---------------------------------------------------------------------------
namespace rtl_detail
{
//---------------------------------------------------------------------------
UnicodeString ClassNameOf(const std::type_info & TypeInfo, bool Qualified)
{
  int Status = 0;
  char * Demangled = abi::__cxa_demangle(TypeInfo.name(), nullptr, nullptr, &Status);
  std::string Name = ((Status == 0) && (Demangled != nullptr)) ? Demangled : TypeInfo.name();
  free(Demangled);
  // Template arguments are not part of Delphi class names
  size_t Template = Name.find('<');
  std::string Base = (Template == std::string::npos) ? Name : Name.substr(0, Template);
  size_t Scope = Base.rfind("::");
  if (!Qualified)
  {
    if (Scope != std::string::npos)
    {
      Name = Name.substr(Scope + 2);
    }
  }
  else
  {
    // Delphi style: System.Sysutils.Exception
    std::string Dotted;
    for (size_t Index = 0; Index < Name.size(); Index++)
    {
      if ((Index < Base.size()) && (Name.compare(Index, 2, "::") == 0))
      {
        Dotted.push_back('.');
        Index++;
      }
      else
      {
        Dotted.push_back(Name[Index]);
      }
    }
    Name = Dotted;
  }
  return UnicodeString(Name.c_str());
}
//---------------------------------------------------------------------------
TClass ClassOfTypeInfo(const std::type_info & TypeInfo)
{
  // Class references of dynamic types, created on demand and kept forever
  static std::mutex Mutex;
  static std::map<const std::type_info *, std::unique_ptr<TMetaClass>> Classes;
  std::lock_guard<std::mutex> Lock(Mutex);
  std::unique_ptr<TMetaClass> & Result = Classes[&TypeInfo];
  if (!Result)
  {
    Result.reset(new TMetaClass(TypeInfo, nullptr));
  }
  return Result.get();
}
//---------------------------------------------------------------------------
} // namespace rtl_detail
//---------------------------------------------------------------------------
UnicodeString TMetaClass::ClassName() const
{
  return rtl_detail::ClassNameOf(FTypeInfo, false);
}
//---------------------------------------------------------------------------
UnicodeString TMetaClass::QualifiedClassName() const
{
  return rtl_detail::ClassNameOf(FTypeInfo, true);
}
//---------------------------------------------------------------------------
bool TMetaClass::IsInstance(const TObject * Object) const
{
  if (Object == nullptr)
  {
    return false;
  }
  if (FIsInstanceFunc != nullptr)
  {
    return FIsInstanceFunc(Object);
  }
  return (typeid(*Object) == FTypeInfo);
}
//---------------------------------------------------------------------------
UnicodeString TObject::ClassName() const
{
  return rtl_detail::ClassNameOf(typeid(*this), false);
}
//---------------------------------------------------------------------------
UnicodeString TObject::QualifiedClassName() const
{
  return rtl_detail::ClassNameOf(typeid(*this), true);
}
//---------------------------------------------------------------------------
TClass TObject::ClassType() const
{
  return rtl_detail::ClassOfTypeInfo(typeid(*this));
}
//---------------------------------------------------------------------------
bool TObject::InheritsFrom(TClass AClass) const
{
  return (AClass != nullptr) && AClass->IsInstance(this);
}
//---------------------------------------------------------------------------
UnicodeString TObject::ToString()
{
  return ClassName();
}
//---------------------------------------------------------------------------
// Resource strings
//---------------------------------------------------------------------------
namespace
{
std::shared_mutex & ResourceMutex()
{
  static std::shared_mutex Mutex;
  return Mutex;
}

std::map<int, const wchar_t *> & ResourceStrings()
{
  static std::map<int, const wchar_t *> Strings;
  return Strings;
}
} // namespace
//---------------------------------------------------------------------------
UnicodeString LoadResString(PResStringRec ResStringRec)
{
  return (ResStringRec != nullptr) ? rtl::FindResourceString(static_cast<int>(ResStringRec->Identifier)) : UnicodeString();
}
//---------------------------------------------------------------------------
// Command line
//---------------------------------------------------------------------------
namespace
{
struct TCommandLine
{
  std::vector<UnicodeString> Params;
  std::wstring Line;
};

UnicodeString QuoteParam(const UnicodeString & Param)
{
  bool NeedsQuotes = Param.IsEmpty();
  for (wchar_t Ch : Param)
  {
    if ((Ch == L' ') || (Ch == L'\t') || (Ch == L'"'))
    {
      NeedsQuotes = true;
    }
  }
  if (!NeedsQuotes)
  {
    return Param;
  }
  // Windows command line conventions, which the core's parser expects
  UnicodeString Result = L"\"";
  int Backslashes = 0;
  for (wchar_t Ch : Param)
  {
    if (Ch == L'\\')
    {
      Backslashes++;
      continue;
    }
    if (Ch == L'"')
    {
      Result += UnicodeString::StringOfChar(L'\\', Backslashes * 2 + 1);
    }
    else
    {
      Result += UnicodeString::StringOfChar(L'\\', Backslashes);
    }
    Backslashes = 0;
    Result += Ch;
  }
  Result += UnicodeString::StringOfChar(L'\\', Backslashes * 2);
  Result += L"\"";
  return Result;
}

TCommandLine BuildCommandLine(const std::vector<std::string> & Args)
{
  TCommandLine Result;
  for (const std::string & Arg : Args)
  {
    UnicodeString Param(rtl::DecodeUtf8(Arg.data(), Arg.size()));
    if (!Result.Line.empty())
    {
      Result.Line += L' ';
    }
    Result.Line += QuoteParam(Param).Std();
    Result.Params.push_back(Param);
  }
  return Result;
}

TCommandLine & CommandLine()
{
  static TCommandLine Line = []()
    {
      std::vector<std::string> Args;
      std::ifstream Stream("/proc/self/cmdline", std::ios::binary);
      std::string Arg;
      while (std::getline(Stream, Arg, '\0'))
      {
        Args.push_back(Arg);
      }
      return BuildCommandLine(Args);
    }();
  return Line;
}
} // namespace
//---------------------------------------------------------------------------
// Constant-initialized, then set before ordinary static objects are constructed
wchar_t * CmdLine = const_cast<wchar_t *>(L"");
int ExitCode = 0;
namespace
{
struct TCmdLineInitializer
{
  TCmdLineInitializer() { CmdLine = CommandLine().Line.data(); }
} CmdLineInitializer __attribute__((init_priority(200)));
} // namespace
//---------------------------------------------------------------------------
int ParamCount()
{
  return static_cast<int>(CommandLine().Params.size()) - 1;
}
//---------------------------------------------------------------------------
UnicodeString ParamStr(int Index)
{
  if (Index == 0)
  {
    // The full path of the executable, as GetModuleFileName returns it
    char Buffer[4096];
    ssize_t Len = readlink("/proc/self/exe", Buffer, sizeof(Buffer) - 1);
    if (Len > 0)
    {
      return rtl::BytesToFileName(std::string(Buffer, Len).c_str());
    }
  }
  TCommandLine & Line = CommandLine();
  return ((Index >= 0) && (Index < static_cast<int>(Line.Params.size()))) ? Line.Params[Index] : UnicodeString();
}
//---------------------------------------------------------------------------
// Modules
//---------------------------------------------------------------------------
namespace
{
TLibModule MainModule = { nullptr, 0, 0, 0, 0, 0, 0 };
} // namespace
TLibModule * LibModuleList = &MainModule;
HINSTANCE HInstance = nullptr;
HINSTANCE MainInstance = nullptr;
//---------------------------------------------------------------------------
// Random numbers
//---------------------------------------------------------------------------
int RandSeed = 0;
//---------------------------------------------------------------------------
void Randomize()
{
  timespec Now;
  clock_gettime(CLOCK_REALTIME, &Now);
  RandSeed = static_cast<int>(static_cast<unsigned int>(Now.tv_sec) * 1000u + static_cast<unsigned int>(Now.tv_nsec / 1000000));
}
//---------------------------------------------------------------------------
namespace
{
unsigned int NextRandom()
{
  RandSeed = static_cast<int>(static_cast<unsigned int>(RandSeed) * 0x08088405u + 1u);
  return static_cast<unsigned int>(RandSeed);
}
} // namespace
//---------------------------------------------------------------------------
int Random(int Range)
{
  unsigned long long Value = NextRandom();
  return static_cast<int>((Value * static_cast<unsigned int>(Range)) >> 32);
}
//---------------------------------------------------------------------------
double Random()
{
  return NextRandom() / 4294967296.0;
}
//---------------------------------------------------------------------------
// Strings
//---------------------------------------------------------------------------
UnicodeString UTF8ToString(const RawByteString & Str)
{
  return UnicodeString(rtl::DecodeUtf8(Str.c_str(), Str.Length()));
}
//---------------------------------------------------------------------------
UnicodeString UTF8ToString(const char * Str)
{
  return (Str != nullptr) ? UnicodeString(rtl::DecodeUtf8(Str, strlen(Str))) : UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString UTF8ArrayToString(const char * A, int A_High)
{
  size_t Length = (A_High >= 0) ? strnlen(A, static_cast<size_t>(A_High) + 1) : 0;
  return UnicodeString(rtl::DecodeUtf8(A, Length));
}
//---------------------------------------------------------------------------
UnicodeString UTF8ToUnicodeString(const RawByteString & Str)
{
  return UTF8ToString(Str);
}
//---------------------------------------------------------------------------
UTF8String UTF8Encode(const UnicodeString & Str)
{
  return UTF8String(Str);
}
//---------------------------------------------------------------------------
UnicodeString UTF8Decode(const RawByteString & Str)
{
  return UTF8ToString(Str);
}
//---------------------------------------------------------------------------
RawByteString AnsiToUtf8(const UnicodeString & Str)
{
  return RawByteString(UTF8String(Str));
}
//---------------------------------------------------------------------------
UnicodeString Utf8ToAnsi(const RawByteString & Str)
{
  return UTF8ToString(Str);
}
//---------------------------------------------------------------------------
UnicodeString StringOfChar(wchar_t Ch, int Count)
{
  return UnicodeString::StringOfChar(Ch, Count);
}
//---------------------------------------------------------------------------
void Move(const void * Source, void * Dest, NativeInt Count)
{
  if (Count > 0)
  {
    memmove(Dest, Source, Count);
  }
}
//---------------------------------------------------------------------------
void FillChar(void * Dest, NativeInt Count, Byte Value)
{
  if (Count > 0)
  {
    memset(Dest, Value, Count);
  }
}
//---------------------------------------------------------------------------
} // namespace System
//---------------------------------------------------------------------------
namespace rtl
{
//---------------------------------------------------------------------------
void RegisterResourceStrings(const TResourceString * Strings, size_t Count)
{
  std::unique_lock<std::shared_mutex> Lock(System::ResourceMutex());
  for (size_t Index = 0; Index < Count; Index++)
  {
    System::ResourceStrings()[Strings[Index].Ident] = Strings[Index].Text;
  }
}
//---------------------------------------------------------------------------
System::UnicodeString FindResourceString(int Ident)
{
  std::shared_lock<std::shared_mutex> Lock(System::ResourceMutex());
  auto I = System::ResourceStrings().find(Ident);
  return (I != System::ResourceStrings().end()) ? System::UnicodeString(I->second) : System::UnicodeString();
}
//---------------------------------------------------------------------------
void SetCommandLine(int ArgC, const char * const * ArgV)
{
  std::vector<std::string> Args(ArgV, ArgV + ArgC);
  System::CommandLine() = System::BuildCommandLine(Args);
  System::CmdLine = System::CommandLine().Line.data();
}
//---------------------------------------------------------------------------
} // namespace rtl
