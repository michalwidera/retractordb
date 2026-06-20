cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(
  retractordb
  VERSION 0.1.4
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
add_definitions(-Wno-deprecated-declarations)
add_definitions(-DHAS_BOOST)

option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_SUPERVISOR "Build Go rsupervisor binary (requires Go)" OFF)
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

set(CMAKE_CXX_FLAGS "-fuse-ld=mold -std=c++23")

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
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/config/retractor.toml.example
  DESTINATION share/retractordb)

# Skrypty maintainer DEB: tworzą usera 'retractor' i włączają jednostkę (enable,
# bez natychmiastowego startu).
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postinst;${CMAKE_CURRENT_SOURCE_DIR}/packaging/debian/postrm"
)

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
    /src/uiRdbApp/
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
