#!/usr/bin/env python3
# Generuje bp_coef.txt — wspolczynniki filtra pasmowoprzepustowego FIR
# stosowanego w kroku 1 algorytmu Pan-Tompkins.
#
# Metoda: okienkowa (windowed sinc)
#   h_bp[n] = (h_lp2[n] - h_lp1[n]) * w[n]
#   gdzie h_lp[n] = 2*fc*sinc(2*fc*(n-M)) to idealny filtr dolnoprzepustowy,
#   a w[n] = 0.54 - 0.46*cos(2*pi*n/(N-1)) to okno Hamminga.
#
# Parametry:
#   N   = 25    liczba wspolczynnikow (dlugosc filtra)
#   fc1 = 5 Hz  dolna czestotliwosc graniczna pasma przepustowego
#   fc2 = 15 Hz gorna czestotliwosc graniczna pasma przepustowego
#   fs  = 360 Hz czestotliwosc probkowania (MIT-BIH)
#   skala = 1000 mnoznik calkowitoliczbowy (dzielony w RQL przez 1000)
#
# Wyjscie: bp_coef.txt — jeden wspolczynnik INTEGER na linie, 25 linii.
# Uzycie w RQL:
#   DECLARE bp_coef INTEGER[25] STREAM bpf, 1 FILE 'bp_coef.txt'
#   SELECT mlii_win[_]*bpf[_] STREAM bp_acc FROM mlii_win+bpf
#   SELECT bp_acc[0]/1000 STREAM bp_out FROM bp_acc.sumc

import math
import pathlib

N = 25
M = (N - 1) // 2   # indeks srodkowy (opoznienie grupowe = M probek)
fc1 = 5.0 / 360    # czestotliwosc znormalizowana dolna
fc2 = 15.0 / 360   # czestotliwosc znormalizowana gorna
SCALE = 1000


def hamming(n, N):
    return 0.54 - 0.46 * math.cos(2 * math.pi * n / (N - 1))


def sinc(x):
    if abs(x) < 1e-12:
        return 1.0
    return math.sin(math.pi * x) / (math.pi * x)


coefs = []
for n in range(N):
    m = n - M
    w = hamming(n, N)
    h_lp1 = 2 * fc1 * sinc(2 * fc1 * m)
    h_lp2 = 2 * fc2 * sinc(2 * fc2 * m)
    coefs.append(round((h_lp2 - h_lp1) * w * SCALE))

out = pathlib.Path(__file__).parent / "bp_coef.txt"
out.write_text("\n".join(str(c) for c in coefs) + "\n")
print(f"Zapisano {len(coefs)} wspolczynnikow do {out}")
print(f"Wspolczynniki: {coefs}")
print(f"Suma (wzmocnienie DC): {sum(coefs)} / {SCALE} = {sum(coefs)/SCALE:.4f}")
