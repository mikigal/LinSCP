// The parts of the core's front end interface (core/Interface.h) that are
// common to the front ends; the rest (user interaction, GUI processing,
// clipboard) is implemented by each front end.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <HierarchicalStorage.h>

#include "AppConfiguration.h"
#include "ProgParams.h"
//---------------------------------------------------------------------------
const UnicodeString AppName = L"OpenSCP";
//---------------------------------------------------------------------------
TConfiguration * __fastcall CreateConfiguration()
{
  AppConfiguration = new TAppConfiguration();
  GUIConfiguration = AppConfiguration;

  TProgramParams * Params = TProgramParams::Instance();
  UnicodeString IniFileName = Params->SwitchValue(INI_SWITCH);
  if (!IniFileName.IsEmpty() && SameText(IniFileName, INI_NUL))
  {
    AppConfiguration->SetNulStorage();
  }
  else if (!IniFileName.IsEmpty() && CheckSafe(Params))
  {
    IniFileName = ExpandFileName(ExpandEnvironmentVariables(IniFileName));
    AppConfiguration->SetExplicitIniFileStorageName(IniFileName);
  }
  else
  {
    // Unlike on Windows, there is no registry storage to detect
    AppConfiguration->SetExplicitIniFileStorageName(TAppConfiguration::DefaultIniFileStorageName());
  }

  if (CheckSafe(Params))
  {
    std::unique_ptr<TStrings> RawConfig(new TStringList());
    if (Params->FindSwitch(RAW_CONFIG_SWITCH, RawConfig.get()))
    {
      AppConfiguration->OptionsStorage = RawConfig.get();
    }
  }

  return AppConfiguration;
}
//---------------------------------------------------------------------------
TOptions * __fastcall GetGlobalOptions()
{
  return TProgramParams::Instance();
}
//---------------------------------------------------------------------------
UnicodeString __fastcall AppNameString()
{
  return AppName;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall SshVersionString()
{
  return FORMAT(L"OpenSCP-release-%s", (Configuration->Version));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall GetCompanyRegistryKey()
{
  return L"Software\\OpenSCP";
}
//---------------------------------------------------------------------------
UnicodeString __fastcall GetRegistryKey()
{
  return GetCompanyRegistryKey() + L"\\OpenSCP";
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
  // The captions of the answers that upstream takes from the VCL
  // (Vcl.Consts) are the VCL's English ones
  switch (Answer)
  {
    case qaYes:
      Caption = L"&Yes";
      Name = L"Yes";
      break;

    case qaNo:
      Caption = L"&No";
      Name = L"No";
      break;

    case qaOK:
      Caption = L"OK";
      Name = L"OK";
      break;

    case qaCancel:
      Caption = L"Cancel";
      Name = L"Cancel";
      break;

    case qaAbort:
      Caption = L"&Abort";
      Name = L"Abort";
      break;

    case qaRetry:
      Caption = L"&Retry";
      Name = L"Retry";
      break;

    case qaIgnore:
      Caption = L"&Ignore";
      Name = L"Ignore";
      break;

    // Own variant to avoid accelerator conflict with "Abort" button.
    case qaAll:
      Caption = LoadStr(ALL_BUTTON);
      Name = L"All";
      break;

    case qaNoToAll:
      Caption = L"N&o to All";
      Name = L"NoToAll";
      break;

    // Own variant to avoid accelerator conflict with "Abort" button.
    case qaYesToAll:
      Caption = LoadStr(YES_TO_ALL_BUTTON);
      Name = L"YesToAll";
      break;

    case qaHelp:
      Caption = L"&Help";
      Name = L"Help";
      break;

    case qaSkip:
      Caption = LoadStr(SKIP_BUTTON);
      Name = L"Skip";
      break;

    case qaReport:
      Caption = LoadStr(REPORT_BUTTON);
      Name = L"Report";
      break;

    default:
      DebugFail();
      throw Exception(L"Undefined answer");
  }
}
//---------------------------------------------------------------------------
