#!/bin/bash
# Pomiar odciazenia lacza dla potoku EKG z redukcja tempa (znalezisko B / H10).
#
# Porownuje w JEDNYM przebiegu dwa zmaterializowane strumienie:
#   ecg_raw   2 x INTEGER @ 1/360 — to, co lacze musialoby przeniesc surowo
#   qrs_ship  2 x INTEGER @ 1/36  — to, co idzie w lacze po potoku
#
# Uzycie:  ./measure.sh [slotow]      (domyslnie 180000 = 500 s sygnalu)
#
# Przebieg jest tempowany do czasu rzeczywistego, wiec 180000 slotow trwa ~8 min.

set -o errexit
set -o nounset

slots="${1:-180000}"
here="$(cd "$(dirname "$0")" && pwd)"
root="$(cd "$here/../../.." && pwd)"

# Binarki: najpierw drzewo Release, potem Debug, na koncu PATH.
for cand in "$root/build/Release/src" "$root/build/Debug/src"; do
  if [ -x "$cand/retractor/xretractor" ]; then
    XRETRACTOR="$cand/retractor/xretractor"
    XTRDB="$cand/rdb/xtrdb"
    break
  fi
done
XRETRACTOR="${XRETRACTOR:-$(command -v xretractor)}"
XTRDB="${XTRDB:-$(command -v xtrdb)}"

if [ ! -x "$XRETRACTOR" ] || [ ! -x "$XTRDB" ]; then
  echo "BLAD: nie znaleziono xretractor/xtrdb (zbuduj: scripts/buildrdb.sh release)" >&2
  exit 1
fi

echo "silnik: $("$XRETRACTOR" --help 2>&1 | grep -o 'Branch: [^,]*')"
echo "slotow zrodla: $slots  (${slots}/360 = $(awk "BEGIN{printf \"%.1f\", $slots/360}") s sygnalu)"
echo

cd "$here"
rm -f ecg_raw qrs_ship ./*.meta ./*.shadow
"$XRETRACTOR" -q rec205-offload.rql -r -k -m "$slots"

# Liczby rekordow i szerokosc rekordu czytamy z xtrdb, nie z rozmiaru pliku
# dzielonego przez zalozona szerokosc.
stat_of() {
  "$XTRDB" -s "$1" -n 2>/dev/null \
    | sed -n 's/.*Records: *\([0-9][0-9]*\).*/\1/p' | head -1
}
raw_rec="$(stat_of ecg_raw)"
shp_rec="$(stat_of qrs_ship)"
raw_b="$(stat -c %s ecg_raw)"
shp_b="$(stat -c %s qrs_ship)"

awk -v rr="$raw_rec" -v sr="$shp_rec" -v rb="$raw_b" -v sb="$shp_b" -v n="$slots" 'BEGIN {
  sec = rr / 360.0
  printf "%-10s %10s %14s %12s\n", "strumien", "rekordow", "bajtow", "B/s"
  printf "%-10s %10d %14d %12.1f\n", "ecg_raw",  rr, rb, rb/sec
  printf "%-10s %10d %14d %12.1f\n", "qrs_ship", sr, sb, sb/sec
  printf "\niloraz zmierzony      : %.4f\n", rb/sb
  printf "iloraz w stanie ustal.: 10.0000  (8 B x 360/s wobec 8 B x 36/s)\n"
  printf "redukcja zmierzona    : %.2f%%\n", 100*(1-sb/rb)
  printf "\nubytek ogona startowego: %d rekordow qrs_ship\n", int(rr/10) - sr
  printf "  (stala planu, nie funkcja dlugosci przebiegu - patrz README)\n"
}'
