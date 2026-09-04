# Weryfikuje że kompilator wykrywa i raportuje cykliczne zależności w grafie
# zapytań (issue #95). Oczekiwany wynik: niezerowy kod wyjścia i komunikat
# "Circular dependency" na stderr w ciągu 10 sekund.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "pt_${TestID}")

add_test(
  NAME ${TestID}-compile
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/check_loop.sh)
