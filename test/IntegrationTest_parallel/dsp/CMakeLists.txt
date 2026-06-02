# Regresja dla przykładu DSP: potok filtra FIR złożony z okna przesuwnego
# @(1,25), mnożenia element-po-element przez indeks _ (accRow), redukcji .sumc
# (suma skumulowana) oraz złączenia sygnału i wyjścia (outputAll).
# Wynik kompilatora porównywany z pattern.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")
add_test(
  NAME ${TestID}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
    xretractor query.rql -c > out.txt ; \
    ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern.txt out.txt")
