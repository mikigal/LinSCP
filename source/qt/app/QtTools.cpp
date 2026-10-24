// Helpers of the Qt front end.
//---------------------------------------------------------------------------
#include "QtTools.h"

#include <QCoreApplication>
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
