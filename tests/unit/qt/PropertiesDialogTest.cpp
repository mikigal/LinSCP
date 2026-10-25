// L3 tests of the properties dialog (dialogs/PropertiesDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <PropertiesDialog.h>
#include <RightsFrame.h>

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
TRemoteFile * NewFile(const UnicodeString & Name, wchar_t Type, unsigned short Rights, __int64 Size = 0)
{
  TRemoteFile * File = new TRemoteFile();
  File->FileName = Name;
  File->FullFileName = L"/dir/" + Name;
  File->Type = Type;
  File->Size = Size;
  File->Rights->Number = Rights;
  File->Owner = TRemoteToken(L"alice");
  File->Group = TRemoteToken(L"staff");
  return File;
}
//---------------------------------------------------------------------------
class TFiles
{
public:
  TFiles()
  {
    List.reset(new TStringList());
    List->OwnsObjects = true;
  }

  void Add(TRemoteFile * File)
  {
    List->AddObject(File->FullFileName, File);
  }

  std::unique_ptr<TStringList> List;
};
//---------------------------------------------------------------------------
QPushButton * Button(QWidget * Dialog, const char * Name)
{
  return Child<QPushButton>(Dialog, Name);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(PropertiesDialogTest, SingleFileChangesPermissionsAndOwner)
{
  TFiles Files;
  Files.Add(NewFile(L"file.txt", FILETYPE_DEFAULT, 0644, 2048));
  TRemoteTokenList Users;
  Users.Add(TRemoteToken(L"alice"));
  Users.Add(TRemoteToken(L"bob"));
  TRemoteTokenList Groups;
  Groups.Add(TRemoteToken(L"staff"));
  std::unique_ptr<TStrings> Algs(new TStringList());

  TRemoteProperties Properties = TRemoteProperties::CommonProperties(Files.List.get());
  QString Title, FileText, SizeText, Location;
  bool OkInitially = true;
  bool RecursiveVisible = true;
  bool ChecksumEnabled = true;
  WhenModal([&](QWidget * Dialog)
  {
    Title = Dialog->windowTitle();
    FileText = Child<QLineEdit>(Dialog, "FileLabel")->text();
    SizeText = Child<QLineEdit>(Dialog, "SizeLabel")->text();
    Location = Child<QLineEdit>(Dialog, "LocationLabel")->text();
    OkInitially = Button(Dialog, "OkButton")->isEnabled();
    RecursiveVisible = Child<QCheckBox>(Dialog, "RecursiveCheck2")->isVisible();
    QTabWidget * Pages = Child<QTabWidget>(Dialog, "PageControl");
    ChecksumEnabled = Pages->isTabEnabled(Pages->indexOf(Child<QWidget>(Dialog, "ChecksumSheet")));
    Child<QCheckBox>(Dialog, "GroupWriteCheck")->click();
    Child<QComboBox>(Dialog, "OwnerComboBox")->setEditText(QStringLiteral("bob"));
    Button(Dialog, "OkButton")->click();
  });
  bool Result = DoPropertiesDialog(Files.List.get(), L"/dir", &Groups, &Users, Algs.get(), &Properties,
    cpMode | cpOwner | cpGroup, 0, nullptr, nullptr);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Title, QStringLiteral("file.txt Properties"));
  EXPECT_EQ(FileText, QStringLiteral("file.txt"));
  EXPECT_EQ(SizeText, QStringLiteral("2.00 KB (2,048 B)"));
  EXPECT_EQ(Location, QStringLiteral("/dir"));
  EXPECT_FALSE(OkInitially);
  EXPECT_FALSE(RecursiveVisible);
  EXPECT_FALSE(ChecksumEnabled);
  EXPECT_TRUE(Properties.Valid.Contains(vpRights));
  EXPECT_EQ(Properties.Rights.Number, 0664);
  EXPECT_TRUE(Properties.Valid.Contains(vpOwner));
  EXPECT_EQ(Properties.Owner.Name, UnicodeString(L"bob"));
}
//---------------------------------------------------------------------------
TEST(PropertiesDialogTest, MultipleFilesWithDirectoryCalculateSize)
{
  TFiles Files;
  Files.Add(NewFile(L"a.txt", FILETYPE_DEFAULT, 0644, 100));
  Files.Add(NewFile(L"sub", FILETYPE_DIRECTORY, 0755));
  std::unique_ptr<TStrings> Algs(new TStringList());

  TRemoteProperties Properties = TRemoteProperties::CommonProperties(Files.List.get());
  QString Title, FileText, SizeBefore, SizeAfter;
  bool OwnerViewVisible = false;
  bool OwnerComboVisible = true;
  bool RecursiveVisible = false;
  bool Undefined = false;
  int Calculations = 0;
  WhenModal([&](QWidget * Dialog)
  {
    Title = Dialog->windowTitle();
    FileText = Child<QLineEdit>(Dialog, "FileLabel")->text();
    SizeBefore = Child<QLineEdit>(Dialog, "SizeLabel")->text();
    OwnerViewVisible = Child<QLineEdit>(Dialog, "OwnerView")->isVisible();
    OwnerComboVisible = Child<QComboBox>(Dialog, "OwnerComboBox")->isVisible();
    RecursiveVisible = Child<QCheckBox>(Dialog, "RecursiveCheck2")->isVisible();
    // The rights differ: the common ones are undefined
    Undefined = (Child<QCheckBox>(Dialog, "OwnerExecuteCheck")->checkState() == Qt::PartiallyChecked);
    Button(Dialog, "CalculateSizeButton")->click();
    SizeAfter = Child<QLineEdit>(Dialog, "SizeLabel")->text();
    Child<QCheckBox>(Dialog, "RecursiveCheck2")->click();
    Button(Dialog, "OkButton")->click();
  });
  TCalculateSizeEvent CalculateSize = [&](TStrings * FileList, __int64 & Size, TCalculateSizeStats & Stats, bool &)
  {
    Calculations++;
    EXPECT_EQ(FileList->Count, 2);
    Size = 5000;
    Stats.Files = 3;
    Stats.Directories = 2;
  };
  bool Result = DoPropertiesDialog(Files.List.get(), L"/dir", NULL, NULL, Algs.get(), &Properties,
    cpMode, 0, CalculateSize, nullptr);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Title, QStringLiteral("a.txt, ... Properties"));
  EXPECT_EQ(FileText, QStringLiteral("1 file, 1 folder"));
  EXPECT_EQ(SizeBefore, QStringLiteral("Unknown"));
  EXPECT_EQ(SizeAfter, QStringLiteral("4.88 KB (5,000 B)"));
  EXPECT_EQ(Calculations, 1);
  EXPECT_TRUE(OwnerViewVisible);
  EXPECT_FALSE(OwnerComboVisible);
  EXPECT_TRUE(RecursiveVisible);
  EXPECT_TRUE(Undefined);
  EXPECT_TRUE(Properties.Recursive);
  EXPECT_EQ(Properties.Rights.RightUndef[TRights::rrUserExec], TRights::rsUndef);
  EXPECT_EQ(Properties.Rights.RightUndef[TRights::rrUserRead], TRights::rsYes);
}
//---------------------------------------------------------------------------
TEST(PropertiesDialogTest, OwnerByIdIsValidated)
{
  TFiles Files;
  TRemoteFile * File = NewFile(L"file.txt", FILETYPE_DEFAULT, 0644);
  TRemoteToken Owner(L"alice");
  Owner.ID = 1000;
  File->Owner = Owner;
  Files.Add(File);
  TRemoteTokenList Users;
  Users.Add(Owner);
  TRemoteToken Bob(L"bob");
  Bob.ID = 1001;
  Users.Add(Bob);
  std::unique_ptr<TStrings> Algs(new TStringList());

  TRemoteProperties Properties = TRemoteProperties::CommonProperties(Files.List.get());
  QString Initial;
  QString Error;
  WhenModal([&](QWidget * Dialog)
  {
    QComboBox * Combo = Child<QComboBox>(Dialog, "OwnerComboBox");
    Initial = Combo->currentText();
    Combo->setEditText(QStringLiteral("nobody"));
    // The error of an unknown owner, then a valid one
    WhenModal([&](QWidget * Message)
    {
      Error = Child<QLabel>(Message, "MainMessage")->text();
      Child<QPushButton>(Message, "OK")->click();
      Combo->setEditText(QStringLiteral("1001"));
      Button(Dialog, "OkButton")->click();
    });
    Button(Dialog, "OkButton")->click();
  });
  bool Result = DoPropertiesDialog(Files.List.get(), L"/dir", NULL, &Users, Algs.get(), &Properties,
    cpMode | cpOwner, poUserGroupByID, nullptr, nullptr);
  ASSERT_TRUE(Result);
  EXPECT_EQ(Initial, QStringLiteral("alice [1000]"));
  EXPECT_TRUE(Error.contains(QStringLiteral("\"nobody\"")));
  EXPECT_TRUE(Properties.Valid.Contains(vpOwner));
  EXPECT_EQ(Properties.Owner.ID, 1001u);
  EXPECT_EQ(Properties.Owner.Name, UnicodeString(L"bob"));
}
//---------------------------------------------------------------------------
TEST(PropertiesDialogTest, ChecksumOfFile)
{
  TFiles Files;
  Files.Add(NewFile(L"file.txt", FILETYPE_DEFAULT, 0644));
  std::unique_ptr<TStrings> Algs(new TStringList());
  Algs->Add(L"md5");
  Algs->Add(L"sha-1");

  TRemoteProperties Properties = TRemoteProperties::CommonProperties(Files.List.get());
  QString Checksum;
  UnicodeString AlgUsed;
  WhenModal([&](QWidget * Dialog)
  {
    QTabWidget * Pages = Child<QTabWidget>(Dialog, "PageControl");
    Child<QComboBox>(Dialog, "ChecksumAlgEdit")->setCurrentIndex(1);
    Pages->setCurrentWidget(Child<QWidget>(Dialog, "ChecksumSheet"));
    // Calculated once the page shows
    QTest::qWait(50);
    Checksum = Child<QLineEdit>(Dialog, "ChecksumEdit")->text();
    Button(Dialog, "CancelButton")->click();
  });
  TCalculateChecksumEvent CalculateChecksum =
    [&](const UnicodeString & Alg, TStrings * FileList, TCalculatedChecksumEvent OnCalculatedChecksum, bool &)
    {
      AlgUsed = Alg;
      OnCalculatedChecksum(FileList->Strings[0], L"sha-1", L"0123abcd");
    };
  bool Result = DoPropertiesDialog(Files.List.get(), L"/dir", NULL, NULL, Algs.get(), &Properties,
    cpMode, 0, nullptr, CalculateChecksum);
  EXPECT_FALSE(Result);
  EXPECT_EQ(AlgUsed, UnicodeString(L"sha-1"));
  EXPECT_EQ(Checksum, QStringLiteral("0123abcd"));
}
//---------------------------------------------------------------------------
