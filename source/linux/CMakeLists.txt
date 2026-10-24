# Platform layer: the Windows API emulated on Linux for the upstream code,
# see docs/port/PLAN.md section 3.3. include/ holds headers named like the
# Windows SDK's (windows.h, shlobj.h, ...) and C++Builder's Windows units
# (Winapi.Windows.hpp, System.Win.ComObj.hpp); they include include/winapi/.

# The version of the upstream WinSCP core, reported as the application's
# version (configuration compatibility depends on it)
file(STRINGS "${PROJECT_SOURCE_DIR}/source/WinSCP.cbproj" OPENSCP_VERSION_KEYS REGEX "<VerInfo_Keys>.*FileVersion=")
if(NOT OPENSCP_VERSION_KEYS MATCHES "FileVersion=([0-9.]+)")
  message(FATAL_ERROR "No FileVersion in source/WinSCP.cbproj")
endif()
set(OPENSCP_CORE_VERSION "${CMAKE_MATCH_1}")
if(OPENSCP_VERSION_KEYS MATCHES "ReleaseType=([a-z]+)")
  set(OPENSCP_RELEASE_TYPE "${CMAKE_MATCH_1}")
else()
  set(OPENSCP_RELEASE_TYPE "stable")
endif()
set(OPENSCP_CORE_VERSION "${OPENSCP_CORE_VERSION}" PARENT_SCOPE)

add_library(openscp_linux STATIC
  src/Com.cpp
  src/Crypt.cpp
  src/FileApi.cpp
  src/Handles.cpp
  src/Process.cpp
  src/Registry.cpp
  src/Shell.cpp
  src/System.cpp
  src/TimeApi.cpp
  src/WinSock.cpp)
add_library(openscp::linux ALIAS openscp_linux)

target_include_directories(openscp_linux PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(openscp_linux PRIVATE
  OPENSCP_CORE_VERSION="${OPENSCP_CORE_VERSION}"
  OPENSCP_RELEASE_TYPE="${OPENSCP_RELEASE_TYPE}")
target_link_libraries(openscp_linux
  PRIVATE openscp::options OpenSSL::Crypto
  PUBLIC openscp::rtl)
