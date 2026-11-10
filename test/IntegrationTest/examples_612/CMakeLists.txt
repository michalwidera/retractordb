# Testuje dostarczane przyklady, nie osobne kopie planow utrzymywane recznie.
# configure_file sledzi zmiany RQL, skryptow i danych takze przy zwyklym ninja.
foreach(example IN ITEMS issue-set/multi-read mwd session-record-1)
  file(
    GLOB
    inputs
    "${PROJECT_SOURCE_DIR}/examples/${example}/*.rql"
    "${PROJECT_SOURCE_DIR}/examples/${example}/*.sh"
    "${PROJECT_SOURCE_DIR}/examples/${example}/*.txt"
    "${PROJECT_SOURCE_DIR}/examples/${example}/*.dat")
  foreach(input ${inputs})
    get_filename_component(filename "${input}" NAME)
    configure_file("${input}" "examples/${example}/${filename}" COPYONLY)
  endforeach()
endforeach()

add_test(
  NAME it_examples_612-value
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 verify.py $<TARGET_FILE:xretractor> $<TARGET_FILE:xtrdb>)
set_tests_properties(it_examples_612-value PROPERTIES TIMEOUT 60)
