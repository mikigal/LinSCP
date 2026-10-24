# libopenscp-core: the converted upstream core (source/core), see
# docs/port/PLAN.md section 3 and docs/port/UPSTREAM.md. The upstream
# directory holds only upstream (converted) sources; the build definition
# lives here.

set(OPENSCP_RESOURCE_DIR "${PROJECT_SOURCE_DIR}/source/resource")

# Resource strings of the core (LoadStr)
openscp_add_string_tables(openscp_core_strings
  "${OPENSCP_RESOURCE_DIR}/TextsCore1.rc"
  "${OPENSCP_RESOURCE_DIR}/TextsCore2.rc")
