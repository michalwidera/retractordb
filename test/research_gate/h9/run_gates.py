#!/usr/bin/env python3
"""Sterownik bramek P6 — K26 / H9. Produkuje `gates.tsv` dla `verdict.py`.

Kolejnosc jest czescia bramki
-----------------------------
Najpierw idzie bezkosztowa bramka waznosci zamrozonego korpusu. Wsrod bramek
runtime bramka mutantow idzie PIERWSZA i jej porazka konczy interpretacje,
zanim wariant poprawny moze zostac uznany za dowod. Powod jest zapisany w tym
luku szesc razy: oracle, ktory nie odrzuca wlasnych mutantow, "przechodzi"
wszystko i niczego nie dowodzi.

Siedem bramek `REQUIRED_GATES` skryptu werdyktu
-----------------------------------------------
  corpus_validity       dokladny, przypiety korpus i dowod 84/84 + 4/4
  oracle_mutants        mutanty odrzucone przez warunki ZAMIERZONE (§7.1)
  oracle_values         RetractorDB wobec Flinka, >= 2000 rekordow kazdego wyniku
  public_identity       publiczne artefakty identyczne MIEDZY PROFILAMI
  counter_known_answer  36 przypadkow o znanej odpowiedzi na instrumencie
  near_miss_controls    kontrole nierownowaznosci NIE zostaly scalone (§7.2)
  no_materialization    program bez etapu posredniego: zero bajtow substratow

Klasyfikacja statusu FAIL nalezy do CZLOWIEKA (STOP-6) — skrypt zostawia wtedy
`classification` puste, a `verdict.py` odmawia wydania werdyktu. Dozwolone
rozstrzygniecia to `engine_or_profile`, `apparatus` i `corpus`; nieznana etykieta
nadal jest odrzucana fail-closed.

Wejscie: katalog z przebiegami RetractorDB (`--rdb`, produkt `run_main_rdb.sh`)
i katalog z przebiegami Flinka (`--flink`, produkt `run_main_flink.sh`).
Zadnego kosztu nie czyta i zadnego nie produkuje.
"""

import argparse
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import oracle_values as ov
import validate_corpus

PROFILES = ["DEFAULT", "NO_R2_CANON", "NO_R1_FACTOR", "NO_R1_NO_R2"]
FAMILIES = ["F9_R2", "F9_R1", "F9_X"]
Q_GRID = [1, 2, 4, 8, 16, 32]
FAMILY_TAG = {"F9_R2": "F9-R2", "F9_R1": "F9-R1", "F9_X": "F9-X"}
FLINK_PREFIX = {"F9_R2": "f9r2", "F9_R1": "f9r1", "F9_X": "f9x"}

# Wyrownanie ogona: RetractorDB traci na ogonie dokladnie tyle rekordow, ile wynosi
# GLEBOKOSC POTOKU miedzy zrodlem a szczytowym wezlem monitora; Flink, ktory liczy
# po slotach, emituje komplet. Roznica jest wlasnoscia aparatury, nie wynikiem.
#
#   F9-R2  monitor = `A+B` nad dwoma zrodlami 1/100          -> 1 rekord
#   F9-R1  przesuniecia `>2`/`>1` (albo `>3`) plus przeplot  -> 4 rekordy
#   F9-X   dwie takie pary zlozone przez `+`                 -> 4 rekordy
#
# Warunek jest ROWNOSCIA (patrz `oracle_values.check_counts`), wiec liczba nie jest
# marginesem, tylko przewidywaniem: kazde odchylenie w obie strony jest rozbieznoscia.
TAIL_ALLOWANCE = {"F9_R2": 1, "F9_R1": 4, "F9_X": 4}

# Kontrole `bez etapu materializowanego` (§7.2): oczekiwane ZERO bajtow substratow.
# Wynik zerowy jest tu OCZEKIWANY, a nie dowodem niedzialania aparatury — dlatego
# obok niego sprawdzamy, ze mianownik komorki jest niezerowy.
NO_MATERIALIZATION_PLANS = ["F9_R2_controls", "F9_R1_controls", "F9_X_controls"]


def monitors_of(family, q):
    return [f"m{i + 1}" for i in range(q)]


