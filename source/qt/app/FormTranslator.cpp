// The translation of the GUI.
//---------------------------------------------------------------------------
#include "FormTranslator.h"

#include <AppConfiguration.h>
#include <FormTexts.h>
#include <TextsWin.h>

#include <QApplication>
#include <QLibraryInfo>
#include <QLocale>

#include <unordered_map>
//---------------------------------------------------------------------------
TFormTranslator::TFormTranslator(QObject * Parent) :
  QTranslator(Parent)
{
}
//---------------------------------------------------------------------------
QString TFormTranslator::Normalize(const QString & Text)
{
  QString Result;
  Result.reserve(Text.length());
  for (int Index = 0; Index < Text.length(); Index++)
  {
    QChar Ch = Text[Index];
    if (Ch == QLatin1Char('&'))
    {
      // && is an ampersand
      if ((Index + 1 < Text.length()) && (Text[Index + 1] == QLatin1Char('&')))
      {
        Result += Ch;
        Index++;
      }
    }
    else if (Ch != QLatin1Char('\r'))
    {
      Result += Ch;
    }
  }
  return ToQString(BrandText(ToUnicodeString(Result)));
}
//---------------------------------------------------------------------------
void TFormTranslator::Add(const QString & English, const QString & Translated)
{
  QString Key = Normalize(English);
  if (!Key.trimmed().isEmpty() && !Translated.isEmpty() && !FTexts.contains(Key))
  {
    FTexts.insert(Key, ToQString(BrandText(ToUnicodeString(Translated))).replace(QStringLiteral("\r\n"), QStringLiteral("\n")));
  }
}
//---------------------------------------------------------------------------
void TFormTranslator::Load(const TTranslationFile & File)
{
  // The English texts by the forms and the keys
  std::unordered_map<std::wstring, const wchar_t *> English;
  for (size_t Index = 0; Index < FormTextCount; Index++)
  {
    English[std::wstring(FormTexts[Index].Form) + L'\n' + FormTexts[Index].Key] = FormTexts[Index].Text;
  }

  for (const auto & Section : File)
  {
    if (!EndsText(L".dfm", Section.first))
    {
      continue;
    }
    for (const auto & Entry : Section.second)
    {
      auto I = English.find(std::wstring(Section.first.c_str()) + L'\n' + Entry.first.c_str());
      if (I != English.end())
      {
        QString Source = QString::fromWCharArray(I->second);
        QString Translated = ToQString(Entry.second);
        Add(Source, Translated);
        // The parts of the hints (short|long), as the GUI shows them apart
        if (EndsText(L".Hint", Entry.first) && Source.contains(QLatin1Char('|')) &&
            Translated.contains(QLatin1Char('|')))
        {
          Add(Source.section(QLatin1Char('|'), 0, 0), Translated.section(QLatin1Char('|'), 0, 0));
          Add(Source.section(QLatin1Char('|'), 1), Translated.section(QLatin1Char('|'), 1));
        }
      }
    }
  }
}
//---------------------------------------------------------------------------
QString TFormTranslator::translate(const char * /*Context*/, const char * SourceText,
  const char * /*Disambiguation*/, int /*N*/) const
{
  QString Result;
  if ((SourceText != nullptr) && !FTexts.isEmpty())
  {
    QString Source = QString::fromUtf8(SourceText);
    auto I = FTexts.constFind(Normalize(Source));
    if (I != FTexts.constEnd())
    {
      Result = I.value();
      // A text without an accelerator (a title, a header) gets none
      if (!Source.contains(QLatin1Char('&')) && Result.contains(QLatin1Char('&')))
      {
        Result.replace(QStringLiteral("&&"), QStringLiteral("\x01")).remove(QLatin1Char('&'))
          .replace(QStringLiteral("\x01"), QStringLiteral("&"));
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void InstallTranslation()
{
  TTranslationFile File;
  UnicodeString Code = LoadTranslation(AppConfiguration->Language, &File);
  if (!Code.IsEmpty())
  {
    TFormTranslator * Translator = new TFormTranslator(qApp);
    Translator->Load(File);
    QCoreApplication::installTranslator(Translator);
    AppLogFmt(L"Translated texts of the forms: %d", (Translator->Count()));

    // The texts of Qt (the standard buttons, the file dialogs, ...)
    QString Locale = ToQString(TranslationLocale(Code)).replace(QStringLiteral("@latin"), QStringLiteral("_Latn"));
    QTranslator * QtTranslator = new QTranslator(qApp);
    if (QtTranslator->load(QLocale(Locale), QStringLiteral("qtbase"), QStringLiteral("_"),
          QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
    {
      QCoreApplication::installTranslator(QtTranslator);
    }
    else
    {
      delete QtTranslator;
    }

    // Upstream's BiDiMode of the translation
    if (LoadStr(BIDI_MODE) == L"1")
    {
      QGuiApplication::setLayoutDirection(Qt::RightToLeft);
    }
  }
}
//---------------------------------------------------------------------------
