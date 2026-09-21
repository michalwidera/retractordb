cmake_minimum_required(VERSION 3.20...4.4 FATAL_ERROR)

include_directories(AFTER ${CMAKE_SOURCE_DIR}/src/include
                    ${CMAKE_SOURCE_DIR}/src)

file(
  GLOB V_GLOB CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  "*.cpp")

# Opakowanie kazdego testu jednostkowego. Na Linuksie jest to valgrind - taka
# jest tam definicja "test przeszedl": bez wycieku i bez odczytu
# niezainicjowanej pamieci. Gdzie valgrinda nie ma (Apple Silicon nie ma go i
# miec nie bedzie), lista jest PUSTA i ctest uruchamia binarium wprost;
# rownowaznikiem strazy pamieci jest wtedy konfiguracja z
# -DRDB_SANITIZE=address,undefined, ktora wkompilowuje kontrole w same binaria.
if(PROG_VALGRIND)
  set(RDB_UT_LAUNCHER ${PROG_VALGRIND} --vgdb=no --error-exitcode=1
                      --leak-check=full)
else()
  set(RDB_UT_LAUNCHER "")
endif()

# Limit czasu POZA valgrindem. Bez opakowania test jednostkowy konczy sie w
# ulamku sekundy (najdluzszy w drzewie: ~2 s), wiec dwie minuty nie sa progiem
# wydajnosci - sa granica miedzy "wolno" a "zawieszony". Rozroznienie jest
# praktyczne: zawieszony test bez limitu zatrzymuje CALA suite i nie wiadomo
# nawet, ktory to byl. Tak wlasnie stanelo pierwsze uruchomienie na macOS, gdzie
# rtAbsoluteSleep liczylo termin na niewlasciwej osi zegara i spalo bez konca.
# Pod valgrindem limitu nie ustawiamy: tam czasy sa o rzad wielkosci wieksze i
# zalezne od obciazenia maszyny, wiec kazda liczba bylaby zgadywaniem.
set(RDB_UT_TIMEOUT 120)

list(FILTER V_GLOB EXCLUDE REGEX ".*test_qry\\.cpp$")

# foreach(item ${V_GLOB}) message( ${item}) endforeach()

set(LIBRARY_TEST_SET
    boost::boost
    gtest::gtest
    spdlog::spdlog_header_only
    magic_enum::magic_enum
    antlr4_static
    retractor
    rdbembed
    rdb
    qry
    rqlparser
    descparser)

# launch/main.cpp kompilowany raz jako OBJECT library - reużywany przez
# wszystkie testy zamiast być kompilowany 26× oddzielnie.
add_library(test_main_obj OBJECT launch/main.cpp)
target_link_libraries(test_main_obj PRIVATE gtest::gtest
                                            spdlog::spdlog_header_only)

foreach(item ${V_GLOB})
  get_filename_component(OUTPUT_FILE_NOEXT ${item} NAME_WLE)
  string(REGEX REPLACE "^test_" "" TEST_SHORT ${OUTPUT_FILE_NOEXT})
  set(SOURCES_${OUTPUT_FILE_NOEXT}_TEST ${item})
  add_executable(${OUTPUT_FILE_NOEXT} ${SOURCES_${OUTPUT_FILE_NOEXT}_TEST}
                                      $<TARGET_OBJECTS:test_main_obj>)
  target_link_libraries(${OUTPUT_FILE_NOEXT} PRIVATE ${LIBRARY_TEST_SET})
  file(MAKE_DIRECTORY
       ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT})
  # Przejecie wywolan systemowych w trzech testach. Opcja --wrap jest
  # rozszerzeniem GNU ld/lld; konsolidatory bez niej dostaja to samo przez
  # definicje symbolu o nazwie systemowej w pliku obiektowym testu (makro
  # RDB_WRAP_SYSCALL, test/UnitTest/syscallWrap.hpp) i wtedy nie ma tu nic do
  # dodania na linii konsolidacji.
  if(RDB_HAS_LD_WRAP)
    if(OUTPUT_FILE_NOEXT STREQUAL "test_faccposix"
       OR OUTPUT_FILE_NOEXT STREQUAL "test_faccposixshd")
      target_link_options(${OUTPUT_FILE_NOEXT} PRIVATE -Wl,--wrap=pread
                          -Wl,--wrap=write)
    endif()
    if(OUTPUT_FILE_NOEXT STREQUAL "test_dumpManager")
      target_link_options(${OUTPUT_FILE_NOEXT} PRIVATE -Wl,--wrap=lseek
                          -Wl,--wrap=write)
    endif()
  endif()
  # test_uxSysTermTools: symbole uxSysTermTools dostępne przez retractor →
  # common (PUBLIC) test_formatter i test_ipcClient: symbole dostępne przez
  # bibliotekę qry (w LIBRARY_TEST_SET)
  add_test(
    NAME ut_${TEST_SHORT}-setup
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND bash -c "rm -f *.meta *.desc")
  set_tests_properties(ut_${TEST_SHORT}-setup
                       PROPERTIES FIXTURES_SETUP ut_${TEST_SHORT}-fixture)

  add_test(
    NAME ut_${TEST_SHORT}
    WORKING_DIRECTORY
      ${CMAKE_BINARY_DIR}/test/UnitTest/Data/${OUTPUT_FILE_NOEXT}
    COMMAND ${RDB_UT_LAUNCHER}
            ${CMAKE_BINARY_DIR}/test/UnitTest/${OUTPUT_FILE_NOEXT})
  set_tests_properties(ut_${TEST_SHORT} PROPERTIES FIXTURES_REQUIRED
                                                   ut_${TEST_SHORT}-fixture)
  if(NOT PROG_VALGRIND)
    set_tests_properties(ut_${TEST_SHORT} PROPERTIES TIMEOUT ${RDB_UT_TIMEOUT})
  endif()