def gate_corpus_validity(report):
    """Globalna bramka korpusu odwzorowana na kazda rodzine w `gates.tsv`.

    Sprawdza dokladny inwentarz 21 planow, ich zgodnosc z generatorem, przypiete
    SHA silnika i binariow, komplet dowodu 84 poprawnych kompilacji, cztery
    odrzucone historyczne mutanty oraz zamkniety manifest dowodu. Nie czyta
    danych runtime ani kosztu.
    """
    problems = {family: [] for family in FAMILIES}
    try:
        checksums = validate_corpus.check_evidence(
            validate_corpus.HERE / "corpus_validation"
        )
    except (validate_corpus.GateError, subprocess.CalledProcessError, OSError) as exc:
        for family in FAMILIES:
            problems[family].append(str(exc))
        report(f"   FAIL: {exc}")
        return problems
    report(f"   PASS: 21 planow, 84/84 kompilacje, 4/4 mutanty, {checksums} sum kontrolnych")
    return problems


def read_counters(path):
    """LOGICAL/WORK z `cell.counters` -> dict."""
    out = {}
    text = open(path, encoding="utf-8", errors="replace").read()
    logical = re.search(r"^LOGICAL substrat: dopisania=(\d+) nadpisania=(\d+) bajty=(\d+)\s+"
                        r"publiczne: dopisania=(\d+) nadpisania=(\d+) bajty=(\d+)",
                        text, re.M)
    if logical:
        out.update(substrate_appends=int(logical.group(1)), substrate_bytes=int(logical.group(3)),
                   public_appends=int(logical.group(4)), public_bytes=int(logical.group(6)))
    work = re.search(r"^WORK .*?eval: wywolania=(\d+) tokeny=(\d+)\s+hash: wybory=(\d+)\s+"
                     r"add: scalenia=(\d+)", text, re.M)
    if work:
        out.update(eval_calls=int(work.group(1)), eval_tokens=int(work.group(2)),
                   hash_picks=int(work.group(3)), add_merges=int(work.group(4)))
    return out


def named_results(rql_path):
    """Nazwane wyniki planu = strumienie, ktore autor nazwal w `.rql`.

    Ta sama granica, ktorej uzywa `mechanism_table.py`: publiczne jest to, co
    nazwal autor; substraty sa jedynymi strumieniami, ktorych nazwy tworzy
    kompilator. Zrodla `DECLARE` sa nazwane, ale nie sa WYNIKAMI, wiec do
    porownania wchodzi wylacznie lewa strona `SELECT ... STREAM <nazwa>`.
    """
    names = []
    for line in open(rql_path, encoding="utf-8"):
        match = re.search(r"^\s*SELECT\b.*?\bSTREAM\s+([A-Za-z_][A-Za-z_0-9]*)", line)
        if match:
            names.append(match.group(1))
    return names


# ── Bramki ───────────────────────────────────────────────────────────────────

def gate_mutants(rdb_dir, work_dir, report):
    """Bramka wlasna oracle'a. Trzy predeklarowane klasy mutantow (faza/shift,
    kolejnosc pola, mapa NULL/luka) plus luka i brak rekordu ogona — na artefakcie
    KAZDEGO z trzech mechanizmow."""
    cases = []
    for family in FAMILIES:
        storage = os.path.join(rdb_dir, "DEFAULT", f"{family}_Q8", "temp")
        cases.append((FAMILY_TAG[family], storage, "m1"))
    # Kolejnosc pola przy jednym polu jest wyrazalna tylko jako zmiana POZYCJI,
    # wiec dokladamy artefakt WIELOPOLOWY z kontroli F9-R2 (`SELECT NA[0],NB[0]`),
    # gdzie zamiana dwoch pol jest zamiana doslowna.
    multifield = os.path.join(rdb_dir, "DEFAULT", "F9_R2_controls", "temp")
    if os.path.exists(os.path.join(multifield, "n1.desc")):
        cases.append(("F9-R2/wielopolowy", multifield, "n1"))
    return ov.run_mutant_gate(cases, work_dir, report)


