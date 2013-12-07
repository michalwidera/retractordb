#
# Copyright 2010-2012 Streams Lab Michal Widera
#
# RESTRICTED RIGHTS LEGEND
#
# Use, duplication or disclosure by any commercial industry (public or
# private), private individual or by any Government Agency without an
# expressed written consent of release from Streams Lab Michal Widera
# is subject to restriction of the Rights.
#
# Manufacturer is:
#     Streams Lab Michal Widera
#     Reymonta str. 56/1
#     Poland, Zabrze, 41-800
#     (+0 48) 606 995 918

#
# How to debug:
# note: Add -DCMAKE_BUILD_TYPE=Debug into "arguments"
# field of qt creator to create and run debug code
# BOOST_ROOT to enviroment 
#

# Continious Integration
# http://streams-lab.com:8010/waterfall
# http://streams-lab.com/doc/html/

cmake_minimum_required(VERSION 2.8 FATAL_ERROR )

project( database )

SET( Boost_USE_STATIC_LIBS ON )

FIND_PACKAGE( Boost 1.55.0 COMPONENTS
    thread
    filesystem
    serialization
    regex
    program_options
    chrono
    date_time
    system
    REQUIRED )

#SET( Boost_INCLUDE_DIRS D:\\boost\\include\\boost-1_50 )

MESSAGE(STATUS "Boost information:")
MESSAGE(STATUS "Boost_INCLUDE_DIRS: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost_LIBRARY_DIRS: ${Boost_LIBRARY_DIRS}")
MESSAGE(STATUS "Boost_LIBRARIES: ${Boost_LIBRARIES}")

include_directories( ${Boost_INCLUDE_DIR} "share" )

link_directories( ${Boost_LIBRARY_DIRS} )
#link_libraries(
#    libboost_program_options-mgw44-mt-1_49.a
#    libboost_serialization-mgw44-mt-1_49.a
#    libboost_regex-mgw44-mt-1_49.a
#)

link_libraries(
    ${Boost_LIBRARIES}
)

#http://msdn.microsoft.com/en-us/library/aa383745.aspx
if (WIN32 AND __COMPILER_GNU)

    #wymagane dla boost::asio
    link_libraries(
        ws2_32.a
        mswsock.a
        )

    # mingw-gcc fails to link boost::thread
    add_definitions( -DBOOST_THREAD_USE_LIB )
    add_definitions( -D_WIN32_WINNT=0x0601  )

endif (WIN32 AND __COMPILER_GNU)

#
# wersje release bez tej opcji !!!
#
add_definitions(-DCMAKE_BUILD_TYPE=Debug )

add_definitions(
  -D_CONSOLE
  -D_DEBUG
)

set(SOURCES_EXECUTOR
  executor/Processor.cpp
  executor/SelfTest.cpp
  executor/executor.cpp
  executor/CRSMath.cpp
  executor/IPhStrMgr.cpp
  executor/NetServer.cpp
  share/Buffer.cpp
  share/dbstream.cpp
  share/QStruct.cpp
  share/RandomFile.cpp
  share/stdafx.cpp
  share/inputFileInstance.cpp
  share/IPhHead.cpp
)
if(WIN32)
  set(SOURCES_EXECUTOR ${SOURCES_EXECUTOR}
        executor/Processor.h
        executor/IPhStrMgr.h
        executor/NetServer.h
        share/dbstream.h
        share/IPhHead.h
	share/QStruct.h 
	share/stdafx.h 
	share/RandomFile.h 
	share/Buffer.h
        share/inputFileInstance.h
	)
endif(WIN32)

set(SOURCES_COMPILER
  compiler/compiler.cpp
  compiler/parser.cpp
  compiler/parser_v2.cpp
  share/QStruct.cpp
  share/stdafx.cpp
)
if(WIN32)
  set(SOURCES_COMPILER ${SOURCES_COMPILER}
	share/QStruct.h 
	share/stdafx.h
	share/tokendefset.h
	)
endif(WIN32)

set(SOURCES_DUMPER
  dumper/dumper.cpp
  share/Buffer.cpp
  share/dbstream.cpp
  share/QStruct.cpp
  share/RandomFile.cpp
  share/stdafx.cpp
)
if(WIN32)
  set(SOURCES_DUMPER ${SOURCES_DUMPER} 
        share/dbstream.h
	share/QStruct.h 
	share/stdafx.h 
	share/tokendefset.h
	)
endif(WIN32)

set(SOURCES_STAGE
  stage/stage.cpp
  share/stdafx.cpp
  share/inputFileInstance.cpp
  share/QStruct.cpp
  regtest/query.txt
  regtest/gotest.bat
  regtest/prepare_runtime_enviroment.bat
  regtest/gocheck.sh
  regtest/gotest.sh
  regtest/query-lnx.txt
  share/rlutil.h
  share/fsmtemplate.h
  stage/test.cpp
  stage/test_bpt.cpp
  stage/server.cpp
)
set(SOURCES_CONSOLE
  console/console.cpp
  console/continiousterminal.cpp
  console/continiousterminal.h
  console/mxterm.cpp
  console/mxterm.h
  share/stdafx.cpp
)
set(SOURCES_STAGE_1
  stage/stage_1.cpp
  share/stdafx.cpp
  share/rlutil.h
  stage/test.cpp
  stage/test_bpt.cpp
)

set(SOURCES_TEST
  regtest/query.txt
  regtest/gotest.sh
  regtest/query-lnx.txt
  regtest/gotest.bat
  regtest/prepare_runtime_enviroment.bat
)

set(SOURCES_STAGE_2
  stage/stage_2.cpp
  share/stdafx.cpp
  share/rlutil.h
)

if(WIN32)
  set(SOURCES_DUMPER ${SOURCES_DUMPER}
        share/stdafx.h
        )
endif(WIN32)


add_executable( xexecutor ${SOURCES_EXECUTOR} )
add_executable( xcompiler ${SOURCES_COMPILER} )
add_executable( xdumper ${SOURCES_DUMPER} )
add_executable( xconsole ${SOURCES_CONSOLE} )
add_executable( xstage ${SOURCES_STAGE} )
add_executable( xstage_1 ${SOURCES_STAGE_1} )
add_executable( xstage_2 ${SOURCES_STAGE_2} )

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

    IF (Boost_FOUND)
       INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
       ADD_DEFINITIONS( "-DHAS_BOOST" )
    ENDIF()

    find_package( Threads )

    target_link_libraries( xexecutor ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt )
    target_link_libraries( xcompiler ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt )
    target_link_libraries( xdumper ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt )
    target_link_libraries( xconsole ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt -lpanel -lncurses -lcdk )
    target_link_libraries( xstage ${Boost_LIBRARIES}   ${CMAKE_THREAD_LIBS_INIT} -lrt )
    target_link_libraries( xstage_1 ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt )
    target_link_libraries( xstage_2 ${Boost_LIBRARIES}  ${CMAKE_THREAD_LIBS_INIT} -lrt )

    add_custom_target( check ALL
        COMMAND chmod +x regtest/gocheck.sh
    )

    add_custom_target( chmod ALL
        COMMAND chmod +x regtest/gotest.sh
    )
    
    add_custom_target( gohome ALL
        DEPENDS xexecutor xcompiler xdumper xconsole xstage xstage_1 xstage_2
        COMMAND mkdir -p ~/xdir
        COMMAND cp x* ~/xdir/
        COMMAND chmod +x ~/xdir/x*
    )

endif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")

