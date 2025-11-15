# cd build/Debug make install ; ctest -R issue31_doc -V cmake . && ctest -R
# issue31_doc -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Graph parameters
set(XRETRACTOR_GRAPH_PARAM_0 "-d -p")
set(XRETRACTOR_GRAPH_PARAM_1 "-d -f -t -s -p")
set(XRETRACTOR_GRAPH_PARAM_2 "-d -f -t -s -u -p")

file(
  GLOB V_GLOB_RQL
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  "*.rql")
foreach(item ${V_GLOB_RQL})
  add_test(
    NAME ${TestID}-${item}-graph
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
      bash -c
      "set -e ; \
      xretractor ${item} -c > ${item}.out.txt ; \
      xretractor ${item} -c ${XRETRACTOR_GRAPH_PARAM_0} > ${item}.out.0.dot && dot -Tsvg ${item}.out.0.dot -o ${item}.out.0.svg ; \
      xretractor ${item} -c ${XRETRACTOR_GRAPH_PARAM_1} > ${item}.out.1.dot && dot -Tsvg ${item}.out.1.dot -o ${item}.out.1.svg ; \
      xretractor ${item} -c ${XRETRACTOR_GRAPH_PARAM_2} > ${item}.out.2.dot && dot -Tsvg ${item}.out.2.dot -o ${item}.out.2.svg ; \
      ")
endforeach()

# set_tests_properties(${TestID}-graph1 PROPERTIES DEPENDS ${TestID}-graph2) feh
# out.png
