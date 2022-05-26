# cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project(retractordb-src)

include_directories(${Boost_INCLUDE_DIR} share include)

set(SOURCES_RETRACTOR
    retractor/Processor.cpp
    retractor/executorsm.cpp
    retractor/CRSMath.cpp
    retractor/dbstream.cpp
    share/QStruct.cpp
    share/inputFileInstance.cpp
    share/SOperations.cpp
    rdb/faccfs.cc
    rdb/faccposix.cc
    rdb/faccposixprm.cc
    rdb/desc.cc
    rdb/dsacc.cc
    rdb/payloadacc.cc
    rdb/dataset.cc)

set(SOURCES_COMPILER
    compiler/comLauncher.cpp
    compiler/compiler.cpp
    compiler/parser.cpp
    share/QStruct.cpp
    share/SOperations.cpp
    compiler/Parser/RQLLexer.cpp
    compiler/Parser/RQLParser.cpp)

set(SOURCES_DUMPER
    dumper/dumper.cpp
    share/QStruct.cpp
    rdb/faccfs.cc
    rdb/faccposix.cc
    rdb/faccposixprm.cc
    rdb/desc.cc
    rdb/dsacc.cc
    rdb/payloadacc.cc
    rdb/dataset.cc)

set(SOURCES_QRY qry/qryLauncher.cpp qry/qry.cpp)

set(SOURCES_RDB
    rdb/main.cpp
    rdb/faccfs.cc
    rdb/faccposix.cc
    rdb/faccposixprm.cc
    rdb/desc.cc
    rdb/dsacc.cc
    rdb/payloadacc.cc
    rdb/dataset.cc)

add_executable(xretractor ${SOURCES_RETRACTOR})
add_executable(xcompiler ${SOURCES_COMPILER})
add_executable(xdumper ${SOURCES_DUMPER})
add_executable(xqry ${SOURCES_QRY})
add_executable(xtrdb ${SOURCES_RDB})

target_link_libraries(xretractor ${CONAN_LIBS})
target_link_libraries(xcompiler ${CONAN_LIBS})
target_link_libraries(xdumper ${CONAN_LIBS})
target_link_libraries(xqry ${CONAN_LIBS})
target_link_libraries(xtrdb ${CONAN_LIBS})

install(TARGETS xretractor xcompiler xdumper xqry xtrdb
        DESTINATION ~/.local/bin)

add_custom_target(
  grammar
  COMMAND ${CMAKE_SOURCE_DIR}/scripts/antlr4call.sh RQL.g4
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/compiler
  SOURCES ${PROJECT_SOURCE_DIR}/compiler/RQL.g4)

# pip install cmake-format sudo apt install astyle
add_custom_target(
  astyle
  COMMAND astyle --options=astyle.options --recursive *.cpp *.hpp *.cc *.h
          --exclude=compiler/Parser
  COMMAND find ${CMAKE_SOURCE_DIR} -name CMakeLists.txt | xargs cmake-format -i
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_custom_target(
  cformat
  COMMAND find . -name *.h -o -iname *.cpp -o -iname *.hpp -o -iname *.cc | grep
          --invert-match 'Parser/' | xargs clang-format -i -style=Google
  COMMAND find ${CMAKE_SOURCE_DIR} -name CMakeLists.txt | xargs cmake-format -i
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

add_custom_target(
  vim
  COMMAND [ ! -f ~/.vim ] && mkdir -p ~/.vim
  COMMAND cp -r ${CMAKE_SOURCE_DIR}/scripts/vim/* ~/.vim)

find_package(Git QUIET)

string(TIMESTAMP GIT_CURRENT_BUILDTIME "%y%m%d%H%M")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
  execute_process(
    COMMAND ${GIT_EXECUTABLE} branch --show-current
    OUTPUT_VARIABLE GIT_CURRENT_BRANCH
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  string(STRIP "${GIT_CURRENT_BRANCH}" GIT_CURRENT_BRANCH)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
    OUTPUT_VARIABLE GIT_PREV_COMMIT
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
  string(STRIP "${GIT_PREV_COMMIT}" GIT_PREV_COMMIT)
else()
  set(GIT_CURRENT_BRANCH Deatached)
  set(GIT_PREV_COMMIT none)
endif()
configure_file(config.h.in ${PROJECT_SOURCE_DIR}/include/config.h @ONLY)
