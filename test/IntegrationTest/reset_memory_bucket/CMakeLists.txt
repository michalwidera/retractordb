# make install && ctest -R reset_memory_bucket -V

# cmake-format: off
# `xqry --reset` do planu, w ktorym strumien VOLATILE `x` o tej samej nazwie ma wezszy rekord (16 pol
# INTEGER -> 1), a okno nad `x` czyta jego historie zaraz po wymianie.
#   -run     scenariusz wobec wzorca ze swiezego procesu (szczegoly w run.sh)
#   -vg-run  to samo pod valgrindem
# Do 2026-09-27 nowy `x` dziedziczyl kubelek memoryStore poprzedniego planu: valgrind zglaszal
# Invalid write w memoryFile::read, a serwer konczyl sie FatalError-em zaraz po wymianie.
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

set_tests_properties(${TestID}-run ${TestID}-vg-run PROPERTIES TIMEOUT 180)
