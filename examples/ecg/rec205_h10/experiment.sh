#!/bin/bash
# Eksperyment: ogon startowy jako skladnik budzetu lacza (znalezisko B / H10).
#
# TEZA SPRAWDZANA
#   Iloraz bajtow surowe/wysylane dazy do 10, a roznica miedzy zmierzonym
#   ilorazem a 10 jest w calosci ubytkiem OGONA STARTOWEGO strumienia
#   wysylanego. Ogon jest funkcja planu, wiec ubytek musi byc TA SAMA LICZBA
#   rekordow niezaleznie od dlugosci przebiegu. Gdyby rosl z dlugoscia,
#   nie bylby ogonem, tylko wyciekiem.
#
# JAK JEST SPRAWDZANA
#   Ten sam plan uruchamiany przy trzech dlugosciach przebiegu. Bramka:
#     (a) ubytek ogona identyczny we wszystkich przebiegach,
#     (b) iloraz monotonicznie malejacy ku 10,
#     (c) odchylenie ilorazu od 10 skalujace sie jak 1/N.
#
#   Warunek (c) w postaci progu bezwzglednego (np. "iloraz < 10,05") bylby
#   ZALEZNY OD DLUGOSCI PRZEBIEGU: przy stalym ogonie odchylenie maleje jak
#   1/N, wiec kazdy prog bezwzgledny jest osiagalny dopiero powyzej pewnej
#   dlugosci i domyslny zestaw moglby go nie dosiegnac. Dlatego sprawdzamy
#   niezmiennik: iloczyn (iloraz - 10) x sekundy ma byc STALY. Dla ogona
#   t slotow zrodla dazy on do 10*t/360; przy t = 590 daje to 16,4.
#
# Uzycie:  ./experiment.sh [slotow ...]     (domyslnie 36000 72000 108000)
#
# Przebiegi sa tempowane do czasu rzeczywistego: domyslny zestaw trwa ~10 min.

set -o errexit
set -o nounset

here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"
cd "$here"

# ---- 1. aparatura: binarki -------------------------------------------------
for cand in "$root/build/Release/src" "$root/build/Debug/src"; do
  if [ -x "$cand/retractor/xretractor" ]; then
    XRETRACTOR="$cand/retractor/xretractor"; XTRDB="$cand/rdb/xtrdb"; break
  fi
done
XRETRACTOR="${XRETRACTOR:-$(command -v xretractor || true)}"
XTRDB="${XTRDB:-$(command -v xtrdb || true)}"
if [ ! -x "${XRETRACTOR:-}" ] || [ ! -x "${XTRDB:-}" ]; then
  echo "BLAD: brak xretractor/xtrdb." >&2
  echo "  zbuduj:  scripts/buildrdb.sh release      (z korzenia repozytorium)" >&2
  exit 1
fi

# ---- 2. aparatura: dane ----------------------------------------------------
missing=0
for f in ../rec205/rec205 ../rec205/bp_coef.txt ../rec205/d_coef.txt; do
  [ -r "$f" ] || { echo "BLAD: brak pliku wejsciowego $f" >&2; missing=1; }
done
if [ "$missing" -ne 0 ]; then
  echo "  wygeneruj:  examples/ecg/build.sh" >&2
  echo "  (pobiera rekord 205 MIT-BIH i przelicza go na format rdb)" >&2
  exit 1
fi

if [ "$#" -eq 0 ]; then
  slot_list=(36000 72000 108000)
else
  slot_list=("$@")
fi

rev="$("$XRETRACTOR" --help 2>&1 | grep -o 'Branch: [^,]*' | sed 's/Branch: //')"
echo "silnik      : $rev"
echo "data        : $(date -Is)"
echo "przebiegi   : ${slot_list[*]}"
echo

printf 'slots\tsec\traw_rec\traw_b\tship_rec\tship_b\tratio\ttail_deficit\n' > results.tsv

rec_of() { "$XTRDB" -s "$1" -n 2>/dev/null | sed -n 's/.*Records: *\([0-9][0-9]*\).*/\1/p' | head -1; }

for slots in "${slot_list[@]}"; do
  echo "--- przebieg $slots slotow ($(awk "BEGIN{printf \"%.0f\", $slots/360}") s sygnalu) ---"
  rm -f ecg_raw qrs_ship ./*.meta ./*.shadow
  "$XRETRACTOR" -q rec205-offload.rql -r -k -m "$slots" > "run-$slots.log" 2>&1
  rr="$(rec_of ecg_raw)"; sr="$(rec_of qrs_ship)"
  rb="$(stat -c %s ecg_raw)"; sb="$(stat -c %s qrs_ship)"
  awk -v s="$slots" -v rr="$rr" -v rb="$rb" -v sr="$sr" -v sb="$sb" \
      'BEGIN{printf "%d\t%.1f\t%d\t%d\t%d\t%d\t%.4f\t%d\n", s, rr/360.0, rr, rb, sr, sb, rb/sb, int(rr/10)-sr}' \
      >> results.tsv
  tail -1 results.tsv | awk -F'\t' '{printf "  raw %d rek / %d B | ship %d rek / %d B | iloraz %s | ogon %s\n",$3,$4,$5,$6,$7,$8}'
done

echo
column -t -s $'\t' results.tsv
echo

# ---- 3. bramka -------------------------------------------------------------
awk -F'\t' 'NR>1 {
    n++
    if (n==1) { d0=$8 } else if ($8 != d0) { same=1 }
    if (n>1 && $7 > prev_r + 1e-9) mono=1
    prev_r=$7; last_r=$7
    k = ($7 - 10) * $2
    if (n==1 || k < kmin) kmin=k
    if (n==1 || k > kmax) kmax=k
  }
  END {
    fail=0
    if (n < 2) { print "BRAMKA: potrzebne co najmniej dwa przebiegi"; exit 2 }
    if (same) { printf "FAIL (a): ubytek ogona nie jest staly\n"; fail=1 }
    else      { printf "PASS (a): ubytek ogona staly = %d rekordow we wszystkich %d przebiegach\n", d0, n }
    if (mono) { printf "FAIL (b): iloraz nie maleje monotonicznie ku 10\n"; fail=1 }
    else      { printf "PASS (b): iloraz maleje monotonicznie ku 10\n" }
    if (kmax/kmin > 1.05) {
      printf "FAIL (c): (iloraz-10) x sek nie jest stale: %.3f..%.3f (rozrzut %.2f%%)\n", kmin, kmax, 100*(kmax/kmin-1); fail=1
    } else {
      printf "PASS (c): (iloraz-10) x sek stale = %.3f..%.3f (rozrzut %.2f%%), czyli odchylenie ~ 1/N\n", kmin, kmax, 100*(kmax/kmin-1)
    }
    print ""
    if (fail) { print "WERDYKT: FAIL"; exit 1 } else { print "WERDYKT: PASS" }
  }' results.tsv
