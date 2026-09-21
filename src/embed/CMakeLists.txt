cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

# L2 architektury osadzania (docs/embedded-roadmap.md sekcja 2). Biblioteka celowo cienka:
# trzyma POLITYKE CYKLU ZYCIA instancji silnika, a nie logike - ta zostaje w rdb (L1).
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
