#!/bin/bash
# Regresja: przebieg z zadeklarowanym budzetem slotow (-m N) musi dac ten sam wynik
# niezaleznie od tego, czy na stdin siedzi terminal z bajtem w buforze.
#
# Bez tej odpornosci `_kbhit()` bral taki bajt za klawisz operatora i konczyl petle PRZED
# pierwszym slotem. Awaria byla niema: proces wychodzil kodem 0, deskryptor juz istnial,
# a plik danych zostawal pusty -- diagnoza "0 Record(s)" zamiast komunikatu o bledzie.
# Tak padl it_agse_array na CI 2026-09-04 i ponownie kilka dni pozniej; lokalnie nie
# odtwarzalo sie nigdy, bo bez terminala `_kbhit()` wychodzi natychmiast.
#
# Test jest samokalibrujacy: porownuje przebieg pod terminalem z przebiegiem bez terminala,
# wiec nie zamraza liczby rekordow. Warunek "wiecej niz zero" pilnuje, zeby zgodnosc nie
# wynikla z tego, ze OBA przebiegi nic nie policzyly.

set -e

buildDir="${1:?podaj katalog buildu}"
xretractor="${buildDir}/src/retractor/xretractor"
readonly kSlots=12

run_clean() {
  rm -f ./*.desc ./*.meta ./*.shadow keyproof
}

size_of() {
  [ -f keyproof ] && stat -c%s keyproof || echo 0
}

# Przebieg odniesienia: stdin nie jest terminalem, wiec klawiatura nie wchodzi w gre.
run_clean
"$xretractor" query.rql -m "$kSlots" -f > verbose-notty.txt 2>&1 < /dev/null
reference=$(size_of)

# Ten sam przebieg, ale stdin to terminal z czekajacym bajtem.
run_clean
python3 pty_run.py "$xretractor" query.rql -m "$kSlots" -f
underTty=$(size_of)

if [ "$reference" -eq 0 ]; then
  echo "FAIL: przebieg odniesienia nie zapisal nic - test nie ma czego pilnowac"
  exit 1
fi

if [ "$underTty" -ne "$reference" ]; then
  echo "FAIL: bajt na terminalu skrocil przebieg: ${underTty} bajtow zamiast ${reference}"
  exit 1
fi

echo "OK (${reference} bajtow w obu przebiegach)"
