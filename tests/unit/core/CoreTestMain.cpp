// The main function of the core's L1 tests: the core is initialized once
// (configuration, PuTTY, neon) for all tests, as the application's main
// function (WinSCP.cpp) does.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
class TCoreEnvironment : public testing::Environment
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
  testing::InitGoogleTest(&argc, argv);
  testing::AddGlobalTestEnvironment(new TCoreEnvironment());
  return RUN_ALL_TESTS();
}
//---------------------------------------------------------------------------
