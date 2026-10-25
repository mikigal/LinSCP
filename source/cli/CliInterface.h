// Declarations of linscp-cli: the console interface of the ported
// ConsoleRunner (upstream has it in WinInterface.h and console/Console.h)
// and the console of a Unix terminal or redirected streams.
//---------------------------------------------------------------------------
#ifndef CliInterfaceH
#define CliInterfaceH
//---------------------------------------------------------------------------
#include <Script.h>
//---------------------------------------------------------------------------
enum TConsoleFlag
{
  cfLimitedOutput,
  cfLiveOutput,
  cfNoInteractiveInput,
  cfInteractive,
  cfCommandLineOnly,
  cfWantsProgress,
  cfStdOut,
  cfStdIn
};
//---------------------------------------------------------------------------
class TConsole
{
public:
  virtual __fastcall ~TConsole() {}
  virtual void __fastcall Print(UnicodeString Str, bool FromBeginning = false, bool Error = false) = 0;
  void __fastcall PrintLine(const UnicodeString & Str = UnicodeString(), bool Error = false);
  virtual bool __fastcall Input(UnicodeString & Str, bool Echo, unsigned int Timer) = 0;
  virtual int __fastcall Choice(
    UnicodeString Options, int Cancel, int Break, int Continue, int Timeouted, bool Timeouting, unsigned int Timer,
    UnicodeString Message) = 0;
  virtual bool __fastcall HasFlag(TConsoleFlag Flag) const = 0;
  virtual bool __fastcall PendingAbort() = 0;
  virtual void __fastcall SetTitle(UnicodeString Title) = 0;
  virtual void __fastcall WaitBeforeExit() = 0;
  virtual void __fastcall Progress(TScriptProgress & Progress) = 0;
  virtual void __fastcall TransferOut(const unsigned char * Data, size_t Len) = 0;
  virtual size_t __fastcall TransferIn(unsigned char * Data, size_t Len) = 0;
  virtual UnicodeString __fastcall FinalLogMessage() = 0;
};
//---------------------------------------------------------------------------
int __fastcall HandleException(TConsole * Console, Exception & E);
//---------------------------------------------------------------------------
enum { RESULT_SUCCESS = 0, RESULT_ANY_ERROR = 1 };
//---------------------------------------------------------------------------
enum TConsoleMode
{
  cmNone, cmScripting, cmHelp, cmBatchSettings, cmKeyGen, cmFingerprintScan, cmInfo, cmCopyId,
};
int __fastcall Console(TConsoleMode Mode);
//---------------------------------------------------------------------------
// The formats of the /stdout and /stdin streams, named as in the protocol
// of upstream's console (console/Console.h)
struct TConsoleCommStruct
{
  struct TInitEvent
  {
    enum STDINOUT { OFF, BINARY, CHUNKED };
  };
};
typedef TConsoleCommStruct::TInitEvent::STDINOUT TStdInOutMode;
//---------------------------------------------------------------------------
// The console of linscp-cli, which works as upstream's winscp.com: on a
// terminal, it is interactive; with redirected input, the input is read
// line by line (and echoed) and choices are not possible; with redirected
// output, the output is not rewritten. With /stdout, the messages go to the
// standard error output.
TConsole * CreateUnixConsole(bool NoInteractiveInput, TStdInOutMode StdOut, TStdInOutMode StdIn);
// Handles SIGINT (Ctrl+C) in a thread of its own, so that no other thread
// is interrupted; to be called before any thread is started
void InitializeConsoleSignals();
//---------------------------------------------------------------------------
#endif
