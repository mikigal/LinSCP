// The desktop shortcuts of sites and workspaces.
//---------------------------------------------------------------------------
#include "QtTools.h"
#include "DesktopShortcut.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
//---------------------------------------------------------------------------
// An argument of the Exec key of a desktop entry
static QString QuoteExecArgument(const QString & Argument)
{
  QString Result;
  for (QChar Char : Argument)
  {
    if ((Char == QLatin1Char('"')) || (Char == QLatin1Char('`')) || (Char == QLatin1Char('$')) || (Char == QLatin1Char('\\')))
    {
      Result += QLatin1Char('\\');
    }
    Result += Char;
  }
  // The field codes of the key start with a percent sign
  Result.replace(QLatin1Char('%'), QStringLiteral("%%"));
  return QLatin1Char('"') + Result + QLatin1Char('"');
}
//---------------------------------------------------------------------------
// A value of a key of a desktop entry
static QString EscapeValue(const QString & Value)
{
  QString Result = Value;
  Result.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
  Result.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
  return Result;
}
//---------------------------------------------------------------------------
static void CreateDesktopShortCut(const QString & Name, const QStringList & Arguments)
{
  QString Directory = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
  QString FileBase = Name;
  for (QChar & Char : FileBase)
  {
    if ((Char == QLatin1Char('/')) || Char.isNull())
    {
      Char = QLatin1Char('_');
    }
  }
  if (!QDir().mkpath(Directory))
  {
    throw ExtException(LoadStr(CREATE_SHORTCUT_ERROR), ToUnicodeString(Directory));
  }
  QString FileName = QDir(Directory).filePath(FileBase + QStringLiteral(".desktop"));

  QString Exec = QuoteExecArgument(QCoreApplication::applicationFilePath());
  for (const QString & Argument : Arguments)
  {
    Exec += QLatin1Char(' ') + QuoteExecArgument(Argument);
  }
  QString Content =
    QStringLiteral("[Desktop Entry]\n"
                   "Type=Application\n"
                   "Version=1.0\n"
                   "Name=%1\n"
                   "Exec=%2\n"
                   "Icon=folder-remote\n"
                   "Terminal=false\n").arg(EscapeValue(Name), Exec);

  QFile File(FileName);
  if (!File.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
      (File.write(Content.toUtf8()) < 0) ||
      // Desktops run executable desktop entries outside of the application
      // directories only
      !File.setPermissions(File.permissions() | QFileDevice::ExeOwner | QFileDevice::ExeUser))
  {
    throw ExtException(LoadStr(CREATE_SHORTCUT_ERROR), ToUnicodeString(FileName + QStringLiteral(": ") + File.errorString()));
  }
}
//---------------------------------------------------------------------------
void CreateDesktopSessionShortCut(const UnicodeString & SessionName)
{
  // The name of the file from the last part of the name of the site;
  // encode session name because of slashes in hierarchical sessions
  CreateDesktopShortCut(ToQString(UnixExtractFileName(SessionName)), { ToQString(EncodeUrlString(SessionName)) });
}
//---------------------------------------------------------------------------
void CreateAppDesktopShortCut()
{
  CreateDesktopShortCut(ToQString(AppName), QStringList());
}
//---------------------------------------------------------------------------