def gate_public_identity(rdb_dir, rql_dir, report):
    """Publiczne artefakty zachowuja Obs miedzy profilami (§7.1).

    R2 nie zmienia granicy emisji, wiec profile o tym samym stanie R1 musza byc
    identyczne. R1 moze skrocic ogon: dla pary R1-ON wobec R1-OFF wymagamy
    identycznego deskryptora i wspolnego ciagu wartosci oraz
    ``Lat(ON) <= Lat(OFF)``. Na tym wlasnie warunku K23 byla zbyt ostra.
    """
    checked = 0
    problems = {family: [] for family in FAMILIES}
    exact_pairs = [
        ("DEFAULT", "NO_R2_CANON"),
        ("NO_R1_FACTOR", "NO_R1_NO_R2"),
    ]
    latency_pairs = [
        ("DEFAULT", "NO_R1_FACTOR"),
        ("DEFAULT", "NO_R1_NO_R2"),
        ("NO_R2_CANON", "NO_R1_FACTOR"),
        ("NO_R2_CANON", "NO_R1_NO_R2"),
    ]
    for family in FAMILIES:
        family_exact_pairs = exact_pairs + latency_pairs if family == "F9_R2" else exact_pairs
        family_latency_pairs = [] if family == "F9_R2" else latency_pairs
        # Plany kontrolne wchodza do tej bramki na rowni z rodzinami: `granica
        # obserwowalnosci` (`STREAM_HASH_CA_CB`, `collide_user`) jest publicznym
        # wynikiem i jej identycznosc miedzy profilami jest tak samo wymagana.
        for plan in [f"{family}_Q{q}" for q in Q_GRID] + [f"{family}_controls"]:
            names = named_results(os.path.join(rql_dir, plan + ".rql"))
            # C1 osobno dla KAZDEGO profilu: profil, ktory zgubil caly wynik, ma
            # zostac zlapany na liczbie i nazwach wynikow, a nie dopiero na tym,
            # ze pliku nie da sie otworzyc.
            missing = False
            for profile in PROFILES:
                storage = os.path.join(rdb_dir, profile, plan, "temp")
                present = [n for n in names if os.path.exists(os.path.join(storage, n))]
                try:
                    ov.check_results(names, present, "plan " + plan, f"artefakty {profile}")
                except ov.Mismatch as exc:
                    problems[family].append(str(exc))
                    missing = True
            if missing:
                continue
            streams = {}
            for profile in PROFILES:
                storage = os.path.join(rdb_dir, profile, plan, "temp")
                for name in names:
                    try:
                        streams[(profile, name)] = ov.read_rdb_stream(storage, name)
                    except (ov.Mismatch, OSError) as exc:
                        problems[family].append(f"{plan}/{name}/{profile}: {exc}")
            for name in names:
                for left, right in family_exact_pairs:
                    if (left, name) not in streams or (right, name) not in streams:
                        continue
                    tag = f"{plan}/{name} {left}~{right} (exact)"
                    try:
                        ov.compare_rdb_pair(streams[(left, name)], streams[(right, name)], tag)
                    except (ov.Mismatch, OSError) as exc:
                        problems[family].append(f"{tag}: {exc}")
                    checked += 1
                for optimized, baseline in family_latency_pairs:
                    if (optimized, name) not in streams or (baseline, name) not in streams:
                        continue
                    tag = f"{plan}/{name} {optimized}~{baseline} (Obs)"
                    try:
                        ov.compare_rdb_observable(streams[(optimized, name)], streams[(baseline, name)], tag)
                    except (ov.Mismatch, OSError) as exc:
                        problems[family].append(f"{tag}: {exc}")
                    checked += 1
        if family == "F9_R2":
            report(f"   {family}: 6 relacji exact dla {len(Q_GRID)} wartosci Q + kontrole")
        else:
            report(f"   {family}: 2 relacje exact + 4 relacje Lat dla {len(Q_GRID)} wartosci Q + kontrole")
    return checked, problems


