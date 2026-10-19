# cd build/Debug && ninja install && ctest -R it_var_adhoc -V

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# Zmienne VAR w kanale ad hoc i cykl zycia planu (#531, etap C): odczyt i zapis
# zmiennych planu z ad hoc, konflikt zapisu, odmowy bez zmiany planu i wartosci,
# --reset bez przecieku nazw i wartosci. Scenariusz w run.sh.

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 120)
