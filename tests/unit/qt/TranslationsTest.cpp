// L3 tests of the translations (app/Translations.cpp of the application
// layer, app/FormTranslator.cpp of the GUI).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <FormTranslator.h>
#include <TextsCore.h>
#include <Translations.h>

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <cstdlib>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The variables of the locale for the test, restored after it
class TLocaleEnvironment
{
public:
  TLocaleEnvironment()
  {
    for (const char * Name : Names)
    {
      const char * Value = getenv(Name);
      FValues.push_back((Value != nullptr) ? std::string(Value) : std::string("\x01"));
      unsetenv(Name);
    }
  }

  ~TLocaleEnvironment()
  {
    for (size_t Index = 0; Index < FValues.size(); Index++)
    {
      if (FValues[Index] == "\x01")
      {
        unsetenv(Names[Index]);
      }
      else
      {
        setenv(Names[Index], FValues[Index].c_str(), 1);
      }
    }
  }

  void Set(const char * Name, const char * Value) { setenv(Name, Value, 1); }

private:
  static constexpr const char * Names[] = { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG" };
  std::vector<std::string> FValues;
};
//---------------------------------------------------------------------------
std::vector<TTranslation> SomeTranslations()
{
  std::vector<TTranslation> Result;
  for (const wchar_t * Code : { L"DE", L"PL", L"PT", L"PTG", L"SRL", L"NO" })
  {
    TTranslation Translation;
    Translation.Code = Code;
    Translation.Locale = TranslationLocale(Code);
    Result.push_back(Translation);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString WriteFile(const QTemporaryDir & Dir, const char * Text)
{
  QString Path = Dir.filePath(QStringLiteral("XX.ini"));
  QFile File(Path);
  EXPECT_TRUE(File.open(QIODevice::WriteOnly));
  File.write(Text);
  return ToUnicodeString(Path);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(TranslationsTest, SystemLanguage)
{
  std::vector<TTranslation> Translations = SomeTranslations();
  TLocaleEnvironment Environment;
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString());
  Environment.Set("LANG", "pl_PL.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"PL"));
  // The country of the language
  Environment.Set("LANG", "pt_BR.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"PT"));
  Environment.Set("LANG", "pt_PT.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"PTG"));
  Environment.Set("LANG", "sr_RS.UTF-8@latin");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"SRL"));
  Environment.Set("LANG", "nn_NO.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"NO"));
  Environment.Set("LANG", "de_AT.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"DE"));
  // English and the languages without translations
  Environment.Set("LANG", "en_GB.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString());
  Environment.Set("LANG", "ja_JP.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString());
  // LANGUAGE's list first, LC_MESSAGES over LANG, the C locale is English
  Environment.Set("LANGUAGE", "ja:de");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"DE"));
  Environment.Set("LANGUAGE", "en:de");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString());
  unsetenv("LANGUAGE");
  Environment.Set("LC_MESSAGES", "pl_PL.UTF-8");
  Environment.Set("LANG", "de_DE.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString(L"PL"));
  Environment.Set("LC_ALL", "C.UTF-8");
  EXPECT_EQ(SystemTranslation(Translations), UnicodeString());
}
//---------------------------------------------------------------------------
TEST(TranslationsTest, ResourceStrings)
{
  QTemporaryDir Dir;
  UnicodeString FileName = WriteFile(Dir,
    "\xEF\xBB\xBF[TextsCore1.rc]\r\n"
    "YES_STR=\"Tak\"\r\n"
    "NO_STR=\"Nie \\\"teraz\\\"\\nlub \\\\ WinSCP\"\r\n"
    "UNKNOWN_STR=\"x\"\r\n"
    "\r\n"
    "[Cleanup.dfm]\r\n"
    "Caption=\"Usu\xC5\x84 dane\"\r\n");
  TTranslationFile File;
  ReadTranslationFile(FileName, File);
  ASSERT_EQ(File.size(), 2u);
  ASSERT_EQ(File[L"TextsCore1.rc"].size(), 3u);
  EXPECT_EQ(File[L"TextsCore1.rc"][1].second, UnicodeString(L"Nie \"teraz\"\nlub \\ WinSCP"));
  EXPECT_EQ(File[L"Cleanup.dfm"][0].second, UnicodeString(L"Usuń dane"));

  // The known strings only, branded
  EXPECT_EQ(ApplyResourceTranslations(File), 2);
  EXPECT_EQ(LoadStr(YES_STR), UnicodeString(L"Tak"));
  EXPECT_EQ(LoadStr(NO_STR), UnicodeString(L"Nie \"teraz\"\nlub \\ LinSCP"));
  ResetTranslation();
  EXPECT_EQ(LoadStr(YES_STR), UnicodeString(L"Yes"));
}
//---------------------------------------------------------------------------
TEST(TranslationsTest, FormTexts)
{
  QTemporaryDir Dir;
  UnicodeString FileName = WriteFile(Dir,
    "[Cleanup.dfm]\r\n"
    "Caption=\"Usu\xC5\x84 dane aplikacji\"\r\n"
    "CheckAllButton.Caption=\"&Wszystkie\"\r\n"
    "Label1.Caption=\"Lista.\\r\\n\\r\\nUwaga.\"\r\n"
    "[NonVisual.dfm]\r\n"
    "ExplorerActions.RemoteCopyNonQueueAction.Caption=\"&Pobierz...\"\r\n"
    "ExplorerActions.RemoteCopyNonQueueAction.Hint=\"Pobierz|Pobierz pliki\"\r\n");
  TTranslationFile File;
  ReadTranslationFile(FileName, File);
  TFormTranslator Translator;
  Translator.Load(File);
  // By the English texts of upstream's forms, with or without their
  // accelerators
  EXPECT_EQ(Translator.translate("QDialog", "Cleanup application data"), QStringLiteral("Usuń dane aplikacji"));
  EXPECT_EQ(Translator.translate("QDialog", "Un/check &all"), QStringLiteral("&Wszystkie"));
  EXPECT_EQ(Translator.translate("QDialog", "Un/check all"), QStringLiteral("Wszystkie"));
  EXPECT_EQ(Translator.translate("QObject", "Down&load..."), QStringLiteral("&Pobierz..."));
  // A text without an accelerator (a title, a header) gets none
  EXPECT_EQ(Translator.translate("QObject", "Download..."), QStringLiteral("Pobierz..."));
  // The line breaks of Windows are not the GUI's
  EXPECT_EQ(Translator.translate("QDialog",
    "Following list contains all application data possibly stored on this computer. "
    "Check those you want to remove.\n\n"
    "If another instance of application is running, please close it before cleaning data.\n\n"
    "Notice: Opening session and/or next execution of application will recreate some of data."),
    QStringLiteral("Lista.\n\nUwaga."));
  // The parts of the hints
  EXPECT_EQ(Translator.translate("QObject", "Download selected remote file(s) to local directory"),
    QStringLiteral("Pobierz pliki"));
  EXPECT_TRUE(Translator.translate("QObject", "Not translated").isEmpty());
}
//---------------------------------------------------------------------------
TEST(TranslationsTest, ProjectTranslations)
{
  // Of the source tree
  std::vector<TTranslation> Translations = FindTranslations();
  EXPECT_GE(Translations.size(), 50u);
  TTranslationFile File;
  UnicodeString PolishFileName;
  for (const TTranslation & Translation : Translations)
  {
    EXPECT_FALSE(Translation.Locale.IsEmpty()) << Translation.Code.c_str();
    if (Translation.Code == L"PL")
    {
      PolishFileName = Translation.FileName;
    }
  }
  ASSERT_FALSE(PolishFileName.IsEmpty());
  ReadTranslationFile(PolishFileName, File);
  EXPECT_GT(ApplyResourceTranslations(File), 1000);
  EXPECT_EQ(LoadStr(YES_STR), UnicodeString(L"Tak"));
  // Of the VCL
  EXPECT_EQ(DelphiResourceString("Vcl_Consts_SMsgDlgYes", L"&Yes"), UnicodeString(L"&Tak"));
  ResetTranslation();
  EXPECT_EQ(DelphiResourceString("Vcl_Consts_SMsgDlgYes", L"&Yes"), UnicodeString(L"&Yes"));
  TFormTranslator Translator;
  Translator.Load(File);
  EXPECT_GT(Translator.Count(), 1000);
  EXPECT_EQ(Translator.translate("QDialog", "Login"), QStringLiteral("Logowanie"));
}
//---------------------------------------------------------------------------
