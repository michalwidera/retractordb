#cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project( retractordb-src )

include_directories( ${Boost_INCLUDE_DIR} "share" "include" )

set(SOURCES_RETRACTOR
  retractor/Processor.cpp
  retractor/executorsm.cpp
  retractor/CRSMath.cpp
  share/Buffer.cpp
  share/dbstream.cpp
  share/QStruct.cpp
  share/RandomFile.cpp
  share/inputFileInstance.cpp
  share/SOperations.cpp
)

set(SOURCES_COMPILER
  compiler/comLauncher.cpp
  compiler/compiler.cpp
  compiler/parser.cpp
  share/QStruct.cpp
  share/SOperations.cpp
)

set(SOURCES_DUMPER
  dumper/dumper.cpp
  share/Buffer.cpp
  share/dbstream.cpp
  share/QStruct.cpp
  share/RandomFile.cpp
)

set(SOURCES_QRY
  qry/qryLauncher.cpp
  qry/qry.cpp
)

set(SOURCES_RDB
  rdb/main.cpp
  rdb/faccfs.cc
  rdb/faccposix.cc
  rdb/faccposixprm.cc
  rdb/desc.cc
  rdb/dsacc.cc
  rdb/payloadacc.cc
  rdb/dataset.cc
)

add_executable( xretractor ${SOURCES_RETRACTOR} )
add_executable( xcompiler ${SOURCES_COMPILER} )
add_executable( xdumper ${SOURCES_DUMPER} )
add_executable( xqry ${SOURCES_QRY} )
add_executable( xrdb ${SOURCES_RDB} )

target_link_libraries( xretractor ${CONAN_LIBS} )
target_link_libraries( xcompiler ${CONAN_LIBS} )
target_link_libraries( xdumper ${CONAN_LIBS} )
target_link_libraries( xqry ${CONAN_LIBS} )
target_link_libraries( xrdb ${CONAN_LIBS} )

install(TARGETS xretractor xcompiler xdumper xqry xrdb DESTINATION ~/.local/bin)
