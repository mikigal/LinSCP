# libopenscp-core: the converted upstream core (source/core), see
# docs/port/PLAN.md section 3 and docs/port/UPSTREAM.md. The upstream
# directory holds only upstream (converted) sources; the build definition
# lives here.

set(OPENSCP_CORE_DIR "${PROJECT_SOURCE_DIR}/source/core")
set(OPENSCP_RESOURCE_DIR "${PROJECT_SOURCE_DIR}/source/resource")

# Resource strings of the core (LoadStr)
openscp_add_string_tables(openscp_core_strings
  "${OPENSCP_RESOURCE_DIR}/TextsCore1.rc"
  "${OPENSCP_RESOURCE_DIR}/TextsCore2.rc")

# The sources that compile so far; the protocol modules follow with their
# third-party libraries (PuTTY, neon, libs3, FileZilla)
add_library(openscp_core STATIC
  ${OPENSCP_CORE_DIR}/Bookmarks.cpp
  ${OPENSCP_CORE_DIR}/Common.cpp
  ${OPENSCP_CORE_DIR}/CopyParam.cpp
  ${OPENSCP_CORE_DIR}/Exceptions.cpp
  ${OPENSCP_CORE_DIR}/FileBuffer.cpp
  ${OPENSCP_CORE_DIR}/FileInfo.cpp
  ${OPENSCP_CORE_DIR}/FileMasks.cpp
  ${OPENSCP_CORE_DIR}/FileOperationProgress.cpp
  ${OPENSCP_CORE_DIR}/FileSystems.cpp
  ${OPENSCP_CORE_DIR}/Global.cpp
  ${OPENSCP_CORE_DIR}/NamedObjs.cpp
  ${OPENSCP_CORE_DIR}/Option.cpp
  ${OPENSCP_CORE_DIR}/Queue.cpp
  ${OPENSCP_CORE_DIR}/RemoteFiles.cpp
  ${OPENSCP_CORE_DIR}/ScpFileSystem.cpp
  ${OPENSCP_CORE_DIR}/Script.cpp
  ${OPENSCP_CORE_DIR}/Security.cpp
  ${OPENSCP_CORE_DIR}/SessionInfo.cpp
  ${OPENSCP_CORE_DIR}/SftpFileSystem.cpp
  ${OPENSCP_CORE_DIR}/Usage.cpp)
add_library(openscp::core ALIAS openscp_core)

target_include_directories(openscp_core PUBLIC "${OPENSCP_CORE_DIR}" "${OPENSCP_RESOURCE_DIR}")
# The string tables register themselves; consumers link their objects directly
target_sources(openscp_core INTERFACE $<TARGET_OBJECTS:openscp_core_strings>)
# Upstream code: C++Builder pragmas, and warnings we do not fix in upstream
# sources (designated initializers out of order in MakeMethod, unhandled
# enumerators, signedness, casts of GetProcAddress results). An interface
# library linked after openscp::options, so that the flags follow -Wall.
add_library(openscp_upstream_warnings INTERFACE)
target_compile_options(openscp_upstream_warnings INTERFACE
  -Wno-unknown-pragmas
  -Wno-reorder-init-list
  -Wno-switch
  -Wno-sign-compare
  -Wno-cast-function-type-mismatch)

target_link_libraries(openscp_core
  PRIVATE openscp::options openscp_upstream_warnings
  PUBLIC openscp::linux OpenSSL::SSL OpenSSL::Crypto)
