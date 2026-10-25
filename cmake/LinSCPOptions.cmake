# Common compile options for LinSCP's own targets.
# Link linscp::options to opt a target in; third-party code (PuTTY, neon, ...)
# and test harness tools deliberately do not (third-party code links
# linscp::sanitize, the sanitizers only).

add_library(linscp_options INTERFACE)
add_library(linscp::options ALIAS linscp_options)

target_compile_options(linscp_options INTERFACE
  -Wall
  -Wextra
  # C++Builder-style properties, see docs/port/PLAN.md section 3.2
  $<$<COMPILE_LANGUAGE:CXX>:-fdeclspec>)

# Sanitizers only, also for third-party code (PuTTY, neon, ...)
add_library(linscp_sanitize INTERFACE)
add_library(linscp::sanitize ALIAS linscp_sanitize)
target_link_libraries(linscp_options INTERFACE linscp_sanitize)

if(LINSCP_SANITIZE)
  target_compile_options(linscp_sanitize INTERFACE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -fno-sanitize-recover=undefined)
  target_link_options(linscp_sanitize INTERFACE
    -fsanitize=address,undefined)
endif()

# Upstream code (the core, the FileZilla engine, the ported application
# files): C++Builder pragmas, and warnings we do not fix in upstream
# sources (designated initializers out of order in MakeMethod, unhandled
# enumerators, signedness, casts of GetProcAddress results, memset/memcpy
# of C++ objects, NULL for reserved DWORD arguments and compared with bools,
# variable length arrays). An interface library
# linked after linscp::options, so that the flags follow -Wall.
add_library(linscp_upstream_warnings INTERFACE)
target_compile_options(linscp_upstream_warnings INTERFACE
  -Wno-unknown-pragmas
  -Wno-reorder-init-list
  -Wno-switch
  -Wno-sign-compare
  -Wno-cast-function-type-mismatch
  -Wno-nontrivial-memcall
  -Wno-null-conversion
  -Wno-null-arithmetic
  -Wno-vla-cxx-extension)
