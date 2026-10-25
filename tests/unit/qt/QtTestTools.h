// Helpers of the L3 tests of the Qt front end.
//---------------------------------------------------------------------------
#ifndef QtTestToolsH
#define QtTestToolsH
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QWidget>

#include <functional>
//---------------------------------------------------------------------------
// Runs Action on the modal window once a modal event loop shows it (a
// dialog executed by the test after the call)
inline void WhenModal(std::function<void(QWidget * Dialog)> Action)
{
  QTimer * Timer = new QTimer();
  Timer->setInterval(10);
  QObject::connect(Timer, &QTimer::timeout, [Timer, Action]()
  {
    QWidget * Dialog = QApplication::activeModalWidget();
    if ((Dialog != nullptr) && Dialog->isVisible())
    {
      Timer->stop();
      Timer->deleteLater();
      Action(Dialog);
    }
  });
  Timer->start();
}
//---------------------------------------------------------------------------
// The same for the modal window of the name (a dialog over another one)
inline void WhenModal(const char * Name, std::function<void(QWidget * Dialog)> Action)
{
  QTimer * Timer = new QTimer();
  Timer->setInterval(10);
  QString AName = QString::fromLatin1(Name);
  QObject::connect(Timer, &QTimer::timeout, [Timer, Action, AName]()
  {
    QWidget * Dialog = QApplication::activeModalWidget();
    if ((Dialog != nullptr) && Dialog->isVisible() && (Dialog->objectName() == AName))
    {
      Timer->stop();
      Timer->deleteLater();
      Action(Dialog);
    }
  });
  Timer->start();
}
//---------------------------------------------------------------------------
// Runs Action on the popup menu once it shows (QMenu::exec)
inline void WhenPopup(std::function<void(QWidget * Popup)> Action)
{
  QTimer * Timer = new QTimer();
  Timer->setInterval(10);
  QObject::connect(Timer, &QTimer::timeout, [Timer, Action]()
  {
    QWidget * Popup = QApplication::activePopupWidget();
    if ((Popup != nullptr) && Popup->isVisible())
    {
      Timer->stop();
      Timer->deleteLater();
      Action(Popup);
    }
  });
  Timer->start();
}
//---------------------------------------------------------------------------
template<class T>
T * Child(QWidget * Parent, const char * Name)
{
  return Parent->findChild<T *>(QString::fromLatin1(Name));
}
//---------------------------------------------------------------------------
#endif
