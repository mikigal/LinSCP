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

# bcb2clang, and the converted core headers parsing against the RTL shim
foreach(BCB2CLANG_TEST test_bcb2clang test_core_headers)
  add_test(NAME unit.bcb2clang.${BCB2CLANG_TEST}
    COMMAND Python3::Interpreter -m unittest -v ${BCB2CLANG_TEST}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bcb2clang")
  set_tests_properties(unit.bcb2clang.${BCB2CLANG_TEST} PROPERTIES
    LABELS unit
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/bcb2clang;PYTHONDONTWRITEBYTECODE=1;OPENSCP_CXX=${CMAKE_CXX_COMPILER}"
    TIMEOUT 300)
endforeach()
