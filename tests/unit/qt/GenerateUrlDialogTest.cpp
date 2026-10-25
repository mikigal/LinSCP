// L3 tests of the generated URLs and scripts (dialogs/GenerateUrlDialog.cpp)
// and of the rich text of the core (app/RtfText.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <Common.h>
#include <QtInterface.h>
#include <RtfText.h>
#include <SessionData.h>

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QGroupBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTextBrowser>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(RtfTextTest, Conversion)
{
  UnicodeString Rtf =
    L"{\\rtf1{\\colortbl ;\\red1\\green1\\blue1;\\red0\\green0\\blue255;}"
    L"{\\fonttbl{\\f0\\fnil Mono;}}\\f0\\fs20 " +
    RtfLink(L"scriptcommand_open", RtfColorText(2, L"open")) + L" " + RtfText(L"sftp://a\\b{c}/<x> & ą") +
    RtfPara + RtfColorItalicText(2, L"# note") + L"}";
  EXPECT_EQ(RtfToPlainText(Rtf), UnicodeString(L"open sftp://a\\b{c}/<x> & ą\n# note"));
  QString Html = RtfToHtml(Rtf);
  EXPECT_TRUE(Html.contains(QStringLiteral("<a href=\"scriptcommand_open\""))) << Html.toStdString();
  EXPECT_TRUE(Html.contains(QStringLiteral("color:#0000ff;\">open</span></a>"))) << Html.toStdString();
  EXPECT_TRUE(Html.contains(QStringLiteral("sftp://a\\b{c}/&lt;x&gt; &amp; ą<br>"))) << Html.toStdString();
  EXPECT_TRUE(Html.contains(QStringLiteral("font-style:italic;\"># note</span>"))) << Html.toStdString();
  EXPECT_FALSE(Html.contains(QStringLiteral("Mono")));
}
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
void SampleSession(TSessionData & Data)
{
  Data.FSProtocol = fsSFTP;
  Data.HostName = L"example.com";
  Data.UserName = L"user";
  Data.Password = L"pa\"$s";
  Data.RemoteDirectory = L"/home/user";
  Data.HostKey = L"ssh-ed25519 255 AbCd";
}
//---------------------------------------------------------------------------
QString Result(QWidget * Dialog)
{
  return Child<QTextBrowser>(Dialog, "ResultMemo")->toPlainText();
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(GenerateUrlDialogTest, SessionUrlAndScripts)
{
  TSessionData Data(L"site");
  SampleSession(Data);
  AppConfiguration->GenerateUrlComponents = -1;
  AppConfiguration->GenerateUrlCodeTarget = guctUrl;
  AppConfiguration->GenerateUrlScriptFormat = sfScriptFile;
  WhenModal([](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("GenerateUrlDialog"));
    EXPECT_EQ(Dialog->windowTitle(), QStringLiteral("Generate session URL/code"));
    // The username and the directory by default
    EXPECT_EQ(Result(Dialog), QStringLiteral("sftp://user@example.com/home/user/"));
    QCheckBox * PasswordCheck = Child<QCheckBox>(Dialog, "PasswordCheck");
    EXPECT_TRUE(PasswordCheck->isEnabled());
    PasswordCheck->setChecked(true);
    Child<QCheckBox>(Dialog, "RemoteDirectoryCheck")->setChecked(false);
    EXPECT_EQ(Result(Dialog), QStringLiteral("sftp://user:pa%22%24s@example.com/"));
    // Without the username, neither the password
    Child<QCheckBox>(Dialog, "UserNameCheck")->setChecked(false);
    EXPECT_FALSE(PasswordCheck->isEnabled());
    EXPECT_EQ(Result(Dialog), QStringLiteral("sftp://example.com/"));
    Child<QCheckBox>(Dialog, "UserNameCheck")->setChecked(true);

    // The scripts
    QTabWidget * Pages = Child<QTabWidget>(Dialog, "OptionsPageControl");
    Pages->setCurrentWidget(Child<QWidget>(Dialog, "ScriptSheet"));
    QString Script = Result(Dialog);
    EXPECT_TRUE(Script.startsWith(QStringLiteral(
      "open sftp://user:pa%22%24s@example.com/ -hostkey=\"ssh-ed25519 255 AbCd\"\n\n# Your command 1\n# Your command 2\n\nexit\n")))
      << Script.toStdString();
    EXPECT_TRUE(Script.contains(QStringLiteral("/ini=nul /script=\"/path/to/script/script.txt\""))) << Script.toStdString();
    EXPECT_EQ(Child<QGroupBox>(Dialog, "ResultGroup")->title(), QStringLiteral("Script file"));

    QComboBox * Format = Child<QComboBox>(Dialog, "ScriptFormatCombo");
    Format->setCurrentIndex(sfBatchFile);
    Script = Result(Dialog);
    EXPECT_TRUE(Script.startsWith(QStringLiteral("#!/bin/sh\n\n"))) << Script.toStdString();
    // The quotes of the shell escaped
    EXPECT_TRUE(Script.contains(QStringLiteral(
      "  /command \\\n    \"open sftp://user:pa%22%24s@example.com/ -hostkey=\\\"ssh-ed25519 255 AbCd\\\"\" \\\n"
      "    \"Your command 1\" \\\n    \"Your command 2\" \\\n    \"exit\"\n\nresult=$?\n"))) << Script.toStdString();

    Format->setCurrentIndex(sfCommandLine);
    Script = Result(Dialog);
    EXPECT_TRUE(Script.startsWith(QStringLiteral("/log=\"/writable/path/to/log/openscp-cli.log\" /ini=nul /command \"open ")))
      << Script.toStdString();
    EXPECT_EQ(Child<QGroupBox>(Dialog, "ResultGroup")->title(), QStringLiteral("Command-line arguments"));

    Child<QPushButton>(Dialog, "ClipboardButton")->click();
    EXPECT_EQ(QApplication::clipboard()->text(), Script);
    Child<QPushButton>(Dialog, "CancelBtn")->click();
  });
  DoGenerateUrlDialog(&Data, NULL);
  // The last page and its format are remembered
  EXPECT_EQ(AppConfiguration->GenerateUrlCodeTarget, guctScript);
  EXPECT_EQ(AppConfiguration->GenerateUrlScriptFormat, sfCommandLine);
}
//---------------------------------------------------------------------------
TEST(GenerateUrlDialogTest, FileUrls)
{
  TSessionData Data(L"site");
  SampleSession(Data);
  AppConfiguration->GenerateUrlComponents = 1;
  std::unique_ptr<TStrings> Paths(new TStringList());
  Paths->Add(L"/home/user/a b.txt");
  Paths->Add(L"/home/user/c.txt");
  WhenModal([](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->windowTitle(), QStringLiteral("Generate file URL"));
    QTabWidget * Pages = Child<QTabWidget>(Dialog, "OptionsPageControl");
    EXPECT_FALSE(Pages->isTabVisible(Pages->indexOf(Child<QWidget>(Dialog, "ScriptSheet"))));
    EXPECT_FALSE(Child<QCheckBox>(Dialog, "RemoteDirectoryCheck")->isEnabled());
    EXPECT_EQ(Result(Dialog),
      QStringLiteral("sftp://user@example.com/home/user/a%20b.txt\nsftp://user@example.com/home/user/c.txt\n"));
    Child<QPushButton>(Dialog, "CancelBtn")->click();
  });
  DoGenerateUrlDialog(&Data, Paths.get());
}
//---------------------------------------------------------------------------
