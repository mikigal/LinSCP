# linscp-cli: the command line front end (the counterpart of upstream's
# winscp.com). ConsoleRunner.cpp is a converted copy of upstream's
# source/windows/ConsoleRunner.cpp, see docs/port/UPSTREAM.md.

add_executable(linscp-cli
  CliInterface.cpp
  CliMain.cpp
  ConsoleRunner.cpp
  UnixConsole.cpp)

target_link_libraries(linscp-cli PRIVATE
  linscp::app
  linscp::options
  linscp_upstream_warnings)

include(GNUInstallDirs)
install(TARGETS linscp-cli RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
