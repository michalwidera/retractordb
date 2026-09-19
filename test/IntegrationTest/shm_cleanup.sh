#!/bin/bash
# Sprzatanie segmentow magistrali nalezacych do puli przestrzeni nazw testow integracyjnych.
#
# Segmentu magistrali z zalozenia nie kasuje nikt (bus.hpp): skasowanie go w chwili, gdy inna
# instancja trzyma odwzorowanie, zerwaloby jej magistrale. Dla przestrzeni PULI ta ostroznosc
# nie ma zastosowania, bo po przebiegu suity nie zyje juz zadna instancja, ktora ich uzywala,
# a kazdy segment zajmuje okolo 1,7 MiB REALNIE (jest zerowany przy tworzeniu, wiec strony sa
# przydzielone). Szesnascie przestrzeni to okolo 26,5 MiB, ktore po suicie zostawaly na stale --
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
. "$(dirname "$0")/portable.sh"

pool=${1:-16}
index=0
while [ "$index" -lt "$pool" ]; do
  suffix=$(printf 'it%02d' "$index")
  shm_remove "^xrdbbus_v[0-9]+_${suffix}\$" || true
  index=$((index + 1))
done

# shm_list konczy sie kodem 2, gdy katalogu obiektow IPC nie da sie ustalic. Kontrola
# jest wtedy jawnie POMINIETA - cicho zdana bramka higieny bylaby gorsza niz jej brak.
shm_status=0
leftovers=$(shm_list '^xrdbbus_v[0-9]+_it[0-9]{2}$') || shm_status=$?
if [ "$shm_status" -ne 0 ]; then
  echo "POMINIETO: higiena IPC niesprawdzalna na tej platformie"
elif [ -n "$leftovers" ]; then
  echo "shm_cleanup: segmenty puli przetrwaly kasowanie:"
  echo "$leftovers"
  exit 1
fi
