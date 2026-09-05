#!/bin/bash
# Scenariusz trybu usługowego/idle xretractor. Argument $1:
#   (brak) -> tryb flagi --service
#   env    -> tryb zmiennej środowiskowej XRETRACTOR_SERVICE=1
#   empty  -> plik zapytań istnieje, ale nie niesie ani jednej instrukcji
# Logikę trzymamy w skrypcie (nie w 'bash -c' w add_test), bo średniki w CMake
# są separatorem listy i rozbiłyby polecenie na osobne argumenty.
set -e

# Nie czekamy tu na cudzą blokadę: xretractor biegnie poniżej pierwszoplanowo
# (-m 5), a za to, żeby poprzedni test nie zostawił po sobie instancji, odpowiada
# bramka higieny w ../serverlib.sh — obarcza winowajcę, nie następną ofiarę.

# Ciche raportowanie w Release (SPDLOG_ACTIVE_LEVEL=ERROR — zwiazane z
# efektywnoscia, patrz glowny CMakeLists.txt) wycina komunikaty INFO. Marker
# INFO trybu uslugowego wtedy LEGALNIE nie powstaje. Test podaza za ta decyzja:
# w Release (RDB_INFO_ACTIVE=0 z CMakeLists) sprawdzamy tylko czysty start/stop
# trybu idle (set -e), a asercje tresci/prefiksu INFO pomijamy. Domyslnie 1
# (build z INFO), gdy uruchamiany bez zmiennej.
info_active="${RDB_INFO_ACTIVE:-1}"

if [ "$1" = "empty" ]; then
  # Plik zapytań bez ani jednej instrukcji = tryb bezczynny, a nie błąd. Jednostka systemd
  # wskazuje ExecStart-em stały plik (RETRACTOR_QUERY_FILE), a ten przy pierwszym starcie
  # systemu jest pusty — i tak jest udokumentowana w samej jednostce. Do 2026-09-05 taki
  # start kończył się kodem błędu ("Parse result:Empty file."), więc Restart=on-failure
  # zapętlał usługę zamiast pozostawić ją czekającą na plan.
  printf '# zestaw bez instrukcji\n\n' > empty-plan.rql
  xretractor --service --noanykey -m 5 empty-plan.rql 2>stderr.txt
  if [ "$info_active" = "1" ]; then
    grep -q 'holds no statements' stderr.txt
    grep -q 'Idle mode: no queries to process' stderr.txt
  fi
elif [ "$1" = "env" ]; then
  # Logowanie usługowe włączone zmienną środowiskową, bez flagi --service.
  XRETRACTOR_SERVICE=1 xretractor --noanykey -m 5 2>stderr.txt
else
  # Logowanie usługowe włączone flagą --service; start bez pliku .rql = tryb idle.
  xretractor --service --noanykey -m 5 2>stderr.txt
  if [ "$info_active" = "1" ]; then
    grep -q 'No query file provided' stderr.txt
    grep -q 'Idle mode: no queries to process' stderr.txt
  fi
fi

# Format logu usługowego: prefiks priorytetu sd-daemon '<N>' + 'poziom + treść' na stderr,
# bez znacznika czasu. Linia INFO ma priorytet 6 (LOG_INFO) => '<6>[I] ' — obecna
# tylko gdy INFO jest wkompilowane (poza Release).
if [ "$info_active" = "1" ]; then
  grep -qE '^<6>\[I\] ' stderr.txt
fi
