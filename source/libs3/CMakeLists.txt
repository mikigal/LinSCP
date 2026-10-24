# WinSCP's libs3 (libs/libs3): Amazon S3 over WinSCP's neon. As WinSCP's
# makefile (libs/libs3/Makefile, whose source list this reads), compiled as
# C++; without its Windows replacements of POSIX functions
# (src/mingw_functions.c, inc/mingw); include/libs3-linux.h provides the
# Windows functions WinSCP's changes use.

set(OPENSCP_LIBS3_DIR "${PROJECT_SOURCE_DIR}/libs/libs3")

file(READ "${OPENSCP_LIBS3_DIR}/Makefile" OPENSCP_LIBS3_MAKEFILE)
string(REGEX MATCHALL "\\(OBJ_D\\)\\\\[a-z0-9_]+\\.\\$\\(O\\)" OPENSCP_LIBS3_OBJECTS "${OPENSCP_LIBS3_MAKEFILE}")
set(OPENSCP_LIBS3_SOURCES)
foreach(OBJECT IN LISTS OPENSCP_LIBS3_OBJECTS)
  string(REGEX REPLACE "^.*\\\\([a-z0-9_]+)\\..*$" "\\1" NAME "${OBJECT}")
  if(NOT NAME STREQUAL "mingw_functions")
    list(APPEND OPENSCP_LIBS3_SOURCES "${OPENSCP_LIBS3_DIR}/src/${NAME}.c")
  endif()
endforeach()
list(LENGTH OPENSCP_LIBS3_SOURCES OPENSCP_LIBS3_SOURCE_COUNT)
if(OPENSCP_LIBS3_SOURCE_COUNT LESS 10)
  message(FATAL_ERROR "Unexpected libs3 sources in libs/libs3/Makefile: ${OPENSCP_LIBS3_SOURCES}")
endif()
set_source_files_properties(${OPENSCP_LIBS3_SOURCES} PROPERTIES LANGUAGE CXX)

add_library(openscp_libs3 STATIC ${OPENSCP_LIBS3_SOURCES})
add_library(openscp::libs3 ALIAS openscp_libs3)

target_include_directories(openscp_libs3 PUBLIC "${OPENSCP_LIBS3_DIR}/inc")
target_compile_definitions(openscp_libs3 PRIVATE WINSCP NE_LFS _GNU_SOURCE)
target_compile_options(openscp_libs3 PRIVATE
  -include "${CMAKE_CURRENT_SOURCE_DIR}/include/libs3-linux.h"
  # As WinSCP's makefile, and variable length arrays (C++ compilation of C code)
  -Wno-deprecated-declarations
  -Wno-vla-cxx-extension)
target_link_libraries(openscp_libs3
  PRIVATE openscp::sanitize openscp::linux
  PUBLIC openscp::neon OpenSSL::SSL OpenSSL::Crypto EXPAT::EXPAT)
