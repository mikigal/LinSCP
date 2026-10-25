// L3 tests of LinSCP's icon theme (source/qt/icons/linscp, app/QtTools.cpp):
// installed over the desktop's, its icons drawn; the images of the dialogs.
//---------------------------------------------------------------------------
#include "QtTestTools.h"

#include <AppConfiguration.h>
#include <QtInterface.h>

#include <QIcon>
#include <QImage>
#include <QLabel>
#include <QPixmap>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(IconThemeTest, OwnIcons)
{
  QString DesktopTheme = QIcon::themeName();
  QStringList SearchPaths = QIcon::themeSearchPaths();
  InstallIconTheme();
  EXPECT_EQ(QIcon::themeName(), QStringLiteral("linscp"));
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
TEST(IconThemeTest, DialogImages)
{
  // upstream's DialogImages (Animations96.dfm) but the ones of the updates
  for (const wchar_t * Name : {
         L"Keep remote directory up to date", L"Open console window", L"Download File",
         L"Download File Remove Original", L"Upload File", L"Upload File Remove Original", L"Duplicate L to R",
         L"Move L to R", L"Synchronize directories", L"Open folder", L"Delete file", L"Commander", L"Explorer",
         L"Multiple Files", L"Error", L"Help Blue", L"Information", L"Warning" })
  {
    QImage Image = DialogImage(Name).pixmap(32, 32).toImage();
    ASSERT_FALSE(Image.isNull()) << UnicodeString(Name).c_str();
    EXPECT_EQ(Image.width(), 32);
    int Opaque = 0;
    for (int Y = 0; Y < Image.height(); Y++)
    {
      for (int X = 0; X < Image.width(); X++)
      {
        Opaque += (qAlpha(Image.pixel(X, Y)) > 128) ? 1 : 0;
      }
    }
    EXPECT_GT(Opaque, Image.width() * Image.height() / 5) << UnicodeString(Name).c_str();
  }
  QLabel Label;
  LoadDialogImage(&Label, L"Upload File");
  EXPECT_EQ(Label.size(), QSize(32, 32));
  EXPECT_EQ(Label.property("automationImageName").toString(), QStringLiteral("Upload File"));
}
//---------------------------------------------------------------------------
