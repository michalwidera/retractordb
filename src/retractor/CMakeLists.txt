cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(xretractor LANGUAGES CXX)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(antlr4-runtime REQUIRED)

set(SOURCES_RETRACTOR
    Processor.cpp
    executorsm.cpp
    CRSMath.cpp
    dataModel.cpp
    expressionEvaluator.cpp
    launcher.cpp
    compiler.cpp
    parser.cpp
    dumper.cpp
    Parser/RQLLexer.cpp
    Parser/RQLParser.cpp
    ../share/SOperations.cpp
    ../share/QStruct.cpp
    ../share/inputFileInstance.cpp
    ../share/convertTypes.cpp
    ../rdb/faccfs.cc
    ../rdb/faccposix.cc
    ../rdb/faccposixprm.cc
    ../rdb/descriptor.cc
    ../rdb/storageacc.cc
    ../rdb/faccbindev.cc
    ../rdb/facctxtsrc.cc
    ../rdb/payload.cc)

file(GLOB BUILDSRCS "*.cc")

add_executable(${PROJECT_NAME} ${SOURCES_RETRACTOR})

target_link_libraries(${PROJECT_NAME} boost::boost spdlog::spdlog_header_only
                      antlr4_static)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)

add_custom_target(
  grammar
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/antlr4call.sh RQL.g4
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/retractor
  SOURCES ${CMAKE_SOURCE_DIR}/src/retractor/RQL.g4)
