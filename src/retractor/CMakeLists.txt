cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(xretractor LANGUAGES CXX)

add_subdirectory(lib)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

set(SOURCES_RETRACTOR ${BUILDSRCS_CPP} ${BUILDCOMMON_CPP})

add_executable(${PROJECT_NAME} launcher.cpp)

target_link_libraries(
  ${PROJECT_NAME}
  boost::boost
  spdlog::spdlog_header_only
  antlr4_static
  retractor
  rdb
  descParser
  rqlParser)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)

add_custom_target(
  rqlgrammar
  COMMAND
    ${javaPath}/java -classpath ${antlr4Path}/res/antlr-complete.jar
    org.antlr.v4.Tool -o ${CMAKE_SOURCE_DIR}/src/retractor/lib/Parser -lib
    ${CMAKE_SOURCE_DIR}/src/retractor/lib/Parser -encoding UTF-8 -Dlanguage=Cpp
    -listener RQL.g4
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/retractor
  SOURCES ${CMAKE_SOURCE_DIR}/src/retractor/RQL.g4)
