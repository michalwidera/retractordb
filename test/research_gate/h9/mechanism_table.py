#!/usr/bin/env python3
"""Tabela liczb mechanizmu ze zrzutów planu — aparatura K26.

Wersja obalona z K23 rozpoznawała substraty po konwencji nazw. K26 nie przenosi
tego skryptu jako aparatury; zachowuje jedynie jego minimalną funkcję
`legacy_classify` jako mutanta, którego `--gate` musi odróżnić.

Na czym polega poprawka
-----------------------
Klasyfikacja nie po nazwie, lecz po ŹRÓDLE NAZWY: substratem jest strumień
obecny w planie, którego nazwa NIE WYSTĘPUJE w `.rql`. Wszystko, co autor
nazwał — czy to `DECLARE ... STREAM X`, czy `SELECT ... STREAM Y` — jest
publiczne, niezależnie od tego, jak wygląda. To odwzorowuje granicę, którą
naprawdę rozróżnia silnik (`qry.isSubstrat` → `storage::markAsSubstrate()`), bo
substraty są jedynymi strumieniami, których nazwy tworzy kompilator.

Zrzut planu nie niesie flagi `isSubstrat`, więc bez `.rql` tej granicy odtworzyć
nie można — i dlatego skrypt wymaga obu plików, zamiast zgadywać z jednego.

Uruchomienie:
    ./mechanism_table.py --plan-dir DIR --rql-dir DIR [--plans A B ...]
    ./mechanism_table.py --gate     # bramka o znanej odpowiedzi (patrz niżej)
"""
import argparse
import re
import sys
from fractions import Fraction
from pathlib import Path

HERE = Path(__file__).resolve().parent

PROFILES = ["DEFAULT", "NO_R2_CANON", "NO_R1_FACTOR", "NO_R1_NO_R2"]

HEAD = re.compile(r"^(?P<name>[A-Za-z_][A-Za-z0-9_]*)\((?P<num>\d+)/(?P<den>\d+)\)")
FIELD = re.compile(r"^\t[A-Za-z_][A-Za-z0-9_]*: (?P<type>[A-Z]+)")
PUSH = re.compile(r"^\t:- PUSH_STREAM\((?P<src>[^)]+)\)")
#: Nazwa nadana przez AUTORA — w `DECLARE ... STREAM X` i w `SELECT ... STREAM Y`.
RQL_STREAM = re.compile(r"\bSTREAM\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)")

#: Jednostka odniesienia: `n_h*w` przy 150 Hz i jednym polu INTEGER (RAPORT_PILOTA.md §1).
UNIT_RATE = 150


def canonical_bytes(fields):
    """`probe::canonicalRecordBytes`: INTEGER = 8 B, plus mapa NULL/luk ceil(n/8)."""
    return 8 * fields + (fields + 7) // 8


def parse_plan(path):
    """Zwraca {nazwa: {'interval': Fraction, 'fields': int, 'sources': [str]}}."""
    streams, current = {}, None
    for line in path.read_text().splitlines():
        head = HEAD.match(line)
        if head:
            current = head.group("name")
            streams[current] = {
                "interval": Fraction(int(head.group("num")), int(head.group("den"))),
                "fields": 0,
                "sources": [],
            }
            continue
        if current is None:
            continue
        if FIELD.match(line):
            streams[current]["fields"] += 1
        push = PUSH.match(line)
        if push:
            streams[current]["sources"].append(push.group("src"))
    return streams


def authored_names(rql_path):
    """Zbiór nazw strumieni nadanych przez autora w `.rql`."""
    return {m.group("name") for m in RQL_STREAM.finditer(rql_path.read_text())}


def classify(streams, authored):
    """Substraty = strumienie planu o nazwie, której autor nie napisał."""
    return sorted(name for name in streams if name not in authored)