endforeach()

add_executable(test_xqry test_qry.cpp $<TARGET_OBJECTS:test_main_obj>)
target_link_libraries(test_xqry PRIVATE ${LIBRARY_TEST_SET})
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry)
add_test(
  NAME ut_xqry
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_qry
  COMMAND ${RDB_UT_LAUNCHER} ${CMAKE_BINARY_DIR}/test/UnitTest/test_xqry)
if(NOT PROG_VALGRIND)
  set_tests_properties(ut_xqry PROPERTIES TIMEOUT ${RDB_UT_TIMEOUT})
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel)
add_test(
  NAME ut_dataModel-compile
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND bash run_compile.sh ${CMAKE_BINARY_DIR})

add_test(
  NAME ut_dataModel-compare
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test/UnitTest/Data/test_dataModel
  COMMAND bash run_compare.sh ${CMAKE_BINARY_DIR})

set_tests_properties(ut_dataModel-compare
                     PROPERTIES DEPENDS "ut_dataModel-compile;ut_dataModel")

# ut_ipcServer kasuje globalne nazwy IPC (segment, kolejka komend, muteks mapy),
# wiec nie moze biec obok testu, ktory ich uzywa. ut_ipcClient przy `ctest -j 8`
# rzucal boost::interprocess_exception::library_error (2026-09-14), a
# samodzielnie przechodzil; przyczyny nie badano.
#
# ut_bus i ut_lockManager zakladaja w /tmp porzucone blokady i segmenty, zeby
# sprawdzic sprzatacz. /tmp jest wspolny dla maszyny, a kazdy xretractor
# konczacy test integracyjny sprzata to samo przy wyjsciu - zabieral je pierwszy
# (nightly 2026-09-19: licznik sprzatacza 0 zamiast 1). Ponadto okno wlasciciela
# przejsciowego (50 ms, lockFile.cpp) nie wytrzymuje kwoty CPU kontenera CI:
# przy `ctest -j` kwota zamraza caly cgroup na dluzej niz 50 ms.
set_tests_properties(
  ut_rdb
  ut_xqry
  ut_dataModel
  ut_dataModel-compile
  ut_dataModel-compare
  ut_soperations
  ut_ipcServer
  ut_ipcClient
  ut_bus
  ut_lockManager
  PROPERTIES RUN_SERIAL TRUE)

# Takze z valgrindem musi byc ograniczenie dla regresji zakleszczenia.
set_tests_properties(ut_busFallback PROPERTIES TIMEOUT 60)
# test_busFallback dolacza bus.cpp jako zrodlo; GCC ostrzega wtedy o typach z
# przestrzeni anonimowej uzytych w polach Bus::Impl, choc w tej jednostce
# kompilacji sa jedyne.
target_compile_options(test_busFallback
                       PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-subobject-linkage>)
