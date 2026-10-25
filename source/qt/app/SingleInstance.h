// A session of the command line opened by a running instance of the GUI
// (upstream's SendToAnotherInstance with its WM_COPYDATA messages,
// windows/WinMain.cpp and forms/CustomScpExplorer.cpp), over D-Bus: the
// running instances queue for the name of the service, the first one
// serves it.
//---------------------------------------------------------------------------
#ifndef SingleInstanceH
#define SingleInstanceH
//---------------------------------------------------------------------------
#include <QObject>
#include <QStringList>

#include <functional>
//---------------------------------------------------------------------------
class TInstanceService : public QObject
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.openscp.OpenSCP")

public:
  explicit TInstanceService(QObject * Parent);
  virtual ~TInstanceService();

  // On the session bus, the service is served once the instances before
  // this one end; false when there is no bus
  bool Register();

  // Takes the arguments of the command line of another instance (and the
  // token to activate its window with); false when it cannot open them
  // now. Called before the reply: the session is to be opened later.
  std::function<bool(const QStringList & Arguments, const QString & ActivationToken)> OnCommandLine;

public Q_SLOTS:
  Q_SCRIPTABLE bool CommandLine(const QStringList & Arguments, const QString & ActivationToken);

private:
  bool FRegistered;
};
//---------------------------------------------------------------------------
// Passes the arguments to a running instance; true when it took them
bool SendToAnotherInstance(const QStringList & Arguments);
//---------------------------------------------------------------------------
#endif
