#!/usr/bin/env python3
"""Wykonanie planu i porównanie treści rekordów — bramka poprawności
odwzorowania oracle'a.

Bramka nie mierzy czasu i nie porównuje systemów. Sprawdza jedno: czy silnik
emituje dokładnie te wartości, które przewiduje odwzorowanie rekordów
w model.py. Dopiero po jej przejściu rozbieżność ogona wolno czytać jako
rozbieżność ogona, a nie jako różnicę w definicji operatora.
"""

import os
import struct
import subprocess
from fractions import Fraction
from pathlib import Path

from engine import EngineError
from model import C1, content, evaluate, source_record
from plan import SOURCE, payload_words, to_rql


def write_sources(plan, workdir, records):
    order = [node.name for node in plan.nodes if node.kind == SOURCE]
    for index, name in enumerate(order):
        node = plan.by_name(name)
        lines = []
        for record in range(records):
            lines.append(" ".join(str(value) for value in source_record(index, record, node.width)))
        (Path(workdir) / f"{name}.txt").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return order


def decode_payload(path, width):
    raw = Path(path).read_bytes()
    record_bytes = 4 * width
    if record_bytes == 0 or len(raw) % record_bytes:
        raise EngineError(f"{path}: rozmiar {len(raw)} niezgodny z szerokością {width}")
    values = struct.unpack(f"<{len(raw) // 4}i", raw)
    return [tuple(values[pos:pos + width]) for pos in range(0, len(values), width)]


