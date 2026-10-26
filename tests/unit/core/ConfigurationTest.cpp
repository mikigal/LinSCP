// The application's version: LinSCP's (X.Y.Z of the core, N of LinSCP's
// release on it), see cmake/LinSCPVersion.cmake.
//---------------------------------------------------------------------------
#include <CorePCH.h>

#include <Configuration.h>
#include <CoreMain.h>
#include <FileInfo.h>

#include <gtest/gtest.h>
//---------------------------------------------------------------------------
TEST(ConfigurationTest, Version)
{
  EXPECT_EQ(Configuration->Version, UnicodeString(LINSCP_VERSION));
  EXPECT_EQ(Configuration->ProductVersion, UnicodeString(LINSCP_VERSION));
  // The release of LinSCP is a part of the version, not a build number:
  // "Version 6.7.1.1 (Build 2026-10-26)"
  UnicodeString VersionStr = Configuration->VersionStr;
  EXPECT_TRUE(StartsStr(UnicodeString(L"Version ") + LINSCP_VERSION + L" (", VersionStr)) << VersionStr;
  // The compatibility of the configuration depends on the core's version only
  EXPECT_EQ(Configuration->CompoundVersion, StrToCompoundVersion(LINSCP_CORE_VERSION));
}
//---------------------------------------------------------------------------
