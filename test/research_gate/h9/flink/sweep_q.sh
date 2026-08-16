#!/bin/bash
# Krzywa flinkowa po siatce Q = {1,2,4,8,16,32} (§10) — liczba instancji badanego podplanu
# i jednostki bajtowe dla oba wariantow, przy zamrozonej regule alokacji
# F(Q) = min(F_max, floor(Q/2)).
#
# Powod, dla ktorego ta krzywa jest osobnym artefaktem: po stronie RetractorDB redukcja nasyca
# sie na 1 - 1/F i NIE rosnie powyzej Q = 4, a po stronie Flinka rosnie jak 1 - 1/Q.
# Skrypt werdyktu musi przewidywac DWIE rozne krzywe (SZKIC_RODZIN.md §3.3 pkt 3).
#
# Plany per-Q sa produktem ubocznym i nie sa zachowywane — zachowany material planistyczny to
# komorka rozstrzygajaca Q = 8 w plans/. Zaden job nie jest uruchamiany (--plan-only).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAVA_HOME_PINNED="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
FLINK_HOME="${FLINK_HOME:-/home/michal/opt/flink-2.3.0}"

CP="$HERE/build:$(find "$FLINK_HOME/lib" -maxdepth 1 -name '*.jar' | sort | paste -sd:)"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

for q in 1 2 4 8 16 32; do
  for job in F9R2Job F9R1Job F9XJob; do
    for variant in natural manual; do
      "$JAVA_HOME_PINNED/bin/java" -cp "$CP" "$job" \
        --variant "$variant" --q "$q" --plan-only --out-dir "$WORK" >/dev/null
    done
  done
done

mkdir -p "$HERE/results"
cp "$WORK/results/flink_instances.tsv" "$HERE/results/flink_q_curve.tsv"
cp "$WORK/results/flink_work.tsv" "$HERE/results/flink_work_q_curve.tsv"
column -t -s $'\t' "$HERE/results/flink_q_curve.tsv"
echo "---"
column -t -s $'\t' "$HERE/results/flink_work_q_curve.tsv"
