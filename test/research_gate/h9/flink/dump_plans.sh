#!/bin/bash
# Krok D: dla kazdego z szesciu jobow (3 rodziny x 2 warianty) zapisz plan LOGICZNY
# i FIZYCZNY oraz policz instancje operatorow badanego podplanu.
#
# `--plan-only` buduje graf transformacji i NIE wola env.execute() — zaden job nie jest
# uruchamiany, wiec ten krok nie moze wygenerowac zadnego wyniku kosztowego. Predeklaracja
# jest niezamrozona (STOP-5), wiec pomiar kosztowy jest zabroniony.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
JAVA_HOME_PINNED="${JAVA_HOME_PINNED:-/usr/lib/jvm/java-17-openjdk-amd64}"
FLINK_HOME="${FLINK_HOME:-/home/michal/opt/flink-2.3.0}"
Q="${Q:-8}"

CP="$HERE/build:$(find "$FLINK_HOME/lib" -maxdepth 1 -name '*.jar' | sort | paste -sd:)"

# Oba zestawienia sa dopisywane przez PlanDump, wiec oba musza zniknac przed przebiegiem —
# inaczej kolejne uruchomienie skryptu dokleja wiersze do poprzednich.
rm -f "$HERE/results/flink_instances.tsv" "$HERE/results/flink_work.tsv"
rm -rf "$HERE/plans"
mkdir -p "$HERE/plans"

for job in F9R2Job F9R1Job F9XJob; do
  for variant in natural manual; do
    "$JAVA_HOME_PINNED/bin/java" -cp "$CP" "$job" \
      --variant "$variant" --q "$Q" --plan-only --out-dir "$HERE"
  done
done

echo "---"
column -t -s $'\t' "$HERE/results/flink_instances.tsv"
