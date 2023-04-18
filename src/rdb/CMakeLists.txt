cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(xtrdb LANGUAGES CXX)

add_definitions(-DSPDLOG_FMT_EXTERNAL)
add_definitions(-DFMT_HEADER_ONLY)
add_definitions(-Wno-deprecated-declarations)

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

set(SOURCES_RDB
    main.cpp
    faccfs.cc
    faccposix.cc
    faccposixprm.cc
    faccbindev.cc
    facctxtsrc.cc
    descriptor.cc
    storageacc.cc
    payload.cc
    ../share/convertTypes.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_RDB})

target_link_libraries(${PROJECT_NAME} PUBLIC boost::boost
                                             spdlog::spdlog_header_only)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)
