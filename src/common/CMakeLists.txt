cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(common LANGUAGES CXX)

# Małe źródła współdzielone przez xretractor, xtrdb i xqry. Budowane raz jako
# biblioteka statyczna zamiast być kompilowane oddzielnie w każdym docelowym
# projekcie.
add_library(${PROJECT_NAME} STATIC uxSysTermTools.cpp)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})
