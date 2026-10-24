add_executable(openscp_toolchain_tests
  ToolchainTest.cpp)
target_link_libraries(openscp_toolchain_tests PRIVATE
  openscp::options
  GTest::gtest_main)
gtest_discover_tests(openscp_toolchain_tests
  PROPERTIES LABELS unit)

add_executable(openscp_rtl_tests
  rtl/ClassesTest.cpp
  rtl/DataFormatsTest.cpp
  rtl/DateTimeTest.cpp
  rtl/FilesTest.cpp
  rtl/FormatTest.cpp
  rtl/StringsTest.cpp
  rtl/SystemTest.cpp
  rtl/SysUtilsTest.cpp
  rtl/UtilsTest.cpp)
target_link_libraries(openscp_rtl_tests PRIVATE
  openscp::rtl
  openscp::options
  GTest::gtest_main)
# Locale and time zone independent results; tests needing others set them
gtest_discover_tests(openscp_rtl_tests
  PROPERTIES LABELS unit ENVIRONMENT "LC_ALL=C;TZ=UTC")
