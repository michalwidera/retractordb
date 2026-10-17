#!/bin/bash
# Pozycja 7 z paper-arXiv/debs/done/requested.md: wykonanie bylo taktowane zegarem sciennym,
# bez trybu "policz najszybciej jak sie da". Rodzina wolnej telemetrii musiala byc
# skracana do kilkudziesieciu rekordow, zeby dala sie w ogole weryfikowac w petli.
#
# Test dowodzi dwoch rzeczy naraz:
#  1. --no-clock nie zmienia WYNIKU  - artefakt jest bajtowo identyczny,
#  2. --no-clock zmienia CZAS        - przebieg schodzi ponizej polowy taktowanego.
#
# Punkt 2 jest tu potrzebny, bo sam warunek rownosci przeszedlby rowniez wtedy,
# gdyby przelacznik byl ignorowany.
set -e
. "$(dirname "$0")/../portable.sh"

run_plan() {
  rm -rf temp
  mkdir -p temp
  local start end
  start=$(now_ns)
  xretractor query.rql -k -r -m 8 "$@"
  end=$(now_ns)
  echo $(((end - start) / 1000000))  # milisekundy
}

clocked_ms=$(run_plan)
cp temp/dst clocked.bin

noclock_ms=$(run_plan -f)
cp temp/dst noclock.bin

echo "clocked=${clocked_ms} ms  no-clock=${noclock_ms} ms"

# 1. Ten sam wynik.
cmp clocked.bin noclock.bin

# 2. Realnie szybciej. Takt 1/2 s przez 8 slotow to okolo 3,5 s czekania;
#    prog polowy jest luzny wzgledem tej roznicy, wiec nie jest krucha
#    na wolnej maszynie CI, a nadal wykrywa zignorowany przelacznik.
if [ "$noclock_ms" -ge $((clocked_ms / 2)) ]; then
  echo "FAIL: --no-clock nie skrocilo przebiegu (${noclock_ms} ms vs ${clocked_ms} ms)"
  exit 1
fi

# 3. Sprzecznosc argumentow jest bledem, nie cichym pierwszenstwem jednej z opcji.
if xretractor query.rql -k -r -m 1 -f -t 2>/dev/null; then
  echo "FAIL: -f razem z -t powinno byc odrzucone"
  exit 1
fi

# 4. Budzet -m (#521, etap 9b): -m N to N obrotow petli liczac krok zerowy, czyli N-1 slotow.
#    Plan ma jeden interwal, wiec kazdy slot liczy dst raz (rekord INTEGER = 4 B), a krok zerowy
#    czyta tylko deklaracje. -m 1 nie liczy zadnego slotu, -m 4 liczy trzy.
for budget in 1 4; do
  rm -rf temp
  mkdir -p temp
  xretractor query.rql -k -r -f -m "$budget"
  want=$(((budget - 1) * 4))
  got=$(file_size temp/dst)
  if [ "$got" -ne "$want" ]; then
    echo "FAIL: -m $budget dal $got B w dst, oczekiwano $want B ($((budget - 1)) slotow)"
    exit 1
  fi
done
