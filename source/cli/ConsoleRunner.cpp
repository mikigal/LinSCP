//---------------------------------------------------------------------------
#include <AppPCH.h>
#pragma hdrstop

#include <Script.h>
#include <Terminal.h>
#include <PuttyTools.h>
#include <Queue.h>
#include <HierarchicalStorage.h>
#include "CliInterface.h"
#include "AppConfiguration.h"
#include "ProgParams.h"
#include "SynchronizeController.h"
#include "FtpFileSystem.h"
#include "SessionInfo.h"
//---------------------------------------------------------------------------
#define BATCH_INPUT_TIMEOUT 10000
//---------------------------------------------------------------------------
void TrimNewLine(UnicodeString & Str)
{
  while (!Str.IsEmpty() &&
    ((Str[Str.Length()] == L'\n') || (Str[Str.Length()] == L'\r')))
  {
    Str.SetLength(Str.Length() - 1);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsole::PrintLine(const UnicodeString & Str, bool Error)
{
  Print(Str + L"\n", false, Error);
};
//---------------------------------------------------------------------------
// OpenSCP: the consoles of Windows (TOwnConsole, the console window of
// WinSCP.exe, and TExternalConsole, which talks to winscp.com) are replaced
// by the console of a Unix terminal (UnixConsole.cpp)
//---------------------------------------------------------------------------
class TNullConsole : public TConsole
{
public:
  __fastcall TNullConsole();

  virtual void __fastcall Print(UnicodeString Str, bool FromBeginning = false, bool Error = false);
  virtual bool __fastcall Input(UnicodeString & Str, bool Echo, unsigned int Timer);
  virtual int __fastcall Choice(
    UnicodeString Options, int Cancel, int Break, int Continue, int Timeouted, bool Timeouting, unsigned int Timer,
    UnicodeString Message);
  virtual bool __fastcall HasFlag(TConsoleFlag Flag) const;
  virtual bool __fastcall PendingAbort();
  virtual void __fastcall SetTitle(UnicodeString Title);
  virtual void __fastcall WaitBeforeExit();

  virtual void __fastcall Progress(TScriptProgress & Progress);
  virtual void __fastcall TransferOut(const unsigned char * Data, size_t Len);
  virtual size_t __fastcall TransferIn(unsigned char * Data, size_t Len);
  virtual UnicodeString __fastcall FinalLogMessage();
};
//---------------------------------------------------------------------------
__fastcall TNullConsole::TNullConsole()
{
}
//---------------------------------------------------------------------------
void __fastcall TNullConsole::Print(UnicodeString /*Str*/, bool /*FromBeginning*/, bool /*Error*/)
{
  // noop
}
//---------------------------------------------------------------------------
bool __fastcall TNullConsole::Input(UnicodeString & /*Str*/, bool /*Echo*/,
  unsigned int /*Timer*/)
{
  return false;
}
//---------------------------------------------------------------------------
int __fastcall TNullConsole::Choice(
  UnicodeString /*Options*/, int /*Cancel*/, int Break, int /*Continue*/, int Timeouted, bool Timeouting,
  unsigned int Timer, UnicodeString /*Message*/)
{
  int Result;
  if (Timeouting)
  {
    Sleep(Timer);
    Result = Timeouted;
  }
  else
  {
    Result = Break;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TNullConsole::PendingAbort()
{
  return false;
}
//---------------------------------------------------------------------------
void __fastcall TNullConsole::SetTitle(UnicodeString /*Title*/)
{
  // noop
}
//---------------------------------------------------------------------------
bool __fastcall TNullConsole::HasFlag(TConsoleFlag Flag) const
{
  switch (Flag)
  {
    // do not matter, even if we return false,
    // it fails immediately afterwards in TNullConsole::Input
    case cfNoInteractiveInput:
      return true;

    case cfLimitedOutput:
    case cfLiveOutput:
    case cfInteractive:
    case cfCommandLineOnly:
    case cfWantsProgress:
    case cfStdOut:
    case cfStdIn:
      return false;

    default:
      DebugFail();
      return false;
  }

}
//---------------------------------------------------------------------------
void __fastcall TNullConsole::WaitBeforeExit()
{
  DebugFail();
  // noop
}
//---------------------------------------------------------------------------
void __fastcall TNullConsole::Progress(TScriptProgress & /*Progress*/)
{
  DebugFail();
}
//---------------------------------------------------------------------------
void __fastcall TNullConsole::TransferOut(const unsigned char * DebugUsedArg(Data), size_t DebugUsedArg(Len))
{
  DebugFail();
}
//---------------------------------------------------------------------------
size_t __fastcall TNullConsole::TransferIn(unsigned char * DebugUsedArg(Data), size_t DebugUsedArg(Len))
{
  DebugFail();
  return 0;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TNullConsole::FinalLogMessage()
{
  return UnicodeString();
}
//---------------------------------------------------------------------------
static UnicodeString TimestampVarName(L"TIMESTAMP");
//---------------------------------------------------------------------------
class TConsoleRunner
{
public:
  TConsoleRunner(TConsole * Console);
  ~TConsoleRunner();

  int Run(
    const UnicodeString Session, TOptions * Options, TStrings * ScriptCommands, TStrings * ScriptParameters,
    bool UsageWarnings);
  void __fastcall ShowException(Exception * E);
  inline void __fastcall PrintMessage(const UnicodeString & Str, bool Error = false);

protected:
  bool __fastcall DoInput(UnicodeString & Str, bool Echo, unsigned int Timer,
    bool Interactive);
  void __fastcall Input(const UnicodeString Prompt, UnicodeString & Str,
    bool Echo, bool Interactive);
  inline void __fastcall Print(const UnicodeString & Str, bool FromBeginning = false, bool Error = false);
  void __fastcall UpdateTitle();
  inline bool __fastcall NotifyAbort();
  inline bool __fastcall Aborted(bool AllowCompleteAbort = true);
  void __fastcall MasterPasswordPrompt();
  void __fastcall DoShowException(TTerminal * Terminal, Exception * E);

private:
  TManagementScript * FScript;
  TConsole * FConsole;
  TSynchronizeController FSynchronizeController;
  int FLastProgressLen;
  bool FSynchronizeAborted;
  bool FCommandError;
  bool FBatchScript;
  bool FAborted;
  bool FExternalTimestampVar;

  void __fastcall ScriptPrint(TScript * Script, const UnicodeString Str, bool Error);
  void __fastcall ScriptPrintProgress(TScript * Script, bool First, const UnicodeString Str);
  void __fastcall ScriptInput(TScript * Script, const UnicodeString Prompt, UnicodeString & Str);
  void __fastcall ScriptTerminalPromptUser(TTerminal * Terminal,
    TPromptKind Kind, UnicodeString Name, UnicodeString Instructions, TStrings * Prompts,
    TStrings * Results, bool & Result, void * Arg);
  void __fastcall ScriptShowExtendedException(TTerminal * Terminal,
    Exception * E, void * Arg);
  void __fastcall ScriptTerminalQueryUser(TObject * Sender, const UnicodeString Query,
    TStrings * MoreMessages, unsigned int Answers, const TQueryParams * Params, unsigned int & Answer,
    TQueryType QueryType, void * Arg);
  void __fastcall ScriptQueryCancel(TScript * Script, bool & Cancel);
  void __fastcall SynchronizeControllerAbort(TObject * Sender, bool Close);
  void __fastcall SynchronizeControllerLog(TSynchronizeController * Controller,
    TSynchronizeLogEntry Entry, const UnicodeString Message);
  void __fastcall ScriptSynchronizeStartStop(TScript * Script,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
    const TCopyParamType & CopyParam, int SynchronizeParams);
  void __fastcall SynchronizeControllerSynchronize(TSynchronizeController * Sender,
    const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
    const TCopyParamType & CopyParam, const TSynchronizeParamType & Params,
    TSynchronizeChecklist ** Checklist, TSynchronizeOptions * Options, bool Full);
  void __fastcall SynchronizeControllerSynchronizeInvalid(TSynchronizeController * Sender,
    const UnicodeString Directory, const UnicodeString ErrorStr);
  void __fastcall SynchronizeControllerTooManyDirectories(TSynchronizeController * Sender,
    int & MaxDirectories);
  unsigned int InputTimeout();
  UnicodeString ExpandCommand(UnicodeString Command, TStrings * ScriptParameters);
  void __fastcall Failed(bool & AnyError);
  void __fastcall ScriptProgress(TScript * Script, TScriptProgress & Progress);
  void __fastcall ScriptTransferOut(TObject *, const unsigned char * Data, size_t Len);
  size_t __fastcall ScriptTransferIn(TObject *, unsigned char * Data, size_t Len);
  void __fastcall ConfigurationChange(TObject * Sender);
};
//---------------------------------------------------------------------------
TConsoleRunner::TConsoleRunner(TConsole * Console) :
  FSynchronizeController(::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerSynchronize),
    ::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerSynchronizeInvalid),
    ::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerTooManyDirectories))
{
  FConsole = Console;
  FLastProgressLen = 0;
  FScript = NULL;
  FAborted = false;
  FBatchScript = false;
  DebugAssert(AppConfiguration->OnMasterPasswordPrompt == NULL);
  AppConfiguration->OnMasterPasswordPrompt = ::rtl::MethodPtr(this, &TConsoleRunner::MasterPasswordPrompt);
  DebugAssert(Configuration->OnChange == NULL);
  FExternalTimestampVar = !GetEnvironmentVariable(TimestampVarName).IsEmpty();
  Configuration->OnChange = ::rtl::MethodPtr(this, &TConsoleRunner::ConfigurationChange);
  Configuration->Scripting = true;
}
//---------------------------------------------------------------------------
TConsoleRunner::~TConsoleRunner()
{
  DebugAssert(AppConfiguration->OnMasterPasswordPrompt == MasterPasswordPrompt);
  AppConfiguration->OnMasterPasswordPrompt = NULL;
  DebugAssert(Configuration->OnChange == ConfigurationChange);
  Configuration->OnChange = NULL;
}
//---------------------------------------------------------------------------
unsigned int TConsoleRunner::InputTimeout()
{
  return ((FScript != NULL) && (FScript->Batch != TScript::BatchOff) ? BATCH_INPUT_TIMEOUT : 0);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::Input(
  const UnicodeString Prompt, UnicodeString & Str, bool Echo, bool Interactive)
{
  Print(Prompt);

  if (!DoInput(Str, Echo, InputTimeout(), Interactive))
  {
    Abort();
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptInput(TScript * /*Script*/,
  const UnicodeString Prompt, UnicodeString & Str)
{
  Input(Prompt, Str, true, true);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::Print(const UnicodeString & Str, bool FromBeginning, bool Error)
{
  if (FLastProgressLen > 0)
  {
    FConsole->Print(L"\n" + Str, FromBeginning, Error);
    FLastProgressLen = 0;
  }
  else
  {
    FConsole->Print(Str, FromBeginning, Error);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::PrintMessage(const UnicodeString & Str, bool Error)
{
  UnicodeString Line = RemoveEmptyLines(Str);

  if (FScript != NULL)
  {
    // this also logs the message
    FScript->PrintLine(Line, Error);
  }
  else
  {
    FConsole->PrintLine(Line, Error);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TConsoleRunner::NotifyAbort()
{
  bool Result = FBatchScript;
  if (Result)
  {
    FAborted = true;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TConsoleRunner::Aborted(bool AllowCompleteAbort)
{
  bool Result;
  if (FAborted)
  {
    Result = true;
  }
  else
  {
    Result = FConsole->PendingAbort();
    if (Result)
    {
      PrintMessage(LoadStr(USER_TERMINATED), true);

      if (AllowCompleteAbort && NotifyAbort())
      {
        if (FScript->Terminal != NULL)
        {
          std::unique_ptr<TStringList> Failure(TextToStringList(LoadStr(USER_TERMINATED)));
          FScript->Terminal->ActionLog->AddFailure(Failure.get());
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptPrint(TScript * /*Script*/,
  const UnicodeString Str, bool Error)
{
  Print(Str, false, Error);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptPrintProgress(TScript * /*Script*/,
  bool First, const UnicodeString Str)
{
  UnicodeString S = Str;
  if (First && (FLastProgressLen > 0))
  {
    S = L"\n" + S;
  }
  else if (S.Length() < FLastProgressLen)
  {
    int Padding = FLastProgressLen - S.Length();
    S += UnicodeString::StringOfChar(L' ', Padding) +
      UnicodeString::StringOfChar(L'\b', Padding);
  }
  FConsole->Print(S, true);
  FLastProgressLen = Str.Length();
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptTerminalPromptUser(TTerminal * /*Terminal*/,
  TPromptKind /*Kind*/, UnicodeString DebugUsedArg(Name), UnicodeString Instructions, TStrings * Prompts,
  TStrings * Results, bool & Result, void * /*Arg*/)
{
  if (!Instructions.IsEmpty())
  {
    PrintMessage(Instructions);
  }

  // if there are no prompts, success is default
  Result = true;

  for (int Index = 0; Index < Prompts->Count; Index++)
  {
    UnicodeString Prompt = Prompts->Strings[Index];
    if (!Prompt.IsEmpty() && (Prompt[Prompt.Length()] != L' '))
    {
      Prompt += L' ';
    }
    int P = Prompt.Pos(L'&');
    if (P > 0)
    {
      Prompt.Delete(P, 1);
    }
    Print(Prompt);

    UnicodeString AResult = Results->Strings[Index]; // useless
    bool Echo = FLAGSET(int(reinterpret_cast<intptr_t>(Prompts->Objects[Index])), pupEcho);
    Result = DoInput(AResult, Echo, InputTimeout(), true);
    Results->Strings[Index] = AResult;
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptShowExtendedException(
  TTerminal * Terminal, Exception * E, void * /*Arg*/)
{
  DoShowException(Terminal, E);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptTerminalQueryUser(TObject * /*Sender*/,
  const UnicodeString Query, TStrings * MoreMessages, unsigned int Answers,
  const TQueryParams * Params, unsigned int & Answer, TQueryType /*QueryType*/,
  void * /*Arg*/)
{
  UnicodeString AQuery = Query;
  unsigned int Timer = 0;
  unsigned int Timeout = 0;
  unsigned int TimeoutA = 0;
  unsigned int TimeoutR = 0;
  unsigned int NoBatchA = 0;

  if (Params != NULL)
  {
    if (Params->Timeout > 0)
    {
      Timeout = Params->Timeout;
      TimeoutA = Params->TimeoutAnswer;
      TimeoutR = Params->TimeoutResponse;
      DebugAssert((TimeoutA != 0) && (TimeoutR != 0));
    }

    if (Params->Timer > 0)
    {
      Timer = Params->Timer;
      if (Params->TimerAnswers > 0)
      {
        Answers = Params->TimerAnswers;
      }
      // not considering TimerQueryType as we do not use QueryType anyway
      if (!Params->TimerMessage.IsEmpty())
      {
        AQuery = Params->TimerMessage;
      }
    }

    if (FLAGSET(Params->Params, qpFatalAbort))
    {
      AQuery = FMTLOAD(WARN_FATAL_ERROR, (AQuery));
    }

    NoBatchA = Params->NoBatchAnswers;

    if (FLAGSET(Params->Params, qpWaitInBatch))
    {
      DebugAssert(Timeout == 0);
      Timeout = InputTimeout();
      if (Timeout != 0)
      {
        DebugAssert((TimeoutA == 0) && (TimeoutR == 0));
        // See a duplicate AbortAnswer call below
        TimeoutA = AbortAnswer(Answers & ~NoBatchA);
        TimeoutR = TimeoutA;
      }
    }
  }

  AQuery = UnformatMessage(AQuery);
  AQuery = RemoveInteractiveMsgTag(AQuery);

  ApplyTabs(AQuery, L' ', NULL, NULL);

  unsigned int AAnswers = Answers;

  UnicodeString Message = AQuery;
  PrintMessage(AQuery);
  if ((MoreMessages != NULL) && (MoreMessages->Count > 0))
  {
    PrintMessage(MoreMessages->Text);
    Message += L"\n" + MoreMessages->Text;
  }

  std::vector<unsigned int> Buttons;
  std::vector<UnicodeString> Captions;
  std::vector<TButtonSubmitEvent> OnSubmits;

  for (unsigned int Answer = qaFirst; Answer <= qaLast; Answer = Answer << 1)
  {
    if (FLAGSET(Answers, Answer))
    {
      UnicodeString Name; // unused
      UnicodeString Caption;
      AnswerNameAndCaption(Answer, Name, Caption);
      Captions.push_back(Caption);
      Buttons.push_back(Answer);
      OnSubmits.push_back(NULL);
      AAnswers -= Answer;
    }
  }

  DebugUsedParam(AAnswers);
  DebugAssert(AAnswers == 0);
  DebugAssert(!Buttons.empty());

  if ((Params != NULL) && (Params->Aliases != NULL))
  {
    for (unsigned int bi = 0; bi < Buttons.size(); bi++)
    {
      for (unsigned int ai = 0; ai < Params->AliasesCount; ai++)
      {
        if (Params->Aliases[ai].Button == Buttons[bi])
        {
          if (!Params->Aliases[ai].Alias.IsEmpty())
          {
            Captions[bi] = Params->Aliases[ai].Alias;
          }
          OnSubmits[bi] = Params->Aliases[ai].OnSubmit;
          break;
        }
      }
    }
  }

  UnicodeString Accels;
  for (unsigned int Index = 0; Index < Buttons.size(); Index++)
  {
    UnicodeString & Caption = Captions[Index];
    int P = Caption.Pos(L'&');
    if ((P > 0) && (P < Caption.Length()))
    {
      wchar_t Accel = AnsiUpperCase(Caption)[P + 1];
      if (Accels.Pos(Accel) > 0)
      {
        Caption.Delete(P, 1);
        Accels += L' ';
      }
      else
      {
        Accels += Accel;
      }
    }
    else
    {
      Accels += L' ';
    }
  }

  DebugAssert(Accels.Length() == static_cast<int>(Buttons.size()));
  int NumberAccel = 0;
  unsigned int CancelA = CancelAnswer(Answers);
  int CancelIndex = -1;
  // AbortAnswer call duplicated in qpWaitInBatch branch above
  unsigned int AbortA = AbortAnswer(Answers & ~NoBatchA);
  int AbortIndex = -1;
  unsigned int ContinueA = ContinueAnswer(Answers & ~NoBatchA);
  int ContinueIndex = -1;
  int TimeoutIndex = 0;

  for (unsigned int Index = 0; Index < Buttons.size(); Index++)
  {
    UnicodeString & Caption = Captions[Index];

    if (Accels[Index + 1] == L' ')
    {
      for (int Index2 = 1; Index2 <= Caption.Length(); Index2++)
      {
        wchar_t C = AnsiUpperCase(Caption)[Index2];
        if (IsLetter(C) && (Accels.Pos(C) == 0))
        {
          Caption.Insert(L"&", Index2);
          Accels[Index + 1] = C;
          break;
        }
      }
    }

    if (Accels[Index + 1] == L' ')
    {
      for (int Index2 = 1; Index2 <= Caption.Length(); Index2++)
      {
        wchar_t C = AnsiUpperCase(Caption)[Index2];
        if ((C != L' ') && (Accels.Pos(C) == 0))
        {
          Caption.Insert(L"&", Index2);
          Accels[Index + 1] = C;
          break;
        }
      }
    }

    if (Accels[Index + 1] == L' ')
    {
      NumberAccel++;
      DebugAssert(NumberAccel <= 9);
      Caption = FORMAT(L"&%d%s", (NumberAccel, Caption));
      Accels[Index + 1] = Caption[2];
    }

    if (Buttons[Index] == CancelA)
    {
      CancelIndex = Index + 1;
    }
    if (Buttons[Index] == AbortA)
    {
      AbortIndex = Index + 1;
    }
    if (Buttons[Index] == ContinueA)
    {
      ContinueIndex = Index + 1;
    }
    if (Buttons[Index] == TimeoutA)
    {
      TimeoutIndex = Index + 1;
    }
  }

  DebugAssert(Accels.Pos(L' ') == 0);

  bool Timeouting = (Timeout > 0);
  bool FirstOutput = true;

  do
  {
    Answer = 0;
    int AnswerIndex;
    bool Retry;

    do
    {
      Retry = false;

      if (FirstOutput || FConsole->HasFlag(cfLiveOutput))
      {
        UnicodeString Output;
        for (unsigned int i = 0; i < Buttons.size(); i++)
        {
          if (i > 0)
          {
            Output += L", ";
          }

          UnicodeString Caption = Captions[i];
          int P = Caption.Pos(L'&');
          if (DebugAlwaysTrue(P >= 0))
          {
            Caption[P] = L'(';
            Caption.Insert(L")", P + 2);
          }

          if (i + 1 == static_cast<unsigned int>(TimeoutIndex))
          {
            DebugAssert(Timeouting);
            Caption = FMTLOAD(TIMEOUT_BUTTON, (Caption, int(Timeout / MSecsPerSec)));
          }

          Output += Caption;
        }
        Output += L": ";

        // note that length of string may decrease over time due to number of
        // seconds length, but supposing it decreases by one character at time
        // at most, we do not mind as the prompt is terminated with space

        // If output is not live (file or pipe), do not use 'from beginning'
        // as it means that the output is not actually stored until new line
        // is sent (and we will not [because we cannot] rewrite the output anyway)
        Print(Output, !FirstOutput);
        FirstOutput = false;
      }

      if (!Timeouting && (FScript->Batch == TScript::BatchContinue))
      {
        AnswerIndex = ContinueIndex;
      }
      else if (!Timeouting && (FScript->Batch != TScript::BatchOff))
      {
        AnswerIndex = AbortIndex;
      }
      else if (Timeouting && (Timeout < MSecsPerSec))
      {
        AnswerIndex = TimeoutIndex;
        Answer = TimeoutR;
      }
      else
      {
        unsigned int ActualTimer;
        if (Timeouting)
        {
          if (Timer == 0)
          {
            if (FConsole->HasFlag(cfNoInteractiveInput))
            {
              ActualTimer = Timeout;
            }
            else
            {
              ActualTimer = MSecsPerSec;
            }
          }
          else
          {
            if (Timer < MSecsPerSec)
            {
              ActualTimer = Timer;
            }
            else
            {
              ActualTimer = MSecsPerSec;
            }
          }
        }
        else
        {
          ActualTimer = Timer;
        }
        // Not to get preliminary "host is not responding" messages to .NET assembly
        if (FConsole->HasFlag(cfNoInteractiveInput) && (Timer > 0))
        {
          Sleep(Timer);
          AnswerIndex = -2;
        }
        else
        {
          AnswerIndex =
            FConsole->Choice(Accels, CancelIndex, -1, ContinueIndex, -2, Timeouting, ActualTimer, Message);
        }
        if (AnswerIndex == -1)
        {
          NotifyAbort();
          AnswerIndex = AbortIndex;
        }
        else if (AnswerIndex == -2)
        {
          if (Timeouting)
          {
            DebugAssert(Timeout >= MSecsPerSec);
            Timeout -= ActualTimer;
            Retry = true;
          }

          // this does not take Timer into account,
          // but as of now Timer is used for TSecureShell timeout prompt only,
          // where Timer is less than MSecsPerSec
          if (Timer > 0)
          {
            DebugAssert((Params != NULL) && (Params->TimerEvent != NULL));
            if ((Params != NULL) && (Params->TimerEvent != NULL))
            {
              unsigned int AAnswer = 0;
              Params->TimerEvent(AAnswer);
              if (AAnswer != 0)
              {
                Answer = AAnswer;
                Retry = false;
              }
              else
              {
                Retry = true;
              }
            }
          }
        }
      }
    }
    while (Retry);

    if (Answer == 0)
    {
      DebugAssert((AnswerIndex >= 1) && (AnswerIndex <= Accels.Length()));
      UnicodeString AnswerCaption = Captions[AnswerIndex - 1];
      int P = AnswerCaption.Pos(L"&");
      DebugAssert(P >= 0);
      AnswerCaption.Delete(P, 1);
      FConsole->PrintLine(AnswerCaption);
      FirstOutput = true;

      if (OnSubmits[AnswerIndex - 1] != NULL)
      {
        OnSubmits[AnswerIndex - 1](NULL, Answer);
      }
      else
      {
        Answer = Buttons[AnswerIndex - 1];
      }
    }
    else
    {
      FConsole->PrintLine();
    }
  }
  while (Answer == 0);

  if ((Answer == AbortA) &&
      ((Params == NULL) || FLAGCLEAR(Params->Params, qpIgnoreAbort)))
  {
    if (FScript->Terminal != NULL)
    {
      TStrings * Messages = new TStringList();
      { ::rtl::TFinally Finally([&]()
      {
        delete Messages;
      }); try
      {
        Messages->Add(Query);
        if (MoreMessages != NULL)
        {
          Messages->AddStrings(MoreMessages);
        }
        FScript->Terminal->ActionLog->AddFailure(Messages);
      }
      catch (...) { Finally.Unwind(); } }
    }

    FCommandError = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptQueryCancel(TScript * /*Script*/, bool & Cancel)
{
  if (Aborted())
  {
    Cancel = true;
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptSynchronizeStartStop(TScript * /*Script*/,
  const UnicodeString LocalDirectory, const UnicodeString RemoteDirectory,
  const TCopyParamType & CopyParam, int SynchronizeParams)
{
  TSynchronizeParamType Params;
  Params.LocalDirectory = LocalDirectory;
  Params.RemoteDirectory = RemoteDirectory;
  Params.Params = SynchronizeParams;
  Params.Options = soRecurse;

  FSynchronizeController.StartStop(NULL, true, Params,
    CopyParam, NULL, ::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerAbort), NULL,
    ::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerLog));

  { ::rtl::TFinally Finally([&]()
  {
    FSynchronizeController.StartStop(NULL, false, Params,
      CopyParam, NULL, ::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerAbort), NULL,
      ::rtl::MethodPtr(this, &TConsoleRunner::SynchronizeControllerLog));
  }); try
  {
    FSynchronizeAborted = false;

    while (!FSynchronizeAborted && !Aborted(false))
    {
      // OpenSCP: the changes are reported through TThread::Synchronize
      CheckSynchronize(MSecsPerSec);
      FScript->Terminal->Idle();
    }
  }
  catch (...) { Finally.Unwind(); } }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptProgress(TScript * /*Script*/, TScriptProgress & Progress)
{
  FConsole->Progress(Progress);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ScriptTransferOut(TObject *, const unsigned char * Data, size_t Len)
{
  FConsole->TransferOut(Data, Len);
}
//---------------------------------------------------------------------------
size_t __fastcall TConsoleRunner::ScriptTransferIn(TObject *, unsigned char * Data, size_t Len)
{
  return FConsole->TransferIn(Data, Len);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::SynchronizeControllerLog(
  TSynchronizeController * /*Controller*/, TSynchronizeLogEntry /*Entry*/,
  const UnicodeString Message)
{
  PrintMessage(Message);
  LogSynchronizeEvent(FScript->Terminal, Message);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::SynchronizeControllerAbort(TObject * /*Sender*/,
  bool /*Close*/)
{
  FSynchronizeAborted = true;
  NotifyAbort();
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::SynchronizeControllerSynchronize(
  TSynchronizeController * /*Sender*/, const UnicodeString LocalDirectory,
  const UnicodeString RemoteDirectory, const TCopyParamType & CopyParam,
  const TSynchronizeParamType & Params, TSynchronizeChecklist ** Checklist,
  TSynchronizeOptions * /*Options*/, bool Full)
{
  if (!Full)
  {
    try
    {
      FScript->Synchronize(LocalDirectory, RemoteDirectory, CopyParam,
        Params.Params, Checklist);
    }
    catch (Exception &)
    {
      if ((FScript->Batch == TScript::BatchContinue) &&
          FScript->Terminal->Active)
      {
        // noop
      }
      else
      {
        throw;
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::SynchronizeControllerSynchronizeInvalid(
  TSynchronizeController * /*Sender*/, const UnicodeString Directory, const UnicodeString ErrorStr)
{
  if (!Directory.IsEmpty())
  {
    PrintMessage(FMTLOAD(WATCH_ERROR_DIRECTORY, (Directory)), true);
  }
  else
  {
    PrintMessage(LoadStr(WATCH_ERROR_GENERAL), true);
  }

  if (!ErrorStr.IsEmpty())
  {
    PrintMessage(ErrorStr, true);
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::SynchronizeControllerTooManyDirectories(
  TSynchronizeController * /*Sender*/, int & MaxDirectories)
{
  if (Aborted())
  {
    Abort();
  }

  if (MaxDirectories < GUIConfiguration->MaxWatchDirectories)
  {
    MaxDirectories = GUIConfiguration->MaxWatchDirectories;
  }
  else
  {
    MaxDirectories *= 2;
  }
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ShowException(Exception * E)
{
  DoShowException(NULL, E);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::DoShowException(TTerminal * Terminal, Exception * E)
{
  if ((Terminal == NULL) && (FScript != NULL))
  {
    Terminal = FScript->Terminal;
  }

  UnicodeString Message;
  if (ExceptionFullMessage(E, Message))
  {
    FCommandError = true;
    PrintMessage(Message, true);
  }

  TTerminal * LoggingTerminal = Terminal;
  TSecondaryTerminal * SecondaryTerminal = dynamic_cast<TSecondaryTerminal *>(LoggingTerminal);
  if (SecondaryTerminal != NULL)
  {
    LoggingTerminal = SecondaryTerminal->MainTerminal;
  }

  if (LoggingTerminal != NULL)
  {
    LoggingTerminal->ActionLog->AddFailure(E);
  }
}
//---------------------------------------------------------------------------
bool __fastcall TConsoleRunner::DoInput(UnicodeString & Str, bool Echo,
  unsigned int Timeout, bool Interactive)
{
  bool Result;
  if (Interactive && FConsole->HasFlag(cfNoInteractiveInput))
  {
    Result = false;
  }
  else
  {
    Result = FConsole->Input(Str, Echo, Timeout);
  }

  if (!Result)
  {
    NotifyAbort();
  }

  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::MasterPasswordPrompt()
{
  bool Retry;
  do
  {
    UnicodeString Password;
    Input(LoadStr(CONSOLE_MASTER_PASSWORD_PROMPT), Password, false, true);
    Retry = !AppConfiguration->ValidateMasterPassword(Password);
    if (Retry)
    {
      FConsole->PrintLine(LoadStr(MASTER_PASSWORD_INCORRECT));
    }
    else
    {
      AppConfiguration->SetMasterPassword(Password);
    }
  }
  while (Retry);

}
//---------------------------------------------------------------------------
UnicodeString TConsoleRunner::ExpandCommand(UnicodeString Command, TStrings * ScriptParameters)
{
  DebugAssert(ScriptParameters != NULL);
  for (int Index = 0; Index < ScriptParameters->Count; Index++)
  {
    Command = ReplaceStr(Command, FORMAT(L"%%%d%%", (Index+1)),
      ScriptParameters->Strings[Index]);
  }

  TDateTime N = Now();

  if (!FExternalTimestampVar)
  {
    Command =
      ReplaceStr(Command, FORMAT(L"%%%s%%", (TimestampVarName)), FormatDateTime(L"yyyymmddhhnnss", N));
  }

  int Offset = 1;
  int P2;
  do
  {
    int P = Pos(UpperCase(L"%" + TimestampVarName), UpperCase(Command), Offset);
    if (P > 0)
    {
      Offset = P + 1 + TimestampVarName.Length();
      P2 = Pos(L"%", Command, Offset);
      int P3 = Pos(L"#", Command, Offset);
      if ((P2 > 0) && (P3 > 0) && (P3 < P2) &&
          ((P3 == Offset) || (Command[Offset] == L'+') || (Command[Offset] == L'-')))
      {
        bool Valid = true;
        TDateTime T = N;
        if (P3 > Offset)
        {
          bool Add = (Command[Offset] == L'+');
          Offset++;
          Valid = TryRelativeStrToDateTime(Command.SubString(Offset, P3 - Offset), T, Add);
        }

        Offset = P3 + 1;
        if (Valid)
        {
          UnicodeString TimestampFormat = Command.SubString(Offset, P2 - Offset);
          UnicodeString TimestampValue = FormatDateTime(TimestampFormat, T);
          Command = Command.SubString(1, P - 1) + TimestampValue + Command.SubString(P2 + 1, Command.Length() - P2);
          Offset = P + TimestampValue.Length();
        }
      }
    }
    else
    {
      P2 = 0;
    }
  }
  while (P2 > 0);

  Command = ExpandEnvironmentVariables(Command);
  return Command;
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::Failed(bool & AnyError)
{
  if (FScript != NULL)
  {
    FScript->Log(llMessage, L"Failed");
  }
  AnyError = true;
}
//---------------------------------------------------------------------------
int TConsoleRunner::Run(
  const UnicodeString Session, TOptions * Options, TStrings * ScriptCommands, TStrings * ScriptParameters,
  bool UsageWarnings)
{
  int ExitCode;
  { ::rtl::TFinally Finally([&]()
  {
    delete FScript;
    FScript = NULL;
  }); try
  {
    bool AnyError = false;

    try
    {
      FScript = new TManagementScript(StoredSessions, FConsole->HasFlag(cfLimitedOutput));

      FScript->CopyParam = GUIConfiguration->DefaultCopyParam;
      FScript->SynchronizeParams = GUIConfiguration->SynchronizeParams;
      FScript->WantsProgress = FConsole->HasFlag(cfWantsProgress);
      FScript->OnPrint = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptPrint);
      FScript->OnPrintProgress = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptPrintProgress);
      FScript->OnInput = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptInput);
      FScript->OnTerminalPromptUser = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptTerminalPromptUser);
      FScript->OnShowExtendedException = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptShowExtendedException);
      FScript->OnTerminalQueryUser = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptTerminalQueryUser);
      FScript->OnQueryCancel = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptQueryCancel);
      FScript->OnSynchronizeStartStop = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptSynchronizeStartStop);
      FScript->OnProgress = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptProgress);
      FScript->UsageWarnings = UsageWarnings;
      if (FConsole->HasFlag(cfStdOut))
      {
        FScript->OnTransferOut = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptTransferOut);
      }
      if (FConsole->HasFlag(cfStdIn))
      {
        FScript->OnTransferIn = ::rtl::MethodPtr(this, &TConsoleRunner::ScriptTransferIn);
      }

      UpdateTitle();

      // everything until the first manually entered command is "batch"
      // (including opening session from command line and script file)
      FBatchScript = true;

      if (!Session.IsEmpty())
      {
        if (UsageWarnings)
        {
          PrintMessage(LoadStr(SCRIPT_CMDLINE_SESSION));
        }
        FCommandError = false;
        FScript->Connect(Session, Options, false);
        if (FCommandError)
        {
          Failed(AnyError);
        }
      }

      FScript->Groups = Options->SwitchValue(L"xmlgroups", true, false);

      int ScriptPos = 0;
      bool Result;
      do
      {
        UpdateTitle();

        UnicodeString Command;
        if ((ScriptCommands != NULL) && (ScriptPos < ScriptCommands->Count))
        {
          Result = true;
          Command = ScriptCommands->Strings[ScriptPos];
          ScriptPos++;
        }
        else
        {
          if (FBatchScript)
          {
            // no longer batch
            FBatchScript = false;
            FScript->StartInteractive();
          }
          Print(L"winscp> ");
          Result = DoInput(Command, true, 0, false);
        }

        if (Result)
        {
          FCommandError = false;
          FScript->Command(ExpandCommand(Command, ScriptParameters));

          if (FCommandError)
          {
            Failed(AnyError);
            if (FScript->Batch == TScript::BatchAbort)
            {
              Result = false;
            }
          }

          if (FScript->Terminal != NULL)
          {
            FScript->Terminal->Idle();
          }
        }
      }
      while (Result && FScript->Continue && !Aborted());
    }
    catch(Exception & E)
    {
      Failed(AnyError);
      ShowException(&E);
    }

    if (FLastProgressLen > 0)
    {
      FConsole->Print(L"\n");
      FLastProgressLen = 0;
    }

    ExitCode = (AnyError || FAborted) ? RESULT_ANY_ERROR : RESULT_SUCCESS;

    if (FScript != NULL)
    {
      UnicodeString ExitCodeMessage = FORMAT(L"Exit code: %d", (ExitCode));
      FScript->Log(llMessage, ExitCodeMessage);
      if (Configuration->ActualLogProtocol >= 1)
      {
        FConsole->Print(ExitCodeMessage + L"\n");
        UnicodeString LogMessage = FConsole->FinalLogMessage();
        if (!LogMessage.IsEmpty())
        {
          FScript->Log(llMessage, LogMessage);
          FConsole->Print(LogMessage + L"\n");
        }
      }
    }
  }
  catch (...) { Finally.Unwind(); } }

  return ExitCode;
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::UpdateTitle()
{
  UnicodeString NewTitle;
  if (FScript->Terminal != NULL)
  {
    NewTitle = FormatMainFormCaption(FScript->Terminal->SessionData->SessionName);
  }
  else
  {
    NewTitle = FormatMainFormCaption(L"");
  }
  FConsole->SetTitle(NewTitle);
}
//---------------------------------------------------------------------------
void __fastcall TConsoleRunner::ConfigurationChange(TObject * /*Sender*/)
{
  if (FScript != NULL)
  {
    FScript->ReflectSettings();
  }
}
//---------------------------------------------------------------------------
static UnicodeString __fastcall GetExeBaseName()
{
  return ExtractFileBaseName(ParamStr(0));
}
//---------------------------------------------------------------------------
static void __fastcall PrintUsageSyntax(TConsole * Console, const UnicodeString & Str)
{
  Console->PrintLine(GetExeBaseName() + L" " + Str);
}
//---------------------------------------------------------------------------
typedef std::vector<std::pair<UnicodeString, UnicodeString> > TSwitchesUsage;
//---------------------------------------------------------------------------
static void __fastcall RegisterSwitch(
  TSwitchesUsage & SwitchesUsage, const UnicodeString & Name, const UnicodeString & Desc)
{
  SwitchesUsage.push_back(std::make_pair(LowerCase(Name), Desc));
}
//---------------------------------------------------------------------------
static void __fastcall RegisterSwitch(
  TSwitchesUsage & SwitchesUsage, const UnicodeString & Name, int DescID)
{
  UnicodeString Desc = LoadStr(DescID);
  Desc = ReplaceText(Desc, L"%APP%", GetExeBaseName());
  RegisterSwitch(SwitchesUsage, Name, Desc);
}
//---------------------------------------------------------------------------
void __fastcall Usage(TConsole * Console)
{
  Console->PrintLine(FORMAT(L"%s, %s", (AppName, Configuration->VersionStr)));
  UnicodeString Copyright =
    ReplaceText(LoadStr(WINSCP_COPYRIGHT), L"©", L"(c)");
  Console->PrintLine(Copyright);
  Console->PrintLine();
  Console->PrintLine(LoadStr(USAGE_SYNTAX_LABEL));

  bool CommandLineOnly = Console->HasFlag(cfCommandLineOnly);
  if (!CommandLineOnly)
  {
    PrintUsageSyntax(Console, L"site|workspace|folder");
    PrintUsageSyntax(Console, L"(sftp|scp|ftp[es]|dav[s]|s3)://[user[:password]@]host[:port][/path/[file]]");
    PrintUsageSyntax(Console, FORMAT(L"[mysession] /%s=<name>", (LowerCase(SESSIONNAME_SWICH))));
    PrintUsageSyntax(Console, L"[mysession] /newinstance");
    PrintUsageSyntax(Console, L"[mysession] /edit <path>");
    PrintUsageSyntax(Console, FORMAT(L"[mysession] /%s[=<file>]", (LowerCase(EXPLORE_SWITCH))));
    PrintUsageSyntax(Console, FORMAT(L"[mysession] /%s [local_dir] [remote_dir] [/%s]", (LowerCase(SYNCHRONIZE_SWITCH), LowerCase(DEFAULTS_SWITCH))));
    PrintUsageSyntax(Console, FORMAT(L"[mysession] /%s [local_dir] [remote_dir] [/%s]", (LowerCase(KEEP_UP_TO_DATE_SWITCH), LowerCase(DEFAULTS_SWITCH))));
    PrintUsageSyntax(Console, FORMAT(L"[mysession] /%s [path]", (LowerCase(REFRESH_SWITCH))));
    PrintUsageSyntax(Console, FORMAT(L"[mysession] [/%s=<file> [/%s=<passphrase>]]", (LowerCase(PRIVATEKEY_SWITCH), PassphraseOption)));
    PrintUsageSyntax(Console, L"[mysession] [/hostkey=<fingerprint>]");
    PrintUsageSyntax(Console, FORMAT(L"[mysession] [/%s=<user> [/%s=<password>]]", (LowerCase(USERNAME_SWITCH), LowerCase(PASSWORD_SWITCH))));
    PrintUsageSyntax(Console, FORMAT(L"[mysession] [/%s]", (LowerCase(PASSWORDSFROMFILES_SWITCH))));
    PrintUsageSyntax(Console, FORMAT(L"[mysession] [/clientcert=<file> [/%s=<passphrase>]]", (PassphraseOption)));
    PrintUsageSyntax(Console, L"[mysession] [/certificate=<fingerprint>]");
    PrintUsageSyntax(Console, L"[mysession] [/passive[=on|off]] [/implicit|explicit]");
    PrintUsageSyntax(Console, L"[mysession] [/timeout=<sec>]");
    PrintUsageSyntax(Console, L"[mysession] [/rawsettings setting1=value1 setting2=value2 ...]");
  }
  PrintUsageSyntax(Console,
    UnicodeString(!CommandLineOnly ? L"[/console] " : L"") +
    FORMAT(L"[/script=<file>] [/%s cmd1...] [/parameter // param1...]", (LowerCase(COMMAND_SWITCH))));
  if (CommandLineOnly)
  {
    PrintUsageSyntax(
      Console, FORMAT(L"[/%s[=%s|%s]] [/%s]",
      (LowerCase(STDOUT_SWITCH), STDINOUT_BINARY_VALUE, STDINOUT_CHUNKED_VALUE, LowerCase(STDIN_SWITCH))));
  }
  PrintUsageSyntax(Console,
    FORMAT(L"[/%s=<logfile> [/loglevel=<level>]] [/%s=[<count>%s]<size>]", (LowerCase(LOG_SWITCH), LowerCase(LOGSIZE_SWITCH), LOGSIZE_SEPARATOR)));
  if (!CommandLineOnly)
  {
    PrintUsageSyntax(Console, L"[/xmllog=<logfile> [/xmlgroups]]");
  }
  else
  {
    PrintUsageSyntax(Console, FORMAT(L"[/xmllog=<logfile> [/xmlgroups]] [/%s]", (LowerCase(NOINTERACTIVEINPUT_SWITCH))));
  }
  PrintUsageSyntax(Console,
    FORMAT(L"[/%s=<inifile>]", (LowerCase(INI_SWITCH))));
  PrintUsageSyntax(Console, FORMAT(L"[/%s config1=value1 config2=value2 ...]", (LowerCase(RAW_CONFIG_SWITCH))));
  PrintUsageSyntax(Console, FORMAT(L"[/%s setting1=value1 setting2=value2 ...]", (LowerCase(RAWTRANSFERSETTINGS_SWITCH))));
  PrintUsageSyntax(Console, L"/batchsettings <site_mask> setting1=value1 setting2=value2 ...");
  PrintUsageSyntax(Console, FORMAT(L"/%s keyfile [/%s=<file>] [/%s]", (
    LowerCase(KEYGEN_SWITCH), LowerCase(KEYGEN_OUTPUT_SWITCH), LowerCase(KEYGEN_CHANGE_PASSPHRASE_SWITCH))));
  PrintUsageSyntax(Console, FORMAT(L"/%s keyfile [/%s=<text>] [/%s=<file>]",
    (LowerCase(KEYGEN_SWITCH), LowerCase(KEYGEN_COMMENT_SWITCH), LowerCase(KEYGEN_CERTIFICATE_SWITCH))));
  PrintUsageSyntax(Console, FORMAT(L"/%s /%s=<file> mysession", (
    LowerCase(COPYID_SWITCH), LowerCase(IDENTITY_SWITCH))));
  if (!CommandLineOnly)
  {
    PrintUsageSyntax(Console, L"/update");
  }
  PrintUsageSyntax(Console, TProgramParams::FormatSwitch(LowerCase(INFO_SWITCH)));
  PrintUsageSyntax(Console, L"/help");

  Console->PrintLine();

  TSwitchesUsage SwitchesUsage;
  if (!CommandLineOnly)
  {
    RegisterSwitch(SwitchesUsage, L"session", USAGE_SESSION);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(SESSIONNAME_SWICH) + L"=", USAGE_SESSIONNAME);
    RegisterSwitch(SwitchesUsage, L"/newinstance", USAGE_NEWINSTANCE);
    RegisterSwitch(SwitchesUsage, L"/edit", USAGE_EDIT);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(EXPLORE_SWITCH), USAGE_EXPLORE);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(SYNCHRONIZE_SWITCH), USAGE_SYNCHRONIZE);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(KEEP_UP_TO_DATE_SWITCH), USAGE_KEEPUPTODATE);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(REFRESH_SWITCH), USAGE_REFRESH);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(DEFAULTS_SWITCH), USAGE_DEFAULTS);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(PRIVATEKEY_SWITCH) + L"=", USAGE_PRIVATEKEY);
    RegisterSwitch(SwitchesUsage, L"/hostkey=", USAGE_HOSTKEY);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(USERNAME_SWITCH), USAGE_USERNAME);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(PASSWORD_SWITCH), USAGE_PASSWORD);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(PASSWORDSFROMFILES_SWITCH), USAGE_PASSWORDSFROMFILES);
    RegisterSwitch(SwitchesUsage, L"/clientcert=", USAGE_CLIENTCERT);
    RegisterSwitch(SwitchesUsage, L"/certificate=", USAGE_CERTIFICATE);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(PassphraseOption) + L"=", USAGE_PASSPHRASE);
    RegisterSwitch(SwitchesUsage, L"/passive=", USAGE_PASSIVE);
    RegisterSwitch(SwitchesUsage, L"/implicit", USAGE_IMPLICIT);
    RegisterSwitch(SwitchesUsage, L"/explicit", USAGE_EXPLICIT);
    RegisterSwitch(SwitchesUsage, L"/timeout=", USAGE_TIMEOUT);
    RegisterSwitch(SwitchesUsage, L"/rawsettings", USAGE_RAWSETTINGS);
    RegisterSwitch(SwitchesUsage, L"/console", USAGE_CONSOLE);
  }
  RegisterSwitch(SwitchesUsage, L"/script=", USAGE_SCRIPT);
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(COMMAND_SWITCH), USAGE_COMMAND);
  RegisterSwitch(SwitchesUsage, L"/parameter", USAGE_PARAMETER);
  if (CommandLineOnly)
  {
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(STDOUT_SWITCH), USAGE_STDOUT);
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(STDIN_SWITCH), USAGE_STDIN);
  }
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(LOG_SWITCH) + L"=", USAGE_LOG);
  RegisterSwitch(SwitchesUsage, L"/loglevel=", USAGE_LOGLEVEL);
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(LOGSIZE_SWITCH) + L"=", USAGE_LOGSIZE);
  RegisterSwitch(SwitchesUsage, L"/xmllog=", USAGE_XMLLOG);
  RegisterSwitch(SwitchesUsage, L"/xmlgroups", USAGE_XMLGROUPS);
  if (CommandLineOnly)
  {
    RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(NOINTERACTIVEINPUT_SWITCH), USAGE_INTERACTIVEINPUT);
  }
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(INI_SWITCH) + L"=", USAGE_INI);
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(RAW_CONFIG_SWITCH), USAGE_RAWCONFIG);
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(RAWTRANSFERSETTINGS_SWITCH), USAGE_RAWTRANSFERSETTINGS);
  RegisterSwitch(SwitchesUsage, L"/batchsettings", USAGE_BATCHSETTINGS);
  UnicodeString KeyGenDesc =
    FMTLOAD(USAGE_KEYGEN2, (
      TProgramParams::FormatSwitch(LowerCase(KEYGEN_OUTPUT_SWITCH)) + L"=",
      TProgramParams::FormatSwitch(LowerCase(KEYGEN_CHANGE_PASSPHRASE_SWITCH)),
      TProgramParams::FormatSwitch(LowerCase(KEYGEN_COMMENT_SWITCH)) + L"=",
      TProgramParams::FormatSwitch(LowerCase(KEYGEN_CERTIFICATE_SWITCH)) + L"="));
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(KEYGEN_SWITCH), KeyGenDesc);
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(COPYID_SWITCH), USAGE_COPYID);
  if (!CommandLineOnly)
  {
    RegisterSwitch(SwitchesUsage, L"/update", USAGE_UPDATE);
  }
  RegisterSwitch(SwitchesUsage, TProgramParams::FormatSwitch(INFO_SWITCH), USAGE_INFO);
  RegisterSwitch(SwitchesUsage, L"/help", USAGE_HELP);

  int MaxSwitchLen = 0;
  TSwitchesUsage::const_iterator Index = SwitchesUsage.begin();
  while (Index != SwitchesUsage.end())
  {
    MaxSwitchLen = std::max(Index->first.Length(), MaxSwitchLen);
    ++Index;
  }

  Index = SwitchesUsage.begin();
  while (Index != SwitchesUsage.end())
  {
    UnicodeString Label =
      UnicodeString(L" ") +
      Index->first +
      UnicodeString::StringOfChar(L' ', MaxSwitchLen - Index->first.Length()) +
      L" ";
    Console->Print(Label);

    const int ConsoleWidth = 80;
    int DescWidth = ConsoleWidth - Label.Length() - 1;

    bool FirstLine = true;
    UnicodeString Desc = Index->second;
    while (!Desc.IsEmpty())
    {
      UnicodeString DescLine = CutToChar(Desc, L'\n', true);
      DescLine = WrapText(DescLine, L"\n", TSysCharSet() << L' ', DescWidth);
      while (!DescLine.IsEmpty())
      {
        UnicodeString DescLineLine = CutToChar(DescLine, L'\n', true);
        if (!FirstLine)
        {
          DescLineLine =
            UnicodeString::StringOfChar(L' ', Label.Length()) +
            DescLineLine;
        }
        FirstLine = false;
        Console->PrintLine(DescLineLine);
      }
    }
    ++Index;
  }

  Console->WaitBeforeExit();
}
//---------------------------------------------------------------------------
int __fastcall HandleException(TConsole * Console, Exception & E)
{
  UnicodeString Message;
  if (ExceptionFullMessage(&E, Message))
  {
    Console->Print(Message);
  }
  return RESULT_ANY_ERROR;
}
//---------------------------------------------------------------------------
int __fastcall BatchSettings(TConsole * Console, TProgramParams * Params)
{
  int Result = RESULT_SUCCESS;
  try
  {
    std::unique_ptr<TStrings> Arguments(new TStringList());
    if (!DebugAlwaysTrue(Params->FindSwitch(L"batchsettings", Arguments.get())))
    {
      Abort();
    }
    else
    {
      if (Arguments->Count < 1)
      {
        throw Exception(LoadStr(BATCH_SET_NO_MASK));
      }
      else if (Arguments->Count < 2)
      {
        throw Exception(LoadStr(BATCH_SET_NO_SETTINGS));
      }
      else
      {
        TFileMasks Mask(Arguments->Strings[0]);
        Arguments->Delete(0);

        std::unique_ptr<TOptionsStorage> OptionsStorage(new TOptionsStorage(Arguments.get(), false));

        int Matches = 0;
        int Changes = 0;

        for (int Index = 0; Index < StoredSessions->Count; Index++)
        {
          TSessionData * Data = StoredSessions->Sessions[Index];
          if (!Data->IsWorkspace &&
              Mask.Matches(Data->Name, false, false))
          {
            Matches++;
            std::unique_ptr<TSessionData> OriginalData(new TSessionData(L""));
            OriginalData->CopyDataNoRecrypt(Data);
            bool UnsafeSettings; // unused
            Data->ApplyRawSettings(OptionsStorage.get(), false, true, UnsafeSettings);
            bool Changed = !OriginalData->IsSame(Data, false);
            if (Changed)
            {
              Changes++;
            }
            UnicodeString StateStr = LoadStr(Changed ? BATCH_SET_CHANGED : BATCH_SET_NOT_CHANGED);
            Console->PrintLine(Data->Name + TitleSeparator + StateStr);
          }
        }

        StoredSessions->Save(false, true); // explicit
        Console->PrintLine(FMTLOAD(BATCH_SET_SUMMARY, (Matches, Changes)));
      }
    }
  }
  catch (Exception & E)
  {
    Result = HandleException(Console, E);
  }

  Console->WaitBeforeExit();
  return Result;
}
//---------------------------------------------------------------------------
bool FindNixCompatibleSwitch(
  TProgramParams * Params, const UnicodeString & Name, const UnicodeString & NixName, UnicodeString & Value, bool & Set)
{
  bool Result =
    !Name.IsEmpty() &&
    Params->FindSwitch(Name, Value, Set);
  if (!Result)
  {
    std::unique_ptr<TStrings> Args(new TStringList());
    Result = Params->FindSwitchCaseSensitive(NixName, Args.get(), 1);
    if (Result && (Args->Count >= 1))
    {
      Value = Args->Strings[0];
      Set = true;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall KeyGen(TConsole * Console, TProgramParams * Params)
{
  int Result = RESULT_SUCCESS;
  UnicodeString Passphrase;
  UnicodeString NewPassphrase;
  try
  {
    UnicodeString InputFileName;
    std::unique_ptr<TStrings> Args(new TStringList());
    if (!Params->FindSwitch(KEYGEN_SWITCH, Args.get(), 1) ||
        (Args->Count < 1) ||
        Args->Strings[0].IsEmpty())
    {
      throw Exception(LoadStr(KEYGEN_NO_INPUT));
    }
    InputFileName = Args->Strings[0];

    bool ValueSet;
    UnicodeString OutputFileName;
    FindNixCompatibleSwitch(Params, KEYGEN_OUTPUT_SWITCH, L"o", OutputFileName, ValueSet);

    UnicodeString NewComment;
    FindNixCompatibleSwitch(Params, KEYGEN_COMMENT_SWITCH, L"C", NewComment, ValueSet);

    bool ChangePassphrase;
    bool NewPassphraseSet;
    if (Params->FindSwitchCaseSensitive(L"P"))
    {
      ChangePassphrase = true;
      FindNixCompatibleSwitch(Params, EmptyStr, L"-new-passphrase", NewPassphrase, NewPassphraseSet);
    }
    else
    {
      ChangePassphrase = Params->FindSwitch(KEYGEN_CHANGE_PASSPHRASE_SWITCH, NewPassphrase, NewPassphraseSet);
    }

    UnicodeString Certificate;
    // It's --certificate in puttygen
    FindNixCompatibleSwitch(Params, KEYGEN_CERTIFICATE_SWITCH, L"-certificate", Certificate, ValueSet);

    FindNixCompatibleSwitch(Params, PassphraseOption, L"-old-passphrase", Passphrase, ValueSet);

    if (Params->ParamCount > 0)
    {
      throw Exception(LoadStr(TOO_MANY_PARAMS_ERROR));
    }

    TKeyType Type = KeyType(InputFileName);
    int Error = errno;
    switch (Type)
    {
      case ktSSH1:
        throw Exception(LoadStr(KEYGEN_SSH1));

      case ktSSH2:
        if (NewComment.IsEmpty() && !ChangePassphrase && Certificate.IsEmpty() &&
            (Configuration->KeyVersion == 0)) // We should better check for version change
        {
          throw Exception(LoadStr(KEYGEN_NO_ACTION));
        }
        break;

      case ktOpenSSHPEM:
      case ktOpenSSHNew:
      case ktSSHCom:
        if (OutputFileName.IsEmpty())
        {
          throw Exception(LoadStr(KEYGEN_NO_OUTPUT));
        }
        break;

      case ktSSH1Public:
      case ktSSH2PublicRFC4716:
      case ktSSH2PublicOpenSSH:
        throw Exception(LoadStr(KEYGEN_PUBLIC));

      case ktUnopenable:
        throw EOSExtException(FMTLOAD(KEY_TYPE_UNOPENABLE, (InputFileName)), Error);

      case ktOpenSSHAuto:
      default:
        DebugFail();
        // fallthru
      case ktUnknown:
        throw Exception(FMTLOAD(KEY_TYPE_UNKNOWN2, (InputFileName)));
    }

    UnicodeString Comment;
    if (IsKeyEncrypted(Type, InputFileName, Comment) &&
        Passphrase.IsEmpty())
    {
      Console->Print(StripHotkey(FMTLOAD(PROMPT_KEY_PASSPHRASE, (Comment))) + L" ");
      if (!Console->Input(Passphrase, false, 0) ||
          Passphrase.IsEmpty())
      {
        Abort();
      }
    }

    TPrivateKey * PrivateKey = LoadKey(Type, InputFileName, Passphrase);

    { ::rtl::TFinally Finally([&]()
    {
      FreeKey(PrivateKey);
    }); try
    {
      if (!NewComment.IsEmpty())
      {
        ChangeKeyComment(PrivateKey, NewComment);
      }

      if (!Certificate.IsEmpty())
      {
        AddCertificateToKey(PrivateKey, Certificate);
      }

      if (ChangePassphrase)
      {
        if (!NewPassphraseSet)
        {
          Console->Print(LoadStr(KEYGEN_PASSPHRASE) + L" ");
          if (!Console->Input(NewPassphrase, false, 0))
          {
            Abort();
          }

          Console->Print(LoadStr(KEYGEN_PASSPHRASE2) + L" ");
          UnicodeString NewPassphrase2;
          if (!Console->Input(NewPassphrase2, false, 0))
          {
            Abort();
          }

          if (NewPassphrase != NewPassphrase2)
          {
            Shred(NewPassphrase2);
            throw Exception(LoadStr(KEYGEN_PASSPHRASES_MISMATCH));
          }
        }
      }
      else
      {
        NewPassphrase = Passphrase;
      }

      if (OutputFileName.IsEmpty())
      {
        OutputFileName = InputFileName;
      }

      SaveKey(ktSSH2, OutputFileName, NewPassphrase, PrivateKey);

      Console->PrintLine(FMTLOAD(KEYGEN_SAVED, (OutputFileName)));
    }
    catch (...) { Finally.Unwind(); } }
  }
  catch (Exception & E)
  {
    Result = HandleException(Console, E);
  }

  Shred(Passphrase);
  Shred(NewPassphrase);

  Console->WaitBeforeExit();
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall FingerprintScan(TConsole * Console, TProgramParams * Params)
{
  int Result = RESULT_SUCCESS;
  try
  {
    CheckLogParam(Params);

    std::unique_ptr<TSessionData> SessionData;

    if (Params->ParamCount > 0)
    {
      UnicodeString SessionUrl = Params->Param[1];
      int ParsedInfo;
      SessionData.reset(StoredSessions->ParseUrl(SessionUrl, Params, ParsedInfo));
      if (FLAGSET(ParsedInfo, piDefaultsOnly) || !SessionData->CanLogin ||
          (!SessionData->UsesSsh && (SessionData->Ftps == ftpsNone)))
      {
        SessionData.reset(NULL);
      }
    }

    if (!SessionData)
    {
      throw Exception(LoadStr(FINGERPRINTSCAN_NEED_SECURE_SESSION));
    }

    std::unique_ptr<TTerminal> Terminal(new TTerminal(SessionData.get(), Configuration));
    UnicodeString SHA256;
    UnicodeString SHA1;
    UnicodeString MD5;
    Terminal->FingerprintScan(SHA256, SHA1, MD5);
    if (!SHA256.IsEmpty())
    {
      Console->PrintLine(FORMAT(L"SHA-256: %s", (SHA256)));
    }
    if (!SHA1.IsEmpty())
    {
      Console->PrintLine(FORMAT(L"SHA-1: %s", (SHA1)));
    }
    if (!MD5.IsEmpty())
    {
      Console->PrintLine(FORMAT(L"MD5:     %s", (MD5)));
    }
  }
  catch (Exception & E)
  {
    Result = HandleException(Console, E);
  }

  Console->WaitBeforeExit();
  return Result;
}
//---------------------------------------------------------------------------
// OpenSCP: no DumpCallstack (the callstack dumps of WinSCP.exe on Windows)
//---------------------------------------------------------------------------
void static PrintList(TConsole * Console, const UnicodeString & Caption, TStrings * List)
{
  Console->PrintLine(Caption);
  for (int Index = 0; Index < List->Count; Index++)
  {
    Console->PrintLine(List->Strings[Index]);
  }
  Console->PrintLine();
}
//---------------------------------------------------------------------------
void static PrintListAndFree(TConsole * Console, const UnicodeString & Caption, TStrings * List)
{
  std::unique_ptr<TStrings> Owner(List);
  PrintList(Console, Caption, List);
}
//---------------------------------------------------------------------------
int Info(TConsole * Console)
{
  int Result = RESULT_SUCCESS;
  try
  {
    Console->PrintLine(FORMAT(L"SSH implementation: PuTTY %s", (GetPuTTYVersion())));
    Console->PrintLine();
    PrintListAndFree(Console, L"SSH encryption ciphers:", SshCipherList());
    PrintListAndFree(Console, L"SSH key exchange algorithms:", SshKexList());
    PrintListAndFree(Console, L"SSH host key algorithms:", SshHostKeyList());
    PrintListAndFree(Console, L"SSH MAC algorithms:", SshMacList());

    Console->PrintLine(TSessionLog::GetSeparator());

    Console->PrintLine(FORMAT(L"TLS/SSL implementation: %s", (GetOpenSSLVersionText())));
    Console->PrintLine();
    PrintListAndFree(Console, L"TLS/SSL cipher suites:", TlsCipherList());
  }
  catch (Exception & E)
  {
    Result = HandleException(Console, E);
  }

  Console->WaitBeforeExit();
  return Result;
}
//---------------------------------------------------------------------------
TStdInOutMode ParseStdInOutMode(TProgramParams * Params, const UnicodeString & Switch, bool AllowChunked)
{
  TStdInOutMode Result;
  UnicodeString Value;
  if (!Params->FindSwitch(Switch, Value))
  {
    Result = TConsoleCommStruct::TInitEvent::OFF;
  }
  else
  {
    if (Value.IsEmpty() || SameText(Value, STDINOUT_BINARY_VALUE))
    {
      Result = TConsoleCommStruct::TInitEvent::BINARY;
    }
    else if (SameText(Value, STDINOUT_CHUNKED_VALUE) && AllowChunked)
    {
      Result = TConsoleCommStruct::TInitEvent::CHUNKED;
    }
    else
    {
      throw Exception(FORMAT(SCRIPT_VALUE_UNKNOWN, (Value, Switch))); // abuse of the string
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall Console(TConsoleMode Mode)
{
  DebugAssert(Mode != cmNone);
  TProgramParams * Params = TProgramParams::Instance();
  int Result = RESULT_SUCCESS;
  TConsole * Console = NULL;
  TConsoleRunner * Runner = NULL;
  TStrings * ScriptCommands = new TStringList();
  TStrings * ScriptParameters = new TStringList();
  { ::rtl::TFinally Finally([&]()
  {
    delete Runner;
    delete Console;
    delete ScriptCommands;
    delete ScriptParameters;
  }); try
  {
    // OpenSCP: the console of the terminal, which works as winscp.com
    Configuration->Usage->Inc(L"ConsoleExternal");
    TStdInOutMode StdOut = ParseStdInOutMode(Params, STDOUT_SWITCH, true);
    TStdInOutMode StdIn = ParseStdInOutMode(Params, STDIN_SWITCH, false);
    bool NoInteractiveInput = Params->FindSwitch(NOINTERACTIVEINPUT_SWITCH) || (StdIn != TConsoleCommStruct::TInitEvent::OFF);
    Console = CreateUnixConsole(NoInteractiveInput, StdOut, StdIn);

    SetNoGUI();

    if (Mode == cmHelp)
    {
      Configuration->Usage->Inc(L"UsageShown");
      Usage(Console);
    }
    else if (Mode == cmBatchSettings)
    {
      if (CheckSafe(Params))
      {
        Configuration->Usage->Inc(L"BatchSettings");
        Result = BatchSettings(Console, Params);
      }
    }
    else if (Mode == cmKeyGen)
    {
      if (CheckSafe(Params))
      {
        Configuration->Usage->Inc(L"KeyGen");
        Result = KeyGen(Console, Params);
      }
    }
    else if (Mode == cmFingerprintScan)
    {
      if (CheckSafe(Params))
      {
        Configuration->Usage->Inc(L"FingerprintScan");
        Result = FingerprintScan(Console, Params);
      }
    }
    else if (Mode == cmInfo)
    {
      Result = Info(Console);
    }
    else
    {
      Runner = new TConsoleRunner(Console);

      try
      {
        if (CheckSafe(Params))
        {
          UnicodeString Value;
          if (Mode == cmCopyId)
          {
            Configuration->Usage->Inc(L"CopyId");
            UnicodeString IdentityFileName;
            bool ValueSet;
            FindNixCompatibleSwitch(Params, IDENTITY_SWITCH, L"i", IdentityFileName, ValueSet);
            if (IdentityFileName.IsEmpty())
            {
              throw Exception(LoadStr(COPYID_IDETITY_MISSING));
            }
            ScriptCommands->Add(FORMAT(L"%s \"%s\"", (COPYID_COMMAND, IdentityFileName)));
            ScriptCommands->Add(EXIT_COMMAND);
          }
          else if (Params->FindSwitch(SCRIPT_SWITCH, Value) && !Value.IsEmpty())
          {
            Configuration->Usage->Inc(L"ScriptFile");
            LoadScriptFromFile(Value, ScriptCommands);
          }
          Params->FindSwitch(COMMAND_SWITCH, ScriptCommands);
          if (ScriptCommands->Count > 0)
          {
            Configuration->Usage->Inc(L"ScriptCommands");
          }
          Params->FindSwitch(L"parameter", ScriptParameters);
          if (ScriptParameters->Count > 0)
          {
            Configuration->Usage->Inc(L"ScriptParameters");
          }
        }

        UnicodeString Session;
        if (Params->ParamCount >= 1)
        {
          Session = Params->Param[1];
          if (Params->ParamCount > 1)
          {
            // Check if the pending parameters will be consumed by ParseUrl (/rawsettings) in TManagementScript::Connect.
            // This way we parse the options twice, but we do not want to refactor the code just for nicer test for this minor warning.
            TOptions OptionsCopy(*Params);
            int ParsedInfo;
            StoredSessions->ParseUrl(Session, &OptionsCopy, ParsedInfo);
            if (OptionsCopy.ParamCount > 1)
            {
              Runner->PrintMessage(LoadStr(SCRIPT_CMDLINE_PARAMETERS));
            }
          }
        }

        CheckLogParam(Params);
        CheckXmlLogParam(Params);

        bool UsageWarnings = (Mode != cmCopyId) && ((ScriptCommands->Count > 0) || !Console->HasFlag(cfInteractive));
        Result = Runner->Run(Session, Params, ScriptCommands, ScriptParameters, UsageWarnings);
      }
      catch(Exception & E)
      {
        Runner->ShowException(&E);
        Result = RESULT_ANY_ERROR;
      }

      if (Mode == cmCopyId)
      {
        Console->WaitBeforeExit();
      }
    }
  }
  catch (...) { Finally.Unwind(); } }

  return Result;
}
