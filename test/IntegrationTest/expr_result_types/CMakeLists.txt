# cd build/Debug && ninja install && ctest -R it_expr_result_types -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Kontrakt typu wyniku wyrazenia (pozycja 16 w
# paper-arXiv/usecases/requested.md).
#
# Test sprawdza JEDNOCZESNIE cztery rzeczy, bo kazda z osobna przepuszcza inny
# blad: nazwy typow w `.desc`, rozmiar rekordu i rozklad bajtow na pola,
# wartosci payloadu (w tym NIECALKOWITE, ktore ujawniaja stratne rzutowanie)
# oraz NULL.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
