# WinSCP's libs3 (libs/libs3): Amazon S3 over WinSCP's neon. As WinSCP's
# makefile (libs/libs3/Makefile, whose source list this reads), compiled as
# C++; without its Windows replacements of POSIX functions
# (src/mingw_functions.c, inc/mingw); include/libs3-linux.h provides the
# Windows functions WinSCP's changes use.

set(LINSCP_LIBS3_DIR "${PROJECT_SOURCE_DIR}/libs/libs3")

file(READ "${LINSCP_LIBS3_DIR}/Makefile" LINSCP_LIBS3_MAKEFILE)
string(REGEX MATCHALL "\\(OBJ_D\\)\\\\[a-z0-9_]+\\.\\$\\(O\\)" LINSCP_LIBS3_OBJECTS "${LINSCP_LIBS3_MAKEFILE}")
set(LINSCP_LIBS3_SOURCES)
foreach(OBJECT IN LISTS LINSCP_LIBS3_OBJECTS)
  string(REGEX REPLACE "^.*\\\\([a-z0-9_]+)\\..*$" "\\1" NAME "${OBJECT}")
  if(NOT NAME STREQUAL "mingw_functions")
    list(APPEND LINSCP_LIBS3_SOURCES "${LINSCP_LIBS3_DIR}/src/${NAME}.c")
  endif()
endforeach()
list(LENGTH LINSCP_LIBS3_SOURCES LINSCP_LIBS3_SOURCE_COUNT)
if(LINSCP_LIBS3_SOURCE_COUNT LESS 10)
  message(FATAL_ERROR "Unexpected libs3 sources in libs/libs3/Makefile: ${LINSCP_LIBS3_SOURCES}")
endif()
set_source_files_properties(${LINSCP_LIBS3_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(linscp_libs3 STATIC ${LINSCP_LIBS3_SOURCES})
add_library(linscp::libs3 ALIAS linscp_libs3)

target_include_directories(linscp_libs3 PUBLIC "${LINSCP_LIBS3_DIR}/inc")
target_compile_definitions(linscp_libs3 PRIVATE WINSCP NE_LFS _GNU_SOURCE)
target_compile_options(linscp_libs3 PRIVATE
  -include "${CMAKE_CURRENT_SOURCE_DIR}/include/libs3-linux.h"
  # As WinSCP's makefile, and variable length arrays (C++ compilation of C code)
  -Wno-deprecated-declarations
  -Wno-vla-cxx-extension)
# libs3 makes variable length arrays of zero elements (empty bucket
# listings), which GCC and Clang allow; not reported by UBSan (linked after
# linscp::sanitize, so that the flag follows -fsanitize=undefined)
add_library(linscp_libs3_sanitize INTERFACE)
if(LINSCP_SANITIZE)
  target_compile_options(linscp_libs3_sanitize INTERFACE -fno-sanitize=vla-bound)
endif()
target_link_libraries(linscp_libs3
  PRIVATE linscp::sanitize linscp_libs3_sanitize linscp::linux
  PUBLIC linscp::neon OpenSSL::SSL OpenSSL::Crypto EXPAT::EXPAT)
