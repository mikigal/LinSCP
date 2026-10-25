// Local programs executed by the custom commands (upstream's
// ExecuteShell*/ExecuteProcess* of windows/GUITools.cpp and Tools.cpp). The
// commands are run by /bin/sh, as typed in a terminal; while waiting for
// them, the events are processed.
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"

#include <CoreMain.h>

#include <QCoreApplication>
#include <QProcess>
//---------------------------------------------------------------------------
static const QString Shell(QStringLiteral("/bin/sh"));
//---------------------------------------------------------------------------
static QStringList ShellArguments(const UnicodeString & Command)
{
  return { QStringLiteral("-c"), ToQString(Command) };
}
//---------------------------------------------------------------------------
static void WaitForProcess(QProcess & Process, std::function<void()> OnRead = nullptr)
{
  // Same as in upstream's ExecuteProcessAndReadOutput
  while (!Process.waitForFinished(50))
  {
    if (Process.state() == QProcess::NotRunning)
    {
      break;
    }
    if (OnRead)
    {
      OnRead();
    }
    QCoreApplication::processEvents();
  }
  if (OnRead)
  {
    OnRead();
  }
}
//---------------------------------------------------------------------------
void ExecuteShellChecked(const UnicodeString & Command)
{
  AppLogFmt(L"Executing command: %s", (Command));
  if (!QProcess::startDetached(Shell, ShellArguments(Command)))
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, (Command)));
  }
}
//---------------------------------------------------------------------------
void ExecuteShellCheckedAndWait(const UnicodeString & Command)
{
  AppLogFmt(L"Executing command: %s", (Command));
  QProcess Process;
  // The output of the program goes where ours does
  Process.setProcessChannelMode(QProcess::ForwardedChannels);
  Process.start(Shell, ShellArguments(Command));
  if (!Process.waitForStarted())
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, (Command)));
  }
  Process.closeWriteChannel();
  WaitForProcess(Process);
}
//---------------------------------------------------------------------------
void ExecuteProcessAndReadOutput(const UnicodeString & Command, UnicodeString & Output, int & ExitCode, bool ReadStdErr)
{
  AppLogFmt(L"Executing command: %s", (Command));
  QProcess Process;
  if (ReadStdErr)
  {
    Process.setProcessChannelMode(QProcess::MergedChannels);
  }
  else
  {
    Process.setStandardErrorFile(QProcess::nullDevice());
  }
  Process.start(Shell, ShellArguments(Command));
  if (!Process.waitForStarted())
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, (Command)));
  }
  Process.closeWriteChannel();
  QByteArray Buffer;
  WaitForProcess(Process, [&]() { Buffer += Process.readAllStandardOutput(); });
  Output += UnicodeString(UTF8String(Buffer.constData(), Buffer.size()));
  ExitCode = (Process.exitStatus() == QProcess::NormalExit) ? Process.exitCode() : -1;
}
//---------------------------------------------------------------------------
static void DoExecuteProcessAndReadOutput(const UnicodeString & Command, UnicodeString & Output)
{
  int ExitCode;
  ExecuteProcessAndReadOutput(Command, Output, ExitCode, true);
  if (ExitCode != 0)
  {
    UnicodeString Buf = Output;
    UnicodeString Buf2;
    if (ExtractMainInstructions(Buf, Buf2))
    {
      throw ExtException(Output, EmptyStr);
    }
    else
    {
      UnicodeString Message = MainInstructions(FMTLOAD(COMMAND_FAILED_CODEONLY, (ExitCode)));
      throw ExtException(Message, Output);
    }
  }
}
//---------------------------------------------------------------------------
void ExecuteProcessChecked(const UnicodeString & Command, UnicodeString * Output)
{
  if (Output == NULL)
  {
    ExecuteShellChecked(Command);
  }
  else
  {
    DoExecuteProcessAndReadOutput(Command, *Output);
  }
}
//---------------------------------------------------------------------------
void ExecuteProcessCheckedAndWait(const UnicodeString & Command, UnicodeString * Output)
{
  if (Output == NULL)
  {
    ExecuteShellCheckedAndWait(Command);
  }
  else
  {
    DoExecuteProcessAndReadOutput(Command, *Output);
  }
}
//---------------------------------------------------------------------------
