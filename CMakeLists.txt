cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

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
# Sonda pomiarowa benchmarku (E1 czas obliczeń, E3 rozmiar planu). Domyślnie
# WYŁĄCZONA: kod sondy jest wtedy całkowicie wycięty z kompilacji (#ifdef
# RDB_BENCH_PROBE), zero kosztu. Włączenie tylko do pomiarów wydajności — NIE
# dla produkcji. Użycie: scripts/buildrdb.sh probe
option(RDB_BENCH_PROBE
       "Build with benchmark measurement probe (E1/E3); NOT for production" OFF)
if(RDB_BENCH_PROBE)
  add_compile_definitions(RDB_BENCH_PROBE)
  message(
    WARNING
      "RDB_BENCH_PROBE=ON — benchmark probe compiled in; NOT a production build."
  )
endif()

# Licznik kopii obiektu query (diagnostyka inwestygacji speed_improvement:
# kwantyfikuje kopie query/interwał w gorącej ścieżce processRows). Domyślnie
# WYŁĄCZONY: pod #ifdef RDB_COPY_COUNTER, zero kosztu w produkcji. Włączenie
# tylko do pomiaru mechanizmu (nie perturbuje E1 istotnie — inkrement relaxed).
option(
  RDB_COPY_COUNTER
  "Count query-object copies (speed_improvement diagnostic); NOT for production"
  OFF)
if(RDB_COPY_COUNTER)
  add_compile_definitions(RDB_COPY_COUNTER)
  message(
    WARNING
      "RDB_COPY_COUNTER=ON — query copy counter compiled in; NOT a production build."
  )
endif()

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

add_subdirectory(src)
add_subdirectory(scripts)
add_subdirectory(test)

option(BUILD_EXAMPLES "Build example programs" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples/generator)
endif()

include(InstallRequiredSystemLibraries)

set(CPACK_GENERATOR "DEB;TGZ")
set(CPACK_SOURCE_GENERATOR "DEB;TGZ")

# Prefiks /usr dla pakietów BINARNYCH (DEB i TGZ): binaria -> /usr/bin,
# jednostka systemd -> /usr/lib/systemd/system (ścieżka rozpoznawana przez
# systemd). TGZ rozpakowany w / daje te same ścieżki. Dla pakietu ŹRÓDŁOWEGO
# prefiks jest zerowany w project-config.cmake (drzewo źródeł w korzeniu
# archiwum).
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_PROJECT_CONFIG_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/cpack/project-config.cmake")

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

set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-source-${CMAKE_SYSTEM_NAME}"
)

set(CPACK_SOURCE_IGNORE_FILES
    /build/
    /CMakeFiles/
    /CMakeCache.txt
    /bin/
    /lib/
    # Wygenerowane / lokalne artefakty — nie należą do pakietu źródłowego.
    /coverage/
    /Testing/
    /dokumentacja-rdb/
    /.claude/
    /.venv/
    /.cache/
    /src/UML/
    /src/retractor/UML/
    /src/rdb/UML/
    /.vscode/
    /scripts/.vim/
    /scripts/dev-test/
    /scripts/sublime/
    /.circleci/
    /.github/
    /.gitignore
    /.gitattributes
    /CMakeUserPresets.json
    /examples/
    /.git/
    /test/
    /embold.yaml
    README.md
    CONTRIBUTING.md
    CODE_OF_CONDUCT.md
    /docker/
    Dockerfile
    DockerConan.txt
    sonar-project.properties
    CLAUDE.md
    .clang-format
    .clang-tidy
    .claudeignore
    .dockerignore
    ~$
    ${CPACK_SOURCE_IGNORE_FILES})

include(CPack) # include after setting variables above to take effect
