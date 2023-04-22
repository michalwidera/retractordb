cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(xretractor LANGUAGES CXX)

add_subdirectory(Parser)

find_package(antlr4-runtime REQUIRED)

file(GLOB BUILDSRCS_CPP "*.cpp")

set(SOURCES_RETRACTOR ${BUILDSRCS_CPP})

add_executable(${PROJECT_NAME} ${SOURCES_RETRACTOR})

target_link_libraries(
  ${PROJECT_NAME} PUBLIC boost::boost spdlog::spdlog_header_only antlr4_static
                         rdb parser)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)

add_custom_target(
  grammar
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/antlr4call.sh RQL.g4
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/retractor
  SOURCES ${CMAKE_SOURCE_DIR}/src/retractor/RQL.g4)
