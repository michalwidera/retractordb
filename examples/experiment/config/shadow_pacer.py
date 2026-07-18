#!/usr/bin/env python3
"""Proces-cien dla sledztwa ~40 ms (Faza 0).

Taktowana petla z absolutnymi deadline'ami na CLOCK_MONOTONIC -- ZERO pracy
w slocie: bez IPC, bez artefaktow, bez alokacji. Rejestruje wylacznie wlasne
opoznienie pobudki (wake_lag). Mechanizm taktowania identyczny jak w
pan_tompkins_numpy.py (deadline = anchor + n*period; time.sleep reszty
budzetu), zeby wynik byl porownywalny z opublikowanym baseline'em NumPy.

Role:
  - badanie S1 (pacer-solo):    sam na rdzeniu 3 -- czy platforma bez silnika
                                generuje zdarzenia >= progu?
  - badanie S2 (engine-shadow): na rdzeniu 2 rownolegle z silnikiem na 3 --
                                czy zdarzenia silnika sa system-wide?

Protokol startu (jak run_numpy_baseline.sh): proces drukuje "PID <n>" i spi
--settle sekund; w tym oknie skrypt nadaje mu SCHED_FIFO przez
`sudo -n chrt -f -p 50 <pid>`. Kotwica czasu (anchor) jest pobierana PO fazie
settle. SIGTERM konczy przedwczesnie, ale zapisuje komplet zebranych danych
(tryb engine-shadow: pacer jest ubijany po zakonczeniu silnika).

Wyjscie CSV (naglowek z metadanymi w komentarzach '#'):
  anchor_mono_ns / real_at_anchor_ns -- mapowanie MONOTONIC<->REALTIME do
  zgrubnej korelacji zdarzen miedzy procesami.
  Wiersze: iter,wake_lag_ns  (czas bezwzgledny pobudki odtwarzalny jako
  anchor_mono_ns + (iter+1)*period_ns + wake_lag_ns).
"""

import argparse
import array
import os
import signal
import sys
import time

stop_requested = False


def on_sigterm(_sig, _frm):
    global stop_requested
    stop_requested = True


def main() -> int:
    ap = argparse.ArgumentParser(description="proces-cien: taktowana petla bez pracy")
    ap.add_argument("--rate", type=float, default=360.0, help="czestosc slotow [Hz]")
    ap.add_argument("--samples", type=int, required=True, help="liczba slotow")
    ap.add_argument("--out", required=True, help="plik CSV wynikow (najlepiej w /dev/shm)")
    ap.add_argument("--settle", type=float, default=2.0, help="okno na chrt -f -p z zewnatrz [s]")
    ap.add_argument("--tag", default="", help="etykieta roli (np. S1-core3, S2-core2)")
    args = ap.parse_args()

    signal.signal(signal.SIGTERM, on_sigterm)
    signal.signal(signal.SIGINT, on_sigterm)

    print(f"PID {os.getpid()}", flush=True)
    time.sleep(args.settle)

    period_ns = round(1e9 / args.rate)
    lags = array.array("q", bytes(8 * args.samples))  # prealokacja: zero alokacji w petli

    mono = time.clock_gettime_ns
    MONO = time.CLOCK_MONOTONIC
    anchor = mono(MONO)
    real_at_anchor = time.clock_gettime_ns(time.CLOCK_REALTIME)

    n = 0
    for n in range(args.samples):
        if stop_requested:
            break
        deadline = anchor + (n + 1) * period_ns
        now = mono(MONO)
        if now < deadline:
            time.sleep((deadline - now) / 1e9)
            now = mono(MONO)
        lags[n] = now - deadline
    done = n + (0 if stop_requested else 1)

    with open(args.out, "w") as f:
        f.write(f"# shadow_pacer tag={args.tag} rate_hz={args.rate} period_ns={period_ns}\n")
        f.write(f"# anchor_mono_ns={anchor} real_at_anchor_ns={real_at_anchor}\n")
        f.write(f"# samples_requested={args.samples} samples_done={done} "
                f"terminated_early={int(stop_requested)}\n")
        f.write("iter,wake_lag_ns\n")
        for i in range(done):
            f.write(f"{i},{lags[i]}\n")

    lag_sorted = sorted(lags[:done])
    if lag_sorted:
        med = lag_sorted[done // 2]
        print(f"shadow_pacer[{args.tag}]: sloty={done} mediana={med / 1e3:.1f}us "
              f"max={lag_sorted[-1] / 1e6:.3f}ms", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
