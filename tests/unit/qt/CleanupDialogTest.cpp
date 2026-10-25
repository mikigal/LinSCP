// L3 tests of the cleanup of the application data (dialogs/CleanupDialog.cpp).
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <QtInterface.h>

#include <QDir>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTreeWidget>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(CleanupDialogTest, TemporaryFolders)
{
  QTemporaryDir Temp;
  ASSERT_TRUE(Temp.isValid());
  ASSERT_TRUE(QDir(Temp.path()).mkdir(QStringLiteral("scp12345")));
  UnicodeString PrevTemporaryDirectory = AppConfiguration->DDTemporaryDirectory;
  AppConfiguration->DDTemporaryDirectory = ToUnicodeString(Temp.path());
  ::rtl::TFinally Finally([&]() { AppConfiguration->DDTemporaryDirectory = PrevTemporaryDirectory; });

  WhenModal([](QWidget * Dialog)
  {
    EXPECT_EQ(Dialog->objectName(), QStringLiteral("CleanupDialog"));
    QTreeWidget * DataListView = Child<QTreeWidget>(Dialog, "DataListView");
    QStringList Data;
    for (int Index = 0; Index < DataListView->topLevelItemCount(); Index++)
    {
      Data.append(DataListView->topLevelItem(Index)->text(0));
    }
    EXPECT_TRUE(Data.contains(QStringLiteral("Configuration INI file"))) << Data.join(QLatin1Char(',')).toStdString();
    int TemporaryFolders = Data.indexOf(QStringLiteral("Temporary folders"));
    ASSERT_GE(TemporaryFolders, 0);
    // Nothing checked, nothing to clean up
    QPushButton * OKButton = Child<QPushButton>(Dialog, "OKButton");
    EXPECT_FALSE(OKButton->isEnabled());
    Child<QPushButton>(Dialog, "CheckAllButton")->click();
    EXPECT_TRUE(OKButton->isEnabled());
    Child<QPushButton>(Dialog, "CheckAllButton")->click();
    EXPECT_FALSE(OKButton->isEnabled());
    // The temporary folders only (the configuration must stay for the
    // other tests)
    DataListView->topLevelItem(TemporaryFolders)->setCheckState(0, Qt::Checked);
    EXPECT_TRUE(OKButton->isEnabled());
    OKButton->click();
  });
  EXPECT_TRUE(DoCleanupDialog());
  EXPECT_FALSE(QDir(Temp.path()).exists(QStringLiteral("scp12345")));
}
//---------------------------------------------------------------------------
