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

for f in temp/str1_*_dump*.tmp; do
  size=$(wc -c < "$f")
  printf '%s %d rekord(ow)\n' "${f##*/}" "$((size / 4))"
  # -v jest konieczne: bez niego od skraca powtorzenia do gwiazdki i wlasnie ciag zer,
  # o ktory tu chodzi, przestaje byc widoczny.
  od -An -td4 -w4 -v "$f" | tr -d ' ' | paste -sd' ' -
done
