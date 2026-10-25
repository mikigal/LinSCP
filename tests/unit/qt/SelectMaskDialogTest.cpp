// L3 tests of the dialog of the file masks (dialogs/SelectMaskDialog.cpp)
// and of the color menu (dialogs/ColorMenu.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <QtInterface.h>

#include <QAction>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class SelectMaskDialogTest : public testing::Test
{
protected:
  void TearDown() override
  {
    AppConfiguration->SelectMask = AnyMask;
    AppConfiguration->SelectDirectories = false;
    AppConfiguration->FontColors = L"";
    AppConfiguration->SessionColors = L"";
  }
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST_F(SelectMaskDialogTest, SelectsWithTheLastMask)
{
  AppConfiguration->SelectMask = L"*.txt";
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("SelectMaskDialog"));
    QComboBox * MaskEdit = Child<QComboBox>(Dialog, "MaskEdit");
    EXPECT_EQ(MaskEdit->currentText(), QStringLiteral("*.txt"));
    EXPECT_FALSE(Child<QPushButton>(Dialog, "ColorButton")->isVisibleTo(Dialog));
    EXPECT_TRUE(Child<QCheckBox>(Dialog, "ApplyToDirectoriesCheck")->isVisibleTo(Dialog));
    MaskEdit->setEditText(QStringLiteral("*.log"));
    Child<QCheckBox>(Dialog, "ApplyToDirectoriesCheck")->setChecked(true);
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  TFileFilter Filter;
  EXPECT_TRUE(DoSelectMaskDialog(nullptr, true, Filter));
  EXPECT_EQ(Filter.Masks, UnicodeString(L"*.log"));
  EXPECT_TRUE(Filter.Directories);
  // The next selection starts with it
  EXPECT_EQ(AppConfiguration->SelectMask, UnicodeString(L"*.log"));
  EXPECT_TRUE(AppConfiguration->SelectDirectories);
}
//---------------------------------------------------------------------------
TEST_F(SelectMaskDialogTest, InvalidMaskIsNotAccepted)
{
  bool Rejected = false;
  WhenModal([&](QWidget * Dialog)
  {
    Child<QComboBox>(Dialog, "MaskEdit")->setEditText(QStringLiteral("*.txt>abc"));
    // The error of the mask
    WhenModal("MessageForm", [&](QWidget * Message)
    {
      Rejected = true;
      Message->close();
      WhenModal("SelectMaskDialog", [&](QWidget * ADialog)
      {
        Child<QPushButton>(ADialog, "CancelBtn")->click();
      });
    });
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  TFileFilter Filter;
  EXPECT_FALSE(DoSelectMaskDialog(nullptr, false, Filter));
  EXPECT_TRUE(Rejected);
}
//---------------------------------------------------------------------------
TEST_F(SelectMaskDialogTest, FilterIsCleared)
{
  WhenModal([&](QWidget * Dialog)
  {
    EXPECT_FALSE(Child<QCheckBox>(Dialog, "ApplyToDirectoriesCheck")->isVisibleTo(Dialog));
    EXPECT_EQ(Child<QComboBox>(Dialog, "MaskEdit")->currentText(), QStringLiteral("*.c"));
    Child<QPushButton>(Dialog, "ClearButton")->click();
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  UnicodeString Mask = L"*.c";
  EXPECT_TRUE(DoFilterMaskDialog(nullptr, Mask));
  EXPECT_TRUE(Mask.IsEmpty());
}
//---------------------------------------------------------------------------
TEST_F(SelectMaskDialogTest, FileColorFromThePalette)
{
  TFileColorData FileColor;
  FileColor.FileMask = L"*.txt";
  WhenModal([&](QWidget * Dialog)
  {
    QPushButton * ColorButton = Child<QPushButton>(Dialog, "ColorButton");
    EXPECT_TRUE(ColorButton->isVisibleTo(Dialog));
    QComboBox * MaskEdit = Child<QComboBox>(Dialog, "MaskEdit");
    // The color needs a mask
    MaskEdit->setEditText(QString());
    EXPECT_FALSE(Child<QPushButton>(Dialog, "OKBtn")->isEnabled());
    MaskEdit->setEditText(QStringLiteral("*.log"));
    EXPECT_TRUE(Child<QPushButton>(Dialog, "OKBtn")->isEnabled());
    WhenPopup([&](QWidget * Popup)
    {
      EXPECT_EQ(Popup->objectName(), QStringLiteral("ColorMenu"));
      // Blue of the fonts' palette
      Child<QToolButton>(Popup, "ColorFF0000")->click();
      Child<QPushButton>(Dialog, "OKBtn")->click();
    });
    ColorButton->click();
  });
  EXPECT_TRUE(DoFileColorDialog(FileColor));
  EXPECT_EQ(FileColor.FileMask.Masks, UnicodeString(L"*.log"));
  EXPECT_EQ(FileColor.Color, TColor(0xFF0000));
}
//---------------------------------------------------------------------------
TEST_F(SelectMaskDialogTest, ColorMenu)
{
  // The default color
  TColor Color = TColor(0x0000FF);
  WhenPopup([&](QWidget * Popup)
  {
    QAction * DefaultItem = Popup->findChild<QAction *>(QStringLiteral("ColorDefaultItem"));
    ASSERT_NE(DefaultItem, nullptr);
    EXPECT_FALSE(DefaultItem->isChecked());
    // The color is checked in the palette
    EXPECT_TRUE(Child<QToolButton>(Popup, "Color0000FF")->isChecked());
    static_cast<QMenu *>(Popup)->setActiveAction(DefaultItem);
    QTest::keyClick(Popup, Qt::Key_Return);
  });
  EXPECT_TRUE(ColorMenuPopup(QPoint(10, 10), Color, false));
  EXPECT_EQ(Color, TColor(0));

  // Any color, remembered with the custom colors
  WhenPopup([&](QWidget * Popup)
  {
    QAction * PickItem = Popup->findChild<QAction *>(QStringLiteral("ColorPickItem"));
    ASSERT_NE(PickItem, nullptr);
    WhenModal("ColorDialog", [&](QWidget * Dialog)
    {
      QColorDialog * ColorDialog = qobject_cast<QColorDialog *>(Dialog);
      ColorDialog->setCurrentColor(QColor(0x12, 0x34, 0x56));
      ColorDialog->accept();
    });
    static_cast<QMenu *>(Popup)->setActiveAction(PickItem);
    QTest::keyClick(Popup, Qt::Key_Return);
  });
  EXPECT_TRUE(ColorMenuPopup(QPoint(10, 10), Color, true));
  EXPECT_EQ(Color, TColor(0x563412));
  EXPECT_EQ(AppConfiguration->SessionColors, UnicodeString(L"563412"));

  // Offered next time
  WhenPopup([&](QWidget * Popup)
  {
    EXPECT_NE(Popup->findChild<QWidget *>(QStringLiteral("ColorUserPalette")), nullptr);
    Child<QToolButton>(Popup, "Color563412")->click();
  });
  Color = TColor(0);
  EXPECT_TRUE(ColorMenuPopup(QPoint(10, 10), Color, true));
  EXPECT_EQ(Color, TColor(0x563412));

  // Closed without a color
  WhenPopup([&](QWidget * Popup) { Popup->close(); });
  EXPECT_FALSE(ColorMenuPopup(QPoint(10, 10), Color, true));
  EXPECT_EQ(Color, TColor(0x563412));
}
//---------------------------------------------------------------------------
