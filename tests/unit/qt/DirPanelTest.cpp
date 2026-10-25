// L3 tests of the selection in the file panels (views/DirPanel.cpp,
// models/DirModel.cpp): the selection by masks, the filter, the file
// colors, the comparison of directories and the keyboard selection.
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <DirModel.h>
#include <DirPanel.h>

#include <QBrush>
#include <QDir>
#include <QFile>
#include <QLabel>
#include <QTemporaryDir>
#include <QTreeView>

#include <utime.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
class DirPanelTest : public testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_TRUE(FDir.isValid());
    FPanel.reset(new TLocalDirPanel(nullptr));
    FPanel->resize(600, 400);
  }

  void TearDown() override
  {
    AppConfiguration->NortonLikeMode = nlKeyboard;
    AppConfiguration->FileColors = L"";
    FPanel.reset();
  }

  // A file of the size, modified at the time (seconds of the epoch)
  static void CreateFile(const QString & Dir, const QString & Name, int Size, time_t Time = 1700000000)
  {
    QFile File(Dir + QLatin1Char('/') + Name);
    ASSERT_TRUE(File.open(QIODevice::WriteOnly));
    File.write(QByteArray(Size, 'x'));
    File.close();
    struct utimbuf Times = { Time, Time };
    utime(File.fileName().toLocal8Bit().constData(), &Times);
  }

  void Load(const QString & Path)
  {
    FPanel->ChangeDirectory(ToUnicodeString(Path));
  }

  QStringList SelectedNames()
  {
    QStringList Result;
    for (const QModelIndex & Index : FPanel->GetDirView()->selectionModel()->selectedRows())
    {
      Result.append(ToQString(FPanel->GetModel()->Item(Index.row()).FileName));
    }
    Result.sort();
    return Result;
  }

  void Focus(const QString & Name)
  {
    int Row = FPanel->GetModel()->FindFile(ToUnicodeString(Name));
    ASSERT_GE(Row, 0);
    FPanel->GetDirView()->selectionModel()->setCurrentIndex(
      FPanel->GetModel()->index(Row, 0), QItemSelectionModel::NoUpdate);
  }

  QString Current()
  {
    return ToQString(FPanel->GetModel()->Item(FPanel->GetDirView()->currentIndex().row()).FileName);
  }

  QTemporaryDir FDir;
  std::unique_ptr<TLocalDirPanel> FPanel;
};
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST_F(DirPanelTest, SelectsByMask)
{
  CreateFile(FDir.path(), QStringLiteral("a.txt"), 1);
  CreateFile(FDir.path(), QStringLiteral("b.txt"), 1);
  CreateFile(FDir.path(), QStringLiteral("c.log"), 1);
  QDir(FDir.path()).mkdir(QStringLiteral("d.txt"));
  Load(FDir.path());

  TFileFilter Filter;
  Filter.Masks = L"*.txt";
  FPanel->SelectFiles(Filter, true);
  // The directories only when asked
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("a.txt"), QStringLiteral("b.txt") }));
  Filter.Directories = true;
  FPanel->SelectFiles(Filter, true);
  EXPECT_EQ(FPanel->GetSelCount(), 3);
  Filter.Masks = L"b*";
  FPanel->SelectFiles(Filter, false);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("a.txt"), QStringLiteral("d.txt") }));

  // The parent directory is never selected
  FPanel->SelectAll(smInvert);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b.txt"), QStringLiteral("c.log") }));
  FPanel->SaveSelectedNames();
  FPanel->SelectAll(smAll);
  EXPECT_EQ(FPanel->GetSelCount(), 4);
  EXPECT_EQ(FPanel->GetFilesCount(), 4);
  FPanel->SelectAll(smNone);
  EXPECT_EQ(FPanel->GetSelCount(), 0);
  EXPECT_TRUE(FPanel->GetSelectedNamesSaved());
  FPanel->RestoreSelectedNames();
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b.txt"), QStringLiteral("c.log") }));

  // The selection stays over a reload
  FPanel->Reload();
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b.txt"), QStringLiteral("c.log") }));
}
//---------------------------------------------------------------------------
TEST_F(DirPanelTest, FiltersAndColorsFiles)
{
  CreateFile(FDir.path(), QStringLiteral("a.txt"), 1);
  CreateFile(FDir.path(), QStringLiteral("b.log"), 1);
  CreateFile(FDir.path(), QStringLiteral(".hidden"), 1);
  QDir(FDir.path()).mkdir(QStringLiteral("dir"));
  // Red for the logs, blue for the directories
  AppConfiguration->FileColors = L"0000FF:*.log,FF0000:*/";
  FPanel->ApplyConfiguration();
  Load(FDir.path());

  const TDirModel * Model = FPanel->GetModel();
  EXPECT_EQ(Model->GetHiddenCount(), 1);
  QModelIndex Log = Model->index(Model->FindFile(L"b.log"), 0);
  EXPECT_EQ(Model->data(Log, Qt::ForegroundRole).value<QBrush>().color(), QColor(Qt::red));
  QModelIndex Dir = Model->index(Model->FindFile(L"dir"), 0);
  EXPECT_EQ(Model->data(Dir, Qt::ForegroundRole).value<QBrush>().color(), QColor(Qt::blue));
  EXPECT_FALSE(Model->data(Model->index(Model->FindFile(L"a.txt"), 0), Qt::ForegroundRole).isValid());
  EXPECT_TRUE(Child<QLabel>(FPanel.get(), "LocalHiddenFilesLabel")->text().contains(QStringLiteral("1")));

  // The directories stay with a mask of files
  FPanel->SetMask(L"*.txt");
  EXPECT_EQ(Model->FindFile(L"b.log"), -1);
  EXPECT_GE(Model->FindFile(L"a.txt"), 0);
  EXPECT_GE(Model->FindFile(L"dir"), 0);
  EXPECT_EQ(Model->GetFilteredCount(), 1);
  FPanel->SetMask(UnicodeString());
  EXPECT_GE(Model->FindFile(L"b.log"), 0);
  EXPECT_EQ(Model->GetFilteredCount(), 0);
}
//---------------------------------------------------------------------------
TEST_F(DirPanelTest, ComparesDirectories)
{
  QString Other = FDir.path() + QStringLiteral("/other");
  QString Mine = FDir.path() + QStringLiteral("/mine");
  QDir(FDir.path()).mkdir(QStringLiteral("other"));
  QDir(FDir.path()).mkdir(QStringLiteral("mine"));
  CreateFile(Mine, QStringLiteral("same"), 10);
  CreateFile(Other, QStringLiteral("same"), 10);
  CreateFile(Mine, QStringLiteral("newer"), 10, 1700000100);
  CreateFile(Other, QStringLiteral("newer"), 10);
  CreateFile(Mine, QStringLiteral("older"), 10);
  CreateFile(Other, QStringLiteral("older"), 10, 1700000100);
  CreateFile(Mine, QStringLiteral("size"), 20);
  CreateFile(Other, QStringLiteral("size"), 10);
  CreateFile(Mine, QStringLiteral("missing"), 10);
  Load(Mine);
  TLocalDirPanel OtherPanel(nullptr);
  OtherPanel.ChangeDirectory(ToUnicodeString(Other));

  FPanel->CompareFiles(&OtherPanel, true, false);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("missing"), QStringLiteral("newer") }));
  FPanel->CompareFiles(&OtherPanel, true, true);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("missing"), QStringLiteral("newer"), QStringLiteral("size") }));
}
//---------------------------------------------------------------------------
TEST_F(DirPanelTest, KeyboardSelection)
{
  CreateFile(FDir.path(), QStringLiteral("a"), 1);
  CreateFile(FDir.path(), QStringLiteral("b"), 1);
  CreateFile(FDir.path(), QStringLiteral("c"), 1);
  CreateFile(FDir.path(), QStringLiteral("d"), 1);
  AppConfiguration->NortonLikeMode = nlKeyboard;
  FPanel->ApplyConfiguration();
  Load(FDir.path());
  QTreeView * View = FPanel->GetDirView();
  FPanel->show();
  View->setFocus();

  // The keys only move the focus, Insert and Space select
  Focus(QStringLiteral("a"));
  QTest::keyClick(View, Qt::Key_Down);
  EXPECT_EQ(Current(), QStringLiteral("b"));
  EXPECT_TRUE(SelectedNames().isEmpty());
  QTest::keyClick(View, Qt::Key_Insert);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b") }));
  EXPECT_EQ(Current(), QStringLiteral("c"));
  QTest::keyClick(View, Qt::Key_Space);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b"), QStringLiteral("c") }));
  QTest::keyClick(View, Qt::Key_Space);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b") }));
  // Shift selects what the focus moves over, without unselecting
  QTest::keyClick(View, Qt::Key_Down, Qt::ShiftModifier);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d") }));
  QTest::keyClick(View, Qt::Key_Up);
  EXPECT_EQ(SelectedNames().size(), 3);

  // A file selected by a click is unselected by the keyboard
  FPanel->SelectAll(smNone);
  QRect Rect = View->visualRect(FPanel->GetModel()->index(FPanel->GetModel()->FindFile(L"a"), 0));
  QTest::mouseClick(View->viewport(), Qt::LeftButton, Qt::NoModifier, Rect.center());
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("a") }));
  QTest::keyClick(View, Qt::Key_Down);
  EXPECT_TRUE(SelectedNames().isEmpty());
  EXPECT_EQ(Current(), QStringLiteral("b"));

  // The Explorer-style selection by the keyboard
  AppConfiguration->NortonLikeMode = nlOff;
  FPanel->ApplyConfiguration();
  QTest::keyClick(View, Qt::Key_Down);
  EXPECT_EQ(SelectedNames(), QStringList({ QStringLiteral("c") }));
}
//---------------------------------------------------------------------------
