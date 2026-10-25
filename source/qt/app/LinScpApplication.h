// The application object of the Qt front end.
//---------------------------------------------------------------------------
#ifndef LinScpApplicationH
#define LinScpApplicationH
//---------------------------------------------------------------------------
#include <Classes.hpp>

#include <QApplication>
//---------------------------------------------------------------------------
class TLinScpApplication : public QApplication
{
  Q_OBJECT

public:
  TLinScpApplication(int & argc, char ** argv);
  virtual ~TLinScpApplication();

  // Exceptions that escape an event handler are shown to the user, as the
  // VCL does (Application->OnException); Qt does not support exceptions
  // that propagate through its event loop
  virtual bool notify(QObject * Receiver, QEvent * Event) override;

private:
  // The requests of the threads to the main thread (TThread::Synchronize,
  // e.g. of the monitoring of local directories) are run from the event
  // loop, as the VCL does
  void WakeMainThread(TObject * Sender);
};
//---------------------------------------------------------------------------
#endif
