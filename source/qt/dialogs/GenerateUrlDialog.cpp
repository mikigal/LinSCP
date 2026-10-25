// The URL of the session or of files and the script opening the session
// (upstream's forms/GenerateUrl.dfm). The scripts are for openscp-cli: a
// script file, a shell script (in place of upstream's batch file) and the
// command-line arguments; upstream's PowerShell script and the code of the
// .NET assembly are not generated.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <AppConfiguration.h>
#include <GUIConfiguration.h>
#include <Interface.h>
#include <ProgParams.h>
#include <QtInterface.h>
#include <RtfText.h>
#include <SessionData.h>
#include <TextsWin.h>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <vector>
//---------------------------------------------------------------------------
namespace {
//---------------------------------------------------------------------------
const UnicodeString LogSwitch(L"log");
const UnicodeString IniSwitch(L"ini");
// Of the hyperlink fields of the core's RTF (Common.cpp)
const UnicodeString RtfHyperlinkFieldPrefix(L"HYPERLINK \"");
const UnicodeString RtfHyperlinkFieldSuffix(L"\" ");
//---------------------------------------------------------------------------
UnicodeString RtfScriptComment(const UnicodeString & Text)
{
  return RtfColorItalicText(7, Text);
}
//---------------------------------------------------------------------------
UnicodeString RtfScriptPlaceholder(const UnicodeString & Text)
{
  return RtfColorText(7, Text);
}
//---------------------------------------------------------------------------
UnicodeString RtfScriptCommand(const UnicodeString & Command)
{
  return RtfLink(ScriptCommandLink(Command), RtfKeyword(Command));
}
//---------------------------------------------------------------------------
UnicodeString RtfCommandlineSwitch(const UnicodeString & Switch, const UnicodeString & Anchor)
{
  return RtfLink(L"commandline#" + Anchor, RtfParameter(TProgramParams::FormatSwitch(Switch.LowerCase())));
}
//---------------------------------------------------------------------------
// The text of the RTF escaped for a double-quoted string of the shell:
// the quotes, the dollars, the backticks and the backslashes get a
// backslash; the targets of the links stay as they are
UnicodeString RtfShellEscape(const UnicodeString & Rtf)
{
  const UnicodeString EscapedBackslash(L"\\\\");
  UnicodeString Result;
  int Index = 1;
  int Length = Rtf.Length();
  while (Index <= Length)
  {
    wchar_t Ch = Rtf[Index];
    if ((Ch == L'H') && (Rtf.SubString(Index, RtfHyperlinkFieldPrefix.Length()) == RtfHyperlinkFieldPrefix))
    {
      int End = PosEx(RtfHyperlinkFieldSuffix, Rtf, Index + RtfHyperlinkFieldPrefix.Length());
      int Count = (End > 0) ? (End + RtfHyperlinkFieldSuffix.Length() - Index) : (Length - Index + 1);
      Result += Rtf.SubString(Index, Count);
      Index += Count;
    }
    else if (Ch == L'\\')
    {
      wchar_t Next = (Index < Length) ? Rtf[Index + 1] : L'\0';
      if (Next == L'\\')
      {
        // A literal backslash
        Result += EscapedBackslash + EscapedBackslash;
        Index += 2;
      }
      else if (IsLetter(Next))
      {
        // A control word with its parameter and its delimiter (and the
        // substitute character of \u)
        int Start = Index;
        Index++;
        while ((Index <= Length) && IsLetter(Rtf[Index]))
        {
          Index++;
        }
        bool Unicode = (Rtf.SubString(Start, Index - Start) == L"\\u");
        if ((Index <= Length) && (Rtf[Index] == L'-'))
        {
          Index++;
        }
        while ((Index <= Length) && IsDigit(Rtf[Index]))
        {
          Index++;
        }
        if ((Index <= Length) && (Unicode || (Rtf[Index] == L' ')))
        {
          Index++;
        }
        Result += Rtf.SubString(Start, Index - Start);
      }
      else
      {
        Result += Rtf.SubString(Index, 2);
        Index += 2;
      }
    }
    else
    {
      if ((Ch == L'"') || (Ch == L'$') || (Ch == L'`'))
      {
        Result += EscapedBackslash;
      }
      Result += Ch;
      Index++;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
class TGenerateUrlDialog : public QDialog
{
public:
  TGenerateUrlDialog(TSessionData * Data, TStrings * Paths);

  void Execute();

private:
  TSessionData * FData;
  std::unique_ptr<TStrings> FPaths;
  bool FChanging;
  bool FUrlCounted;
  bool FScriptCounted;
  UnicodeString FResult;

  QTabWidget * OptionsPageControl;
  QWidget * UrlSheet;
  QWidget * ScriptSheet;
  QCheckBox * UserNameCheck;
  QCheckBox * PasswordCheck;
  QCheckBox * HostKeyCheck;
  QCheckBox * RemoteDirectoryCheck;
  QCheckBox * WinSCPSpecificCheck;
  QCheckBox * SaveExtensionCheck;
  QCheckBox * RawSettingsCheck;
  std::vector<std::pair<QCheckBox *, int>> FComponentChecks;
  QComboBox * ScriptFormatCombo;
  QLabel * ScriptDescriptionLabel;
  QGroupBox * ResultGroup;
  QTextBrowser * ResultMemo;

  bool IsFileUrl() const { return (FPaths.get() != NULL); }
  QCheckBox * AddCheck(QGridLayout * Grid, int Row, int Column, const char * Name, const QString & Caption, int Component);
  UnicodeString GenerateUrl(UnicodeString Path);
  UnicodeString GenerateUrl();
  UnicodeString GenerateScript(UnicodeString & ScriptDescription);
  static UnicodeString CliExeName();
  void UpdateControls();
};
//---------------------------------------------------------------------------
TGenerateUrlDialog::TGenerateUrlDialog(TSessionData * Data, TStrings * Paths) :
  QDialog(QApplication::activeWindow()),
  FData(Data),
  FChanging(false),
  FUrlCounted(false),
  FScriptCounted(false)
{
  setObjectName(QStringLiteral("GenerateUrlDialog"));
  if (Paths != NULL)
  {
    FPaths.reset(new TStringList());
    FPaths->AddStrings(Paths);
  }

  QVBoxLayout * MainLayout = new QVBoxLayout(this);
  OptionsPageControl = new QTabWidget(this);
  OptionsPageControl->setObjectName(QStringLiteral("OptionsPageControl"));
  MainLayout->addWidget(OptionsPageControl);

  UrlSheet = new QWidget(OptionsPageControl);
  UrlSheet->setObjectName(QStringLiteral("UrlSheet"));
  QGridLayout * UrlGrid = new QGridLayout(UrlSheet);
  // The components of the URL (upstream's tags of the check boxes)
  UserNameCheck = AddCheck(UrlGrid, 0, 0, "UserNameCheck", tr("&Username"), 1);
  PasswordCheck = AddCheck(UrlGrid, 1, 0, "PasswordCheck", tr("&Password"), 2);
  HostKeyCheck = AddCheck(UrlGrid, 2, 0, "HostKeyCheck", tr("SSH &host key"), 4);
  RemoteDirectoryCheck = AddCheck(UrlGrid, 0, 1, "RemoteDirectoryCheck", tr("Initial &directory"), 8);
  WinSCPSpecificCheck = AddCheck(UrlGrid, 1, 1, "WinSCPSpecificCheck", tr("&OpenSCP-specific"), 16);
  SaveExtensionCheck = AddCheck(UrlGrid, 2, 1, "SaveExtensionCheck", tr("&Save extension"), 32);
  RawSettingsCheck = AddCheck(UrlGrid, 3, 0, "RawSettingsCheck", tr("&Advanced settings"), 64);
  UrlGrid->setRowStretch(4, 1);
  OptionsPageControl->addTab(UrlSheet, tr("URL"));

  ScriptSheet = new QWidget(OptionsPageControl);
  ScriptSheet->setObjectName(QStringLiteral("ScriptSheet"));
  QGridLayout * ScriptGrid = new QGridLayout(ScriptSheet);
  QLabel * Label2 = new QLabel(tr("&Format:"), ScriptSheet);
  ScriptGrid->addWidget(Label2, 0, 0);
  ScriptFormatCombo = new QComboBox(ScriptSheet);
  ScriptFormatCombo->setObjectName(QStringLiteral("ScriptFormatCombo"));
  // In the order of TScriptFormat (a shell script for the batch file)
  ScriptFormatCombo->addItems({ tr("Script file"), tr("Shell script"), tr("Command-line") });
  Label2->setBuddy(ScriptFormatCombo);
  ScriptGrid->addWidget(ScriptFormatCombo, 0, 1);
  ScriptGrid->setColumnStretch(2, 1);
  ScriptDescriptionLabel = new QLabel(ScriptSheet);
  ScriptDescriptionLabel->setObjectName(QStringLiteral("ScriptDescriptionLabel"));
  ScriptDescriptionLabel->setWordWrap(true);
  ScriptGrid->addWidget(ScriptDescriptionLabel, 1, 0, 1, 3);
  ScriptGrid->setRowStretch(2, 1);
  OptionsPageControl->addTab(ScriptSheet, tr("Script"));
  connect(ScriptFormatCombo, &QComboBox::currentIndexChanged, this, [this]() { UpdateControls(); });
  connect(OptionsPageControl, &QTabWidget::currentChanged, this, [this]() { UpdateControls(); });

  ResultGroup = new QGroupBox(this);
  ResultGroup->setObjectName(QStringLiteral("ResultGroup"));
  QVBoxLayout * ResultLayout = new QVBoxLayout(ResultGroup);
  ResultMemo = new QTextBrowser(ResultGroup);
  ResultMemo->setObjectName(QStringLiteral("ResultMemo"));
  ResultMemo->setOpenLinks(false);
  // The links of the commands and switches lead to their documentation
  connect(ResultMemo, &QTextBrowser::anchorClicked, this, [](const QUrl & Url)
  {
    UnicodeString Link = ToUnicodeString(Url.toString());
    if (IsHttpOrHttpsUrl(Link))
    {
      OpenBrowser(Link);
    }
    else
    {
      ShowHelp(Link);
    }
  });
  ResultLayout->addWidget(ResultMemo);
  MainLayout->addWidget(ResultGroup, 1);

  QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
  Buttons->button(QDialogButtonBox::Close)->setObjectName(QStringLiteral("CancelBtn"));
  QPushButton * ClipboardButton = Buttons->addButton(tr("&Copy to Clipboard"), QDialogButtonBox::ActionRole);
  ClipboardButton->setObjectName(QStringLiteral("ClipboardButton"));
  connect(ClipboardButton, &QPushButton::clicked, this, [this]()
  {
    UnicodeString Text = RtfToPlainText(FResult);
    // A trailing line break of the text of several lines
    if ((Text.Pos(L"\n") > 0) && !EndsStr(L"\n", Text))
    {
      Text += L"\n";
    }
    CopyToClipboard(Text);
  });
  connect(Buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
  MainLayout->addWidget(Buttons);
  AddHelpButton(Buttons, FormHelpKeyword(L"GenerateUrl.dfm"));
  resize(fontMetrics().averageCharWidth() * 90, fontMetrics().height() * 26);
}
//---------------------------------------------------------------------------
QCheckBox * TGenerateUrlDialog::AddCheck(QGridLayout * Grid, int Row, int Column, const char * Name,
  const QString & Caption, int Component)
{
  QCheckBox * Check = new QCheckBox(Caption, Grid->parentWidget());
  Check->setObjectName(QString::fromLatin1(Name));
  Grid->addWidget(Check, Row, Column);
  connect(Check, &QCheckBox::toggled, this, [this]() { UpdateControls(); });
  FComponentChecks.emplace_back(Check, Component);
  return Check;
}
//---------------------------------------------------------------------------
UnicodeString TGenerateUrlDialog::GenerateUrl(UnicodeString Path)
{
  UnicodeString Url =
    FData->GenerateSessionUrl(
      FLAGMASK(WinSCPSpecificCheck->isChecked(), sufSpecific) |
      FLAGMASK(UserNameCheck->isEnabled() && UserNameCheck->isChecked(), sufUserName) |
      FLAGMASK(PasswordCheck->isEnabled() && PasswordCheck->isChecked(), sufPassword) |
      FLAGMASK(HostKeyCheck->isEnabled() && HostKeyCheck->isChecked(), sufHostKey) |
      FLAGMASK(RawSettingsCheck->isEnabled() && RawSettingsCheck->isChecked(), sufRawSettings) |
      FLAGMASK(AppConfiguration->HttpForWebDAV, sufHttpForWebDAV));

  if ((RemoteDirectoryCheck->isEnabled() && RemoteDirectoryCheck->isChecked()) || IsFileUrl())
  {
    if (StartsStr(L"/", Path))
    {
      Path.Delete(1, 1);
    }

    Url += EncodeUrlPath(Path);
  }

  if (SaveExtensionCheck->isEnabled() && SaveExtensionCheck->isChecked())
  {
    Url += UnicodeString(UrlParamSeparator) + UrlSaveParamName;
  }

  return Url;
}
//---------------------------------------------------------------------------
UnicodeString TGenerateUrlDialog::GenerateUrl()
{
  UnicodeString Result;
  if (!IsFileUrl())
  {
    UnicodeString Path = FData->RemoteDirectory;
    if (!Path.IsEmpty() && !EndsStr(L"/", Path))
    {
      Path += L"/";
    }
    Result = RtfText(GenerateUrl(Path));
  }
  else
  {
    for (int Index = 0; Index < FPaths->Count; Index++)
    {
      UnicodeString Url = GenerateUrl(FPaths->Strings[Index]);
      Result += RtfText(Url) + RtfPara;
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString TGenerateUrlDialog::CliExeName()
{
  // The command-line client next to the GUI, as installed, else from the
  // search path
  QString Path = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("openscp-cli"));
  return QFileInfo(Path).isExecutable() ? ToUnicodeString(Path) : UnicodeString(L"openscp-cli");
}
//---------------------------------------------------------------------------
UnicodeString TGenerateUrlDialog::GenerateScript(UnicodeString & ScriptDescription)
{
  UnicodeString Result;

  UnicodeString ExeName = CliExeName();
  UnicodeString BaseExeName = ExtractFileBaseName(ExeName);
  UnicodeString OpenCommand = FData->GenerateOpenCommandArgs(true);
  UnicodeString CommandPlaceholder1 = FMTLOAD(GENERATE_URL_COMMAND, (1));
  UnicodeString CommandPlaceholder2 = FMTLOAD(GENERATE_URL_COMMAND, (2));
  // Upstream's GENERATE_URL_WRITABLE_PATH_TO_LOG is a Windows path
  UnicodeString LogPath = ToUnicodeString(tr("/writable/path/to/log/")) + RtfText(BaseExeName + L".log");
  UnicodeString LogParameter =
    RtfCommandlineSwitch(LogSwitch, L"logging") + RtfText(L"=") +
    RtfScriptPlaceholder(L"\"" + LogPath + L"\"");
  UnicodeString IniParameter =
    RtfCommandlineSwitch(IniSwitch, L"configuration") + RtfText(UnicodeString(L"=") + INI_NUL);
  UnicodeString CommandParameter = RtfCommandlineSwitch(COMMAND_SWITCH, L"scripting");

  std::vector<UnicodeString> Commands;
  Commands.push_back(RtfScriptCommand(L"open") + L" " + OpenCommand);
  Commands.push_back(UnicodeString());
  Commands.push_back(L"# " + CommandPlaceholder1);
  Commands.push_back(L"# " + CommandPlaceholder2);
  Commands.push_back(UnicodeString());
  Commands.push_back(RtfScriptCommand(L"exit"));

  int ScriptFormat = ScriptFormatCombo->currentIndex();
  if (ScriptFormat == sfScriptFile)
  {
    for (const UnicodeString & Command : Commands)
    {
      if (!Command.IsEmpty())
      {
        Result += (Command[1] == L'#') ? RtfScriptComment(Command) : Command;
      }
      Result += RtfPara;
    }

    UnicodeString ScriptCommandLine =
      FORMAT(L"\"%s\" /%s=\"%s\" /%s=%s /%s=\"%s\"",
        (ExeName, LogSwitch, LogPath, IniSwitch, INI_NUL, UnicodeString(SCRIPT_SWITCH),
         ToUnicodeString(tr("/path/to/script/script.txt"))));
    Result +=
      RtfPara +
      RtfScriptComment(L"# " + LoadStr(GENERATE_URL_SCRIPT_DESC)) + RtfPara +
      RtfScriptComment(L"# " + ScriptCommandLine) + RtfPara;
  }
  else if (ScriptFormat == sfBatchFile)
  {
    // A shell script (upstream's batch file)
    Result =
      RtfScriptPlaceholder(L"#!/bin/sh") + RtfPara +
      RtfPara +
      RtfText(L"\"" + ExeName + L"\" \\") + RtfPara +
      RtfText(L"  ") + LogParameter + L" " + IniParameter + RtfText(L" \\") + RtfPara +
      RtfText(L"  ") + CommandParameter;

    for (UnicodeString Command : Commands)
    {
      if (!Command.IsEmpty())
      {
        Result += RtfText(L" \\") + RtfPara + RtfText(L"    \"");
        if (Command[1] == L'#')
        {
          Command.Delete(1, 1);
          Result += RtfScriptPlaceholder(Command.TrimLeft());
        }
        else
        {
          Result += RtfShellEscape(Command);
        }
        Result += L"\"";
      }
    }

    Result +=
      RtfPara +
      RtfPara +
      RtfText(L"result=$?") + RtfPara +
      RtfKeyword(L"if") + RtfText(L" [ $result -eq 0 ]; ") + RtfKeyword(L"then") + RtfPara +
      RtfText(L"  ") + RtfKeyword(L"echo") + RtfText(L" Success") + RtfPara +
      RtfKeyword(L"else") + RtfPara +
      RtfText(L"  ") + RtfKeyword(L"echo") + RtfText(L" Error") + RtfPara +
      RtfKeyword(L"fi") + RtfPara +
      RtfPara +
      RtfKeyword(L"exit") + RtfText(L" $result") + RtfPara;
  }
  else if (ScriptFormat == sfCommandLine)
  {
    Result =
      LogParameter + L" " +
      IniParameter + L" " +
      CommandParameter;

    for (UnicodeString Command : Commands)
    {
      if (!Command.IsEmpty())
      {
        Result += RtfText(L" \"");
        if (Command[1] == L'#')
        {
          Command.Delete(1, 1);
          Result += RtfScriptPlaceholder(Command.TrimLeft());
        }
        else
        {
          Result += RtfShellEscape(Command);
        }
        Result += L"\"";
      }
    }
    ScriptDescription = FMTLOAD(GENERATE_URL_COMMANDLINE_DESC, (FORMAT(L"\"%s\"", (ExeName)))) + L"\n";
  }

  return Result;
}
//---------------------------------------------------------------------------
void TGenerateUrlDialog::UpdateControls()
{
  if (FChanging)
  {
    return;
  }
  setWindowTitle(ToQString(LoadStr(IsFileUrl() ? GENERATE_URL_FILE_TITLE : GENERATE_URL_SESSION_TITLE)));
  OptionsPageControl->setTabVisible(OptionsPageControl->indexOf(ScriptSheet), !IsFileUrl());

  bool HostKeyUnknown = FData->UsesSsh && FData->HostKey.IsEmpty();
  bool Url = (OptionsPageControl->currentWidget() == UrlSheet);

  UnicodeString ResultGroupCaption;
  if (Url)
  {
    ResultGroupCaption = LoadStr(GENERATE_URL_URL);
  }
  else if (ScriptFormatCombo->currentIndex() == sfCommandLine)
  {
    ResultGroupCaption = LoadStr(GENERATE_URL_COMMANDLINE_LABEL);
  }
  else
  {
    ResultGroupCaption = ToUnicodeString(ScriptFormatCombo->currentText());
  }
  ResultGroup->setTitle(ToQString(ResultGroupCaption));

  UserNameCheck->setEnabled(!FData->UserNameExpanded.IsEmpty());
  bool UserNameIncluded = UserNameCheck->isEnabled() && UserNameCheck->isChecked();
  PasswordCheck->setEnabled(UserNameIncluded && FData->HasPassword());
  HostKeyCheck->setEnabled(UserNameIncluded && !FData->HostKey.IsEmpty());
  RemoteDirectoryCheck->setEnabled(!FData->RemoteDirectory.IsEmpty() && !IsFileUrl());
  SaveExtensionCheck->setEnabled(!IsFileUrl());
  RawSettingsCheck->setEnabled(UserNameIncluded && FData->HasRawSettingsForUrl());

  bool WordWrap;
  if (Url)
  {
    if (!FUrlCounted)
    {
      FUrlCounted = true;
      Configuration->Usage->Inc(L"GeneratedUrls");
    }
    FResult = GenerateUrl();
    WordWrap = true;
  }
  else
  {
    if (!FScriptCounted)
    {
      FScriptCounted = true;
      Configuration->Usage->Inc(L"GeneratedScripts");
    }
    UnicodeString ScriptDescription;
    if (HostKeyUnknown)
    {
      ScriptDescription += LoadStr(GENERATE_URL_HOSTKEY_UNKNOWN) + L"\n";
    }
    FResult = GenerateScript(ScriptDescription);
    ScriptDescriptionLabel->setText(ToQString(ScriptDescription).trimmed());
    WordWrap = (ScriptFormatCombo->currentIndex() == sfCommandLine);
  }

  // The colors of upstream's table (the same as on its wiki)
  UnicodeString Rtf =
    UnicodeString(L"{\\rtf1{\\colortbl ;") +
    L"\\red1\\green1\\blue1;" + // near-black (links without their style)
    L"\\red0\\green128\\blue0;" + // code comment (green)
    L"\\red0\\green128\\blue128;" + // class (teal)
    L"\\red128\\green0\\blue0;" + // string (maroon)
    L"\\red0\\green0\\blue255;" + // keyword (blue)
    L"\\red153\\green51\\blue51;" + // command-line argument (reddish)
    L"\\red128\\green128\\blue128;" + // script command (gray)
    L"}" + FResult + L"}";
  ResultMemo->setLineWrapMode(WordWrap ? QTextEdit::WidgetWidth : QTextEdit::NoWrap);
  ResultMemo->setFont(WordWrap ? font() : QFontDatabase::systemFont(QFontDatabase::FixedFont));
  ResultMemo->setHtml(RtfToHtml(Rtf));
  FResult = Rtf;
}
//---------------------------------------------------------------------------
void TGenerateUrlDialog::Execute()
{
  int Components = AppConfiguration->GenerateUrlComponents;
  if (Components < 0)
  {
    // The username and the initial directory by default
    Components = 1 | 8;
  }
  TGenerateUrlCodeTarget Target = AppConfiguration->GenerateUrlCodeTarget;

  {
    TAutoFlag ChangingFlag(FChanging);
    OptionsPageControl->setCurrentWidget((IsFileUrl() || (Target != guctScript)) ? UrlSheet : ScriptSheet);
    for (auto & Check : FComponentChecks)
    {
      Check.first->setChecked(FLAGSET(Components, Check.second));
    }
    int ScriptFormat = AppConfiguration->GenerateUrlScriptFormat;
    ScriptFormatCombo->setCurrentIndex(
      ((ScriptFormat >= 0) && (ScriptFormat < ScriptFormatCombo->count())) ? ScriptFormat : sfScriptFile);
  }

  UpdateControls();
  if (OptionsPageControl->currentWidget() != UrlSheet)
  {
    resize(fontMetrics().averageCharWidth() * 110, fontMetrics().height() * 36);
  }

  exec();

  // Not saved for the files, as their URL was selected implicitly
  if (!IsFileUrl())
  {
    Target = (OptionsPageControl->currentWidget() == UrlSheet) ? guctUrl : guctScript;
    AppConfiguration->GenerateUrlCodeTarget = Target;
  }

  if (Target == guctUrl)
  {
    Components = 0;
    for (auto & Check : FComponentChecks)
    {
      if (Check.first->isChecked())
      {
        Components |= Check.second;
      }
    }
    AppConfiguration->GenerateUrlComponents = Components;
  }
  else
  {
    AppConfiguration->GenerateUrlScriptFormat = static_cast<TScriptFormat>(ScriptFormatCombo->currentIndex());
  }
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
void DoGenerateUrlDialog(TSessionData * Data, TStrings * Paths)
{
  TGenerateUrlDialog Dialog(Data, Paths);
  Dialog.Execute();
}
//---------------------------------------------------------------------------
