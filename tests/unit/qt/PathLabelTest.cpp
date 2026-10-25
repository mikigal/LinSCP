// L3 tests of the path labels of the panels (views/PathLabel.cpp): the
// path shortened to fit, the directories and the mask clicked.
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <PathLabel.h>

#include <QSignalSpy>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
namespace
{
//---------------------------------------------------------------------------
// The x of the end of the start of the shown path (upstream's indent first)
int TextX(TPathLabel & Label, const QString & Start)
{
  return 5 + Label.fontMetrics().horizontalAdvance(Start);
}
//---------------------------------------------------------------------------
void Click(TPathLabel & Label, int X)
{
  QPoint Point(X, Label.height() / 2);
  QTest::mouseMove(&Label, Point);
  QEnterEvent Enter(Point, Label.mapToGlobal(Point), Label.mapToGlobal(Point));
  QApplication::sendEvent(&Label, &Enter);
  QTest::mouseClick(&Label, Qt::LeftButton, Qt::NoModifier, Point);
}
//---------------------------------------------------------------------------
} // namespace
//---------------------------------------------------------------------------
TEST(PathLabelTest, DirectoriesClicked)
{
  TPathLabel Label(nullptr);
  Label.resize(600, Label.sizeHint().height());
  Label.SetPath(QStringLiteral("/home/user/docs"));
  Label.show();
  EXPECT_EQ(Label.GetDisplayPath(), QStringLiteral("/home/user/docs/"));
  QSignalSpy Paths(&Label, &TPathLabel::PathClicked);
  QSignalSpy Masks(&Label, &TPathLabel::MaskClicked);

  // A directory of the path
  Click(Label, TextX(Label, QStringLiteral("/home/us")));
  ASSERT_EQ(Paths.count(), 1);
  EXPECT_EQ(Paths.takeFirst().at(0).toString(), QStringLiteral("/home/user"));
  Click(Label, TextX(Label, QStringLiteral("/")) - 2);
  ASSERT_EQ(Paths.count(), 1);
  EXPECT_EQ(Paths.takeFirst().at(0).toString(), QStringLiteral("/"));

  // The mask (*.* without a filter) after the path
  Click(Label, TextX(Label, QStringLiteral("/home/user/docs/*")));
  EXPECT_EQ(Paths.count(), 0);
  EXPECT_EQ(Masks.count(), 1);
}
//---------------------------------------------------------------------------
TEST(PathLabelTest, Shortened)
{
  TPathLabel Label(nullptr);
  QString Path = QStringLiteral("/home/user/projects/some/very/long/path/to/directory");
  QFontMetrics Metrics = Label.fontMetrics();
  // The last two directories fit
  int Width = Metrics.horizontalAdvance(QStringLiteral("/.../to/directory/")) + Metrics.horizontalAdvance(QStringLiteral("*.*")) + 12;
  Label.resize(Width, Label.sizeHint().height());
  Label.SetPath(Path);
  Label.show();
  EXPECT_EQ(Label.GetDisplayPath(), QStringLiteral("/.../to/directory/"));
  EXPECT_EQ(Label.toolTip(), Path);

  // The shown directories are the original ones, the "..." the hidden ones
  QSignalSpy Paths(&Label, &TPathLabel::PathClicked);
  Click(Label, TextX(Label, QStringLiteral("/.../t")));
  ASSERT_EQ(Paths.count(), 1);
  EXPECT_EQ(Paths.takeFirst().at(0).toString(), QStringLiteral("/home/user/projects/some/very/long/path/to"));
  Click(Label, TextX(Label, QStringLiteral("/..")));
  ASSERT_EQ(Paths.count(), 1);
  EXPECT_EQ(Paths.takeFirst().at(0).toString(), QStringLiteral("/home/user/projects/some/very/long/path"));

  // Not even the last directory fits
  Label.resize(Metrics.horizontalAdvance(QStringLiteral(".../dire...")) + 12, Label.height());
  EXPECT_TRUE(Label.GetDisplayPath().startsWith(QStringLiteral(".../")));
  EXPECT_TRUE(Label.GetDisplayPath().contains(QStringLiteral("...")));
}
//---------------------------------------------------------------------------
