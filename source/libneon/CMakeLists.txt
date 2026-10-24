# WinSCP's neon (libs/neon): HTTP and WebDAV, with WinSCP's changes. The
# sources are those of WinSCP's makefile (libs/neon/Makefile.bcb), so that
# upstream changes to the list arrive with merges; libs/neon/src/config.h
# includes include/neon-linux-config.h on Linux.

set(OPENSCP_NEON_DIR "${PROJECT_SOURCE_DIR}/libs/neon")

# (read as a whole: the lines end with backslashes, which would escape the
# separators of a CMake list)
file(READ "${OPENSCP_NEON_DIR}/Makefile.bcb" OPENSCP_NEON_MAKEFILE)
string(REGEX MATCHALL "\\(OBJ_D\\)\\\\ne_[a-z0-9_]+" OPENSCP_NEON_OBJECTS "${OPENSCP_NEON_MAKEFILE}")
set(OPENSCP_NEON_SOURCES)
foreach(OBJECT IN LISTS OPENSCP_NEON_OBJECTS)
  string(REGEX REPLACE "^.*(ne_[a-z0-9_]+)$" "\\1" NAME "${OBJECT}")
  list(APPEND OPENSCP_NEON_SOURCES "${OPENSCP_NEON_DIR}/src/${NAME}.c")
endforeach()
list(LENGTH OPENSCP_NEON_SOURCES OPENSCP_NEON_SOURCE_COUNT)
if(OPENSCP_NEON_SOURCE_COUNT LESS 20)
  message(FATAL_ERROR "Unexpected neon sources in libs/neon/Makefile.bcb: ${OPENSCP_NEON_SOURCES}")
endif()

add_library(openscp_neon STATIC ${OPENSCP_NEON_SOURCES} linux-compat.c)
add_library(openscp::neon ALIAS openscp_neon)

target_include_directories(openscp_neon
  PUBLIC "${OPENSCP_NEON_DIR}/src"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${OPENSCP_NEON_DIR}/src")
set_target_properties(openscp_neon PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
# As WinSCP's makefile, without the Windows specifics
# As WinSCP's makefile: warnings of the upstream code
target_compile_options(openscp_neon PRIVATE -Wno-pointer-sign -Wno-incompatible-pointer-types -Wno-typedef-redefinition)
target_compile_definitions(openscp_neon PRIVATE
  NE_HAVE_SSL HAVE_OPENSSL HAVE_EXPAT HAVE_EXPAT_H NE_HAVE_DAV NE_LFS HAVE_STRTOLL WINSCP
  _GNU_SOURCE _LARGEFILE64_SOURCE)
target_link_libraries(openscp_neon
  PRIVATE openscp::sanitize
  PUBLIC OpenSSL::SSL OpenSSL::Crypto EXPAT::EXPAT)
