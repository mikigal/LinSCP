// The application object of the Qt front end.
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"
#include "TerminalManager.h"

#include "LinScpApplication.h"

#include <QIcon>
//---------------------------------------------------------------------------
TLinScpApplication::TLinScpApplication(int & argc, char ** argv) :
  QApplication(argc, argv)
{
  setApplicationName(QStringLiteral("linscp"));
  // Matches the .desktop file, so that the compositor finds the icon
  setDesktopFileName(QStringLiteral("linscp"));
  // The icon of the .desktop file, for the platforms that take it from the
  // application (again once the icons are installed)
  setWindowIcon(ApplicationIcon());
  // The captions of the windows are formatted by the windows themselves
  // (FormatMainFormCaption), not suffixed by Qt
  setQuitOnLastWindowClosed(true);
  ::WakeMainThread = ::rtl::MethodPtr(this, &TLinScpApplication::WakeMainThread);
}
//---------------------------------------------------------------------------
TLinScpApplication::~TLinScpApplication()
{
  ::WakeMainThread = NULL;
}
//---------------------------------------------------------------------------
void TLinScpApplication::WakeMainThread(TObject * /*Sender*/)
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
bool TLinScpApplication::notify(QObject * Receiver, QEvent * Event)
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
