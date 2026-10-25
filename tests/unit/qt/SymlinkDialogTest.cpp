// L3 tests of the dialog of the links (dialogs/SymlinkDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <QtInterface.h>

#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(SymlinkDialogTest, AddLink)
{
  UnicodeString FileName;
  UnicodeString PointTo = L"target.txt";
  bool SymbolicLink = true;
  WhenModal([](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("SymlinkDialog"));
    EXPECT_EQ(Dialog->windowTitle(), QStringLiteral("Add link/shortcut"));
    EXPECT_EQ(Child<QLineEdit>(Dialog, "PointToEdit")->text(), QStringLiteral("target.txt"));
    // The name is needed
    QPushButton * OkButton = Child<QPushButton>(Dialog, "OkButton");
    EXPECT_FALSE(OkButton->isEnabled());
    QLineEdit * FileNameEdit = Child<QLineEdit>(Dialog, "FileNameEdit");
    EXPECT_FALSE(FileNameEdit->isReadOnly());
    FileNameEdit->setText(QStringLiteral("link"));
    EXPECT_TRUE(OkButton->isEnabled());
    QCheckBox * HardLinkCheck = Child<QCheckBox>(Dialog, "HardLinkCheck");
    EXPECT_TRUE(HardLinkCheck->isEnabled());
    HardLinkCheck->setChecked(true);
    OkButton->click();
  });
  EXPECT_TRUE(DoSymlinkDialog(FileName, PointTo, SymbolicLink, false, true));
  EXPECT_EQ(FileName, UnicodeString(L"link"));
  EXPECT_EQ(PointTo, UnicodeString(L"target.txt"));
  EXPECT_FALSE(SymbolicLink);
}
//---------------------------------------------------------------------------
TEST(SymlinkDialogTest, EditLink)
{
  UnicodeString FileName = L"link";
  UnicodeString PointTo = L"old";
  bool SymbolicLink = true;
  WhenModal([](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->windowTitle(), QStringLiteral("Edit link/shortcut"));
    // Only the target of the link is edited
    EXPECT_TRUE(Child<QLineEdit>(Dialog, "FileNameEdit")->isReadOnly());
    EXPECT_FALSE(Child<QCheckBox>(Dialog, "HardLinkCheck")->isEnabled());
    Child<QLineEdit>(Dialog, "PointToEdit")->setText(QStringLiteral("new"));
    Child<QPushButton>(Dialog, "OkButton")->click();
  });
  EXPECT_TRUE(DoSymlinkDialog(FileName, PointTo, SymbolicLink, true, true));
  EXPECT_EQ(FileName, UnicodeString(L"link"));
  EXPECT_EQ(PointTo, UnicodeString(L"new"));
  EXPECT_TRUE(SymbolicLink);

  // Cancelled, nothing changes
  WhenModal([](QWidget * Dialog)
  {
    Child<QLineEdit>(Dialog, "PointToEdit")->setText(QStringLiteral("other"));
    Child<QPushButton>(Dialog, "CancelButton")->click();
  });
  EXPECT_FALSE(DoSymlinkDialog(FileName, PointTo, SymbolicLink, true, false));
  EXPECT_EQ(PointTo, UnicodeString(L"new"));
}
//---------------------------------------------------------------------------