def decode_meta(path):
    raw = Path(path).read_bytes()
    entries = []
    pos = 8
    while pos < len(raw):
        if pos + 17 > len(raw):
            raise EngineError(f"{path}: ucięty nagłówek wpisu meta przy {pos}")
        is_gap = bool(raw[pos])
        count, bit_count = struct.unpack_from("<QQ", raw, pos + 1)
        byte_count = (bit_count + 7) // 8
        pos += 17
        packed = raw[pos:pos + byte_count]
        pos += byte_count
        bitset = tuple(bool(packed[i // 8] >> (i % 8) & 1) for i in range(bit_count))
        entries.append({"gap": is_gap, "records": count, "null": bitset})
    return entries


def null_flags(entries):
    flags = []
    for entry in entries:
        if entry["gap"]:
            continue
        flags.extend([any(entry["null"])] * entry["records"])
    return flags


def horizon_of(plan, origins, tails, records):
    """Czas scienny, po ktorym KAZDY wezel ma juz `records` rekordow.

    Rekord n jest emitowany w chwili (n+1+W)*Delta, a PIERWSZYM istniejacym jest
    rekord o indeksie `origin` — przed nim rekordow nie ma. Ostatni potrzebny ma
    wiec indeks origin+records-1 i to on wyznacza horyzont.

    `origins` i `tails` sa wielkosciami INDEKSOWYMI i maja pochodzic z MODELU
    ZDARZENIOWEGO (`model.evaluate`), nie z repliki postaci zamknietej: skrypt,
    ktory wymiarowalby przebieg rachunkiem silnika, badalby silnik jego wlasna
    miara. Funkcja ich nie liczy sama wlasnie po to, zeby zrodlo bylo widoczne
    w wywolaniu.

    Do 2026-09-12 tego rachunku nie bylo: przebieg wymiarowala sama ROZPIETOSC
    interwalow (`(records+8)*spread`), a origin do wzoru nie wchodzil. W planie,
    w ktorym `>N` skladaja sie w lancuch, origin narasta (zmierzone: 8 -> 13 ->
    26 -> 34) i budzet konczyl sie, zanim najglebszy wezel doszedl do wlasnego
    origin. Artefakt zostawal pusty — poprawnie, bo przed origin nie ma rekordow
    — a bramka odwzorowania raportowala to jako `zero rekordow`, czyli ROZBIEZNOSC
    TRESCI. Byla to granica aparatury podana jako wynik o silniku; trzy takie
    przypadki zatrzymaly poziom bramki w K24f (patrz jej STOP.md).

    JEDYNA definicja tego rachunku w aparaturze — `run_mapping_gate.py`
    i `check_agse_capacity.py` ja importuja. Nie wolno jej kopiowac: dwa zapisy
    tej samej reguly rozjezdzaja sie po cichu i raz juz to zrobily (naprawa
    z 2026-09-12 trafila najpierw tylko do bramki odwzorowania).
    """
    worst = Fraction(0)
    for node in plan.nodes:
        if node.kind == SOURCE:
            continue
        last = origins[node.name] + records - 1
        worst = max(worst, (Fraction(last) + 1 + tails[node.name]) * node.delta)
    return worst


def wakeup_budget(plan, horizon):
    """Gorne ograniczenie liczby pobudek w czasie `horizon`.

    `-m N` jest budzetem SLOTOW, a slot jest chwila, w ktorej tyka co najmniej
    jeden strumien — nie taktem najszybszego strumienia. Pobudek jest wiec
    najwyzej tyle, ile sumarycznie tykniec wszystkich strumieni w horyzoncie;
    chwile wspolne tylko zmniejszaja te liczbe, wiec suma jest bezpieczna.
    Wlasnosc "suma tykniec >= liczba roznych chwil" sprawdza wprost
    `tests/test_sizing.py`, wyliczajac te chwile.

    Stary wzor mylil te dwie wielkosci i przez to zanizal budzet takze tam,
    gdzie origin byl zerowy: plan wielotaktowy ma WIECEJ pobudek niz taktow
    najszybszego strumienia.
    """
    return sum(int(horizon / node.delta) + 1 for node in plan.nodes)


def run_plan(plan, binary, workdir, loops, records=512, timeout=120):
    workdir = Path(workdir)
    workdir.mkdir(parents=True, exist_ok=True)
    for stale in workdir.iterdir():
        if stale.is_file():
            stale.unlink()
    write_sources(plan, workdir, records)
    rql = to_rql(plan)
    (workdir / "query.rql").write_text(rql, encoding="utf-8")

    env = dict(os.environ, SPDLOG_LEVEL="warn")
    done = subprocess.run([str(binary), "query.rql", "-r", "-k", "-m", str(loops)],
                          cwd=workdir, capture_output=True, text=True, timeout=timeout, env=env)
    if done.returncode != 0:
        raise EngineError(f"wykonanie nieudane ({done.returncode}):\n{done.stdout}\n{done.stderr}")
    return rql


def compare_content(plan, workdir, limit=24):
    """Zwraca listę rozbieżności treści (pusta = bramka przeszła).

    Pozycja w artefakcie jest indeksem FIZYCZNYM. Strumień o niezerowym
    początku logicznym nie ma rekordów przed origin, więc jego rekord fizyczny
    0 nosi indeks logiczny równy origin — i to pod tym indeksem trzeba pytać
    model o treść. Porównywanie pozycji z pozycją dawałoby fałszywe
    rozbieżności dla każdego planu z `@` albo `>N`.
    """
    workdir = Path(workdir)
    findings = []
    origins = {item.name: item.origin for item in evaluate(plan, convention=C1)}
    for node in plan.nodes:
        if node.kind == SOURCE:
            continue
        payload = workdir / node.name
        if not payload.exists():
            findings.append({"node": node.name, "kind": node.kind, "issue": "brak artefaktu"})
            continue
        emitted = decode_payload(payload, payload_words(node))
        meta = workdir / f"{node.name}.meta"
        nulls = null_flags(decode_meta(meta)) if meta.exists() else []
        checked = min(limit, len(emitted))
        if checked == 0:
            findings.append({"node": node.name, "kind": node.kind, "issue": "zero rekordów"})
            continue
        for index in range(checked):
            logical = origins[node.name] + index
            expected = content(plan, node.name, logical)
            if index < len(nulls) and nulls[index]:
                findings.append({"node": node.name, "kind": node.kind, "issue": "rekord z NULL",
                                 "index": index, "logical": logical,
                                 "expected": expected, "got": emitted[index]})
                break
            if emitted[index] != expected:
                findings.append({"node": node.name, "kind": node.kind, "issue": "inna treść",
                                 "index": index, "logical": logical,
                                 "expected": expected, "got": emitted[index]})
                break
    return findings
