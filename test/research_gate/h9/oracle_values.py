#!/usr/bin/env python3
"""Wspolny oracle wartosci K26 / H9 — bramka poprawnosci P6 (predeklaracja kampanii K26v3 §7.1).

Co ten plik jest, a czego nie jest
----------------------------------
§7.1 zada oracle'a na co najmniej 2000 publicznych rekordach KAZDEGO nazwanego
wyniku po ogonie, a porownanie ma obejmowac: liczbe i nazwy wynikow, deskryptory,
kolejnosc, wartosci, `NULL`, luki oraz brak rekordow ogona. Dla RetractorDB
dodatkowo identycznosc publicznych artefaktow MIEDZY PROFILAMI. To jest ten
oracle. Nie mierzy zadnego kosztu — czyta wylacznie artefakty publiczne.

Dlaczego oracle NIE ma wlasnej implementacji semantyki
-----------------------------------------------------
Gdyby oracle liczyl `Sqrt(A*A+B*B)` po swojemu, sprawdzalby zgodnosc dwoch
przepisan tej samej specyfikacji — a projekt ma juz przypadek, w ktorym dwa
przepisania zgadzaly sie ze soba i OBA rozjezdzaly z kodem (§6 predeklaracji,
serializer kanoniczny). Dlatego oracle jest ROZJEMCA miedzy dwoma niezaleznymi
wykonaniami: RetractorDB (cztery profile) i Flink (dwa warianty). Rozbieznosc
jest faktem do sklasyfikowania przez czlowieka (STOP-6), a nie do rozstrzygniecia
przez trzecia implementacje.

Mapa NULL/luk pochodzi z `xtrdb`, czyli z CZYTNIKA SILNIKA, a nie z wlasnego
parsera `.meta` — z tego samego powodu.

Warunki, w ustalonej kolejnosci
-------------------------------
Kolejnosc jest czescia bramki, nie kosmetyka. Regula tego luku brzmi: przypadek
odrzucany musi DOJSC do warunku, ktory ma go zlapac. Bramka zatrzymujaca sie na
warunku wczesniejszym niczego nie dowodzi, wiec `--mutants` sprawdza dla kazdego
mutanta OBIE rzeczy: ze zadzialal warunek zamierzony i ze wszystkie warunki przed
nim przeszly.

  C1 results_count_names  liczba i nazwy nazwanych wynikow
  C2 descriptor           deskryptor: liczba pol, typy, kolejnosc, szerokosc
  C3 record_count_tail    liczby rekordow i zadeklarowane wyrownanie ogona
  C4 order_values         kolejnosc i wartosci rekordow na wspolnym oknie
  C5 null_gap_map         mapa NULL i luk (czytnik silnika)
  C6 window_size          okno porownania ma >= 2000 rekordow

C6 jest ostatni celowo: gdyby stal pierwszy, korpus z 5 rekordami konczylby sie
komunikatem o za malym oknie zamiast pokazac, ze wartosci sie rozjechaly.
"""

import argparse
import os
import re
import struct
import subprocess
import sys

MIN_WINDOW = 2000           # §7.1 — twarde, nie parametr
XTRDB = os.environ.get("XTRDB", os.path.expanduser("~/.local/bin/xtrdb"))

CONDITIONS = ["results_count_names", "descriptor", "record_count_tail",
              "order_values", "null_gap_map", "window_size"]


class Mismatch(Exception):
    """Rozbieznosc wykryta przez oracle. `condition` mowi, KTORY warunek zadzialal."""

    def __init__(self, condition, detail):
        super().__init__(f"[{condition}] {detail}")
        self.condition = condition
        self.detail = detail


# ── Czytanie artefaktow ──────────────────────────────────────────────────────

TYPE_WIDTH = {"INTEGER": 4, "DOUBLE": 8, "FLOAT": 4, "BYTE": 1, "RATIONAL": 16,
              "INTPAIR": 16, "NULLTYPE": 0}


def parse_descriptor(path):
    """Deskryptor `.desc` -> [(typ, nazwa_pola)] plus pozycje konfiguracyjne.

    Nazwy pol niosa nazwe strumienia (`m1_0`), wiec do porownania miedzy roznymi
    wynikami idzie SUFIKS pozycji, a nie cala nazwa. Typ i kolejnosc ida w calosci.
    """
    fields, config = [], []
    for line in open(path, encoding="utf-8", errors="replace").read().splitlines():
        line = line.strip().lstrip("{").rstrip("}").strip()
        if not line:
            continue
        parts = line.split()
        if parts[0] in TYPE_WIDTH:
            fields.append((parts[0], parts[1] if len(parts) > 1 else ""))
        else:
            config.append(line)
    return fields, config


