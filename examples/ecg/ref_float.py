#!/usr/bin/env python3
# =============================================================================
# ref_float.py — referencja zmiennoprzecinkowa dla eksperymentu E2 (dokładność)
# =============================================================================
#
# CEL EKSPERYMENTU E2
#   Zweryfikować główny wynik teoretyczny pracy: operatory algebry są dokładne
#   i deterministyczne nad liczbami wymiernymi (Tw. 2, Wniosek o determinizmie).
#   Porównujemy dwie ścieżki tego samego potoku Pan–Tompkins:
#
#     (a) ścieżka WYMIERNA  — RetractorDB (arytmetyka całkowita/wymierna);
#                             wynik przechwytujemy z xqry, jest bit-dokładny
#                             i powtarzalny między uruchomieniami i platformami.
#     (b) ścieżka FLOAT     — TEN skrypt: identyczna struktura operacji, ale
#                             w double i z prawdziwym dzieleniem; akumuluje
#                             błąd zaokrągleń. To "naiwny baseline".
#
#   Różnica decyzji QRS oraz brak powtarzalności (b) wobec powtarzalności (a)
#   stanowi eksperymentalny dowód, że dokładność na Q nie jest formalnością.
#
# ODWZOROWANY POTOK (zgodny z rec205-detect.rql)
#   1. band-pass  : okno 25, splot z bp_coef, /1000
#   2. różniczka  : okno  5, splot z d_coef
#   3. kwadrat    : x*x/1000
#   4. MWI        : średnia ruchoma 30  (~83 ms @360 Hz)
#   5. próg       : średnia ruchoma 180 z MWI (0,5 s); QRS gdy mwi > 2*próg
#
#   UWAGA O ORIENTACJI OKNA: zakładamy, że operator @(1,N) tworzy okno
#   [próbka n-(N-1) .. próbka n] (od najstarszej do najnowszej) i paruje je
#   ze współczynnikami w kolejności pliku — co odpowiada splotowi przyczynowemu
#   z odwróconym wektorem współczynników. Dla bp_coef (symetryczny) orientacja
#   jest bez znaczenia, ale dla d_coef (antysymetryczny) — istotna. Przed
#   raportowaniem liczb do artykułu należy potwierdzić tę orientację względem
#   faktycznej semantyki AGSE w kodzie (kompilacja-zapytan/przetwarzanie-symbolu-_).
#
# JAK UŻYĆ
#   # 1. policz baseline float i zapisz decyzje:
#   python3 ref_float.py --rec rec205/rec205 \
#                        --bp rec205/bp_coef.txt --d rec205/d_coef.txt
#
#   # 2. przechwyć ścieżkę wymierną z RetractorDB (dwukrotnie, ew. na ARM):
#   #    ./xretractor rec205-detect.rql -k -x &
#   #    xqry detect_out -m 650000 -r > rational_run1.txt
#
#   # 3. porównaj decyzje QRS (wymierna vs float):
#   python3 ref_float.py --rec rec205/rec205 \
#                        --bp rec205/bp_coef.txt --d rec205/d_coef.txt \
#                        --rational rational_run1.txt
#
#   Powtarzalność (a) sprawdza się niezależnie: md5sum rational_run*.txt
#   (muszą być identyczne); md5sum dla dwóch uruchomień baseline float pokaże,
#   czy float jest powtarzalny na danej maszynie (zwykle tak na jednej, ale
#   rozjeżdża się między architekturami / flagami -ffast-math / kolejnością SIMD).
#
# ZALEŻNOŚCI: numpy.
# =============================================================================

import argparse

import numpy as np


def fir_causal(x, coef):
    """
    Splot przyczynowy odwzorowujący okno @(1,N) parowane z 'coef' w kolejności
    pliku (najstarsza..najnowsza próbka). Zwraca wektor długości len(x):
        y[n] = sum_{k=0..N-1} coef[k] * x[n-(N-1)+k]
    co jest równoważne splotowi z odwróconymi współczynnikami, obciętemu
    do części przyczynowej (brzeg początkowy zależy od stanu zerowego okna).
    """
    coef = np.asarray(coef, dtype=np.float64)
    return np.convolve(x, coef[::-1], mode="full")[: len(x)]


def parse_last_column(path):
    """
    Wyciąga ostatnią liczbową kolumnę z surowego wyjścia xqry (-r). Format raw
    bywa rozdzielany spacją/tabem/przecinkiem; bierzemy ostatni token dający
    się sparsować jako float. W razie nietypowego formatu dopasuj tę funkcję.
    """
    vals = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            toks = line.replace(",", " ").split()
            for tok in reversed(toks):
                try:
                    vals.append(float(tok))
                    break
                except ValueError:
                    continue
    return np.asarray(vals, dtype=np.float64)


def main():
    ap = argparse.ArgumentParser(description="Referencja float dla E2 (dokładność).")
    ap.add_argument("--rec", default="rec205/rec205",
                    help="binarny plik EKG (int32 LE, pary MLII,V1)")
    ap.add_argument("--bp", default="rec205/bp_coef.txt", help="współczynniki band-pass (25)")
    ap.add_argument("--d", default="rec205/d_coef.txt", help="współczynniki różniczki (5)")
    ap.add_argument("--rational", default=None,
                    help="przechwyt detect_out z xqry (ścieżka wymierna) do porównania")
    ap.add_argument("--out", default="/tmp/detect_float.npy",
                    help="gdzie zapisać decyzje QRS baseline float")
    args = ap.parse_args()

    # --- wczytanie danych ---
    raw = np.fromfile(args.rec, dtype="<i4").reshape(-1, 2).astype(np.float64)
    mlii = raw[:, 0]
    bp = np.loadtxt(args.bp)
    d = np.loadtxt(args.d)

    # --- potok Pan–Tompkins w double (baseline) ---
    bp_out = fir_causal(mlii, bp) / 1000.0          # 1. band-pass + skala
    d_out = fir_causal(bp_out, d)                   # 2. różniczka
    sq = d_out * d_out / 1000.0                      # 3. kwadrat
    mwi = fir_causal(sq, np.ones(30) / 30.0)         # 4. MWI (średnia 30)
    thr = fir_causal(mwi, np.ones(180) / 180.0)      # 5. próg (średnia 180)

    detect_signal = mwi - 2.0 * thr                  # >0 => wykryty QRS
    detect_float = detect_signal > 0
    np.save(args.out, detect_float)
    print(f"[float] N={len(detect_float)}  wykrytych próbek QRS={int(detect_float.sum())}")
    print(f"[float] zapisano decyzje -> {args.out}")

    # --- porównanie ze ścieżką wymierną (jeśli podano) ---
    if args.rational:
        col = parse_last_column(args.rational)       # trzecie pole detect_out (skalowane *5)
        decision_rational = col > 0
        n = min(len(decision_rational), len(detect_float))
        diff = int(np.sum(decision_rational[:n] != detect_float[:n]))
        print(f"[E2] porównano {n} decyzji QRS")
        print(f"[E2] różniących się decyzji (wymierna vs float): {diff}  "
              f"({diff / n * 100:.4f} %)")
        print("[E2] metrykę 'maks. rozbieżność w MWI' uzyskasz, dodając do .rql "
              "podgląd 'SELECT mwi[0] STREAM mwi_dbg ...' i porównując mwi_dbg "
              "z 'mwi' z tego skryptu.")


if __name__ == "__main__":
    main()
