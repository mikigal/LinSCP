# openscp-cli: the command line front end (the counterpart of upstream's
# winscp.com). ConsoleRunner.cpp is a converted copy of upstream's
# source/windows/ConsoleRunner.cpp, see docs/port/UPSTREAM.md.

add_executable(openscp-cli
  CliInterface.cpp
  CliMain.cpp
  ConsoleRunner.cpp
  UnixConsole.cpp)

target_link_libraries(openscp-cli PRIVATE
  openscp::app
  openscp::options
  openscp_upstream_warnings)

include(GNUInstallDirs)
install(TARGETS openscp-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
