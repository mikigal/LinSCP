# liblinscp-core: the converted upstream core (source/core), see
# docs/port/PLAN.md section 3 and docs/port/UPSTREAM.md. The upstream
# directory holds only upstream (converted) sources; the build definition
# lives here.

set(LINSCP_CORE_DIR "${PROJECT_SOURCE_DIR}/source/core")
set(LINSCP_RESOURCE_DIR "${PROJECT_SOURCE_DIR}/source/resource")

# Resource strings of the core (LoadStr)
linscp_add_string_tables(linscp_core_strings
  "${LINSCP_RESOURCE_DIR}/TextsCore1.rc"
  "${LINSCP_RESOURCE_DIR}/TextsCore2.rc"
  "${LINSCP_RESOURCE_DIR}/TextsFileZilla.rc")

add_library(linscp_core STATIC
  ${LINSCP_CORE_DIR}/Bookmarks.cpp
  ${LINSCP_CORE_DIR}/Common.cpp
  ${LINSCP_CORE_DIR}/Configuration.cpp
  ${LINSCP_CORE_DIR}/CoreMain.cpp
  ${LINSCP_CORE_DIR}/CopyParam.cpp
  ${LINSCP_CORE_DIR}/Cryptography.cpp
  ${LINSCP_CORE_DIR}/Exceptions.cpp
  ${LINSCP_CORE_DIR}/FileBuffer.cpp
  ${LINSCP_CORE_DIR}/FileInfo.cpp
  ${LINSCP_CORE_DIR}/FileMasks.cpp
  ${LINSCP_CORE_DIR}/FileOperationProgress.cpp
  ${LINSCP_CORE_DIR}/FileSystems.cpp
  ${LINSCP_CORE_DIR}/FtpFileSystem.cpp
  ${LINSCP_CORE_DIR}/Global.cpp
  ${LINSCP_CORE_DIR}/HierarchicalStorage.cpp
  ${LINSCP_CORE_DIR}/Http.cpp
  ${LINSCP_CORE_DIR}/NamedObjs.cpp
  ${LINSCP_CORE_DIR}/NeonIntf.cpp
  ${LINSCP_CORE_DIR}/Option.cpp
  ${LINSCP_CORE_DIR}/PuttyIntf.cpp
  ${LINSCP_CORE_DIR}/Queue.cpp
  ${LINSCP_CORE_DIR}/RemoteFiles.cpp
  ${LINSCP_CORE_DIR}/S3FileSystem.cpp
  ${LINSCP_CORE_DIR}/ScpFileSystem.cpp
  ${LINSCP_CORE_DIR}/Script.cpp
  ${LINSCP_CORE_DIR}/SecureShell.cpp
  ${LINSCP_CORE_DIR}/Security.cpp
  ${LINSCP_CORE_DIR}/SessionData.cpp
  ${LINSCP_CORE_DIR}/SessionInfo.cpp
  ${LINSCP_CORE_DIR}/SftpFileSystem.cpp
  ${LINSCP_CORE_DIR}/Terminal.cpp
  ${LINSCP_CORE_DIR}/Usage.cpp
  ${LINSCP_CORE_DIR}/WebDAVFileSystem.cpp)
add_library(linscp::core ALIAS linscp_core)
# Upstream's official builds: the version is not tagged as a development
# build not to be distributed
if(LINSCP_RELEASE_BUILD)
  target_compile_definitions(linscp_core PRIVATE BUILD_OFFICIAL)
endif()

target_include_directories(linscp_core PRIVATE "${LINSCP_CORE_DIR}" "${LINSCP_RESOURCE_DIR}")
# For consumers, the core's headers are system headers: their upstream
# warnings (see linscp_upstream_warnings) are not reported in the
# consumers' code
target_include_directories(linscp_core SYSTEM INTERFACE "${LINSCP_CORE_DIR}" "${LINSCP_RESOURCE_DIR}")
# The string tables register themselves; consumers link their objects directly
target_sources(linscp_core INTERFACE $<TARGET_OBJECTS:linscp_core_strings>)
target_link_libraries(linscp_core
  PRIVATE linscp::options linscp_upstream_warnings
  PUBLIC linscp::linux linscp::putty linscp::neon linscp::libs3 linscp::filezilla OpenSSL::SSL OpenSSL::Crypto)