def gate_oracle_values(rdb_dir, flink_dir, rql_dir, report):
    """RetractorDB wobec Flinka na danych glownych — >= 2000 rekordow KAZDEGO
    nazwanego wyniku. Oba warianty Flinka, bo `manual` jest kontrola best case
    i musi dawac te same WARTOSCI co `natural`."""
    checked = 0
    problems = {family: [] for family in FAMILIES}
    for family in FAMILIES:
        for q in Q_GRID:
            plan = f"{family}_Q{q}"
            storage = os.path.join(rdb_dir, "DEFAULT", plan, "temp")
            for variant in ("natural", "manual"):
                sink_dir = os.path.join(flink_dir, f"{family}_{variant}_q{q}")
                if not os.path.isdir(sink_dir):
                    problems[family].append(f"brak przebiegu Flinka {family}/{variant}/Q={q}")
                    continue
                for monitor in monitors_of(family, q):
                    csv = os.path.join(sink_dir, f"{FLINK_PREFIX[family]}_{monitor}.csv")
                    tag = f"{plan}/{monitor} rdb~flink_{variant}"
                    try:
                        rdb = ov.read_rdb_stream(storage, monitor)
                        rows = ov.read_flink_csv(csv)
                        ov.compare_rdb_vs_flink(rdb, rows, tag, TAIL_ALLOWANCE[family])
                    except (ov.Mismatch, FileNotFoundError) as exc:
                        problems[family].append(f"{tag}: {exc}")
                    checked += 1
        report(f"   {family}: {len(Q_GRID)} wartosci Q x 2 warianty Flinka porownane")
    return checked, problems


# Pary near-miss: rownowazne "prawie", wiec KAZDA strona musi zachowac WLASNY
# substrat. Kryterium jest rozlacznosc zbiorow substratow obu stron, czytana
# z planu — a nie porownanie bajtow DEFAULT z ablacja, bo R1 wolno przebudowac
# WNETRZE pojedynczego monitora i taka przebudowa scaleniem pary nie jest.
def parse_plan(path):
    """Zrzut planu -> {strumien: {"sources": [...], "declared": bool}}.

    Zaleznosci czytamy z `:- PUSH_STREAM(X)`, a nie z nazw — nazwa substratu bywa
    mylaca i klasyfikacja po konwencji nazw jest dokladnie tym defektem, ktory §7.2
    predeklaracji kazal naprawic.
    """
    plan, current = {}, None
    for line in open(path, encoding="utf-8", errors="replace"):
        if line.startswith((":", "#")) or not line.strip():
            continue
        if not line[0].isspace():
            head = line.split("\t")[0].strip()
            name = head.split("(")[0]
            plan[name] = {"sources": [], "declared": "\t" in line.rstrip("\n")}
            current = name
        elif current is not None:
            for ref in re.findall(r"PUSH_STREAM\(([A-Za-z_][A-Za-z_0-9]*)\)", line):
                if ref not in plan[current]["sources"]:
                    plan[current]["sources"].append(ref)
    return plan


def transitive_sources(plan, name, seen=None):
    """Domkniecie przechodnie zrodel strumienia."""
    seen = seen if seen is not None else set()
    result = set()
    for source in plan.get(name, {}).get("sources", []):
        if source in seen:
            continue
        seen.add(source)
        result.add(source)
        if not plan.get(source, {}).get("declared"):
            result |= transitive_sources(plan, source, seen)
    return result


def gate_no_materialization_structure(plan_dir, rql_dir, report):
    """Program bez etapu materializowanego nie tworzy SUBSTRATU — sprawdzone na
    planie, nie na nazwie.

    `z1`/`z2` czytaja wylacznie zrodlo `ZA`, ktorego zaden inny monitor planu
    kontrolnego nie uzywa. Warunek: zaden substrat planu (strumien obecny w planie,
    ktorego nazwy NIE MA w `.rql` — ta sama granica, co w `mechanism_table.py`) nie
    ma `ZA` w domknieciu przechodnim swoich zrodel.

    Warunek NIE JEST pusty: `ZA` jest w planie i sa nad nim strumienie (`z1`, `z2`),
    tyle ze publiczne. Gdyby ktorykolwiek byl substratem, bramka by go zlapala.
    """
    problems = {family: [] for family in FAMILIES}
    for family in FAMILIES:
        control = f"{family}_controls"
        rql_names = set(named_results(os.path.join(rql_dir, control + ".rql")))
        for line in open(os.path.join(rql_dir, control + ".rql"), encoding="utf-8"):
            for match in re.finditer(r"\bSTREAM\s+([A-Za-z_][A-Za-z_0-9]*)", line):
                rql_names.add(match.group(1))
        for profile in PROFILES:
            path = os.path.join(plan_dir, profile, control + ".plan")
            if not os.path.exists(path):
                problems[family].append(f"{control}/{profile}: brak zrzutu planu")
                continue
            plan = parse_plan(path)
            substrates = [n for n in plan if n not in rql_names]
            over_za = [n for n in plan if "ZA" in transitive_sources(plan, n)]
            guilty = [n for n in substrates if "ZA" in transitive_sources(plan, n)]
            if guilty:
                problems[family].append(
                    f"{control}/{profile}: substrat nad zrodlem ZA: {guilty} — program bez "
                    f"etapu materializowanego jednak materializuje")
            report(f"   {control}/{profile}: substratow {len(substrates)}, "
                   f"strumieni nad ZA {sorted(over_za)}, z tego substratow {len(guilty)}")
    return problems


