cmake_minimum_required(VERSION 3.5 FATAL_ERROR)
project(xtrdb LANGUAGES CXX)

add_subdirectory(lib)

set(SOURCES_RDB main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_RDB})

target_link_libraries(${PROJECT_NAME} PUBLIC boost::boost
                                             spdlog::spdlog_header_only rdb)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)
