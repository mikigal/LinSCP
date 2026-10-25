# WinSCP's PuTTY (source/putty) on Linux. The portable sources are those of
# WinSCP's own PuTTY project (source/Putty.cbproj), so that upstream changes
# to the list arrive with merges; its Windows platform sources
# (putty/windows) are replaced by putty/unix, which derives from PuTTY
# 0.85's Unix platform code with WinSCP's changes.

set(LINSCP_PUTTY_DIR "${PROJECT_SOURCE_DIR}/source/putty")

file(STRINGS "${PROJECT_SOURCE_DIR}/source/Putty.cbproj" LINSCP_PUTTY_PROJECT_LINES
  REGEX "<C(pp)?Compile Include=\"putty\\\\")
set(LINSCP_PUTTY_SOURCES)
foreach(LINE IN LISTS LINSCP_PUTTY_PROJECT_LINES)
  if(LINE MATCHES "Include=\"putty\\\\([^\"]+)\"")
    string(REPLACE "\\" "/" SOURCE "${CMAKE_MATCH_1}")
    if(NOT SOURCE MATCHES "^windows/")
      list(APPEND LINSCP_PUTTY_SOURCES "${LINSCP_PUTTY_DIR}/${SOURCE}")
    endif()
  endif()
endforeach()
list(LENGTH LINSCP_PUTTY_SOURCES LINSCP_PUTTY_SOURCE_COUNT)
if(LINSCP_PUTTY_SOURCE_COUNT LESS 100)
  message(FATAL_ERROR "Unexpected PuTTY sources in source/Putty.cbproj: ${LINSCP_PUTTY_SOURCES}")
endif()

file(GLOB LINSCP_PUTTY_UNIX_SOURCES CONFIGURE_DEPENDS
  "${LINSCP_PUTTY_DIR}/unix/*.c"
  "${LINSCP_PUTTY_DIR}/unix/utils/*.c")

add_library(linscp_putty STATIC
  ${LINSCP_PUTTY_SOURCES}
  ${LINSCP_PUTTY_UNIX_SOURCES}
  # Portable despite its directory
  "${LINSCP_PUTTY_DIR}/windows/utils/escape_registry_key.c")
add_library(linscp::putty ALIAS linscp_putty)

target_include_directories(linscp_putty PUBLIC "${LINSCP_PUTTY_DIR}" "${LINSCP_PUTTY_DIR}/unix")
set_target_properties(linscp_putty PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
# As WinSCP's project; the Unix configuration that PuTTY's CMake would
# detect (cmake.h) is fixed for Linux
# WINSCP for every user of PuTTY's headers, as in WinSCP's projects; MPEXT
# only here (the core's PuttyIntf.h defines it itself)
target_compile_definitions(linscp_putty
  PUBLIC WINSCP
  PRIVATE
    MPEXT
    HAVE_CLOCK_GETTIME=1
    HAVE_CLOCK_MONOTONIC=1
    HAVE_ENDPWENT=1
    HAVE_SETPWENT=1
    HAVE_SO_PEERCRED=1)
target_compile_options(linscp_putty PRIVATE -Wno-unknown-pragmas)
# AES-NI is built for 64-bit targets only (_WIN64 means that here)
set_source_files_properties("${LINSCP_PUTTY_DIR}/crypto/aes-ni.c" PROPERTIES COMPILE_DEFINITIONS _WIN64)
# The core defines appname (PuttyIntf.cpp); C++Builder's linker takes the
# first of the duplicate definitions, GNU ld does not
set_source_files_properties("${LINSCP_PUTTY_DIR}/be_list.c" PROPERTIES COMPILE_DEFINITIONS appname=be_list_appname)
# EnterCriticalSection and friends
target_link_libraries(linscp_putty PRIVATE linscp::sanitize PUBLIC linscp::linux ${CMAKE_DL_LIBS})