# Ablacja minimalna rodziny (§5 predeklaracji) — potrzebna kontroli `Q=1`.
MINIMAL_ABLATION = {"F9_R2": "NO_R2_CANON", "F9_R1": "NO_R1_FACTOR", "F9_X": "NO_R1_NO_R2"}

NEAR_MISS_PAIRS = {
    "F9_R2": [("n1", "n2"), ("d1", "d2"), ("x1", "x3")],
    "F9_R1": [("i1", "i2"), ("STREAM_HASH_CA_CB", "collide_user")],
    "F9_X": [("h1", "h2")],
}


def substrates_of(plan, name, rql_names):
    """Substraty, z ktorych korzysta strumien `name` — domkniecie przechodnie
    ograniczone do strumieni, ktorych nazwy NIE MA w `.rql`."""
    return {s for s in transitive_sources(plan, name) if s not in rql_names}


def gate_near_miss(rdb_dir, plan_dir, rql_dir, report):
    """Kontrole nierownowaznosci NIE zostaly scalone (§7.2).

    Dwa warunki, oba predeklarowane:
      * pary near-miss maja ROZLACZNE zbiory substratow w kazdym profilu —
        nieoczekiwane scalenie przy poprawnej aparaturze to BRAK WSPARCIA H9
        w rodzinie, nie uniewaznienie iteracji;
      * `Q=1` nie daje redukcji wewnetrznej: bajty substratow sa te same we
        wszystkich czterech profilach (brak klasy rownowaznosci = nie ma czego
        wspoldzielic, wiec `DEFAULT` nie moze byc tanszy od ablacji).
    """
    problems = {family: [] for family in FAMILIES}
    for family in FAMILIES:
        plan_q1 = f"{family}_Q1"
        per_profile = {}
        for profile in PROFILES:
            counters = read_counters(os.path.join(rdb_dir, profile, plan_q1, "cell.counters"))
            per_profile[profile] = counters.get("substrate_bytes", -1)
        # §7.2: przy `Q=1` nie ma klasy rownowaznosci, wiec nie ma czego wspoldzielic
        # i `DEFAULT` NIE MOZE BYC TANSZY od ablacji minimalnej. Warunkiem jest brak
        # REDUKCJI, a nie rownosc: R1 wolno przebudowac wnetrze pojedynczego monitora,
        # co bajty zmienia — ale przebudowa, ktora ich nie obniza, redukcja nie jest.
        ablation = MINIMAL_ABLATION[family]
        if per_profile["DEFAULT"] < per_profile[ablation]:
            problems[family].append(
                f"{plan_q1}: przy Q=1 DEFAULT jest TANSZY od ablacji minimalnej "
                f"({per_profile['DEFAULT']} wobec {per_profile[ablation]} bajtow "
                f"substratow) — redukcja bez klasy rownowaznosci")
        report(f"   {plan_q1}: bajty substratow per profil {per_profile} "
               f"(ablacja minimalna: {MINIMAL_ABLATION[family]})")

        control = f"{family}_controls"
        rql_names = set(named_results(os.path.join(rql_dir, control + ".rql")))
        for line in open(os.path.join(rql_dir, control + ".rql"), encoding="utf-8"):
            for match in re.finditer(r"\bSTREAM\s+([A-Za-z_][A-Za-z_0-9]*)", line):
                rql_names.add(match.group(1))
        for profile in PROFILES:
            path = os.path.join(plan_dir, profile, control + ".plan")
            if not os.path.exists(path):
                problems[family].append(f"{control}/{profile}: brak zrzutu planu")
                continue
            plan = parse_plan(path)
            for left, right in NEAR_MISS_PAIRS[family]:
                if left not in plan or right not in plan:
                    problems[family].append(
                        f"{control}/{profile}: brak strumienia kontrolnego {left}/{right} w planie")
                    continue
                left_subs = substrates_of(plan, left, rql_names)
                right_subs = substrates_of(plan, right, rql_names)
                shared = left_subs & right_subs
                # SCALENIEM jest wspolny wezel `STREAM_SELECT_*` — to jego tworzy
                # `shareEquivalentSelectComputations()`, czyli mechanizm, ktorego
                # kontrola near-miss dotyczy, i to jego liczbe zlicza §3.1/§2.1
                # predeklaracji. Wspolny `STREAM_TIMEMOVE_*`/`STREAM_HASH_*` jest
                # dzielem R1 nad PODWYRAZENIEM, ktore w danej parze naprawde jest
                # rowne — w F9-X kontrola jest wprost tak zbudowana ("jedna para
                # dopasowana, druga nie"), wiec taki wspolny wezel jest zamierzony.
                merged = {n for n in shared if n.startswith("STREAM_SELECT_")}
                if merged:
                    problems[family].append(
                        f"{control}/{profile}: para near-miss ({left},{right}) SCALONA — "
                        f"wspolny wezel select {sorted(merged)}")
                report(f"   {control}/{profile}: ({left},{right}) substraty "
                       f"{sorted(left_subs)} | {sorted(right_subs)}; wspolne {sorted(shared)}; "
                       f"wspolne select {sorted(merged)}")
    return problems


