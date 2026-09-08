# Zakres `<min>...<max>`, nie jedna liczba: dolna granica mowi, czym da sie ten
# projekt skonfigurowac (3.20), gorna — z ktorej wersji brane sa DOMYSLNE
# ZACHOWANIA polityk (4.4). Nowszy cmake dostaje wiec zachowanie NEW zamiast
# ostrzegac, a starszy nadal konfiguruje projekt.
#
# Powod historyczny: `tool_requires("cmake/[>=3.25]")` nie byl przypiety, wiec
# CI w swiezym kontenerze pobieral nowsza wersje niz deweloper. 2026-08-18
# rozjazd ujawnil sie jako CMP0219 (od 4.4) w testach integracyjnych, gdzie
# `add_test` jest przeslonione makrem: przy zachowaniu OLD ginal backslash i
# `grep '\.shadow$'` stawal sie `grep '.shadow$'`. Podloga wersji stoi teraz w
# conanfile.py i w RDB_CMAKE_MIN_VERSION w scripts/buildrdb.sh — dryft w gore
# jest dozwolony, a te trzy miejsca podnosi sie razem i swiadomie.
cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

# Jedyne źródło prawdy o wersji: plik VERSION w korzeniu repo. Czytany też przez
# conanfile.py (set_version) i Dockerfile (--build-arg). NIE wpisywać wersji
# tutaj na sztywno — zmieniać wyłącznie plik VERSION.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" RDB_VERSION)

project(
  retractordb
  VERSION ${RDB_VERSION}
  LANGUAGES CXX)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# Domyślny prefiks instalacji: katalog użytkownika (~/.local), aby `ninja
# install` działał bez sudo (domyślny /usr/local wymaga roota → Permission
# denied). Warunek łapie: świeżą konfigurację (INITIALIZED_TO_DEFAULT),
# istniejący cache z nietkniętym /usr/local oraz prefiks wskazujący w drzewo
# źródeł (CMAKE_SOURCE_DIR) — instalacja do korzenia repo zaśmieca bin/ lib/
# share/ i jest zawsze błędna. Własny sensowny prefiks (dowolny inny) jest
# uszanowany. Pakiety CPack używają CPACK_PACKAGING_INSTALL_PREFIX (/usr) — bez
# zmian.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT
   OR CMAKE_INSTALL_PREFIX STREQUAL "/usr/local"
   OR CMAKE_INSTALL_PREFIX STREQUAL CMAKE_SOURCE_DIR)
  set(CMAKE_INSTALL_PREFIX
      "$ENV{HOME}/.local"
      CACHE PATH "Install prefix (user-local, no sudo)" FORCE)
endif()

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
# Polityka logowania per konfiguracja: na urządzeniu pomiarowym (rpi400) /tmp
# leży na karcie SD, a logger flushuje każdą linię — w Release do logu mogą
# trafiać wyłącznie błędy (makra poniżej ERROR są wycinane w kompilacji). W
# Debug pełna diagnostyka łącznie z SPDLOG_DEBUG (m.in. rotacja segmentów).
# Poziom runtime dopasowuje setupLoggerMain() na podstawie SPDLOG_ACTIVE_LEVEL.
add_compile_definitions(
  $<IF:$<CONFIG:Release>,SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_ERROR,SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
)
add_definitions(-Wno-deprecated-declarations)
add_definitions(-DHAS_BOOST)

option(ENABLE_COVERAGE "Enable code coverage" OFF)
# Unity Build łączy wiele .cpp w jedną jednostkę kompilacji, zmniejszając liczbę
# uruchomień kompilatora i wielokrotne parsowanie nagłówków. Może ujawnić
# konflikty nazw między TU (np. anonimowe namespace, statyczne zmienne) — testuj
# przed włączeniem. Użycie: scripts/buildrdb.sh debug -DENABLE_UNITY_BUILD=ON
option(ENABLE_UNITY_BUILD "Enable unity build for faster compilation" OFF)

# Przełączniki optymalizatora planu używane w kontrolowanych badaniach
# ablacyjnych. Domyślna konfiguracja zachowuje dotychczasowe działanie.
option(RDB_OPT_DEDUP_SUBSTRATES
       "Deduplicate structurally identical compiler substrates" ON)
option(RDB_OPT_SHARE_EQUIVALENT_SELECTS
       "Share equivalent public SELECT computations" ON)
option(RDB_OPT_COMMUTATIVE_ADD
       "Use A+B == B+A when fingerprinting equivalent SELECT plans" ON)
