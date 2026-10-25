// Helpers of the Qt front end.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <AppConfiguration.h>

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QPainter>
#include <QThread>

#include <cmath>
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
UnicodeString UrlToLocalPath(const QUrl & Url)
{
  // QUrl keeps percent-encoded bytes that are not valid UTF-8
  QByteArray Bytes = QByteArray::fromPercentEncoding(Url.path(QUrl::FullyEncoded).toLatin1());
  UnicodeString Result = UTF8ToString(RawByteString(Bytes.constData(), Bytes.size()));
  if ((Result.Length() > 1) && EndsStr(L"/", Result))
  {
    Result.SetLength(Result.Length() - 1);
  }
  return Result;
}
//---------------------------------------------------------------------------
QUrl LocalPathToUrl(const UnicodeString & Path)
{
  UTF8String Bytes = UTF8Encode(Path);
  return QUrl::fromEncoded(QByteArray("file://") + QByteArray(Bytes.c_str(), Bytes.Length()).toPercentEncoding("/"));
}
//---------------------------------------------------------------------------
QString FileDialogFilter(const UnicodeString & Filter)
{
  QStringList Result;
  QStringList Parts = ToQString(Filter).split(QLatin1Char('|'));
  for (int Index = 0; Index + 1 < Parts.count(); Index += 2)
  {
    QString Description = Parts[Index];
    // Qt takes the patterns from the parentheses
    int P = Description.lastIndexOf(QLatin1Char('('));
    if (P > 0)
    {
      Description = Description.left(P).trimmed();
    }
    QStringList Patterns = Parts[Index + 1].split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (QString & Pattern : Patterns)
    {
      if (Pattern == QLatin1String("*.*"))
      {
        Pattern = QStringLiteral("*");
      }
    }
    Result.append(QStringLiteral("%1 (%2)").arg(Description, Patterns.join(QLatin1Char(' '))));
  }
  return Result.join(QStringLiteral(";;"));
}
//---------------------------------------------------------------------------
QColor TColorToQColor(TColor Color)
{
  return QColor(Color & 0xFF, (Color >> 8) & 0xFF, (Color >> 16) & 0xFF);
}
//---------------------------------------------------------------------------
TColor QColorToTColor(const QColor & Color)
{
  return static_cast<TColor>(Color.red() | (Color.green() << 8) | (Color.blue() << 16));
}
//---------------------------------------------------------------------------
QIcon ColorIcon(TColor Color, const QSize & Size)
{
  QPixmap Pixmap(Size);
  Pixmap.fill(Qt::transparent);
  QPainter Painter(&Pixmap);
  Painter.setPen(TColorToQColor(Color).darker(150));
  Painter.setBrush(TColorToQColor(Color));
  Painter.drawRect(QRect(QPoint(0, 0), Size).adjusted(1, 1, -2, -2));
  return QIcon(Pixmap);
}
//---------------------------------------------------------------------------
QIcon SessionColorIcon(const QIcon & Icon, TColor Color, const QSize & Size)
{
  QPixmap Pixmap = Icon.pixmap(Size);
  QPainter Painter(&Pixmap);
  // The bottom right quarter
  QSize MarkSize(Size.width() / 2, Size.height() / 2);
  QRect Mark(QPoint(Size.width() - MarkSize.width(), Size.height() - MarkSize.height()), MarkSize);
  Painter.setPen(TColorToQColor(Color).darker(150));
  Painter.setBrush(TColorToQColor(Color));
  Painter.drawRect(Mark.adjusted(0, 0, -1, -1));
  return QIcon(Pixmap);
}
//---------------------------------------------------------------------------
QFont EditorFont(const TEditorConfiguration & Editor)
{
  QFont Font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  if (!Editor.FontName.IsEmpty())
  {
    Font.setFamily(ToQString(Editor.FontName));
  }
  if (Editor.FontSize > 0)
  {
    Font.setPointSize(Editor.FontSize);
  }
  return Font;
}
//---------------------------------------------------------------------------
QFont PanelFont(const TFontConfiguration & AFont)
{
  QFont Font = QApplication::font();
  if (!AFont.FontName.IsEmpty())
  {
    Font.setFamily(ToQString(AFont.FontName));
    if (AFont.FontSize > 0)
    {
      Font.setPointSize(AFont.FontSize);
    }
    Font.setBold(FLAGSET(AFont.FontStyle, fsBold));
    Font.setItalic(FLAGSET(AFont.FontStyle, fsItalic));
    Font.setUnderline(FLAGSET(AFont.FontStyle, fsUnderline));
    Font.setStrikeOut(FLAGSET(AFont.FontStyle, fsStrikeOut));
  }
  return Font;
}
//---------------------------------------------------------------------------
bool IsMainThread()
{
  QCoreApplication * Application = QCoreApplication::instance();
  return (Application != nullptr) && (QThread::currentThread() == Application->thread());
}
//---------------------------------------------------------------------------
UnicodeString FormatBytes(__int64 Bytes, TFormatBytesStyle Style, bool UseUnitsForBytes)
{
  Extended Value;
  UnicodeString Format;
  if ((Style == fbNone) || ((Style == fbShort) && (Bytes < 1024)))
  {
    Value = static_cast<Extended>(Bytes);
    Format = L"#,##0";
    if (UseUnitsForBytes)
    {
      Format += L" \"B\"";
    }
  }
  else if (Style == fbKilobytes)
  {
    // Rounded up
    __int64 Kilobytes = (Bytes / 1024) + (((Bytes % 1024) > 0) ? 1 : 0);
    Value = static_cast<Extended>(Kilobytes);
    Format = L"#,##0 \"KB\"";
  }
  else
  {
    __int64 Order;
    UnicodeString SizeUnit;
    if (Bytes < 1000 * 1024)
    {
      Order = 1024;
      SizeUnit = L"KB";
    }
    else if (Bytes < 1000 * 1024 * 1024)
    {
      Order = 1024 * 1024;
      SizeUnit = L"MB";
    }
    else
    {
      Order = 1024 * 1024 * 1024;
      SizeUnit = L"GB";
    }

    Value = static_cast<Extended>(Bytes) / Order;
    if (Value >= 100)
    {
      Value = floor(Value);
      Format = L"#,##0";
    }
    else if (Value >= 10)
    {
      Value = floor(Value * 10) / 10;
      Format = L"#0.0";
    }
    else
    {
      Value = floor(Value * 100) / 100;
      Format = L"0.00";
    }
    Format += L" \"" + SizeUnit + L"\"";
  }
  return FormatFloat(Format, Value);
}
//---------------------------------------------------------------------------
UnicodeString FormatPanelBytes(__int64 Bytes, TFormatBytesStyle Style)
{
  return FormatBytes(Bytes, Style, (Style != fbNone));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
QString ApplicationDataFile(const QString & Installed, const QString & Source)
{
  QDir BinaryDirectory(QCoreApplication::applicationDirPath());
  QString Result = BinaryDirectory.absoluteFilePath(QStringLiteral("../share/") + Installed);
  if (!QFileInfo::exists(Result))
  {
    Result = QDir(QStringLiteral(OPENSCP_SOURCE_DIR)).filePath(Source);
  }
  return Result;
}
//---------------------------------------------------------------------------
