# Platform layer: the Windows API emulated on Linux for the upstream code,
# see docs/port/PLAN.md section 3.3. include/ holds headers named like the
# Windows SDK's (windows.h, shlobj.h, ...) and C++Builder's Windows units
# (Winapi.Windows.hpp, System.Win.ComObj.hpp); they include include/winapi/.

add_library(linscp_linux STATIC
  src/Com.cpp
  src/Crt.cpp
  src/Crypt.cpp
  src/FileApi.cpp
  src/Handles.cpp
  src/Messages.cpp
  src/Process.cpp
  src/Registry.cpp
  src/Shell.cpp
  src/System.cpp
  src/TimeApi.cpp
  src/WinSock.cpp)
add_library(linscp::linux ALIAS linscp_linux)

target_include_directories(linscp_linux PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
# The version information of the executables: LinSCP's version, see
# cmake/LinSCPVersion.cmake (configuration compatibility depends on the
# core's version, its first three parts)
target_compile_definitions(linscp_linux PRIVATE
  LINSCP_VERSION="${LinSCP_VERSION}"
  LINSCP_RELEASE_TYPE="${LINSCP_RELEASE_TYPE}")
target_link_libraries(linscp_linux
  PRIVATE linscp::options OpenSSL::Crypto
  PUBLIC linscp::rtl)
