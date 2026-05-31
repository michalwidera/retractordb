#!/usr/bin/env python3
"""
Konwertuje nagranie MIT-BIH (format 212) na binarny plik retractordb.

Wejście:  <rekord>.hea  — nagłówek MIT-BIH (kanały, częstotliwość, liczba próbek)
          <rekord>.dat  — dane binarne format 212 (12-bit packed, 2 próbki / 3 bajty)

Wyjście:  <rekord>            — plik binarny retractordb (N × INTEGER na rekord, int32 LE)
          <rekord>-replay.rql — skrypt RQL DECLARE + SELECT odtwarzający sygnał w pętli

Plik <rekord>.desc jest tworzony przez build.sh za pomocą xretractor (nie przez ten skrypt).

Użycie:
    python3 mitbih2rdb.py rec205/205.hea

Odtwarzanie w xretractor (uruchamiać z katalogu rec205/):
    cd rec205
    xretractor 205-replay.rql -m 1000
"""

import argparse
import os
import struct
import sys
from math import gcd


def parse_hea(hea_path: str) -> tuple:
    """Parsuje nagłówek MIT-BIH (.hea). Zwraca (record, ns, fs, nsamp, signals)."""
    with open(hea_path) as f:
        lines = [ln.strip() for ln in f if ln.strip() and not ln.startswith('#')]

    parts = lines[0].split()
    record = parts[0]
    ns     = int(parts[1])
    fs     = int(parts[2])
    nsamp  = int(parts[3])

    signals = []
    for line in lines[1 : 1 + ns]:
        p        = line.split()
        filename = p[0]
        fmt      = int(p[1])
        gain     = int(p[2].split('/')[0])
        baseline = int(p[4]) if len(p) > 4 else 0
        # Nazwa kanału — ostatni token; musi pasować do gramatyki DESC: litera + [litera|cyfra|_|$]*
        name = p[8] if len(p) > 8 else f'ch{len(signals)}'
        signals.append({'filename': filename, 'format': fmt, 'gain': gain, 'baseline': baseline, 'name': name})

    return record, ns, fs, nsamp, signals


