#!/usr/bin/env bash
#
# Zawartosc plikow zrzutu DO DUMP, rozlozona na wartosci INTEGER (4 B na rekord).
#
# Do 2026-09-23 zrzutow nie ogladalo NIC. pattern-run.txt porownuje strumien str1 (term.script
# robi `list 9`), a pliki temp/str1_*_dump*.tmp nie wchodzily do zadnego wzorca. To jest powod,
# dla ktorego defekt "DO DUMP siegajacy glebiej niz zgromadzona historia zapisuje rekordy zerowe
# jako historie, ktorej nigdy nie bylo" mogl lezec w drzewie niezauwazony.
#
# UWAGA: wzorzec pattern-dump.txt przypina stan FAKTYCZNY, nie pozadany. Wiodace rekordy `0`
# w str1_testrule2_dump*.tmp sa wlasnie ta sfabrykowana historia. Zostaja w nim swiadomie, zeby
# ich znikniecie bylo ruchem wzorca, a nie cicha zmiana pliku, ktorego nikt nie czyta.
set -e
export LC_ALL=C
. "$(dirname "$0")/../portable.sh"

for f in temp/str1_*_dump*.tmp; do
  printf '%s %d rekord(ow)\n' "${f##*/}" "$(record_count "$f" 4)"
  # read_binary_values, a nie `od -An -td4 -w4 -v`: `-w` jest rozszerzeniem GNU i BSD `od`
  # (macOS) konczy sie na nim bledem uzycia, wiec linie wartosci znikaly z wyniku. Odczyt
  # wypisuje KAZDY rekord - takze ciag zer, ktorego `od` bez `-v` skracal do gwiazdki, a o
  # ktory tu wlasnie chodzi.
  read_binary_values "$f" d4
done
