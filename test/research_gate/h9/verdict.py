#!/usr/bin/env python3
"""Skrypt werdyktu K26 / H9 — liczy progi sam i wydaje werdykt bez interpretacji.

Pozycja w protokole
-------------------
§10 („Zamrożenie i produkt") wymienia wykonywalny skrypt werdyktu wśród pozycji
zamrażanych PRZED pomiarem, a §5 Krok 8 mówi: „werdykt wydany przez skrypt, nie
przez interpretację". Ten plik jest tą pozycją. Progi, ablacje minimalne, siatka
`Q`, komórka rozstrzygająca i reguła 2/3 są tu STAŁYMI, nie parametrami — nie da
się ich podać z wiersza poleceń, bo wtedy zamrożenie niczego by nie znaczyło.

Wejście (katalog `--matrix`)
----------------------------
`mechanism.tsv`  — wielkości DETERMINISTYCZNE, jeden wiersz na (rodzina, system,
                   profil, Q): instancje, `STREAM_SELECT_*`, substraty, r1, r2,
                   bajty substratów, publiczne dopisania, liczniki pracy.
`timing.tsv`     — wielkość ZMIENNA, jeden wiersz na (rodzina, profil, Q, blok):
                   `compute_median_ns`, `compute_p99_ns`, `slot_ns`,
                   `lost_records`.
`gates.tsv`      — bramki poprawności/mechanizmu: (rodzina, bramka, status,
                   klasyfikacja). Klasyfikacja rozstrzyga, czy porażka bramki
                   liczy się PRZECIW H9 (`engine_or_profile`), czy unieważnia
                   iterację bez werdyktu (`apparatus` albo `corpus`).

Kody wyjścia
------------
0  H9 WSPARTA        (co najmniej 2/3 ważnych rodzin przechodzi komplet progów)
1  H9 BEZ WSPARCIA   (rodziny ważne, progi nieosiągnięte)
2  BRAK WERDYKTU     (iteracja technicznie nieważna albo niepełna — §10 zabrania
                      wtedy wydawania werdyktu; powtórzyć w NOWYM katalogu)

Bramka własna
-------------
`--selftest` uruchamia skrypt na sztucznych danych o ZNANEJ odpowiedzi, w tym na
wersjach celowo obalonych. Skrypt werdyktu jest aparaturą i podlega tej samej
regule co reszta: musi umieć odróżnić wersję obaloną. W tym projekcie bramka
niezdolna do tego zawiodła już czterokrotnie.
"""
import argparse
import sys
import tempfile
import traceback
from fractions import Fraction
from pathlib import Path

# ══════════════════════════════════════════════════════════════════════════════
#  ZAMROŻONE STAŁE — predeklaracji kampanii K26v3 §7. Zmiana którejkolwiek wymaga NOWEJ
#  predeklaracji i nowego katalogu wyników (§10). Nie są parametrami CLI.
# ══════════════════════════════════════════════════════════════════════════════

FAMILIES = ["F9-R2", "F9-R1", "F9-X"]
Q_GRID = [1, 2, 4, 8, 16, 32]

#: Komórka rozstrzygająca. `Q=1,2,4` są kontrolą trendu, `Q=16,32` pomiarem
#: skalowania — NIE dodatkowymi szansami na zaliczenie progu (§10).
DECISIVE_Q = 8

#: Ablacja minimalna rodziny — profil różniący się od `DEFAULT` DOKŁADNIE jednym
#: przełącznikiem badanego mechanizmu (dla F9-X komórka kontrolna układu 2×2).
MINIMAL_ABLATION = {"F9-R2": "NO_R2_CANON", "F9-R1": "NO_R1_FACTOR", "F9-X": "NO_R1_NO_R2"}

#: Kontrola pusta rodziny: profil, który nie ma czego dopasować i MUSI dać liczby
#: identyczne z `DEFAULT`. Różnica dowodzi, że plan nie izoluje mechanizmu.
EMPTY_CONTROL = {"F9-R2": "NO_R1_FACTOR", "F9-R1": "NO_R2_CANON", "F9-X": None}

#: Próg metryki pierwotnej: `DEFAULT` musi zmniejszyć ją o co najmniej tyle
#: wobec ablacji minimalnej ORAZ wobec `FLINK_NATURAL` (§10, próg H9 pkt 2).
THRESHOLD_REDUCTION = Fraction(40, 100)

#: Bramka ceny czasowej: górna granica 95% CI ilorazu `DEFAULT/minimal_ablation`.
THRESHOLD_TIME_CI_UPPER = Fraction(105, 100)

#: Reguła 2/3 (§10). Rodzina ważna, która nie przechodzi, liczy się PRZECIW H9.
FAMILIES_REQUIRED = 2

#: Bootstrap: dwustronny, sparowany po blokach, percentylowy.
BOOTSTRAP_BLOCKS = 20
BOOTSTRAP_REPLICATIONS = 10000
BOOTSTRAP_SEED = 20260809_2603
CI_LOWER_PCT, CI_UPPER_PCT = 2.5, 97.5

#: Przekroczenie budżetu slotu = STOP-8: zatrzymanie CAŁEJ rodziny bez werdyktu.
#: Komórka NIE jest po cichu wykluczana (§10).
SLOT_BUDGET_STOP = Fraction(80, 100)

#: Która wielkość pracy ROZDZIELA w danej rodzinie. Rozstrzygnięte osobno per
#: rodzina, bo w F9-R1 program pola daje 0,0% — współdzielenie dotyczy tam
#: przeplotu, nie arytmetyki (PLANY_FLINKA.md §4.3). Jedna reguła na trzy różne
#: mechanizmy dawałaby fałszywy wynik zerowy w F9-R1.
RESOLVING_WORK = {"F9-R2": "work_costly_evals", "F9-R1": "work_hash_picks", "F9-X": "work_costly_evals"}

