# Weryfikuje operacje xtrdb descc (wyświetlenie deskryptora), rread (odczyt
# wsteczny) i printt (wydruk sformatowany) na strumieniu TEXTSOURCE: plik
# tekstowy jako źródło danych. Wynik porównywany z pattern-run.txt.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash -c "set -e ; \
  ${CMAKE_BINARY_DIR}/src/rdb/xtrdb noprompt < term.script > out.txt ;  \
  ${CMAKE_COMMAND} -E compare_files --ignore-eol pattern-run.txt out.txt")
