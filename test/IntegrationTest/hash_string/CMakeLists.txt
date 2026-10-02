# make install && ctest -R hash_string -V

# cmake-format: off
# Przeplot `#` nad polami STRING o roznej dlugosci deklarowanej (STRING[8] i STRING[16]) oraz
# nad para INTEGER/DOUBLE na tej samej pozycji. Slot wejscia i pole wyjscia maja na kazdej
# pozycji szerszy z dwoch elementow, a rekord skladnika wchodzi do slotu wejscia slot po slocie.
#   -run     obie kolejnosci skladnikow wobec wzorca (szczegoly w run.sh)
#   -vg-run  to samo pod valgrindem
#   -adhoc   ten sam przeplot zgloszony przez `xqry -a` do dzialajacego serwera
# Do 2026-09-27 plan przechodzil `-c`, a przebieg konczyl sie w pierwszym slocie FatalError-em
# `payload: descriptor not empty before assign - schema mismatch`; para INTEGER/DOUBLE liczyla
# po cichu bajty inta nalozone na bajty poprzedniego double'a.
# cmake-format: on

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR}/src/rdb/xtrdb)

add_test(
  NAME ${TestID}-vg-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash run.sh ${CMAKE_BINARY_DIR}/src/rdb/xtrdb ${IT_VALGRIND})

add_test(
  NAME ${TestID}-adhoc
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash adhoc.sh)

set_tests_properties(${TestID}-run ${TestID}-vg-run ${TestID}-adhoc
                     PROPERTIES TIMEOUT 120)
