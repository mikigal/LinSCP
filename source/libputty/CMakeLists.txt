# WinSCP's PuTTY (source/putty) on Linux. The portable sources are those of
# WinSCP's own PuTTY project (source/Putty.cbproj), so that upstream changes
# to the list arrive with merges; its Windows platform sources
# (putty/windows) are replaced by putty/unix, which derives from PuTTY
# 0.85's Unix platform code with WinSCP's changes.

set(OPENSCP_PUTTY_DIR "${PROJECT_SOURCE_DIR}/source/putty")

file(STRINGS "${PROJECT_SOURCE_DIR}/source/Putty.cbproj" OPENSCP_PUTTY_PROJECT_LINES
  REGEX "<C(pp)?Compile Include=\"putty\\\\")
set(OPENSCP_PUTTY_SOURCES)
foreach(LINE IN LISTS OPENSCP_PUTTY_PROJECT_LINES)
  if(LINE MATCHES "Include=\"putty\\\\([^\"]+)\"")
    string(REPLACE "\\" "/" SOURCE "${CMAKE_MATCH_1}")
    if(NOT SOURCE MATCHES "^windows/")
      list(APPEND OPENSCP_PUTTY_SOURCES "${OPENSCP_PUTTY_DIR}/${SOURCE}")
    endif()
  endif()
endforeach()
list(LENGTH OPENSCP_PUTTY_SOURCES OPENSCP_PUTTY_SOURCE_COUNT)
if(OPENSCP_PUTTY_SOURCE_COUNT LESS 100)
  message(FATAL_ERROR "Unexpected PuTTY sources in source/Putty.cbproj: ${OPENSCP_PUTTY_SOURCES}")
endif()

file(GLOB OPENSCP_PUTTY_UNIX_SOURCES CONFIGURE_DEPENDS
  "${OPENSCP_PUTTY_DIR}/unix/*.c"
  "${OPENSCP_PUTTY_DIR}/unix/utils/*.c")

add_library(openscp_putty STATIC
  ${OPENSCP_PUTTY_SOURCES}
  ${OPENSCP_PUTTY_UNIX_SOURCES}
  # Portable despite its directory
  "${OPENSCP_PUTTY_DIR}/windows/utils/escape_registry_key.c")
add_library(openscp::putty ALIAS openscp_putty)

target_include_directories(openscp_putty PUBLIC "${OPENSCP_PUTTY_DIR}" "${OPENSCP_PUTTY_DIR}/unix")
set_target_properties(openscp_putty PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
# As WinSCP's project; the Unix configuration that PuTTY's CMake would
# detect (cmake.h) is fixed for Linux
# WINSCP for every user of PuTTY's headers, as in WinSCP's projects; MPEXT
# only here (the core's PuttyIntf.h defines it itself)
target_compile_definitions(openscp_putty
  PUBLIC WINSCP
  PRIVATE
    MPEXT
    HAVE_CLOCK_GETTIME=1
    HAVE_CLOCK_MONOTONIC=1
    HAVE_ENDPWENT=1
    HAVE_SETPWENT=1
    HAVE_SO_PEERCRED=1)
target_compile_options(openscp_putty PRIVATE -Wno-unknown-pragmas)
# AES-NI is built for 64-bit targets only (_WIN64 means that here)
set_source_files_properties("${OPENSCP_PUTTY_DIR}/crypto/aes-ni.c" PROPERTIES COMPILE_DEFINITIONS _WIN64)
# The core defines appname (PuttyIntf.cpp); C++Builder's linker takes the
# first of the duplicate definitions, GNU ld does not
set_source_files_properties("${OPENSCP_PUTTY_DIR}/be_list.c" PROPERTIES COMPILE_DEFINITIONS appname=be_list_appname)
# EnterCriticalSection and friends
target_link_libraries(openscp_putty PRIVATE openscp::sanitize PUBLIC openscp::linux ${CMAKE_DL_LIBS})
