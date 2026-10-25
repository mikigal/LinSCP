# liblinscp-app: the application layer shared by the front ends (linscp-cli,
# later the GUI). Some of its files are converted copies of upstream's
# source/windows, see docs/port/UPSTREAM.md (Ported application files).

# Resource strings of the application layer (LoadStr)
linscp_add_string_tables(linscp_app_strings
  "${PROJECT_SOURCE_DIR}/source/resource/TextsWin1.rc")

add_library(linscp_app STATIC
  AppConfiguration.cpp
  AppInterface.cpp
  AppTools.cpp
  CustomCommands.cpp
  EditorPreferences.cpp
  DiscMon.cpp
  GUIConfiguration.cpp
  ProgParams.cpp
  SynchronizeController.cpp
  Translations.cpp)
add_library(linscp::app ALIAS linscp_app)

target_include_directories(linscp_app PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
# The translations of the source tree, for the builds that are not installed
target_compile_definitions(linscp_app PRIVATE LINSCP_SOURCE_TRANSLATIONS="${PROJECT_SOURCE_DIR}/translations")
target_sources(linscp_app INTERFACE $<TARGET_OBJECTS:linscp_app_strings>)
target_link_libraries(linscp_app
  PRIVATE linscp::options linscp_upstream_warnings
  PUBLIC linscp::core)
