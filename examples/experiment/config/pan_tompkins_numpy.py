#!/usr/bin/env python3
# =============================================================================
# pan_tompkins_numpy.py — baseline float64 potoku Pan–Tompkinsa
# (kampania baseline-numpy; REQUIREMENTS.md, sekcja "Plan badawczy — kampanie
# baseline"; cel: sekcja 7.5(i) artykułu main-debs.tex)
# =============================================================================
#
# Odwzorowuje etapy rec205-detect.rql w float64 (prawdziwe dzielenie zamiast
# arytmetyki wymiernej/całkowitej — to jest właśnie mierzony kontrast).
# Orientacja okna i struktura etapów IDENTYCZNE z examples/ecg/ref_float.py
# (okno @(1,N) = próbki n-(N-1)..n, parowane ze współczynnikami w kolejności
# pliku; patrz uwaga o orientacji w nagłówku ref_float.py).
#
# Dwa tryby pomiaru — raportowane OSOBNO, nie wolno ich porównywać wprost:
#
#   --mode slot   pętla próbka-po-próbce z absolutnymi terminami slotów
#                 (odpowiednik osi czasu xretractor -t). Emituje CSV sondy
#                 w formacie zgodnym z RDB_BENCH_CSV silnika:
#                     iter,compute_ns,wake_lag_ns,e2e_ns
#                 więc analiza tym samym examples/ecg/e1_stats.py.
#                 compute_ns  = etapy potoku (odpowiednik E1, bez emisji),
#                 wake_lag_ns = pobudka - termin slotu,
#                 e2e_ns      = termin slotu -> koniec emisji do sinka.
#                 UWAGA interpretacyjna: zawiera narzut interpretera CPython
#                 i wywołań NumPy per slot — to część mierzonego kosztu
#                 modelu wykonania, nie artefakt.
#
#   --mode batch  wektoryzowany przebieg całości (np.convolve, jak
#                 ref_float.py), R powtórzeń; raportuje czas/próbkę
#                 i przepustowość. To przepustowość batch bez semantyki
#                 slotowej — NIE odpowiednik E1.
#
# SCHED_FIFO: proces sam próbuje os.sched_setscheduler(FIFO,50); bez uprawnień
# degraduje się miękko (jak xretractor bez setcap), a runner może nadać
# politykę z zewnątrz przez `sudo -n chrt -f -p 50 <pid>` w czasie fazy
# --settle (dlatego pętla pomiarowa startuje z opóźnieniem).
#
# Stan początkowy okien: zera (jak ref_float.py). Silnik emituje na brzegu
# wartości null — dla pomiaru kosztu slotu różnica bez znaczenia, dla
# porównań dokładności (7.6) brzeg wykluczyć z porównania.
# =============================================================================

import argparse
import gc
import json
import os
import sys
import time

import numpy as np

SCHED_NAMES = {0: "OTHER", 1: "FIFO", 2: "RR"}


def fir_causal(x, coef):
    """Splot przyczynowy zgodny z ref_float.py (okno @(1,N), coef w kolejności pliku)."""
    coef = np.asarray(coef, dtype=np.float64)
    return np.convolve(x, coef[::-1], mode="full")[: len(x)]


def load_inputs(rec_path, bp_path, d_path):
    raw = np.fromfile(rec_path, dtype="<i4").reshape(-1, 2).astype(np.float64)
    return raw[:, 0].copy(), raw[:, 1].copy(), np.loadtxt(bp_path), np.loadtxt(d_path)


def try_set_fifo(prio):
    try:
        os.sched_setscheduler(0, os.SCHED_FIFO, os.sched_param(prio))
    except PermissionError:
        pass  # miękka degradacja; runner może nadać politykę przez chrt -p


def report_env(rate_hz, samples):
    policy = os.sched_getscheduler(0)
    meta = {
        "python": sys.version.split()[0],
        "numpy": np.__version__,
        "scheduler": SCHED_NAMES.get(policy, str(policy)),
        "rt_priority": os.sched_getparam(0).sched_priority,
        "affinity": sorted(os.sched_getaffinity(0)),
        "rate_hz": rate_hz,
        "samples": samples,
    }
    print("META " + json.dumps(meta), flush=True)
    return meta


