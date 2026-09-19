#!/bin/bash
# Scenariusz trybu usługowego/idle xretractor. Argument $1 to ścieżka do binarki
# xretractor (przekazywana z CMake jako $<TARGET_FILE:xretractor>), argument $2:
#   (brak) -> tryb flagi --service
#   env    -> tryb zmiennej środowiskowej XRETRACTOR_SERVICE=1
#   empty  -> plik zapytań istnieje, ale nie niesie ani jednej instrukcji
# Logikę trzymamy w skrypcie (nie w 'bash -c' w add_test), bo średniki w CMake
# są separatorem listy i rozbiłyby polecenie na osobne argumenty.
set -e

# Binarka z katalogu budowy, a nie 'xretractor' z PATH: asercje niżej dotyczą
# markerów INFO, których Release nie kompiluje, a w PATH bywa kopia z innego
# profilu niż ten właśnie sprawdzany.
xretractor="${1:?podaj sciezke do binarki xretractor}"
mode="$2"

# Nie czekamy tu na cudzą blokadę: xretractor biegnie poniżej pierwszoplanowo
# (-m 5), a za to, żeby poprzedni test nie zostawił po sobie instancji, odpowiada
# bramka higieny w ../serverlib.sh - obarcza winowajcę, nie następną ofiarę.

# Ciche raportowanie w Release (SPDLOG_ACTIVE_LEVEL=ERROR - zwiazane z
# efektywnoscia, patrz glowny CMakeLists.txt) wycina komunikaty INFO. Marker
# INFO trybu uslugowego wtedy LEGALNIE nie powstaje. Test podaza za ta decyzja:
# w Release (RDB_INFO_ACTIVE=0 z CMakeLists) sprawdzamy tylko czysty start/stop
# trybu idle (set -e), a asercje tresci/prefiksu INFO pomijamy. Domyslnie 1
# (build z INFO), gdy uruchamiany bez zmiennej.
info_active="${RDB_INFO_ACTIVE:-1}"

if [ "$mode" = "empty" ]; then
  # Plik zapytań bez ani jednej instrukcji = tryb bezczynny, a nie błąd. Jednostka systemd
  # wskazuje ExecStart-em stały plik (RETRACTOR_QUERY_FILE), a ten przy pierwszym starcie
  # systemu jest pusty - i tak jest udokumentowana w samej jednostce. Do 2026-09-05 taki
  # start kończył się kodem błędu ("Parse result:Empty file."), więc Restart=on-failure
  # zapętlał usługę zamiast pozostawić ją czekającą na plan.
  printf '# zestaw bez instrukcji\n\n' > empty-plan.rql
  "$xretractor" --service --noanykey -m 5 empty-plan.rql 2>stderr.txt
  if [ "$info_active" = "1" ]; then
    grep -q 'holds no statements' stderr.txt
    grep -q 'Idle mode: no queries to process' stderr.txt
  fi
elif [ "$mode" = "env" ]; then
  # Logowanie usługowe włączone zmienną środowiskową, bez flagi --service.
  XRETRACTOR_SERVICE=1 "$xretractor" --noanykey -m 5 2>stderr.txt
else
  # Logowanie usługowe włączone flagą --service; start bez pliku .rql = tryb idle.
  "$xretractor" --service --noanykey -m 5 2>stderr.txt
  if [ "$info_active" = "1" ]; then
    grep -q 'No query file provided' stderr.txt
    grep -q 'Idle mode: no queries to process' stderr.txt
  fi
fi

# Format logu usługowego: 'poziom + treść' na stderr, bez znacznika czasu. Pod systemd
# linia niesie dodatkowo prefiks priorytetu sd-daemon '<N>' (INFO => '<6>'), bo czyta go
# journald i zamienia na wagę komunikatu; poza systemd tego prefiksu NIE MA i mieć nie
# powinien - launchd przepisuje stderr usługi do pliku dosłownie, więc byłby to śmieć na
# początku każdej linii (patrz setupLoggerMain). Który wariant obowiązuje, mówi CMake
# przez RDB_SD_PREFIX - to ta sama wartość, z której kompilował się silnik.
# Linia INFO jest obecna tylko gdy INFO jest wkompilowane (poza Release).
if [ "$info_active" = "1" ]; then
  if [ "${RDB_SD_PREFIX:-1}" = "1" ]; then
    grep -qE '^<6>\[I\] ' stderr.txt
  else
    grep -qE '^\[I\] ' stderr.txt
    grep -qv '^<' stderr.txt
  fi
fi