#: Predeklarowana krzywa redukcji WEWNĘTRZNEJ (wobec ablacji minimalnej).
#: Nasyca się na `1 − 1/F` — NIE rośnie z `Q` powyżej progu postaci. Skrypt nie
#: ma prawa oczekiwać po tej stronie trendu rosnącego (SZKIC_RODZIN.md §3.3).
PREDECLARED_RDB = {
    "F9-R2": {1: Fraction(0), 2: Fraction(0), 4: Fraction(1, 2), 8: Fraction(1, 2),
              16: Fraction(1, 2), 32: Fraction(1, 2)},
    "F9-R1": {1: Fraction(0), 2: Fraction(0), 4: Fraction(1, 2), 8: Fraction(1, 2),
              16: Fraction(1, 2), 32: Fraction(1, 2)},
    "F9-X": {1: Fraction(0), 2: Fraction(0), 4: Fraction(1, 2), 8: Fraction(7, 12),
             16: Fraction(7, 12), 32: Fraction(7, 12)},
}

#: Predeklarowana krzywa redukcji wobec `FLINK_NATURAL` — liniowa w `Q`, więc
#: `1 − 1/Q` dla rodzin jednowęzłowych i `1 − 5/(4Q)` dla F9-X (pięć węzłów
#: wspólnego podplanu). PRZY `Q=1` W F9-X JEST UJEMNA (−25%) i tak ma być:
#: `FLINK_NATURAL` ma tam 4 jednostki wobec 5 w `DEFAULT` (PLANY_FLINKA.md §4.2).
PREDECLARED_FLINK = {
    "F9-R2": {q: Fraction(q - 1, q) for q in Q_GRID},
    "F9-R1": {q: Fraction(q - 1, q) for q in Q_GRID},
    "F9-X": {q: 1 - Fraction(5, 4 * q) for q in Q_GRID},
}

#: Tolerancja porównania z krzywą predeklarowaną. Metryka bajtowa jest
#: deterministyczna, więc tolerancja jest tu na wypadek zaokrągleń licznika,
#: a nie na rozrzut. Do PROGU 40% stosuje się porównanie ŚCISŁE, bez tolerancji.
CURVE_TOLERANCE = Fraction(5, 1000)

#: Tolerancja kontroli negatywnej przy `Q=1` (§7.3). Metryka jest ilorazem
#: ZMIERZONYCH bajtów na rekord publiczny, a profile kończą się różną długością
#: ogona, więc dokładne zero nie jest na niej osiągalne. W K26v2 ścisłe
#: porównanie `!= 0` odwróciło werdykt rodziny F9-X przy reszcie 4,9·10⁻⁸,
#: wytworzonej przez różnicę dwóch rekordów publicznych (D8).
CONTROL_TOLERANCE = CURVE_TOLERANCE

#: Bramki, które muszą być obecne w `gates.tsv` dla każdej rodziny. Brak wpisu to
#: iteracja NIEPEŁNA, nie bramka zdana.
REQUIRED_GATES = [
    "corpus_validity",      # exact 21 plans, pinned SHA, 84/84 compile and 4/4 reject evidence
    "oracle_values",        # ≥2000 publicznych rekordów każdego nazwanego wyniku
    "oracle_mutants",       # ≥3 mutanty na mechanizm: faza/shift, kolejność pola, NULL/luka
    "counter_known_answer", # ≥30 przypadków o znanej odpowiedzi na instrumencie
    "public_identity",      # identyczność publicznych artefaktów między profilami
    "near_miss_controls",   # kontrole nierównoważności nie zostały scalone
    "no_materialization",   # program bez etapu pośredniego: zerowe bajty substratów
]

MECHANISM_COLUMNS = [
    "family", "system", "profile", "q", "instances", "stream_selects", "substrates",
    "r1", "r2", "substrate_bytes", "public_appends",
    "work_costly_evals", "work_hash_picks", "work_add_merges",
]
TIMING_COLUMNS = ["family", "profile", "q", "block", "compute_median_ns", "compute_p99_ns",
                  "slot_ns", "lost_records"]
GATE_COLUMNS = ["family", "gate", "status", "classification"]
INVALIDATING_CLASSIFICATIONS = {"apparatus", "corpus"}


# ══════════════════════════════════════════════════════════════════════════════
#  Narzędzia liczbowe — bez zależności zewnętrznych, żeby werdykt nie zależał od
#  wersji biblioteki zainstalowanej na maszynie liczącej.
# ══════════════════════════════════════════════════════════════════════════════

