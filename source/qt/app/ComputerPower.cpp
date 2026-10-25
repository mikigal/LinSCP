// Suspending and shutting down the computer.
//---------------------------------------------------------------------------
#include "ComputerPower.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
//---------------------------------------------------------------------------
static const QString ServiceName = QStringLiteral("org.freedesktop.login1");
static const QString ObjectPath = QStringLiteral("/org/freedesktop/login1");
static const QString InterfaceName = QStringLiteral("org.freedesktop.login1.Manager");
// An authorization may be asked for (while logind waits)
static const int CallTimeout = 120000;
//---------------------------------------------------------------------------
static bool CanDo(const QString & Method)
{
  QDBusConnection Bus = QDBusConnection::systemBus();
  if (!Bus.isConnected())
  {
    return false;
  }
  QDBusMessage Call = QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, Method);
  QDBusReply<QString> Reply = Bus.call(Call, QDBus::Block, 5000);
  // "challenge": after an authorization
  return Reply.isValid() && ((Reply.value() == QLatin1String("yes")) || (Reply.value() == QLatin1String("challenge")));
}
//---------------------------------------------------------------------------
static void Do(const QString & Method)
{
  QDBusConnection Bus = QDBusConnection::systemBus();
  if (!Bus.isConnected())
  {
    throw Exception(ToUnicodeString(Bus.lastError().message()));
  }
  QDBusMessage Call = QDBusMessage::createMethodCall(ServiceName, ObjectPath, InterfaceName, Method);
  // Interactive: an authorization may be asked for
  Call << true;
  QDBusMessage Reply = Bus.call(Call, QDBus::Block, CallTimeout);
  if (Reply.type() == QDBusMessage::ErrorMessage)
  {
    throw Exception(ToUnicodeString(Reply.errorMessage()));
  }
}
//---------------------------------------------------------------------------
bool CanSuspendComputer()
{
  return CanDo(QStringLiteral("CanSuspend"));
}
//---------------------------------------------------------------------------
bool CanShutDownComputer()
{
  return CanDo(QStringLiteral("CanPowerOff"));
}
//---------------------------------------------------------------------------
void SuspendComputer()
{
  Do(QStringLiteral("Suspend"));
}
//---------------------------------------------------------------------------
void ShutDownComputer()
{
  Do(QStringLiteral("PowerOff"));
}
//---------------------------------------------------------------------------
