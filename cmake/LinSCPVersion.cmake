# LinSCP's version (project() in the top-level CMakeLists.txt) against the
# version of the upstream WinSCP core it is built on (FileVersion of
# source/WinSCP.cbproj, X.Y.Z): LinSCP's version is X.Y.Z.N, N being the
# number of LinSCP's release on that core (from 1), see docs/port/PLAN.md. A
# merge of upstream that changes the core's version stops the configuration
# until LinSCP's version is set to the new X.Y.Z.1 (docs/port/UPSTREAM.md).
#
# Sets LINSCP_CORE_VERSION (X.Y.Z) and LINSCP_RELEASE_TYPE (upstream's
# ReleaseType).

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
    OR NOT "${LinSCP_VERSION_TWEAK}" MATCHES "^[1-9][0-9]*$")
  message(FATAL_ERROR
    "LinSCP's version ${LinSCP_VERSION} (project() in CMakeLists.txt) must be "
    "${LINSCP_CORE_VERSION}.N: the core's version (source/WinSCP.cbproj) and the number of "
    "LinSCP's release on that core (N from 1), see docs/port/UPSTREAM.md")
endif()
