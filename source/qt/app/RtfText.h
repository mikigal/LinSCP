// The rich text of the core (the URLs, scripts and code it generates, its
// RTF of Common.cpp's Rtf* functions) shown in Qt: converted to HTML or to
// plain text. Only the subset of RTF the core generates is understood: the
// color table, the colors, the italic and bold text, the paragraphs, the
// escaped characters, the hyperlink fields.
//---------------------------------------------------------------------------
#ifndef RtfTextH
#define RtfTextH
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QString>
//---------------------------------------------------------------------------
// HTML of the text (white space kept); the targets of the links are the
// hrefs of the anchors
QString RtfToHtml(const UnicodeString & Rtf);
// The text without the formatting and the links (their texts only)
UnicodeString RtfToPlainText(const UnicodeString & Rtf);
//---------------------------------------------------------------------------
#endif
