// L3 tests of the bookmarks and location profiles
// (dialogs/OpenDirectoryDialog.cpp, LocationProfilesDialog.cpp, the
// shortcuts of app/ShortCuts.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <Bookmarks.h>
#include <QtInterface.h>
#include <SessionData.h>
#include <ShortCuts.h>
#include <Terminal.h>

#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidget>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
QStringList ListItems(QListWidget * List)
{
  QStringList Result;
  for (int Index = 0; Index < List->count(); Index++)
  {
    Result.append(List->item(Index)->text());
  }
  return Result;
}
//---------------------------------------------------------------------------
// The tree as "folder/name" lines
QStringList TreeItems(QTreeWidget * Tree)
{
  QStringList Result;
  for (QTreeWidgetItemIterator I(Tree); *I != nullptr; ++I)
  {
    QTreeWidgetItem * Parent = (*I)->parent();
    Result.append(((Parent != nullptr) ? (Parent->text(0) + QLatin1Char('/')) : QString()) + (*I)->text(0));
  }
  return Result;
}
//---------------------------------------------------------------------------
QTreeWidgetItem * FindItem(QTreeWidget * Tree, const QString & Text)
{
  QList<QTreeWidgetItem *> Items = Tree->findItems(Text, Qt::MatchExactly | Qt::MatchRecursive);
  return Items.isEmpty() ? nullptr : Items.first();
}
//---------------------------------------------------------------------------
class TBookmarksTest : public testing::Test
{
protected:
  virtual void TearDown() override
  {
    std::unique_ptr<TBookmarkList> Empty(new TBookmarkList());
    AppConfiguration->SharedBookmarks = Empty.get();
    AppConfiguration->UseSharedBookmarks = false;
    AppConfiguration->UseLocationProfiles = false;
  }
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(ShortCutsTest, Conversions)
{
  TShortCut CtrlOne = ShortCut(L'1', TShiftState() << ssCtrl);
  EXPECT_EQ(CtrlOne, TShortCut(scCtrl | L'1'));
  EXPECT_EQ(ShortCutToKeySequence(CtrlOne), QKeySequence(Qt::CTRL | Qt::Key_1));
  EXPECT_EQ(ShortCutToText(CtrlOne), UnicodeString(L"Ctrl+1"));
  TShortCut Letter = ShortCut(L'K', TShiftState() << ssShift << ssCtrl << ssAlt);
  EXPECT_EQ(ShortCutToKeySequence(Letter), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::ALT | Qt::Key_K));
  EXPECT_TRUE(IsCustomShortCut(CtrlOne));
  EXPECT_TRUE(IsCustomShortCut(Letter));
  EXPECT_FALSE(IsCustomShortCut(ShortCut(L'K', TShiftState() << ssCtrl)));
  EXPECT_TRUE(ShortCutToKeySequence(0).isEmpty());
}
//---------------------------------------------------------------------------
TEST_F(TBookmarksTest, SharedBookmarks)
{
  std::unique_ptr<TStrings> Directories(new TStringList());
  Directories->Add(L"/srv");
  UnicodeString Directory = L"/home/user/";
  QStringList Initial, Added, Moved;
  QString Title, ShortCutText;
  bool SwitchVisible = true;
  WhenModal([&](QWidget * Dialog)
  {
    Title = Dialog->windowTitle();
    SwitchVisible = Child<QPushButton>(Dialog, "SwitchButton")->isVisible();
    QListWidget * List = Child<QListWidget>(Dialog, "SharedBookmarksList");
    // Added already, when adding a bookmark
    Initial = ListItems(List);
    Child<QComboBox>(Dialog, "RemoteDirectoryEdit")->setEditText(QStringLiteral("/srv"));
    Child<QPushButton>(Dialog, "AddSharedBookmarkButton")->click();
    Added = ListItems(List);
    // Before the selected one; down again
    Child<QPushButton>(Dialog, "DownSharedBookmarkButton")->click();
    Moved = ListItems(List);
    WhenModal([&](QWidget * ShortCutDialog)
    {
      QComboBox * Combo = Child<QComboBox>(ShortCutDialog, "ShortCutCombo");
      Combo->setCurrentIndex(Combo->findText(QStringLiteral("Ctrl+1")));
      Child<QPushButton>(ShortCutDialog, "OKButton")->click();
      QTimer::singleShot(50, [&, Dialog]()
      {
        ShortCutText = Child<QListWidget>(Dialog, "SharedBookmarksList")->item(1)->text();
        Child<QPushButton>(Dialog, "OKBtn")->click();
      });
    });
    Child<QPushButton>(Dialog, "ShortCutSharedBookmarkButton")->click();
  });
  ASSERT_TRUE(DoOpenDirectoryDialog(odAddBookmark, osRemote, Directory, Directories.get(), NULL, false));
  EXPECT_EQ(Title, QStringLiteral("Manage bookmarks"));
  EXPECT_FALSE(SwitchVisible);
  EXPECT_EQ(Initial, QStringList({ QStringLiteral("/home/user") }));
  EXPECT_EQ(Added, QStringList({ QStringLiteral("/srv"), QStringLiteral("/home/user") }));
  EXPECT_EQ(Moved, QStringList({ QStringLiteral("/home/user"), QStringLiteral("/srv") }));
  EXPECT_EQ(ShortCutText, QStringLiteral("/srv (Ctrl+1)"));
  EXPECT_EQ(Directory, UnicodeString(L"/srv"));

  TBookmarkList * Shared = AppConfiguration->SharedBookmarks;
  ASSERT_NE(Shared, nullptr);
  ASSERT_EQ(Shared->Count, 2);
  EXPECT_EQ(Shared->Bookmarks[0]->Remote, UnicodeString(L"/home/user"));
  EXPECT_EQ(Shared->Bookmarks[1]->Remote, UnicodeString(L"/srv"));
  EXPECT_EQ(Shared->Bookmarks[1]->ShortCut, ShortCut(L'1', TShiftState() << ssCtrl));
  EXPECT_EQ(Shared->FindByShortCut(ShortCut(L'1', TShiftState() << ssCtrl)), Shared->Bookmarks[1]);

  // Opened by a bookmark; a removed one is gone
  Directory = L"/";
  QString Selected;
  QStringList AfterRemove;
  WhenModal([&](QWidget * Dialog)
  {
    QListWidget * List = Child<QListWidget>(Dialog, "SharedBookmarksList");
    List->setCurrentRow(0);
    Selected = Child<QComboBox>(Dialog, "RemoteDirectoryEdit")->currentText();
    List->setCurrentRow(1);
    Child<QPushButton>(Dialog, "RemoveSharedBookmarkButton")->click();
    AfterRemove = ListItems(List);
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  ASSERT_TRUE(DoOpenDirectoryDialog(odBrowse, osRemote, Directory, Directories.get(), NULL, false));
  EXPECT_EQ(Selected, QStringLiteral("/home/user"));
  EXPECT_EQ(AfterRemove, QStringList({ QStringLiteral("/home/user") }));
  EXPECT_EQ(Directory, UnicodeString(L"/home/user"));
  EXPECT_EQ(AppConfiguration->SharedBookmarks->Count, 1);
}
//---------------------------------------------------------------------------
TEST_F(TBookmarksTest, LocationProfiles)
{
  std::unique_ptr<TSessionData> SessionData(new TSessionData(L"profiles"));
  SessionData->HostName = L"example.com";
  SessionData->UserName = L"user";
  std::unique_ptr<TTerminal> Terminal(new TTerminal(SessionData.get(), Configuration));
  std::unique_ptr<TStrings> LocalDirectories(new TStringList());
  std::unique_ptr<TStrings> RemoteDirectories(new TStringList());
  UnicodeString Local = L"/home/user/work";
  UnicodeString Remote = L"/var/www";
  QStringList Added, Moved, Renamed, Final;
  WhenModal([&](QWidget * Dialog)
  {
    QTreeWidget * View = Child<QTreeWidget>(Dialog, "SessionProfilesView");
    // The name of the profile
    WhenModal([&](QWidget * NameDialog)
    {
      Child<QComboBox>(NameDialog, "NameCombo")->setEditText(QStringLiteral("web"));
      Child<QPushButton>(NameDialog, "OKButton")->click();
      QTimer::singleShot(50, [&, Dialog, View]()
      {
        Added = TreeItems(View);
        // Into a new folder
        WhenModal([&](QWidget * FolderDialog)
        {
          Child<QComboBox>(FolderDialog, "NameCombo")->setEditText(QStringLiteral("work"));
          Child<QPushButton>(FolderDialog, "OKButton")->click();
          QTimer::singleShot(50, [&, Dialog, View]()
          {
            Moved = TreeItems(View);
            // The folder renamed (as edited in place)
            FindItem(View, QStringLiteral("work"))->setText(0, QStringLiteral("projects"));
            Renamed = TreeItems(View);
            Child<QPushButton>(Dialog, "OKBtn")->click();
          });
        });
        Child<QPushButton>(Dialog, "SessionBookmarkMoveToButton")->click();
      });
    });
    Child<QPushButton>(Dialog, "AddSessionBookmarkButton")->click();
  });
  ASSERT_TRUE(LocationProfilesDialog(odBrowse, osLocal, Local, Remote,
    LocalDirectories.get(), RemoteDirectories.get(), Terminal.get()));
  EXPECT_EQ(Added, QStringList({ QStringLiteral("web") }));
  EXPECT_EQ(Moved, QStringList({ QStringLiteral("work"), QStringLiteral("work/web") }));
  EXPECT_EQ(Renamed, QStringList({ QStringLiteral("projects"), QStringLiteral("projects/web") }));

  TBookmarkList * Bookmarks = AppConfiguration->Bookmarks[SessionData->SessionKey];
  ASSERT_NE(Bookmarks, nullptr);
  ASSERT_EQ(Bookmarks->Count, 1);
  EXPECT_EQ(Bookmarks->Bookmarks[0]->Name, UnicodeString(L"web"));
  EXPECT_EQ(Bookmarks->Bookmarks[0]->Node, UnicodeString(L"projects"));
  EXPECT_EQ(Bookmarks->Bookmarks[0]->Local, UnicodeString(L"/home/user/work"));
  EXPECT_EQ(Bookmarks->Bookmarks[0]->Remote, UnicodeString(L"/var/www"));

  // Selecting the profile sets both directories
  Local = L"/";
  Remote = L"/";
  WhenModal([&](QWidget * Dialog)
  {
    QTreeWidget * View = Child<QTreeWidget>(Dialog, "SessionProfilesView");
    View->setCurrentItem(FindItem(View, QStringLiteral("web")));
    Final = TreeItems(View);
    Child<QPushButton>(Dialog, "OKBtn")->click();
  });
  ASSERT_TRUE(LocationProfilesDialog(odBrowse, osLocal, Local, Remote,
    LocalDirectories.get(), RemoteDirectories.get(), Terminal.get()));
  EXPECT_EQ(Final, QStringList({ QStringLiteral("projects"), QStringLiteral("projects/web") }));
  EXPECT_EQ(Local, UnicodeString(L"/home/user/work"));
  EXPECT_EQ(Remote, UnicodeString(L"/var/www"));

  std::unique_ptr<TBookmarkList> Empty(new TBookmarkList());
  AppConfiguration->Bookmarks[SessionData->SessionKey] = Empty.get();
}
//---------------------------------------------------------------------------
