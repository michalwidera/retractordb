#!/usr/bin/env bash
# Strona Flinka bramek P6 — PIERWSZE uruchomienie jobow z `env.execute()`.
#
# Do tej pory strona Flinka biegala wylacznie z `--plan-only`, czyli konczyla sie
# PRZED `env.execute()`. Krok "strona Flinka" produkowal plany, nie wyniki. Tutaj
# joby wykonuja sie naprawde, bo bramka poprawnosci §7.1 porownuje WARTOSCI, a nie
# plany — i bo pulapka tego luku brzmi: compile-only (u Flinka: plan-only) nie jest
# dowodem wykonywalnosci.
#
# Liczba rekordow jest ZAMROZONA: `--slots 3000` (§4 predeklaracji; joby same biora
# polowe dla zrodel `1/50`). To nie jest pomiar kosztowy — czasu nikt nie mierzy,
# powtorzen nie ma, rate'u sie nie dobiera.
#
# Wynik: $OUT/<rodzina>_<wariant>_q<Q>/{*.csv,job.out,job.err}
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAVA_HOME_PINNED="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
FLINK_HOME="${FLINK_HOME:-/home/michal/opt/flink-2.3.0}"
SLOTS="${SLOTS:-3000}"
OUT="${OUT:-$HOME/k26v3_gates_flink}"
DATA="${DATA:-$HERE/data/main}"

CP="$HERE/flink/build:$(find "$FLINK_HOME/lib" -maxdepth 1 -name '*.jar' | sort | paste -sd:)"

# Domyslne `java` na tym hoscie to 25.x — JDK 17 jest przypiety SCIEZKA, nigdy
# przez PATH, i bramka sprawdza sciezke oraz sume, nie `java -version`.
[ -x "$JAVA_HOME_PINNED/bin/java" ] || { echo "BLAD: brak przypietego JDK 17" >&2; exit 2; }

rm -rf "$OUT"
mkdir -p "$OUT"
runs=0

for spec in "F9_R2:F9R2Job:--a $DATA/axis_x.txt --b $DATA/axis_y.txt" \
            "F9_R1:F9R1Job:--a $DATA/vib.txt --b $DATA/cur.txt" \
            "F9_X:F9XJob:--a $DATA/front_vib.txt --b $DATA/front_cur.txt --c $DATA/rear_vib.txt --d $DATA/rear_cur.txt"; do
  family="${spec%%:*}"
  rest="${spec#*:}"
  job="${rest%%:*}"
  data_args="${rest#*:}"
  for q in 1 2 4 8 16 32; do
    for variant in natural manual; do
      dir="$OUT/${family}_${variant}_q${q}"
      mkdir -p "$dir"
      # shellcheck disable=SC2086
      "$JAVA_HOME_PINNED/bin/java" -cp "$CP" "$job" \
        --variant "$variant" --q "$q" --slots "$SLOTS" $data_args \
        --out-dir "$dir" --sink-dir "$dir" >"$dir/job.out" 2>"$dir/job.err"
      grep -q '^LOGICAL ' "$dir/job.out" || {
        echo "BLAD: $family/$variant/Q=$q — brak wiersza LOGICAL, job sie nie wykonal" >&2
        tail -5 "$dir/job.err" >&2; exit 3; }
      grep -q '^WORK ' "$dir/job.out" || {
        echo "BLAD: $family/$variant/Q=$q — brak wiersza WORK, eksport mechanizmu bylby niepelny" >&2
        tail -5 "$dir/job.err" >&2; exit 3; }
      printf '  ok  %-22s %s\n' "${family}_${variant}_q${q}" "$(grep -m1 '^LOGICAL ' "$dir/job.out")"
      runs=$((runs + 1))
    done
  done
done

echo
echo "OK: $runs przebiegow Flinka z env.execute(), po $SLOTS slotow"
