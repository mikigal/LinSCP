// L3 tests of the toolbars as upstream's (views/Toolbars.cpp): the layout
// and the hidden buttons in upstream's format, the selective text labels,
// the hidden buttons, the rows of the panels' toolbars.
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <Toolbars.h>

#include <QAction>
#include <QBoxLayout>
#include <QMenu>
#include <QToolButton>

#include <memory>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QAction * NewAction(QObject * Parent, const char * Name, const QString & Caption)
{
  QAction * Action = new QAction(Caption, Parent);
  Action->setObjectName(QString::fromLatin1(Name));
  return Action;
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(ToolbarsTest, LayoutOfUpstream)
{
  // The default of upstream's Commander
  TToolbarsLayout Layout(AppConfiguration->CommanderToolbarsLayout);
  TToolbarPosition Position;
  ASSERT_TRUE(Layout.Get(QStringLiteral("Commands"), Position));
  EXPECT_TRUE(Position.Visible);
  EXPECT_EQ(Position.Dock, QStringLiteral("TopDock"));
  EXPECT_EQ(Position.Row, 1);
  EXPECT_EQ(Position.Pos, 0);
  ASSERT_TRUE(Layout.Get(QStringLiteral("Preferences"), Position));
  EXPECT_EQ(Position.Pos, 228);
  ASSERT_TRUE(Layout.Get(QStringLiteral("Session"), Position));
  EXPECT_FALSE(Position.Visible);
  ASSERT_TRUE(Layout.Get(QStringLiteral("Toolbar2"), Position));
  EXPECT_FALSE(Position.Visible);
  EXPECT_EQ(Position.Dock, QStringLiteral("BottomDock"));
  ASSERT_TRUE(Layout.Get(QStringLiteral("RemoteFile"), Position));
  EXPECT_EQ(Position.Dock, QStringLiteral("RemoteTopDock"));
  EXPECT_EQ(Position.Row, 1);
  EXPECT_FALSE(Layout.Get(QStringLiteral("Unknown"), Position));

  // The values that are not toolbars are kept
  Position.Visible = true;
  Position.Dock = QStringLiteral("BottomDock");
  Position.Row = 2;
  Position.Pos = 10;
  Layout.Set(QStringLiteral("Toolbar2"), Position);
  UnicodeString Str = Layout.ToString();
  EXPECT_TRUE(ContainsText(Str, L"Toolbar2=1:BottomDock:2+10"));
  EXPECT_TRUE(ContainsText(Str, L"Menu=1:TopDock:0+0"));
  EXPECT_TRUE(ContainsText(Str, L"Updates=1:TopDock:1+596"));
  EXPECT_TRUE(ContainsText(Str, L"PixelsPerInch=96"));
  EXPECT_TRUE(ContainsText(Str, L"Queue=1::0+-1"));
  TToolbarsLayout Again(Str);
  ASSERT_TRUE(Again.Get(QStringLiteral("Toolbar2"), Position));
  EXPECT_TRUE(Position.Visible);
  EXPECT_EQ(Position.Row, 2);
  EXPECT_EQ(Position.Pos, 10);
}
//---------------------------------------------------------------------------
TEST(ToolbarsTest, SelectiveTextLabels)
{
  QWidget Window;
  TToolbar * Toolbar = new TToolbar(QStringLiteral("CommandsToolbar"), QStringLiteral("Commands"), &Window);
  EXPECT_EQ(Toolbar->GetKey(), QStringLiteral("Commands"));
  QAction * Compare = NewAction(&Window, "CompareDirectoriesAction2", QStringLiteral("&Compare Directories"));
  QAction * Synchronize = NewAction(&Window, "FullSynchronizeAction2", QStringLiteral("&Synchronize..."));
  Toolbar->AddButton(Compare);
  Toolbar->AddButton(Synchronize, true);
  // Only the important command shows its text
  EXPECT_EQ(Toolbar->GetButton(Compare)->toolButtonStyle(), Qt::ToolButtonIconOnly);
  EXPECT_EQ(Toolbar->GetButton(Synchronize)->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
  EXPECT_EQ(Toolbar->GetButton(Synchronize)->text(), QStringLiteral("Synchronize"));
  // The buttons have the names of their actions
  EXPECT_EQ(Toolbar->GetButton(Synchronize)->objectName(), QStringLiteral("FullSynchronizeAction2"));
  Toolbar->SetSelectiveText(false);
  EXPECT_EQ(Toolbar->GetButton(Synchronize)->toolButtonStyle(), Qt::ToolButtonIconOnly);
  Toolbar->SetSelectiveText(true);
  EXPECT_EQ(Toolbar->GetButton(Synchronize)->toolButtonStyle(), Qt::ToolButtonTextBesideIcon);
  // upstream's sizes of the icons
  Toolbar->SetLargerToolbar(0);
  EXPECT_EQ(Toolbar->iconSize(), QSize(16, 16));
  Toolbar->SetLargerToolbar(2);
  EXPECT_EQ(Toolbar->iconSize(), QSize(24, 24));
}
//---------------------------------------------------------------------------
TEST(ToolbarsTest, HiddenButtons)
{
  QWidget Window;
  TToolbar * Toolbar = new TToolbar(QStringLiteral("RemoteFileToolbar"), QStringLiteral("Remote Files"), &Window);
  QAction * Copy = NewAction(&Window, "RemoteCopyAction", QStringLiteral("Down&load..."));
  QAction * Delete = NewAction(&Window, "RemoteDeleteAction2", QStringLiteral("&Delete"));
  QAction * Rename = NewAction(&Window, "RemoteRenameAction2", QStringLiteral("&Rename"));
  QMenu * NewMenu = new QMenu(QStringLiteral("&New"), &Window);
  QMenu * CopyMenu = new QMenu(&Window);
  Toolbar->AddMenuButton(Copy, CopyMenu, tmmSplit, true);
  Toolbar->AddSeparatorItem();
  Toolbar->AddButton(Delete);
  Toolbar->AddButton(Rename);
  Toolbar->AddSeparatorItem();
  Toolbar->AddSubmenu(QStringLiteral("RemoteNewSubmenuItem"), NewMenu, true);
  // upstream's names of the buttons
  EXPECT_EQ(Toolbar->GetItemNames(),
    QStringList({ QStringLiteral("RemoteCopy"), QStringLiteral("RemoteDelete"), QStringLiteral("RemoteRename"),
                  QStringLiteral("RemoteNew") }));
  EXPECT_EQ(Toolbar->GetButton(Copy)->popupMode(), QToolButton::MenuButtonPopup);

  // Hidden, the separator they leave alone goes too
  Toolbar->SetItemVisible(QStringLiteral("RemoteDelete"), false);
  Toolbar->SetItemVisible(QStringLiteral("RemoteRename"), false);
  EXPECT_EQ(Toolbar->GetButton(Delete), nullptr);
  int Separators = 0;
  for (QAction * Action : Toolbar->actions())
  {
    Separators += Action->isSeparator() ? 1 : 0;
  }
  EXPECT_EQ(Separators, 1);
  // The button of the copy was created again, with its menu
  ASSERT_NE(Toolbar->GetButton(Copy), nullptr);
  EXPECT_EQ(Toolbar->GetButton(Copy)->menu(), CopyMenu);

  // In upstream's format, and back
  std::vector<TToolbar *> Toolbars = { Toolbar };
  EXPECT_EQ(SaveToolbarsButtons(Toolbars), UnicodeString(L"RemoteFile=RemoteDelete:0,RemoteRename:0"));
  LoadToolbarsButtons(Toolbars, L"RemoteFile=RemoteRename:1,RemoteNew:0;Other=X:0");
  EXPECT_FALSE(Toolbar->IsItemVisible(QStringLiteral("RemoteDelete")));
  EXPECT_TRUE(Toolbar->IsItemVisible(QStringLiteral("RemoteRename")));
  EXPECT_FALSE(Toolbar->IsItemVisible(QStringLiteral("RemoteNew")));

  // The menu of upstream's Customize Toolbar
  QMenu Menu;
  Toolbar->FillCustomizeMenu(&Menu);
  QAction * Item = Menu.findChild<QAction *>(QStringLiteral("RemoteDeleteCustomizeItem"));
  ASSERT_NE(Item, nullptr);
  EXPECT_FALSE(Item->isChecked());
  Item->trigger();
  EXPECT_TRUE(Toolbar->IsItemVisible(QStringLiteral("RemoteDelete")));
  EXPECT_NE(Toolbar->GetButton(Delete), nullptr);
}
//---------------------------------------------------------------------------
TEST(ToolbarsTest, DockRows)
{
  QWidget Window;
  Window.resize(600, 200);
  QVBoxLayout * Layout = new QVBoxLayout(&Window);
  TToolbarDock * Dock = new TToolbarDock(QStringLiteral("RemoteTopDock"), &Window);
  Layout->addWidget(Dock);
  Layout->addStretch(1);
  TToolbar * Path = new TToolbar(QStringLiteral("RemotePathToolbar"), QString(), &Window);
  TToolbar * History = new TToolbar(QStringLiteral("RemoteHistoryToolbar"), QString(), &Window);
  TToolbar * File = new TToolbar(QStringLiteral("RemoteFileToolbar"), QString(), &Window);
  for (TToolbar * Toolbar : { Path, History, File })
  {
    Toolbar->AddButton(NewAction(&Window, "SomeAction", QStringLiteral("Some")));
  }
  Dock->AddToolbar(File);
  Dock->AddToolbar(History);
  Dock->AddToolbar(Path, true);
  // upstream's default positions
  Dock->SetPosition(Path, 0, 0);
  Dock->SetPosition(History, 0, 172);
  Dock->SetPosition(File, 1, 0);
  Dock->Arrange();
  Window.show();
  QApplication::processEvents();
  EXPECT_EQ(Path->y(), History->y());
  EXPECT_LT(Path->x(), History->x());
  EXPECT_GT(File->y(), Path->y());
  // The stretched one takes the rest of its row
  EXPECT_GT(Path->width(), History->width() * 3);
  int Row;
  int Pos;
  Dock->GetPosition(History, Row, Pos);
  EXPECT_EQ(Row, 0);
  EXPECT_EQ(Pos, 172);
}
//---------------------------------------------------------------------------
