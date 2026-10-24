# WinSCP's FileZilla FTP engine (source/filezilla) and the subset of MFC it
# uses (libs/mfc, which upstream already reduced to CString, CFile and CTime
# on top of the VCL). The engine runs on a thread of its own with a Windows
# message loop: window messages, timers and WSAAsyncSelect come from the
# platform layer (source/linux, winapi/Messages.h).

set(OPENSCP_FILEZILLA_DIR "${PROJECT_SOURCE_DIR}/source/filezilla")
set(OPENSCP_MFC_DIR "${PROJECT_SOURCE_DIR}/libs/mfc")

add_library(openscp_mfc STATIC
  ${OPENSCP_MFC_DIR}/source/filecore.cpp
  ${OPENSCP_MFC_DIR}/source/strcore.cpp
  ${OPENSCP_MFC_DIR}/source/strex.cpp
  ${OPENSCP_MFC_DIR}/source/timecore.cpp)
target_include_directories(openscp_mfc PUBLIC "${OPENSCP_MFC_DIR}/include")
target_link_libraries(openscp_mfc
  PRIVATE openscp::options
  PUBLIC openscp::linux)

add_library(openscp_filezilla STATIC
  ${OPENSCP_FILEZILLA_DIR}/ApiLog.cpp
  ${OPENSCP_FILEZILLA_DIR}/AsyncProxySocketLayer.cpp
  ${OPENSCP_FILEZILLA_DIR}/AsyncSocketEx.cpp
  ${OPENSCP_FILEZILLA_DIR}/AsyncSocketExLayer.cpp
  ${OPENSCP_FILEZILLA_DIR}/AsyncSslSocketLayer.cpp
  ${OPENSCP_FILEZILLA_DIR}/FileZillaApi.cpp
  ${OPENSCP_FILEZILLA_DIR}/FileZillaIntern.cpp
  ${OPENSCP_FILEZILLA_DIR}/FileZillaIntf.cpp
  ${OPENSCP_FILEZILLA_DIR}/FtpControlSocket.cpp
  ${OPENSCP_FILEZILLA_DIR}/FtpListResult.cpp
  ${OPENSCP_FILEZILLA_DIR}/FzApiStructures.cpp
  ${OPENSCP_FILEZILLA_DIR}/MainThread.cpp
  ${OPENSCP_FILEZILLA_DIR}/MFC64bitFix.cpp
  ${OPENSCP_FILEZILLA_DIR}/ServerPath.cpp
  ${OPENSCP_FILEZILLA_DIR}/structures.cpp
  ${OPENSCP_FILEZILLA_DIR}/TransferSocket.cpp)
add_library(openscp::filezilla ALIAS openscp_filezilla)

# (The engine includes the core's headers, and FileZillaIntf.h, which the
# core includes, uses the core's transfer event types)
target_include_directories(openscp_filezilla
  PUBLIC "${OPENSCP_FILEZILLA_DIR}"
  PRIVATE "${PROJECT_SOURCE_DIR}/source/core" "${PROJECT_SOURCE_DIR}/source/resource")
# As FileZilla.cbproj (WIN32 aside)
target_compile_definitions(openscp_filezilla PRIVATE
  WINSCP UNICODE OPENSSL_API_COMPAT=11000 OPENSSL_NO_DEPRECATED)
# Warnings of the engine's sources that we do not fix, in addition to those
# of openscp_upstream_warnings (linked after openscp::options, so that the
# flags follow -Wall)
add_library(openscp_filezilla_warnings INTERFACE)
target_compile_options(openscp_filezilla_warnings INTERFACE
  -Wno-overloaded-virtual
  -Wno-extra-tokens
  -Wno-misleading-indentation
  -Wno-unused-but-set-variable
  -Wno-dangling-else)
target_link_libraries(openscp_filezilla
  PRIVATE openscp::options openscp_upstream_warnings openscp_filezilla_warnings
  PUBLIC openscp_mfc openscp::linux OpenSSL::SSL OpenSSL::Crypto)