option(RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES
       "Factor matched time moves across the stream hash operator" ON)
option(RDB_OPT_SIMPLIFY_EXPRESSIONS
       "Fold constants and simplify algebraic field expressions" ON)

# Agresywne przepisania wyrazenia — regula D w R3 (`a*a` -> `a^2`). CELOWO poza
# rodzina RDB_OPT_*: tamte piec przelacznikow tworzy macierz ablacyjna i sa
# wypisywane przez `xretractor --build-info`, a korpus H9 weryfikuje ZBIOR
# kluczy tego wypisu dokladnie (test/research_gate/h9/validate_corpus.py). Nowy
# klucz zapalilby bramke badawcza.
#
# Domyslnie OFF, i nie z powodu poprawnosci: regula D nie zmienia wyniku (patrz
# exactPower w expressionEvaluator). Powodem jest APARATURA — mierzone plany H9
# stoja na normie `Sqrt(A[0]*A[0]+B[0]*B[0])`, a bramka wymaga, zeby R3 nie
# ruszyla ich ani razu. Wlaczenie: scripts/buildrdb.sh release
# -Daggressive_expr_optimization=ON
option(aggressive_expr_optimization
       "Aggressive field-expression rewrites (rule D: a*a -> a^2)" OFF)

if(NOT RDB_OPT_SHARE_EQUIVALENT_SELECTS AND RDB_OPT_COMMUTATIVE_ADD)
  message(
    FATAL_ERROR
      "Invalid optimizer configuration: RDB_OPT_COMMUTATIVE_ADD=ON requires RDB_OPT_SHARE_EQUIVALENT_SELECTS=ON."
  )
endif()

# Sondy pomiarowe benchmarku (E1 czas slotu, E3 rozmiar planu, E4 praca na slot,
# K6 materializacja). Domyślnie WYŁĄCZONE: kod sond jest wtedy wycinany przez
# kompilator (if constexpr na stałych rdb_probe_* z probeConfig.h), zero kosztu
# — ale nadal PRZECHODZI przez kompilator, więc nie może zgnić niezauważony.
# Włączenie tylko do pomiarów wydajności — NIE dla produkcji. Użycie:
# scripts/buildrdb.sh probe
#
# Jedna opcja steruje wszystkimi sondami, bo tak wprowadzają je eksperymenty.
# Stałe są osobne, żeby dało się w przyszłości zbudować wariant z jedną sondą.
option(
  RDB_BENCH_PROBE
  "Build with benchmark measurement probes (E1/E3/E4/K6); NOT for production"
  OFF)
if(RDB_BENCH_PROBE)
  set(RDB_PROBE_SLOT true)
  set(RDB_PROBE_PLAN true)
  set(RDB_PROBE_MATERIALIZE true)
  set(RDB_PROBE_WORK true)
  message(
    WARNING
      "RDB_BENCH_PROBE=ON — benchmark probes compiled in; NOT a production build."
  )
else()
  set(RDB_PROBE_SLOT false)
  set(RDB_PROBE_PLAN false)
  set(RDB_PROBE_MATERIALIZE false)
  set(RDB_PROBE_WORK false)
endif()

message(STATUS "RDB_OPT_DEDUP_SUBSTRATES=${RDB_OPT_DEDUP_SUBSTRATES}")
message(
  STATUS "RDB_OPT_SHARE_EQUIVALENT_SELECTS=${RDB_OPT_SHARE_EQUIVALENT_SELECTS}")
message(STATUS "RDB_OPT_COMMUTATIVE_ADD=${RDB_OPT_COMMUTATIVE_ADD}")
message(
  STATUS
    "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=${RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES}"
)
message(STATUS "RDB_OPT_SIMPLIFY_EXPRESSIONS=${RDB_OPT_SIMPLIFY_EXPRESSIONS}")
message(STATUS "RDB_BENCH_PROBE=${RDB_BENCH_PROBE}")

# mold jest domyślnym linkerem (szybszy niż ld/lld), ale na platformach z
# ograniczoną pamięcią (np. RPi) potrafi zgłaszać błędy alokacji przy
# linkowaniu. RDB_USE_MOLD=OFF w env wymusza (FORCE) wyłączenie przy każdej
# konfiguracji — działa też przez 'conan build', które nie pozwala wstrzyknąć -D
# (patrz scripts/buildrdb.sh: opcje 'mold'/'nomold').
if(DEFINED ENV{RDB_USE_MOLD})
  set(RDB_USE_MOLD
      $ENV{RDB_USE_MOLD}
      CACHE BOOL "Use mold linker" FORCE)
