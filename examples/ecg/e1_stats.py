#!/usr/bin/env python3
# =============================================================================
# e1_stats.py — analiza pomiaru E1 (budżet czasowy / przepustowość)
# =============================================================================
#
# CO ROBI
#   Czyta plik CSV wyprodukowany przez sondę benchmarku E1 wbudowaną w
#   xretractor (src/retractor/lib/executorsm.cpp, blok #ifdef __linux__
#   bramkowany zmienną środowiskową RDB_BENCH_CSV). CSV ma postać:
#
#       iter,compute_ns
#       0,18342
#       1,17995
#       ...
#
#   gdzie compute_ns to czas wykonania jednego wywołania proc.processRows()
#   (rdzeń obliczeń jednego interwału), w nanosekundach. Pacing/sleep pętli
#   NIE jest wliczony — mierzymy czysty czas obliczeń.
#
#   Skrypt liczy medianę, p99 i maksimum czasu na interwał oraz odnosi je do
#   budżetu czasu rzeczywistego (domyślnie 1/360 s = 2,78 ms, częstotliwość
#   próbkowania rekordu MIT-BIH 205). Podaje też przepustowość "unpaced"
#   (N interwałów / suma czasów obliczeń) — ile próbek/s system przerobiłby
#   bez oczekiwania na zegar.
#
# JAK URUCHOMIĆ POMIAR (z build/Release)
#   # (a) tryb zwykły:
#   RDB_BENCH_CSV=/tmp/e1_normal.csv \
#       ./xretractor <ścieżka>/rec205-detect.rql -k -m 650000
#   # (b) tryb real-time (SCHED_FIFO, wymaga uprawnień):
#   sudo RDB_BENCH_CSV=/tmp/e1_rt.csv \
#       taskset -c 3 ./xretractor <ścieżka>/rec205-detect.rql -k -m 650000 -t
#
# JAK URUCHOMIĆ ANALIZĘ
#   python3 e1_stats.py /tmp/e1_normal.csv
#   python3 e1_stats.py /tmp/e1_rt.csv --fs 360        # inna częstotliwość
#
# WYNIK
#   Linia statystyk gotowa do wklejenia w tabelę E1 manuskryptu (kolumny:
#   mediana / p99 / max / % budżetu / przepustowość).
# =============================================================================

import argparse
import statistics as st


def percentile(sorted_vals, p):
    """Percentyl metodą najbliższego rangu (bez interpolacji)."""
    if not sorted_vals:
        return float("nan")
    idx = min(len(sorted_vals) - 1, int(p * len(sorted_vals)))
    return sorted_vals[idx]


def main():
    ap = argparse.ArgumentParser(description="Analiza pomiaru E1 (budżet czasowy).")
    ap.add_argument("csv", help="plik CSV z sondy (RDB_BENCH_CSV)")
    ap.add_argument("--fs", type=float, default=360.0,
                    help="częstotliwość próbkowania w Hz (budżet = 1/fs); domyślnie 360")
    args = ap.parse_args()

    ns = []
    with open(args.csv, encoding="utf-8") as f:
        next(f)  # pomiń nagłówek "iter,compute_ns"
        for line in f:
            line = line.strip()
            if line:
                ns.append(int(line.split(",")[1]))

    if not ns:
        raise SystemExit("Pusty zbiór pomiarów — sprawdź, czy RDB_BENCH_CSV był ustawiony.")

    us = sorted(x / 1000.0 for x in ns)          # nanosekundy -> mikrosekundy
    budget_us = 1e6 / args.fs                     # budżet na interwał w mikrosekundach
    total_s = sum(ns) / 1e9                       # łączny czas obliczeń w sekundach
    max_us = max(us)

    print(f"plik           : {args.csv}")
    print(f"interwałów (N) : {len(us)}")
    print(f"mediana        : {st.median(us):.1f} us")
    print(f"p99            : {percentile(us, 0.99):.1f} us")
    print(f"max            : {max_us:.1f} us")
    print(f"budżet (1/{args.fs:g}s): {budget_us:.1f} us")
    print(f"max / budżet   : {max_us / budget_us * 100:.1f} %   "
          f"({'MIEŚCI SIĘ' if max_us < budget_us else 'PRZEKROCZONY'})")
    print(f"przepustowość  : {len(us) / total_s:,.0f} próbek/s (unpaced)")


if __name__ == "__main__":
    main()
