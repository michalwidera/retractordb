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
  ${TestID}-semantic ${TestID}-factor-semantic ${TestID}-dedup-steady-semantic
  ${TestID}-dedup-exact-semantic PROPERTIES RUN_SERIAL TRUE TIMEOUT 60)

# Bez przebiegu faktoryzacji obecny runtime wykonuje przesunięcia na osobnych
# zegarach wejściowych i nie daje tego samego payloadu co (A#B)>(i+k). Test
# zachowujemy jako jawnie oczekiwaną porażkę ablacyjną, zamiast ją ukrywać.
if(NOT RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES)
  set_tests_properties(
    ${TestID}-factor-semantic
    PROPERTIES WILL_FAIL TRUE LABELS
               "expected_ablation_failure;factor_runtime_semantic_divergence")
endif()

# Gdy wyłączone są jednocześnie deduplikacja i faktoryzacja, dwa równoważne
# węzły DA+DB są harmonogramowane osobno. Wartości stanu ustalonego są zgodne,
# ale badany wynik otrzymuje o jeden rekord zerowego prefiksu więcej niż
# konfiguracja domyślna.
if(NOT RDB_OPT_DEDUP_SUBSTRATES AND NOT RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES)
  set_tests_properties(
    ${TestID}-dedup-exact-semantic
    PROPERTIES WILL_FAIL TRUE LABELS
               "expected_ablation_failure;dedup_startup_prefix_divergence")
endif()
