cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

project(xqry LANGUAGES CXX)

# Logika klienta jako biblioteka statyczna — reużywana przez testy bez potrzeby
# kompilowania ipc_transport/formatters/qry dla każdego testu.
add_library(qry STATIC qry.cpp ipc_transport.cpp formatters.cpp)

target_include_directories(qry PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  qry
  PUBLIC #
         boost::boost #
         spdlog::spdlog_header_only #
         magic_enum::magic_enum #
         common)

# Cienka binarka: tylko launcher + biblioteka qry
add_executable(${PROJECT_NAME} qryLauncher.cpp ../retractor/lib/appConfig.cpp)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_SOURCE_DIR}/src/retractor/lib)

target_link_libraries(${PROJECT_NAME} qry)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
