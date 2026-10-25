// Helpers of the Qt front end: conversions between the core's strings and
// Qt's.
//---------------------------------------------------------------------------
#ifndef QtToolsH
#define QtToolsH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <QColor>
#include <QFont>
#include <QIcon>
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
// The filter of a file dialog from a VCL filter ("Description|*.a;*.b|...")
QString FileDialogFilter(const UnicodeString & Filter);
// The colors of the configuration (VCL's 0x00BBGGRR)
QColor TColorToQColor(TColor Color);
TColor QColorToTColor(const QColor & Color);
// A square of the color; the icon with the mark of the color of the
// session (upstream's color mask of the site icons)
QIcon ColorIcon(TColor Color, const QSize & Size);
QIcon SessionColorIcon(const QIcon & Icon, TColor Color, const QSize & Size);
// The fonts of the configuration (the fixed width font of the desktop for
// the editor, the font of the desktop for the panels, by default)
struct TEditorConfiguration;
struct TFontConfiguration;
QFont EditorFont(const TEditorConfiguration & Editor);
QFont PanelFont(const TFontConfiguration & Font);
//---------------------------------------------------------------------------
bool IsMainThread();
//---------------------------------------------------------------------------
// Sizes as upstream's file panels format them (packages/filemng/BaseUtils.pas;
// TFormatBytesStyle in AppInterface.h)
UnicodeString FormatBytes(__int64 Bytes, TFormatBytesStyle Style = fbShort, bool UseUnitsForBytes = true);
UnicodeString FormatPanelBytes(__int64 Bytes, TFormatBytesStyle Style);
//---------------------------------------------------------------------------
#endif
