cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(retractor LANGUAGES CXX)

add_subdirectory(.antlr)

file(GLOB BUILDSRCS_CPP "*.cpp")

add_library(${PROJECT_NAME} STATIC ${BUILDSRCS_CPP})

message(STATUS "setup rqlgrammar in PROJECT_SOURCE_DIR=" ${PROJECT_SOURCE_DIR})

add_custom_target(
  rqlgrammar
  COMMAND
    ${javaPath}/java -classpath ${antlr4Path}/res/antlr-complete.jar
    org.antlr.v4.Tool -o ${PROJECT_SOURCE_DIR}/.antlr -lib
    ${PROJECT_SOURCE_DIR}/.antlr -encoding UTF-8 -Dlanguage=Cpp -listener RQL.g4
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  SOURCES ${PROJECT_SOURCE_DIR}/RQL.g4)

target_include_directories(
  ${PROJECT_NAME} #
  PRIVATE #
          ${Boost_INCLUDE_DIR} #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})

# uxSysTermTools (używane przez executorsm.cpp) pochodzi z biblioteki common
target_link_libraries(${PROJECT_NAME} PUBLIC common)

if(ENABLE_UNITY_BUILD)
  set_target_properties(${PROJECT_NAME} PROPERTIES UNITY_BUILD ON)
endif()

# Precompiled headers — te same co rdb, skraca czas kompilacji ~20 plików .cpp.
target_precompile_headers(
  ${PROJECT_NAME} PRIVATE #
  <algorithm> #
  <filesystem> #
  <memory> #
  <string> #
  <vector> #
  <boost/rational.hpp> #
  <spdlog/spdlog.h>)
