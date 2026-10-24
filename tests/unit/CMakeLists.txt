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

add_executable(openscp_core_tests
  core/ResourceStringsTest.cpp
  $<TARGET_OBJECTS:openscp_core_strings>)
target_include_directories(openscp_core_tests PRIVATE "${PROJECT_SOURCE_DIR}/source/resource")
target_link_libraries(openscp_core_tests PRIVATE
  openscp::rtl
  openscp::options
  GTest::gtest_main)
gtest_discover_tests(openscp_core_tests
  PROPERTIES LABELS unit ENVIRONMENT "LC_ALL=C;TZ=UTC")

# .rc string table generator
add_test(NAME unit.rc2cpp
  COMMAND Python3::Interpreter -m unittest -v test_rc2cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rc2cpp")
set_tests_properties(unit.rc2cpp PROPERTIES
  LABELS unit
  ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/rc2cpp;PYTHONDONTWRITEBYTECODE=1")

# bcb2clang, and the converted core headers parsing against the RTL shim
foreach(BCB2CLANG_TEST test_bcb2clang test_convert test_core_headers)
  add_test(NAME unit.bcb2clang.${BCB2CLANG_TEST}
    COMMAND Python3::Interpreter -m unittest -v ${BCB2CLANG_TEST}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bcb2clang")
  set_tests_properties(unit.bcb2clang.${BCB2CLANG_TEST} PROPERTIES
    LABELS unit
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/bcb2clang;PYTHONDONTWRITEBYTECODE=1;OPENSCP_CXX=${CMAKE_CXX_COMPILER}"
    TIMEOUT 300)
endforeach()
