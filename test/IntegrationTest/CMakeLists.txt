# Przestrzenie nazw dla rownoczesnych testow integracyjnych.
#
# Uruchomienie xretractora zajmuje cztery zasoby globalne DLA MASZYNY, a nie dla
# katalogu roboczego: plik blokady instancji, obiekty IPC w /dev/shm, plik logu
# w katalogu tymczasowym i slot na magistrali xrdbbus. Ostatni z nich decyduje,
# bo magistrala wymusza rozlacznosc nazw strumieni miedzy WSZYSTKIMI zywymi
# instancjami, a nazwy powtarzaja sie w testach masowo (`core0` w czternastu
# katalogach, `src` w trzynastu, `dst` w jedenastu). Osobne katalogi robocze nie
# pomagaja: drugi rownoczesny start odpada z ClaimStatus::Conflict.
#
# RDB_NAMESPACE (servername::environmentNamespace) rozdziela wszystkie cztery
# naraz — nadaje instancji nazwe, przez co blokada i obiekty IPC dostaja sufiks,
# i dokleja sie do nazwy segmentu magistrali (bus::segmentName). Wlasny TMPDIR
# odsuwa plik blokady i logi.
#
# Pula, a nie jedna przestrzen na katalog. Segment magistrali ma 874 KB i z
# zalozenia nikt go nie kasuje (bus.hpp), a /dev/shm w kontenerze CI ma
# domyslnie 64 MB: piecdziesiat kilka segmentow zmiescilo by sie na styk, a
# przepelnienie /dev/shm NIE wywraca testu — daje ClaimStatus::Unavailable,
# ktory jest fail-open, czyli po cichu zdejmuje izolacje i wraca do kolizji
# nazw. Szesnascie przestrzeni to najwyzej 14 MB niezaleznie od liczby katalogow
# i nadal wiecej rownoczesnych serwerow, niz `ctest -j $(nproc)` uruchomi na
# typowej maszynie.
#
# Stale ida do pamieci podrecznej, a nie do zmiennych katalogu: makro jest
# globalne od chwili zdefiniowania, wiec wolaja je takze katalogi dodane pozniej
# (UnitTest, ScriptTest, samo test/CMakeLists.txt), gdzie zmienne tego katalogu
# juz nie istnieja.
set(IT_NAMESPACE_POOL_SIZE
    16
    CACHE INTERNAL "liczba przestrzeni nazw testow integracyjnych")
set(IT_NAMESPACE_ROOT
    ${CMAKE_CURRENT_BINARY_DIR}/.namespaces
    CACHE INTERNAL "katalog tymczasowy przestrzeni nazw")
set(IT_TREE_ROOT
    ${CMAKE_CURRENT_SOURCE_DIR}
    CACHE INTERNAL "korzen drzewa testow integracyjnych")
set_property(GLOBAL PROPERTY IT_NAMESPACE_NEXT 0)

macro(add_test)
  _add_test(${ARGV})
  cmake_parse_arguments(_at "" "NAME" "" ${ARGV})
  if(_at_NAME)
    set(_it_test_name ${_at_NAME})
  else()
    set(_it_test_name ${ARGV0})
  endif()

  # Poza drzewem IntegrationTest makro ma byc przezroczyste — testy jednostkowe
  # i skryptowe rzadza sie wlasnymi zasadami i nie startuja silnika przez to
  # makro.
  string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${IT_TREE_ROOT}" _it_in_tree)
  if(_it_in_tree EQUAL 0)
    if(IT_NO_NAMESPACE)
      # Katalog badajacy tozsamosc globalna maszyny: instancja bezimienna albo
      # nazwy nadawane przez sam scenariusz (alfa, beta, ...). Przestrzen nazw
      # zmienilaby tu przedmiot badania, wiec zamiast niej zostaje wylacznosc na
      # czas testu.
      #
      # Probowany byl slabszy srodek — wspolny RESOURCE_LOCK zamiast RUN_SERIAL,
      # bo z testem w przestrzeni nazw taki test nie dzieli zadnego zasobu. Nie
      # dal nic mierzalnego (94,65 s wobec 94,34 s: ogon i tak konczy sie po
      # najdluzszym tescie jednostkowym), a multiserver_routing mierzy czas
      # `--bus` z progiem 1 s, wiec obciazenie sasiadow moglo mu tylko
      # zaszkodzic. Zostaje mocniejsza gwarancja.
      set_tests_properties(${_it_test_name} PROPERTIES RUN_SERIAL TRUE)
    else()
      # Przydzial jest per KATALOG, nie per test. Makro jest wolane wielokrotnie
      # w jednym CMakeLists, a testy jednego katalogu i tak nie moga biec obok
      # siebie: dziela katalog roboczy i podkatalog magazynu (`temp/`), wiec
      # pisza po tych samych artefaktach. RESOURCE_LOCK na nazwie przestrzeni
      # zalatwia oba wykluczenia naraz — wewnatrz katalogu i miedzy katalogami,
      # ktore trafily na ten sam slot puli. Makro nie tworzy zakresu, wiec
      # _it_namespace zostaje ustawione dla calego CMakeLists tego katalogu.
      if(NOT DEFINED _it_namespace)
        get_property(_it_slot GLOBAL PROPERTY IT_NAMESPACE_NEXT)
        math(EXPR _it_next "(${_it_slot} + 1) % ${IT_NAMESPACE_POOL_SIZE}")
        set_property(GLOBAL PROPERTY IT_NAMESPACE_NEXT ${_it_next})
        if(_it_slot LESS 10)
          set(_it_namespace "it0${_it_slot}")
        else()
          set(_it_namespace "it${_it_slot}")
        endif()
        set(_it_tmpdir ${IT_NAMESPACE_ROOT}/${_it_namespace})
        file(MAKE_DIRECTORY ${_it_tmpdir})
      endif()
      # FIXTURES_REQUIRED wiaze test z jednym zadaniem sprzatajacym: CTest
      # uruchamia test oznaczony FIXTURES_CLEANUP dopiero po WSZYSTKICH testach
      # wymagajacych tej oprawy, wiec kasowanie segmentow puli nie ma jak trafic
      # w biegnaca instancje. Oprawa nie ma czesci SETUP -- pule zaklada sam
      # start pierwszego serwera.
      set_tests_properties(
        ${_it_test_name}
        PROPERTIES RESOURCE_LOCK ${_it_namespace} FIXTURES_REQUIRED
                   it_namespace_pool ENVIRONMENT
                   "RDB_NAMESPACE=${_it_namespace};TMPDIR=${_it_tmpdir}")
    endif()
  endif()
  unset(_it_test_name)
endmacro()

file(
  GLOB V_GLOB
  LIST_DIRECTORIES true
  "*")
foreach(item ${V_GLOB})
  if(IS_DIRECTORY ${item})
    add_subdirectory(${item})
  endif()
endforeach()

# Zwrot pamieci dzielonej zajetej przez pule. Rejestracja przez _add_test, bo
# makro powyzej doklada FIXTURES_REQUIRED -- test sprzatajacy wlasna oprawe
# czekalby sam na siebie.
_add_test(
  NAME
  it_namespace_pool-cleanup
  WORKING_DIRECTORY
  ${CMAKE_CURRENT_BINARY_DIR}
  COMMAND
  bash
  shm_cleanup.sh
  ${IT_NAMESPACE_POOL_SIZE})
set_tests_properties(it_namespace_pool-cleanup PROPERTIES FIXTURES_CLEANUP
                                                          it_namespace_pool)
