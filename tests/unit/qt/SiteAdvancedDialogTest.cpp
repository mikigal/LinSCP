// L3 tests of the advanced settings of the sites
// (dialogs/SiteAdvancedDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <QtInterface.h>
#include <SessionData.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRadioButton>
#include <QSpinBox>
#include <QToolButton>
#include <QTreeWidget>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QStringList VisiblePages(QWidget * Dialog)
{
  QStringList Result;
  QTreeWidget * Tree = Child<QTreeWidget>(Dialog, "NavigationTree");
  for (QTreeWidgetItemIterator It(Tree); *It != nullptr; ++It)
  {
    if (!(*It)->isHidden())
    {
      Result.append((*It)->text(0));
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
void SelectPage(QWidget * Dialog, const QString & Caption)
{
  QTreeWidget * Tree = Child<QTreeWidget>(Dialog, "NavigationTree");
  QList<QTreeWidgetItem *> Items = Tree->findItems(Caption, Qt::MatchExactly | Qt::MatchRecursive);
  ASSERT_EQ(Items.size(), 1);
  Tree->setCurrentItem(Items[0]);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(SiteAdvancedDialogTest, SftpSettings)
{
  TSessionData Data(L"site");
  Data.FSProtocol = fsSFTP;
  Data.HostName = L"example.com";
  Data.RemoteDirectory = L"/var/www";
  Data.ProxyMethod = pmSocks5;
  Data.ProxyHost = L"proxy";
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("SiteAdvancedDialog"));
    QStringList Pages = VisiblePages(Dialog);
    // The pages of SSH and SFTP only
    for (const char * Page : { "Environment", "Directories", "SFTP", "Shell", "Connection", "Proxy", "Tunnel", "SSH",
                               "Key exchange", "Authentication", "Bugs", "Note" })
    {
      EXPECT_TRUE(Pages.contains(QString::fromLatin1(Page))) << Page;
    }
    for (const char * Page : { "FTP", "S3", "WebDAV", "TLS/SSL" })
    {
      EXPECT_FALSE(Pages.contains(QString::fromLatin1(Page))) << Page;
    }
    EXPECT_EQ(Child<QLineEdit>(Dialog, "RemoteDirectoryEdit")->text(), QStringLiteral("/var/www"));
    EXPECT_TRUE(Child<QCheckBox>(Dialog, "AllowScpFallbackCheck")->isChecked());
    EXPECT_EQ(Child<QComboBox>(Dialog, "SshProxyMethodCombo")->currentText(), QStringLiteral("SOCKS5"));
    EXPECT_TRUE(Child<QLineEdit>(Dialog, "ProxyPasswordEdit")->isEnabled());
    EXPECT_TRUE(Child<QGroupBox>(Dialog, "FtpPingGroup")->isHidden());

    Child<QLineEdit>(Dialog, "RemoteDirectoryEdit")->setText(QStringLiteral("/srv"));
    Child<QCheckBox>(Dialog, "AllowScpFallbackCheck")->setChecked(false);
    Child<QCheckBox>(Dialog, "CompressionCheck")->setChecked(true);
    // The ciphers reordered
    QListWidget * CipherListBox = Child<QListWidget>(Dialog, "CipherListBox");
    CipherListBox->setCurrentRow(1);
    QString Second = CipherListBox->item(1)->text();
    Child<QPushButton>(Dialog, "CipherUpButton")->click();
    EXPECT_EQ(CipherListBox->item(0)->text(), Second);
    Child<QComboBox>(Dialog, "SshProxyMethodCombo")->setCurrentIndex(pmNone);
    EXPECT_FALSE(Child<QLineEdit>(Dialog, "ProxyHostEdit")->isEnabled());
    Child<QSpinBox>(Dialog, "TimeoutEdit")->setValue(30);
    Child<QRadioButton>(Dialog, "IPv4Button")->setChecked(true);
    Child<QPlainTextEdit>(Dialog, "NoteMemo")->setPlainText(QStringLiteral("A note"));
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  TCipher First = Data.Cipher[1];
  EXPECT_TRUE(DoSiteAdvancedDialog(&Data));
  EXPECT_EQ(Data.RemoteDirectory, UnicodeString(L"/srv"));
  EXPECT_EQ(Data.FSProtocol, fsSFTPonly);
  EXPECT_TRUE(Data.Compression);
  EXPECT_EQ(Data.Cipher[0], First);
  EXPECT_EQ(Data.ProxyMethod, pmNone);
  EXPECT_EQ(Data.Timeout, 30);
  EXPECT_EQ(Data.AddressFamily, afIPv4);
  EXPECT_EQ(Data.Note, UnicodeString(L"A note"));
}
//---------------------------------------------------------------------------
TEST(SiteAdvancedDialogTest, FtpSettings)
{
  TSessionData Data(L"site");
  Data.FSProtocol = fsFTP;
  Data.Ftps = ftpsExplicitTls;
  Data.HostName = L"example.com";
  WhenModal([&](QWidget * Dialog)
  {
    QStringList Pages = VisiblePages(Dialog);
    for (const char * Page : { "FTP", "TLS/SSL", "Proxy" })
    {
      EXPECT_TRUE(Pages.contains(QString::fromLatin1(Page))) << Page;
    }
    for (const char * Page : { "SFTP", "SSH", "Tunnel", "Encryption" })
    {
      EXPECT_FALSE(Pages.contains(QString::fromLatin1(Page))) << Page;
    }
    EXPECT_FALSE(Child<QGroupBox>(Dialog, "FtpPingGroup")->isHidden());
    EXPECT_TRUE(Child<QGroupBox>(Dialog, "PingGroup")->isHidden());
    Child<QPlainTextEdit>(Dialog, "PostLoginCommandsMemo")->setPlainText(QStringLiteral("SITE CHMOD"));
    // A proxy of FTP needs the passive mode
    Child<QCheckBox>(Dialog, "FtpPasvModeCheck")->setChecked(false);
    WhenModal("MessageForm", [](QWidget * Message) { Message->close(); });
    Child<QComboBox>(Dialog, "FtpProxyMethodCombo")->setCurrentIndex(pmHTTP);
    EXPECT_TRUE(Child<QCheckBox>(Dialog, "FtpPasvModeCheck")->isChecked());
    Child<QComboBox>(Dialog, "MinTlsVersionCombo")->setCurrentIndex(3);
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  EXPECT_TRUE(DoSiteAdvancedDialog(&Data));
  EXPECT_EQ(Data.PostLoginCommands, UnicodeString(L"SITE CHMOD"));
  EXPECT_EQ(Data.ProxyMethod, pmHTTP);
  EXPECT_TRUE(Data.FtpPasvMode);
  EXPECT_EQ(Data.MinTlsVersion, tls13);
}
//---------------------------------------------------------------------------
TEST(SiteAdvancedDialogTest, EncryptionAndColor)
{
  TSessionData Data(L"site");
  Data.FSProtocol = fsSFTPonly;
  Data.HostName = L"example.com";
  bool Rejected = false;
  WhenModal([&](QWidget * Dialog)
  {
    SelectPage(Dialog, QStringLiteral("Encryption"));
    Child<QCheckBox>(Dialog, "EncryptFilesCheck")->setChecked(true);
    Child<QLineEdit>(Dialog, "EncryptKeyPasswordEdit")->setText(QStringLiteral("1234"));
    // An invalid key keeps the dialog open
    WhenModal("MessageForm", [&](QWidget * Message)
    {
      Rejected = true;
      Message->close();
      WhenModal("SiteAdvancedDialog", [](QWidget * ADialog)
      {
        // A generated key
        WhenModal("MessageForm", [](QWidget * Generated)
        {
          Generated->close();
          WhenModal("SiteAdvancedDialog", [](QWidget * BDialog)
          {
            WhenPopup([BDialog](QWidget * Popup)
            {
              Child<QToolButton>(Popup, "ColorCC99FF")->click();
              Child<QPushButton>(BDialog, "OKBtn")->click();
            });
            Child<QPushButton>(BDialog, "ColorButton")->click();
          });
        });
        Child<QPushButton>(ADialog, "GenerateKeyButton")->click();
      });
    });
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  EXPECT_TRUE(DoSiteAdvancedDialog(&Data));
  EXPECT_TRUE(Rejected);
  EXPECT_EQ(Data.EncryptKey.Length(), 64);
  EXPECT_EQ(Data.Color, 0xCC99FF);
}
//---------------------------------------------------------------------------
TEST(SiteAdvancedDialogTest, RawSettings)
{
  TSessionData Data(L"site");
  Data.FSProtocol = fsSFTPonly;
  Data.HostName = L"example.com";
  Data.UserName = L"user";
  Data.Compression = true;
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("SiteRawDialog"));
    QPlainTextEdit * SettingsMemo = Child<QPlainTextEdit>(Dialog, "SettingsMemo");
    // The settings other than the defaults, not the basic ones
    QString Text = SettingsMemo->toPlainText();
    EXPECT_TRUE(Text.contains(QStringLiteral("Compression=1"))) << Text.toStdString();
    EXPECT_FALSE(Text.contains(QStringLiteral("example.com")));
    // A setting added from the list
    WhenModal("SiteRawAddDialog", [](QWidget * AddDialog)
    {
      QComboBox * AddComboBox = Child<QComboBox>(AddDialog, "AddComboBox");
      int Index = AddComboBox->findText(QStringLiteral("Timeout"));
      ASSERT_GE(Index, 0);
      AddComboBox->setCurrentIndex(Index);
      Child<QPushButton>(AddDialog, "OKButton")->click();
    });
    Child<QPushButton>(Dialog, "AddButton")->click();
    EXPECT_TRUE(SettingsMemo->toPlainText().contains(QStringLiteral("Timeout=")));
    SettingsMemo->setPlainText(SettingsMemo->toPlainText().replace(QRegularExpression(QStringLiteral("Timeout=\\d+")), QStringLiteral("Timeout=33")));
    Child<QPushButton>(Dialog, "OKButton")->click();
  });
  DoSiteRawDialog(&Data);
  EXPECT_EQ(Data.Timeout, 33);
  EXPECT_TRUE(Data.Compression);
  EXPECT_EQ(Data.HostName, UnicodeString(L"example.com"));
  EXPECT_EQ(Data.FSProtocol, fsSFTPonly);
}
//---------------------------------------------------------------------------
