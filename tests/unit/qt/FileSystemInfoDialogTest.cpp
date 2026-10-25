// L3 tests of the information about the server and the protocol
// (dialogs/FileSystemInfoDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <QtInterface.h>
#include <SessionInfo.h>
#include <Terminal.h>

#include <QClipboard>
#include <QGroupBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTreeWidget>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QString ItemValue(QWidget * Dialog, const char * View, const QString & Item)
{
  QTreeWidget * ListView = Child<QTreeWidget>(Dialog, View);
  QList<QTreeWidgetItem *> Items = ListView->findItems(Item, Qt::MatchExactly);
  return (Items.size() == 1) ? Items[0]->text(1) : QString();
}
//---------------------------------------------------------------------------
void SampleInfo(TSessionInfo & SessionInfo, TFileSystemInfo & FileSystemInfo)
{
  SessionInfo.SecurityProtocolName = L"SSH-2";
  SessionInfo.SshImplementation = L"OpenSSH_9.9";
  SessionInfo.CSCipher = L"AES-256 SDCTR";
  SessionInfo.SCCipher = L"AES-256 SDCTR";
  SessionInfo.CSCompression = L"";
  SessionInfo.SCCompression = L"";
  SessionInfo.HostKeyFingerprintSHA256 = L"ssh-ed25519 255 AbCdEf";
  SessionInfo.HostKeyFingerprintMD5 = L"ssh-ed25519 255 12:34:56";
  FileSystemInfo.ProtocolName = L"SFTP-3";
  FileSystemInfo.RemoteSystem = L"Linux";
  FileSystemInfo.IsCapable[fcModeChanging] = true;
  FileSystemInfo.IsCapable[fcCheckingSpaceAvailable] = true;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(FileSystemInfoDialogTest, ShowsInformation)
{
  TSessionInfo SessionInfo;
  TFileSystemInfo FileSystemInfo;
  SampleInfo(SessionInfo, FileSystemInfo);
  FileSystemInfo.AdditionalInfo = L"Some extensions";
  UnicodeString CheckedPath;
  int Checks = 0;
  TGetSpaceAvailableEvent OnGetSpaceAvailable =
    [&](const UnicodeString & Path, TSpaceAvailable & ASpaceAvailable, bool & /*Close*/)
    {
      CheckedPath = Path;
      Checks++;
      ASpaceAvailable.BytesOnDevice = 10 * 1024 * 1024;
      ASpaceAvailable.BytesAvailableToUser = 2048;
    };
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("FileSystemInfoDialog"));
    EXPECT_EQ(ItemValue(Dialog, "ServerView", QStringLiteral("Remote system")), QStringLiteral("Linux"));
    EXPECT_EQ(ItemValue(Dialog, "ServerView", QStringLiteral("File transfer protocol")), QStringLiteral("SFTP-3"));
    // The same cipher both ways is shown once, no compression
    EXPECT_EQ(ItemValue(Dialog, "ServerView", QStringLiteral("Encryption algorithm")), QStringLiteral("AES-256 SDCTR"));
    EXPECT_EQ(ItemValue(Dialog, "ServerView", QStringLiteral("Compression")), QStringLiteral("No"));
    EXPECT_EQ(Child<QLineEdit>(Dialog, "HostKeyAlgorithmEdit")->text(), QStringLiteral("ssh-ed25519 255"));
    EXPECT_EQ(Child<QLineEdit>(Dialog, "HostKeyFingerprintSHA256Edit")->text(), QStringLiteral("AbCdEf"));
    EXPECT_EQ(Child<QLineEdit>(Dialog, "HostKeyFingerprintMD5Edit")->text(), QStringLiteral("12:34:56"));
    EXPECT_TRUE(Child<QGroupBox>(Dialog, "CertificateGroup")->isHidden());
    EXPECT_FALSE(Child<QGroupBox>(Dialog, "HostKeyGroup")->isHidden());
    EXPECT_EQ(ItemValue(Dialog, "ProtocolView", QStringLiteral("Can change permissions")), QStringLiteral("Yes"));
    EXPECT_EQ(ItemValue(Dialog, "ProtocolView", QStringLiteral("Can execute arbitrary command")), QStringLiteral("No"));
    EXPECT_EQ(Child<QPlainTextEdit>(Dialog, "InfoMemo")->toPlainText(), QStringLiteral("Some extensions"));

    // The space available is checked once its page is shown
    EXPECT_EQ(Checks, 0);
    QTabWidget * PageControl = Child<QTabWidget>(Dialog, "PageControl");
    PageControl->setCurrentWidget(Child<QWidget>(Dialog, "SpaceAvailableSheet"));
    EXPECT_EQ(Checks, 1);
    EXPECT_EQ(CheckedPath, UnicodeString(L"/home/user"));
    EXPECT_EQ(ItemValue(Dialog, "SpaceAvailableView", QStringLiteral("Total bytes on device")),
      QStringLiteral("10.0 MB (10,485,760 B)"));
    EXPECT_EQ(ItemValue(Dialog, "SpaceAvailableView", QStringLiteral("Free bytes on device")), QStringLiteral("Unknown"));
    Child<QLineEdit>(Dialog, "SpaceAvailablePathEdit")->setText(QStringLiteral("/tmp"));
    Child<QPushButton>(Dialog, "SpaceAvailableButton")->click();
    EXPECT_EQ(Checks, 2);
    EXPECT_EQ(CheckedPath, UnicodeString(L"/tmp"));

    Child<QPushButton>(Dialog, "ClipboardButton")->click();
    QString Text = QApplication::clipboard()->text();
    EXPECT_TRUE(Text.contains(QStringLiteral("Remote system = Linux\n"))) << Text.toStdString();
    EXPECT_TRUE(Text.contains(QStringLiteral("Server host key fingerprints\n"))) << Text.toStdString();
    // The fingerprints with their algorithm
    EXPECT_TRUE(Text.contains(QStringLiteral("SHA-256 = ssh-ed25519 255 AbCdEf\n"))) << Text.toStdString();
    EXPECT_FALSE(Text.contains(QStringLiteral("Certificate fingerprint"))) << Text.toStdString();
    EXPECT_TRUE(Text.contains(QStringLiteral("Some extensions\n"))) << Text.toStdString();
    // Already checked, not again
    EXPECT_EQ(Checks, 2);
    Child<QPushButton>(Dialog, "CloseButton")->click();
  });
  DoFileSystemInfoDialog(SessionInfo, FileSystemInfo, L"/home/user", OnGetSpaceAvailable);
}
//---------------------------------------------------------------------------
TEST(FileSystemInfoDialogTest, NoSpaceAvailable)
{
  TSessionInfo SessionInfo;
  TFileSystemInfo FileSystemInfo;
  SampleInfo(SessionInfo, FileSystemInfo);
  SessionInfo.HostKeyFingerprintSHA256 = L"";
  SessionInfo.HostKeyFingerprintMD5 = L"";
  SessionInfo.CertificateFingerprintSHA256 = L"AA:BB";
  SessionInfo.CertificateFingerprintSHA1 = L"CC:DD";
  WhenModal([&](QWidget * Dialog)
  {
    QTabWidget * PageControl = Child<QTabWidget>(Dialog, "PageControl");
    EXPECT_FALSE(PageControl->isTabEnabled(PageControl->indexOf(Child<QWidget>(Dialog, "SpaceAvailableSheet"))));
    EXPECT_TRUE(Child<QGroupBox>(Dialog, "HostKeyGroup")->isHidden());
    EXPECT_FALSE(Child<QGroupBox>(Dialog, "CertificateGroup")->isHidden());
    EXPECT_EQ(Child<QLineEdit>(Dialog, "CertificateFingerprintSha256Edit")->text(), QStringLiteral("AA:BB"));
    EXPECT_TRUE(Child<QGroupBox>(Dialog, "InfoGroup")->isHidden());
    Child<QPushButton>(Dialog, "ClipboardButton")->click();
    QString Text = QApplication::clipboard()->text();
    EXPECT_TRUE(Text.contains(QStringLiteral("Certificate fingerprint\nSHA-256 = AA:BB\nSHA-1 = CC:DD\n"))) << Text.toStdString();
    EXPECT_FALSE(Text.contains(QStringLiteral("Total bytes"))) << Text.toStdString();
    Dialog->close();
  });
  DoFileSystemInfoDialog(SessionInfo, FileSystemInfo, L"/", TGetSpaceAvailableEvent());
}
//---------------------------------------------------------------------------
