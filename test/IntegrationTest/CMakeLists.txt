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
# naraz - nadaje instancji nazwe, przez co blokada i obiekty IPC dostaja sufiks,
# i dokleja sie do nazwy segmentu magistrali (bus::segmentName). Wlasny TMPDIR
# odsuwa plik blokady i logi.
#
# Pula, a nie jedna przestrzen na katalog, bo kazdy zywy serwer trzyma w
# /dev/shm okolo 5,1 MiB: segment magistrali swojej przestrzeni (~2 MiB, kasuje
# go ostatni proces, ktory go mapuje - bus.hpp), kolejke komend (~1 MiB) i
# segment odpowiedzi (~2 MiB). Szesnascie przestrzeni to wiec do ~83 MiB naraz
# niezaleznie od liczby katalogow, plus segmenty testow jednostkowych spoza
# puli. Zmierzone 2026-10-24 (#574): CircleCI (executor large) ma /dev/shm 48
# GiB i miesci to z zapasem; przy 64 MiB (domyslnym dockera) juz nie - szczyt 63
# MiB w pierwszych sekundach `ctest -j 20` i kilkadziesiat porazek, kazda jawna
# (odmowa startu "shared memory too small" albo "No space left on device").
# Szesnascie to nadal wiecej rownoczesnych serwerow, niz ctest uruchamia naraz
# (do 15 przy `ctest -j 36`, jak w CI).
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

# Testy integracyjne wolaja xretractor / xqry / xtrdb GOLA NAZWA, czyli z PATH-u
# - i celowo, bo sprawdzaja binarium ZAINSTALOWANE, a nie to z drzewa budowy
# (patrz test/CLAUDE.md). Katalog instalacji (${CMAKE_INSTALL_PREFIX}/bin,
# domyslnie ~/.local/bin) nie zawsze jest jednak w PATH - na macOS i w powloce
# nielogowanej zwykle go nie ma - i wtedy WSZYSTKIE te testy padaja na "command
# not found", bez zwiazku z tym, co badaja.
#
# Dlatego katalog instalacji jest DOPISYWANY NA KONIEC PATH-u w chwili
# uruchomienia testu (ENVIRONMENT_MODIFICATION, path_list_append w makrze
# ponizej). Pierwsza wersja (a0082a34) wpisywala do testu PATH zamrozony w
# chwili konfiguracji, z instalacja Z PRZODU, i przez to nadpisywala kazdy PATH
# ustawiony swiadomie przez uruchamiajacego - m.in. `buildrdb.sh coverage`,
# ktory kieruje testy na binaria z drzewa budowy, zeby zebrac z nich pokrycie.
# Teraz o wyborze binarki decyduje PATH uruchamiajacego, tak jak przed portem, a
# katalog instalacji jest tylko ostatnia pozycja, gdy tamten PATH nie zna
# zadnej.

# Opakowanie valgrindem dla tych testow integracyjnych, ktore sprawdzaja nie
# tylko wynik, ale i czystosc pamieci silnika. PUSTE tam, gdzie valgrinda nie ma
# (Apple silicon nie ma go i miec nie bedzie). Testy -vg-* (etykieta valgrind)
# nie biegna tam wtedy w `ninja test` ani w test-valgrind, ktorego bez valgrinda
# nie ma; wykonuje je tylko jawne `ctest` (pelne albo -L valgrind) - bez
# opakowania, czyli sprawdzaja samo zachowanie, a straz pamieci przechodzi na
# konfiguracje z -DRDB_SANITIZE=address,undefined. Lepsze to niz "command not
# found" w dziewieciu testach, ktore z pamiecia nie maja nic wspolnego.
if(PROG_VALGRIND)
  set(IT_VALGRIND
      ${PROG_VALGRIND} --vgdb=no --error-exitcode=1 --leak-check=full
      --suppressions=${CMAKE_CURRENT_SOURCE_DIR}/../valgrind.supp
      CACHE INTERNAL "opakowanie valgrindem dla testow integracyjnych")
else()
  set(IT_VALGRIND
      ""
      CACHE INTERNAL "opakowanie valgrindem dla testow integracyjnych")
endif()