def record_width(fields):
    return sum(TYPE_WIDTH[t] for t, _ in fields)


def read_rdb_stream(storage_dir, name):
    """Publiczny artefakt RetractorDB -> dict."""
    desc_path = os.path.join(storage_dir, name + ".desc")
    data_path = os.path.join(storage_dir, name)
    fields, config = parse_descriptor(desc_path)
    width = record_width(fields)
    raw = open(data_path, "rb").read()
    if width == 0 or len(raw) % width:
        raise Mismatch("descriptor",
                       f"{name}: {len(raw)} B nie dzieli sie przez szerokosc rekordu {width} B")
    count = len(raw) // width
    records = []
    for i in range(count):
        chunk = raw[i * width:(i + 1) * width]
        offset, row = 0, []
        for typ, _ in fields:
            w = TYPE_WIDTH[typ]
            if typ == "INTEGER":
                row.append(struct.unpack_from("<i", chunk, offset)[0])
            elif typ == "DOUBLE":
                row.append(struct.unpack_from("<d", chunk, offset)[0])
            elif typ == "FLOAT":
                row.append(struct.unpack_from("<f", chunk, offset)[0])
            elif typ == "BYTE":
                row.append(chunk[offset])
            else:
                row.append(chunk[offset:offset + w])
            offset += w
        records.append(tuple(row))
    return {"name": name, "fields": fields, "config": config, "width": width,
            "records": records, "nullmap": read_nullmap(data_path)}


def read_nullmap(data_path):
    """Mapa NULL/luk oczami CZYTNIKA SILNIKA (`xtrdb`), nie wlasnego parsera.

    Z rysunku bierzemy wylacznie wiersze niosace tresc: liczbe segmentow, liczbe
    rekordow i znaczniki `data/partial null/nullfill/gap`. Ramka i sciezki sa
    odrzucane, bo sciezka rozni sie miedzy profilami z natury rzeczy.
    """
    out = subprocess.run([XTRDB, "-n", "-s", data_path], capture_output=True, text=True)
    if out.returncode != 0:
        raise Mismatch("null_gap_map", f"xtrdb nie odczytal {data_path}: {out.stderr.strip()}")
    keep = []
    for line in out.stdout.splitlines():
        body = line.strip("│┌┐└┘├┤ \t")
        if re.search(r"(Segments|records|no nulls|nullfill|gap|partial)", body):
            if ("Legend" in body or "DESCRIPTOR" in body or "DATA" in body or "META" in body
                    or "[~~~~]" in body or "[XXXX]" in body):
                continue
            keep.append(re.sub(r"\s+", " ", body))
    return keep


def read_flink_csv(path):
    """Sink Flinka: `monitor,slot,wartosc` -> lista (slot, (wartosc,)) w kolejnosci slotu."""
    rows = []
    for line in open(path, encoding="utf-8"):
        line = line.strip()
        if not line:
            continue
        parts = line.split(",")
        rows.append((int(parts[1]), (int(parts[2]),)))
    rows.sort(key=lambda r: r[0])
    return rows


# ── Warunki ──────────────────────────────────────────────────────────────────

def check_results(left_names, right_names, left_tag, right_tag):
    """C1 — liczba i nazwy nazwanych wynikow."""
    if sorted(left_names) != sorted(right_names):
        only_l = sorted(set(left_names) - set(right_names))
        only_r = sorted(set(right_names) - set(left_names))
        raise Mismatch("results_count_names",
                       f"{left_tag} ma {len(left_names)} wynikow, {right_tag} ma {len(right_names)}; "
                       f"tylko w {left_tag}: {only_l}; tylko w {right_tag}: {only_r}")


def check_descriptor(left, right, tag):
    """C2 — deskryptor: liczba pol, typy, KOLEJNOSC pol, szerokosc rekordu."""
    lt = [t for t, _ in left["fields"]]
    rt = [t for t, _ in right["fields"]]
    if lt != rt:
        raise Mismatch("descriptor", f"{tag}: typy/kolejnosc pol {lt} wobec {rt}")
    lp = [n.rsplit("_", 1)[-1] for _, n in left["fields"]]
    rp = [n.rsplit("_", 1)[-1] for _, n in right["fields"]]
    if lp != rp:
        raise Mismatch("descriptor", f"{tag}: kolejnosc pozycji pol {lp} wobec {rp}")
    if left["width"] != right["width"]:
        raise Mismatch("descriptor", f"{tag}: szerokosc rekordu {left['width']} wobec {right['width']} B")


