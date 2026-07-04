#!/usr/bin/env python3
# =============================================================================
# e1_stats.py — analiza pomiaru E1 (budżet czasowy) i E2E (latencja end-to-end)
# =============================================================================
#
# CO ROBI
#   Czyta plik CSV wyprodukowany przez sondę benchmarku wbudowaną w
#   xretractor (src/retractor/lib/executorsm.cpp, blok #ifdef __linux__
#   bramkowany zmienną środowiskową RDB_BENCH_CSV). Obsługuje dwa formaty:
#
#   stary (tylko E1):          nowy (E1 + E2E):
#       iter,compute_ns            iter,compute_ns,wake_lag_ns,e2e_ns
#       0,18342                    0,18342,41210,60113
#       ...                        ...
#
#   compute_ns  — czas jednego wywołania proc.processRows() (rdzeń obliczeń
#                 jednego interwału, E1); pacing/sleep pętli NIE jest wliczony.
#   wake_lag_ns — spóźnienie pobudki pętli względem deadline'u interwału
#                 (jitter planisty OS).
#   e2e_ns      — latencja end-to-end: od deadline'u interwału (nominalny
#                 moment pojawienia się krotki wejściowej w modelu czasowym)
#                 do końca boradcast() (emisja wyniku do kolejek IPC).
#                 W przybliżeniu: e2e ≈ wake_lag + compute + emisja.
#
#   Skrypt liczy medianę, p99 i maksimum czasu na interwał oraz odnosi je do
#   budżetu czasu rzeczywistego (domyślnie 1/360 s = 2,78 ms, częstotliwość
#   próbkowania rekordu MIT-BIH 205). Podaje też przepustowość "unpaced"
#   (N interwałów / suma czasów obliczeń). Dla nowego formatu dodatkowo
#   raportuje statystyki E2E i wake_lag (z p99,9 — ogon pochodzi z planisty).
#
# JAK URUCHOMIĆ POMIAR (z build/Release)
#   # (a) tryb zwykły — uwaga: sleep względny, dryf kumuluje się w e2e;
#   #     kolumny E2E miarodajne tylko dla trybu (b):
#   RDB_BENCH_CSV=/tmp/e1_normal.csv \
#       ./xretractor <ścieżka>/rec205-detect.rql -k -m 650000
#   # (b) tryb real-time (SCHED_FIFO, absolutne budzenie; wymaga uprawnień):
#   sudo RDB_BENCH_CSV=/tmp/e1_rt.csv \
#       taskset -c 3 ./xretractor <ścieżka>/rec205-detect.rql -k -m 650000 -t
#   # Pełny tor emisji: równolegle podpiąć jedno xqry nasłuchujące na
#   # strumieniu wynikowym (bez klienta koszt boradcast() ≈ 0).
#
# JAK URUCHOMIĆ ANALIZĘ
#   python3 e1_stats.py /tmp/e1_rt.csv
#   python3 e1_stats.py /tmp/e1_rt.csv --fs 360        # inna częstotliwość
#
# WYNIK
#   Linie statystyk gotowe do wklejenia w tabele E1/E2E manuskryptu (kolumny:
#   mediana / p99 [/ p99,9] / max / % budżetu / przepustowość).
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
    ap = argparse.ArgumentParser(
        description="Analiza pomiaru E1 (budżet czasowy) i E2E (latencja end-to-end).")
    ap.add_argument("csv", help="plik CSV z sondy (RDB_BENCH_CSV)")
    ap.add_argument("--fs", type=float, default=360.0,
                    help="częstotliwość próbkowania w Hz (budżet = 1/fs); domyślnie 360")
    args = ap.parse_args()

    ns = []        # compute_ns (E1)
    wake_ns = []   # wake_lag_ns (tylko nowy format)
    e2e_ns = []    # e2e_ns (tylko nowy format)
    with open(args.csv, encoding="utf-8") as f:
        next(f)  # pomiń nagłówek
        for line in f:
            line = line.strip()
            if not line:
                continue
            cols = line.split(",")
            ns.append(int(cols[1]))
            if len(cols) >= 4:  # nowy format: iter,compute_ns,wake_lag_ns,e2e_ns
                wake_ns.append(int(cols[2]))
                e2e_ns.append(int(cols[3]))

    if not ns:
        raise SystemExit("Pusty zbiór pomiarów — sprawdź, czy RDB_BENCH_CSV był ustawiony.")

    budget_us = 1e6 / args.fs                     # budżet na interwał w mikrosekundach

    us = sorted(x / 1000.0 for x in ns)          # nanosekundy -> mikrosekundy
    total_s = sum(ns) / 1e9                       # łączny czas obliczeń w sekundach
    max_us = max(us)

    print(f"plik           : {args.csv}")
    print(f"interwałów (N) : {len(us)}")
    print("--- E1: rdzeń obliczeń (processRows) ---")
    print(f"mediana        : {st.median(us):.1f} us")
    print(f"p99            : {percentile(us, 0.99):.1f} us")
    print(f"max            : {max_us:.1f} us")
    print(f"budżet (1/{args.fs:g}s): {budget_us:.1f} us")
    print(f"max / budżet   : {max_us / budget_us * 100:.1f} %   "
          f"({'MIEŚCI SIĘ' if max_us < budget_us else 'PRZEKROCZONY'})")
    print(f"przepustowość  : {len(us) / total_s:,.0f} próbek/s (unpaced)")

    if not e2e_ns:
        return  # stary format CSV — tylko raport E1

    e2e_us = sorted(x / 1000.0 for x in e2e_ns)
    wake_us = sorted(x / 1000.0 for x in wake_ns)
    e2e_max = max(e2e_us)
    print("--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---")
    print(f"mediana        : {st.median(e2e_us):.1f} us")
    print(f"p99            : {percentile(e2e_us, 0.99):.1f} us")
    print(f"p99,9          : {percentile(e2e_us, 0.999):.1f} us")
    print(f"max            : {e2e_max:.1f} us")
    print(f"max / budżet   : {e2e_max / budget_us * 100:.1f} %   "
          f"({'MIEŚCI SIĘ' if e2e_max < budget_us else 'PRZEKROCZONY'})")
    print("--- jitter pobudki (wake_lag) ---")
    print(f"mediana        : {st.median(wake_us):.1f} us")
    print(f"p99            : {percentile(wake_us, 0.99):.1f} us")
    print(f"p99,9          : {percentile(wake_us, 0.999):.1f} us")
    print(f"max            : {max(wake_us):.1f} us")


if __name__ == "__main__":
    main()