def cell(plan_path, probe_path, rql_path):
    """Liczby mechanizmu jednej komórki (profil × plan)."""
    streams = parse_plan(plan_path)
    authored = authored_names(rql_path)
    substrates = classify(streams, authored)

    rewrites = re.findall(r"^REWRITE_APPLIED r1=(\d+) r2=(\d+) r3=(\d+)$",
                          probe_path.read_text(), re.M)
    if len(rewrites) != 1:
        raise ValueError(f"{probe_path}: oczekiwano jednego pelnego wiersza REWRITE_APPLIED")
    r1, r2, r3 = (int(x) for x in rewrites[0])

    consumers = {
        name: sum(1 for s in streams.values() if name in s["sources"]) for name in substrates
    }
    units = sum(
        (1 / streams[name]["interval"]) * canonical_bytes(streams[name]["fields"])
        for name in substrates
    ) / (UNIT_RATE * canonical_bytes(1))
    public = [name for name in streams if name in authored and streams[name]["sources"]]
    public_appends = sum(1 / streams[name]["interval"] for name in public) / UNIT_RATE

    return {
        "r1": r1,
        "r2": r2,
        "r3": r3,
        "selects": [n for n in substrates if n.startswith("STREAM_SELECT_")],
        "substrates": substrates,
        "consumers": consumers,
        "units": float(units),
        "public_streams": len(public),
        "public_appends": float(public_appends),
    }


def report(plan_dir, rql_dir, plans, profiles, verbose=True):
    """Tabela dla listy planów. Zwraca {(profil, plan): komórka}."""
    out = {}
    for plan in plans:
        if verbose:
            print(f"\n=== {plan} ===")
            print(f"{'profil':<13} {'r1':>3} {'r2':>3} {'r3':>3} {'SELECT_':>8} {'substraty':>10} {'jednostki':>10}")
        for profile in profiles:
            plan_file = plan_dir / f"{profile}_{plan}.plan"
            probe_file = plan_dir / f"{profile}_{plan}.probe"
            rql_file = rql_dir / f"{plan}.rql"
            for path in (plan_file, probe_file, rql_file):
                if not path.exists():
                    sys.exit(f"brak {path}")
            data = cell(plan_file, probe_file, rql_file)
            out[(profile, plan)] = data
            if verbose:
                print(
                    f"{profile:<13} {data['r1']:>3} {data['r2']:>3} {data['r3']:>3} {len(data['selects']):>8} "
                    f"{len(data['substrates']):>10} {data['units']:>10.3f}"
                )
                for name in data["substrates"]:
                    s = parse_plan(plan_file)[name]
                    print(
                        f"    {name:<52} {str(s['interval']):>7}  pol={s['fields']}"
                        f"  konsumentow={data['consumers'][name]}"
                    )
    return out


# ─── Bramka o znanej odpowiedzi ──────────────────────────────────────────────
#
# Reguła łuku K26/K24: bramka musi umieć odróżnić wersję OBALONĄ. Sprawdzenie
# „nowy skrypt daje sensowne liczby" jej nie spełnia — spełniają ją dopiero DWA
# warunki naraz:
#
#   (a) na sześciu planach pilota, na których stoi zamknięty RAPORT_PILOTA.md §2,
#       nowa klasyfikacja daje liczby IDENTYCZNE ze starą — poprawka nie rusza
#       niczego, czego ruszać nie miała;
#   (b) na planach kontrolnych nowa klasyfikacja RÓŻNI SIĘ od starej dokładnie o
#       publiczne nazwy, których mutant nie rozpoznaje: `z1`, `z2` oraz w F9-R1
#       `STREAM_HASH_CA_CB` — gdyby nie różniła się niczym, poprawka nie byłaby
#       dowiedziona.
#
# Warunek (b) jest tym, którego brak przepuścił defekt w wersji pilota.

GATE_MAIN_PLANS = ["F9_R2_Q8", "F9_R1_Q8", "F9_X_Q8"]
GATE_CONTROL_PLANS = ["F9_R2_controls", "F9_R1_controls", "F9_X_controls"]

#: Stara klasyfikacja — przepisana z `pilot/mechanism_table.py` DOSŁOWNIE, żeby
#: bramka porównywała się z wersją obaloną, a nie z jej opisem.
LEGACY_USER = re.compile(r"(m|q|n|d|x|i|h|mm|collide_user)\d*")


