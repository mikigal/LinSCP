// L3 tests of the About dialog (dialogs/AboutDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <QtInterface.h>

#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTextBrowser>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(AboutDialogTest, ApplicationAndComponents)
{
  bool License = false;
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("AboutDialog"));
    EXPECT_EQ(Child<QLabel>(Dialog, "ApplicationLabel")->text(), QStringLiteral("OpenSCP"));
    EXPECT_TRUE(Child<QLabel>(Dialog, "VersionLabel")->text().startsWith(QStringLiteral("Version ")));
    EXPECT_TRUE(Child<QLabel>(Dialog, "WinSCPCopyrightLabel")->text().contains(QStringLiteral("Martin Prikryl")));
    EXPECT_TRUE(Child<QLabel>(Dialog, "HomepageLabel")->text().contains(QStringLiteral("https://github.com/mikigal/openscp")));
    QString ThirdParty = Child<QTextBrowser>(Dialog, "ThirdPartyPanel")->toPlainText();
    for (const char * Component : { "PuTTY", "OpenSSL", "FileZilla", "neon", "libs3", "eXpat", "Qt " })
    {
      EXPECT_TRUE(ThirdParty.contains(QString::fromLatin1(Component))) << Component;
    }
    // The license of the source tree
    WhenModal("LicenseDialog", [&](QWidget * LicenseDialog)
    {
      License = Child<QPlainTextEdit>(LicenseDialog, "LicenseMemo")->toPlainText().contains(
        QStringLiteral("GNU GENERAL PUBLIC LICENSE"));
      Child<QPushButton>(LicenseDialog, "CloseButton")->click();
      WhenModal("AboutDialog", [](QWidget * ADialog) { Child<QPushButton>(ADialog, "OKButton")->click(); });
    });
    Child<QPushButton>(Dialog, "LicenseButton")->click();
  });
  DoAboutDialog();
  EXPECT_TRUE(License);
}
//---------------------------------------------------------------------------
