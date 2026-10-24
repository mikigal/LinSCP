# The interface of WinSCP's FileZilla FTP engine (source/filezilla) for the
# core. Until the engine is ported (milestone M4, it is built on MFC), the
# interface is implemented by a stub: the core builds and links, and opening
# an FTP session fails with an error.

set(OPENSCP_FILEZILLA_DIR "${PROJECT_SOURCE_DIR}/source/filezilla")

add_library(openscp_filezilla STATIC FileZillaStub.cpp)
add_library(openscp::filezilla ALIAS openscp_filezilla)

# (FileZillaIntf.h uses the core's transfer event types)
target_include_directories(openscp_filezilla
  PUBLIC "${OPENSCP_FILEZILLA_DIR}"
  PRIVATE "${PROJECT_SOURCE_DIR}/source/core")
target_link_libraries(openscp_filezilla
  PRIVATE openscp::options
  PUBLIC openscp::linux OpenSSL::SSL)
