set(OPENSCP_TEST_PYTHONPATH
  "${PROJECT_SOURCE_DIR}/tools/testenv:${PROJECT_SOURCE_DIR}/tools/wlharness")

add_test(NAME integration.local_sshd
  COMMAND Python3::Interpreter -m unittest -v test_local_sshd
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(integration.local_sshd PROPERTIES
  LABELS "integration;ssh"
  ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1"
  TIMEOUT 60)

# L2: SFTP and SCP with openscp-cli against the local sshd
add_test(NAME integration.cli_ssh
  COMMAND Python3::Interpreter -m unittest -v test_cli_ssh
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(integration.cli_ssh PROPERTIES
  LABELS "integration;ssh"
  ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;OPENSCP_CLI=$<TARGET_FILE:openscp-cli>"
  TIMEOUT 600)

# L2 against the test VM (tools/testenv/testvm.py, tests in vmtests.py):
# skipped unless OPENSCP_TEST_VM is set, e.g.
# OPENSCP_TEST_VM=10.10.100.170 ctest --preset dev -L vm
# SFTP and SCP (test_cli_vm), FTP and FTPS, WebDAV, S3
foreach(VM_TEST cli_vm cli_vm_ftp cli_vm_webdav cli_vm_s3)
  add_test(NAME integration.${VM_TEST}
    COMMAND Python3::Interpreter -m unittest -v test_${VM_TEST}
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(integration.${VM_TEST} PROPERTIES
    LABELS "integration;vm"
    ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;OPENSCP_CLI=$<TARGET_FILE:openscp-cli>"
    # The VM is small, one at a time
    RESOURCE_LOCK test_vm
    TIMEOUT 1200)
endforeach()

if(OPENSCP_BUILD_HARNESS)
  add_test(NAME integration.wlharness
    COMMAND Python3::Interpreter -m unittest -v test_wlharness
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(integration.wlharness PROPERTIES
    LABELS "integration;gui"
    ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;WLH_PROBE=$<TARGET_FILE:wlh-probe>"
    RUN_SERIAL TRUE
    TIMEOUT 180)

  # L4: the GUI in the headless KWin, driven through its automation hook
  if(OPENSCP_AUTOMATION)
    add_test(NAME integration.gui
      COMMAND Python3::Interpreter -m unittest -v test_gui
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(integration.gui PROPERTIES
      LABELS "integration;gui"
      ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;OPENSCP_GUI=$<TARGET_FILE:openscp>"
      RUN_SERIAL TRUE
      TIMEOUT 300)
  endif()
endif()
