set(LINSCP_TEST_PYTHONPATH
  "${PROJECT_SOURCE_DIR}/tools/testenv:${PROJECT_SOURCE_DIR}/tools/wlharness")

add_test(NAME integration.local_sshd
  COMMAND Python3::Interpreter -m unittest -v test_local_sshd
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(integration.local_sshd PROPERTIES
  LABELS "integration;ssh"
  ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1"
  TIMEOUT 60)

# L2: SFTP and SCP with linscp-cli against the local sshd
add_test(NAME integration.cli_ssh
  COMMAND Python3::Interpreter -m unittest -v test_cli_ssh
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(integration.cli_ssh PROPERTIES
  LABELS "integration;ssh"
  ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;LINSCP_CLI=$<TARGET_FILE:linscp-cli>"
  TIMEOUT 600)

# L2 against the test VM (tools/testenv/testvm.py, tests in vmtests.py):
# skipped unless LINSCP_TEST_VM is set, e.g.
# LINSCP_TEST_VM=10.10.100.170 ctest --preset dev -L vm
# SFTP and SCP (test_cli_vm), FTP and FTPS, WebDAV, S3
foreach(VM_TEST cli_vm cli_vm_ftp cli_vm_webdav cli_vm_s3)
  add_test(NAME integration.${VM_TEST}
    COMMAND Python3::Interpreter -m unittest -v test_${VM_TEST}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(integration.${VM_TEST} PROPERTIES
    LABELS "integration;vm"
    ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;LINSCP_CLI=$<TARGET_FILE:linscp-cli>"
    # The VM is small, one at a time
    RESOURCE_LOCK test_vm
    TIMEOUT 1200)
endforeach()

# Startup smoke tests of the GUI, offscreen (test_smoke): of this build, and
# of the release build of LINSCP_RELEASE_DIR, skipped when it is not built;
# build the release preset before
set(LINSCP_RELEASE_DIR "${PROJECT_SOURCE_DIR}/build/release" CACHE PATH
  "The release build whose GUI the smoke test starts")
foreach(SMOKE_TEST smoke smoke_release)
  if(SMOKE_TEST STREQUAL "smoke")
    set(SMOKE_GUI "$<TARGET_FILE:linscp>")
  else()
    set(SMOKE_GUI "${LINSCP_RELEASE_DIR}/source/qt/linscp")
  endif()
  add_test(NAME integration.${SMOKE_TEST}
    COMMAND Python3::Interpreter -m unittest -v test_smoke
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(integration.${SMOKE_TEST} PROPERTIES
    LABELS "integration;ssh;smoke"
    ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;LINSCP_SMOKE_GUI=${SMOKE_GUI};LINSCP_CLI=$<TARGET_FILE:linscp-cli>"
    SKIP_REGULAR_EXPRESSION "skipped 'no GUI binary"
    TIMEOUT 180)
endforeach()

if(LINSCP_BUILD_HARNESS)
  add_test(NAME integration.wlharness
    COMMAND Python3::Interpreter -m unittest -v test_wlharness
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(integration.wlharness PROPERTIES
    LABELS "integration;gui"
    ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;WLH_PROBE=$<TARGET_FILE:wlh-probe>"
    RUN_SERIAL TRUE
    TIMEOUT 180)

  # L4: the GUI in the headless KWin, driven through its automation hook
  if(LINSCP_AUTOMATION)
    add_test(NAME integration.gui
      COMMAND Python3::Interpreter -m unittest -v test_gui
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(integration.gui PROPERTIES
      LABELS "integration;gui"
      ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;LINSCP_GUI=$<TARGET_FILE:linscp>"
      RUN_SERIAL TRUE
      TIMEOUT 300)

    # The GUI with SFTP against the local sshd
    add_test(NAME integration.gui_ssh
      COMMAND Python3::Interpreter -m unittest -v test_gui_ssh
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(integration.gui_ssh PROPERTIES
      LABELS "integration;gui;ssh"
      ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;LINSCP_GUI=$<TARGET_FILE:linscp>;LINSCP_CLI=$<TARGET_FILE:linscp-cli>"
      RUN_SERIAL TRUE
      TIMEOUT 600)

    # The desktop integration: drag and drop, the clipboard, with Dolphin
    add_test(NAME integration.gui_desktop
      COMMAND Python3::Interpreter -m unittest -v test_gui_desktop
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(integration.gui_desktop PROPERTIES
      LABELS "integration;gui;ssh"
      ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;WLH_PROBE=$<TARGET_FILE:wlh-probe>;LINSCP_GUI=$<TARGET_FILE:linscp>;LINSCP_CLI=$<TARGET_FILE:linscp-cli>"
      RUN_SERIAL TRUE
      TIMEOUT 600)

    # The GUI against the test VM (skipped without LINSCP_TEST_VM)
    add_test(NAME integration.gui_vm
      COMMAND Python3::Interpreter -m unittest -v test_gui_vm
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(integration.gui_vm PROPERTIES
      LABELS "integration;gui;vm"
      ENVIRONMENT "PYTHONPATH=${LINSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;LINSCP_GUI=$<TARGET_FILE:linscp>;LINSCP_CLI=$<TARGET_FILE:linscp-cli>"
      RUN_SERIAL TRUE
      RESOURCE_LOCK test_vm
      TIMEOUT 1200)
  endif()
endif()
