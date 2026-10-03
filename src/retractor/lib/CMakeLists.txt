cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

project(retractor LANGUAGES CXX)

add_subdirectory(.antlr)

# Rdzen silnika: wszystko, czego potrzebuje plan, kompilator i model danych - i
# nic z uslugi demona (IPC, watek komunikacyjny, konfiguracja, prezentacja). To
# jest jedyna czesc tego katalogu, ktora laduje silnik osadzony (rdbembed, modul
# Pythona), i jedyna, ktora kompiluje sie bez wyjatkow (RDB_NO_EXCEPTIONS,
# korzen CMakeLists.txt). Lista jest JAWNA, a nie GLOB: nowy plik trafia do
# rdzenia z decyzji, nie przypadkiem.
set(RETRACTOR_CORE_SOURCES
    CRSMath.cpp
    compiler.cpp
    coreState.cpp
    dataModel.cpp
    dumpManager.cpp
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
# deklaracja pojawila sie wraz z rdbembed (faza 3), ktore linkuje rdzen jako
# biblioteke, a nie przez reczna kolejnosc na linii konsolidacji - bez niej
# CMake nie wie, ze librdb.a musi stac ZA kazdym wystapieniem
# libretractorcore.a.
target_link_libraries(retractorcore PUBLIC rdb rqlparser antlr4_static)

# Bez wyjatkow; RQLParser.cpp jest wyspa - steruje parserem ANTLR, ktory
# przerywa parsowanie wyjatkiem, i sam lapie go na granicy (parserRQLString
# zwraca status).
rdb_no_exceptions(retractorcore)
rdb_exception_island(RQLParser.cpp)

# Usluga demona: petla wykonawcy, watek komunikacyjny i IPC
# (Boost.Interprocess), konfiguracja, blokady, prezentacja. Ma wyjatki, bo
# wymagaja ich biblioteki, na ktorych stoi (Boost.Interprocess i program_options
# zglaszaja bledy wylacznie rzutem) - lapane sa na granicy wywolania biblioteki,
# a bledy rdzenia przychodza tu jako rdb::Result.
file(GLOB BUILDSRCS_CPP "*.cpp")
list(REMOVE_ITEM BUILDSRCS_CPP ${RETRACTOR_CORE_SOURCES})
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

# uxSysTermTools (używane przez executorsm.cpp) pochodzi z biblioteki common.
target_link_libraries(${PROJECT_NAME} PUBLIC retractorcore common rdb rqlparser
                                             antlr4_static)

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
