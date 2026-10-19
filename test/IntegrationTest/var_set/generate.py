#!/usr/bin/env python3
"""Generator danych testu akcji DO SET (#531, etap B).

Dane `*.txt` sa zapisane w tym katalogu; test ich nie generuje - czyta pliki z repozytorium.
Generator jest po to, zeby dane dalo sie odtworzyc: stale ziarno, wiec ponowne uruchomienie daje
te same bajty. Plany `*.rql` pisane sa recznie.

Losowosc wypelnia tlo, a zdarzenia, ktore test musi zobaczyc, sa WYMUSZONE: przekroczenia progow,
krok automatu, konflikt zapisu w slocie wolnego taktu (D3). Ze zdarzenie rzeczywiscie zaszlo,
sprawdza wyrocznia (`oracle.py`), nie generator.

Interwaly i progi szkicow uc10-uc14 sa przeskalowane, zeby przebieg mial setki rekordow, a nie
setki tysiecy: takt godzinny uc10 3600 -> 10, dobowy uc14 86400 -> 20, cisza uc11 600 -> 20,
pojemnosc uc14 400 -> 40. Logika planow jest ta sama co w szkicach.

    python3 generate.py            # z tego katalogu; nadpisuje *.txt
"""
import random
from pathlib import Path

HERE = Path(__file__).resolve().parent


def write(name, rows):
    lines = [" ".join(str(v) for v in row) if isinstance(row, (list, tuple)) else str(row) for row in rows]
    (HERE / name).write_text("\n".join(lines) + "\n")


def signal(rng):
    # Histereza 250/200, licznik > 100, EMA: wartosci w pasmie, nad i pod nim, ze stalymi przejsciami.
    rows = []
    for _ in range(12):
        rows += [rng.randint(60, 100) for _ in range(rng.randint(2, 4))]
        rows += [rng.randint(201, 249) for _ in range(rng.randint(2, 4))]
        rows += [rng.randint(250, 320) for _ in range(rng.randint(1, 3))]
        rows += [rng.randint(201, 249) for _ in range(rng.randint(2, 4))]
        rows += [rng.randint(120, 199) for _ in range(rng.randint(1, 3))]
    return rows


def slot(rng):
    src = [rng.randint(1, 999) for _ in range(40)]
    slow = [rng.randint(1000, 1999) for _ in range(40)]
    return src, slow


def conflict(rng):
    # > 50: pisza obie reguly (konflikt), 1..50: tylko r_down, <= 0: zadna.
    return [rng.choice([rng.randint(51, 99), rng.randint(1, 50), rng.randint(-9, 0)]) for _ in range(60)]


def uc10(rng):
    # Temperatura komory: przekroczenie T_ON = 50 w gore, T_OFF = 20 w dol. Godzina = 10 slotow,
    # rekord n strumienia temp liczy sie w slocie n + 1, wiec slot zerowania godziny (10(m+1))
    # to rekord 10m + 9. Wymuszone: wlaczenie sprezarki dokladnie w takim rekordzie (konflikt
    # cycle / hour_reset, D3) i trzy cykle w jednej godzinie (alarm krotkich cykli).
    n = 200
    temp = [rng.randint(21, 49) for _ in range(n)]
    ups = [3, 29, 52, 55, 58, 89, 120, 149, 175]
    downs = [10, 40, 54, 57, 70, 100, 135, 160, 185]
    for i in ups:
        temp[i] = rng.randint(50, 70)
    for i in downs:
        temp[i] = rng.randint(5, 20)
    hour = list(range(1, n // 10 + 3))
    return temp, hour


def uc11(rng):
    # Wiatr i obroty: pelny przebieg 0 -> 1 -> 2 -> 3 -> 0, nieudany rozruch 1 -> 0 i przerwana cisza.
    rows = []

    def add(count, wind, rpm):
        for _ in range(count):
            rows.append((rng.randint(*wind), rng.randint(*rpm)))

    add(5, (0, 30), (0, 100))        # postoj
    add(3, (35, 60), (100, 800))     # rozruch
    add(3, (10, 29), (100, 800))     # wiatr siada: 1 -> 0
    add(3, (35, 60), (100, 800))     # ponowny rozruch
    add(5, (60, 200), (900, 1500))   # produkcja
    add(2, (250, 300), (900, 1500))  # burza: 2 -> 3, zrzut
    add(8, (150, 200), (0, 100))     # cisza
    add(1, (210, 240), (0, 100))     # poryw przerywa cisze
    add(25, (100, 200), (0, 100))    # cisza do konca: 3 -> 0
    add(10, (0, 30), (0, 100))       # postoj
    return rows


def uc12(rng):
    # SpO2 w dziesiatych procenta: baza okolo 960, dwa zdarzenia desaturacji z powrotem.
    rows = [rng.randint(955, 970) for _ in range(30)]
    rows += [rng.randint(915, 925) for _ in range(6)]   # spadek > 3 pkt ponizej bazy
    rows += [rng.randint(935, 945) for _ in range(4)]   # w histerezie
    rows += [rng.randint(955, 970) for _ in range(20)]  # powrot
    rows += [rng.randint(900, 920) for _ in range(5)]
    rows += [rng.randint(955, 970) for _ in range(15)]
    return rows


def uc14(rng):
    # Bramki: najpierw napelnianie do pojemnosci 40, potem ruch zrownowazony; przeliczenie co 20
    # slotow z dryfem wzgledem bilansu bramek.
    n = 160
    gate = []
    occupancy = 0
    for i in range(n):
        if i < 60:
            vin, vout = rng.randint(0, 2), rng.randint(0, 1)
        else:
            vin, vout = rng.randint(0, 2), rng.randint(0, 2)
        vout = min(vout, occupancy + vin)
        occupancy += vin - vout
        gate.append((vin, vout))
    recount = [max(0, 20 * (m + 1) // 2 + rng.randint(-3, 3)) for m in range(n // 20 + 2)]
    return gate, recount


def main():
    rng = random.Random(533)
    write("signal.txt", signal(rng))
    src, slow = slot(rng)
    write("slot_src.txt", src)
    write("slot_slow.txt", slow)
    write("conflict.txt", conflict(rng))
    temp, hour = uc10(rng)
    write("uc10_temp.txt", temp)
    write("uc10_hour.txt", hour)
    write("uc11_turbine.txt", uc11(rng))
    write("uc12_spo2.txt", uc12(rng))
    gate, recount = uc14(rng)
    write("uc14_gate.txt", gate)
    write("uc14_recount.txt", recount)


if __name__ == "__main__":
    main()
