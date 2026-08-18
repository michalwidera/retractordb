# cd build/Debug make install ; ctest -R operations-run -V
#
# Weryfikuje operator # (merge dwóch strumieni: core0 i core1).
#
# UWAGA: test używa awk zamiast compare_files z powodu race condition na
# poziomie połączenia klienta (okno między "get"+"show" a otwarciem kolejki).
#
# xretractor startuje pierwszy (lock file jako sygnał gotowości), potem xqry
# synchronicznie — unika race condition xqry-przed-xretractorem.
#
# Weryfikacja awk: - core1 (≥100) rosnące, core0 (<100) rosnące - między
# kolejnymi core1 dokładnie 2 core0 (stosunek rate 1/8 : 1/16) - sprawdzenie
# odporne na utratę pierwszych pakietów z powodu race connection.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

# Startuje serwer (blokada uslugi) -> RUN_SERIAL jawnie: flagi -k/-x sa w
# run.sh, wiec makro add_test ich nie widzi.
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)
# set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 900)
