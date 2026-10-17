# Jednostka i pomocnik ExecStopPost sa instalowane tylko na Linuksie.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  return()
endif()

# covers: launcher.xretractor.31~1
add_test(
  NAME it_service_abort-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor>
          ${CMAKE_BINARY_DIR}/xretractor-stop-post)
set_tests_properties(it_service_abort-run PROPERTIES TIMEOUT 90)
