cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

# Fasada osadzenia (#446, C2): rdb::embed::Engine - wlasciciel stanu JEDNEJ
# instancji silnika w procesie hosta (notatnik, binding Pythona). Biblioteka
# celowo cienka: trzyma polityke cyklu zycia instancji i granice bledow
# (Result), a nie logike - ta zostaje w rdb i w retractorcore (plan, kompilator,
# model danych; od C3).
project(rdbembed LANGUAGES CXX)

file(GLOB_RECURSE BUILDRDBEMBED_CC "*.cc")

# appConfig.cpp dokladana zrodlowo, tak samo jak w xtrdb i xqry: jawnie podany
# plik konfiguracji daje paths.lock_dir i embedded.lenient_lock_dirs (#450).
# Zalezy wylacznie od toml++ i spdlog, wiec nie wnosi uslugi (magistrala, IPC) z
# biblioteki `retractor`.
add_library(${PROJECT_NAME} STATIC ${BUILDRDBEMBED_CC}
                                   ../retractor/lib/appConfig.cpp)

# Rdzen, nie `retractor`: silnik osadzony nie linkuje uslugi (magistrala, IPC) -
# instancje osadzone nie pojawiaja sie na magistrali (#445). PUBLIC, bo
# konsument (modul Pythona, testy) potrzebuje tych samych bibliotek na linii
# konsolidacji, a ich kolejnosc wyznacza CMake z tego grafu.
target_link_libraries(${PROJECT_NAME} PUBLIC retractorcore rdb)

# Naglowki silnika wlaczane sciezka od korzenia src
# ("retractor/lib/dataModel.hpp"), tak jak w testach jednostkowych; naglowek
# publiczny rdb/embed/engine.hpp nie wlacza zadnego z nich.
target_include_directories(
  ${PROJECT_NAME} #
  PRIVATE #
          ${CMAKE_SOURCE_DIR}/src #
          ${Boost_INCLUDE_DIR} #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})

rdb_noexcept_check(${PROJECT_NAME} ISLANDS configBridge.cc
                   ../retractor/lib/appConfig.cpp)
