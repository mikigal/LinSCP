// The console of openscp-cli on a Unix terminal or redirected streams: the
// counterpart of upstream's TExternalConsole (ConsoleRunner.cpp) together
// with winscp.com (console/Main.cpp), which does the actual input and
// output for it on Windows.
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include "CliInterface.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>

#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// Ctrl+C: set by the signal thread, consumed by the console (as winscp.com's
// auto-reset cancel event); the pipe wakes up a console waiting for input
std::atomic<bool> InterruptPending(false);
int InterruptPipe[2] = { -1, -1 };
//---------------------------------------------------------------------------
class TInterruptWaker
{
public:
  void Wake()
  {
    // Only wakes up the main thread waiting in CheckSynchronize
  }
};
TInterruptWaker InterruptWaker;
//---------------------------------------------------------------------------
void ClearInterruptPipe()
{
  char Buffer[64];
  while (read(InterruptPipe[0], Buffer, sizeof(Buffer)) > 0)
  {
  }
}
//---------------------------------------------------------------------------
bool ConsumeInterrupt()
{
  bool Result = InterruptPending.exchange(false);
  if (Result && (InterruptPipe[0] >= 0))
  {
    ClearInterruptPipe();
  }
  return Result;
}
//---------------------------------------------------------------------------
void WriteAll(int Fd, const char * Data, size_t Len)
{
  while (Len > 0)
  {
    ssize_t Written = write(Fd, Data, Len);
    if (Written < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      // Nowhere to report an error of the output
      break;
    }
    Data += Written;
    Len -= static_cast<size_t>(Written);
  }
}
//---------------------------------------------------------------------------
// Waits for the input or an interrupt; false on timeout (Timeout < 0: none)
bool WaitForInput(int Fd, int Timeout, bool & Interrupted)
{
  pollfd Fds[2];
  Fds[0].fd = Fd;
  Fds[0].events = POLLIN;
  Fds[1].fd = InterruptPipe[0];
  Fds[1].events = POLLIN;
  int Count = (InterruptPipe[0] >= 0) ? 2 : 1;
  int Result;
  do
  {
    Result = poll(Fds, Count, Timeout);
  }
  while ((Result < 0) && (errno == EINTR));
  Interrupted = ConsumeInterrupt();
  return Interrupted || (Result > 0);
}
//---------------------------------------------------------------------------
class TTerminalMode
{
public:
  TTerminalMode(int Fd) :
    FFd(Fd)
  {
    FValid = (tcgetattr(FFd, &FPrevious) == 0);
  }

  ~TTerminalMode()
  {
    if (FValid)
    {
      tcsetattr(FFd, TCSANOW, &FPrevious);
    }
  }

  void Set(tcflag_t Clear)
  {
    if (FValid)
    {
      termios Mode = FPrevious;
      Mode.c_lflag &= ~Clear;
      Mode.c_cc[VMIN] = 1;
      Mode.c_cc[VTIME] = 0;
      tcsetattr(FFd, TCSANOW, &Mode);
    }
  }

private:
  int FFd;
  bool FValid;
  termios FPrevious;
};
//---------------------------------------------------------------------------
class TUnixConsole : public TConsole
{
public:
  TUnixConsole(bool NoInteractiveInput, TStdInOutMode StdOut, TStdInOutMode StdIn);
  virtual ~TUnixConsole();

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

private:
  int FInput;
  int FOutput;
  // Input from a terminal
  bool FInteractive;
  // Output to a terminal
  bool FLiveOutput;
  bool FNoInteractiveInput;
  TStdInOutMode FStdOut;
  TStdInOutMode FStdIn;
  // A progress line not written to a redirected output yet
  UnicodeString FLastFromBeginning;
  // FTP transfers stdout/stdin data from another thread
  std::recursive_mutex FSection;

