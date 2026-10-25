# WinSCP's FileZilla FTP engine (source/filezilla) and the subset of MFC it
# uses (libs/mfc, which upstream already reduced to CString, CFile and CTime
# on top of the VCL). The engine runs on a thread of its own with a Windows
# message loop: window messages, timers and WSAAsyncSelect come from the
# platform layer (source/linux, winapi/Messages.h).

set(LINSCP_FILEZILLA_DIR "${PROJECT_SOURCE_DIR}/source/filezilla")
set(LINSCP_MFC_DIR "${PROJECT_SOURCE_DIR}/libs/mfc")

add_library(linscp_mfc STATIC
  ${LINSCP_MFC_DIR}/source/filecore.cpp
  ${LINSCP_MFC_DIR}/source/strcore.cpp
  ${LINSCP_MFC_DIR}/source/strex.cpp
  ${LINSCP_MFC_DIR}/source/timecore.cpp)
target_include_directories(linscp_mfc PUBLIC "${LINSCP_MFC_DIR}/include")
target_link_libraries(linscp_mfc
  PRIVATE linscp::options
  PUBLIC linscp::linux)

add_library(linscp_filezilla STATIC
  ${LINSCP_FILEZILLA_DIR}/ApiLog.cpp
  ${LINSCP_FILEZILLA_DIR}/AsyncProxySocketLayer.cpp
  ${LINSCP_FILEZILLA_DIR}/AsyncSocketEx.cpp
  ${LINSCP_FILEZILLA_DIR}/AsyncSocketExLayer.cpp
  ${LINSCP_FILEZILLA_DIR}/AsyncSslSocketLayer.cpp
  ${LINSCP_FILEZILLA_DIR}/FileZillaApi.cpp
  ${LINSCP_FILEZILLA_DIR}/FileZillaIntern.cpp
  ${LINSCP_FILEZILLA_DIR}/FileZillaIntf.cpp
  ${LINSCP_FILEZILLA_DIR}/FtpControlSocket.cpp
  ${LINSCP_FILEZILLA_DIR}/FtpListResult.cpp
  ${LINSCP_FILEZILLA_DIR}/FzApiStructures.cpp
  ${LINSCP_FILEZILLA_DIR}/MainThread.cpp
  ${LINSCP_FILEZILLA_DIR}/MFC64bitFix.cpp
  ${LINSCP_FILEZILLA_DIR}/ServerPath.cpp
  ${LINSCP_FILEZILLA_DIR}/structures.cpp
  ${LINSCP_FILEZILLA_DIR}/TransferSocket.cpp)
add_library(linscp::filezilla ALIAS linscp_filezilla)

# (The engine includes the core's headers, and FileZillaIntf.h, which the
# core includes, uses the core's transfer event types)
target_include_directories(linscp_filezilla
  PUBLIC "${LINSCP_FILEZILLA_DIR}"
  PRIVATE "${PROJECT_SOURCE_DIR}/source/core" "${PROJECT_SOURCE_DIR}/source/resource")
# As FileZilla.cbproj (WIN32 aside)
target_compile_definitions(linscp_filezilla PRIVATE
  WINSCP UNICODE OPENSSL_API_COMPAT=11000 OPENSSL_NO_DEPRECATED)
# Warnings of the engine's sources that we do not fix, in addition to those
# of linscp_upstream_warnings (linked after linscp::options, so that the
# flags follow -Wall)
add_library(linscp_filezilla_warnings INTERFACE)
target_compile_options(linscp_filezilla_warnings INTERFACE
  -Wno-overloaded-virtual
  -Wno-extra-tokens
  -Wno-misleading-indentation
  -Wno-unused-but-set-variable
  -Wno-dangling-else)
target_link_libraries(linscp_filezilla
  PRIVATE linscp::options linscp_upstream_warnings linscp_filezilla_warnings
  PUBLIC linscp_mfc linscp::linux OpenSSL::SSL OpenSSL::Crypto)
