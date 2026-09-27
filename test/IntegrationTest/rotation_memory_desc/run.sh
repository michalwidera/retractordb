#!/bin/bash
# Start planu z :ROTATION na katalogu, w ktorym zostal .desc strumienia STORAGE MEMORY zapisany przed
# 108a5e94 - bez RETMEMORY i bez TYPE. Po przebiegu .desc ma nosic konfiguracje z PLANU.
#
# Magazyn bierze TYPE i RETMEMORY z wczytanego .desc, nie z planu (porownanie z planem obejmuje tylko
# sloty danych), a do 2026-09-27 dropStalePlanArtifacts przy :ROTATION nie kasowal niczego. Stary .desc
# zostawal wiec na miejscu i memoryFile dostawal pierscien bez granicy - rekord na kazdy takt, az do
# wyczerpania pamieci. To, co jest w .desc po przebiegu, jest dokladnie tym, z czego magazyn sie
# skonfigurowal: plik wczytany albo zapisany na nowo z planu.
set -e
rm -rf temp rotation_counter.txt && mkdir -p temp
cp stale.desc temp/m.desc
xretractor query.rql -m 3 -f </dev/null >server.txt 2>&1 || {
  cat server.txt
  exit 1
}
bash ../compare.sh --ignore-eol pattern.txt temp/m.desc
