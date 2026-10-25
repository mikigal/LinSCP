// A session of the command line opened by a running instance of the GUI.
//---------------------------------------------------------------------------
#include "SingleInstance.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
//---------------------------------------------------------------------------
static const QString ServiceName = QStringLiteral("org.linscp.LinSCP");
static const QString ObjectPath = QStringLiteral("/org/linscp/LinSCP");
static const QString InterfaceName = QStringLiteral("org.linscp.LinSCP");
// Opening a session may ask the user (a host key), the reply comes first
static const int CallTimeout = 10000;
//---------------------------------------------------------------------------
TInstanceService::TInstanceService(QObject * Parent) :
  QObject(Parent),
  FRegistered(false),
  FOnlyInstance(true)
{
}
//---------------------------------------------------------------------------
TInstanceService::~TInstanceService()
{
  if (FRegistered)
  {
    QDBusConnection Bus = QDBusConnection::sessionBus();
    Bus.unregisterObject(ObjectPath);
    Bus.interface()->unregisterService(ServiceName);
  }
}
//---------------------------------------------------------------------------
bool TInstanceService::Register()
{
  QDBusConnection Bus = QDBusConnection::sessionBus();
  if (Bus.isConnected() && Bus.registerObject(ObjectPath, this, QDBusConnection::ExportScriptableSlots))
  {
    // Queued behind the running instances
    QDBusReply<QDBusConnectionInterface::RegisterServiceReply> Reply =
      Bus.interface()->registerService(
        ServiceName, QDBusConnectionInterface::QueueService, QDBusConnectionInterface::DontAllowReplacement);
    FRegistered = Reply.isValid();
    FOnlyInstance = !FRegistered || (Reply.value() == QDBusConnectionInterface::ServiceRegistered);
    if (!FRegistered)
    {
      Bus.unregisterObject(ObjectPath);
    }
  }
  return FRegistered;
}
//---------------------------------------------------------------------------
bool TInstanceService::CommandLine(const QStringList & Arguments, const QString & ActivationToken)
{
  return OnCommandLine && OnCommandLine(Arguments, ActivationToken);
}
//---------------------------------------------------------------------------
bool SendToAnotherInstance(const QStringList & Arguments)
{
  bool Result = false;
  QDBusConnection Bus = QDBusConnection::sessionBus();
  if (Bus.isConnected() && Bus.interface()->isServiceRegistered(ServiceName))
  {
    QDBusMessage Message = QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, QStringLiteral("CommandLine"));
    // Lets the other instance activate its window (Wayland's focus
    // stealing prevention), when a launcher gave us a token
    Message << Arguments << qEnvironmentVariable("XDG_ACTIVATION_TOKEN");
    QDBusReply<bool> Reply = Bus.call(Message, QDBus::Block, CallTimeout);
    Result = Reply.isValid() && Reply.value();
  }
  return Result;
}
//---------------------------------------------------------------------------
bool AnyOtherInstanceOfSelf()
{
  bool Result = false;
  QDBusConnection Bus = QDBusConnection::sessionBus();
  if (Bus.isConnected())
  {
    QDBusMessage Message =
      QDBusMessage::createMethodCall(
        QStringLiteral("org.freedesktop.DBus"), QStringLiteral("/org/freedesktop/DBus"),
        QStringLiteral("org.freedesktop.DBus"), QStringLiteral("ListQueuedOwners"));
    Message << ServiceName;
    QDBusReply<QStringList> Reply = Bus.call(Message);
    if (Reply.isValid())
    {
      for (const QString & Owner : Reply.value())
      {
        if (Owner != Bus.baseService())
        {
          Result = true;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
