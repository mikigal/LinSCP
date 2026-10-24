# libopenscp-core: the converted upstream core (source/core), see
# docs/port/PLAN.md section 3 and docs/port/UPSTREAM.md. The upstream
# directory holds only upstream (converted) sources; the build definition
# lives here.

set(OPENSCP_CORE_DIR "${PROJECT_SOURCE_DIR}/source/core")
set(OPENSCP_RESOURCE_DIR "${PROJECT_SOURCE_DIR}/source/resource")

# Resource strings of the core (LoadStr)
openscp_add_string_tables(openscp_core_strings
  "${OPENSCP_RESOURCE_DIR}/TextsCore1.rc"
  "${OPENSCP_RESOURCE_DIR}/TextsCore2.rc"
  "${OPENSCP_RESOURCE_DIR}/TextsFileZilla.rc")

add_library(openscp_core STATIC
  ${OPENSCP_CORE_DIR}/Bookmarks.cpp
  ${OPENSCP_CORE_DIR}/Common.cpp
  ${OPENSCP_CORE_DIR}/Configuration.cpp
  ${OPENSCP_CORE_DIR}/CoreMain.cpp
  ${OPENSCP_CORE_DIR}/CopyParam.cpp
  ${OPENSCP_CORE_DIR}/Cryptography.cpp
  ${OPENSCP_CORE_DIR}/Exceptions.cpp
  ${OPENSCP_CORE_DIR}/FileBuffer.cpp
  ${OPENSCP_CORE_DIR}/FileInfo.cpp
  ${OPENSCP_CORE_DIR}/FileMasks.cpp
  ${OPENSCP_CORE_DIR}/FileOperationProgress.cpp
  ${OPENSCP_CORE_DIR}/FileSystems.cpp
  ${OPENSCP_CORE_DIR}/FtpFileSystem.cpp
  ${OPENSCP_CORE_DIR}/Global.cpp
  ${OPENSCP_CORE_DIR}/HierarchicalStorage.cpp
  ${OPENSCP_CORE_DIR}/Http.cpp
  ${OPENSCP_CORE_DIR}/NamedObjs.cpp
  ${OPENSCP_CORE_DIR}/NeonIntf.cpp
  ${OPENSCP_CORE_DIR}/Option.cpp
  ${OPENSCP_CORE_DIR}/PuttyIntf.cpp
  ${OPENSCP_CORE_DIR}/Queue.cpp
  ${OPENSCP_CORE_DIR}/RemoteFiles.cpp
  ${OPENSCP_CORE_DIR}/S3FileSystem.cpp
  ${OPENSCP_CORE_DIR}/ScpFileSystem.cpp
  ${OPENSCP_CORE_DIR}/Script.cpp
  ${OPENSCP_CORE_DIR}/SecureShell.cpp
  ${OPENSCP_CORE_DIR}/Security.cpp
  ${OPENSCP_CORE_DIR}/SessionData.cpp
  ${OPENSCP_CORE_DIR}/SessionInfo.cpp
  ${OPENSCP_CORE_DIR}/SftpFileSystem.cpp
  ${OPENSCP_CORE_DIR}/Terminal.cpp
  ${OPENSCP_CORE_DIR}/Usage.cpp
  ${OPENSCP_CORE_DIR}/WebDAVFileSystem.cpp)
add_library(openscp::core ALIAS openscp_core)

target_include_directories(openscp_core PRIVATE "${OPENSCP_CORE_DIR}" "${OPENSCP_RESOURCE_DIR}")
# For consumers, the core's headers are system headers: their upstream
# warnings (see openscp_upstream_warnings) are not reported in the
# consumers' code
target_include_directories(openscp_core SYSTEM INTERFACE "${OPENSCP_CORE_DIR}" "${OPENSCP_RESOURCE_DIR}")
# The string tables register themselves; consumers link their objects directly
target_sources(openscp_core INTERFACE $<TARGET_OBJECTS:openscp_core_strings>)
target_link_libraries(openscp_core
  PRIVATE openscp::options openscp_upstream_warnings
  PUBLIC openscp::linux openscp::putty openscp::neon openscp::libs3 openscp::filezilla OpenSSL::SSL OpenSSL::Crypto)
