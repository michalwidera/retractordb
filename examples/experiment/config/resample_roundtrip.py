#!/usr/bin/env python3
"""Badanie 7.6(b/c) — referencja zmiennoprzecinkowa round-tripu konwersji tempa.

Kontrast dla tozsamosci okreznej przeplotu/rozplotu silnika (cor:exact):
w algebrze wymiernej konwersja tempa jest DOKLADNYM przeindeksowaniem
(blad == 0 co do bitu), w klasycznym DSP jest filtracja — round-trip
360 Hz -> 720 Hz -> 360 Hz na kanale MLII (float64) akumuluje blad zalezny
od dlugosci nagrania. Raportujemy norme bledu w funkcji N (plan badawczy
REQUIREMENTS.md, "Czesc dla 7.6(b)").

Dwie metody referencyjne (obie standardowe w scipy.signal):
  poly — resample_poly(x,2,1) -> resample_poly(y,1,2)  (filtr polifazowy FIR)
  fft  — resample(x,2N) -> resample(y,N)               (metoda Fouriera)

Normy: pelna (caly wektor) oraz wnetrze (z marginesem brzegowym
max(64, N//20) probek z kazdej strony — efekty brzegowe filtracji
raportowane osobno od bledu wnetrza).

Uzycie: resample_roundtrip.py --rec205 <plik> --out <csv>
"""

import argparse
import json
import platform
import sys

import numpy as np
from scipy import signal
from scipy import __version__ as scipy_version

GRID = [1250, 2500, 5000, 10000, 20000, 40000, 80000, 160000, 325000, 650000]


def norms(err: np.ndarray, margin: int) -> tuple[float, float, float, float]:
    full_max = float(np.max(np.abs(err)))
    full_rms = float(np.sqrt(np.mean(err * err)))
    interior = err[margin:-margin] if margin > 0 and len(err) > 2 * margin else err
    int_max = float(np.max(np.abs(interior)))
    int_rms = float(np.sqrt(np.mean(interior * interior)))
    return full_max, full_rms, int_max, int_rms


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--rec205", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    raw = np.fromfile(args.rec205, dtype="<i4").reshape(-1, 2)
    mlii = raw[:, 0].astype(np.float64)

    meta = {
        "python": platform.python_version(),
        "numpy": np.__version__,
        "scipy": scipy_version,
        "rec205_samples": int(len(mlii)),
        "grid": GRID,
    }
    print("META", json.dumps(meta))

    rows = ["method,n,margin,max_abs_full,rms_full,max_abs_interior,rms_interior"]
    for n in GRID:
        if n > len(mlii):
            break
        x = mlii[:n]
        margin = max(64, n // 20)

        up = signal.resample_poly(x, 2, 1)
        down = signal.resample_poly(up, 1, 2)
        rows.append("poly,%d,%d,%.6e,%.6e,%.6e,%.6e" % (n, margin, *norms(down - x, margin)))

        up = signal.resample(x, 2 * n)
        down = signal.resample(up, n)
        rows.append("fft,%d,%d,%.6e,%.6e,%.6e,%.6e" % (n, margin, *norms(down - x, margin)))

    with open(args.out, "w", encoding="utf-8") as f:
        f.write("\n".join(rows) + "\n")
    print("\n".join(rows))
    return 0


if __name__ == "__main__":
    sys.exit(main())