# Podkladki .vgshim/{xretractor,xqry,xtrdb} dla wariantow -vg- calych
# scenariuszy (it_add_vg_test nizej). Valgrind sprawdza kod silnika, a nie
# testy: wariant dostaje ten scenariusz, ktory jako jedyny prowadzi przez kod
# niewidziany przez kopie ut_*-valgrind, i wtedy pod valgrindem ma biec KAZDY
# program projektu, ktory scenariusz uruchamia - takze xqry i xtrdb, ktore w
# starszych testach -vg- biegly obok valgrinda (pomiar 2026-11-28: 279 z 848
# wierszy "pokrytych" przez tamte testy szlo spoza valgrinda). Wywolanie z nazwy
# trafia na podkladke przez PATH, wywolanie sciezka z drzewa budowy wariant
# zapisuje jako ${IT_VG_SHIM}/<program>. Podkladka uruchamia binarium z drzewa
# budowy, nie zainstalowane. --child-silent-after-fork wycisza dzieci po fork
# (komenda `system` xtrdb, procesy pomocnicze uslugi): ich logi nie maja
# podsumowania, bo dziecko od razu wykonuje exec, i myliloby to vg_check.sh.
set(IT_VG_SHIM
    ${CMAKE_CURRENT_BINARY_DIR}/.vgshim
    CACHE INTERNAL "katalog podkladek valgrinda dla wariantow -vg-")
set(IT_VG_CHECK
    ${CMAKE_CURRENT_BINARY_DIR}/vg_check.sh
    CACHE INTERNAL "straznik logow valgrinda dla wariantow -vg-")
string(JOIN " " _it_vg_command ${IT_VALGRIND})
foreach(_it_vg_program xretractor xqry xtrdb)
  if(IT_VALGRIND)
    set(_it_vg_exec
        "exec ${_it_vg_command} --child-silent-after-fork=yes --log-file=\"\${RDB_VG_LOG_DIR:?}/${_it_vg_program}.%p.log\" $<TARGET_FILE:${_it_vg_program}> \"$@\""
    )
  else()
    set(_it_vg_exec "exec $<TARGET_FILE:${_it_vg_program}> \"$@\"")
  endif()
  file(
    GENERATE
    OUTPUT ${IT_VG_SHIM}/${_it_vg_program}
    CONTENT
      "#!/bin/bash\n# Wygenerowane przez test/IntegrationTest/CMakeLists.txt.\n${_it_vg_exec}\n"
      FILE_PERMISSIONS
      OWNER_READ
      OWNER_WRITE
      OWNER_EXECUTE
      GROUP_READ
      GROUP_EXECUTE
      WORLD_READ
      WORLD_EXECUTE)
endforeach()
unset(_it_vg_command)
unset(_it_vg_exec)

macro(add_test)
  _add_test(${ARGV})
  cmake_parse_arguments(_at "" "NAME" "" ${ARGV})
  if(_at_NAME)
    set(_it_test_name ${_at_NAME})
  else()
    set(_it_test_name ${ARGV0})
  endif()

  # Poza drzewem IntegrationTest makro ma byc przezroczyste - testy jednostkowe
  # i skryptowe rzadza sie wlasnymi zasadami i nie startuja silnika przez to
  # makro.
  string(FIND "${CMAKE_CURRENT_SOURCE_DIR}" "${IT_TREE_ROOT}" _it_in_tree)
  if(_it_in_tree EQUAL 0)
    # Konfiguracja operatora nie moze zmieniac wyniku testow. Scenariusze
    # konfiguracji moga ustawic wlasny XDG_CONFIG_HOME wewnatrz skryptu.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/.config")
    # Jawne proby pamieci maja osobny cel; nazwy -vg-* sa ich konwencja.
    if(_it_test_name MATCHES "-vg-")
      set_tests_properties(${_it_test_name} PROPERTIES LABELS valgrind)
    endif()
    if(IT_NO_NAMESPACE)
      # Katalog badajacy tozsamosc globalna maszyny: instancja bezimienna albo
      # nazwy nadawane przez sam scenariusz (alfa, beta, ...). Przestrzen nazw
      # zmienilaby tu przedmiot badania, wiec zamiast niej zostaje wylacznosc na
      # czas testu.
      #
      # Probowany byl slabszy srodek - wspolny RESOURCE_LOCK zamiast RUN_SERIAL,
      # bo z testem w przestrzeni nazw taki test nie dzieli zadnego zasobu. Nie
      # dal nic mierzalnego (94,65 s wobec 94,34 s: ogon i tak konczy sie po
      # najdluzszym tescie jednostkowym), a multiserver_routing mierzy czas
      # `--bus` z progiem 1 s, wiec obciazenie sasiadow moglo mu tylko
      # zaszkodzic. Zostaje mocniejsza gwarancja.
      set_tests_properties(
        ${_it_test_name}
        PROPERTIES
          RUN_SERIAL
          TRUE
          ENVIRONMENT_MODIFICATION
          "PATH=path_list_append:${CMAKE_INSTALL_PREFIX}/bin;XDG_CONFIG_HOME=set:${CMAKE_CURRENT_BINARY_DIR}/.config"
      )
    else()
      # Przydzial jest per KATALOG, nie per test. Makro jest wolane wielokrotnie
      # w jednym CMakeLists, a testy jednego katalogu i tak nie moga biec obok
      # siebie: dziela katalog roboczy i podkatalog magazynu (`temp/`), wiec
      # pisza po tych samych artefaktach. RESOURCE_LOCK na nazwie przestrzeni
      # zalatwia oba wykluczenia naraz - wewnatrz katalogu i miedzy katalogami,
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
        PROPERTIES
          RESOURCE_LOCK
          ${_it_namespace}
          FIXTURES_REQUIRED
          it_namespace_pool
          ENVIRONMENT
          "RDB_NAMESPACE=${_it_namespace};TMPDIR=${_it_tmpdir}"
          ENVIRONMENT_MODIFICATION
          "PATH=path_list_append:${CMAKE_INSTALL_PREFIX}/bin;XDG_CONFIG_HOME=set:${CMAKE_CURRENT_BINARY_DIR}/.config"
      )
    endif()
  endif()
  unset(_it_test_name)
