cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(xqry LANGUAGES CXX)

set(SOURCES_QRY qryLauncher.cpp qry.cpp)

add_executable(${PROJECT_NAME} ${SOURCES_QRY})

target_link_libraries(${PROJECT_NAME} PUBLIC boost::boost
                                             spdlog::spdlog_header_only)

install(TARGETS ${PROJECT_NAME} DESTINATION ~/.local/bin)
