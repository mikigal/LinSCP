// Helpers of the Qt front end: conversions between the core's strings and
// Qt's.
//---------------------------------------------------------------------------
#ifndef QtToolsH
#define QtToolsH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <QString>
#include <QUrl>
//---------------------------------------------------------------------------
// The core's strings are UTF-32 (wchar_t); names that are not valid UTF-8
// map their bytes to lone surrogates, which QString does not keep. Use
// these for what is displayed or typed, and keep the core's strings for
// names that go back to the core.
QString ToQString(const UnicodeString & Str);
UnicodeString ToUnicodeString(const QString & Str);
// file:// URLs of local paths and back, with the bytes of the names
UnicodeString UrlToLocalPath(const QUrl & Url);
QUrl LocalPathToUrl(const UnicodeString & Path);
//---------------------------------------------------------------------------
bool IsMainThread();
//---------------------------------------------------------------------------
// Sizes as upstream's file panels format them (packages/filemng/BaseUtils.pas)
enum TFormatBytesStyle { fbNone, fbKilobytes, fbShort };
UnicodeString FormatBytes(__int64 Bytes, TFormatBytesStyle Style = fbShort, bool UseUnitsForBytes = true);
UnicodeString FormatPanelBytes(__int64 Bytes, TFormatBytesStyle Style);
//---------------------------------------------------------------------------
#endif
