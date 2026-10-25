// The translation of the GUI: the texts of the GUI are the English texts of
// upstream's forms, a translator maps them to the texts of the translation
// (its sections of the .dfm files); the resource strings are translated by
// the application layer (app/Translations.cpp). Qt's own texts come from
// Qt's translations.
//---------------------------------------------------------------------------
#ifndef FormTranslatorH
#define FormTranslatorH
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <Translations.h>

#include <QHash>
#include <QTranslator>
//---------------------------------------------------------------------------
class TFormTranslator : public QTranslator
{
public:
  explicit TFormTranslator(QObject * Parent = nullptr);

  // The texts of the forms of the translation
  void Load(const TTranslationFile & File);
  int Count() const { return FTexts.size(); }

  virtual QString translate(const char * Context, const char * SourceText,
    const char * Disambiguation = nullptr, int N = -1) const override;
  virtual bool isEmpty() const override { return FTexts.isEmpty(); }

  // The key of a text: without the accelerators and the line breaks of
  // Windows, branded
  static QString Normalize(const QString & Text);

private:
  QHash<QString, QString> FTexts;

  void Add(const QString & English, const QString & Translated);
};
//---------------------------------------------------------------------------
// The translation of the configuration (or of the language of the system)
// is loaded and installed; the Qt's translation of its language too
void InstallTranslation();
//---------------------------------------------------------------------------
#endif