def check_counts(left_n, right_n, tail_allowance, tag):
    """C3 — liczby rekordow i ZADEKLAROWANE wyrownanie ogona.

    `tail_allowance` to DOKLADNA liczba rekordow, o ktore prawa strona ma byc
    dluzsza — glebokosc potoku, ktora lewa strona traci na ogonie. Warunek jest
    rownoscia, nie nierownoscia: `<=` przepuscilby wersje, ktorej brakuje rekordow,
    a bramka, ktora nie odroznia wersji obalonej, nie jest bramka.
    """
    delta = right_n - left_n
    if delta != tail_allowance:
        raise Mismatch("record_count_tail",
                       f"{tag}: {left_n} wobec {right_n} rekordow, roznica {delta}, "
                       f"zadeklarowany ogon {tail_allowance}")


def check_values(left_records, right_records, tag):
    """C4 — kolejnosc i wartosci na wspolnym oknie."""
    n = min(len(left_records), len(right_records))
    for i in range(n):
        if left_records[i] != right_records[i]:
            raise Mismatch("order_values",
                           f"{tag}: rekord {i}: {left_records[i]} wobec {right_records[i]}")
    return n


def check_nullmap(left, right, tag):
    """C5 — mapa NULL i luk."""
    if left["nullmap"] != right["nullmap"]:
        raise Mismatch("null_gap_map", f"{tag}: {left['nullmap']} wobec {right['nullmap']}")


def check_window(n, tag):
    """C6 — okno porownania >= 2000 rekordow (§7.1)."""
    if n < MIN_WINDOW:
        raise Mismatch("window_size", f"{tag}: okno {n} rekordow, wymagane >= {MIN_WINDOW}")


# ── Porownania zlozone ───────────────────────────────────────────────────────

def compare_rdb_pair(a, b, tag, tail_allowance=0):
    """Dwa artefakty RetractorDB — komplet warunkow w ustalonej kolejnosci."""
    check_descriptor(a, b, tag)
    check_counts(len(a["records"]), len(b["records"]), tail_allowance, tag)
    n = check_values(a["records"], b["records"], tag)
    check_nullmap(a, b, tag)
    check_window(n, tag)
    return n


def compare_rdb_observable(optimized, baseline, tag):
    """R1-aware public identity: Val equal and Lat(optimized) <= Lat(baseline).

    Both executions receive the same finite sources.  A shorter causal tail is
    therefore visible as an equal value sequence with zero or more additional
    records at the end of the optimized run.  Losing a record, changing a
    common-prefix value, changing the descriptor, or producing NULL/gap data is
    a failure.  K26 sources contain no NULL values, so requiring the no-NULL/no-
    gap control on both sides is stronger and less ambiguous than normalizing
    xtrdb summaries with different record counts.
    """
    check_descriptor(optimized, baseline, tag)
    optimized_n = len(optimized["records"])
    baseline_n = len(baseline["records"])
    if optimized_n < baseline_n:
        raise Mismatch(
            "record_count_tail",
            f"{tag}: optimized plan has {optimized_n} records, baseline {baseline_n}; "
            "required Lat(optimized) <= Lat(baseline)",
        )
    n = check_values(optimized["records"], baseline["records"], tag)
    for side, stream in (("optimized", optimized), ("baseline", baseline)):
        bad = [
            line for line in stream["nullmap"]
            if re.search(r"partial null|nullfill|\bgap\b", line, re.IGNORECASE)
        ]
        if bad or not any("no nulls" in line.lower() for line in stream["nullmap"]):
            raise Mismatch("null_gap_map", f"{tag}: {side} is not the no-NULL/no-gap control: {stream['nullmap']}")
    check_window(n, tag)
    return n


def evaluate_all(a, b, tag, tail_allowance=0):
    """Ocenia WSZYSTKIE warunki po kolei i zwraca (pierwszy_ktory_padl, [zdane_przed]).

    Zwyklemu porownaniu wystarczy pierwsza rozbieznosc, ale bramce mutantow nie:
    musi pokazac, ze przypadek odrzucany DOSZEDL do warunku, ktory ma go zlapac.
    Dlatego ta funkcja zwraca takze liste warunkow zdanych PRZED tym, ktory padl.
    """
    passed = []
    steps = [
        ("descriptor", lambda: check_descriptor(a, b, tag)),
        ("record_count_tail", lambda: check_counts(len(a["records"]), len(b["records"]),
                                                   tail_allowance, tag)),
        ("order_values", lambda: check_values(a["records"], b["records"], tag)),
        ("null_gap_map", lambda: check_nullmap(a, b, tag)),
        ("window_size", lambda: check_window(min(len(a["records"]), len(b["records"])), tag)),
    ]
    for name, step in steps:
        try:
            step()
        except Mismatch as exc:
            return exc, passed
        passed.append(name)
    return None, passed


