# WinSCP's neon (libs/neon): HTTP and WebDAV, with WinSCP's changes. The
# sources are those of WinSCP's makefile (libs/neon/Makefile.bcb), so that
# upstream changes to the list arrive with merges; libs/neon/src/config.h
# includes include/neon-linux-config.h on Linux.

set(LINSCP_NEON_DIR "${PROJECT_SOURCE_DIR}/libs/neon")

# (read as a whole: the lines end with backslashes, which would escape the
# separators of a CMake list)
file(READ "${LINSCP_NEON_DIR}/Makefile.bcb" LINSCP_NEON_MAKEFILE)
string(REGEX MATCHALL "\\(OBJ_D\\)\\\\ne_[a-z0-9_]+" LINSCP_NEON_OBJECTS "${LINSCP_NEON_MAKEFILE}")
set(LINSCP_NEON_SOURCES)
foreach(OBJECT IN LISTS LINSCP_NEON_OBJECTS)
  string(REGEX REPLACE "^.*(ne_[a-z0-9_]+)$" "\\1" NAME "${OBJECT}")
  list(APPEND LINSCP_NEON_SOURCES "${LINSCP_NEON_DIR}/src/${NAME}.c")
endforeach()
list(LENGTH LINSCP_NEON_SOURCES LINSCP_NEON_SOURCE_COUNT)
if(LINSCP_NEON_SOURCE_COUNT LESS 20)
  message(FATAL_ERROR "Unexpected neon sources in libs/neon/Makefile.bcb: ${LINSCP_NEON_SOURCES}")
endif()

add_library(linscp_neon STATIC ${LINSCP_NEON_SOURCES} linux-compat.c)
add_library(linscp::neon ALIAS linscp_neon)

target_include_directories(linscp_neon
  PUBLIC "${LINSCP_NEON_DIR}/src"
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" "${LINSCP_NEON_DIR}/src")
set_target_properties(linscp_neon PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
# As WinSCP's makefile, without the Windows specifics
# As WinSCP's makefile: warnings of the upstream code
target_compile_options(linscp_neon PRIVATE -Wno-pointer-sign -Wno-incompatible-pointer-types -Wno-typedef-redefinition)
target_compile_definitions(linscp_neon PRIVATE
  NE_HAVE_SSL HAVE_OPENSSL HAVE_EXPAT HAVE_EXPAT_H NE_HAVE_DAV NE_LFS HAVE_STRTOLL WINSCP
  _GNU_SOURCE _LARGEFILE64_SOURCE)
target_link_libraries(linscp_neon
  PRIVATE linscp::sanitize
  PUBLIC OpenSSL::SSL OpenSSL::Crypto EXPAT::EXPAT)
