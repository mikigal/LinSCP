// The application object of the Qt front end.
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"
#include "TerminalManager.h"

#include "OpenScpApplication.h"
//---------------------------------------------------------------------------
TOpenScpApplication::TOpenScpApplication(int & argc, char ** argv) :
  QApplication(argc, argv)
{
  setApplicationName(QStringLiteral("openscp"));
  // Matches the .desktop file, so that the compositor finds the icon
  setDesktopFileName(QStringLiteral("openscp"));
  // The captions of the windows are formatted by the windows themselves
  // (FormatMainFormCaption), not suffixed by Qt
  setQuitOnLastWindowClosed(true);
  ::WakeMainThread = ::rtl::MethodPtr(this, &TOpenScpApplication::WakeMainThread);
}
//---------------------------------------------------------------------------
TOpenScpApplication::~TOpenScpApplication()
{
  ::WakeMainThread = NULL;
}
//---------------------------------------------------------------------------
void TOpenScpApplication::WakeMainThread(TObject * /*Sender*/)
{
  // Called by any thread
  QMetaObject::invokeMethod(this, []() { CheckSynchronize(); }, Qt::QueuedConnection);
}
//---------------------------------------------------------------------------
void HandleApplicationException(Exception & E)
{
  TTerminalManager * Manager = TTerminalManager::Instance(false);
  if (Manager != NULL)
  {
    Manager->HandleException(&E);
  }
  else
  {
    ShowExtendedException(&E);
  }
}
//---------------------------------------------------------------------------
void ExecuteHandler(const std::function<void()> & Handler)
{
  try
  {
    Handler();
  }
  catch (Exception & E)
  {
    HandleApplicationException(E);
  }
}
//---------------------------------------------------------------------------
bool TOpenScpApplication::notify(QObject * Receiver, QEvent * Event)
{
  try
  {
    return QApplication::notify(Receiver, Event);
  }
  catch (Exception & E)
  {
    HandleApplicationException(E);
  }
  catch (std::exception & E)
  {
    std::unique_ptr<Exception> Error(new Exception(UnicodeString(E.what())));
    ShowExtendedException(Error.get());
  }
  return false;
}
//---------------------------------------------------------------------------
