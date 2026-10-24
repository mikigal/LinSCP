add_executable(openscp_toolchain_tests
  ToolchainTest.cpp)
target_link_libraries(openscp_toolchain_tests PRIVATE
  openscp::options
  GTest::gtest_main)
gtest_discover_tests(openscp_toolchain_tests
  PROPERTIES LABELS unit)
