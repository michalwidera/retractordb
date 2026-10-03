cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

# L2 architektury osadzania (docs/embedded-roadmap.md sekcja 2). Biblioteka
# celowo cienka: trzyma POLITYKE CYKLU ZYCIA instancji silnika, a nie logike -
# ta zostaje w rdb (L1) i w retractor (plan, kompilator, model danych), ktore od
# fazy 3 ta warstwa tez laczy.
project(rdbembed LANGUAGES CXX)

file(GLOB_RECURSE BUILDRDBEMBED_CC "*.cc")

add_library(${PROJECT_NAME} STATIC ${BUILDRDBEMBED_CC})

# PUBLIC, bo konsument (modul Pythona, testy) potrzebuje tych samych bibliotek
# na linii konsolidacji; kolejnosc dla bibliotek statycznych wyznacza CMake z
# tego grafu (retractorcore deklaruje rdb, rqlparser i antlr4_static jako swoje
# zaleznosci). Rdzen, nie `retractor`: silnik osadzony nie linkuje uslugi
# demona.
target_link_libraries(${PROJECT_NAME} PUBLIC retractorcore rdb)

# Fasada osadzenia jest czescia rdzenia bez wyjatkow (RDB_NO_EXCEPTIONS): bledy
# wychodza z niej jako rdb::Result, a na wyjatek Pythona zamienia je dopiero
# modul nanobind.
rdb_no_exceptions(${PROJECT_NAME})

# Naglowki silnika sa wlaczane sciezka od korzenia src
# ("retractor/lib/dataModel.hpp"), tak jak robia to testy jednostkowe; naglowek
# publiczny rdb/embed/engine.hpp nie wlacza zadnego.
target_include_directories(
  ${PROJECT_NAME} #
  PRIVATE #
          ${CMAKE_SOURCE_DIR}/src #
          ${Boost_INCLUDE_DIR} #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})
