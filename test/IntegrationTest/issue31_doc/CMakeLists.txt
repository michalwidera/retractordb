# cd build/Debug make install ; ctest -R issue31_doc -V cmake . && ctest -R
# issue31_doc -V

# Weryfikuje że wszystkie przykłady dokumentacyjne (q-31.*.rql) kompilują się
# bez błędów i generują poprawny DOT na trzech poziomach szczegółowości: -d -p,
# -d -f -t -s -p, -d -f -t -s -u -p. Generuje pliki SVG przez graphviz.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Graph parameters
set(XRETRACTOR_GRAPH_PARAM_0 "-d -p")
set(XRETRACTOR_GRAPH_PARAM_1 "-d -f -t -s -p")
set(XRETRACTOR_GRAPH_PARAM_2 "-d -f -t -s -u -p")

# Jedyne miejsce w calym drzewie testow, ktore wola graphviza: pozostale testy
# `-dot` porownuja sam TEKST DOT z xretractora i `dot` im niepotrzebny. Test bez
# narzedzia nie ma sie degradowac do slabszego sprawdzenia — ma sie NIE
# ZAREJESTROWAC, zeby zielony przebieg nie znaczyl czegos innego na maszynie bez
# graphviza. Obraz projektu (docker/ci/Dockerfile) ma graphviza, wiec na CI z
# tego obrazu testy istnieja jak dotad.
find_program(RDB_DOT_EXECUTABLE dot)
if(NOT RDB_DOT_EXECUTABLE)
  message(
    STATUS
      "graphviz (dot) nieobecny — testy ${TestID}-*-graph NIE sa rejestrowane")
  return()
endif()

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
      "set -e && \
      xretractor ${item} -c > ${item}.out.txt && \
      xretractor ${item} -c ${XRETRACTOR_GRAPH_PARAM_0} > ${item}.out.0.dot && dot -Tsvg ${item}.out.0.dot -o ${item}.out.0.svg && \
      xretractor ${item} -c ${XRETRACTOR_GRAPH_PARAM_1} > ${item}.out.1.dot && dot -Tsvg ${item}.out.1.dot -o ${item}.out.1.svg && \
      xretractor ${item} -c ${XRETRACTOR_GRAPH_PARAM_2} > ${item}.out.2.dot && dot -Tsvg ${item}.out.2.dot -o ${item}.out.2.svg"
  )
endforeach()

# set_tests_properties(${TestID}-graph1 PROPERTIES DEPENDS ${TestID}-graph2) feh
# out.png
