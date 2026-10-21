cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

enable_testing()

# message(STATUS ${CMAKE_CURRENT_BINARY_DIR}) message(STATUS
# ${CMAKE_BINARY_DIR}) message(STATUS ${CMAKE_PREFIX_PATH}) message(STATUS
# ${CMAKE_CURRENT_SOURCE_DIR})

message(STATUS "Remove folder " ${CMAKE_BINARY_DIR}/test)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/test)

message(STATUS "Copy folder " ${CMAKE_CURRENT_SOURCE_DIR} " to "
               ${CMAKE_CURRENT_BINARY_DIR}/..)
file(
  COPY ${CMAKE_CURRENT_SOURCE_DIR}
  DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/..
  PATTERN "CMakeLists.txt" EXCLUDE)

file(
  GLOB V_GLOB
  LIST_DIRECTORIES true
  "*")
foreach(item ${V_GLOB})
  if(IS_DIRECTORY ${item})
    add_subdirectory(${item})
  endif()
endforeach()

# Straznik oprawy. Pilnuje, ze kazdy zarejestrowany test wykonuje CALY swoj
# skrypt: po `-c` ma stac dokladnie jeden argument. Makro add_test z
# IntegrationTest przekazuje argumenty przez `_add_test(${ARGV})`, co tnie je po
# wewnetrznych srednikach - tak cztery testy naraz stalo sie zawsze-zielonymi
# no-opami. Objaw jest odwrotny do awarii, wiec potrzebna jest osobna bramka.
# Samotest idzie pierwszy: straznik, ktory nie umie oblac, niczego nie pilnuje.
add_test(
  NAME harness_guard-selftest
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND bash harness_guard_selftest.sh)

add_test(
  NAME harness_command_integrity
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND python3 harness_command_integrity.py ${CMAKE_BINARY_DIR})

set_tests_properties(harness_command_integrity
                     PROPERTIES DEPENDS harness_guard-selftest)

# Dryft dokumentacji. Lista funkcji skalarnych stoi w rqlFunctions.hpp, ale
# dokumentacja uzytkownika powtarza ja wlasnym tekstem w dwoch osobnych
# repozytoriach i cicho sie z nia rozjezdza. Ani bramka `-c`, ani `test_drift`
# tego nie widza: pierwsza nie kompiluje tekstow, druga konfrontuje silnik z
# modelem zdarzeniowym.
#
# Dokumentacja jest POZA tym drzewem, wiec test rejestruje sie tylko wtedy, gdy
# oba pliki sa na dysku. Bezwarunkowa rejestracja zapalilaby `ninja test` i
# kazdy kontener CI, ktory kopiuje wylacznie to repozytorium.
set(RDB_DOCS_PL
    "$ENV{HOME}/github/dokumentacja-rdb"
    CACHE PATH "Repozytorium dokumentacji PL (dryft listy funkcji)")
set(RDB_DOCS_EN
    "$ENV{HOME}/github/documentation-rdb"
    CACHE PATH "Repozytorium dokumentacji EN (dryft listy funkcji)")

set(RDB_DOC_PL_FUNCTIONS
    "${RDB_DOCS_PL}/konstrukcja-jezyka-zapytan/polecenie-select/wyrazenia-pol-i-funkcje-skalarne.md"
)
set(RDB_DOC_EN_FUNCTIONS
    "${RDB_DOCS_EN}/query-language-construction/select-command/field-expressions-and-scalar-functions.md"
)

if(EXISTS "${RDB_DOC_PL_FUNCTIONS}" AND EXISTS "${RDB_DOC_EN_FUNCTIONS}")
  add_test(
    NAME doc_drift_scalar_functions
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND
      python3 doc_drift_scalar_functions.py
      ${CMAKE_SOURCE_DIR}/src/include/rqlFunctions.hpp
      "PL=${RDB_DOC_PL_FUNCTIONS}" "EN=${RDB_DOC_EN_FUNCTIONS}")
else()
  message(
    STATUS
      "doc_drift_scalar_functions: NIESPRAWDZONY - brak dokumentacji (RDB_DOCS_PL/RDB_DOCS_EN)"
  )
endif()

# Bramka fazy 2 (#446, C2): warstwa osadzalna (biblioteka rdb + jej naglowki
# publiczne) nie trzyma stanu procesu, nie konfiguruje logera i konczy proces
# tylko przez detail::terminateProcess. Uzasadnienie i lista swiadomych wyjatkow
# stoja w samym skrypcie; znany dlug jest wypisywany, ale nie oblewa.
add_test(NAME embedding_boundary
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/embedding_boundary.py
                 ${CMAKE_SOURCE_DIR})

