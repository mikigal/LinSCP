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

# L2 against the test VM (tools/testenv/testvm.py): skipped unless
# OPENSCP_TEST_VM is set, e.g. OPENSCP_TEST_VM=10.10.100.170 ctest --preset dev -L vm
add_test(NAME integration.cli_vm
  COMMAND Python3::Interpreter -m unittest -v test_cli_vm
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
set_tests_properties(integration.cli_vm PROPERTIES
  LABELS "integration;vm"
  ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;OPENSCP_CLI=$<TARGET_FILE:openscp-cli>"
  TIMEOUT 900)

if(OPENSCP_BUILD_HARNESS)
  add_test(NAME integration.wlharness
    COMMAND Python3::Interpreter -m unittest -v test_wlharness
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  set_tests_properties(integration.wlharness PROPERTIES
    LABELS "integration;gui"
    ENVIRONMENT "PYTHONPATH=${OPENSCP_TEST_PYTHONPATH};PYTHONDONTWRITEBYTECODE=1;WLH_INPUT=$<TARGET_FILE:wlh-input>;WLH_PROBE=$<TARGET_FILE:wlh-probe>"
    RUN_SERIAL TRUE
    TIMEOUT 180)
endif()