  void Write(const UnicodeString & Str);
  bool ReadLine(std::string & Line, int Timeout, bool & Interrupted);
};
//---------------------------------------------------------------------------
TUnixConsole::TUnixConsole(bool NoInteractiveInput, TStdInOutMode StdOut, TStdInOutMode StdIn)
{
  FInput = STDIN_FILENO;
  // With the standard output used for data, the messages go to the error output
  FOutput = (StdOut != TConsoleCommStruct::TInitEvent::OFF) ? STDERR_FILENO : STDOUT_FILENO;
  FInteractive = (isatty(FInput) != 0);
  FLiveOutput = (isatty(FOutput) != 0);
  FNoInteractiveInput = NoInteractiveInput;
  FStdOut = StdOut;
  FStdIn = StdIn;
}
//---------------------------------------------------------------------------
TUnixConsole::~TUnixConsole()
{
  // The pending progress line
  if (!FLastFromBeginning.IsEmpty())
  {
    Write(FLastFromBeginning);
  }
}
//---------------------------------------------------------------------------
void TUnixConsole::Write(const UnicodeString & Str)
{
  UTF8String Bytes(Str);
  WriteAll(FOutput, Bytes.c_str(), Bytes.Length());
}
//---------------------------------------------------------------------------
void __fastcall TUnixConsole::Print(UnicodeString Str, bool FromBeginning, bool /*Error*/)
{
  std::lock_guard<std::recursive_mutex> Guard(FSection);
  if (!FLiveOutput)
  {
    // A line printed from the beginning (progress) replaces the previous one,
    // so it is written only once another line follows
    if (FromBeginning && !StartsStr(L"\n", Str))
    {
      FLastFromBeginning = Str;
    }
    else
    {
      if (!FLastFromBeginning.IsEmpty())
      {
        Write(FLastFromBeginning);
        FLastFromBeginning = UnicodeString();
      }

      if (FromBeginning)
      {
        Write(L"\n");
        FLastFromBeginning = Str.SubString(2, Str.Length() - 1);
      }
      else
      {
        Write(Str);
      }
    }
  }
  else
  {
    if (FromBeginning)
    {
      Write(L"\r");
    }
    Write(Str);
  }
}
//---------------------------------------------------------------------------
bool TUnixConsole::ReadLine(std::string & Line, int Timeout, bool & Interrupted)
{
  // Byte by byte, not to read ahead of the line
  Line.clear();
  Interrupted = false;
  bool Result = false;
  while (true)
  {
    if (FInteractive && !WaitForInput(FInput, Timeout, Interrupted))
    {
      break;
    }
    if (Interrupted)
    {
      break;
    }
    char Ch;
    ssize_t Read = read(FInput, &Ch, 1);
    if ((Read < 0) && (errno == EINTR))
    {
      continue;
    }
    if (Read <= 0)
    {
      // Unless anything was read, the end of the input fails it
      Result = !Line.empty();
      break;
    }
    if (Ch == '\n')
    {
      Result = true;
      break;
    }
    if (Ch != '\r')
    {
      Line += Ch;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TUnixConsole::Input(UnicodeString & Str, bool Echo, unsigned int Timer)
{
  std::lock_guard<std::recursive_mutex> Guard(FSection);
  bool Result;
  std::string Line;
  bool Interrupted;
  if (!FInteractive)
  {
    Result = ReadLine(Line, -1, Interrupted);
    Str = UTF8ToString(Line.c_str());
    // As if typed
    Print(Str);
    Print(L"\n");
  }
  else
  {
    if (!FLastFromBeginning.IsEmpty())
    {
      Print(UnicodeString());
    }

    TTerminalMode Mode(FInput);
    if (!Echo)
    {
      Mode.Set(ECHO);
    }

    Result = ReadLine(Line, (Timer > 0) ? static_cast<int>(Timer) : -1, Interrupted);
    Str = UTF8ToString(Line.c_str());

    if (Interrupted || !Echo)
    {
      Write(L"\n");
    }
    if (Interrupted)
    {
      Result = false;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
int __fastcall TUnixConsole::Choice(
  UnicodeString Options, int Cancel, int Break, int /*Continue*/, int Timeouted, bool Timeouting, unsigned int Timer,
  UnicodeString /*Message*/)
{
  std::lock_guard<std::recursive_mutex> Guard(FSection);
  int Result = 0;
  if (!FInteractive)
  {
    if (Timeouting)
    {
      Sleep(Timer);
      Result = Timeouted;
    }
    else
    {
      Result = Break;
    }
  }
  else
  {
    TTerminalMode Mode(FInput);
    Mode.Set(ICANON | ECHO);

    auto End = std::chrono::steady_clock::now() + std::chrono::milliseconds(Timer);
    while (Result == 0)
    {
      int Timeout = -1;
      if (Timer > 0)
      {
        auto Remaining =
          std::chrono::duration_cast<std::chrono::milliseconds>(End - std::chrono::steady_clock::now()).count();
        Timeout = static_cast<int>(std::max<long long>(Remaining, 0));
      }
      bool Interrupted;
      if (!WaitForInput(FInput, Timeout, Interrupted))
      {
        Result = Timeouted;
      }
      else if (Interrupted)
      {
        Result = Break;
      }
      else
      {
        char Buffer[64];
        ssize_t Read = read(FInput, Buffer, sizeof(Buffer));
        if ((Read < 0) && (errno == EINTR))
        {
          continue;
        }
        if (Read <= 0)
        {
          // End of the input
          Result = Break;
        }
        else if (Buffer[0] == '\x1B')
        {
          // A sequence of a special key, or Alt with a key, is ignored
          if (Read == 1)
          {
            Result = Cancel;
          }
        }
        else
        {
          UnicodeString Key = UTF8ToString(std::string(Buffer, static_cast<size_t>(Read)).c_str());
          if (Key.Length() == 1)
          {
            int Index = Options.Pos(AnsiUpperCase(Key)[1]);
            if (Index > 0)
            {
              Result = Index;
            }
          }
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TUnixConsole::HasFlag(TConsoleFlag Flag) const
{
  switch (Flag)
  {
    case cfLimitedOutput:
    case cfLiveOutput:
      return FLiveOutput;

    case cfNoInteractiveInput:
      return FNoInteractiveInput;

    case cfInteractive:
      return FInteractive;

    case cfCommandLineOnly:
      return true;

    case cfWantsProgress:
      return false;

    case cfStdOut:
      return (FStdOut != TConsoleCommStruct::TInitEvent::OFF);

    case cfStdIn:
      return (FStdIn != TConsoleCommStruct::TInitEvent::OFF);

    default:
      DebugFail();
      return false;
  }
}
//---------------------------------------------------------------------------
bool __fastcall TUnixConsole::PendingAbort()
{
  return ConsumeInterrupt();
}
//---------------------------------------------------------------------------
void __fastcall TUnixConsole::SetTitle(UnicodeString /*Title*/)
{
  // The title of a terminal cannot be restored reliably, so it is left alone
}
//---------------------------------------------------------------------------
void __fastcall TUnixConsole::WaitBeforeExit()
{
  // noop
}
//---------------------------------------------------------------------------
void __fastcall TUnixConsole::Progress(TScriptProgress & /*Progress*/)
{
  // Only for the .NET assembly (cfWantsProgress)
  DebugFail();
}
//---------------------------------------------------------------------------
void __fastcall TUnixConsole::TransferOut(const unsigned char * Data, size_t Len)
{
  std::lock_guard<std::recursive_mutex> Guard(FSection);
  DebugAssert((Data == NULL) == (Len == 0));
  if (FStdOut == TConsoleCommStruct::TInitEvent::BINARY)
  {
    WriteAll(STDOUT_FILENO, reinterpret_cast<const char *>(Data), Len);
  }
  else if (FStdOut == TConsoleCommStruct::TInitEvent::CHUNKED)
  {
    AnsiString Header = AnsiString(IntToHex(static_cast<__int64>(Len), 1).LowerCase()) + "\r\n";
    WriteAll(STDOUT_FILENO, Header.c_str(), Header.Length());
    WriteAll(STDOUT_FILENO, reinterpret_cast<const char *>(Data), Len);
    WriteAll(STDOUT_FILENO, "\r\n", 2);
  }
}
//---------------------------------------------------------------------------
size_t __fastcall TUnixConsole::TransferIn(unsigned char * Data, size_t Len)
{
  std::lock_guard<std::recursive_mutex> Guard(FSection);
  size_t Result = 0;
  while (Result < Len)
  {
    ssize_t Read = read(STDIN_FILENO, Data + Result, Len - Result);
    if (Read < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      throw Exception(LoadStr(STREAM_READ_ERROR));
    }
    if (Read == 0)
    {
      break;
    }
    Result += static_cast<size_t>(Read);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TUnixConsole::FinalLogMessage()
{
  return UnicodeString();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TConsole * CreateUnixConsole(bool NoInteractiveInput, TStdInOutMode StdOut, TStdInOutMode StdIn)
{
  return new TUnixConsole(NoInteractiveInput, StdOut, StdIn);
}
//---------------------------------------------------------------------------
void InitializeConsoleSignals()
{
  // Writes to closed sockets and pipes fail with EPIPE instead
  signal(SIGPIPE, SIG_IGN);

  if (pipe2(InterruptPipe, O_CLOEXEC | O_NONBLOCK) != 0)
  {
    InterruptPipe[0] = InterruptPipe[1] = -1;
  }

  // SIGINT is blocked in all threads (they inherit the mask) and waited for
  // by a thread of its own, so it interrupts no system call
  sigset_t Signals;
  sigemptyset(&Signals);
  sigaddset(&Signals, SIGINT);
  pthread_sigmask(SIG_BLOCK, &Signals, NULL);
  std::thread([Signals]()
    {
      while (true)
      {
        int Signal;
        if (sigwait(&Signals, &Signal) == 0)
        {
          InterruptPending = true;
          if (InterruptPipe[1] >= 0)
          {
            char Ch = 0;
            WriteAll(InterruptPipe[1], &Ch, 1);
          }
          TThread::Queue(NULL, ::rtl::MethodPtr(&InterruptWaker, &TInterruptWaker::Wake));
        }
      }
    }).detach();
}
//---------------------------------------------------------------------------