def compare_rdb_vs_flink(rdb, flink_rows, tag, tail_allowance):
    """RetractorDB wobec Flinka. Flink niesie slot jawnie, wiec sprawdzamy takze,
    ze sloty sa ciagle — inaczej `kolejnosc` bylaby sprawdzona tylko po jednej
    stronie."""
    slots = [s for s, _ in flink_rows]
    # Sloty maja byc CIAGLE; ich POCZATEK moze byc przesuniety, bo wariant `manual`
    # stosuje `>3` na strumieniu przeplecionym i numeruje od 3. Przesuniecie etykiety
    # nie jest roznica semantyczna — dowodem jest to, ze `natural` i `manual` daja
    # identyczne CIAGI WARTOSCI. Roznica w ciaglosci bylaby luka i musi byc zlapana.
    if slots and slots != list(range(slots[0], slots[0] + len(slots))):
        raise Mismatch("order_values", f"{tag}: sloty Flinka nie sa ciagle "
                                       f"(od {slots[0]} do {slots[-1]}, {len(slots)} rekordow)")
    if len(rdb["fields"]) != 1:
        raise Mismatch("descriptor",
                       f"{tag}: strona Flinka niesie jedno pole, RetractorDB {len(rdb['fields'])}")
    check_counts(len(rdb["records"]), len(flink_rows), tail_allowance, tag)
    n = check_values(rdb["records"], [v for _, v in flink_rows], tag)
    check_window(n, tag)
    return n


# ── Mutanty: bramka wlasna oracle'a ──────────────────────────────────────────
#
# §7.1: "Dla kazdego mechanizmu co najmniej trzy mutanty: zmieniona faza/shift,
# kolejnosc pola, mapa NULL/luka — oracle ma wykryc wszystkie."
#
# Regula tego luku dokłada do tego drugi warunek, twardszy: NAJPIERW pokaz, ze
# oracle ODRZUCA mutanty, dopiero potem, ze akceptuje wariant poprawny; i pokaz,
# ze przypadek odrzucany DOCHODZI do warunku, ktory ma go zlapac. Mutant zlapany
# przez warunek wczesniejszy niz zamierzony NIE JEST dowodem, ze zamierzony
# warunek dziala — dlatego kazdy mutant ma tu pole `expects` i bramka sprawdza
# rownosc, nie samo "cokolwiek padlo".
#
# Mutanty sa liczone na KOPII artefaktu. Zamrozonych plikow nie dotykaja.

import shutil

META_OFFSET_GAP = 8         # znacznik isGap wpisu indeksu (potwierdzony empirycznie)
META_OFFSET_NULLBITS = 25   # wzorzec null wpisu indeksu


def _copy_artifact(src_dir, dst_dir, name):
    os.makedirs(dst_dir, exist_ok=True)
    for suffix in ("", ".desc", ".meta", ".shadow"):
        src = os.path.join(src_dir, name + suffix)
        if os.path.exists(src):
            shutil.copy2(src, os.path.join(dst_dir, name + suffix))


def mutate_phase_shift(art_dir, name):
    """Zmieniona faza: ciag rekordow przesuniety o jeden slot, DLUGOSC ZACHOWANA.

    Zachowanie dlugosci jest istotne — mutant ma dojsc do warunku wartosci,
    a nie zostac zatrzymany wczesniej na liczbie rekordow.
    """
    path = os.path.join(art_dir, name)
    fields, _ = parse_descriptor(path + ".desc")
    width = record_width(fields)
    raw = open(path, "rb").read()
    shifted = raw[width:] + raw[-width:]
    open(path, "wb").write(shifted)