# Tryb wyjatkow rdzenia (#511): cele wydawane bez -fno-exceptions (jednostki z
# wyjatkami i bez nich w jednym programie lamaly ODR), a rdzen sprawdzony przez
# biblioteki kontrolne RDB_NOEXCEPT_CHECK. Czyta compile_commands.json, wiec
# widzi flagi, ktorymi build naprawde kompiluje. Samotest wewnatrz skryptu.
add_test(NAME exception_mode_guard
         COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/exception_mode_guard.py
                 ${CMAKE_BINARY_DIR})

# Dryft wobec dowodow Lean (math_proofs). Wartosci sprawdza ut_proofOracle na
# tablicach z proofOracle.hpp; ten test pilnuje, ze kazde twierdzenie z Profs ma
# wiersz w proof_manifest.tsv i ze tablice nie sa starsze od dowodow. Czyta
# tylko pliki .lean, wiec nie wymaga Lean i biegnie takze w CI.
add_test(
  NAME proof_drift
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
    python3 ${CMAKE_CURRENT_SOURCE_DIR}/proof_drift.py
    ${CMAKE_SOURCE_DIR}/math_proofs
    ${CMAKE_CURRENT_SOURCE_DIR}/proof_manifest.tsv
    ${CMAKE_CURRENT_SOURCE_DIR}/UnitTest/test_proofOracle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/UnitTest/proofOracle.hpp)

# Binding Pythona (#447) w zwyklym `ninja test` buildu z RDB_PYTHON=ON - bez
# drugiej kompilacji silnika w osobnym drzewie. python_stage odswieza kopie .py
# i .pyi, bo POST_BUILD _core nie odpala, gdy zmieniono tylko pakiet. Tryb jawny
# conftest.py (RDB_PYTHON_MODULE_DIR, RDB_XRETRACTOR) zamienia brak zaleznosci,
# nieaktualna kopie i obcy xretractor/xqry w PATH w blad, nie w skip. Pusty
# PYTEST_ADDOPTS: ustawienia uzytkownika nie moga zawezic zestawu np. przez -k.
# Bez etykiety valgrind, wiec poza `ninja test-valgrind`.
if(TARGET _core)
  get_directory_property(
    RDB_PYTHON_INTERPRETER DIRECTORY ${CMAKE_SOURCE_DIR}/src/python DEFINITION
                                     Python_EXECUTABLE)
  add_test(
    NAME python_stage
    COMMAND
      ${CMAKE_COMMAND} -E copy_directory
      ${CMAKE_SOURCE_DIR}/api/python/retractordb
      ${RDB_PYTHON_MODULE_DIR}/retractordb)
  add_test(
    NAME python_binding
    COMMAND
      ${RDB_PYTHON_INTERPRETER} -m pytest -q -ra
      ${CMAKE_SOURCE_DIR}/api/python/tests
      --junitxml=${CMAKE_BINARY_DIR}/python-test-results.xml)
  set_tests_properties(python_stage PROPERTIES LABELS python FIXTURES_SETUP
                                               python_package)
  set_tests_properties(
    python_binding
    PROPERTIES
      LABELS
      python
      FIXTURES_REQUIRED
      python_package
      ENVIRONMENT
      "RDB_PYTHON_MODULE_DIR=${RDB_PYTHON_MODULE_DIR};RDB_XRETRACTOR=$<TARGET_FILE:xretractor>;PYTEST_ADDOPTS="
      ENVIRONMENT_MODIFICATION
      "PATH=path_list_prepend:$<TARGET_FILE_DIR:xqry>;PATH=path_list_prepend:$<TARGET_FILE_DIR:xretractor>"
  )
endif()

# Srodowisko kontroli higieny IPC dla skryptow testowych. Generowane PO
# file(COPY ...) wyzej, bo trafia do tego samego katalogu w drzewie budowy i
# kopia skasowalaby plik wygenerowany wczesniej. Tresc i uzasadnienie: patrz
# IntegrationTest/shmEnv.sh.in oraz blok RDB_BOOST_POSIX_SHM w korzeniu.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/IntegrationTest/shmEnv.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/IntegrationTest/shmEnv.sh @ONLY)
