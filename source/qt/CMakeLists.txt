# openscp: the graphical front end (Qt 6 Widgets, docs/port/PLAN.md section
# 3.4). Everything but main() is in the openscp_qt library, which the GUI
# component tests link too.

find_package(Qt6 6.5 REQUIRED COMPONENTS DBus Widgets)

add_library(openscp_qt STATIC
  app/EditorManager.cpp
  app/ExportedFiles.cpp
  app/LocalFiles.cpp
  app/LocalProcesses.cpp
  app/Notifications.cpp
  app/OpenScpApplication.cpp
  app/QtInterface.cpp
  app/QtTools.cpp
  app/ShortCuts.cpp
  app/SingleInstance.cpp
  app/TaskbarProgress.cpp
  app/TerminalManager.cpp
  dialogs/AuthenticateForm.cpp
  dialogs/ConsoleDialog.cpp
  dialogs/CopyDialog.cpp
  dialogs/CopyParamDialogs.cpp
  dialogs/CopyParamsFrame.cpp
  dialogs/CustomCommandDialog.cpp
  dialogs/CustomDialog.cpp
  dialogs/EditMaskDialog.cpp
  dialogs/EditorForm.cpp
  dialogs/FileFindDialog.cpp
  dialogs/FullSynchronizeDialog.cpp
  dialogs/HistoryComboBox.cpp
  dialogs/LocationProfilesDialog.cpp
  dialogs/LoginDialog.cpp
  dialogs/MessageDialog.cpp
  dialogs/OpenDirectoryDialog.cpp
  dialogs/ProgressForm.cpp
  dialogs/PropertiesDialog.cpp
  dialogs/RemoteTransferDialog.cpp
  dialogs/RightsFrame.cpp
  dialogs/SynchronizeChecklistDialog.cpp
  dialogs/SynchronizeDialog.cpp
  dialogs/SynchronizeProgressForm.cpp
  models/DirModel.cpp
  views/DirPanel.cpp
  views/QueueController.cpp
  views/ScpCommander.cpp
  views/ScpCommanderBookmarks.cpp
  views/ScpCommanderClipboard.cpp
  views/ScpCommanderCommandLine.cpp
  views/ScpCommanderCustomCommands.cpp
  views/ScpCommanderDesktop.cpp
  views/ScpCommanderDragDrop.cpp
  views/ScpCommanderEditor.cpp
  views/ScpCommanderQueue.cpp
  views/ScpCommanderSynchronize.cpp)

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
