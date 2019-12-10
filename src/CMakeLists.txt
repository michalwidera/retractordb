# How to debug:
# note: Add -DCMAKE_BUILD_TYPE=Debug into "arguments"
# field of qt creator to create and run debug code
# BOOST_ROOT to enviroment 
#

#
# Google testing
# http://www.thebigblob.com/getting-started-with-google-test-on-ubuntu/
#

#
# Formating
# http://astyle.sourceforge.net/
# astyle *.*  cpp --style=google --indent=spaces -S -xG -C -K -xU -N  
#

# Continious Integration
# http://streams-lab.com:8010/waterfall
# http://streams-lab.com/doc/html/
#
# Enviroment
# sudo route add -net 224.0.0.0/4 dev eth0

#
# Setup enviroment
# sudo ./b2 install --prefix=/usr/lib
# sudo mv boost /usr/include/boost
#
# sudo apt-get install gcc cmake libncurses-dev libcdk5-dev libgtest-dev libboost-all-dev make build-essential cppcheck libcppunit-dev libccpunit-doc 
# jesli chcemy używać podglądy grafczne planów zapytań
# sudo apt-get install graphviz gpicview
# 
# cmake CMakelist.txt
# make aptinstall
#

cmake_minimum_required(VERSION 3.0 FATAL_ERROR )

project( database-src )

set(Boost_USE_STATIC_LIBS ON )
set(Boost_USE_MULTITHREADED ON)

FIND_PACKAGE( Boost COMPONENTS
    thread
    filesystem
    serialization
    regex
    program_options
    chrono
    date_time
    system
    REQUIRED )

MESSAGE(STATUS "Boost information:")
MESSAGE(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
MESSAGE(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

include_directories( ${Boost_INCLUDE_DIR} "share" )

link_libraries(
    ${Boost_LIBRARIES}
)

# Release version without Debug option !!!
#

add_definitions(-DCMAKE_BUILD_TYPE=Debug )

set(CMAKE_BUILD_TYPE Debug)

add_definitions(
  -D_CONSOLE
  -D_DEBUG
)

set(SOURCES_ABRACADABRA
  abracadabra/Processor.cpp
  abracadabra/executorsm.cpp
  abracadabra/CRSMath.cpp
  share/Buffer.cpp
  share/dbstream.cpp
  share/QStruct.cpp
  share/RandomFile.cpp
  share/inputFileInstance.cpp
  share/SOperations.cpp
)

set(SOURCES_COMPILER
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
  qry/qry.cpp
)

add_executable( xabracadabra ${SOURCES_ABRACADABRA} )
add_executable( xcompiler ${SOURCES_COMPILER} )
add_executable( xdumper ${SOURCES_DUMPER} )
add_executable( xqry ${SOURCES_QRY} )

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
ADD_DEFINITIONS( "-DHAS_BOOST" )

find_package( Threads )

target_link_libraries( xabracadabra ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt -ldl )    
target_link_libraries( xcompiler ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt -ldl )
target_link_libraries( xdumper ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt -ldl )
target_link_libraries( xqry ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt -ldl )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  add_custom_target(iwyu
    COMMAND iwyu_tool -p "${CMAKE_BINARY_DIR}"
    COMMENT "Running include-what-you-use tool"
    VERBATIM
  )

install(TARGETS xabracadabra xcompiler xdumper xqry DESTINATION /usr/local/bin) 