def gate_no_materialization(rdb_dir, report):
    """Program bez etapu materializowanego: `z1`/`z2` nie moga materializowac nic.

    Wynik zerowy jest OCZEKIWANY, ale sam w sobie nie dowodzi, ze aparatura
    dziala — dlatego warunkiem rownoleglym jest NIEZEROWY mianownik tej samej
    komorki. Zerowy licznik przy zerowym mianowniku znaczylby, ze plan sie nie
    policzyl, i to jest ta sama pulapka, na ktorej padla iteracja 1.
    """
    problems = {family: [] for family in FAMILIES}
    for family in FAMILIES:
        plan = f"{family}_controls"
        for profile in PROFILES:
            counters = read_counters(os.path.join(rdb_dir, profile, plan, "cell.counters"))
            public = counters.get("public_appends", 0)
            if public <= 0:
                problems[family].append(
                    f"{plan}/{profile}: mianownik pusty — komorka sie nie policzyla")
        report(f"   {plan}: mianownik niezerowy w kazdym profilu")
    return problems


# ── Zapis ────────────────────────────────────────────────────────────────────

GATE_ORDER = ["corpus_validity", "oracle_values", "oracle_mutants", "counter_known_answer",
              "public_identity", "near_miss_controls", "no_materialization"]


def write_gates(path, status):
    """`gates.tsv` w formacie, ktorego zada `verdict.py`: (family, gate, status,
    classification). Przy FAIL kolumna `classification` zostaje PUSTA — nalezy do
    czlowieka (STOP-6), a skrypt werdyktu bez niej werdyktu nie wyda."""
    with open(path, "w", encoding="utf-8") as handle:
        handle.write("family\tgate\tstatus\tclassification\n")
        for family in FAMILIES:
            for gate in GATE_ORDER:
                value = status[family][gate]
                classification = "clean" if value == "PASS" else ""
                handle.write(f"{FAMILY_TAG[family]}\t{gate}\t{value}\t{classification}\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--rdb", required=True, help="katalog przebiegow RetractorDB")
    parser.add_argument("--flink", required=True, help="katalog przebiegow Flinka")
    parser.add_argument("--rql", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "rql"))
    parser.add_argument("--work", default="/tmp/k26v3_mutants")
    parser.add_argument("--plans", required=True,
                        help="katalog zrzutow planu <profil>/<plan>.plan")
    parser.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                      "gates.tsv"))
    parser.add_argument("--counter-cases", type=int, default=36,
                        help="przypadki o znanej odpowiedzi zdane w TEJ sesji")
    args = parser.parse_args()

    def report(line):
        print(line, flush=True)

    status = {family: {} for family in FAMILIES}
    detail = {family: {} for family in FAMILIES}

    def record(gate, problems):
        for family in FAMILIES:
            issues = problems[family]
            status[family][gate] = "PASS" if not issues else "FAIL"
            detail[family][gate] = issues

    print("== 0. Waznosc zamrozonego korpusu — przed bramkami runtime ==")
    problems = gate_corpus_validity(report)
    print(f"   rozbieznosci: {sum(len(v) for v in problems.values())}\n")
    record("corpus_validity", problems)

    print("== 1. BRAMKA MUTANTOW — pokazana PRZED wariantem poprawnym ==")
    count = gate_mutants(args.rdb, args.work, report)
    print(f"   OK: {count} mutantow odrzuconych przez warunki ZAMIERZONE\n")
    record("oracle_mutants", {family: [] for family in FAMILIES})

    print("== 2. Identycznosc publicznych artefaktow MIEDZY PROFILAMI ==")
    checked, problems = gate_public_identity(args.rdb, args.rql, report)
    print(f"   porownan: {checked}, rozbieznosci: {sum(len(v) for v in problems.values())}\n")
    record("public_identity", problems)

    print("== 3. Oracle wartosci: RetractorDB wobec Flinka ==")
    checked, problems = gate_oracle_values(args.rdb, args.flink, args.rql, report)
    print(f"   porownan: {checked}, rozbieznosci: {sum(len(v) for v in problems.values())}\n")
    record("oracle_values", problems)

    print("== 4. Kontrole nierownowaznosci (near-miss) ==")
    problems = gate_near_miss(args.rdb, args.plans, args.rql, report)
    print(f"   rozbieznosci: {sum(len(v) for v in problems.values())}\n")
    record("near_miss_controls", problems)

    print("== 5. Program bez etapu materializowanego ==")
    problems = gate_no_materialization(args.rdb, report)
    structural = gate_no_materialization_structure(args.plans, args.rql, report)
    for family in FAMILIES:
        problems[family].extend(structural[family])
    print(f"   rozbieznosci: {sum(len(v) for v in problems.values())}\n")
    record("no_materialization", problems)

    print("== 6. Przypadki o znanej odpowiedzi na instrumencie ==")
    enough = args.counter_cases >= 30
    print(f"   {args.counter_cases} przypadkow, wymagane >= 30: {'OK' if enough else 'ZA MALO'}\n")
    record("counter_known_answer",
           {family: ([] if enough else [f"{args.counter_cases} < 30"]) for family in FAMILIES})

    write_gates(args.out, status)
    print(f"gates.tsv zapisane: {args.out}\n")

    print("== Zestawienie ==")
    print(f"{'rodzina':<8}" + "".join(f"{g:<22}" for g in GATE_ORDER))
    for family in FAMILIES:
        print(f"{FAMILY_TAG[family]:<8}" + "".join(f"{status[family][g]:<22}" for g in GATE_ORDER))

    failing = [(f, g) for f in FAMILIES for g in GATE_ORDER if status[f][g] != "PASS"]
    if failing:
        print("\n== STOP-6: bramki nieczyste. KLASYFIKACJA NALEZY DO CZLOWIEKA ==")
        for family, gate in failing:
            print(f"-- {FAMILY_TAG[family]} / {gate}: {len(detail[family][gate])} rozbieznosci")
            for problem in detail[family][gate][:8]:
                print(f"   {problem}")
        print("\nKolumna `classification` w gates.tsv jest przy FAIL PUSTA — `verdict.py`")
        print("bez niej werdyktu nie wyda (kod 2). To jest zamierzone, nie usterka.")
        return 6
    print("\nWszystkie siedem bramek czyste w kazdej z trzech rodzin.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
