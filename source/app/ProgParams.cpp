//---------------------------------------------------------------------------
#include <AppPCH.h>
#pragma hdrstop

#include "ProgParams.h"
//---------------------------------------------------------------------------
static std::unique_ptr<TProgramParams> ProgramParamsOwner;
//---------------------------------------------------------------------------
TProgramParams * __fastcall TProgramParams::Instance()
{
  if (ProgramParamsOwner.get() == NULL)
  {
    ProgramParamsOwner.reset(new TProgramParams());
  }
  return ProgramParamsOwner.get();
}
//---------------------------------------------------------------------------
__fastcall TProgramParams::TProgramParams()
{
  // The process's arguments as they are; the command line string (CmdLine)
  // quotes them as on Windows, which CutToken does not undo
  for (int Index = 1; Index <= System::ParamCount(); Index++)
  {
    Add(System::ParamStr(Index));
  }
}
//---------------------------------------------------------------------------
__fastcall TProgramParams::TProgramParams(const UnicodeString & CmdLine)
{
  Init(CmdLine);
}
//---------------------------------------------------------------------------
void __fastcall TProgramParams::Init(const UnicodeString & CmdLine)
{
  UnicodeString CommandLine = CmdLine;

  UnicodeString Param;
  CutToken(CommandLine, Param);
  Parse(CommandLine);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TProgramParams::FormatSwitch(const UnicodeString & Switch)
{
  return FORMAT(L"/%s", (Switch));
}
