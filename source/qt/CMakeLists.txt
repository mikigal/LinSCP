# linscp: the graphical front end (Qt 6 Widgets, docs/port/PLAN.md section
# 3.4). Everything but main() is in the linscp_qt library, which the GUI
# component tests link too.

find_package(Qt6 6.5 REQUIRED COMPONENTS DBus Widgets)

add_library(linscp_qt STATIC
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
  app/LinScpApplication.cpp
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
  dialogs/SplitButton.cpp
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
file(GLOB LINSCP_FORMS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/source/forms/*.dfm")
set(LINSCP_FORM_TEXTS "${CMAKE_CURRENT_BINARY_DIR}/FormTexts.cpp")
add_custom_command(
  OUTPUT "${LINSCP_FORM_TEXTS}"
  COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/dfm2cpp/dfm2cpp.py" "${LINSCP_FORM_TEXTS}" ${LINSCP_FORMS}
  DEPENDS ${LINSCP_FORMS} "${PROJECT_SOURCE_DIR}/tools/dfm2cpp/dfm2cpp.py"
  COMMENT "Generating the texts of the forms"
  VERBATIM)
target_sources(linscp_qt PRIVATE "${LINSCP_FORM_TEXTS}")

# LinSCP's icon theme and the images of the dialogs (tools/icons/make_icons.py)
# in the resources
set(LINSCP_ICONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/icons/linscp")
file(GLOB LINSCP_ICONS CONFIGURE_DEPENDS "${LINSCP_ICONS_DIR}/scalable/*.svg" "${LINSCP_ICONS_DIR}/dialogs/*.svg")
qt_add_resources(linscp_qt "linscp_icons" PREFIX "/icons/linscp" BASE "${LINSCP_ICONS_DIR}"
  FILES "${LINSCP_ICONS_DIR}/index.theme" "${LINSCP_ICONS_DIR}/linscp.svg" ${LINSCP_ICONS})

set_target_properties(linscp_qt PROPERTIES AUTOMOC ON)
target_include_directories(linscp_qt PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/app"
  "${CMAKE_CURRENT_SOURCE_DIR}/dialogs"
  "${CMAKE_CURRENT_SOURCE_DIR}/models"
  "${CMAKE_CURRENT_SOURCE_DIR}/views")
# Qt's lowercase keywords (signals, slots, emit) are macros of common words;
# Q_SIGNALS, Q_SLOTS and Q_EMIT cannot clash with the headers of the core
# and the libraries it includes
target_compile_definitions(linscp_qt PUBLIC QT_NO_KEYWORDS)
# The data of the source tree, for the builds that are not installed
target_compile_definitions(linscp_qt PRIVATE LINSCP_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
target_link_libraries(linscp_qt
  PRIVATE linscp::options
  PUBLIC linscp::app Qt6::DBus Qt6::Widgets)

# The automation hook of the end-to-end tests (app/Automation.h)
if(LINSCP_AUTOMATION)
  find_package(Qt6 6.5 REQUIRED COMPONENTS Network)
  target_sources(linscp_qt PRIVATE app/Automation.cpp)
  target_compile_definitions(linscp_qt PUBLIC LINSCP_AUTOMATION)
  target_link_libraries(linscp_qt PRIVATE Qt6::Network)
endif()

add_executable(linscp app/Main.cpp)
target_link_libraries(linscp PRIVATE linscp_qt linscp::options)

# The application of the desktop (the URLs of the protocols open with it)
include(GNUInstallDirs)
install(TARGETS linscp RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
install(FILES desktop/linscp.desktop DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES icons/linscp/linscp.svg DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
install(FILES "${PROJECT_SOURCE_DIR}/license.txt" DESTINATION "${CMAKE_INSTALL_DATADIR}/licenses/linscp")
# Upstream's translations (app/Translations.cpp finds them next to the
# binaries)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/translations/" DESTINATION "${CMAKE_INSTALL_DATADIR}/linscp/translations"
  FILES_MATCHING PATTERN "*.ini" PATTERN "version.ini" EXCLUDE)
