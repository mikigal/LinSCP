// The main function of the L3 tests of the Qt front end's components: an
// application on the offscreen platform and the core initialized as by
// linscp, with the configuration in a temporary directory.
//---------------------------------------------------------------------------
#include <QtTools.h>

#include <QApplication>
#include <QDir>
#include <QTemporaryDir>

#include <gtest/gtest.h>

#include <cstdlib>
//---------------------------------------------------------------------------
class TQtEnvironment : public testing::Environment
{
public:
  void SetUp() override
  {
    ApplicationLog = new TApplicationLog();
    CoreInitialize();
  }

  void TearDown() override
  {
    CoreFinalize();
    delete ApplicationLog;
    ApplicationLog = NULL;
  }
};
//---------------------------------------------------------------------------
int main(int argc, char ** argv)
{
  // Nothing of the user's session
  QTemporaryDir Home;
  for (const char * Name : { "HOME", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME", "XDG_STATE_HOME" })
  {
    QByteArray Path = QDir(Home.path()).filePath(QString::fromLatin1(Name)).toLocal8Bit();
    QDir().mkpath(QString::fromLocal8Bit(Path));
    setenv(Name, Path.constData(), 1);
  }
  setenv("QT_QPA_PLATFORM", "offscreen", 1);
  unsetenv("WAYLAND_DISPLAY");
  unsetenv("DISPLAY");

  QApplication Application(argc, argv);
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new TQtEnvironment());
  return RUN_ALL_TESTS();
}
//---------------------------------------------------------------------------
