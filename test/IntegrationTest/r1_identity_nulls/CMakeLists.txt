# Tozsamosc R1 nad dziedzina z NULL-ami: phi(tau_i A, tau_k B) == tau_(i+k)
# phi(A,B).
#
# Pozostale testy reguly uzywaja danych bez NULL-i. Tutaj NULL-e przechodza
# przez przeplot i przesuniecie po obu stronach, a porownaniu podlega takze mapa
# null w .meta — rekord all-null i rekord zerowy maja te sama zawartosc binarna.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")

add_test(
  NAME ${TestID}-run
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh)

set_tests_properties(${TestID}-run PROPERTIES TIMEOUT 60)
