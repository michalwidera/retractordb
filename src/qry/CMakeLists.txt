cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

project(xqry LANGUAGES CXX)

# Logika klienta jako biblioteka statyczna — reużywana przez testy bez potrzeby
# kompilowania ipcClient/formatters/qry dla każdego testu.
add_library(qry STATIC qry.cpp ipcClient.cpp formatters.cpp serverRouting.cpp)

# ${CMAKE_SOURCE_DIR}/src jest PUBLIC, bo serverRouting.hpp wystawia
# bus::InstanceInfo w swoim interfejsie i dociaga "retractor/lib/bus.hpp" - tak
# samo jak test_bus.cpp.
target_include_directories(
  qry
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(
  qry
  PUBLIC #
         boost::boost #
         spdlog::spdlog_header_only #
         magic_enum::magic_enum #
         common)

# Cienka binarka: tylko launcher + biblioteka qry. bus.cpp dokladana zrodlowo,
# tak samo jak appConfig.cpp: zalezy wylacznie od
# pthread/boost.interprocess/spdlog, wiec nie ciagnie ANTLR-a ani rdb, a
# linkowanie calej biblioteki `retractor` do klienta byloby za drogie. W
# bibliotece `qry` jej NIE ma celowo - testy jednostkowe linkuja jednoczesnie
# `qry` i `retractor`, a dwie kopie bus.o daloby duplikaty symboli.
add_executable(${PROJECT_NAME} qryLauncher.cpp ../retractor/lib/appConfig.cpp
                               ../retractor/lib/bus.cpp)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_SOURCE_DIR}/src/retractor/lib)

target_link_libraries(${PROJECT_NAME} qry)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