def splitmix64(state):
    state = (state + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    z = state
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    return state, z ^ (z >> 31)


def median(values):
    ordered = sorted(values)
    n = len(ordered)
    if n == 0:
        raise ValueError("mediana z pustego zbioru")
    mid = n // 2
    if n % 2:
        return Fraction(ordered[mid])
    return (Fraction(ordered[mid - 1]) + Fraction(ordered[mid])) / 2


def percentile(ordered, pct):
    """Percentyl metodą najbliższej rangi na posortowanej liście."""
    if not ordered:
        raise ValueError("percentyl z pustego zbioru")
    rank = max(1, min(len(ordered), int(round(pct / 100.0 * len(ordered) + 0.5))))
    return ordered[rank - 1]


def paired_bootstrap_ratio(default_by_block, ablation_by_block):
    """Dwustronny sparowany bootstrap ilorazu median.

    Statystyka: `median_bloków(DEFAULT) / median_bloków(ablacja)`.
    Sparowanie: replikacja losuje INDEKSY BLOKÓW, a nie wartości osobno dla
    każdego profilu — bloki są jednostką losowania, bo profile biegły w nich
    razem, na tej samej maszynie i w tej samej kolejności z zamrożonego ziarna.
    """
    blocks = sorted(default_by_block)
    n = len(blocks)
    point = median([default_by_block[b] for b in blocks]) / median([ablation_by_block[b] for b in blocks])

    state, ratios = BOOTSTRAP_SEED, []
    for _ in range(BOOTSTRAP_REPLICATIONS):
        picked = []
        for _ in range(n):
            state, value = splitmix64(state)
            picked.append(blocks[value % n])
        num = median([default_by_block[b] for b in picked])
        den = median([ablation_by_block[b] for b in picked])
        ratios.append(num / den)
    ratios.sort()
    return point, percentile(ratios, CI_LOWER_PCT), percentile(ratios, CI_UPPER_PCT)


# ══════════════════════════════════════════════════════════════════════════════
#  Wejście
# ══════════════════════════════════════════════════════════════════════════════

def read_tsv(path, columns):
    if not path.exists():
        raise FileNotFoundError(f"brak pliku wejsciowego: {path}")
    rows, header = [], None
    for raw in path.read_text().splitlines():
        if not raw.strip() or raw.startswith("#"):
            continue
        fields = raw.split("\t")
        if header is None:
            header = fields
            if header != columns:
                raise ValueError(f"{path.name}: naglowek {header}, oczekiwano {columns}")
            continue
        if len(fields) != len(columns):
            raise ValueError(f"{path.name}: wiersz o {len(fields)} polach, oczekiwano {len(columns)}")
        rows.append(dict(zip(columns, fields)))
    if header is None:
        raise ValueError(f"{path.name}: pusty plik")
    return rows


class Problem(Exception):
    """Iteracja technicznie nieważna albo niepełna — werdyktu nie wolno wydać."""


def load(matrix_dir):
    mechanism = read_tsv(matrix_dir / "mechanism.tsv", MECHANISM_COLUMNS)
    timing = read_tsv(matrix_dir / "timing.tsv", TIMING_COLUMNS)
    gates = read_tsv(matrix_dir / "gates.tsv", GATE_COLUMNS)

    mech = {}
    for row in mechanism:
        key = (row["family"], row["system"], row["profile"], int(row["q"]))
        if key in mech:
            raise ValueError(f"mechanism.tsv: zdublowany wiersz {key}")
        mech[key] = {k: (int(v) if k not in ("family", "system", "profile") else v)
                     for k, v in row.items()}
    time = {}
    for row in timing:
        key = (row["family"], row["profile"], int(row["q"]))
        time.setdefault(key, {})[int(row["block"])] = {
            "median": int(row["compute_median_ns"]),
            "p99": int(row["compute_p99_ns"]),
            "slot": int(row["slot_ns"]),
            "lost": int(row["lost_records"]),
        }
    gate = {}
    for row in gates:
        gate.setdefault(row["family"], {})[row["gate"]] = (row["status"], row["classification"])
    return mech, time, gate


# ══════════════════════════════════════════════════════════════════════════════
#  Ocena rodziny
# ══════════════════════════════════════════════════════════════════════════════

def metric(cell):
    """Metryka pierwotna: logiczne bajty zapisów do badanego podplanu na jeden
    publiczny rekord wyjściowy. Mianownik MUSI być niezerowy (§10)."""
    if cell["public_appends"] <= 0:
        raise Problem("mianownik metryki jest zerowy — §10 wymaga niezerowej liczby "
                      "publicznych rekordów wyjściowych")
    return Fraction(cell["substrate_bytes"], cell["public_appends"])


def reduction(default_cell, reference_cell):
    """Redukcja metryki pierwotnej, z regułą zdegenerowaną z §7.4.

    Oryginał K26v2 liczył wprost `1 − m(D)/m(ref)` i przerywał się
    `ZeroDivisionError`, gdy substrat nie zmaterializował się po żadnej ze stron
    — a tak jest w F9-R2 przy `Q=1`, gdzie nie ma klasy równoważności (D6).
    Predeklaracja podaje tam wartość 0, więc redukcja MUSI być określona:

      * `m(ref) = 0` i `m(D) = 0` → redukcja 0; nie było czego zredukować;
      * `m(ref) = 0` i `m(D) > 0` → BRAK WERDYKTU; wzrost bez odniesienia nie ma
        wyrażenia w tej metryce i nie wolno go zgadywać.
    """
    reference = metric(reference_cell)
    default = metric(default_cell)
    if reference == 0:
        if default == 0:
            return Fraction(0)
        raise Problem("metryka odniesienia jest zerowa przy niezerowym DEFAULT — §7.4 "
                      "zabrania werdyktu w tym przypadku")
    return 1 - default / reference


def need(mech, key):
    if key not in mech:
        raise Problem(f"brak komorki {key} — iteracja niepelna")
    return mech[key]


def evaluate_family(family, mech, time, gate):
    """Zwraca (werdykt, szczegóły). Werdykt: 'SUPPORT' albo 'NO_SUPPORT'.
    Nieważność iteracji sygnalizuje wyjątek `Problem`."""
    detail = {"notes": []}
    ablation = MINIMAL_ABLATION[family]

    # ── Bramki. Klasyfikacja rozstrzyga, czy porażka liczy się przeciw H9, czy
    #    unieważnia iterację (§10, bramka poprawności).
    family_gates = gate.get(family, {})
    for name in REQUIRED_GATES:
        if name not in family_gates:
            raise Problem(f"{family}: brak wpisu bramki '{name}' — iteracja niepelna")
    gates_clean = True
    for name, (status, classification) in sorted(family_gates.items()):
        if status == "PASS":
            continue
        if classification in INVALIDATING_CLASSIFICATIONS:
            raise Problem(f"{family}: bramka '{name}' nieczysta i sklasyfikowana jako defekt "
                          f"{classification} — STOP-6, nowa iteracja bez laczenia danych")
        if classification != "engine_or_profile":
            raise Problem(f"{family}: bramka '{name}' nieczysta o nieznanej klasyfikacji "
                          f"'{classification}' — bez klasyfikacji werdyktu wydac nie wolno")
        gates_clean = False
        detail["notes"].append(f"bramka '{name}' nieczysta, przypisana silnikowi/profilowi")

    # ── Budżet slotu i zgubione rekordy. Komórki NIE wolno po cichu wykluczyc.
    for q in Q_GRID:
        for profile in ("DEFAULT", ablation):
            key = (family, profile, q)
            if key not in time:
                raise Problem(f"brak pomiarow czasu {key} — iteracja niepelna")
            blocks = time[key]
            if len(blocks) != BOOTSTRAP_BLOCKS:
                raise Problem(f"{key}: {len(blocks)} blokow, predeklarowano {BOOTSTRAP_BLOCKS}")
            for index, block in sorted(blocks.items()):
                if block["lost"] > 0:
                    raise Problem(f"{key} blok {index}: zgubiony rekord — STOP-8, rodzina "
                                  f"zatrzymana bez werdyktu")
                if Fraction(block["p99"], block["slot"]) > SLOT_BUDGET_STOP:
                    raise Problem(f"{key} blok {index}: p99 = {float(Fraction(block['p99'], block['slot'])):.3f} "
                                  f"slotu > {float(SLOT_BUDGET_STOP)} — STOP-8, rodzina zatrzymana "
                                  f"bez werdyktu")

    # ── Krzywe deterministyczne po obu stronach porównania. Dwie różne krzywe,
    #    nie jedna: wewnętrzna nasyca się na `1 − 1/F`, flinkowa rośnie liniowo z Q.
    #
    #    ODCHYLENIE OD KRZYWEJ NIE UNIEWAŻNIA ITERACJI. §10 rozstrzyga to wprost
    #    dla najważniejszego przypadku: „jeśli naturalny Flink sam współdzieli
    #    badany podplan, taki wynik pozostaje w kampanii i działa przeciw H9".
    #    Krzywa jest PRZEWIDYWANIEM, a nie bramką; bramką struktury planu jest
    #    tabela mechanizmu z P6 (`gates.tsv`), którą przy rozbieżności klasyfikuje
    #    człowiek na STOP-6. Gdyby skrypt unieważniał iterację za samo odejście od
    #    przewidywania, żaden wynik negatywny nie mógłby się nigdy wydarzyć —
    #    a to jest dokładnie ta postać bramki, która w tym projekcie zawiodła.
    controls_clean = True
    for q in Q_GRID:
        rdb_default = need(mech, (family, "RDB", "DEFAULT", q))
        rdb_ablation = need(mech, (family, "RDB", ablation, q))
        flink_natural = need(mech, (family, "FLINK", "NATURAL", q))

        got_rdb = reduction(rdb_default, rdb_ablation)
        want_rdb = PREDECLARED_RDB[family][q]
        if abs(got_rdb - want_rdb) > CURVE_TOLERANCE:
            detail["notes"].append(
                f"Q={q}: redukcja wewnetrzna {float(got_rdb) * 100:.3f}% wobec predeklarowanej "
                f"{float(want_rdb) * 100:.3f}%")
        got_flink = reduction(rdb_default, flink_natural)
        want_flink = PREDECLARED_FLINK[family][q]
        if abs(got_flink - want_flink) > CURVE_TOLERANCE:
            detail["notes"].append(
                f"Q={q}: redukcja wobec FLINK_NATURAL {float(got_flink) * 100:.3f}% wobec "
                f"predeklarowanej {float(want_flink) * 100:.3f}%")

        # Kontrola negatywna Q=1 dotyczy porownania WEWNETRZNEGO: brak klasy
        # rownowaznosci = brak redukcji. Po stronie Flinka przy Q=1 w F9-X
        # redukcja jest UJEMNA i tak zostalo predeklarowane wyzej.
        #
        # §10: „kazde nieoczekiwane scalenie kontroli nierownowaznosci przy
        # poprawnej aparaturze oznacza BRAK WSPARCIA H9 W RODZINIE" — a wiec
        # NO_SUPPORT, nie uniewaznienie iteracji.
        if q == 1 and abs(got_rdb) > CONTROL_TOLERANCE:
            controls_clean = False
            detail["notes"].append(
                f"przy Q=1 redukcja wewnetrzna wynosi {float(got_rdb) * 100:.3f}%, choc klasy "
                f"rownowaznosci nie ma — nieoczekiwane scalenie kontroli")

        # Pulapka potwierdzona w tym luku: liczba instancji pokazuje efekt tam,
        # gdzie go nie ma. Sama w sobie nie jest metryka i nie wolno jej czytac
        # jako wyniku — sprawdzamy wylacznie spojnosc kierunku planu, ktora nalezy
        # do struktury, a nie do wyniku (stad STOP-6).
        if rdb_default["instances"] > rdb_ablation["instances"]:
            raise Problem(f"{family} Q={q}: DEFAULT ma WIECEJ instancji wspolnego podplanu "
                          f"niz ablacja — plan nie izoluje mechanizmu (STOP-6)")

    # ── Kontrola pusta: profil bez czego dopasowac musi dac liczby DEFAULT.
    empty = EMPTY_CONTROL[family]
    if empty:
        for q in Q_GRID:
            a = need(mech, (family, "RDB", "DEFAULT", q))
            b = need(mech, (family, "RDB", empty, q))
            if (a["substrate_bytes"], a["instances"]) != (b["substrate_bytes"], b["instances"]):
                raise Problem(f"{family} Q={q}: kontrola pusta {empty} rozni sie od DEFAULT "
                              f"— plan nie izoluje mechanizmu (STOP-6)")

    # ── Punkt 4 progu: publiczne wyniki zachowują Obs. Liczba dopisań nie musi
    # być identyczna: R1 może skrócić ogon, więc Lat(Q) <= Lat(P). Semantyczną
    # relację wartości, indeksów i metadanych sprawdza bramka public_identity;
    # tutaj wymagamy jej przejścia oraz niezerowego mianownika każdej komórki.
    identity_ok = family_gates.get("public_identity", ("PASS", ""))[0] == "PASS"
    for q in Q_GRID:
        for profile in ("DEFAULT", "NO_R2_CANON", "NO_R1_FACTOR", "NO_R1_NO_R2"):
            key = (family, "RDB", profile, q)
            if key in mech and mech[key]["public_appends"] <= 0:
                raise Problem(f"{family} Q={q}/{profile}: brak publicznych rekordow")

    # ── Punkt 2 progu, komórka rozstrzygająca. Porównanie ŚCISŁE, bez tolerancji.
    q = DECISIVE_Q
    red_ablation = reduction(need(mech, (family, "RDB", "DEFAULT", q)),
                             need(mech, (family, "RDB", ablation, q)))
    red_flink = reduction(need(mech, (family, "RDB", "DEFAULT", q)),
                          need(mech, (family, "FLINK", "NATURAL", q)))
    bytes_ok = red_ablation >= THRESHOLD_REDUCTION and red_flink >= THRESHOLD_REDUCTION

    # ── Punkt 3 progu: cena czasowa.
    default_blocks = {b: v["median"] for b, v in time[(family, "DEFAULT", q)].items()}
    ablation_blocks = {b: v["median"] for b, v in time[(family, ablation, q)].items()}
    point, ci_low, ci_high = paired_bootstrap_ratio(default_blocks, ablation_blocks)
    time_ok = ci_high <= THRESHOLD_TIME_CI_UPPER

    # ── Praca: RAPORTOWANA, nie progowa. §10 stawia próg wyłącznie na metryce
    #    pierwotnej; dokładanie tu drugiego progu byłoby progiem spoza §10.
    column = RESOLVING_WORK[family]
    work_default = need(mech, (family, "RDB", "DEFAULT", q))[column]
    work_flink = need(mech, (family, "FLINK", "NATURAL", q))[column]
    work_reduction = 1 - Fraction(work_default, work_flink) if work_flink else Fraction(0)

    detail.update({
        "red_ablation": red_ablation, "red_flink": red_flink,
        "ratio": point, "ci_low": ci_low, "ci_high": ci_high,
        "bytes_ok": bytes_ok, "time_ok": time_ok,
        "gates_clean": gates_clean, "identity_ok": identity_ok,
        "controls_clean": controls_clean,
        "work_column": column, "work_reduction": work_reduction,
    })
    supported = gates_clean and controls_clean and bytes_ok and time_ok and identity_ok
    return ("SUPPORT" if supported else "NO_SUPPORT"), detail


# ══════════════════════════════════════════════════════════════════════════════
#  Werdykt
# ══════════════════════════════════════════════════════════════════════════════

def run(matrix_dir, stream=sys.stdout):
    try:
        mech, time, gate = load(matrix_dir)
    except (FileNotFoundError, ValueError) as exc:
        print(f"BRAK WERDYKTU: {exc}", file=stream)
        return 2

    results, problems = {}, {}
    for family in FAMILIES:
        try:
            results[family] = evaluate_family(family, mech, time, gate)
        except Problem as exc:
            problems[family] = str(exc)

    print("=" * 78, file=stream)
    print("WERDYKT K26 / H9 — automatyczny, wg progow zamrozonych w predeklaracji kampanii K26v3 §7", file=stream)
    print("=" * 78, file=stream)

    for family in FAMILIES:
        print(f"\n--- {family} (ablacja minimalna: {MINIMAL_ABLATION[family]}) ---", file=stream)
        if family in problems:
            print(f"  ITERACJA NIEWAZNA: {problems[family]}", file=stream)
            continue
        verdict, d = results[family]
        print(f"  metryka pierwotna @Q={DECISIVE_Q}:", file=stream)
        print(f"    redukcja wobec ablacji      {float(d['red_ablation']) * 100:8.3f}%   "
              f"prog {float(THRESHOLD_REDUCTION) * 100:.0f}%   "
              f"{'OK' if d['red_ablation'] >= THRESHOLD_REDUCTION else 'PONIZEJ'}", file=stream)
        print(f"    redukcja wobec FLINK_NATURAL{float(d['red_flink']) * 100:8.3f}%   "
              f"prog {float(THRESHOLD_REDUCTION) * 100:.0f}%   "
              f"{'OK' if d['red_flink'] >= THRESHOLD_REDUCTION else 'PONIZEJ'}", file=stream)
        print(f"  cena czasowa DEFAULT/ablacja: punkt {float(d['ratio']):.4f}, "
              f"95% CI [{float(d['ci_low']):.4f}, {float(d['ci_high']):.4f}], "
              f"prog gornej granicy {float(THRESHOLD_TIME_CI_UPPER):.2f}   "
              f"{'OK' if d['time_ok'] else 'PRZEKROCZONY'}", file=stream)
        print(f"  praca ({d['work_column']}, rozdzielajaca w tej rodzinie): redukcja wobec "
              f"FLINK_NATURAL {float(d['work_reduction']) * 100:.3f}%  [raportowana, nie progowa]",
              file=stream)
        print(f"  bramki czyste: {'tak' if d['gates_clean'] else 'NIE'}   "
              f"kontrole negatywne czyste: {'tak' if d['controls_clean'] else 'NIE'}   "
              f"identycznosc publicznych wynikow: {'tak' if d['identity_ok'] else 'NIE'}", file=stream)
        for note in d["notes"]:
            print(f"    uwaga: {note}", file=stream)
        print(f"  RODZINA: {verdict}", file=stream)

    print("\n" + "=" * 78, file=stream)
    if problems:
        print(f"BRAK WERDYKTU — {len(problems)} rodzin(a) technicznie niewaznych lub niepelnych.",
              file=stream)
        print("§10: taka iteracja nie wydaje werdyktu i musi zostac powtorzona w NOWYM", file=stream)
        print("katalogu; danych miedzy iteracjami nie wolno laczyc.", file=stream)
        for family, why in problems.items():
            print(f"  {family}: {why}", file=stream)
        return 2

    supporting = [f for f in FAMILIES if results[f][0] == "SUPPORT"]
    print(f"Rodzin wspierajacych H9: {len(supporting)}/{len(FAMILIES)} "
          f"({', '.join(supporting) if supporting else 'zadna'})", file=stream)
    print(f"Regula: wsparcie przy co najmniej {FAMILIES_REQUIRED}/{len(FAMILIES)}", file=stream)
    if len(supporting) >= FAMILIES_REQUIRED:
        print(f"\nWERDYKT: H9 WSPARTA w klasie Q={DECISIVE_Q}.", file=stream)
        print("Wniosek dotyczy AUTOMATYZACJI wspoldzielenia materializacji, nie ogolnej", file=stream)
        print("szybkosci. Zdania 'RetractorDB jest szybszy od Flinka', 'zawsze zuzywa mniej", file=stream)
        print("pamieci' i 'Flink nie potrafi wspoldzielic' pozostaja NIEUPRAWNIONE (§10).", file=stream)
        return 0
    print("\nWERDYKT: H9 BEZ WSPARCIA.", file=stream)
    print("Wynik K6c pozostaje obserwacja W9; jego mechanizmu nie wolno uogolniac (§10).", file=stream)
    return 1


# ══════════════════════════════════════════════════════════════════════════════
#  Bramka skryptu — sztuczne dane o znanej odpowiedzi
# ══════════════════════════════════════════════════════════════════════════════

#: Jednostki predeklarowane (RAPORT_PILOTA.md §2, PLANY_FLINKA.md §5), w `n_h*w`.
SYNTHETIC_UNITS = {
    "F9-R2": {"DEFAULT": Fraction(2, 3), "NO_R2_CANON": Fraction(4, 3),
              "NO_R1_FACTOR": Fraction(2, 3), "NO_R1_NO_R2": Fraction(4, 3),
              "FLINK_NATURAL_AT_Q8": Fraction(16, 3), "FLINK_UNIT": Fraction(2, 3)},
    "F9-R1": {"DEFAULT": Fraction(1), "NO_R1_FACTOR": Fraction(2),
              "NO_R2_CANON": Fraction(1), "NO_R1_NO_R2": Fraction(2),
              "FLINK_NATURAL_AT_Q8": Fraction(8), "FLINK_UNIT": Fraction(1)},
    "F9-X": {"DEFAULT": Fraction(5), "NO_R1_NO_R2": Fraction(12),
             "NO_R2_CANON": Fraction(6), "NO_R1_FACTOR": Fraction(10),
             "FLINK_NATURAL_AT_Q8": Fraction(32), "FLINK_UNIT": Fraction(4)},
}
SYNTHETIC_SCALE = 4500 * 9   # n_h * w przy zamrozonej liczbie rekordow
SYNTHETIC_PUBLIC_BASE = 4500


def synthetic_matrix(path, *, families=FAMILIES):
    """Zapisuje komplet zgodny co do cyfry z predeklarowanymi krzywymi."""
    path.mkdir(parents=True, exist_ok=True)
    mech, timing, gates = [], [], []

    for family in families:
        units = SYNTHETIC_UNITS[family]
        ablation = MINIMAL_ABLATION[family]
        for q in Q_GRID:
            # Wewnetrzna krzywa: DEFAULT stale, ablacja tak, by redukcja rownala
            # sie predeklarowanej.
            default_units = units["DEFAULT"]
            want = PREDECLARED_RDB[family][q]
            ablation_units = default_units / (1 - want)
            # Flinkowa krzywa: liniowa w Q.
            flink_units = units["FLINK_UNIT"] * q
            public = SYNTHETIC_PUBLIC_BASE * q

            rows = {
                ("RDB", "DEFAULT"): default_units,
                ("RDB", ablation): ablation_units,
                ("FLINK", "NATURAL"): flink_units,
                ("FLINK", "MANUAL"): default_units,
            }
            empty = EMPTY_CONTROL[family]
            if empty:
                rows[("RDB", empty)] = default_units
            for (system, profile), value in rows.items():
                work_costly = int(value * 12)
                mech.append({
                    "family": family, "system": system, "profile": profile, "q": q,
                    "instances": max(1, int(value * 3)),
                    "stream_selects": 1, "substrates": max(1, int(value)),
                    "r1": 2, "r2": 4,
                    "substrate_bytes": int(value * SYNTHETIC_SCALE),
                    "public_appends": public,
                    "work_costly_evals": work_costly,
                    "work_hash_picks": work_costly,
                    "work_add_merges": work_costly,
                })
            for profile in ("DEFAULT", ablation):
                for block in range(1, BOOTSTRAP_BLOCKS + 1):
                    timing.append({
                        "family": family, "profile": profile, "q": q, "block": block,
                        "compute_median_ns": 1000, "compute_p99_ns": 2000,
                        "slot_ns": 10000, "lost_records": 0,
                    })
        for gate_name in REQUIRED_GATES:
            gates.append({"family": family, "gate": gate_name, "status": "PASS",
                          "classification": "clean"})

    def write(name, columns, rows):
        lines = ["\t".join(columns)]
        for row in rows:
            lines.append("\t".join(str(row[c]) for c in columns))
        (path / name).write_text("\n".join(lines) + "\n")

    write("mechanism.tsv", MECHANISM_COLUMNS, mech)
    write("timing.tsv", TIMING_COLUMNS, timing)
    write("gates.tsv", GATE_COLUMNS, gates)
    return path


def patch_tsv(path, name, columns, predicate, changes):
    """Podmienia pola w wierszach spelniajacych `predicate` — mutacja sztucznych
    danych o znanej odpowiedzi."""
    rows = read_tsv(path / name, columns)
    for row in rows:
        if predicate(row):
            row.update({k: str(v) for k, v in changes.items()})
    lines = ["\t".join(columns)] + ["\t".join(row[c] for c in columns) for row in rows]
    (path / name).write_text("\n".join(lines) + "\n")


def selftest():
    """Sztuczne dane o ZNANEJ odpowiedzi, w tym wersje celowo obalone."""
    import io

    cases = []

    def case(name, expected, mutate=None):
        cases.append((name, expected, mutate))

    case("komplet zgodny z predeklaracja -> 3/3, H9 wsparta", 0)

    case("dwie rodziny ponizej progu bajtowego -> 1/3, brak wsparcia", 1,
         lambda p: [
             patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                       lambda r: r["family"] == fam and r["system"] == "FLINK"
                       and r["profile"] == "NATURAL" and int(r["q"]) == DECISIVE_Q,
                       {"substrate_bytes": int(SYNTHETIC_UNITS[fam]["DEFAULT"]
                                                / Fraction(65, 100) * SYNTHETIC_SCALE)})
             for fam in ("F9-R2", "F9-R1")])

    case("jedna rodzina ponizej progu -> 2/3, H9 nadal wsparta", 0,
         lambda p: patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                             lambda r: r["family"] == "F9-R2" and r["system"] == "FLINK"
                             and r["profile"] == "NATURAL" and int(r["q"]) == DECISIVE_Q,
                             {"substrate_bytes": int(SYNTHETIC_UNITS["F9-R2"]["DEFAULT"]
                                                     / Fraction(65, 100) * SYNTHETIC_SCALE)}))

    case("cena czasowa przekroczona w dwoch rodzinach -> brak wsparcia", 1,
         lambda p: [
             patch_tsv(p, "timing.tsv", TIMING_COLUMNS,
                       lambda r: r["family"] == fam and r["profile"] == "DEFAULT"
                       and int(r["q"]) == DECISIVE_Q,
                       {"compute_median_ns": 1200})
             for fam in ("F9-R2", "F9-X")])

    case("cena czasowa dokladnie na progu 1,05 -> przechodzi", 0,
         lambda p: patch_tsv(p, "timing.tsv", TIMING_COLUMNS,
                             lambda r: r["profile"] == "DEFAULT" and int(r["q"]) == DECISIVE_Q,
                             {"compute_median_ns": 1050}))

    case("cena czasowa tuz nad progiem (1,051) -> brak wsparcia", 1,
         lambda p: patch_tsv(p, "timing.tsv", TIMING_COLUMNS,
                             lambda r: r["profile"] == "DEFAULT" and int(r["q"]) == DECISIVE_Q,
                             {"compute_median_ns": 1051}))

    case("defekt aparatury w jednej rodzinie -> BRAK WERDYKTU", 2,
         lambda p: patch_tsv(p, "gates.tsv", GATE_COLUMNS,
                             lambda r: r["family"] == "F9-X" and r["gate"] == "oracle_mutants",
                             {"status": "FAIL", "classification": "apparatus"}))

    case("bramka waznosci korpusu w jednej rodzinie -> BRAK WERDYKTU", 2,
         lambda p: patch_tsv(p, "gates.tsv", GATE_COLUMNS,
                             lambda r: r["family"] == "F9-X" and r["gate"] == "corpus_validity",
                             {"status": "FAIL", "classification": "corpus"}))

    case("nieznana klasyfikacja nadal fail-closed -> BRAK WERDYKTU", 2,
         lambda p: patch_tsv(p, "gates.tsv", GATE_COLUMNS,
                             lambda r: r["family"] == "F9-R2" and r["gate"] == "oracle_values",
                             {"status": "FAIL", "classification": "unknown_new_label"}))

    case("rozbieznosc przypisana silnikowi w jednej rodzinie -> 2/3, wsparta", 0,
         lambda p: patch_tsv(p, "gates.tsv", GATE_COLUMNS,
                             lambda r: r["family"] == "F9-X" and r["gate"] == "oracle_values",
                             {"status": "FAIL", "classification": "engine_or_profile"}))

    case("brak wpisu bramki -> BRAK WERDYKTU (iteracja niepelna)", 2,
         lambda p: patch_tsv(p, "gates.tsv", GATE_COLUMNS,
                             lambda r: r["family"] == "F9-R1" and r["gate"] == "near_miss_controls",
                             {"gate": "cos_innego"}))

    case("p99 ponad 80% slotu w jednym bloku -> BRAK WERDYKTU (STOP-8)", 2,
         lambda p: patch_tsv(p, "timing.tsv", TIMING_COLUMNS,
                             lambda r: r["family"] == "F9-R1" and int(r["q"]) == 16
                             and int(r["block"]) == 7,
                             {"compute_p99_ns": 8100}))

    case("zgubiony rekord w jednym bloku -> BRAK WERDYKTU (STOP-8)", 2,
         lambda p: patch_tsv(p, "timing.tsv", TIMING_COLUMNS,
                             lambda r: r["family"] == "F9-R2" and int(r["block"]) == 3
                             and int(r["q"]) == 2,
                             {"lost_records": 1}))

    # §10: nieoczekiwane scalenie kontroli nierownowaznosci = brak wsparcia H9
    # W RODZINIE. Nie uniewaznia iteracji, wiec przy jednej rodzinie regula 2/3
    # nadal daje wsparcie, a dopiero przy dwoch werdykt sie odwraca.
    case("scalenie przy Q=1 w jednej rodzinie -> 2/3, H9 nadal wsparta", 0,
         lambda p: patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                             lambda r: r["family"] == "F9-R1" and r["system"] == "RDB"
                             and r["profile"] == MINIMAL_ABLATION["F9-R1"] and int(r["q"]) == 1,
                             {"substrate_bytes": 2 * int(SYNTHETIC_UNITS["F9-R1"]["DEFAULT"]
                                                         * SYNTHETIC_SCALE)}))

    case("scalenie przy Q=1 w dwoch rodzinach -> 1/3, brak wsparcia", 1,
         lambda p: [
             patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                       lambda r: r["family"] == fam and r["system"] == "RDB"
                       and r["profile"] == MINIMAL_ABLATION[fam] and int(r["q"]) == 1,
                       {"substrate_bytes": 2 * int(SYNTHETIC_UNITS[fam]["DEFAULT"] * SYNTHETIC_SCALE)})
             for fam in ("F9-R1", "F9-R2")])

    # Pulapka potwierdzona w tym luku: przy Q=1 w F9-R1 liczba INSTANCJI spada,
    # a liczba BAJTOW nie. Skrypt nie ma prawa zrobic z tego efektu.
    case("instancje spadaja, bajty nie -> brak efektu, komplet nadal wsparty", 0,
         lambda p: patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                             lambda r: r["family"] == "F9-R1" and r["system"] == "RDB"
                             and r["profile"] == MINIMAL_ABLATION["F9-R1"] and int(r["q"]) == 1,
                             {"instances": 99}))

    case("kontrola pusta rozni sie od DEFAULT -> BRAK WERDYKTU", 2,
         lambda p: patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                             lambda r: r["family"] == "F9-R2" and r["system"] == "RDB"
                             and r["profile"] == EMPTY_CONTROL["F9-R2"] and int(r["q"]) == 8,
                             {"substrate_bytes": 7 * SYNTHETIC_SCALE}))

    case("nieczysta bramka public_identity w dwoch rodzinach -> brak wsparcia", 1,
         lambda p: [
             patch_tsv(p, "gates.tsv", GATE_COLUMNS,
                       lambda r: r["family"] == fam and r["gate"] == "public_identity",
                       {"status": "FAIL", "classification": "engine_or_profile"})
             for fam in ("F9-R2", "F9-X")])

    case("mianownik zerowy -> BRAK WERDYKTU", 2,
         lambda p: patch_tsv(p, "mechanism.tsv", MECHANISM_COLUMNS,
                             lambda r: r["family"] == "F9-X" and r["system"] == "RDB"
                             and r["profile"] == "DEFAULT" and int(r["q"]) == 32,
                             {"public_appends": 0}))

    # ── §7.4, metryka zdegenerowana (D6) ────────────────────────────────────
    def zero_bytes(path, family, profiles):
        return patch_tsv(path, "mechanism.tsv", MECHANISM_COLUMNS,
                         lambda r: r["family"] == family and r["system"] == "RDB"
                         and int(r["q"]) == 1 and r["profile"] in profiles,
                         {"substrate_bytes": 0})

    def rdb_profiles(family):
        present = {"DEFAULT", MINIMAL_ABLATION[family]}
        if EMPTY_CONTROL[family]:
            present.add(EMPTY_CONTROL[family])
        return present

    case("zero substratu po obu stronach przy Q=1 -> redukcja 0, komplet przechodzi", 0,
         lambda p: zero_bytes(p, "F9-R2", rdb_profiles("F9-R2")))

    case("zero substratu tylko w odniesieniu -> BRAK WERDYKTU", 2,
         lambda p: zero_bytes(p, "F9-R2", {MINIMAL_ABLATION["F9-R2"]}))

    # ── §7.3, tolerancja kontroli negatywnej (D8) ───────────────────────────
    def bump_control(path, family, numerator, denominator):
        profiles = {"DEFAULT"}
        if EMPTY_CONTROL[family]:
            profiles.add(EMPTY_CONTROL[family])
        bumped = int(SYNTHETIC_UNITS[family]["DEFAULT"] * SYNTHETIC_SCALE) * numerator // denominator
        return patch_tsv(path, "mechanism.tsv", MECHANISM_COLUMNS,
                         lambda r: r["family"] == family and r["system"] == "RDB"
                         and int(r["q"]) == 1 and r["profile"] in profiles,
                         {"substrate_bytes": bumped})

    case("reszta ponizej tolerancji przy Q=1 -> kontrola czysta", 0,
         lambda p: [bump_control(p, fam, 100001, 100000) for fam in FAMILIES])

    case("reszta ponad tolerancja przy Q=1 -> brak wsparcia w dwoch rodzinach", 1,
         lambda p: [bump_control(p, fam, 101, 100) for fam in ("F9-R2", "F9-R1")])

    case("brak calej rodziny -> BRAK WERDYKTU (iteracja niepelna)", 2,
         lambda p: None, )

    failures = 0
    with tempfile.TemporaryDirectory() as tmp:
        for index, (name, expected, mutate) in enumerate(cases, start=1):
            path = Path(tmp) / f"case{index:02d}"
            if name.startswith("brak calej rodziny"):
                synthetic_matrix(path, families=["F9-R2", "F9-R1"])
            else:
                synthetic_matrix(path)
                if mutate:
                    mutate(path)
            sink = io.StringIO()
            code = run(path, stream=sink)
            status = "OK " if code == expected else "BLAD"
            if code != expected:
                failures += 1
            print(f"[{status}] przypadek {index:02d}: {name}")
            print(f"          oczekiwano kodu {expected}, otrzymano {code}")
            if code != expected:
                print("          --- wyjscie skryptu ---")
                for line in sink.getvalue().splitlines():
                    print(f"          {line}")

    print()
    if failures:
        print(f"BRAMKA SKRYPTU WERDYKTU: {failures} z {len(cases)} przypadkow NIEZGODNYCH")
        return 1
    print(f"BRAMKA SKRYPTU WERDYKTU: {len(cases)}/{len(cases)} przypadkow o znanej odpowiedzi zgodnych")
    print("W tym wersje celowo obalone: ponizej progu bajtowego, ponad progiem czasowym,")
    print("defekt aparatury/korpusu, nieznana klasyfikacja, iteracja niepelna, STOP-8,")
    print("scalenie przy Q=1, kontrola pusta")
    print("rozjechana z DEFAULT oraz pulapka 'instancje spadaja, bajty nie'.")
    print("Oraz przypadki zdegenerowane §7.4 i tolerancja kontroli §7.3: zero substratu po")
    print("obu stronach, zero tylko w odniesieniu, reszta pod i nad tolerancja.")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--matrix", type=Path, help="katalog z mechanism.tsv, timing.tsv, gates.tsv")
    parser.add_argument("--selftest", action="store_true", help="bramka na sztucznych danych")
    parser.add_argument("--emit-synthetic", type=Path, help="zapisz wzorcowy komplet i zakoncz")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    if args.emit_synthetic:
        synthetic_matrix(args.emit_synthetic)
        print(f"OK: wzorcowy komplet w {args.emit_synthetic}")
        return 0
    if not args.matrix:
        parser.error("podaj --matrix albo --selftest")
    return run(args.matrix)


def guarded_main():
    """Kod 1 znaczy WAZNY wynik negatywny i nie wolno go osiagnac awaria (§7.5).

    W K26v2 nieprzechwycony `ZeroDivisionError` zakonczyl skrypt kodem 1, czyli
    dokladnie tym samym kodem co legalne „brak wsparcia H9" (D6). Kazdy wyjatek
    jest tu zamieniany na kod 2 — BRAK WERDYKTU — a slad pozostaje na stderr.
    """
    try:
        return main()
    except SystemExit:
        raise
    except BaseException:  # noqa: BLE001 - kazda awaria ma dac BRAK WERDYKTU
        traceback.print_exc()
        print("BRAK WERDYKTU — skrypt werdyktu przerwal sie awaria (§7.5); "
              "kod 2, nie 1.", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(guarded_main())
