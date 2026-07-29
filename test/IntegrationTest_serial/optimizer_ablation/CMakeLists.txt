# Konfiguracyjne testy infrastruktury ablacyjnej. Oczekiwania są przekazywane
# bezpośrednio z cache CMake i porównywane z raportem skompilowanej binarki.

get_filename_component(TestID ${CMAKE_CURRENT_SOURCE_DIR} NAME)
set(TestID "it_${TestID}")
set(OPTIMIZER_EXPECTED
    ${RDB_OPT_DEDUP_SUBSTRATES} ${RDB_OPT_SHARE_EQUIVALENT_SELECTS}
    ${RDB_OPT_COMMUTATIVE_ADD} ${RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES}
    ${RDB_BENCH_PROBE})

add_test(
  NAME ${TestID}-build-info
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          build-info ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-plan
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor plan
          ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor semantic
          ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-factor-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          factor-semantic ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-factor-shared-substrate-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          factor-shared-substrate-semantic ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-factor-name-collision-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          factor-name-collision-semantic ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-factor-multiquery-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          factor-multiquery-semantic ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-dedup-steady-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          dedup-steady-semantic ${OPTIMIZER_EXPECTED})

add_test(
  NAME ${TestID}-dedup-exact-semantic
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash verify.sh ${CMAKE_BINARY_DIR}/src/retractor/xretractor
          dedup-exact-semantic ${OPTIMIZER_EXPECTED})

set_tests_properties(
  ${TestID}-semantic
  ${TestID}-factor-semantic
  ${TestID}-factor-shared-substrate-semantic
  ${TestID}-factor-name-collision-semantic
  ${TestID}-factor-multiquery-semantic
  ${TestID}-dedup-steady-semantic
  ${TestID}-dedup-exact-semantic
  PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)

# Do 2026-07-26 dwa testy tej macierzy były oznaczone WILL_FAIL jako jawnie
# oczekiwane porażki ablacyjne:
#
# factor_runtime_semantic_divergence  — bez przebiegu faktoryzacji runtime
# wykonywał przesunięcia na osobnych zegarach wejściowych i nie dawał tego
# samego payloadu co (A#B)>(i+k); dedup_startup_prefix_divergence     — przy
# jednocześnie wyłączonych dedup, faktoryzacji i współdzieleniu SELECT badany
# wynik dostawał o jeden rekord zerowego prefiksu więcej niż konfiguracja
# domyślna.
#
# Obie przyczyny zostały usunięte (porządek topologiczny planu, przyczynowe
# opóźnienie przeplotu, jedna konwencja tau, ogon zamiast prefiksu), więc oba
# testy przechodzą w KAŻDEJ konfiguracji przełączników. Adnotacje WILL_FAIL są
# tym samym nieaktualne — a test oznaczony WILL_FAIL, który przechodzi, jest
# raportowany jako porażka, więc ich pozostawienie psułoby macierz.
#
# Zweryfikowane przebiegiem w profilu wszystkich przełączników OFF.
