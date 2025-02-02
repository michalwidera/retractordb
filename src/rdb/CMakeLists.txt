cmake_minimum_required(VERSION 3.6)
project(xtrdb LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

add_subdirectory(lib)

set(SOURCES_RDB main.cpp ../common/uxSysTermTools.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_RDB})

target_link_libraries(${PROJECT_NAME} boost::boost spdlog::spdlog_header_only
                      rdb descparser)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)
