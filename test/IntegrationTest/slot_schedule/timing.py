#!/usr/bin/env python3
"""#43: harmonogram slotow mierzony z zewnatrz, po chwilach pojawiania sie rekordow w magazynie.

Rekord jest zapisywany w trakcie swojego slotu (zapis bez bufora), wiec chwila wzrostu pliku
to chwila slotu z dokladnoscia do odpytywania (~0,5 ms). Zegar monotoniczny, wylacznie roznice.

  watch STOP BYTES OUT PLIK...     zapisuje do OUT wiersze "<plik> <indeks> <czas [s]>",
                                   az powstanie plik STOP (ostatnie odpytanie juz po nim)
  drift OUT PLIK OKRES LIMIT       przesuniecie wzgledem siatki nie rosnie: mediana w ostatniej
                                   tercji minus mediana w srodkowej < LIMIT
  catchup OUT PLIK OKRES PRZESTOJ LIMIT
                                   po jednym przestoju zalegle sloty ida bez snu, a wykonanie
                                   wraca na pierwotna siatke (przesuniecie < LIMIT)
  grid OUT PLIK OKRES LIMIT        caly ciag na jednej siatce: rozrzut przesuniec < LIMIT
  phase OUT PLIK OKRES WZOR OKRES_WZORU LIMIT
                                   rekordy PLIKU leza na siatce OKRESU od kotwicy wyznaczonej
                                   z mediany rekordow WZORU (rekord k pojawia sie (k+1)*OKRES_WZORU
                                   po kotwicy); mediana, bo jitter budzika jednego rekordu (macOS,
                                   #408) przesuwalby cala os

Kod wyjscia 1 i opis, gdy warunek nie zachodzi.
"""

import os
import statistics
import sys
import time


def watch(stop, record_bytes, out, paths):
    seen = {path: 0 for path in paths}
    with open(out, "w") as sink:
        while True:
            finished = os.path.exists(stop)
            now = time.monotonic()
            for path in paths:
                try:
                    count = os.stat(path).st_size // record_bytes
                except FileNotFoundError:
                    count = 0
                while seen[path] < count:
                    sink.write(f"{path} {seen[path]} {now:.6f}\n")
                    seen[path] += 1
            if finished:
                return 0
            time.sleep(0.0005)


def load(out, path):
    times = [float(line.split()[2]) for line in open(out) if line.split()[0] == path]
    if len(times) < 9:
        sys.exit(f"{path}: tylko {len(times)} rekordow - za malo do oceny harmonogramu")
    return times


def offsets(times, period):
    return [t - times[0] - k * period for k, t in enumerate(times)]


def ms(value):
    return f"{value * 1000:.1f} ms"


def drift(out, path, period, limit):
    offs = offsets(load(out, path), period)
    n = len(offs)
    middle = statistics.median(offs[n // 3 : 2 * n // 3])
    last = statistics.median(offs[2 * n // 3 :])
    print(f"drift {path}: {n} rekordow, mediana srodkowej tercji {ms(middle)}, ostatniej {ms(last)}")
    if last - middle >= limit:
        sys.exit(f"opoznienie rosnie o {ms(last - middle)} (limit {ms(limit)}) - sloty przesuwa czas pracy")
    return 0


def catchup(out, path, period, stall, limit):
    times = load(out, path)
    offs = offsets(times, period)
    gaps = [b - a for a, b in zip(times, times[1:])]
    s = max(range(len(gaps)), key=gaps.__getitem__)
    overdue = int(stall / period) - 1
    after = s + int(stall / period) + 2
    if gaps[s] < 0.8 * stall or overdue < 2 or len(times) - after < 5:
        sys.exit(f"przestoj nie wystapil jak zaplanowano: najwieksza przerwa {ms(gaps[s])} po rekordzie {s}")
    burst = times[s + overdue] - times[s + 1]
    before = statistics.median(offs[: s + 1])
    back = statistics.median(offs[after:])
    print(f"catchup {path}: przestoj {ms(gaps[s])} po rekordzie {s}, {overdue} zaleglych slotow w {ms(burst)}, "
          f"przesuniecie przed {ms(before)}, po powrocie {ms(back)}")
    if burst >= period / 2:
        sys.exit(f"zalegle sloty nie poszly bez snu: {overdue} slotow w {ms(burst)}")
    if back - before >= limit:
        sys.exit(f"wykonanie nie wrocilo na siatke: przesuniecie {ms(back - before)} (limit {ms(limit)})")
    return 0


def grid(out, path, period, limit):
    offs = offsets(load(out, path), period)
    spread = max(offs) - min(offs)
    print(f"grid {path}: {len(offs)} rekordow, rozrzut przesuniec {ms(spread)}")
    if spread >= limit:
        sys.exit(f"rekordy zeszly z siatki: rozrzut {ms(spread)} (limit {ms(limit)})")
    return 0


def phase(out, path, period, reference, reference_period, limit):
    anchor = statistics.median(t - (k + 1) * reference_period for k, t in enumerate(load(out, reference)))
    times = load(out, path)
    residuals = [(t - anchor) - round((t - anchor) / period) * period for t in times]
    worst = max(residuals, key=abs)
    print(f"phase {path}: {len(times)} rekordow, najwieksze odchylenie od siatki kotwicy {ms(worst)}")
    if abs(worst) >= limit:
        sys.exit(f"strumien zszedl z osi kotwicy: odchylenie {ms(worst)} (limit {ms(limit)})")
    return 0


def main(argv):
    command, args = argv[1], argv[2:]
    if command == "watch":
        return watch(args[0], int(args[1]), args[2], args[3:])
    if command == "drift":
        return drift(args[0], args[1], float(args[2]), float(args[3]))
    if command == "catchup":
        return catchup(args[0], args[1], float(args[2]), float(args[3]), float(args[4]))
    if command == "grid":
        return grid(args[0], args[1], float(args[2]), float(args[3]))
    if command == "phase":
        return phase(args[0], args[1], float(args[2]), args[3], float(args[4]), float(args[5]))
    sys.exit(f"nieznane polecenie: {command}")


if __name__ == "__main__":
    sys.exit(main(sys.argv))
