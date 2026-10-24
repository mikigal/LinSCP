// The parts of the core's front end interface (core/Interface.h) specific
// to the Qt front end (upstream has them in WinInterface.cpp,
// UserInterface.cpp and Tools.cpp).
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
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
void ShowExtendedExceptionEx(TTerminal * /*Terminal*/, Exception * E)
{
  UnicodeString Message;
  if (ShouldDisplayException(E) && ExceptionFullMessage(E, Message))
  {
    QMessageBox::critical(
      QApplication::activeWindow(), ToQString(AppName), ToQString(UnformatMessage(Message)));
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
