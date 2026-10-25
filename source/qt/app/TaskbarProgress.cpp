// The progress of the operations on the button of the application.
//---------------------------------------------------------------------------
#include "TaskbarProgress.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QVariantMap>
//---------------------------------------------------------------------------
// The application, by its desktop file
static const QString ApplicationUri = QStringLiteral("application://openscp.desktop");
static const QString ObjectPath = QStringLiteral("/org/openscp/LauncherEntry");
static const QString InterfaceName = QStringLiteral("com.canonical.Unity.LauncherEntry");
// Not shown
static const int NoProgress = -1;
//---------------------------------------------------------------------------
TTaskbarProgress::TTaskbarProgress() :
  FProgress(NoProgress),
  FUrgent(false)
{
}
//---------------------------------------------------------------------------
TTaskbarProgress::~TTaskbarProgress()
{
  if ((FProgress != NoProgress) || FUrgent)
  {
    Update(false, 0, false);
  }
}
//---------------------------------------------------------------------------
void TTaskbarProgress::SetProgress(int Progress)
{
  if (Progress < 0)
  {
    SetNoProgress();
  }
  else if (Progress != FProgress)
  {
    FProgress = Progress;
    Update(true, Progress, FUrgent);
  }
}
//---------------------------------------------------------------------------
void TTaskbarProgress::SetNoProgress()
{
  if (FProgress != NoProgress)
  {
    FProgress = NoProgress;
    Update(false, 0, FUrgent);
  }
}
//---------------------------------------------------------------------------
void TTaskbarProgress::SetUrgent(bool Urgent)
{
  if (FUrgent != Urgent)
  {
    FUrgent = Urgent;
    Update((FProgress != NoProgress), (FProgress != NoProgress) ? FProgress : 0, Urgent);
  }
}
//---------------------------------------------------------------------------
void TTaskbarProgress::Update(bool ProgressVisible, int Progress, bool Urgent)
{
  QDBusMessage Signal = QDBusMessage::createSignal(ObjectPath, InterfaceName, QStringLiteral("Update"));
  QVariantMap Properties;
  Properties[QStringLiteral("progress-visible")] = ProgressVisible;
  Properties[QStringLiteral("progress")] = Progress / 100.0;
  Properties[QStringLiteral("urgent")] = Urgent;
  Signal << ApplicationUri << Properties;
  QDBusConnection::sessionBus().send(Signal);
}
//---------------------------------------------------------------------------
