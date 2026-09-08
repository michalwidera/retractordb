cmake_minimum_required(VERSION 3.20)
project(
  RetractorDBClient
  VERSION 0.1.0
  LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
find_package(Threads REQUIRED)
if(NOT TARGET boost::boost AND NOT TARGET Boost::headers)
  find_package(Boost CONFIG REQUIRED)
endif()

add_library(rdb_api_json STATIC src/json.cpp)
target_link_libraries(rdb_api_json PRIVATE $<BUILD_INTERFACE:Boost::headers>)
target_compile_definitions(rdb_api_json PUBLIC BOOST_JSON_NO_LIB)
set_target_properties(rdb_api_json PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(rdb_client STATIC src/client.cpp)
add_library(RetractorDB::client ALIAS rdb_client)
set_target_properties(rdb_client PROPERTIES EXPORT_NAME client
                                            POSITION_INDEPENDENT_CODE ON)
target_compile_features(rdb_client PUBLIC cxx_std_23)
target_include_directories(
  rdb_client PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>)
target_link_libraries(
  rdb_client PRIVATE rdb_api_json $<BUILD_INTERFACE:Boost::headers>
                     Threads::Threads)

add_executable(rdb_monitor examples/monitor.cpp)
target_link_libraries(rdb_monitor PRIVATE RetractorDB::client)

# RDB_API_INSTALL_COMPONENT ustawia drzewo nadrzedne (silnik) na komponent 'api'
# z EXCLUDE_FROM_ALL: wtedy `ninja install` i `cmake --install` bez --component
# pomijaja API, a cpack dolacza je wylacznie gdy 'api' jest wymienione w
# CPACK_COMPONENTS_ALL (-DRDB_WITH_API=ON). Wejsciem deweloperskim jest `ninja
# install-withapi`. W budowie samodzielnej zmienna jest pusta i instalacja
# zachowuje sie zwyczajnie.
install(
  TARGETS rdb_client rdb_api_json
  EXPORT RetractorDBClientTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} ${RDB_API_INSTALL_COMPONENT})
install(
  DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  ${RDB_API_INSTALL_COMPONENT})
install(
  EXPORT RetractorDBClientTargets
  NAMESPACE RetractorDB::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RetractorDBClient
  ${RDB_API_INSTALL_COMPONENT})
configure_package_config_file(
  cmake/RetractorDBClientConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/RetractorDBClientConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RetractorDBClient)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/RetractorDBClientConfigVersion.cmake
  COMPATIBILITY SameMajorVersion)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/RetractorDBClientConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/RetractorDBClientConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RetractorDBClient
  ${RDB_API_INSTALL_COMPONENT})
