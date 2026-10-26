# LinSCP's version (project() in the top-level CMakeLists.txt) against the
# version of the upstream WinSCP core it is built on (FileVersion of
# source/WinSCP.cbproj, X.Y.Z): LinSCP's version is X.Y.Z for the first
# release on that core and X.Y.Z.N for its further releases (N from 1), see
# docs/port/PLAN.md. A merge of upstream that changes the core's version stops
# the configuration until LinSCP's version is set to it (docs/port/UPSTREAM.md).
#
# Sets LINSCP_CORE_VERSION (X.Y.Z), LINSCP_FILE_VERSION (LinSCP's version in
# four parts, X.Y.Z.N or X.Y.Z.0, for the version information of the
# executables) and LINSCP_RELEASE_TYPE (upstream's ReleaseType).

file(STRINGS "${PROJECT_SOURCE_DIR}/source/WinSCP.cbproj" LINSCP_VERSION_KEYS
  REGEX "<VerInfo_Keys>.*FileVersion=")
if(NOT LINSCP_VERSION_KEYS MATCHES "FileVersion=([0-9]+\\.[0-9]+\\.[0-9]+)")
  message(FATAL_ERROR "No FileVersion in source/WinSCP.cbproj")
endif()
set(LINSCP_CORE_VERSION "${CMAKE_MATCH_1}")
if(LINSCP_VERSION_KEYS MATCHES "ReleaseType=([a-z]+)")
  set(LINSCP_RELEASE_TYPE "${CMAKE_MATCH_1}")
else()
  set(LINSCP_RELEASE_TYPE "stable")
endif()

if(NOT "${LinSCP_VERSION_MAJOR}.${LinSCP_VERSION_MINOR}.${LinSCP_VERSION_PATCH}" STREQUAL "${LINSCP_CORE_VERSION}"
    OR "${LinSCP_VERSION_TWEAK}" STREQUAL "0")
  message(FATAL_ERROR
    "LinSCP's version ${LinSCP_VERSION} (project() in CMakeLists.txt) is not of the core, "
    "${LINSCP_CORE_VERSION} (source/WinSCP.cbproj): it must be ${LINSCP_CORE_VERSION}, "
    "or ${LINSCP_CORE_VERSION}.N (N from 1) for a further release of LinSCP on that core, "
    "see docs/port/UPSTREAM.md")
endif()

if("${LinSCP_VERSION_TWEAK}" STREQUAL "")
  set(LINSCP_FILE_VERSION "${LinSCP_VERSION}.0")
else()
  set(LINSCP_FILE_VERSION "${LinSCP_VERSION}")
endif()
