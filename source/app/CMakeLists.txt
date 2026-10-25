# libopenscp-app: the application layer shared by the front ends (openscp-cli,
# later the GUI). Some of its files are converted copies of upstream's
# source/windows, see docs/port/UPSTREAM.md (Ported application files).

# Resource strings of the application layer (LoadStr)
openscp_add_string_tables(openscp_app_strings
  "${PROJECT_SOURCE_DIR}/source/resource/TextsWin1.rc")

add_library(openscp_app STATIC
  AppConfiguration.cpp
  AppInterface.cpp
  AppTools.cpp
  CustomCommands.cpp
  DiscMon.cpp
  GUIConfiguration.cpp
  ProgParams.cpp
  SynchronizeController.cpp)
add_library(openscp::app ALIAS openscp_app)

target_include_directories(openscp_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_sources(openscp_app INTERFACE $<TARGET_OBJECTS:openscp_app_strings>)
target_link_libraries(openscp_app
  PRIVATE openscp::options openscp_upstream_warnings
  PUBLIC openscp::core)
