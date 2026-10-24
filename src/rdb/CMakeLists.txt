cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

project(xtrdb LANGUAGES CXX)

find_package(antlr4-runtime REQUIRED)
find_package(antlr4 REQUIRED)

add_subdirectory(lib)

set(SOURCES_RDB
    xtrdbLauncher.cpp
    xtrdbStorageMap.cpp
    cmdDropFile.cpp
    cmdFieldAccess.cpp
    cmdMeta.cpp
    cmdOpen.cpp
    cmdPayload.cpp
    cmdStatus.cpp
    cmdStorage.cpp)

# appConfig.cpp dokladana zrodlowo, tak samo jak w xqry: straznik "silnik
# dziala" czyta katalog blokad (paths.lock_dir) z tej samej konfiguracji co
# silnik. Plik zalezy wylacznie od toml++ i spdlog, wiec biblioteka `rdb` nie
# dostaje zaleznosci od biblioteki `retractor`. bus.cpp, lockFile.cpp i
# serverName.cpp dochodza tak samo jak w xqry: drugi dowod zycia silnika to slot
# na magistrali (#573), niezalezny od pliku blokady w /tmp.
add_executable(
  ${PROJECT_NAME}
  ${SOURCES_RDB} ../retractor/lib/appConfig.cpp ../retractor/lib/bus.cpp
  ../retractor/lib/lockFile.cpp ../retractor/lib/serverName.cpp)

target_include_directories(${PROJECT_NAME}
                           PRIVATE ${CMAKE_SOURCE_DIR}/src/retractor/lib)

target_link_libraries(
  ${PROJECT_NAME}
  boost::boost
  spdlog::spdlog_header_only
  magic_enum::magic_enum
  rdb
  descparser
  common)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                                        COMPONENT portable)
