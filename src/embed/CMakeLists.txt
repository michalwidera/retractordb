cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

# Fasada osadzenia (#446, C2): rdb::embed::Engine - wlasciciel stanu JEDNEJ
# instancji silnika w procesie hosta (notatnik, binding Pythona). Biblioteka
# celowo cienka: trzyma polityke cyklu zycia instancji i granice bledow
# (ThrowOnFatalScope), a nie logike - ta zostaje w rdb.
project(rdbembed LANGUAGES CXX)

file(GLOB_RECURSE BUILDRDBEMBED_CC "*.cc")

add_library(${PROJECT_NAME} STATIC ${BUILDRDBEMBED_CC})

target_link_libraries(${PROJECT_NAME} PUBLIC rdb)

target_include_directories(
  ${PROJECT_NAME} #
  PRIVATE #
          ${Boost_INCLUDE_DIR} #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})
