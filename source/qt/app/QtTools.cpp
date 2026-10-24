// Helpers of the Qt front end.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <QCoreApplication>
#include <QThread>

#include <string>
//---------------------------------------------------------------------------
QString ToQString(const UnicodeString & Str)
{
  return QString::fromWCharArray(Str.c_str(), Str.Length());
}
//---------------------------------------------------------------------------
UnicodeString ToUnicodeString(const QString & Str)
{
  std::wstring Wide = Str.toStdWString();
  return UnicodeString(Wide.c_str(), static_cast<int>(Wide.size()));
}
//---------------------------------------------------------------------------
bool IsMainThread()
{
  QCoreApplication * Application = QCoreApplication::instance();
  return (Application != nullptr) && (QThread::currentThread() == Application->thread());
}
//---------------------------------------------------------------------------
