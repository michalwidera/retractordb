cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(xtrdb LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

add_subdirectory(lib)

set(SOURCES_RDB main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_RDB})

target_link_libraries(${PROJECT_NAME} boost::boost spdlog::spdlog_header_only
                      rdb descParser)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)

add_custom_target(
  descgrammar
  COMMAND
    ${javaPath}/java -classpath ${antlr4Path}/res/antlr-complete.jar
    org.antlr.v4.Tool -o ${CMAKE_SOURCE_DIR}/src/rdb/lib/descParser -lib
    ${CMAKE_SOURCE_DIR}/src/rdb/lib/descParser -encoding UTF-8 -Dlanguage=Cpp
    -listener DESC.g4
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/rdb
  SOURCES ${CMAKE_SOURCE_DIR}/src/rdb/DESC.g4)
