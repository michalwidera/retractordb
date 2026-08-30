#!/bin/bash
# Pozycja 7 z usecases/requested.md: wykonanie bylo taktowane zegarem sciennym,
# bez trybu "policz najszybciej jak sie da". Rodzina wolnej telemetrii musiala byc
# skracana do kilkudziesieciu rekordow, zeby dala sie w ogole weryfikowac w petli.
#
# Test dowodzi dwoch rzeczy naraz:
#  1. --no-clock nie zmienia WYNIKU  — artefakt jest bajtowo identyczny,
#  2. --no-clock zmienia CZAS        — przebieg schodzi ponizej polowy taktowanego.
#
# Punkt 2 jest tu potrzebny, bo sam warunek rownosci przeszedlby rowniez wtedy,
# gdyby przelacznik byl ignorowany.
set -e

run_plan() {
  rm -rf temp
  mkdir -p temp
  local start end
  start=$(date +%s%N)
  xretractor query.rql -k -r -m 8 "$@"
  end=$(date +%s%N)
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