def decode_fmt212(dat_path: str, ns: int, nsamp: int) -> list[list[int]]:
    """
    Dekoduje format MIT-BIH 212 (12-bit packed).

    Dla ns=2: 3 bajty [B0, B1, B2] → 2 próbki ze znakiem:
        s0 = B0 | ((B1 & 0x0F) << 8)   → kanał 0
        s1 = B2 | ((B1 >>  4) << 8)    → kanał 1
    Rozszerzenie znaku z 12 bitów: wartości >= 2048 są ujemne.

    Dla ns=1: pary kolejnych próbek pakowane tak samo (3 bajty = 2 próbki).

    Zwraca listę ns list z nsamp próbkami każda.
    """
    with open(dat_path, 'rb') as f:
        raw = f.read()

    if ns not in (1, 2):
        raise ValueError(f'Obsługiwane liczby kanałów: 1 lub 2 (podano {ns})')

    samples: list[list[int]] = [[] for _ in range(ns)]

    if ns == 2:
        for i in range(nsamp):
            off = i * 3
            b0, b1, b2 = raw[off], raw[off + 1], raw[off + 2]
            s0 = b0 | ((b1 & 0x0F) << 8)
            s1 = b2 | ((b1 >> 4) << 8)
            if s0 >= 2048:
                s0 -= 4096
            if s1 >= 2048:
                s1 -= 4096
            samples[0].append(s0)
            samples[1].append(s1)
    else:  # ns == 1
        for i in range(0, nsamp, 2):
            off = (i // 2) * 3
            b0, b1, b2 = raw[off], raw[off + 1], raw[off + 2]
            s0 = b0 | ((b1 & 0x0F) << 8)
            s1 = b2 | ((b1 >> 4) << 8)
            if s0 >= 2048:
                s0 -= 4096
            if s1 >= 2048:
                s1 -= 4096
            samples[0].append(s0)
            if i + 1 < nsamp:
                samples[0].append(s1)

    return samples


def write_bin(bin_path: str, samples: list[list[int]], ns: int, nsamp: int) -> None:
    """Zapisuje plik binarny retractordb: ns × INTEGER (int32 LE) na rekord."""
    with open(bin_path, 'wb') as f:
        for i in range(nsamp):
            for ch in range(ns):
                f.write(struct.pack('<i', samples[ch][i]))



def interval_rql(fs: int) -> str:
    """Zwraca interwał próbkowania jako ułamek RQL (np. 1/360 dla 360 Hz)."""
    g = gcd(1, fs)
    num, den = 1 // g, fs // g
    return f'{num}/{den}' if den != 1 else str(num)


def write_rql(rql_path: str, record: str, bin_name: str, fs: int, signals: list[dict]) -> None:
    """Generuje plik RQL odtwarzający sygnał ECG w pętli.

    DECLARE deklaruje strumień wejściowy z pliku binarnego (TYPE DEVICE = pętla).
    SELECT przepisuje kanały do strumienia wyjściowego VOLATILE (brak zapisu na dysk).
    Plik należy uruchamiać z katalogu zawierającego pliki danych (rec205/).
    bin_name: nazwa pliku binarnego (zaczyna się od litery, zgodna z tokenem FILENAME w DESC.g4)
    """
    interval     = interval_rql(fs)
    channel_list = ', '.join(f'{s["name"]} INTEGER' for s in signals)
    select_list  = ', '.join(f'ecg.{s["name"]}' for s in signals)
    duration_s   = 1.0 / fs

    with open(rql_path, 'w') as f:
        f.write(f'# Odtwarzanie EKG — rekord {record}\n')
        f.write(f'# Kanaly: {", ".join(s["name"] for s in signals)}\n')
        f.write(f'# Czestotliwosc: {fs} Hz, interwal {duration_s:.6f} s ({interval} s w RQL)\n')
        f.write(f'# Uruchamiac z katalogu zawierajacego pliki danych:\n')
        f.write(f'#   cd {os.path.dirname(rql_path)}\n')
        f.write(f'#   xretractor {os.path.basename(rql_path)} -m <liczba_cykli>\n')
        f.write('#\n')
        f.write(f'# Zrodlo danych czyta plik binarny w petli (TYPE DEVICE bez ONESHOT).\n')
        f.write(f'# Kazdy cykl xretractor odpowiada jednej probce EKG ({duration_s:.6f} s).\n')
        f.write('\n')
        # Nazwa strumienia musi zaczynać się od litery (gramatyka: ID: [A-Za-z][A-Za-z_$0-9]*)
        out_stream = ('s' + record if record[0].isdigit() else record) + 'out'
        f.write(f"DECLARE {channel_list} STREAM ecg, {interval} FILE '{bin_name}'\n")
        f.write('\n')
        f.write(f'SELECT {select_list} STREAM {out_stream} FROM ecg VOLATILE\n')


def main() -> None:
    parser = argparse.ArgumentParser(
        description='Konwertuje nagranie MIT-BIH format 212 na format retractordb'
    )
    parser.add_argument('hea_file', help='Ścieżka do pliku nagłówkowego (.hea)')
    args = parser.parse_args()

    hea_path = os.path.abspath(args.hea_file)
    if not os.path.exists(hea_path):
        sys.exit(f'Błąd: plik {hea_path} nie istnieje')

    base_dir = os.path.dirname(hea_path)
    record, ns, fs, nsamp, signals = parse_hea(hea_path)

    dat_path = os.path.join(base_dir, signals[0]['filename'])
    if not os.path.exists(dat_path):
        sys.exit(f'Błąd: plik danych {dat_path} nie istnieje')

    fmt = signals[0]['format']
    if fmt != 212:
        sys.exit(f'Błąd: obsługiwany jest tylko format 212 (podano {fmt})')

    # Nazwa pliku binarnego musi zaczynać się od litery — gramatyka DESC FILENAME
    # nie może rozróżnić cyfr od tokenu DECIMAL gdy cała nazwa to cyfry (np. "205").
    bin_name = ('rec' + record if record[0].isdigit() else record)
    bin_path = os.path.join(base_dir, bin_name)
    rql_path = os.path.join(base_dir, bin_name + '-replay.rql')

    channel_names = [s['name'] for s in signals]
    duration_s    = nsamp / fs

    print(f'Rekord  : {record}')
    print(f'Kanały  : {ns} ({", ".join(channel_names)})')
    print(f'Próbk.  : {fs} Hz, {nsamp} próbek, czas ~{duration_s:.1f} s ({duration_s/60:.1f} min)')
    print(f'Gain    : {[s["gain"] for s in signals]} ADU/mV, baseline {[s["baseline"] for s in signals]}')
    print()

    print(f'Dekodowanie {dat_path} ...')
    samples = decode_fmt212(dat_path, ns, nsamp)

    size_b = ns * nsamp * 4
    print(f'Zapis binarny       : {bin_path}')
    print(f'  {ns} kanaly × {nsamp} próbek × 4 B = {size_b} B ({size_b / 1024:.1f} KB)')
    write_bin(bin_path, samples, ns, nsamp)

    print(f'Zapis RQL           : {rql_path}')
    write_rql(rql_path, record, bin_name, fs, signals)

    rel_open = os.path.relpath(bin_path)
    rel_dir  = os.path.relpath(base_dir)
    print()
    print('─' * 60)
    print('Inspekcja w xtrdb (z dowolnego katalogu roboczego):')
    print(f'  xtrdb')
    print(f'  > open {rel_open}')
    print(f'  > size')
    print(f'  > read 0 ; print')
    print()
    print('Odtwarzanie w xretractor (z katalogu rec205/):')
    print(f'  cd {rel_dir}')
    print(f'  xretractor {bin_name}-replay.rql -m 1000')
    print('─' * 60)


if __name__ == '__main__':
    main()