endmacro()

# it_add_vg_test(NAME <nazwa z -vg-> WORKING_DIRECTORY <katalog> COMMAND ...)
# rejestruje wariant -vg- scenariusza: PATH zaczyna sie od podkladek, a komende
# obejmuje vg_check.sh, ktory po przebiegu ocenia logi valgrinda. Bez valgrinda
# (Darwin) komenda biegnie bez straznika, a podkladki uruchamiaja programy bez
# opakowania. Makro, nie funkcja: add_test przydziela przestrzen nazw w zakresie
# katalogu (_it_namespace), a funkcja zgubilaby ten przydzial i rozdzielila
# testy jednego katalogu na rozne RESOURCE_LOCK.
macro(it_add_vg_test)
  cmake_parse_arguments(_vg "" "NAME;WORKING_DIRECTORY" "COMMAND" ${ARGV})
  if(IT_VALGRIND)
    add_test(
      NAME ${_vg_NAME}
      WORKING_DIRECTORY ${_vg_WORKING_DIRECTORY}
      COMMAND bash ${IT_VG_CHECK} ${CMAKE_CURRENT_BINARY_DIR}/vglog-${_vg_NAME}
              ${_vg_COMMAND})
  else()
    add_test(
      NAME ${_vg_NAME}
      WORKING_DIRECTORY ${_vg_WORKING_DIRECTORY}
      COMMAND ${_vg_COMMAND})
  endif()
  set_property(
    TEST ${_vg_NAME}
    APPEND
    PROPERTY ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${IT_VG_SHIM}")
endmacro()

# Warstwy uzytkownika odcina XDG_CONFIG_HOME z makra, warstwy systemowej nie da
# sie odciac - jej sciezki sa stale (appConfig.cpp). Istniejacy plik zmienia
# wynik testow, a uszkodzony zatrzymuje kazdy test uruchamiajacy programy
# (#426).
set(_it_system_configs /etc/retractor/retractor.toml)
if(RDB_OS_DARWIN)
  list(APPEND _it_system_configs /usr/local/etc/retractor/retractor.toml
       /opt/homebrew/etc/retractor/retractor.toml)
endif()
foreach(_it_system_config ${_it_system_configs})
  if(EXISTS "${_it_system_config}")
    message(
      WARNING
        "Integration tests will load the system configuration layer ${_it_system_config}"
    )
  endif()
endforeach()
unset(_it_system_configs)

file(
  GLOB V_GLOB
  LIST_DIRECTORIES true
  "*")
foreach(item ${V_GLOB})
  if(IS_DIRECTORY ${item})
    add_subdirectory(${item})
  endif()
endforeach()

# Zwrot pamieci dzielonej zajetej przez pule po zwyklych testach integracyjnych.
# Ten test pozostaje w normalnym zestawie: po przebiegu serwerow segmenty
# magistrali zajmowalyby pamiec /dev/shm takze po zakonczeniu CTest. CTest
# dolacza go automatycznie rowniez do filtrowanych testow uzywajacych oprawy.
# Rejestracja przez _add_test, bo makro powyzej doklada FIXTURES_REQUIRED --
# test sprzatajacy wlasna oprawe czekalby sam na siebie.
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
