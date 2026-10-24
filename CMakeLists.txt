cmake_minimum_required(VERSION 3.28)

project(OpenSCP
  VERSION 0.1.0
  DESCRIPTION "Native Linux port of WinSCP"
  LANGUAGES C CXX)

# The ported core relies on Clang's __declspec(property) support (-fdeclspec)
# to keep C++Builder property use sites unchanged, see docs/port/PLAN.md.
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
  message(FATAL_ERROR
    "OpenSCP must be built with Clang (found C: ${CMAKE_C_COMPILER_ID}, C++: ${CMAKE_CXX_COMPILER_ID}). "
    "Use one of the presets: cmake --preset dev")
endif()

# C++17, the dialect the upstream core is written for (C++Builder's Clang-based
# compilers). C++20 would change overload resolution of the core's mixed-type
# comparisons (rewritten operator== candidates), see docs/port/PROGRESS.md.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(OPENSCP_BUILD_TESTS "Build unit and integration tests" ON)
option(OPENSCP_BUILD_HARNESS "Build the Wayland test harness tools" ON)
option(OPENSCP_SANITIZE "Build OpenSCP targets with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(OpenSCPOptions)
include(OpenSCPStrings)

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
find_package(EXPAT 2.5 REQUIRED)

add_subdirectory(source/rtl)
add_subdirectory(source/libcore)

if(OPENSCP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(OPENSCP_BUILD_HARNESS)
  add_subdirectory(tools/wlharness)
endif()
