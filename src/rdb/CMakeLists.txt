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

message(STATUS "setup descgrammar in PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR} )

add_custom_target(
  descgrammar
  COMMAND
    ${javaPath}/java -classpath ${antlr4Path}/res/antlr-complete.jar
    org.antlr.v4.Tool -o ${PROJECT_SOURCE_DIR}/lib/descParser -lib
    ${PROJECT_SOURCE_DIR}/lib/descParser -encoding UTF-8 -Dlanguage=Cpp
    -listener DESC.g4
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  SOURCES ${PROJECT_SOURCE_DIR}/DESC.g4)
