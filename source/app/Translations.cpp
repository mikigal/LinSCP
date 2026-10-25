// The translations of upstream.
//---------------------------------------------------------------------------
#include "AppPCH.h"

#include "Translations.h"

#include <cstdlib>
#include <fstream>
#include <sstream>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
struct TLanguage
{
  const wchar_t * Code;
  const wchar_t * Locale;
};
//---------------------------------------------------------------------------
// Upstream's codes of the translations (Windows' abbreviations of the
// languages, mostly) and the locales of their languages
const TLanguage Languages[] =
{
  { L"AF", L"af" }, { L"AR", L"ar" }, { L"BE", L"be" }, { L"BG", L"bg" }, { L"BN", L"bn" },
  { L"CA", L"ca" }, { L"CH", L"zh_TW" }, { L"CHS", L"zh_CN" }, { L"CS", L"cs" }, { L"DA", L"da" },
  { L"DE", L"de" }, { L"EL", L"el" }, { L"ES", L"es" }, { L"ET", L"et" }, { L"FA", L"fa" },
  { L"FI", L"fi" }, { L"FP", L"fil" }, { L"FR", L"fr" }, { L"GL", L"gl" }, { L"HE", L"he" },
  { L"HI", L"hi" }, { L"HR", L"hr" }, { L"HU", L"hu" }, { L"HY", L"hy" }, { L"IN", L"id" },
  { L"IS", L"is" }, { L"IT", L"it" }, { L"JP", L"ja" }, { L"KA", L"ka" }, { L"KO", L"ko" },
  { L"LT", L"lt" }, { L"LV", L"lv" }, { L"MK", L"mk" }, { L"MS", L"ms" }, { L"MYA", L"my" },
  { L"NL", L"nl" }, { L"NO", L"nb" }, { L"PA", L"pa" }, { L"PL", L"pl" }, { L"PT", L"pt_BR" },
  { L"PTG", L"pt_PT" }, { L"RO", L"ro" }, { L"RU", L"ru" }, { L"SK", L"sk" }, { L"SL", L"sl" },
  { L"SQ", L"sq" }, { L"SRL", L"sr@latin" }, { L"SV", L"sv" }, { L"TA", L"ta" }, { L"TH", L"th" },
  { L"TR", L"tr" }, { L"UK", L"uk" }, { L"VI", L"vi" }, { L"XXE", L"eo" }, { L"XXK", L"kab" },
};
//---------------------------------------------------------------------------
UnicodeString FAppliedTranslation;
std::map<std::string, UnicodeString> FDelphiResourceStrings;
//---------------------------------------------------------------------------
// The language, the country and the modifier of a locale (pl_PL.UTF-8,
// sr_RS@latin)
void SplitLocale(UnicodeString Locale, UnicodeString & Language, UnicodeString & Country, UnicodeString & Modifier)
{
  int P = Locale.Pos(L"@");
  Modifier = (P > 0) ? Locale.SubString(P + 1, Locale.Length() - P) : UnicodeString();
  if (P > 0)
  {
    Locale.SetLength(P - 1);
  }
  P = Locale.Pos(L".");
  if (P > 0)
  {
    Locale.SetLength(P - 1);
  }
  P = Locale.Pos(L"_");
  Language = ((P > 0) ? Locale.SubString(1, P - 1) : Locale).LowerCase();
  Country = (P > 0) ? Locale.SubString(P + 1, Locale.Length() - P).UpperCase() : UnicodeString();
}
//---------------------------------------------------------------------------
// The translation of the locale: of its language and country (pt_BR), of
// its language and modifier (sr@latin), of its language (pl); the Norwegian
// languages share the translation
UnicodeString LocaleTranslation(const UnicodeString & Locale, const std::vector<TTranslation> & Translations, bool & English)
{
  UnicodeString Language, Country, Modifier;
  SplitLocale(Locale, Language, Country, Modifier);
  English = (Language == L"en");
  if ((Language == L"no") || (Language == L"nn"))
  {
    Language = L"nb";
  }
  std::vector<UnicodeString> Candidates;
  if (!Country.IsEmpty())
  {
    Candidates.push_back(Language + L"_" + Country);
  }
  if (!Modifier.IsEmpty())
  {
    Candidates.push_back(Language + L"@" + Modifier);
  }
  Candidates.push_back(Language);
  for (const UnicodeString & Candidate : Candidates)
  {
    for (const TTranslation & Translation : Translations)
    {
      if (Translation.Locale == Candidate)
      {
        return Translation.Code;
      }
    }
  }
  // Another country of the language (pt_AO is pt_PT, zh_SG is zh_CN)
  for (const TTranslation & Translation : Translations)
  {
    UnicodeString TranslationLanguage, TranslationCountry, TranslationModifier;
    SplitLocale(Translation.Locale, TranslationLanguage, TranslationCountry, TranslationModifier);
    if ((TranslationLanguage == Language) && TranslationModifier.IsEmpty() && Modifier.IsEmpty())
    {
      return Translation.Code;
    }
  }
  return UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString Unescape(const UnicodeString & Value)
{
  UnicodeString Result;
  for (int Index = 1; Index <= Value.Length(); Index++)
  {
    wchar_t Ch = Value[Index];
    if ((Ch == L'\\') && (Index < Value.Length()))
    {
      Index++;
      switch (Value[Index])
      {
        case L'n': Result += L'\n'; break;
        case L'r': Result += L'\r'; break;
        case L't': Result += L'\t'; break;
        default: Result += Value[Index]; break;
      }
    }
    else
    {
      Result += Ch;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
UnicodeString TranslationsDirectory()
{
  const char * Environment = getenv("LINSCP_TRANSLATIONS");
  if ((Environment != NULL) && (*Environment != '\0'))
  {
    return UTF8ToString(Environment);
  }
  UnicodeString BinaryDirectory = ExcludeTrailingBackslash(ExtractFilePath(ParamStr(0)));
  UnicodeString Installed =
    IncludeTrailingBackslash(ExtractFilePath(BinaryDirectory)) + L"share/linscp/translations";
  if (DirectoryExists(ApiPath(Installed)))
  {
    return Installed;
  }
#ifdef LINSCP_SOURCE_TRANSLATIONS
  return UTF8ToString(LINSCP_SOURCE_TRANSLATIONS);
#else
  return Installed;
#endif
}
//---------------------------------------------------------------------------
UnicodeString TranslationLocale(const UnicodeString & Code)
{
  for (const TLanguage & Language : Languages)
  {
    if (SameText(Code, Language.Code))
    {
      return Language.Locale;
    }
  }
  return UnicodeString();
}
//---------------------------------------------------------------------------
std::vector<TTranslation> FindTranslations()
{
  std::vector<TTranslation> Result;
  UnicodeString Directory = IncludeTrailingBackslash(TranslationsDirectory());
  for (const TLanguage & Language : Languages)
  {
    UnicodeString FileName = Directory + Language.Code + L".ini";
    if (FileExists(ApiPath(FileName)))
    {
      TTranslation Translation;
      Translation.Code = Language.Code;
      Translation.Locale = Language.Locale;
      Translation.FileName = FileName;
      Result.push_back(Translation);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString SystemTranslation(const std::vector<TTranslation> & Translations)
{
  // gettext's order: the C locale is English, else LANGUAGE's list wins
  UnicodeString Locale;
  for (const char * Name : { "LC_ALL", "LC_MESSAGES", "LANG" })
  {
    const char * Value = getenv(Name);
    if ((Value != NULL) && (*Value != '\0'))
    {
      Locale = UTF8ToString(Value);
      break;
    }
  }
  if (Locale.IsEmpty() || (Locale == L"C") || (Locale == L"POSIX") || StartsStr(L"C.", Locale))
  {
    return UnicodeString();
  }
  std::vector<UnicodeString> Candidates;
  const char * LanguageList = getenv("LANGUAGE");
  if (LanguageList != NULL)
  {
    UnicodeString List = UTF8ToString(LanguageList);
    while (!List.IsEmpty())
    {
      UnicodeString Candidate = CutToChar(List, L':', true);
      if (!Candidate.IsEmpty())
      {
        Candidates.push_back(Candidate);
      }
    }
  }
  Candidates.push_back(Locale);
  for (const UnicodeString & Candidate : Candidates)
  {
    bool English;
    UnicodeString Result = LocaleTranslation(Candidate, Translations, English);
    if (!Result.IsEmpty() || English)
    {
      return Result;
    }
  }
  return UnicodeString();
}
//---------------------------------------------------------------------------
void ReadTranslationFile(const UnicodeString & FileName, TTranslationFile & File)
{
  std::ifstream Stream(UTF8Encode(FileName).c_str(), std::ios::binary);
  if (!Stream)
  {
    throw Exception(FMTLOAD(FILE_NOT_FOUND, (FileName)));
  }
  std::stringstream Buffer;
  Buffer << Stream.rdbuf();
  std::string Data = Buffer.str();
  // Without the byte order mark
  if (Data.compare(0, 3, "\xEF\xBB\xBF") == 0)
  {
    Data.erase(0, 3);
  }

  TTranslationSection * Section = NULL;
  std::istringstream Lines(Data);
  std::string RawLine;
  while (std::getline(Lines, RawLine))
  {
    UnicodeString Line = UTF8ToString(RawLine.c_str()).TrimRight();
    if (Line.IsEmpty() || (Line[1] == L';'))
    {
      continue;
    }
    if ((Line[1] == L'[') && (Line[Line.Length()] == L']'))
    {
      Section = &File[Line.SubString(2, Line.Length() - 2)];
    }
    else if (Section != NULL)
    {
      int P = Line.Pos(L"=");
      if (P > 1)
      {
        UnicodeString Key = Line.SubString(1, P - 1).Trim();
        UnicodeString Value = Line.SubString(P + 1, Line.Length() - P);
        if ((Value.Length() >= 2) && (Value[1] == L'"') && (Value[Value.Length()] == L'"'))
        {
          Value = Value.SubString(2, Value.Length() - 2);
        }
        Section->push_back(std::make_pair(Key, Unescape(Value)));
      }
    }
  }
}
//---------------------------------------------------------------------------
int ApplyResourceTranslations(const TTranslationFile & File)
{
  int Result = 0;
  for (const auto & Section : File)
  {
    // The resource strings of the units of Delphi
    if (Section.first == L"WinSCP_DRC.rc")
    {
      for (const auto & Entry : Section.second)
      {
        if (!Entry.second.IsEmpty())
        {
          FDelphiResourceStrings[std::string(UTF8Encode(Entry.first).c_str())] = BrandText(Entry.second);
          Result++;
        }
      }
    }
    // The sections of the string tables (TextsCore1.rc, TextsWin1.rc, ...)
    else if (EndsText(L".rc", Section.first))
    {
      for (const auto & Entry : Section.second)
      {
        int Ident = rtl::FindResourceStringIdent(UTF8Encode(Entry.first).c_str());
        // An untranslated string is empty
        if ((Ident >= 0) && !Entry.second.IsEmpty())
        {
          rtl::SetResourceString(Ident, BrandText(Entry.second));
          Result++;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString LoadTranslation(const UnicodeString & Language, TTranslationFile * File)
{
  std::vector<TTranslation> Translations = FindTranslations();
  UnicodeString Code = Language.IsEmpty() ? SystemTranslation(Translations) : Language;
  UnicodeString Result;
  for (const TTranslation & Translation : Translations)
  {
    if (SameText(Translation.Code, Code))
    {
      TTranslationFile Data;
      try
      {
        ReadTranslationFile(Translation.FileName, Data);
      }
      catch (Exception & E)
      {
        AppLogFmt(L"Cannot load translation %s: %s", (Translation.FileName, E.Message));
        break;
      }
      ApplyResourceTranslations(Data);
      Result = Translation.Code;
      AppLogFmt(L"Loaded translation %s", (Translation.FileName));
      if (File != NULL)
      {
        *File = std::move(Data);
      }
      break;
    }
  }
  FAppliedTranslation = Result;
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString AppliedTranslation()
{
  return FAppliedTranslation;
}
//---------------------------------------------------------------------------
void ResetTranslation()
{
  rtl::ResetResourceStrings();
  FDelphiResourceStrings.clear();
  FAppliedTranslation = UnicodeString();
}
//---------------------------------------------------------------------------
UnicodeString DelphiResourceString(const char * Name, const UnicodeString & English)
{
  auto I = FDelphiResourceStrings.find(Name);
  return (I != FDelphiResourceStrings.end()) ? I->second : English;
}
//---------------------------------------------------------------------------
UnicodeString BrandText(const UnicodeString & Text)
{
  return ReplaceStr(Text, L"WinSCP", AppName);
}
//---------------------------------------------------------------------------