def mutate_field_order(art_dir, name):
    """Kolejnosc pola. Przy wielu polach — prawdziwa zamiana dwoch pierwszych pol
    w deskryptorze i w bajtach rekordu. Przy jednym polu zamiana jest niewyrazalna,
    wiec mutacja dotyka POZYCJI pola (`m1_0` -> `m1_1`): wynik deklaruje, ze niesie
    inna pozycje rekordu zrodlowego niz naprawde niesie."""
    desc_path = os.path.join(art_dir, name + ".desc")
    text = open(desc_path, encoding="utf-8").read()
    fields, _ = parse_descriptor(desc_path)
    if len(fields) >= 2:
        (t0, n0), (t1, n1) = fields[0], fields[1]
        first = f"{t0}\t{n0}" if f"{t0}\t{n0}" in text else f"{t0} {n0}"
        second = f"{t1}\t{n1}" if f"{t1}\t{n1}" in text else f"{t1} {n1}"
        text = text.replace(first, "\x00PLACEHOLDER\x00").replace(second, first)
        text = text.replace("\x00PLACEHOLDER\x00", second)
        open(desc_path, "w", encoding="utf-8").write(text)
        return
    typ, fname = fields[0]
    base, _, position = fname.rpartition("_")
    open(desc_path, "w", encoding="utf-8").write(
        text.replace(fname, f"{base}_{int(position) + 1}"))


def mutate_null_map(art_dir, name):
    """Mapa NULL: wzorzec wpisu indeksu ustawiony na `all nulls`. Plik danych
    i deskryptor NIETKNIETE, wiec mutant dochodzi do warunku mapy."""
    path = os.path.join(art_dir, name + ".meta")
    data = bytearray(open(path, "rb").read())
    data[META_OFFSET_NULLBITS] = 1
    open(path, "wb").write(data)


def mutate_gap(art_dir, name):
    """Luka: wpis indeksu oznaczony jako przerwa transmisji."""
    path = os.path.join(art_dir, name + ".meta")
    data = bytearray(open(path, "rb").read())
    data[META_OFFSET_GAP] = 1
    open(path, "wb").write(data)


def mutate_missing_tail(art_dir, name):
    """Brak rekordu ogona — dowod, ze warunek liczby rekordow nie jest pusty."""
    path = os.path.join(art_dir, name)
    fields, _ = parse_descriptor(path + ".desc")
    raw = open(path, "rb").read()
    open(path, "wb").write(raw[:-record_width(fields)])


MUTANTS = [
    ("faza_shift", "order_values", mutate_phase_shift),
    ("kolejnosc_pola", "descriptor", mutate_field_order),
    ("mapa_null", "null_gap_map", mutate_null_map),
    ("luka", "null_gap_map", mutate_gap),
    ("brak_rekordu_ogona", "record_count_tail", mutate_missing_tail),
]


def run_mutant_gate(cases, work_dir, report):
    """Dla kazdego (mechanizm, katalog, nazwa) uruchamia komplet mutantow.

    Zwraca liczbe zdanych. Kazdy mutant musi (a) zostac odrzucony, (b) przez
    warunek ZAMIERZONY, (c) po przejsciu wszystkich warunkow wczesniejszych.
    """
    ok = 0
    for mechanism, src_dir, name in cases:
        original = read_rdb_stream(src_dir, name)
        report(f"-- mechanizm {mechanism}: {name} ({len(original['records'])} rekordow)")
        for label, expects, mutate in MUTANTS:
            dst = os.path.join(work_dir, f"{mechanism}_{label}")
            shutil.rmtree(dst, ignore_errors=True)
            _copy_artifact(src_dir, dst, name)
            mutate(dst, name)
            try:
                mutated = read_rdb_stream(dst, name)
            except Mismatch as exc:
                failure, passed = exc, []
            else:
                failure, passed = evaluate_all(original, mutated, f"{mechanism}/{label}")
            if failure is None:
                raise SystemExit(
                    f"BLAD BRAMKI: mutant '{label}' mechanizmu {mechanism} PRZESZEDL. "
                    f"Oracle, ktory nie wykrywa wlasnych mutantow, nie jest oracle'em.")
            if failure.condition != expects:
                raise SystemExit(
                    f"BLAD BRAMKI: mutant '{label}' mechanizmu {mechanism} zostal odrzucony przez "
                    f"warunek '{failure.condition}', a mial przez '{expects}'. Bramka zatrzymujaca "
                    f"sie na warunku wczesniejszym niczego o warunku zamierzonym nie dowodzi.")
            expected_before = CONDITIONS[1:CONDITIONS.index(expects)]
            if passed != expected_before:
                raise SystemExit(
                    f"BLAD BRAMKI: mutant '{label}' mechanizmu {mechanism} NIE DOSZEDL do warunku "
                    f"'{expects}' — zdane przed nim: {passed}, oczekiwano {expected_before}.")
            report(f"   ok  mutant {label:<20} odrzucony przez '{failure.condition}' "
                   f"po zdanych {passed}")
            ok += 1
    return ok
