add_executable(linscp_toolchain_tests
  ToolchainTest.cpp)
target_link_libraries(linscp_toolchain_tests PRIVATE
  linscp::options
  GTest::gtest_main)
gtest_discover_tests(linscp_toolchain_tests
  PROPERTIES LABELS unit)

add_executable(linscp_rtl_tests
  rtl/ClassesTest.cpp
  rtl/DataFormatsTest.cpp
  rtl/DateTimeTest.cpp
  rtl/FilesTest.cpp
  rtl/FormatTest.cpp
  rtl/StringsTest.cpp
  rtl/SystemTest.cpp
  rtl/SysUtilsTest.cpp
  rtl/UtilsTest.cpp)
target_link_libraries(linscp_rtl_tests PRIVATE
  linscp::rtl
  linscp::options
  GTest::gtest_main)
# Locale and time zone independent results; tests needing others set them
gtest_discover_tests(linscp_rtl_tests
  PROPERTIES LABELS unit ENVIRONMENT "LC_ALL=C;TZ=UTC")

add_executable(linscp_linux_tests
  linux/CrtTest.cpp
  linux/FileApiTest.cpp
  linux/HandlesTest.cpp
  linux/MessagesTest.cpp
  linux/ShellTest.cpp
  linux/SystemTest.cpp
  linux/TimeApiTest.cpp)
target_compile_definitions(linscp_linux_tests PRIVATE LINSCP_FILE_VERSION="${LINSCP_FILE_VERSION}")
target_link_libraries(linscp_linux_tests PRIVATE
  linscp::linux
  linscp::options
  OpenSSL::Crypto
  GTest::gtest_main)
gtest_discover_tests(linscp_linux_tests
  PROPERTIES LABELS unit ENVIRONMENT "LC_ALL=C;TZ=UTC")

add_executable(linscp_core_tests
  core/CommonTest.cpp
  core/ConfigurationTest.cpp
  core/CopyParamTest.cpp
  core/CoreTestInterface.cpp
  core/CoreTestMain.cpp
  core/FileMasksTest.cpp
  core/RemoteFilesTest.cpp
  core/ResourceStringsTest.cpp
  core/SessionDataTest.cpp)
target_compile_definitions(linscp_core_tests PRIVATE
  LINSCP_VERSION="${LinSCP_VERSION}"
  LINSCP_CORE_VERSION="${LINSCP_CORE_VERSION}")
target_link_libraries(linscp_core_tests PRIVATE
  linscp::core
  linscp::options
  GTest::gtest)
gtest_discover_tests(linscp_core_tests
  PROPERTIES LABELS unit ENVIRONMENT "LC_ALL=C;TZ=UTC")

# .rc string table generator
add_test(NAME unit.rc2cpp
  COMMAND Python3::Interpreter -m unittest -v test_rc2cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/rc2cpp")
set_tests_properties(unit.rc2cpp PROPERTIES
  LABELS unit
  ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/rc2cpp;PYTHONDONTWRITEBYTECODE=1")

# The generator of the texts of the forms
add_test(NAME unit.dfm2cpp
  COMMAND Python3::Interpreter -m unittest -v test_dfm2cpp
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/dfm2cpp")
set_tests_properties(unit.dfm2cpp PROPERTIES
  LABELS unit
  ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/dfm2cpp;PYTHONDONTWRITEBYTECODE=1")

# bcb2clang, and the converted core headers parsing against the RTL shim
foreach(BCB2CLANG_TEST test_bcb2clang test_convert test_core_headers)
  add_test(NAME unit.bcb2clang.${BCB2CLANG_TEST}
    COMMAND Python3::Interpreter -m unittest -v ${BCB2CLANG_TEST}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bcb2clang")
  set_tests_properties(unit.bcb2clang.${BCB2CLANG_TEST} PROPERTIES
    LABELS unit
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/bcb2clang;PYTHONDONTWRITEBYTECODE=1;LINSCP_CXX=${CMAKE_CXX_COMPILER}"
    TIMEOUT 300)
endforeach()

# L3: components of the Qt front end on the offscreen platform
add_executable(linscp_qt_tests
  qt/AboutDialogTest.cpp
  qt/BookmarksTest.cpp
  qt/CleanupDialogTest.cpp
  qt/CopyDialogTest.cpp
  qt/CustomCommandsTest.cpp
  qt/DirPanelTest.cpp
  qt/EditorTest.cpp
  qt/FileSystemInfoDialogTest.cpp
  qt/GenerateUrlDialogTest.cpp
  qt/HelpButtonTest.cpp
  qt/IconThemeTest.cpp
  qt/ImportSessionsTest.cpp
  qt/MessageDialogTest.cpp
  qt/PathLabelTest.cpp
  qt/PreferencesDialogTest.cpp
  qt/PrivateFilesTest.cpp
  qt/ProgressFormTest.cpp
  qt/PropertiesDialogTest.cpp
  qt/QtTestMain.cpp
  qt/RightsFrameTest.cpp
  qt/SelectMaskDialogTest.cpp
  qt/SiteAdvancedDialogTest.cpp
  qt/SymlinkDialogTest.cpp
  qt/SynchronizeDialogTest.cpp
  qt/ToolbarsTest.cpp
  qt/TranslationsTest.cpp)
find_package(Qt6 6.5 REQUIRED COMPONENTS Test)
target_link_libraries(linscp_qt_tests PRIVATE
  linscp_qt
  Qt6::Test
  linscp::options
  GTest::gtest)
gtest_discover_tests(linscp_qt_tests
  PROPERTIES LABELS unit ENVIRONMENT "LC_ALL=C.UTF-8;TZ=UTC;QT_QPA_PLATFORM=offscreen")

# The files of the desktop integration follow the freedesktop specifications
find_program(DESKTOP_FILE_VALIDATE desktop-file-validate)
if(DESKTOP_FILE_VALIDATE)
  add_test(NAME unit.desktop_files
    COMMAND "${DESKTOP_FILE_VALIDATE}" "${PROJECT_SOURCE_DIR}/source/qt/desktop/linscp.desktop")
  set_tests_properties(unit.desktop_files PROPERTIES LABELS unit)
endif()
