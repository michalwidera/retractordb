get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "timeout 10 xretractor brokenQuery.rql -c 2>out.txt ; \
    test $? -ne 0 && \
    grep -q 'Circular dependency' out.txt")
