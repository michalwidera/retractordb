#!/usr/bin/env python3
"""Analiza Fazy 0 sledztwa ~40 ms: zdarzenia ogonowe w wake_lag.

Wejscie: dowolna liczba zrodel w formie rola=plik.csv. Dwa formaty
rozpoznawane po naglowku:
  - sonda silnika (RDB_BENCH_CSV): iter,compute_ns,wake_lag_ns,e2e_ns
  - shadow_pacer.py:               iter,wake_lag_ns (+ metadane '#')

Dla kazdego zrodla: percentyle wake_lag, lista zdarzen >= progu wraz z
czasem wzglednym slotu (iter/rate), statystyka odstepow miedzy zdarzeniami.
Dla pary silnik+cien: zgrubna koincydencja czasowa -- kotwica silnika znana
tylko z widelek [launch, exit - N*period] (przekazanych przez --engine-launch/
--engine-exit, CLOCK_MONOTONIC ns), wiec tolerancja dopasowania jest jawnie
raportowana; rozstrzyganie koincydencji co do ms nalezy do Fazy 2 (ftrace).

Werdykt (kryteria z JOURNAL.md, plan Fazy 0):
  - cien widzi zdarzenia w liczbie porownywalnej z silnikiem -> platforma
    (system-wide), H1;
  - cien czysty przy silniku ze zdarzeniami -> zdarzenia zwiazane z rdzeniem 3
    lub aktywnoscia silnika, H2 (z zastrzezeniem: stall per-rdzen tez mozliwy).

Wyjscie: markdown na stdout (wklejany do results.md badania).
"""

import argparse
import sys


def read_source(path):
    """Zwraca (meta: dict, lags: list[int], rate_hz: float|None)."""
    meta, lags = {}, []
    rate = None
    with open(path) as f:
        header = None
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                for tok in line[1:].split():
                    if "=" in tok:
                        k, v = tok.split("=", 1)
                        meta[k] = v
                rate = float(meta.get("rate_hz", rate or 0)) or rate
                continue
            if header is None:
                header = line.split(",")
                try:
                    lag_idx = header.index("wake_lag_ns")
                except ValueError:
                    sys.exit(f"BLAD: {path}: brak kolumny wake_lag_ns w naglowku {header}")
                continue
            cols = line.split(",")
            lags.append(int(cols[lag_idx]))
    return meta, lags, rate


def pctile(sorted_vals, p):
    if not sorted_vals:
        return 0
    i = min(len(sorted_vals) - 1, int(p / 100.0 * len(sorted_vals)))
    return sorted_vals[i]


def fmt_ms(ns):
    return f"{ns / 1e6:.3f}"


