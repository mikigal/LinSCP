# String tables generated from the upstream .rc files (tools/rc2cpp), the
# name of the application in them is LinSCP.
#
# linscp_add_string_tables(NAME RC_FILE...) creates the OBJECT library NAME
# from the generated sources. Each source registers its strings with the RTL
# shim from a static initializer, so the objects must be linked directly
# (they are not referenced): consumers add $<TARGET_OBJECTS:NAME> to their
# sources, see source/libcore.

function(linscp_add_string_tables NAME)
  set(GENERATED)
  foreach(RC_FILE IN LISTS ARGN)
    get_filename_component(BASE_NAME "${RC_FILE}" NAME_WE)
    get_filename_component(RC_DIRECTORY "${RC_FILE}" DIRECTORY)
    set(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/strings/${BASE_NAME}.cpp")
    add_custom_command(
      OUTPUT "${OUTPUT}"
      COMMAND "${CMAKE_COMMAND}" -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/strings"
      COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/rc2cpp/rc2cpp.py" --brand "WinSCP=LinSCP"
        "${RC_FILE}" "${OUTPUT}"
      DEPENDS "${RC_FILE}" "${PROJECT_SOURCE_DIR}/tools/rc2cpp/rc2cpp.py"
      COMMENT "Generating string table ${BASE_NAME}.cpp"
      VERBATIM)
    list(APPEND GENERATED "${OUTPUT}")
  endforeach()
  add_library(${NAME} OBJECT ${GENERATED})
  target_include_directories(${NAME} PRIVATE "${RC_DIRECTORY}")
  target_link_libraries(${NAME} PRIVATE linscp::rtl linscp::options)
endfunction()
