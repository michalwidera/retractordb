cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

project(rdbpython LANGUAGES CXX)

# Modul rozszerzenia CPythona: warstwa magazynu (etap 1a) i silnik osadzony
# rdb::embed::Engine (J1-J3, docs/jupyter-integration.md). Katalog wchodzi do
# konfiguracji wylacznie przy RDB_PYTHON=ON - patrz src/CMakeLists.txt.

# Platforma hosta jest konsekwencja celu, nie jego wyborem: rozszerzenia dla
# Windows nikt jeszcze nie budowal. Flaga zadana na niewspieranym hoscie ma
# zatrzymac konfiguracje, a nie zostac po cichu pominieta - zielony build bez
# artefaktu jest gorszy niz blad (docs/build-options.md, regula 3).
if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux" OR CMAKE_SYSTEM_NAME STREQUAL
                                              "Darwin"))
  message(
    FATAL_ERROR
      "RDB_PYTHON=ON requires a Linux or macOS host; this one is ${CMAKE_SYSTEM_NAME}."
  )
endif()

# Modul Python, nie Python3: nanobind-config.cmake sprawdza cele Python:: i
# zmienne Python_* i konczy sie FATAL_ERROR, gdy znajdzie tylko Python3_*.
# Sprawdzone w nanobind 3.1.0 (nanobind-config.cmake:3-8).
#
# Development.Module, nie Development: ten drugi wymaga biblioteki libpython do
# linkowania, ktorej modul rozszerzenia NIE linkuje (symbole rozwiazuje
# interpreter, ktory go laduje). Na manylinux libpython zwykle nie istnieje i
# pelne Development zatrzymaloby konfiguracje bez powodu.
#
# Prog 3.10 jest wspolny dla nas i dla nanobinda od wersji 3.0.
#
# SKBUILD_SABI_COMPONENT ustawia wylacznie scikit-build-core, i to tylko przy
# budowie kola abi3 (wheel.py-api w pyproject.toml, interpreter >= 3.12) - wtedy
# rowna sie Development.SABIModule, bez ktorego STABLE_ABI nizej nic nie robi. W
# zwyklym buildzie jest pusta i lista komponentow zostaje ta sama co dotad.
find_package(Python 3.10 REQUIRED COMPONENTS Interpreter Development.Module
                                             ${SKBUILD_SABI_COMPONENT})

# Katalog CMake nanobinda bierzemy z samego interpretera, nie z find_package po
# nazwie: nanobind instaluje sie przez pip do site-packages TEGO interpretera, a
# CMake nie ma powodu tam zagladac. Dzieki temu wybor interpretera (venv, pyenv,
# systemowy) jednoznacznie wyznacza uzyty nanobind.
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_VARIABLE RDB_NANOBIND_CMAKE_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
  RESULT_VARIABLE RDB_NANOBIND_PROBE
  ERROR_QUIET)

if(NOT RDB_NANOBIND_PROBE EQUAL 0)
  message(
    FATAL_ERROR
      "RDB_PYTHON=ON requires nanobind for ${Python_EXECUTABLE}, which does not have it. "
      "Create an environment that does, and point the build at it:\n"
      "  scripts/python-venv.sh\n"
      "  cmake -DRDB_PYTHON=ON -DPython_EXECUTABLE=<repo>/.venv-python/bin/python3 .\n"
      "A Homebrew or distribution Python refuses a direct pip install (PEP 668), which is "
      "why the script exists. See docs/build-options.md.")
endif()

list(APPEND CMAKE_PREFIX_PATH "${RDB_NANOBIND_CMAKE_DIR}")
find_package(nanobind CONFIG REQUIRED)

# Biblioteki statyczne silnika powstaja na potrzeby plikow wykonywalnych, wiec
# nie musza byc pozycyjnie niezalezne. Modul wspoldzielony, ktory je wciaga, juz
# musi - na Linuksie inaczej nie linkuje sie wcale. Wlaczamy to TU, a nie w ich
# wlasnych plikach CMake, zeby build bez RDB_PYTHON zachowal sie bez zmian.
foreach(RDB_PIC_TARGET rdb descparser common retractor rqlparser rdbembed)
  set_property(TARGET ${RDB_PIC_TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)
endforeach()

# NB_STATIC: czesc wspolna nanobinda linkuje sie w modul zamiast do osobnej
# biblioteki wspoldzielonej. Przy jednym module w drzewie to mniej plikow do
# wydania i brak pytania o rpath.
#
# STABLE_ABI: jedno kolo dla Pythona 3.12 i nowszych (abi3). nanobind wlacza je
# tylko przy Pythonie >= 3.12 i obecnym celu Python::SABIModule (komponent
# wyzej), wiec buildy deweloperskie i kola dla 3.10/3.11 powstaja jak dotad.
nanobind_add_module(_core NB_STATIC STABLE_ABI module.cpp)

# rdbembed (L2) wnosi za soba retractor, rqlparser i antlr4_static - plan,
# kompilator i model danych, ktore od fazy 3 stoja za Engine.
target_link_libraries(
  _core
  PRIVATE #
          boost::boost
          spdlog::spdlog_header_only
          magic_enum::magic_enum
          rdbembed
          rdb
          descparser
          common)

# Modul ladowany bez instalacji: build/<config>/python jest kompletnym korzeniem
# importu, a pliki .py pakietu sa do niego kopiowane obok skompilowanego
# rozszerzenia. Katalog nalezy do konkretnej konfiguracji, bo build/Debug i
# build/Release konfigurowane sa niezaleznie - jedna kopia w drzewie zrodel
# dawalaby modul z tej konfiguracji, ktora zbudowano jako ostatnia.
set(RDB_PYTHON_PACKAGE_ROOT "${CMAKE_BINARY_DIR}/python")

set_target_properties(_core PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                       "${RDB_PYTHON_PACKAGE_ROOT}/retractordb")

# copy_directory nie kasuje tego, czego nie kopiuje, wiec zbudowany modul
# przezywa odswiezenie zrodel pakietu obok niego.
add_custom_command(
  TARGET _core
  POST_BUILD
  COMMAND
    "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/api/python/retractordb"
    "${RDB_PYTHON_PACKAGE_ROOT}/retractordb"
  COMMENT "Staging retractordb package sources next to _core")

set(RDB_PYTHON_MODULE_DIR
    "${RDB_PYTHON_PACKAGE_ROOT}"
    CACHE PATH "Importable package root holding the built retractordb._core"
          FORCE)

# Instalacja dla kola (pyproject.toml: install.components = python). Cel
# wzgledny wobec prefiksu, ktory scikit-build-core ustawia na katalog pakietow
# kola, wiec _core laduje obok plikow .py z api/python/retractordb.
# EXCLUDE_FROM_ALL: zwykly `ninja install` go pomija, a cpack bierze tylko
# komponenty z CPACK_COMPONENTS_ALL, wiec zadne z nich modulu nie instaluje.
install(
  TARGETS _core
  LIBRARY DESTINATION retractordb
          COMPONENT python
          EXCLUDE_FROM_ALL)

message(STATUS "RDB_PYTHON: ${Python_EXECUTABLE} -> ${RDB_PYTHON_MODULE_DIR}")
