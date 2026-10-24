// Helpers of the Qt front end: conversions between the core's strings and
// Qt's.
//---------------------------------------------------------------------------
#ifndef QtToolsH
#define QtToolsH
//---------------------------------------------------------------------------
#include <AppPCH.h>

#include <QString>
//---------------------------------------------------------------------------
// The core's strings are UTF-32 (wchar_t); names that are not valid UTF-8
// map their bytes to lone surrogates, which QString does not keep. Use
// these for what is displayed or typed, and keep the core's strings for
// names that go back to the core.
QString ToQString(const UnicodeString & Str);
UnicodeString ToUnicodeString(const QString & Str);
//---------------------------------------------------------------------------
bool IsMainThread();
//---------------------------------------------------------------------------
#endif
