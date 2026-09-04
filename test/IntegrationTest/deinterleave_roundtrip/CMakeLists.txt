# cd build/Debug ; ninja install ; ctest -R deinterleave_roundtrip -V
#
# Weryfikuje tozsamosc okrezna przeplotu/rozplotu (#, &, %) co do bitu: wzorzec
# przeplotu wg definicji formalnej (z=2/3: B,A,A), rozplot Theta z jednoslotowym
# opoznieniem (rekord 0 all-null), rozplot ~Theta dokladny od rekordu 0. Wzorce
# oczekiwane liczone w verify.sh z definicji.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)
set_tests_properties(${TestID}-run PROPERTIES RUN_SERIAL TRUE)