def run_slot(mlii, v1, bp, d, rate_hz, samples, probe_path, sink_path, settle_s):
    n_rec = len(mlii)
    period_ns = round(1e9 / rate_hz)

    # Okna jak w rec205-detect.rql: 25 (band-pass), 5 (różniczka),
    # 30 (MWI), 180 (próg). Przesuwanie bufora + pełny dot/mean per slot
    # odwzorowuje strukturę etapów silnika (okno AGSE + agregat na oknie).
    win_bp = np.zeros(len(bp))
    win_d = np.zeros(len(d))
    win_mwi = np.zeros(30)
    win_thr = np.zeros(180)

    sink = open(sink_path, "w", encoding="utf-8")
    probe = np.empty((samples, 4), dtype=np.int64)

    # Faza settle: okno czasowe dla runnera na `sudo -n chrt -f -p 50 <pid>`.
    print(f"PID {os.getpid()}", flush=True)
    time.sleep(settle_s)
    report_env(rate_hz, samples)

    gc.disable()  # brak pauz GC w pętli pomiarowej (odnotowane w metodyce)
    t0 = time.monotonic_ns()
    for n in range(samples):
        deadline = t0 + n * period_ns
        now = time.monotonic_ns()
        if now < deadline:
            time.sleep((deadline - now) / 1e9)
            now = time.monotonic_ns()
        wake_lag = now - deadline

        t_c0 = now
        x = mlii[n % n_rec]
        win_bp[:-1] = win_bp[1:]
        win_bp[-1] = x
        bp_out = np.dot(win_bp, bp) / 1000.0
        win_d[:-1] = win_d[1:]
        win_d[-1] = bp_out
        d_out = np.dot(win_d, d)
        sq = d_out * d_out / 1000.0
        win_mwi[:-1] = win_mwi[1:]
        win_mwi[-1] = sq
        mwi = win_mwi.mean()
        win_thr[:-1] = win_thr[1:]
        win_thr[-1] = mwi
        thr = win_thr.mean()
        t_c1 = time.monotonic_ns()

        # Emisja (odpowiednik boradcast do kolejki klienta): sformatowany
        # wiersz detect_out do sinka. E2E = termin -> koniec emisji.
        sink.write(f"{x - 900.0} {v1[n % n_rec] - 900.0} {(mwi - 2.0 * thr) * 5.0}\n")
        t_e = time.monotonic_ns()

        probe[n, 0] = n
        probe[n, 1] = t_c1 - t_c0
        probe[n, 2] = wake_lag
        probe[n, 3] = t_e - deadline
    gc.enable()
    sink.close()

    with open(probe_path, "w", encoding="utf-8") as f:
        f.write("iter,compute_ns,wake_lag_ns,e2e_ns\n")
        for row in probe:
            f.write(f"{row[0]},{row[1]},{row[2]},{row[3]}\n")
    print(f"PROBE {probe_path}", flush=True)


def run_batch(mlii, v1, bp, d, samples, repeats):
    n_rec = len(mlii)
    if samples <= n_rec:
        x, xv = mlii[:samples], v1[:samples]
    else:  # zawinięcie źródła jak -k w silniku
        reps = -(-samples // n_rec)
        x = np.tile(mlii, reps)[:samples]
        xv = np.tile(v1, reps)[:samples]

    report_env(rate_hz=None, samples=samples)
    times_ns = []
    for _ in range(repeats):
        t0 = time.monotonic_ns()
        bp_out = fir_causal(x, bp) / 1000.0
        d_out = fir_causal(bp_out, d)
        sq = d_out * d_out / 1000.0
        mwi = fir_causal(sq, np.ones(30) / 30.0)
        thr = fir_causal(mwi, np.ones(180) / 180.0)
        out = np.column_stack((x - 900.0, xv - 900.0, (mwi - 2.0 * thr) * 5.0))
        t1 = time.monotonic_ns()
        times_ns.append(t1 - t0)
        del out

    times_ns.sort()
    med = times_ns[len(times_ns) // 2]
    print(f"batch: N={samples} powtórzeń={repeats}")
    print(f"czas przebiegu  : mediana {med / 1e6:.1f} ms "
          f"(min {times_ns[0] / 1e6:.1f}, max {times_ns[-1] / 1e6:.1f})")
    print(f"czas / próbkę   : {med / samples:.0f} ns")
    print(f"przepustowość   : {samples / (med / 1e9):,.0f} próbek/s (batch, bez semantyki slotowej)")


def main():
    ap = argparse.ArgumentParser(description="Baseline float64 Pan–Tompkinsa (slot|batch).")
    ap.add_argument("--rec", required=True, help="binarny plik EKG (int32 LE, pary MLII,V1)")
    ap.add_argument("--bp", required=True, help="współczynniki band-pass (25)")
    ap.add_argument("--d", required=True, help="współczynniki różniczki (5)")
    ap.add_argument("--mode", choices=["slot", "batch"], required=True)
    ap.add_argument("--rate", type=float, default=360.0, help="tempo napływu [Hz] (slot)")
    ap.add_argument("--samples", type=int, default=20000)
    ap.add_argument("--probe", default="probe.csv", help="wyjściowy CSV sondy (slot)")
    ap.add_argument("--sink", default=os.devnull, help="ujście emisji (slot)")
    ap.add_argument("--settle", type=float, default=3.0,
                    help="pauza przed pętlą pomiarową [s] — okno na chrt -p (slot)")
    ap.add_argument("--repeats", type=int, default=5, help="powtórzenia przebiegu (batch)")
    args = ap.parse_args()

    mlii, v1, bp, d = load_inputs(args.rec, args.bp, args.d)
    try_set_fifo(50)

    if args.mode == "slot":
        run_slot(mlii, v1, bp, d, args.rate, args.samples, args.probe, args.sink, args.settle)
    else:
        run_batch(mlii, v1, bp, d, args.samples, args.repeats)


if __name__ == "__main__":
    main()