def legacy_classify(streams):
    declared = {n for n, s in streams.items() if not s["sources"]}
    user = {n for n in streams if LEGACY_USER.fullmatch(n)}
    return sorted(n for n in streams if n not in declared and n not in user)


def gate():
    plan_dir = HERE / "pilot" / "out"
    rql_dir = HERE / "rql"
    if not plan_dir.exists():
        sys.exit("brak zrzutow pilota — bramki nie da sie uruchomic")

    failures, differences = [], []

    for plan in GATE_MAIN_PLANS + GATE_CONTROL_PLANS:
        for profile in PROFILES:
            plan_file = plan_dir / f"{profile}_{plan}.plan"
            streams = parse_plan(plan_file)
            new = classify(streams, authored_names(rql_dir / f"{plan}.rql"))
            old = legacy_classify(streams)
            if plan in GATE_MAIN_PLANS and new != old:
                failures.append(
                    f"(a) {profile}/{plan}: nowa klasyfikacja rusza plan glowny "
                    f"(dodane {sorted(set(new) - set(old))}, usuniete {sorted(set(old) - set(new))})"
                )
            data = cell(plan_file, plan_dir / f"{profile}_{plan}.probe", rql_dir / f"{plan}.rql")
            if plan in GATE_MAIN_PLANS and data["r3"] != 0:
                failures.append(f"(d) {profile}/{plan}: R3 wykonala {data['r3']} przepisan")
            if plan not in GATE_MAIN_PLANS and new != old:
                differences.append((profile, plan, sorted(set(old) - set(new)), sorted(set(new) - set(old))))

    removed = {tuple(d[2]) for d in differences}
    added = {tuple(d[3]) for d in differences}
    if not differences:
        failures.append(
            "(b) nowa klasyfikacja nie rozni sie od starej na ZADNYM planie kontrolnym "
            "— poprawka nie jest dowiedziona"
        )
    elif removed != {("z1", "z2"), ("STREAM_HASH_CA_CB", "z1", "z2")} or added != {()}:
        failures.append(
            f"(b) roznica jest inna niz oczekiwane publiczne z1/z2/STREAM_HASH_CA_CB: "
            f"usuniete={sorted(removed)}, dodane={sorted(added)}"
        )

    # (c) Kontrola pozytywna samej metryki: publiczny strumien o nazwie konwencji
    #     kompilatora musi trafic do MIANOWNIKA, a nie zniknac z rachunku.
    ctrl = cell(
        plan_dir / "DEFAULT_F9_R1_controls.plan",
        plan_dir / "DEFAULT_F9_R1_controls.probe",
        rql_dir / "F9_R1_controls.rql",
    )
    if "STREAM_HASH_CA_CB" in ctrl["substrates"]:
        failures.append("(c) STREAM_HASH_CA_CB nadal liczony jako substrat")
    if ctrl["public_appends"] <= 0:
        failures.append("(c) mianownik pusty — publiczne zapisy nie sa liczone")

    for line in failures:
        print(f"BLAD BRAMKI: {line}", file=sys.stderr)
    if failures:
        return 1

    print(f"OK: (a) {len(GATE_MAIN_PLANS) * len(PROFILES)} komorek planow glownych bez zmian")
    print(f"OK: (b) roznica wobec wersji obalonej obejmuje dokladnie z1/z2 i STREAM_HASH_CA_CB "
          f"({len(differences)} komorek kontrolnych)")
    print("OK: (c) publiczny strumien konwencji kompilatora liczy sie do mianownika")
    print("OK: (d) R3 nie przepisuje zadnego glownego planu K26")
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--gate", action="store_true", help="bramka o znanej odpowiedzi")
    parser.add_argument("--plan-dir", type=Path, default=HERE / "pilot" / "out")
    parser.add_argument("--rql-dir", type=Path, default=HERE / "rql")
    parser.add_argument("--plans", nargs="*", default=GATE_MAIN_PLANS + GATE_CONTROL_PLANS)
    parser.add_argument("--profiles", nargs="*", default=PROFILES)
    args = parser.parse_args()

    if args.gate:
        return gate()
    report(args.plan_dir, args.rql_dir, args.plans, args.profiles)
    return 0


if __name__ == "__main__":
    sys.exit(main())
