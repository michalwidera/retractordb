# cd build/Debug make install ; ctest -R Pattern1 -V

# Weryfikuje kompilację operatora # (HASH-merge strumieni), selekcji pól z
# przesunięciem czasowym oraz łączenia strumieni (+). Trzy zapytania:
# core0#core1 (merge), core1-1/2 (rate-conversion), core1+core0 (concat). Wynik
# kompilatora porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND sh -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
