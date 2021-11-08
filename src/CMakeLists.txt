#cmake_minimum_required(VERSION 3.3 FATAL_ERROR )

project( retractordb-src )

include_directories( ${Boost_INCLUDE_DIR} "share" )

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

set(SOURCES_STG
  stage/main.cpp
  stage/faccfs.cc
  stage/faccposix.cc
  stage/desc.cc
  stage/dsacc.cc
  stage/payloadacc.cc
)

add_executable( xretractor ${SOURCES_RETRACTOR} )
add_executable( xcompiler ${SOURCES_COMPILER} )
add_executable( xdumper ${SOURCES_DUMPER} )
add_executable( xqry ${SOURCES_QRY} )
add_executable( xstage ${SOURCES_STG} )

target_link_libraries( xretractor ${CONAN_LIBS} )
target_link_libraries( xcompiler ${CONAN_LIBS} )
target_link_libraries( xdumper ${CONAN_LIBS} )
target_link_libraries( xqry ${CONAN_LIBS} )
target_link_libraries( xstage ${CONAN_LIBS} )

install(TARGETS xretractor xcompiler xdumper xqry xstage DESTINATION ~/.local/bin)
