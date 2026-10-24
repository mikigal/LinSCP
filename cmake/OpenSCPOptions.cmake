# Common compile options for OpenSCP's own targets.
# Link openscp::options to opt a target in; third-party code (PuTTY, neon, ...)
# and test harness tools deliberately do not (third-party code links
# openscp::sanitize, the sanitizers only).

add_library(openscp_options INTERFACE)
add_library(openscp::options ALIAS openscp_options)

target_compile_options(openscp_options INTERFACE
  -Wall
  -Wextra
  # C++Builder-style properties, see docs/port/PLAN.md section 3.2
  $<$<COMPILE_LANGUAGE:CXX>:-fdeclspec>)

# Sanitizers only, also for third-party code (PuTTY, neon, ...)
add_library(openscp_sanitize INTERFACE)
add_library(openscp::sanitize ALIAS openscp_sanitize)
target_link_libraries(openscp_options INTERFACE openscp_sanitize)

if(OPENSCP_SANITIZE)
  target_compile_options(openscp_sanitize INTERFACE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -fno-sanitize-recover=undefined)
  target_link_options(openscp_sanitize INTERFACE
    -fsanitize=address,undefined)
endif()
