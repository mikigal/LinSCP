# RTL compatibility layer: the subset of the Delphi RTL/VCL used by the core,
# in standard C++ (see docs/port/PLAN.md, section 3.2).
#
# include/ holds headers named like C++Builder's (System.hpp, Classes.hpp, ...);
# they include the implementation headers from include/rtl/.

add_library(openscp_rtl STATIC
  src/Classes.cpp
  src/Closure.cpp
  src/Contnrs.cpp
  src/Conversion.cpp
  src/DateTime.cpp
  src/DateUtils.cpp
  src/EncdDecd.cpp
  src/Encoding.cpp
  src/ExceptionAllocation.cpp
  src/Files.cpp
  src/Format.cpp
  src/IniFiles.cpp
  src/IOUtils.cpp
  src/Json.cpp
  src/Masks.cpp
  src/Menus.cpp
  src/FormatSettings.cpp
  src/Printf.cpp
  src/Registry.cpp
  src/Streams.cpp
  src/StrUtils.cpp
  src/Strings.cpp
  src/SysUtils.cpp
  src/System.cpp
  src/Variants.cpp
  src/WinError.cpp
  src/Xml.cpp)
add_library(openscp::rtl ALIAS openscp_rtl)

target_include_directories(openscp_rtl PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
# __declspec(property) is used by the RTL headers themselves
target_compile_options(openscp_rtl PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-fdeclspec>)
target_link_libraries(openscp_rtl
  PRIVATE openscp::options
  PUBLIC EXPAT::EXPAT)
# Thrown objects are zero-filled, as Delphi objects (src/ExceptionAllocation.cpp)
target_link_options(openscp_rtl INTERFACE "LINKER:--wrap=__cxa_allocate_exception")
