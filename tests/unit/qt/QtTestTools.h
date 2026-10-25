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
template<class T>
T * Child(QWidget * Parent, const char * Name)
{
  return Parent->findChild<T *>(QString::fromLatin1(Name));
}
//---------------------------------------------------------------------------
#endif
