// Helpers of the platform layer tests: a temporary directory and scoped
// environment variables (tests must not touch the user's directories).
//---------------------------------------------------------------------------
#ifndef LinuxTestUtilsH
#define LinuxTestUtilsH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <SysUtils.hpp>

#include <gtest/gtest.h>

#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
//---------------------------------------------------------------------------
class TTempDirectoryTest : public testing::Test
{
protected:
  void SetUp() override
  {
    const char * TempRoot = getenv("TMPDIR");
    std::string Pattern = std::string((TempRoot != nullptr) ? TempRoot : "/tmp") + "/linscp-linux-XXXXXX";
    std::vector<char> Buffer(Pattern.begin(), Pattern.end());
    Buffer.push_back('\0');
    ASSERT_NE(mkdtemp(Buffer.data()), nullptr);
    FRoot = Buffer.data();
    FDir = UnicodeString(FRoot.c_str()) + L"/";
  }

  void TearDown() override
  {
    std::error_code Error;
    std::filesystem::remove_all(FRoot, Error);
  }

  void WriteText(const std::string & Name, const std::string & Content)
  {
    std::filesystem::create_directories(std::filesystem::path(FRoot + "/" + Name).parent_path());
    std::ofstream Stream(FRoot + "/" + Name, std::ios::binary);
    Stream << Content;
  }

  std::string ReadText(const std::string & Name)
  {
    std::ifstream Stream(FRoot + "/" + Name, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());
  }

  std::string FRoot;
  UnicodeString FDir;
};
//---------------------------------------------------------------------------
// Sets an environment variable (unsets it for nullptr) for the scope
class TEnvironmentScope
{
public:
  TEnvironmentScope(const char * Name, const char * Value) : FName(Name)
  {
    const char * Old = getenv(Name);
    FHadValue = (Old != nullptr);
    if (FHadValue)
    {
      FOldValue = Old;
    }
    if (Value != nullptr)
    {
      setenv(Name, Value, 1);
    }
    else
    {
      unsetenv(Name);
    }
    tzset();
  }

  ~TEnvironmentScope()
  {
    if (FHadValue)
    {
      setenv(FName.c_str(), FOldValue.c_str(), 1);
    }
    else
    {
      unsetenv(FName.c_str());
    }
    tzset();
  }

private:
  std::string FName;
  std::string FOldValue;
  bool FHadValue;
};
//---------------------------------------------------------------------------
#endif
