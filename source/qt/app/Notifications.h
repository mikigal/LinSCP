// The notifications of the desktop (upstream's balloons of the tray icon,
// TTrayIcon::PopupBalloon of windows/Tools.cpp), through the
// org.freedesktop.Notifications service of the session bus.
//---------------------------------------------------------------------------
#ifndef NotificationsH
#define NotificationsH
//---------------------------------------------------------------------------
#include <QtInterface.h>

#include <QObject>

#include <functional>
#include <map>
//---------------------------------------------------------------------------
// The user clicked the notification; the token (if the server gave one)
// lets a window be activated
typedef std::function<void(const QString & ActivationToken)> TNotificationClickEvent;
//---------------------------------------------------------------------------
class TNotifications : public QObject
{
  Q_OBJECT

public:
  explicit TNotifications(QObject * Parent);
  virtual ~TNotifications();

  // Shows the notification for the seconds (none when there is no server)
  void Notify(const QString & Title, const QString & Message, TQueryType Type, int Seconds,
    TNotificationClickEvent OnClick = nullptr);

private Q_SLOTS:
  void ActionInvoked(uint Id, const QString & ActionKey);
  void ActivationToken(uint Id, const QString & Token);
  void NotificationClosed(uint Id, uint Reason);

private:
  struct TNotification
  {
    TNotificationClickEvent OnClick;
    QString ActivationToken;
  };
  std::map<uint, TNotification> FNotifications;
  // The server takes markup in the body (-1: not asked yet)
  int FBodyMarkup;
};
//---------------------------------------------------------------------------
#endif
