// The notifications of the desktop.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include "Notifications.h"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
//---------------------------------------------------------------------------
static const QString ServiceName = QStringLiteral("org.freedesktop.Notifications");
static const QString ObjectPath = QStringLiteral("/org/freedesktop/Notifications");
static const QString InterfaceName = QStringLiteral("org.freedesktop.Notifications");
// The action of a click on the notification
static const QString DefaultAction = QStringLiteral("default");
//---------------------------------------------------------------------------
TNotifications::TNotifications(QObject * Parent) :
  QObject(Parent),
  FBodyMarkup(-1)
{
  QDBusConnection Bus = QDBusConnection::sessionBus();
  Bus.connect(ServiceName, ObjectPath, InterfaceName, QStringLiteral("ActionInvoked"),
    this, SLOT(ActionInvoked(uint, QString)));
  Bus.connect(ServiceName, ObjectPath, InterfaceName, QStringLiteral("ActivationToken"),
    this, SLOT(ActivationToken(uint, QString)));
  Bus.connect(ServiceName, ObjectPath, InterfaceName, QStringLiteral("NotificationClosed"),
    this, SLOT(NotificationClosed(uint, uint)));
}
//---------------------------------------------------------------------------
TNotifications::~TNotifications()
{
  QDBusConnection Bus = QDBusConnection::sessionBus();
  Bus.disconnect(ServiceName, ObjectPath, InterfaceName, QStringLiteral("ActionInvoked"),
    this, SLOT(ActionInvoked(uint, QString)));
  Bus.disconnect(ServiceName, ObjectPath, InterfaceName, QStringLiteral("ActivationToken"),
    this, SLOT(ActivationToken(uint, QString)));
  Bus.disconnect(ServiceName, ObjectPath, InterfaceName, QStringLiteral("NotificationClosed"),
    this, SLOT(NotificationClosed(uint, uint)));
}
//---------------------------------------------------------------------------
void TNotifications::Notify(const QString & Title, const QString & Message, TQueryType Type, int Seconds,
  TNotificationClickEvent OnClick)
{
  // A notification server runs with the desktop shell; one that is only
  // activatable may wait for a shell that does not run
  QDBusConnection Bus = QDBusConnection::sessionBus();
  if (!Bus.isConnected() || !Bus.interface()->isServiceRegistered(ServiceName))
  {
    return;
  }
  QStringList Actions;
  if (OnClick)
  {
    Actions << DefaultAction << QString();
  }
  QVariantMap Hints;
  // The server shows the icon and the name of the application of the file
  Hints[QStringLiteral("desktop-entry")] = QStringLiteral("openscp");
  // Low, normal, critical
  Hints[QStringLiteral("urgency")] = QVariant::fromValue<uchar>((Type == qtError) ? 2 : 1);
  if (FBodyMarkup < 0)
  {
    QDBusMessage CapabilitiesCall =
      QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, QStringLiteral("GetCapabilities"));
    CapabilitiesCall.setAutoStartService(false);
    QDBusReply<QStringList> Capabilities = Bus.call(CapabilitiesCall, QDBus::Block, 1000);
    FBodyMarkup = (Capabilities.isValid() && Capabilities.value().contains(QStringLiteral("body-markup"))) ? 1 : 0;
  }
  QDBusMessage Call = QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, QStringLiteral("Notify"));
  Call.setAutoStartService(false);
  Call
    << QStringLiteral("OpenSCP") << uint(0) << QString() << Title << ((FBodyMarkup > 0) ? Message.toHtmlEscaped() : Message)
    << Actions << Hints << int(Seconds * 1000);
  QDBusPendingCallWatcher * Watcher = new QDBusPendingCallWatcher(Bus.asyncCall(Call), this);
  connect(Watcher, &QDBusPendingCallWatcher::finished, this, [this, OnClick](QDBusPendingCallWatcher * AWatcher)
  {
    QDBusPendingReply<uint> Reply = *AWatcher;
    if (Reply.isValid() && OnClick)
    {
      FNotifications[Reply.value()].OnClick = OnClick;
    }
    AWatcher->deleteLater();
  });
}
//---------------------------------------------------------------------------
void TNotifications::ActivationToken(uint Id, const QString & Token)
{
  // Comes before the action
  auto Iterator = FNotifications.find(Id);
  if (Iterator != FNotifications.end())
  {
    Iterator->second.ActivationToken = Token;
  }
}
//---------------------------------------------------------------------------
void TNotifications::ActionInvoked(uint Id, const QString & ActionKey)
{
  auto Iterator = FNotifications.find(Id);
  if ((Iterator != FNotifications.end()) && (ActionKey == DefaultAction))
  {
    TNotification Notification = Iterator->second;
    FNotifications.erase(Iterator);
    Notification.OnClick(Notification.ActivationToken);
  }
}
//---------------------------------------------------------------------------
void TNotifications::NotificationClosed(uint Id, uint /*Reason*/)
{
  FNotifications.erase(Id);
}
//---------------------------------------------------------------------------
