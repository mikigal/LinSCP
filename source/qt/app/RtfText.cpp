// The rich text of the core in Qt.
//---------------------------------------------------------------------------
#include "RtfText.h"

#include <QColor>

#include <vector>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
// The destinations of the groups
enum TRtfDestination { rdText, rdSkip, rdColorTable, rdFieldInstruction, rdFieldResult };
//---------------------------------------------------------------------------
struct TRtfState
{
  TRtfDestination Destination = rdText;
  int Color = 0;
  bool Italic = false;
  bool Bold = false;
  // The group of the field result, its anchor is closed with the group
  bool LinkGroup = false;
};
//---------------------------------------------------------------------------
class TRtfConverter
{
public:
  explicit TRtfConverter(bool Html) : FHtml(Html) {}

  QString Convert(const UnicodeString & Rtf);

private:
  bool FHtml;
  QString FResult;
  std::vector<TRtfState> FStack;
  TRtfState FState;
  std::vector<QColor> FColors;
  QColor FColor;
  bool FColorPending = false;
  QString FFieldInstruction;
  QString FLink;
  // The formatting of the open span
  bool FSpanOpen = false;
  int FSpanColor = 0;
  bool FSpanItalic = false;
  bool FSpanBold = false;

  void ControlWord(const QString & Word, bool HasParam, int Param);
  void Text(QChar Ch);
  void CloseSpan();
  void OpenGroup();
  void CloseGroup();
};
//---------------------------------------------------------------------------
void TRtfConverter::CloseSpan()
{
  if (FSpanOpen)
  {
    FResult += QStringLiteral("</span>");
    FSpanOpen = false;
  }
}
//---------------------------------------------------------------------------
void TRtfConverter::Text(QChar Ch)
{
  switch (FState.Destination)
  {
    case rdFieldInstruction:
      FFieldInstruction += Ch;
      break;

    case rdText:
    case rdFieldResult:
      if (!FHtml)
      {
        FResult += Ch;
      }
      else
      {
        bool Formatted = (FState.Color > 0) || FState.Italic || FState.Bold;
        if (FSpanOpen &&
            ((FSpanColor != FState.Color) || (FSpanItalic != FState.Italic) || (FSpanBold != FState.Bold)))
        {
          CloseSpan();
        }
        if (Formatted && !FSpanOpen)
        {
          QString Style;
          if ((FState.Color > 0) && (FState.Color < static_cast<int>(FColors.size())))
          {
            Style += QStringLiteral("color:%1;").arg(FColors[FState.Color].name());
          }
          if (FState.Italic)
          {
            Style += QStringLiteral("font-style:italic;");
          }
          if (FState.Bold)
          {
            Style += QStringLiteral("font-weight:bold;");
          }
          FResult += QStringLiteral("<span style=\"%1\">").arg(Style);
          FSpanOpen = true;
          FSpanColor = FState.Color;
          FSpanItalic = FState.Italic;
          FSpanBold = FState.Bold;
        }
        if (Ch == QLatin1Char('\n'))
        {
          FResult += QStringLiteral("<br>");
        }
        else
        {
          FResult += QString(Ch).toHtmlEscaped();
        }
      }
      break;

    case rdColorTable:
      if (Ch == QLatin1Char(';'))
      {
        // The first entry is the default color
        FColors.push_back(FColorPending ? FColor : QColor());
        FColor = QColor(0, 0, 0);
        FColorPending = false;
      }
      break;

    case rdSkip:
      break;
  }
}
//---------------------------------------------------------------------------
void TRtfConverter::ControlWord(const QString & Word, bool HasParam, int Param)
{
  if (Word == QLatin1String("par"))
  {
    Text(QLatin1Char('\n'));
  }
  else if (Word == QLatin1String("tab"))
  {
    Text(QLatin1Char('\t'));
  }
  else if (Word == QLatin1String("cf"))
  {
    FState.Color = HasParam ? Param : 0;
  }
  else if (Word == QLatin1String("i"))
  {
    FState.Italic = !HasParam || (Param != 0);
  }
  else if (Word == QLatin1String("b"))
  {
    FState.Bold = !HasParam || (Param != 0);
  }
  else if (Word == QLatin1String("colortbl"))
  {
    FState.Destination = rdColorTable;
  }
  else if ((Word == QLatin1String("red")) || (Word == QLatin1String("green")) || (Word == QLatin1String("blue")))
  {
    if (FState.Destination == rdColorTable)
    {
      FColorPending = true;
      if (Word == QLatin1String("red"))
      {
        FColor.setRed(Param);
      }
      else if (Word == QLatin1String("green"))
      {
        FColor.setGreen(Param);
      }
      else
      {
        FColor.setBlue(Param);
      }
    }
  }
  else if ((Word == QLatin1String("fonttbl")) || (Word == QLatin1String("stylesheet")) || (Word == QLatin1String("info")))
  {
    FState.Destination = rdSkip;
  }
  else if (Word == QLatin1String("fldinst"))
  {
    FState.Destination = rdFieldInstruction;
    FFieldInstruction.clear();
  }
  else if (Word == QLatin1String("fldrslt"))
  {
    FState.Destination = rdFieldResult;
    // The target of HYPERLINK "target"
    QString Instruction = FFieldInstruction.trimmed();
    FLink.clear();
    if (Instruction.startsWith(QLatin1String("HYPERLINK")))
    {
      int P1 = Instruction.indexOf(QLatin1Char('"'));
      int P2 = (P1 >= 0) ? Instruction.indexOf(QLatin1Char('"'), P1 + 1) : -1;
      if (P2 > P1)
      {
        FLink = Instruction.mid(P1 + 1, P2 - P1 - 1);
      }
    }
    if (FHtml && !FLink.isEmpty())
    {
      CloseSpan();
      FResult += QStringLiteral("<a href=\"%1\" style=\"text-decoration:none\">").arg(FLink.toHtmlEscaped());
      FState.LinkGroup = true;
    }
  }
  // Other control words (\rtf1, \f0, \fs, \field, ...) do not matter
}
//---------------------------------------------------------------------------
void TRtfConverter::OpenGroup()
{
  FStack.push_back(FState);
  FState.LinkGroup = false;
}
//---------------------------------------------------------------------------
void TRtfConverter::CloseGroup()
{
  if (FState.LinkGroup)
  {
    CloseSpan();
    FResult += QStringLiteral("</a>");
  }
  if (!FStack.empty())
  {
    FState = FStack.back();
    FStack.pop_back();
  }
}
//---------------------------------------------------------------------------
QString TRtfConverter::Convert(const UnicodeString & Rtf)
{
  QString Source = ToQString(Rtf);
  int Index = 0;
  int Length = Source.length();
  while (Index < Length)
  {
    QChar Ch = Source[Index];
    if (Ch == QLatin1Char('{'))
    {
      OpenGroup();
      Index++;
    }
    else if (Ch == QLatin1Char('}'))
    {
      CloseGroup();
      Index++;
    }
    else if (Ch == QLatin1Char('\\'))
    {
      Index++;
      if (Index >= Length)
      {
        break;
      }
      QChar Next = Source[Index];
      if (Next.isLetter() && (Next.unicode() < 0x80))
      {
        int Start = Index;
        while ((Index < Length) && Source[Index].isLetter() && (Source[Index].unicode() < 0x80))
        {
          Index++;
        }
        QString Word = Source.mid(Start, Index - Start);
        bool HasParam = false;
        int Param = 0;
        int ParamStart = Index;
        if ((Index < Length) && (Source[Index] == QLatin1Char('-')))
        {
          Index++;
        }
        while ((Index < Length) && Source[Index].isDigit())
        {
          Index++;
        }
        if (Index > ParamStart)
        {
          Param = Source.mid(ParamStart, Index - ParamStart).toInt(&HasParam);
        }
        // The space delimiting the control word is a part of it
        if ((Index < Length) && (Source[Index] == QLatin1Char(' ')))
        {
          Index++;
        }
        if (Word == QLatin1String("u"))
        {
          // The character, the substitute that follows is skipped
          char32_t Code = static_cast<char32_t>((Param < 0) ? (Param + 0x10000) : Param);
          for (QChar UnicodeCh : QString::fromUcs4(&Code, 1))
          {
            Text(UnicodeCh);
          }
          if (Index < Length)
          {
            Index++;
          }
        }
        else
        {
          ControlWord(Word, HasParam, Param);
        }
      }
      else if (Next == QLatin1Char('*'))
      {
        // An ignorable destination, unless known (the field instruction)
        Index++;
        int Look = Index;
        while ((Look < Length) && Source[Look].isSpace())
        {
          Look++;
        }
        if (!Source.mid(Look).startsWith(QLatin1String("\\fldinst")))
        {
          FState.Destination = rdSkip;
        }
      }
      else
      {
        // An escaped character (\\, \{, \})
        if ((Next != QLatin1Char('\n')) && (Next != QLatin1Char('\r')))
        {
          Text(Next);
        }
        Index++;
      }
    }
    else
    {
      // The line breaks of the source are not the text
      if ((Ch != QLatin1Char('\n')) && (Ch != QLatin1Char('\r')))
      {
        Text(Ch);
      }
      Index++;
    }
  }
  CloseSpan();
  return FResult;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
QString RtfToHtml(const UnicodeString & Rtf)
{
  TRtfConverter Converter(true);
  return QStringLiteral("<div style=\"white-space:pre-wrap\">%1</div>").arg(Converter.Convert(Rtf));
}
//---------------------------------------------------------------------------
UnicodeString RtfToPlainText(const UnicodeString & Rtf)
{
  TRtfConverter Converter(false);
  return ToUnicodeString(Converter.Convert(Rtf));
}
//---------------------------------------------------------------------------
