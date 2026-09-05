#!/bin/sh
# Sprzatanie segmentow magistrali nalezacych do puli przestrzeni nazw testow integracyjnych.
#
# Segmentu magistrali z zalozenia nie kasuje nikt (bus.hpp): skasowanie go w chwili, gdy inna
# instancja trzyma odwzorowanie, zerwaloby jej magistrale. Dla przestrzeni PULI ta ostroznosc
# nie ma zastosowania, bo po przebiegu suity nie zyje juz zadna instancja, ktora ich uzywala,
# a kazdy segment zajmuje 854 KiB REALNIE (jest zerowany przy tworzeniu, wiec strony sa
# przydzielone). Szesnascie przestrzeni to 13,3 MiB, ktore po suicie zostawaly na stale --
# w kontenerze CI z domyslnym /dev/shm 64 MiB to jedna piata calego zasobu.
#
# Uruchamiany jako test CTest z FIXTURES_CLEANUP, czyli PO wszystkich testach, ktore
# przestrzeni uzywaja. Kasuje wylacznie nazwy z puli (it00..itNN) i dowolna wersje ukladu
# w nazwie segmentu, zeby bump kSegmentName nie zostawial poprzedniej wersji.
#
# Uwaga: rownolegly DRUGI przebieg ctest na tej samej maszynie uzywa tych samych nazw puli
# i straci swoje segmenty. Ograniczenie jest to samo, ktore ma RESOURCE_LOCK -- pula jest
# zasobem maszyny, a nie katalogu roboczego.
set -e

pool=${1:-16}
index=0
while [ "$index" -lt "$pool" ]; do
  suffix=$(printf 'it%02d' "$index")
  rm -f /dev/shm/xrdbbus_v*_"$suffix"
  index=$((index + 1))
done

leftovers=$(ls /dev/shm/ 2>/dev/null | grep -E '^xrdbbus_v[0-9]+_it[0-9]{2}$' || true)
if [ -n "$leftovers" ]; then
  echo "shm_cleanup: segmenty puli przetrwaly kasowanie:"
  echo "$leftovers"
  exit 1
fi