def main() -> int:
    ap = argparse.ArgumentParser(description="analiza zdarzen ogonowych wake_lag")
    ap.add_argument("sources", nargs="+", metavar="rola=plik.csv")
    ap.add_argument("--threshold-ms", type=float, default=5.0)
    ap.add_argument("--rate", type=float, default=360.0,
                    help="rate silnika [Hz] (dla zrodel bez rate_hz w metadanych)")
    ap.add_argument("--engine-launch-mono-ns", type=int, default=None)
    ap.add_argument("--engine-exit-mono-ns", type=int, default=None)
    args = ap.parse_args()
    thr_ns = int(args.threshold_ms * 1e6)

    data = {}  # rola -> dict
    for spec in args.sources:
        role, _, path = spec.partition("=")
        if not path:
            sys.exit(f"BLAD: zrodlo '{spec}' nie jest w formie rola=plik")
        meta, lags, rate = read_source(path)
        rate = rate or args.rate
        period_ns = round(1e9 / rate)
        events = [(i, lag) for i, lag in enumerate(lags) if lag >= thr_ns]
        data[role] = dict(meta=meta, lags=lags, rate=rate, period_ns=period_ns,
                          events=events, path=path)

    print(f"### Analiza wake_lag (prog zdarzenia: {args.threshold_ms:.1f} ms)\n")
    print("| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |")
    print("|---|---|---|---|---|---|---|")
    for role, d in data.items():
        s = sorted(d["lags"])
        n = len(s)
        print(f"| {role} | {n} | {pctile(s, 50) / 1e3:.1f} | {pctile(s, 99) / 1e3:.1f} "
              f"| {fmt_ms(pctile(s, 99.9))} | {fmt_ms(s[-1] if s else 0)} | {len(d['events'])} |")
    print()

    for role, d in data.items():
        ev = d["events"]
        if not ev:
            print(f"**{role}**: zero zdarzen >= progu.\n")
            continue
        print(f"**{role}**: {len(ev)} zdarzen (slot, t_wzgl [s], wake_lag [ms]):\n")
        print("```")
        for i, lag in ev[:60]:
            print(f"  slot {i:>8}  t={i / d['rate']:>9.3f}s  {fmt_ms(lag)} ms")
        if len(ev) > 60:
            print(f"  ... ({len(ev) - 60} dalszych)")
        print("```")
        if len(ev) >= 2:
            gaps = [(ev[k + 1][0] - ev[k][0]) / d["rate"] for k in range(len(ev) - 1)]
            gs = sorted(gaps)
            print(f"\nOdstepy miedzy zdarzeniami [s]: min={gs[0]:.2f} "
                  f"mediana={gs[len(gs) // 2]:.2f} max={gs[-1]:.2f}\n")

    # --- Zgrubna koincydencja silnik <-> cien -------------------------------
    eng = next((d for r, d in data.items() if r.startswith("engine")), None)
    shd = next((d for r, d in data.items() if r.startswith("shadow")), None)
    if eng and shd and args.engine_launch_mono_ns and args.engine_exit_mono_ns \
            and "anchor_mono_ns" in shd["meta"]:
        n_eng = len(eng["lags"])
        anchor_min = args.engine_launch_mono_ns
        anchor_max = args.engine_exit_mono_ns - n_eng * eng["period_ns"]
        if anchor_max < anchor_min:
            print("UWAGA: widelki kotwicy silnika puste (exit-N*period < launch) -- "
                  "pomijam koincydencje.")
        else:
            anchor_mid = (anchor_min + anchor_max) // 2
            tol_ns = (anchor_max - anchor_min) // 2 + int(0.1e9)
            shd_anchor = int(shd["meta"]["anchor_mono_ns"])
            shd_times = [shd_anchor + (i + 1) * shd["period_ns"] + lag
                         for i, lag in shd["events"]]
            matched = 0
            for i, lag in eng["events"]:
                t_eng = anchor_mid + (i + 1) * eng["period_ns"] + lag
                if any(abs(t_eng - t) <= tol_ns for t in shd_times):
                    matched += 1
            print(f"\n### Koincydencja silnik <-> cien (zgrubna)\n")
            print(f"- widelki kotwicy silnika: {(anchor_max - anchor_min) / 1e9:.2f} s "
                  f"=> tolerancja dopasowania +/-{tol_ns / 1e9:.2f} s (JAWNIE zgrubna; "
                  f"rozstrzyganie co do ms = Faza 2, ftrace)")
            print(f"- zdarzen silnika: {len(eng['events'])}, z dopasowaniem u cienia: {matched}")

    # --- Werdykt ------------------------------------------------------------
    print("\n### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)\n")
    if eng and shd:
        ne, ns_ = len(eng["events"]), len(shd["events"])
        if ne > 0 and ns_ == 0:
            print("- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> "
                  "hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / "
                  "aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony "
                  "do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.")
        elif ne > 0 and ns_ > 0:
            print(f"- zdarzenia po obu stronach (silnik {ne}, cien {ns_}) -> "
                  "wskazanie na zrodlo system-wide (H1); koincydencje doprecyzowac "
                  "w Fazie 2.")
        elif ne == 0:
            print("- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc "
                  "przebieg lub powtorzyc.")
    else:
        solo = next((d for r, d in data.items() if r.startswith("pacer")), None)
        if solo is not None:
            if solo["events"]:
                print(f"- pacer-solo na rdzeniu silnika widzi {len(solo['events'])} "
                      "zdarzen BEZ silnika -> platforma generuje je sama (H1 "
                      "wzmocniona).")
            else:
                print("- pacer-solo czysty -> platforma sama z siebie nie generuje "
                      "zdarzen >= progu na rdzeniu 3 (spojne z czystym baselinem "
                      "NumPy); ciezar przesuwa sie na H2.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
