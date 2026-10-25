// L3 tests of OpenSCP's icon theme (source/qt/icons/openscp, app/QtTools.cpp):
// installed over the desktop's, its icons drawn.
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>

#include <QIcon>
#include <QImage>
#include <QPixmap>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(IconThemeTest, OwnIcons)
{
  QString DesktopTheme = QIcon::themeName();
  QStringList SearchPaths = QIcon::themeSearchPaths();
  InstallIconTheme();
  EXPECT_EQ(QIcon::themeName(), QStringLiteral("openscp"));
  // The icons of the toolbars, of the panels and of the application, in
  // the sizes of upstream's toolbars
  for (const char * Name : { "folder", "inode-directory", "go-up", "view-refresh", "cloud-upload", "download", "edit-delete" })
  {
    QIcon Icon = QIcon::fromTheme(QString::fromLatin1(Name));
    ASSERT_FALSE(Icon.isNull()) << Name;
    for (int Size : { 16, 20, 24 })
    {
      QImage Image = Icon.pixmap(Size, Size).toImage();
      ASSERT_FALSE(Image.isNull()) << Name;
      // Drawn: a good part of it opaque
      int Opaque = 0;
      for (int Y = 0; Y < Image.height(); Y++)
      {
        for (int X = 0; X < Image.width(); X++)
        {
          Opaque += (qAlpha(Image.pixel(X, Y)) > 128) ? 1 : 0;
        }
      }
      EXPECT_GT(Opaque, Image.width() * Image.height() / 5) << Name << " " << Size;
    }
  }
  EXPECT_FALSE(ApplicationIcon().pixmap(64, 64).isNull());
  QIcon::setThemeName(DesktopTheme);
  QIcon::setThemeSearchPaths(SearchPaths);
}
//---------------------------------------------------------------------------
