// L3 tests of upstream's Help buttons of the dialogs (app/QtInterface.cpp):
// right of the buttons wherever the box is laid out, the keyword of the
// form or of its page.
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <QtInterface.h>

#include <QBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QPushButton>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The Help button is right of the OK and Cancel buttons, in their row
void ExpectRightOfButtons(QDialog & Dialog, QDialogButtonBox * Buttons, QPushButton * Help)
{
  Dialog.show();
  QApplication::processEvents();
  QPushButton * Cancel = Buttons->button(QDialogButtonBox::Cancel);
  QPoint CancelPos = Cancel->mapTo(&Dialog, QPoint(0, 0));
  QPoint HelpPos = Help->mapTo(&Dialog, QPoint(0, 0));
  EXPECT_TRUE(Help->isVisible());
  EXPECT_GT(HelpPos.x(), CancelPos.x());
  EXPECT_EQ(HelpPos.y(), CancelPos.y());
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(HelpButtonTest, RightOfTheButtons)
{
  // In a column of the dialog
  {
    QDialog Dialog;
    QVBoxLayout * Layout = new QVBoxLayout(&Dialog);
    QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &Dialog);
    Layout->addWidget(Buttons);
    QPushButton * Help = AddHelpButton(Buttons, L"ui_copy");
    EXPECT_EQ(Help->objectName(), QStringLiteral("HelpButton"));
    ExpectRightOfButtons(Dialog, Buttons, Help);
  }
  // In a row that is laid out later
  {
    QDialog Dialog;
    QVBoxLayout * Layout = new QVBoxLayout(&Dialog);
    QHBoxLayout * Row = new QHBoxLayout();
    QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &Dialog);
    Row->addWidget(new QPushButton(QStringLiteral("Transfer settings..."), &Dialog));
    Row->addWidget(Buttons);
    QPushButton * Help = AddHelpButton(Buttons, L"ui_copy");
    Layout->addLayout(Row);
    ExpectRightOfButtons(Dialog, Buttons, Help);
  }
  // In a grid
  {
    QDialog Dialog;
    QGridLayout * Grid = new QGridLayout(&Dialog);
    QDialogButtonBox * Buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &Dialog);
    Grid->addWidget(Buttons, 1, 0, 1, 2);
    QPushButton * Help = AddHelpButton(Buttons, L"ui_copy");
    ExpectRightOfButtons(Dialog, Buttons, Help);
  }
}
//---------------------------------------------------------------------------
TEST(HelpButtonTest, KeywordsOfTheForms)
{
  // upstream's keywords of the forms and of their pages (tools/dfm2cpp)
  EXPECT_EQ(FormHelpKeyword(L"Copy.dfm"), UnicodeString(L"ui_copy"));
  EXPECT_EQ(FormHelpKeyword(L"Preferences.dfm"), UnicodeString(L"ui_preferences"));
  EXPECT_EQ(FormHelpKeyword(L"Preferences.dfm", QStringLiteral("LogSheet")), UnicodeString(L"ui_pref_logging"));
  EXPECT_EQ(FormHelpKeyword(L"SiteAdvanced.dfm", QStringLiteral("ProxySheet")), UnicodeString(L"ui_login_proxy"));
  // A page without its own is the form's
  EXPECT_EQ(FormHelpKeyword(L"Preferences.dfm", QStringLiteral("NoSuchSheet")), UnicodeString(L"ui_preferences"));
}
//---------------------------------------------------------------------------
