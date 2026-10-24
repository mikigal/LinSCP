find_package(GTest 1.14 REQUIRED)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
include(GoogleTest)

add_subdirectory(unit)
add_subdirectory(integration)
