// Helpers of the application layer, from upstream's UserInterface.cpp,
// WinMain.cpp, VCLCommon.cpp and GUITools.cpp (which are not ported as a
// whole).
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include <HierarchicalStorage.h>

#include "GUIConfiguration.h"
#include "ProgParams.h"
//---------------------------------------------------------------------------
bool __fastcall CheckSafe(TProgramParams * Params)
{
  // Originally we warned when the test didn't pass,
  // but it would actually be helping hackers, so let's be silent.
  return !Params->FindSwitch(UNSAFE_SWITCH);
}
//---------------------------------------------------------------------------
void __fastcall CheckLogParam(TProgramParams * Params)
{
  UnicodeString LogFile;
  if (Params->FindSwitch(LOG_SWITCH, LogFile) && CheckSafe(Params))
  {
    Configuration->Usage->Inc(L"ScriptLog");
    Configuration->TemporaryLogging(LogFile);
  }
}
//---------------------------------------------------------------------------
bool __fastcall CheckXmlLogParam(TProgramParams * Params)
{
  UnicodeString LogFile;
  bool Result =
    Params->FindSwitch(L"XmlLog", LogFile) &&
    CheckSafe(Params);
  if (Result)
  {
    Configuration->Usage->Inc(L"ScriptXmlLog");
    Configuration->TemporaryActionsLogging(LogFile);

    if (Params->FindSwitch(L"XmlLogRequired"))
    {
      Configuration->LogActionsRequired = true;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall ApplyCommandLineConfiguration(TProgramParams * Params)
{
  UnicodeString SwitchValue;
  if (Params->FindSwitch(L"loglevel", SwitchValue))
  {
    int StarPos = SwitchValue.Pos(L"*");
    if (StarPos > 0)
    {
      bool LogSensitive = true;
      SwitchValue.Delete(StarPos, 1);

      if ((StarPos <= SwitchValue.Length()) &&
          (SwitchValue[StarPos] == L'-'))
      {
        LogSensitive = false;
        SwitchValue.Delete(StarPos, 1);
      }

      SwitchValue = SwitchValue.Trim();

      Configuration->TemporaryLogSensitive(LogSensitive);
    }
    int LogProtocol = 0;
    if (!SwitchValue.IsEmpty() && TryStrToInt(SwitchValue, LogProtocol) && (LogProtocol >= -1))
    {
      Configuration->TemporaryLogProtocol(LogProtocol);
    }
  }

  if (Params->FindSwitch(LOGSIZE_SWITCH, SwitchValue))
  {
    int StarPos = SwitchValue.Pos(LOGSIZE_SEPARATOR);
    int LogMaxCount = 0;
    if (StarPos > 1)
    {
      if (!TryStrToInt(SwitchValue.SubString(1, StarPos - 1), LogMaxCount))
      {
        LogMaxCount = -1;
      }
      SwitchValue.Delete(1, StarPos);
      SwitchValue = SwitchValue.Trim();
    }

    __int64 LogMaxSize = 0;
    if ((LogMaxCount >= 0) &&
        !SwitchValue.IsEmpty() &&
        TryStrToSize(SwitchValue, LogMaxSize))
    {
      Configuration->TemporaryLogMaxCount(LogMaxCount);
      Configuration->TemporaryLogMaxSize(LogMaxSize);
    }
  }

  std::unique_ptr<TStrings> RawSettings(new TStringList());
  if (Params->FindSwitch(RAWTRANSFERSETTINGS_SWITCH, RawSettings.get()))
  {
    std::unique_ptr<TOptionsStorage> OptionsStorage(new TOptionsStorage(RawSettings.get(), false));
    GUIConfiguration->LoadDefaultCopyParam(OptionsStorage.get());
  }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FormatMainFormCaption(const UnicodeString & Caption, const UnicodeString & SessionName)
{
  UnicodeString Suffix = AppName;
  if (!SessionName.IsEmpty())
  {
    Suffix = SessionName + TitleSeparator + Suffix;
  }
  UnicodeString Result = Caption;
  if (Result.IsEmpty())
  {
    Result = Suffix;
  }
  else
  {
    Suffix = TitleSeparator + Suffix;
    if (!EndsStr(Suffix, Result))
    {
      Result += Suffix;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UniqTempDir(const UnicodeString BaseDir, const UnicodeString Identity,
  bool Mask)
{
  DebugAssert(!BaseDir.IsEmpty());
  UnicodeString TempDir;
  do
  {
    TempDir = IncludeTrailingBackslash(BaseDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
      TempDir += IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
    }
  }
  while (!Mask && DirectoryExists(ApiPath(TempDir)));

  return TempDir;
}
//---------------------------------------------------------------------------
bool IsKdeDesktop()
{
  // A colon-separated list, e.g. "KDE" or "ubuntu:GNOME"
  const char * Desktops = getenv("XDG_CURRENT_DESKTOP");
  std::unique_ptr<TStringList> List(new TStringList());
  List->Delimiter = L':';
  List->StrictDelimiter = true;
  List->DelimitedText = UnicodeString((Desktops != NULL) ? Desktops : "");
  return (List->IndexOf(L"KDE") >= 0);
}
//---------------------------------------------------------------------------
static bool __fastcall IsEligibleForApplyingTabs(
  UnicodeString Line, int & TabPos, UnicodeString & Start, UnicodeString & Remaining)
{
  bool Result = false;
  TabPos = Line.Pos(L"\t");
  if (TabPos > 0)
  {
    Remaining = Line.SubString(TabPos + 1, Line.Length() - TabPos);
    // Some translations still use obsolete hack of consecutive tabs to align the contents.
    // Delete these, so that the following check does not fail on this
    while (Remaining.SubString(1, 1) == L"\t")
    {
      Remaining.Delete(1, 1);
    }

    // We do not have, not support, multiple tabs on a single line
    if (DebugAlwaysTrue(Remaining.Pos(L"\t") == 0))
    {
      Start = Line.SubString(1, TabPos - 1);
      // Previously we padded the string before tab with spaces,
      // to align the contents across multiple lines
      Start = Start.TrimRight();
      // at least two normal spaces for separation
      Start += L"  ";
      Result = true;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
static int __fastcall CalculateWidthByLength(UnicodeString Text, void * /*Arg*/)
{
  return Text.Length();
}
//---------------------------------------------------------------------------
void __fastcall ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg)
{
  if (CalculateWidth == NULL)
  {
    DebugAssert(CalculateWidthArg == NULL);
    CalculateWidth = CalculateWidthByLength;
  }

  std::unique_ptr<TStringList> Lines(TextToStringList(Text));

  int MaxWidth = -1;
  for (int Index = 0; Index < Lines->Count; Index++)
  {
    UnicodeString Line = Lines->Strings[Index];
    int TabPos;
    UnicodeString Start;
    UnicodeString Remaining;
    if (IsEligibleForApplyingTabs(Line, TabPos, Start, Remaining))
    {
      int Width = CalculateWidth(Start, CalculateWidthArg);
      MaxWidth = Max(MaxWidth, Width);
    }
  }

  // Optimization and also to prevent potential regression for texts without tabs
  if (MaxWidth >= 0)
  {
    for (int Index = 0; Index < Lines->Count; Index++)
    {
      UnicodeString Line = Lines->Strings[Index];
      int TabPos;
      UnicodeString Start;
      UnicodeString Remaining;
      if (IsEligibleForApplyingTabs(Line, TabPos, Start, Remaining))
      {
        int Width;
        int Iterations = 0;
        while ((Width = CalculateWidth(Start, CalculateWidthArg)) < MaxWidth)
        {
          int Wider = CalculateWidth(Start + Padding, CalculateWidthArg);
          // If padded string is wider than max width by more pixels
          // than non-padded string is shorter than max width
          if ((Wider > MaxWidth) && ((Wider - MaxWidth) > (MaxWidth - Width)))
          {
            break;
          }
          Start += Padding;
          Iterations++;
          if (Iterations > 100)
          {
            break;
          }
        }
        Lines->Strings[Index] = Start + Remaining;
      }
    }

    Text = Lines->Text;
    // remove trailing newline
    Text = Text.TrimRight();
  }
}
//---------------------------------------------------------------------------
