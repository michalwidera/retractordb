#!/usr/bin/env python3
# Generuje d_coef.txt — wspolczynniki filtra rozniczkujacego FIR
# stosowanego w kroku 2 algorytmu Pan-Tompkins.
#
# Filtr 5-punktowy (przyblizona pierwsza pochodna) wg Pan & Tompkins (1985):
#   y[n] = (1/8T) * (-x[n-4] - 2*x[n-3] + 2*x[n-1] + x[n])
#
# Wspolczynniki (w kolejnosci od najstarszej do najnowszej probki):
#   h = [-1, -2, 0, 2, 1]
#
# Wlasciwosci:
#   - Suma wspolczynnikow = 0  (zerowe wzmocnienie DC — eliminuje ofsety)
#   - Maksymalizuje pochodna na czestotliwosciach QRS (~10-25 Hz)
#   - Przy fs=360 Hz czynnik skalujacy (1/8T) = 360/8 = 45 Hz
#     (pomijany w implementacji — nie wplywa na detekcje szczytow)
#
# Uzycie w RQL:
#   DECLARE d_coef INTEGER[5] STREAM df, 1 FILE 'd_coef.txt'
#   SELECT bp_win[_]*df[_] STREAM d_acc FROM bp_win+df
#   SELECT d_acc[0] STREAM d_out FROM d_acc.sumc

import pathlib

# Wspolczynniki zgodnie z Pan & Tompkins IEEE Trans. Biomed. Eng. 32(3), 1985
COEFS = [-1, -2, 0, 2, 1]

out = pathlib.Path(__file__).parent / "d_coef.txt"
out.write_text("\n".join(str(c) for c in COEFS) + "\n")
print(f"Zapisano {len(COEFS)} wspolczynnikow do {out}")
print(f"Wspolczynniki: {COEFS}")
print(f"Suma (wzmocnienie DC): {sum(COEFS)}  (powinno byc 0)")
