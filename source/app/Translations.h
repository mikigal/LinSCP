// The translations of upstream (translations/*.ini, one per language, named
// by upstream's codes: PL.ini, CHS.ini, ...): the resource strings by their
// names in the sections of the .rc files ([TextsCore1.rc] ...), the texts
// of the forms by the paths of their components in the sections of the
// .dfm files ([Login.dfm] ...), which the GUI translates.
//---------------------------------------------------------------------------
#ifndef TranslationsH
#define TranslationsH
//---------------------------------------------------------------------------
#include <Classes.hpp>

#include <map>
#include <utility>
#include <vector>
//---------------------------------------------------------------------------
struct TTranslation
{
  // Upstream's code (PL)
  UnicodeString Code;
  // The locale of the language (pl, pt_BR, sr@latin)
  UnicodeString Locale;
  UnicodeString FileName;
};
//---------------------------------------------------------------------------
typedef std::vector<std::pair<UnicodeString, UnicodeString>> TTranslationSection;
typedef std::map<UnicodeString, TTranslationSection> TTranslationFile;
//---------------------------------------------------------------------------
// The directory of the translations: $LINSCP_TRANSLATIONS, the installed
// ones (share/linscp/translations next to the binaries) or those of the
// source tree (a build run from its directory)
UnicodeString TranslationsDirectory();
// The translations found, of the known codes, by the codes
std::vector<TTranslation> FindTranslations();
// The locale of the code, empty if not known
UnicodeString TranslationLocale(const UnicodeString & Code);
// The translation of the language of the system (gettext's order of
// LANGUAGE, LC_ALL, LC_MESSAGES, LANG), empty for English
UnicodeString SystemTranslation(const std::vector<TTranslation> & Translations);
// The sections of the file with their keys and (unescaped) values
void ReadTranslationFile(const UnicodeString & FileName, TTranslationFile & File);
// The resource strings of the file replace the English ones; returns their
// count
int ApplyResourceTranslations(const TTranslationFile & File);
// The translation of the language (a code, empty for the system's one) is
// applied to the resource strings, the file returned when asked; returns
// the code of the translation, empty when English
UnicodeString LoadTranslation(const UnicodeString & Language, TTranslationFile * File = NULL);
// The code of the translation in use, empty for English
UnicodeString AppliedTranslation();
// The English texts again
void ResetTranslation();
// A resource string of the VCL or another unit of Delphi (upstream's
// [WinSCP_DRC.rc] section, Vcl_Consts_SMsgDlgYes), the English text unless
// translated
UnicodeString DelphiResourceString(const char * Name, const UnicodeString & English);
// The texts of upstream name WinSCP, the texts of LinSCP name it (the
// English resource strings are branded by rc2cpp)
UnicodeString BrandText(const UnicodeString & Text);
//---------------------------------------------------------------------------
#endif
