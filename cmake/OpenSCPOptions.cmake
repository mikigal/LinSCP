# Common compile options for OpenSCP's own targets.
# Link openscp::options to opt a target in; third-party code (PuTTY, neon, ...)
# and test harness tools deliberately do not.

add_library(openscp_options INTERFACE)
add_library(openscp::options ALIAS openscp_options)

target_compile_options(openscp_options INTERFACE
  -Wall
  -Wextra
  # C++Builder-style properties, see docs/port/PLAN.md section 3.2
  $<$<COMPILE_LANGUAGE:CXX>:-fdeclspec>)

if(OPENSCP_SANITIZE)
  target_compile_options(openscp_options INTERFACE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -fno-sanitize-recover=undefined)
  target_link_options(openscp_options INTERFACE
    -fsanitize=address,undefined)
endif()
