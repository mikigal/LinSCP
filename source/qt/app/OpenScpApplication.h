// The application object of the Qt front end.
//---------------------------------------------------------------------------
#ifndef OpenScpApplicationH
#define OpenScpApplicationH
//---------------------------------------------------------------------------
#include <QApplication>
//---------------------------------------------------------------------------
class TOpenScpApplication : public QApplication
{
  Q_OBJECT

public:
  TOpenScpApplication(int & argc, char ** argv);

  // Exceptions that escape an event handler are shown to the user, as the
  // VCL does (Application->OnException); Qt does not support exceptions
  // that propagate through its event loop
  virtual bool notify(QObject * Receiver, QEvent * Event) override;
};
//---------------------------------------------------------------------------
#endif
