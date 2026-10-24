# Wayland test harness tools (see docs/port/PLAN.md, section 6.4).
# These are test infrastructure, not part of OpenSCP itself.

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)
find_package(PlasmaWaylandProtocols REQUIRED)
find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets)

set(FAKE_INPUT_XML "${PLASMA_WAYLAND_PROTOCOLS_DIR}/fake-input.xml")
set(FAKE_INPUT_HEADER "${CMAKE_CURRENT_BINARY_DIR}/fake-input-client-protocol.h")
set(FAKE_INPUT_CODE "${CMAKE_CURRENT_BINARY_DIR}/fake-input-protocol.c")
add_custom_command(
  OUTPUT "${FAKE_INPUT_HEADER}" "${FAKE_INPUT_CODE}"
  COMMAND "${WAYLAND_SCANNER}" client-header "${FAKE_INPUT_XML}" "${FAKE_INPUT_HEADER}"
  COMMAND "${WAYLAND_SCANNER}" private-code "${FAKE_INPUT_XML}" "${FAKE_INPUT_CODE}"
  DEPENDS "${FAKE_INPUT_XML}"
  COMMENT "Generating fake-input Wayland protocol code")

add_executable(wlh-input
  input/wlh-input.c
  "${FAKE_INPUT_CODE}"
  "${FAKE_INPUT_HEADER}")
target_include_directories(wlh-input PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(wlh-input PRIVATE -Wall -Wextra)
target_link_libraries(wlh-input PRIVATE PkgConfig::WAYLAND_CLIENT PkgConfig::XKBCOMMON)

add_executable(wlh-probe probe/wlh-probe.cpp)
target_compile_options(wlh-probe PRIVATE -Wall -Wextra)
target_link_libraries(wlh-probe PRIVATE Qt6::Widgets)
