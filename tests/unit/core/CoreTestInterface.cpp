// The core's front end interface (core/Interface.h) for the core's L1 tests.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <Option.h>

#include "CoreTestInterface.h"

#include <cstdio>
//---------------------------------------------------------------------------
TTestConfiguration::TTestConfiguration()
{
  SetNulStorage();
}
//---------------------------------------------------------------------------
UnicodeString TTestConfiguration::TemporaryDir(bool Mask)
{
  UnicodeString Result = IncludeTrailingBackslash(SystemTemporaryDirectory()) + L"openscp-test";
  if (Mask)
  {
    Result += L"*";
  }
  return Result;
}
//---------------------------------------------------------------------------
TStrings * ShownExceptions()
{
  static TStringList Result;
  return &Result;
}
//---------------------------------------------------------------------------
TConfiguration * __fastcall CreateConfiguration()
{
  return new TTestConfiguration();
}
//---------------------------------------------------------------------------
TOptions * __fastcall GetGlobalOptions()
{
  static TOptions Options;
  return &Options;
}
//---------------------------------------------------------------------------
void __fastcall ShowExtendedException(Exception * E)
{
  UnicodeString Message = (E != NULL) ? E->Message : UnicodeString(L"(no exception)");
  ShownExceptions()->Add(Message);
  fprintf(stderr, "ShowExtendedException: %s\n", UTF8String(Message).c_str());
}
//---------------------------------------------------------------------------
bool AppendExceptionStackTrace(Exception *, TStrings *&)
{
  return false;
}
//---------------------------------------------------------------------------
UnicodeString GetExceptionDebugInfo(Exception *)
{
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall GetCompanyRegistryKey()
{
  return L"Software\\OpenSCP";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall GetRegistryKey()
{
  return GetCompanyRegistryKey() + L"\\OpenSCP Tests";
}
//---------------------------------------------------------------------------
void * __fastcall BusyStart()
{
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall BusyEnd(void *)
{
}
//---------------------------------------------------------------------------
void __fastcall SetNoGUI()
{
}
//---------------------------------------------------------------------------
bool __fastcall ProcessGUI(bool)
{
  return false;
}
//---------------------------------------------------------------------------
void SystemRequired()
{
}
//---------------------------------------------------------------------------
UnicodeString __fastcall AppNameString()
{
  return L"OpenSCP";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall SshVersionString()
{
  return FORMAT(L"OpenSCP-test-%s", (Configuration->Version));
}
//---------------------------------------------------------------------------
void __fastcall CopyToClipboard(UnicodeString)
{
}
//---------------------------------------------------------------------------
bool __fastcall TextFromClipboard(UnicodeString &, bool)
{
  return false;
}
//---------------------------------------------------------------------------
struct TThreadParam
{
  TThreadFunc ThreadFunc;
  void * Parameter;
};
//---------------------------------------------------------------------------
static DWORD WINAPI ThreadProc(void * AParam)
{
  TThreadParam * Param = static_cast<TThreadParam *>(AParam);
  TThreadFunc ThreadFunc = Param->ThreadFunc;
  void * Parameter = Param->Parameter;
  delete Param;
  return static_cast<DWORD>(ThreadFunc(Parameter));
}
//---------------------------------------------------------------------------
THandle StartThread(
  void * SecurityAttributes, unsigned StackSize, TThreadFunc ThreadFunc, void * Parameter,
  unsigned CreationFlags, TThreadID & ThreadId)
{
  TThreadParam * Param = new TThreadParam;
  Param->ThreadFunc = ThreadFunc;
  Param->Parameter = Parameter;
  DWORD Id = 0;
  HANDLE Result =
    CreateThread(static_cast<LPSECURITY_ATTRIBUTES>(SecurityAttributes), StackSize, ThreadProc, Param, CreationFlags, &Id);
  if (Result == NULL)
  {
    delete Param;
  }
  ThreadId = Id;
  return reinterpret_cast<THandle>(Result);
}
//---------------------------------------------------------------------------
void __fastcall AnswerNameAndCaption(unsigned int Answer, UnicodeString & Name, UnicodeString & Caption)
{
  Name = IntToStr(static_cast<int>(Answer));
  Caption = Name;
}
//---------------------------------------------------------------------------