else()
  option(RDB_USE_MOLD "Use mold linker" ON)
endif()

set(CMAKE_CXX_FLAGS "-std=c++23")
if(RDB_USE_MOLD)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fuse-ld=mold")
endif()

# ccache buforuje wyniki kompilacji, przyspieszając powtórne budowanie (np.
# iteracja dev/test, przełączanie branchy) -- szczególnie odczuwalne na
# maszynach z ograniczoną mocą/RAM (np. RPi), gdzie pełna rekompilacja
# Boost+antlr4+GTest jest kosztowna. Wykrywane automatycznie: brak ccache w
# systemie nie blokuje budowy, po prostu nic nie przyspiesza. Wyłączenie:
# RDB_USE_CCACHE=OFF w env (patrz scripts/buildrdb.sh, analogicznie do
# RDB_USE_MOLD/mold/nomold).
if(DEFINED ENV{RDB_USE_CCACHE})
  set(RDB_USE_CCACHE
      $ENV{RDB_USE_CCACHE}
      CACHE BOOL "Use ccache compiler launcher" FORCE)
else()
  option(RDB_USE_CCACHE "Use ccache compiler launcher if available" ON)
endif()
if(RDB_USE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(
      STATUS "ccache found, enabling as compiler launcher: ${CCACHE_PROGRAM}")
  else()
    message(
      STATUS
        "ccache not found, skipping (install: scripts/buildrdb.sh toolchain, or apt install ccache)"
    )
  endif()
endif()

if(ENABLE_COVERAGE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# cmake --preset conan-debug -DENABLE_COVERAGE=ON cd build/Debug ninja clean &&
# ninja ctest cd ../.. gcovr --root . --filter 'src/' --gcov-executable gcov-14
# --exclude '.*\.antlr.*' build/Debug --html-details coverage/coverage.html
# --xml coverage/coverage.xml --print-summary

find_package(Boost CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(antlr4-runtime CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(antlr4 CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(tomlplusplus CONFIG REQUIRED)

# Oznacz wszystkie importowane targety (Conan) jako systemowe — clang-tidy z
# SystemHeaders:false pomija ich nagłówki (brak ostrzeżeń i notatek z .conan2/).
get_property(
  _imported_targets
  DIRECTORY .
  PROPERTY IMPORTED_TARGETS)
foreach(_target ${_imported_targets})
  if(TARGET ${_target})
    get_target_property(_inc ${_target} INTERFACE_INCLUDE_DIRECTORIES)
    if(_inc)
      set_target_properties(
        ${_target} PROPERTIES INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${_inc}")
    endif()
  endif()
endforeach()

find_program(PROG_VALGRIND valgrind REQUIRED)
message(STATUS "Required tool: ${PROG_VALGRIND}")

if(DEFINED antlr4_PACKAGE_FOLDER_DEBUG)
  set(antlr4Path ${antlr4_PACKAGE_FOLDER_DEBUG})
else()
  set(antlr4Path ${antlr4_PACKAGE_FOLDER_RELEASE})
endif()

if(DEFINED openjdk_BIN_DIRS_DEBUG)
  set(javaPath ${openjdk_BIN_DIRS_DEBUG})
else()
  set(javaPath ${openjdk_BIN_DIRS_RELEASE})
endif()

# message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}") message(STATUS
# "antlr4Path: ${antlr4Path}") message(STATUS "javaPath: ${javaPath}")

include(CTest)
enable_testing()

file(WRITE ${CMAKE_BINARY_DIR}/CTestCustom.cmake
     "set(CTEST_PARALLEL_LEVEL 0)\n")

include_directories(
  ${Boost_INCLUDE_DIR}
  ${Boost_LIBRARY_DIRS}
  ${spdlog_INCLUDE_DIR}
  ${fmt_INCLUDE_DIR}
  ${magic_enum_INCLUDE_DIR}
  ${antlr4-runtime_INCLUDE_DIR}
  ${PROJECT_BINARY_DIR}/generated # config.h, serviceDefaults.h, probeConfig.h —
                                  # per katalog build
  src/include)

# toml++ jako include SYSTEMOWY (-isystem) — wycisza ostrzeżenia z jego
# nagłówków (np. -Wdeprecated-literal-operator pod C++23). To kod zewnętrzny,
# header-only, nie chcemy go diagnozować jak własnego (-I).
include_directories(SYSTEM ${tomlplusplus_INCLUDE_DIR})

link_directories(${Boost_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compileCommands req. from
                                      # .vsccode/c_cpp_properties.json

# Jedno źródło prawdy dla kanonicznego pliku zapytań serwisu. Podstawiane
# (configure_file) do: jednostki systemd (ExecStart), nagłówka serviceDefaults.h
# (domyślna wartość AppConfig) oraz postinst (utworzenie pliku). Dystrybutor
# może nadpisać -D...
set(RETRACTOR_QUERY_FILE
    "/etc/retractor/startup.rql"
    CACHE
      PATH
      "Kanoniczny plik zapytań serwisu xretractor (ExecStart / fallback AppConfig)"
)

# API klienckie (api/) jest rozwijane i testowane RAZEM z silnikiem, ale nie
# WEWNATRZ niego: zaden cel z src/ nie linkuje niczego z api/. Katalog jest
# konfigurowany zawsze (koszt konfiguracji, nie budowania); z `all`, z `ninja
# install` i z pakietu wypada dwiema osobnymi drogami ponizej. Wejscia opt-in:
# install-withapi, test-api oraz -DRDB_WITH_API=ON dla cpack.
#
# api/cpp jest tez samodzielnym projektem (README opisuje add_subdirectory z
# obcego drzewa), wiec decyzja o komponencie nalezy do NAS, nie do niego:
# przekazujemy ja zmienna, ktora u samodzielnego konsumenta jest pusta.
set(RDB_API_INSTALL_COMPONENT COMPONENT api EXCLUDE_FROM_ALL)
add_subdirectory(api/cpp)

# EXCLUDE_FROM_ALL na celach, nie na katalogu: add_subdirectory z opcja
# EXCLUDE_FROM_ALL odcina rowniez reguly install() od nadrzednego
# cmake_install.cmake, przez co `cmake --install --component api` nie mialby
# czego zainstalowac.
set_target_properties(rdb_client rdb_api_json rdb_monitor
                      PROPERTIES EXCLUDE_FROM_ALL TRUE)
add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)

option(RDB_WITH_API "Dolacz API klienckie (api/) do pakietow cpack" OFF)

# Testy API sa oznaczone etykieta `api` i domyslnie wypadaja z `ninja test`.
# CMAKE_CTEST_ARGUMENTS steruje wywolaniem ctest w celu `test`; -LE api to
# jedyny sposob, by je pominac bez usuwania ich z rejestru (DISABLED czynilby je
# nieuruchamialnymi takze przez `ninja test-api`).
if(NOT RDB_WITH_API)
  list(APPEND CMAKE_CTEST_ARGUMENTS -LE api)
endif()

# `ninja install-withapi` — jedyne wejscie, ktore instaluje API. Zaleznosci
# gwarantuja zbudowanie silnika i celow API przed instalacja; dwa wywolania
# `cmake --install` odpowiadaja komponentowi domyslnemu i komponentowi `api`.
add_custom_target(
  install-withapi
  COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR}
  COMMAND ${CMAKE_COMMAND} --install ${CMAKE_BINARY_DIR} --component api
  COMMENT "Instalacja silnika wraz z API klienckim (komponent api)")
add_dependencies(
  install-withapi
  xretractor
  xqry
  xtrdb
  rdb_client
  rdb_api_json
  rdb_monitor)

# `ninja test-api` — testy API, budowane i uruchamiane wylacznie na zadanie.
add_custom_target(
  test-api
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR} -L api
          --output-on-failure
  COMMENT "Testy API klienckiego (ctest -L api)")
add_dependencies(test-api xretractor xqry test_api_client)

include(InstallRequiredSystemLibraries)

set(CPACK_GENERATOR "DEB;TGZ")
# Pakiet ZRODLOWY jest wylaczony. Nie bylo dla niego odbiorcy, a jego zawartosc
# byla nieprawdziwa: wpisy CPACK_SOURCE_IGNORE_FILES to regexy na pelnej
# sciezce, wiec `/lib/` wycinalo `src/rdb/lib/` i `src/retractor/lib/` — 84
# pliki rdzenia silnika wraz z obiema gramatykami — a `README.md` wycinalo kazdy
# README na kazdej glebokosci. Test pakowania tego nie lapal, bo sprawdzal
# wylacznie NIEOBECNOSC artefaktow, nigdy obecnosc zrodel.
#
# Wylaczenie musi byc jawne. Samo usuniecie CPACK_SOURCE_GENERATOR wlacza
# domyslne generatory CPacka (TBZ2, TGZ, TXZ, TZ) — bez listy ignorowanych
# spakowalyby cale `build/`. Te opcje czyta include(CPack) nizej; CMP0077 (NEW
# od cmake_minimum_required powyzej) sprawia, ze option() honoruje ustawiona tu
# zmienna. Przywrocenie pakietu zrodlowego to osobne zadanie: wymaga
# zakotwiczonych wzorcow i asercji POZYTYWNEJ w tescie pakowania.
set(CPACK_SOURCE_RPM OFF)
set(CPACK_SOURCE_TBZ2 OFF)
set(CPACK_SOURCE_TGZ OFF)
set(CPACK_SOURCE_TXZ OFF)
set(CPACK_SOURCE_TZ OFF)
set(CPACK_SOURCE_ZIP OFF)

# Prefiks /usr dla pakietow BINARNYCH (DEB i TGZ): binaria -> /usr/bin,
# jednostka systemd -> /usr/lib/systemd/system (sciezka rozpoznawana przez
# systemd). TGZ rozpakowany w / daje te same sciezki.
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")

# Jednostka systemd dla xretractor: ExecStart wskazuje realną ścieżkę binarium w
# pakiecie (prefiks + bindir). Generowana z szablonu .in.
set(RETRACTOR_BIN "${CPACK_PACKAGING_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/packaging/systemd/xretractor.service.in
  ${CMAKE_BINARY_DIR}/xretractor.service @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/xretractor.service
        DESTINATION lib/systemd/system)

# Przykładowa konfiguracja: instalowana jako .example, NIGDY jako żywy plik —
# nie nadpisuje konfiguracji administratora. DESTINATION jest WZGLĘDNY (pod
# prefiks /usr → /usr/share/retractordb), bo bezwzględny /etc łamie staging
# cpack (próba zapisu do realnego /etc). Na Debianie postinst kopiuje przykład
# do /etc/retractor.
#
# COMPONENT service + EXCLUDE_FROM_ALL: pliki .example trafiają WYŁĄCZNIE do
# pakietu (cpack), a deweloperski `ninja install`/`cmake --install` do ~/.local
# je pomija. Komponent 'service' jest jawnie wymieniony w CPACK_COMPONENTS_ALL
# poniżej, więc cpack i tak go dołącza.
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/config/retractor.toml.example
        ${CMAKE_CURRENT_SOURCE_DIR}/packaging/config/service.env.example
  DESTINATION share/retractordb
  COMPONENT service
  EXCLUDE_FROM_ALL)

# Skrypty maintainer DEB: tworzą usera 'retractor' i włączają jednostkę (enable,
# bez natychmiastowego startu). postinst generowany z .in (podstawienie
# @RETRACTOR_QUERY_FILE@), by ścieżka pliku zapytań pochodziła z tego samego
# źródła co ExecStart jednostki.
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst.in
               ${CMAKE_BINARY_DIR}/debian/postinst @ONLY)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_BINARY_DIR}/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postrm"
)

# Komponenty pakietu: 'Unspecified' to domyślny komponent binarek (install bez
# COMPONENT), 'service' to pliki .example oznaczone EXCLUDE_FROM_ALL. Pliki
# EXCLUDE_FROM_ALL trafiają do pakietu TYLKO przy włączonym component-install
# (DEB/ARCHIVE) i jawnym wymienieniu komponentu w CPACK_COMPONENTS_ALL — w
# trybie monolitycznym cpack je pomija. GROUPING=ALL_COMPONENTS_IN_ONE scala oba
# komponenty z powrotem w jeden .deb/.tar.gz (nie rozbija na pakiety
# per-komponent).
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)
set(CPACK_COMPONENTS_ALL Unspecified service)
# Komponent 'api' dopisywany tylko przy -DRDB_WITH_API=ON. Cpack ustala
# zawartosc pakietu na etapie konfiguracji, wiec tego jednego wejscia nie da sie
# zastapic celem ninja — bez tej flagi API nigdy nie trafia do .deb/.tar.gz.
if(RDB_WITH_API)
  list(APPEND CPACK_COMPONENTS_ALL api)
endif()

set(CPACK_PACKAGE_NAME "retractordb")
set(CPACK_PACKAGE_VERSION
    "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
)
set(CPACK_PACKAGE_VENDOR "Michal Widera")
set(CPACK_PACKAGE_CONTACT "michal@widera.com.pl")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RetractorDB")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME}")

include(CPack) # include after setting variables above to take effect
