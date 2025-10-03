# cd build/Debug make install ; ctest -R issue31_doc -V
# cmake . && ctest -R issue31_doc -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_test(
  NAME ${TestID}-graph1
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query-31.a.rql -c > out.a.txt ; \
    xretractor query-31.a.rql -c -d -f -t -s > out.a.dot && dot -Tpng out.a.dot -o out.a.png ; \
    ")
# ${CMAKE_COMMAND} -E compare_files --ignore-eol dot-pattern.txt out.dot")

add_test(
  NAME ${TestID}-graph2
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query-31.b.rql -c > out.b.txt ; \
    xretractor query-31.b.rql -c -d -f -t -s > out.b.dot && dot -Tpng out.b.dot -o out.b.png ; \
    ")

add_test(
  NAME ${TestID}-graph3
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query-31.c.rql -c > out.c.txt ; \
    xretractor query-31.c.rql -c -d -f -t -s > out.c.dot && dot -Tpng out.c.dot -o out.c.png ; \
    ")

set_tests_properties(${TestID}-graph1 PROPERTIES DEPENDS ${TestID}-graph2)
# feh out.png 
