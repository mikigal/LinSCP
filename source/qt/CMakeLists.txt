# openscp: the graphical front end (Qt 6 Widgets, docs/port/PLAN.md section
# 3.4). Everything but main() is in the openscp_qt library, which the GUI
# component tests link too.

find_package(Qt6 6.5 REQUIRED COMPONENTS DBus Widgets)

add_library(openscp_qt STATIC
  app/ComputerPower.cpp
  app/DesktopShortcut.cpp
  app/EditorManager.cpp
  app/ExportedFiles.cpp
  app/FormTranslator.cpp
  app/KeyTools.cpp
  app/LocalFiles.cpp
  app/LocalProcesses.cpp
  app/Notifications.cpp
  app/OpenInPutty.cpp
  app/OpenScpApplication.cpp
  app/QtInterface.cpp
  app/QtTools.cpp
  app/RtfText.cpp
  app/ShortCuts.cpp
  app/SingleInstance.cpp
  app/TaskbarProgress.cpp
  app/TerminalManager.cpp
  dialogs/AuthenticateForm.cpp
  dialogs/AboutDialog.cpp
  dialogs/CleanupDialog.cpp
  dialogs/ColorMenu.cpp
  dialogs/ConsoleDialog.cpp
  dialogs/CopyDialog.cpp
  dialogs/CopyParamDialogs.cpp
  dialogs/CopyParamsFrame.cpp
  dialogs/CustomCommandDialog.cpp
  dialogs/CustomDialog.cpp
  dialogs/EditMaskDialog.cpp
  dialogs/EditorPreferencesDialog.cpp
  dialogs/EditorForm.cpp
  dialogs/FileFindDialog.cpp
  dialogs/FullSynchronizeDialog.cpp
  dialogs/HistoryComboBox.cpp
  dialogs/FileSystemInfoDialog.cpp
  dialogs/GenerateUrlDialog.cpp
  dialogs/ImportSessionsDialog.cpp
  dialogs/LocationProfilesDialog.cpp
  dialogs/LoginDialog.cpp
  dialogs/MessageDialog.cpp
  dialogs/OpenDirectoryDialog.cpp
  dialogs/PreferencesDialog.cpp
  dialogs/ProgressForm.cpp
  dialogs/PropertiesDialog.cpp
  dialogs/RemoteTransferDialog.cpp
  dialogs/RightsFrame.cpp
  dialogs/SelectMaskDialog.cpp
  dialogs/SiteAdvancedDialog.cpp
  dialogs/SymlinkDialog.cpp
  dialogs/SynchronizeChecklistDialog.cpp
  dialogs/SynchronizeDialog.cpp
  dialogs/SynchronizeProgressForm.cpp
  dialogs/TreePagesDialog.cpp
  models/DirModel.cpp
  views/DirPanel.cpp
  views/DirTree.cpp
  views/PathLabel.cpp
  views/QueueController.cpp
  views/ScpCommander.cpp
  views/ScpCommanderBookmarks.cpp
  views/ScpCommanderClipboard.cpp
  views/ScpCommanderCommandLine.cpp
  views/ScpCommanderCustomCommands.cpp
  views/ScpCommanderDesktop.cpp
  views/ScpCommanderDragDrop.cpp
  views/ScpCommanderEditor.cpp
  views/ScpCommanderFileCommands.cpp
  views/ScpCommanderMark.cpp
  views/ScpCommanderQueue.cpp
  views/ScpCommanderSessions.cpp
  views/ScpCommanderSynchronize.cpp
  views/ScpCommanderToolbars.cpp
  views/Toolbars.cpp)

# The English texts of upstream's forms, which the translations translate
# (tools/dfm2cpp)
file(GLOB OPENSCP_FORMS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/source/forms/*.dfm")
set(OPENSCP_FORM_TEXTS "${CMAKE_CURRENT_BINARY_DIR}/FormTexts.cpp")
add_custom_command(
  OUTPUT "${OPENSCP_FORM_TEXTS}"
  COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/dfm2cpp/dfm2cpp.py" "${OPENSCP_FORM_TEXTS}" ${OPENSCP_FORMS}
  DEPENDS ${OPENSCP_FORMS} "${PROJECT_SOURCE_DIR}/tools/dfm2cpp/dfm2cpp.py"
  COMMENT "Generating the texts of the forms"
  VERBATIM)
target_sources(openscp_qt PRIVATE "${OPENSCP_FORM_TEXTS}")

set_target_properties(openscp_qt PROPERTIES AUTOMOC ON)
target_include_directories(openscp_qt PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/app"
  "${CMAKE_CURRENT_SOURCE_DIR}/dialogs"
  "${CMAKE_CURRENT_SOURCE_DIR}/models"
  "${CMAKE_CURRENT_SOURCE_DIR}/views")
# Qt's lowercase keywords (signals, slots, emit) are macros of common words;
# Q_SIGNALS, Q_SLOTS and Q_EMIT cannot clash with the headers of the core
# and the libraries it includes
target_compile_definitions(openscp_qt PUBLIC QT_NO_KEYWORDS)
# The data of the source tree, for the builds that are not installed
target_compile_definitions(openscp_qt PRIVATE OPENSCP_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(openscp_qt
  PRIVATE openscp::options
  PUBLIC openscp::app Qt6::DBus Qt6::Widgets)

# The automation hook of the end-to-end tests (app/Automation.h)
if(OPENSCP_AUTOMATION)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Network)
  target_sources(openscp_qt PRIVATE app/Automation.cpp)
  target_compile_definitions(openscp_qt PUBLIC OPENSCP_AUTOMATION)
  target_link_libraries(openscp_qt PRIVATE Qt6::Network)
endif()

add_executable(openscp app/Main.cpp)
target_link_libraries(openscp PRIVATE openscp_qt openscp::options)

# The application of the desktop (the URLs of the protocols open with it)
include(GNUInstallDirs)
install(TARGETS openscp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES desktop/openscp.desktop DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES "${PROJECT_SOURCE_DIR}/license.txt" DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/openscp")
# Upstream's translations (app/Translations.cpp finds them next to the
# binaries)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/translations/" DESTINATION "${CMAKE_INSTALL_DATADIR}/openscp/translations"
  FILES_MATCHING PATTERN "*.ini" PATTERN "version.ini" EXCLUDE)
