cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

project(retractor LANGUAGES CXX)

add_subdirectory(.antlr)

# Rdzen silnika (#446, C3): plan, kompilator, model danych i os czasu - nic z
# uslugi (IPC, magistrala, watek komunikacyjny, konfiguracja, blokady,
# prezentacja). Tylko ta czesc katalogu trafia do silnika osadzonego (rdbembed,
# modul Pythona). Lista jest JAWNA, a nie GLOB: nowy plik trafia do rdzenia z
# decyzji, nie przypadkiem.
set(RETRACTOR_CORE_SOURCES
    CRSMath.cpp
    compiler.cpp
    coreState.cpp
    dataModel.cpp
    dumpManager.cpp
    executor_rt.cpp
    exprSimplify.cpp
    expressionEvaluator.cpp
    expressionShape.cpp
    field.cpp
    persistentCounter.cpp
    planSource.cpp
    qTree.cpp
    query.cpp
    RQLParser.cpp
    streamInstance.cpp
    token.cpp)

add_library(retractorcore STATIC ${RETRACTOR_CORE_SOURCES})

# PUBLIC na rdzeniu: kazdy, kto go linkuje (usluga, xretractor, testy,
# rdbembed), widzi te same wartosci przelacznikow, ktore skompilowaly rdzen.
target_compile_definitions(
  retractorcore
  PUBLIC
    RDB_OPT_DEDUP_SUBSTRATES=$<BOOL:${RDB_OPT_DEDUP_SUBSTRATES}>
    RDB_OPT_SHARE_EQUIVALENT_SELECTS=$<BOOL:${RDB_OPT_SHARE_EQUIVALENT_SELECTS}>
    RDB_OPT_COMMUTATIVE_ADD=$<BOOL:${RDB_OPT_COMMUTATIVE_ADD}>
    RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$<BOOL:${RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES}>
    RDB_OPT_SIMPLIFY_EXPRESSIONS=$<BOOL:${RDB_OPT_SIMPLIFY_EXPRESSIONS}>
    aggressive_expr_optimization=$<BOOL:${aggressive_expr_optimization}>)

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
  retractorcore #
  PRIVATE #
          ${Boost_INCLUDE_DIR} #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})

# rdb i rqlparser (z antlr4_static) sa zaleznosciami rdzenia od zawsze;
# deklaracja jest potrzebna od kiedy rdbembed linkuje rdzen jako biblioteke -
# bez niej CMake nie wie, ze librdb.a musi stac ZA kazdym wystapieniem
# libretractorcore.a.
target_link_libraries(retractorcore PUBLIC rdb rqlparser antlr4_static)

rdb_no_exceptions(retractorcore)
rdb_exception_island(RQLParser.cpp)

# Usluga: reszta katalogu.
file(GLOB BUILDSRCS_CPP "*.cpp")
list(TRANSFORM RETRACTOR_CORE_SOURCES
     PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/" OUTPUT_VARIABLE
                                            RETRACTOR_CORE_PATHS)
list(REMOVE_ITEM BUILDSRCS_CPP ${RETRACTOR_CORE_PATHS})

add_library(${PROJECT_NAME} STATIC ${BUILDSRCS_CPP})

target_include_directories(
  ${PROJECT_NAME} #
  PRIVATE #
          ${Boost_INCLUDE_DIR} #
          ${spdlog_INCLUDE_DIR} #
          ${fmt_INCLUDE_DIR})

# uxSysTermTools (używane przez executorsm.cpp) pochodzi z biblioteki common
target_link_libraries(${PROJECT_NAME} PUBLIC retractorcore common)

if(ENABLE_UNITY_BUILD)
  set_target_properties(${PROJECT_NAME} retractorcore PROPERTIES UNITY_BUILD ON)
endif()

# Precompiled headers - te same co rdb, skraca czas kompilacji ~20 plików .cpp.
foreach(RDB_PCH_TARGET ${PROJECT_NAME} retractorcore)
  target_precompile_headers(
    ${RDB_PCH_TARGET}
    PRIVATE #
    <algorithm> #
    <filesystem> #
    <memory> #
    <string> #
    <vector> #
    <boost/rational.hpp> #
    <spdlog/spdlog.h>)
endforeach()
