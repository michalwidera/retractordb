cmake_minimum_required(VERSION 3.6 FATAL_ERROR)

project(rdb LANGUAGES CXX)

add_subdirectory(.antlr)

file(GLOB_RECURSE BUILDRDB_CC "*.cc")

add_library(${PROJECT_NAME} STATIC ${BUILDRDB_CC})

message(STATUS "setup descgrammar in PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR} )

add_custom_target(
  descgrammar
  COMMAND
    ${javaPath}/java -classpath ${antlr4Path}/res/antlr-complete.jar
    org.antlr.v4.Tool -o ${PROJECT_SOURCE_DIR}/.antlr -lib
    ${PROJECT_SOURCE_DIR}/.antlr -encoding UTF-8 -Dlanguage=Cpp
    -listener DESC.g4
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  SOURCES ${PROJECT_SOURCE_DIR}/DESC.g4)

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIR} ${spdlog_INCLUDE_DIR}
                          ${fmt_INCLUDE_DIR})

