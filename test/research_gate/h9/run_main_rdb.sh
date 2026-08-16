#!/usr/bin/env bash
# Strona RetractorDB bramek P6 — przebiegi na DANYCH GLOWNYCH, cztery profile.
#
# Co ten skrypt robi i czego NIE robi
# -----------------------------------
# Wykonuje kazdy z 21 zamrozonych planow w kazdym z czterech profili na danych
# glownych (3000/1500 rekordow) i zbiera dwie rzeczy: PUBLICZNE ARTEFAKTY (wejscie
# oracle'a §7.1) oraz liczniki LOGICAL i WORK. Nie mierzy czasu, nie ma powtorzen,
# nie ma blokow, nie dobiera rate'u — to nie jest pomiar kosztowy i nie wolno z
# niego czytac kosztu. Macierz kosztowa to P8, po kalibracji P7.
#
# Asercja komorki jest TA SAMA co w pilocie runtime (`pilot/run_pilot_runtime.sh`):
# kod wyjscia 0, wiersz LOGICAL, NIEZEROWY mianownik, wiersz WORK. Compile-only nie
# jest dowodem wykonywalnosci — iteracja 1 padla dokladnie na tym.
#
# Wynik: $OUT/<profil>/<plan>/{temp/,cell.out,cell.counters,cell.rc}
set -euo pipefail

cd "$(dirname "$0")"
HERE="$(pwd)"
CODE_REPO="${CODE_REPO:-/home/michal/github/retractordb}"
OUT="${OUT:-$HOME/k26v3_gates_rdb}"
# Proba generalna procedury decyzyjnej (§7.5) biegnie na danych pilota.
# Domyslne wartosci sa DOKLADNIE te, ktorych uzywa P6 — knoby niczego w nim
# nie zmieniaja i nie moga byc uzyte w kampanii pomiarowej.
DATA="${DATA:-$HERE/data/main}"
SLOTS_DIVISOR="${SLOTS_DIVISOR:-1}"

# Ile iteracji petli planu, zeby ZRODLO SZYBKIEGO TAKTU oddalo swoje 3000 rekordow
# (§4 predeklaracji zamraza LICZBE REKORDOW ZRODLA, nie wartosc `-m`; po stronie
# Flinka ta sama liczba wchodzi jako `--slots 3000`).
#
# `-m` jest limitem iteracji petli, a nie licznikiem rekordow zrodla, wiec przelicznik
# zalezy od taktu planu i musi byc rozny per rodzina:
#   F9-R2  oba zrodla 1/100, brak przeplotu   -> -m 3000, monitor daje 2999
#   F9-R1  1/100 i 1/50 przeplecione na 1/150 -> -m 6000, n_h = 3000+1500 = 4500
#   F9-X   jak F9-R1 dla obu par              -> -m 6000
# Zmierzone, nie zalozone: przy -m 6000 przeplot F9-R1 ma 4500 rekordow, czyli
# dokladnie 3000 + 1500 — obydwa zrodla skonsumowane w calosci.
slots_for() {
  case "$1" in
    F9_R2*) echo $((3000 / SLOTS_DIVISOR)) ;;
    F9_R1*|F9_X*) echo $((6000 / SLOTS_DIVISOR)) ;;
    *) echo "BLAD: nieznana rodzina $1" >&2; exit 2 ;;
  esac
}

profiles=(DEFAULT NO_R2_CANON NO_R1_FACTOR NO_R1_NO_R2)
families=(F9_R2 F9_R1 F9_X)
qs=(1 2 4 8 16 32)

# Plany rodzin materializuja substrat; plany kontrolne z zalozenia moga nie
# materializowac niczego i to jest ich OCZEKIWANY wynik (§7.2).
plans=()
for family in "${families[@]}"; do
  for q in "${qs[@]}"; do plans+=("${family}_Q${q}"); done
done
controls=(F9_R2_controls F9_R1_controls F9_X_controls)

# Wyciekly `xretractor -x` wywrocil kiedys 22 kolejne testy, a kazdy przechodzil
# w izolacji. Sprawdzamy raz, na wejsciu.
if pgrep -af '[x]retractor' >/dev/null; then
  echo "BLAD: w systemie biegnie xretractor — najpierw sprzatnij" >&2
  pgrep -af '[x]retractor' >&2
  exit 2
fi

rm -rf "$OUT"
mkdir -p "$OUT"
cells=0

assert_cell() {  # katalog_komorki rc -> 0 gdy przebieg jest dowodem wykonania
  local dir="$1" rc="$2" counters="$dir/cell.counters" logical work pub
  if [ "$rc" != "0" ]; then
    echo "BLAD: kod wyjscia $rc — plan sie NIE WYKONAL" >&2; return 1
  fi
  logical="$(grep -m1 '^LOGICAL ' "$counters" || true)"
  work="$(grep -m1 '^WORK ' "$counters" || true)"
  [ -n "$logical" ] || { echo "BLAD: brak wiersza LOGICAL" >&2; return 1; }
  [ -n "$work" ] || { echo "BLAD: brak wiersza WORK" >&2; return 1; }
  pub="$(sed -n 's/.*publiczne: dopisania=\([0-9]*\).*/\1/p' <<<"$logical")"
  [ "${pub:-0}" -gt 0 ] || { echo "BLAD: mianownik pusty" >&2; return 1; }
  return 0
}

for profile in "${profiles[@]}"; do
  binary="$CODE_REPO/build/K26v3-$profile/src/retractor/xretractor"
  [ -x "$binary" ] || { echo "BLAD: brak binarki profilu $profile" >&2; exit 2; }
  echo "== profil $profile =="
  for plan in "${plans[@]}" "${controls[@]}"; do
    dir="$OUT/$profile/$plan"
    slots="$(slots_for "$plan")"
    mkdir -p "$dir/temp"
    echo "$slots" >"$dir/cell.slots"
    ( cd "$dir"
      cp "$HERE/rql/$plan.rql" .
      cp "$DATA"/*.txt .
      set +e
      RDB_BENCH_LOGICAL=1 RDB_BENCH_WORK=1 timeout 1800 \
        "$binary" "$plan.rql" -m "$slots" -r -k >cell.out 2>cell.counters
      echo $? >cell.rc
      set -e
      rm -f ./*.txt "$plan.rql"
    )
    if ! assert_cell "$dir" "$(cat "$dir/cell.rc")"; then
      echo "BLAD: $profile/$plan" >&2
      sed -n '1,5p' "$dir/cell.counters" >&2
      exit 3
    fi
    printf '  ok  %-18s %s\n' "$plan" "$(grep -m1 '^LOGICAL ' "$dir/cell.counters")"
    cells=$((cells + 1))
  done
done

echo
echo "OK: $cells komorek na danych $(basename "$DATA"), kazda z licznikami LOGICAL i WORK"
