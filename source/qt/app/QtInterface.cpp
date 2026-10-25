// The parts of the core's front end interface (core/Interface.h) specific
// to the Qt front end (upstream has them in WinInterface.cpp,
// UserInterface.cpp and Tools.cpp).
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"
#include "ComputerPower.h"
#include "ScpCommander.h"
#include "TerminalManager.h"

#include <AppConfiguration.h>

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

#include <chrono>
//---------------------------------------------------------------------------
static bool NoGUI = false;
static std::chrono::steady_clock::time_point LastGUIUpdate;
//---------------------------------------------------------------------------
void __fastcall ShowExtendedException(Exception * E)
{
  ShowExtendedExceptionEx(NULL, E);
}
//---------------------------------------------------------------------------
void ShowExtendedExceptionEx(TTerminal * Terminal, Exception * E)
{
  bool Show = ShouldDisplayException(E);
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  // A fatal error of the session (a lost or failed connection)
  bool ForActiveTerminal =
    (dynamic_cast<EFatal *>(E) != NULL) && (Manager != NULL) && Manager->IsActiveTerminal(Terminal);
  // The session was closed once an operation completed
  ESshTerminate * Terminate = dynamic_cast<ESshTerminate *>(E);

  if (Terminate != NULL)
  {
    if (ForActiveTerminal)
    {
      DebugAssert(!Terminal->Active);
      // Upstream keeps the session disconnected in its tab; there are no
      // tabs yet
      Manager->FreeActiveTerminal();
    }

    if (Terminate->Operation == odoSuspend)
    {
      // suspend, so that exit prompt is shown only after the computer resumes
      try
      {
        SuspendComputer();
      }
      catch (Exception & SuspendE)
      {
        ExceptionMessageDialog(&SuspendE, qtError);
      }
    }

    unsigned int Result = qaNo;
    bool ConfirmExitOnCompletion =
      ((Terminate->Operation == odoDisconnect) || (Terminate->Operation == odoSuspend)) &&
      AppConfiguration->ConfirmExitOnCompletion;
    if (ConfirmExitOnCompletion)
    {
      TMessageParams Params(mpNeverAskAgainCheck);
      if (ForActiveTerminal)
      {
        // Remove the leading "%s\n\n" (not to change the translation originals - previously the error message was prepended)
        UnicodeString MessageFormat = FORMAT(LoadStr(EXIT_ON_COMPLETION), (UnicodeString())).Trim();
        MessageFormat = MainInstructions(MessageFormat) + L"\n\n%s";
        // Upstream offers to reconnect the session kept disconnected in
        // its tab; it is freed here
        Result = ExceptionMessageDialog(E, qtInformation, MessageFormat, qaYes | qaNo, &Params);
      }
      else
      {
        Result = ExceptionMessageDialog(E, qtInformation, L"", qaOK, &Params);
      }

      if (Result == qaNeverAskAgain)
      {
        Result = qaYes;
        AppConfiguration->ConfirmExitOnCompletion = false;
      }
    }
    else if (Terminate->Operation != odoIdle)
    {
      Result = qaYes;
    }

    if ((Result == qaYes) && ForActiveTerminal && (Manager->ScpExplorer != NULL))
    {
      // TerminateApplication
      QMetaObject::invokeMethod(Manager->ScpExplorer, "close", Qt::QueuedConnection);
    }
    if ((Result == qaYes) && (Terminate->Operation == odoShutDown))
    {
      try
      {
        ShutDownComputer();
      }
      catch (Exception & ShutDownE)
      {
        ExceptionMessageDialog(&ShutDownE, qtError);
      }
    }
  }
  else if (ForActiveTerminal)
  {
    unsigned int Answer = qaOK;
    if (Show)
    {
      // Upstream offers a few more choices (a timeout of the automatic
      // reconnect, reconnecting always, closing the session silently),
      // which are configured in the preferences
      Answer = FatalExceptionMessageDialog(E, qtError, EmptyStr, qaOK, NULL);
    }
    Manager->FatalExceptionAnswered(Answer);
  }
  else if (Show)
  {
    ExceptionMessageDialog(E, qtError);
  }
}
//---------------------------------------------------------------------------
void * __fastcall BusyStart()
{
  if (IsMainThread() && !NoGUI)
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    return reinterpret_cast<void *>(1);
  }
  return NULL;
}
//---------------------------------------------------------------------------
void __fastcall BusyEnd(void * Token)
{
  if (Token != NULL)
  {
    QApplication::restoreOverrideCursor();
  }
}
//---------------------------------------------------------------------------
void __fastcall SetNoGUI()
{
  NoGUI = true;
}
//---------------------------------------------------------------------------
bool __fastcall ProcessGUI(bool Force)
{
  bool Result = false;
  if (IsMainThread() && !NoGUI)
  {
    std::chrono::steady_clock::time_point Now = std::chrono::steady_clock::now();
    if (Force || (Now - LastGUIUpdate > std::chrono::milliseconds(GUIUpdateInterval)))
    {
      LastGUIUpdate = Now;
      QCoreApplication::processEvents();
      Result = true;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void SystemRequired()
{
  // Upstream keeps the system from sleeping during transfers
  // (SetThreadExecutionState); the counterpart on Linux is the inhibit
  // portal, which holds an inhibition until it is released, rather than a
  // periodic call
}
//---------------------------------------------------------------------------
void __fastcall CopyToClipboard(UnicodeString Text)
{
  QGuiApplication::clipboard()->setText(ToQString(Text));
}
//---------------------------------------------------------------------------
bool __fastcall TextFromClipboard(UnicodeString & Text, bool Trim)
{
  QClipboard * Clipboard = QGuiApplication::clipboard();
  bool Result = Clipboard->mimeData()->hasText();
  if (Result)
  {
    // As upstream: large enough for all uses (URLs, keys, fingerprints,
    // permissions)
    const int Limit = 64 * 1024;
    Text = ToUnicodeString(Clipboard->text().left(Limit));
    if (Trim)
    {
      Text = Text.Trim();
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void MinimizeApplication(bool ToTray)
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  if ((Manager != NULL) && (Manager->ScpExplorer != NULL))
  {
    Manager->ScpExplorer->MinimizeApplication(ToTray);
  }
}
//---------------------------------------------------------------------------
void RestoreApplication()
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  if ((Manager != NULL) && (Manager->ScpExplorer != NULL))
  {
    Manager->ScpExplorer->RestoreApplication();
  }
}
//---------------------------------------------------------------------------
bool IsSystemTrayAvailable()
{
  return TScpCommanderForm::IsSystemTrayAvailable();
}
//---------------------------------------------------------------------------
