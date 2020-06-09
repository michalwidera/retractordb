cmake_minimum_required(VERSION 3.5)
project (generator)

set(Boost_USE_STATIC_LIBS ON )
set(Boost_USE_MULTITHREADED ON)

FIND_PACKAGE( Boost COMPONENTS    
    system
    program_options
    REQUIRED )

include_directories(${Boost_INCLUDE_DIR})

link_libraries(${Boost_LIBRARIES})

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})
ADD_DEFINITIONS( "-DHAS_BOOST" )

add_executable(xgenerator generator.cpp)
