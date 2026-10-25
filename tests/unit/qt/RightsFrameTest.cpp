// L3 tests of the permissions editor (dialogs/RightsFrame.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <RightsFrame.h>

#include <QAction>
#include <QLineEdit>
#include <QToolButton>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QString Octal(TRightsFrame & Frame)
{
  return Child<QLineEdit>(&Frame, "OctalEdit")->text();
}
//---------------------------------------------------------------------------
QAction * Action(TRightsFrame & Frame, const char * Name)
{
  return Frame.findChild<QAction *>(QString::fromLatin1(Name));
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(RightsFrameTest, ChecksShowTheRights)
{
  TRightsFrame Frame(nullptr);
  Frame.SetRights(TRights(0754));
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "OwnerReadCheck")->isChecked());
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "OwnerExecuteCheck")->isChecked());
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "GroupExecuteCheck")->isChecked());
  EXPECT_FALSE(Child<QCheckBox>(&Frame, "GroupWriteCheck")->isChecked());
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "OthersReadCheck")->isChecked());
  EXPECT_FALSE(Child<QCheckBox>(&Frame, "OthersExecuteCheck")->isChecked());
  EXPECT_FALSE(Child<QCheckBox>(&Frame, "SetUidCheck")->isChecked());
  EXPECT_EQ(Octal(Frame), QStringLiteral("0754"));
  EXPECT_EQ(Frame.GetRights().Number, 0754);
  EXPECT_EQ(Frame.GetText(), UnicodeString(L"rwxr-xr--"));
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, ClickingChecksChangesTheRights)
{
  TRightsFrame Frame(nullptr);
  int Changes = 0;
  Frame.OnChange = [&]() { Changes++; };
  Frame.SetRights(TRights(0644));
  Changes = 0;
  Child<QCheckBox>(&Frame, "GroupWriteCheck")->click();
  Child<QCheckBox>(&Frame, "StickyBitCheck")->click();
  EXPECT_EQ(Frame.GetRights().Number, 01664);
  EXPECT_EQ(Octal(Frame), QStringLiteral("1664"));
  EXPECT_EQ(Changes, 2);
  // Without the undefined state, a checked box gets unchecked
  Child<QCheckBox>(&Frame, "GroupWriteCheck")->click();
  EXPECT_EQ(Frame.GetRights().Number, 01644);
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, GroupButtonsCycleTheRights)
{
  TRightsFrame Frame(nullptr);
  Frame.SetRights(TRights(0640));
  QToolButton * OwnerButton = Child<QToolButton>(&Frame, "OwnerButton");
  // Mixed: all set
  OwnerButton->click();
  EXPECT_EQ(Frame.GetRights().Number, 0740);
  // All set: none
  OwnerButton->click();
  EXPECT_EQ(Frame.GetRights().Number, 0040);
  // None: all, as the undefined state is not allowed
  OwnerButton->click();
  EXPECT_EQ(Frame.GetRights().Number, 0740);

  Frame.SetAllowUndef(true);
  OwnerButton->click();
  OwnerButton->click();
  TRights Rights = Frame.GetRights();
  EXPECT_TRUE(Rights.IsUndef);
  EXPECT_EQ(Rights.RightUndef[TRights::rrUserRead], TRights::rsUndef);
  EXPECT_EQ(Rights.RightUndef[TRights::rrGroupRead], TRights::rsYes);
  EXPECT_EQ(Octal(Frame), QString());
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, GrayedStateWhenAllowed)
{
  TRightsFrame Frame(nullptr);
  // The rights tell whether the undefined state is allowed
  TRights Rights(0600);
  Rights.AllowUndef = true;
  Frame.SetRights(Rights);
  QCheckBox * Check = Child<QCheckBox>(&Frame, "OwnerReadCheck");
  // Checked, grayed, unchecked, checked
  Check->click();
  EXPECT_EQ(Check->checkState(), Qt::PartiallyChecked);
  EXPECT_EQ(Frame.GetRights().RightUndef[TRights::rrUserRead], TRights::rsUndef);
  Check->click();
  EXPECT_EQ(Check->checkState(), Qt::Unchecked);
  Check->click();
  EXPECT_EQ(Check->checkState(), Qt::Checked);
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, OctalEditSetsTheRights)
{
  TRightsFrame Frame(nullptr);
  Frame.SetRights(TRights(0644));
  QLineEdit * Edit = Child<QLineEdit>(&Frame, "OctalEdit");
  Edit->clear();
  QTest::keyClicks(Edit, QStringLiteral("75"));
  // Not complete yet
  EXPECT_EQ(Frame.GetRights().Number, 0644);
  QTest::keyClicks(Edit, QStringLiteral("1"));
  EXPECT_EQ(Frame.GetRights().Number, 0751);
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "OthersExecuteCheck")->isChecked());
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, TextWithAddXToDirectories)
{
  TRightsFrame Frame(nullptr);
  Frame.SetAllowAddXToDirectories(true);
  Frame.SetText(L"rw-r----- (+x)");
  EXPECT_EQ(Frame.GetRights().Number, 0640);
  EXPECT_TRUE(Frame.GetAddXToDirectories());
  EXPECT_EQ(Frame.GetText(), UnicodeString(L"rw-r----- (+x)"));
  // The octal value
  Frame.SetText(L"755");
  EXPECT_EQ(Frame.GetRights().Number, 0755);
  EXPECT_FALSE(Frame.GetAddXToDirectories());
  // X of all makes the option ineffective
  EXPECT_FALSE(Child<QCheckBox>(&Frame, "DirectoriesXCheck")->isEnabled());
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, Actions)
{
  TRightsFrame Frame(nullptr);
  Frame.SetRights(TRights(0640));
  Action(Frame, "AllRightsAction")->trigger();
  EXPECT_EQ(Frame.GetRights().Number, 0777);
  EXPECT_TRUE(Action(Frame, "AllRightsAction")->isChecked());
  Action(Frame, "DefaultRightsAction")->trigger();
  EXPECT_EQ(Frame.GetRights().Number, 0644);
  Action(Frame, "NoRightsAction")->trigger();
  EXPECT_EQ(Frame.GetRights().Number, 0);
  EXPECT_EQ(Octal(Frame), QStringLiteral("0000"));
  // Only with the undefined state
  EXPECT_FALSE(Action(Frame, "LeaveRightsAsIsAction")->isVisible());
  Frame.SetAllowUndef(true);
  EXPECT_TRUE(Action(Frame, "LeaveRightsAsIsAction")->isVisible());
  Action(Frame, "LeaveRightsAsIsAction")->trigger();
  EXPECT_EQ(Frame.GetRights().NumberSet, 0);
  EXPECT_EQ(Frame.GetRights().NumberUnset, 0);
  EXPECT_FALSE(Action(Frame, "CopyOctalAction")->isEnabled());
}
//---------------------------------------------------------------------------
TEST(RightsFrameTest, DisplayAsAcl)
{
  TRightsFrame Frame(nullptr);
  Frame.DisplayAsAcl();
  EXPECT_TRUE(Child<QCheckBox>(&Frame, "OwnerWriteCheck")->isHidden());
  EXPECT_EQ(Child<QCheckBox>(&Frame, "OwnerExecuteCheck")->text(), QStringLiteral("R ACL"));
  EXPECT_EQ(Child<QCheckBox>(&Frame, "StickyBitCheck")->text(), QStringLiteral("W ACL"));
  EXPECT_TRUE(Child<QLineEdit>(&Frame, "OctalEdit")->isHidden());
  EXPECT_FALSE(Action(Frame, "AllRightsAction")->isEnabled());
  // The groups compare the visible checks only (and set all of them, as
  // upstream does)
  Child<QToolButton>(&Frame, "OthersButton")->click();
  EXPECT_EQ(Frame.GetRights().Number,
    TRights::rfOtherRead | TRights::rfOtherWrite | TRights::rfOtherExec | TRights::rfStickyBit);
}
//---------------------------------------------------------------------------
