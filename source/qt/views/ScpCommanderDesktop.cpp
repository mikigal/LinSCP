// The desktop integration of the Commander (upstream's tray icon, its
// balloons and the task bar of TCustomScpExplorerForm,
// forms/CustomScpExplorer.cpp): the notifications, the system tray, the
// progress and the attention on the button of the task manager.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <Notifications.h>
#include <TaskbarProgress.h>
#include <TerminalManager.h>
#include <Vcl.Consts.hpp>

#include "ProgressForm.h"
#include "ScpCommander.h"

#include <QApplication>
#include <QEvent>
#include <QIcon>
#include <QSystemTrayIcon>
//---------------------------------------------------------------------------
void TScpCommanderForm::Notify(TTerminal * ATerminal, const UnicodeString & AMessage, TQueryType Type,
  bool Important, TNotificationClickEvent OnClick)
{
  if (AppConfiguration->BalloonNotifications)
  {
    UnicodeString Message = AMessage;
    UnicodeString Title;
    if ((ATerminal == NULL) && (Type == qtInformation) && ExtractMainInstructions(Message, Title))
    {
      Message = TrimLeft(Message);
    }
    else
    {
      Message = UnformatMessage(Message);
      switch (Type)
      {
        case qtConfirmation:
          Title = Vcl_Consts_SMsgDlgConfirm;
          break;
        case qtWarning:
          Title = Vcl_Consts_SMsgDlgWarning;
          break;
        case qtError:
          Title = Vcl_Consts_SMsgDlgError;
          break;
        default:
          Title = Vcl_Consts_SMsgDlgInformation;
          break;
      }
      if (ATerminal != NULL)
      {
        Title = ATerminal->SessionData->SessionName + TitleSeparator + Title;
      }
    }
    int Seconds = AppConfiguration->NotificationsTimeout;
    if (Important)
    {
      Seconds *= 5;
    }
    FNotifications->Notify(ToQString(Title), ToQString(Message), Type, Seconds, OnClick);
  }
  else
  {
    FlashOnBackground();
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::FlashOnBackground()
{
  if (!QApplication::activeWindow() && AppConfiguration->FlashTaskbar)
  {
    QApplication::alert(this);
    FTaskbarProgress->SetUrgent(true);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::changeEvent(QEvent * Event)
{
  QMainWindow::changeEvent(Event);
  if ((Event->type() == QEvent::ActivationChange) && isActiveWindow())
  {
    FTaskbarProgress->SetUrgent(false);
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::ActivateFromOtherApplication(const QString & ActivationToken)
{
  // Wayland's focus stealing prevention lets the token of the other
  // application activate the window
  if (!ActivationToken.isEmpty())
  {
    qputenv("XDG_ACTIVATION_TOKEN", ActivationToken.toUtf8());
  }
  RestoreApplication();
  QWidget * Window = QApplication::activeModalWidget();
  if (Window == nullptr)
  {
    Window = this;
  }
  Window->raise();
  Window->activateWindow();
}
//---------------------------------------------------------------------------
bool TScpCommanderForm::IsSystemTrayAvailable()
{
  return QSystemTrayIcon::isSystemTrayAvailable();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::MinimizeApplication(bool ToTray)
{
  FHiddenWindows.clear();
  FMinimizedWindows.clear();
  bool Tray = (ToTray || AppConfiguration->MinimizeToTray) && IsSystemTrayAvailable();
  if (Tray)
  {
    if (FTrayIcon == nullptr)
    {
      FTrayIcon = new QSystemTrayIcon(this);
      FTrayIcon->setObjectName(QStringLiteral("TrayIcon"));
      FTrayIcon->setIcon(ApplicationIcon());
      connect(FTrayIcon, &QSystemTrayIcon::activated, this, [this]() { RestoreApplication(); });
    }
    UpdateTrayIcon();
    FTrayIcon->show();
  }
  for (QWidget * Window : QApplication::topLevelWidgets())
  {
    if (Window->isVisible() && !Window->isMinimized() &&
        ((Window->windowType() == Qt::Window) || (Window->windowType() == Qt::Dialog)))
    {
      // A window of a modal loop would end the loop when hidden
      if (Tray && !Window->isModal())
      {
        FHiddenWindows.push_back(Window);
        Window->hide();
      }
      else
      {
        FMinimizedWindows.push_back(Window);
        Window->showMinimized();
      }
    }
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::RestoreApplication()
{
  if ((FTrayIcon != nullptr) && FTrayIcon->isVisible())
  {
    FTrayIcon->hide();
  }
  for (const QPointer<QWidget> & Window : FHiddenWindows)
  {
    if (!Window.isNull())
    {
      Window->show();
    }
  }
  for (const QPointer<QWidget> & Window : FMinimizedWindows)
  {
    if (!Window.isNull() && Window->isMinimized())
    {
      Window->showNormal();
    }
  }
  FHiddenWindows.clear();
  FMinimizedWindows.clear();
}
//---------------------------------------------------------------------------
void TScpCommanderForm::UpdateTrayIcon()
{
  if (FTrayIcon != nullptr)
  {
    FTrayIcon->setToolTip(windowTitle());
  }
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SetTaskbarProgress(TFileOperationProgressType * ProgressData)
{
  int OverallProgress;
  // The progress of a synchronization over its operations
  if ((FProgressForm != nullptr) && (FProgressForm->GetSynchronizeProgress() != nullptr))
  {
    OverallProgress = FProgressForm->GetSynchronizeProgress()->Progress(ProgressData);
  }
  else if (!ProgressData->IsIndeterminate())
  {
    OverallProgress = ProgressData->OverallProgress();
  }
  else
  {
    OverallProgress = -1;
  }
  FTaskbarProgress->SetProgress(OverallProgress);
}
//---------------------------------------------------------------------------
void TScpCommanderForm::SetQueueProgress()
{
  // The progress of the operation in the foreground prevails
  if (FProgressForm == nullptr)
  {
    // Not when items are queued, but processing is disabled
    if ((FQueueStatus != NULL) && (FQueueStatus->ActivePrimaryCount > 0))
    {
      TFileOperationProgressType * ProgressData = NULL;
      if (FQueueStatus->IsOnlyOneActiveAndNoPending() &&
          (FQueueStatus->Items[FQueueStatus->DoneCount] != NULL))
      {
        ProgressData = FQueueStatus->Items[FQueueStatus->DoneCount]->ProgressData;
      }
      if ((ProgressData != NULL) && ProgressData->InProgress)
      {
        SetTaskbarProgress(ProgressData);
      }
      else
      {
        FTaskbarProgress->SetProgress(-1);
      }
    }
    else
    {
      FTaskbarProgress->SetNoProgress();
    }
  }
}
//---------------------------------------------------------------------------
