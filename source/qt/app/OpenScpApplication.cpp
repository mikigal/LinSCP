// The application object of the Qt front end.
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "QtInterface.h"

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
    ShowExtendedException(&E);
  }
  catch (std::exception & E)
  {
    std::unique_ptr<Exception> Error(new Exception(UnicodeString(E.what())));
    ShowExtendedException(Error.get());
  }
  return false;
}
//---------------------------------------------------------------------------
