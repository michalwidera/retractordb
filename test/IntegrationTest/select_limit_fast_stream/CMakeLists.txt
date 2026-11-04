# make install && ctest -R select_limit_fast_stream -V

# cmake-format: off
# `xqry -s -m N` na strumieniu szybszym niz ~1 kHz: klient ma sie zakonczyc zaraz po N elementach.
# Do 2026-11-04 (#599) wypisywal N wierszy i wisial w petli drenazu za petla renderowania
# (szczegoly w run.sh).
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

# covers: qry.qry.5~1, qry.qry.8~1, qry.qry.16~1
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
