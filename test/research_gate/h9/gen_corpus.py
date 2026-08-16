#!/usr/bin/env python3
"""Generator korpusu kampanii K26 — dane główne, dane kalibracyjne i plany RQL.

JEDYNE źródło danych i planów kampanii. Wszystko, co ten skrypt wypisuje, jest
funkcją zamrożonych stałych z predeklaracji kampanii K26v3 §4 i §6 — bez wejścia z zewnątrz,
bez zegara, bez `random` biblioteki standardowej (jej strumień zależy od wersji
Pythona, a korpus ma być identyczny na hoście i na workerze).

Generator PRNG: SplitMix64 — algorytm zapisany tutaj w całości, więc odtworzenie
korpusu nie zależy od żadnej biblioteki.

Uruchomienie:
    ./gen_corpus.py            # zapisuje data/ i rql/
    ./gen_corpus.py --check    # tylko sprawdza, że pliki na dysku zgadzają się
                               # z tym, co generator wypisałby teraz (kod 1 przy
                               # rozbieżności) — bramka niezmienności korpusu
"""
import argparse
import hashlib
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent

# ─── Zamrożone stałe korpusu (predeklaracji kampanii K26v3 §4) ────────────────────────────

#: Liczba rekordów źródła szybkiego taktu. Źródło wolnego taktu ma dokładnie połowę.
#: Ta sama liczba obowiązuje obie maszyny: po stronie Flinka jest to `--slots`.
RECORDS_FAST = 3000
RECORDS_SLOW = RECORDS_FAST // 2

#: Ziarna. Rozdzielone, bo dane kalibracyjne NIE MOGĄ być danymi głównymi (§10).
SEED_MAIN = 20260809_2601
SEED_CALIB = 20260809_2602

#: Zakres wartości źródła. Górna granica dobrana tak, by `A*C + B*D` przy czterech
#: źródłach nie wyszło poza zakres 64-bitowy z ogromnym zapasem, a `Sqrt` miało
#: sensowny argument. Nie ma wpływu na metrykę pierwotną (ta zależy wyłącznie od
#: deskryptora i liczby zapisów) — jest tu, żeby korpus był w ogóle określony.
VALUE_MAX = 1000

#: Dane kalibracyjne są krótsze: kalibracja szuka rate'u, nie liczy metryki.
CALIB_FAST = 600

#: Siatka Q (§10).
Q_GRID = [1, 2, 4, 8, 16, 32]

# ─── Rodziny ─────────────────────────────────────────────────────────────────
#
# `forms` są w ZAMROŻONEJ kolejności — ta sama, którą realizuje `K26Ops.formOf`
# po stronie Flinka. Dla F9-X kolejność to W1, W4, W2, W3 (SZKIC_RODZIN.md §6.2):
# pierwsza para różni się w obu wymiarach naraz, więc przy Q=4 rodzina nadal
# dotyka obu mechanizmów.

FAMILIES = {
    "F9-R2": {
        "f_max": 2,
        "sources": [
            ("v", "A", "1/100", "axis_x.txt", RECORDS_FAST),
            ("v", "B", "1/100", "axis_y.txt", RECORDS_FAST),
        ],
        "select": "Sqrt(A[0]*A[0]+B[0]*B[0])",
        "forms": ["A+B", "B+A"],
        "form_names": ["P1", "P2"],
    },
    "F9-R1": {
        "f_max": 2,
        "sources": [
            ("v", "A", "1/100", "vib.txt", RECORDS_FAST),
            ("v", "B", "1/50", "cur.txt", RECORDS_SLOW),
        ],
        "select": None,  # program pola odwołuje się do własnego strumienia — patrz niżej
        "forms": ["(A>2)#(B>1)", "(A#B)>3"],
        "form_names": ["P1", "P2"],
    },
    "F9-X": {
        "f_max": 4,
        "sources": [
            ("front", "A", "1/100", "front_vib.txt", RECORDS_FAST),
            ("front", "B", "1/50", "front_cur.txt", RECORDS_SLOW),
            ("rear", "C", "1/100", "rear_vib.txt", RECORDS_FAST),
            ("rear", "D", "1/50", "rear_cur.txt", RECORDS_SLOW),
        ],
        "select": "Sqrt(front*front+rear*rear)",
        "forms": [
            "((A>2)#(B>1)) + ((C>2)#(D>1))",
            "((C#D)>3) + ((A#B)>3)",
            "((C>2)#(D>1)) + ((A>2)#(B>1))",
            "((A#B)>3) + ((C#D)>3)",
        ],
        "form_names": ["W1", "W4", "W2", "W3"],
    },
}


def splitmix64(state):
    """Jeden krok SplitMix64. Zwraca (nowy_stan, wartość 64-bitowa)."""
    state = (state + 0x9E3779B97F4A7C15) & 0xFFFFFFFFFFFFFFFF
    z = state
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & 0xFFFFFFFFFFFFFFFF
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & 0xFFFFFFFFFFFFFFFF
    return state, z ^ (z >> 31)


def series(seed, count):
    """Deterministyczny ciąg `count` wartości całkowitych z [0, VALUE_MAX]."""
    state, out = seed & 0xFFFFFFFFFFFFFFFF, []
    for _ in range(count):
        state, value = splitmix64(state)
        out.append(value % (VALUE_MAX + 1))
    return out


def source_seed(base, name):
    """Ziarno pojedynczego źródła — pochodna ziarna zestawu i nazwy pliku.

    Każde źródło ma własny strumień wartości; gdyby dwa źródła dostały ten sam,
    `A+B` i `A#B` liczyłyby się na zduplikowanym sygnale, co nie zmienia metryki,
    ale utrudnia czytanie oracle'a przy diagnozie rozbieżności.
    """
    digest = hashlib.sha256(f"{base}:{name}".encode()).digest()
    return int.from_bytes(digest[:8], "big")


def formsForQ(q, f_max):
    """`F(Q) = min(F_max, floor(Q/2))`, minimum 1 — reguła zamrożona 2026-08-08."""
    return max(1, min(f_max, q // 2))


def formOf(i, q, f_max):
    """Numer postaci (0-based) monitora `i`. Odpowiednik `K26Ops.formOf`."""
    return (i * formsForQ(q, f_max)) // q


def monitor_select(family, name):
    """Program pola monitora.

    F9-R1 odwołuje się do WŁASNEGO strumienia (`m1[0]*m1[0]`) — wzorzec
    `dedup_shifted` z `optimizer_ablation`. W tej rodzinie jest to zamierzone:
    współdzielenie realizuje R1 + dedup substratów, a nie przejście R2, więc
    dyskwalifikacja z SZKIC_RODZIN.md §2 (U-3) niczego tu nie psuje.
    """
    if FAMILIES[family]["select"] is None:
        return f"{name}[0]*{name}[0]"
    return FAMILIES[family]["select"]


def render_family(family, q):
    """Plan RQL rodziny dla danego Q."""
    spec = FAMILIES[family]
    forms, f_max = spec["forms"], spec["f_max"]
    active = formsForQ(q, f_max)
    lines = [
        f"# {family}, Q={q} — plan kampanii K26. Wygenerowany przez gen_corpus.py;",
        "# nie edytować ręcznie (bramka: `gen_corpus.py --check`).",
        f"# Postacie czynne przy tym Q: {active} z {f_max}"
        f" ({', '.join(spec['form_names'][:active])}).",
        "",
        "STORAGE 'temp'",
        "SUBSTRAT 'memory'",
        "",
    ]
    for field, name, interval, path, _ in spec["sources"]:
        lines.append(f"DECLARE {field} INTEGER STREAM {name}, {interval:<6} FILE '{path}'")
    lines.append("")
    previous = None
    for i in range(q):
        form = formOf(i, q, f_max)
        if form != previous:
            lines.append(f"# postać {spec['form_names'][form]}")
            previous = form
        monitor = f"m{i + 1}"
        lines.append(f"SELECT {monitor_select(family, monitor)} STREAM {monitor} FROM {forms[form]}")
    return "\n".join(lines) + "\n"


def render_controls(family):
    """Kontrole negatywne i near-miss rodziny, nad danymi głównymi.

    Treść przeniesiona z planów kontrolnych pilota (`pilot/F9_*_controls.rql`),
    które przeszły w P4 — zmieniają się wyłącznie nazwy plików źródeł, bo pilot
    biegał na danych miniaturowych, a bramki P6 biegną na danych głównych.
    Kontrola `Q=1` NIE jest tu powtórzona: realizuje ją komórka `Q=1` siatki.
    """
    fast = {"F9-R2": "axis_x.txt", "F9-R1": "vib.txt", "F9-X": "front_vib.txt"}[family]
    slow = {"F9-R2": "axis_y.txt", "F9-R1": "cur.txt", "F9-X": "front_cur.txt"}[family]
    third = {"F9-R2": "axis_y.txt", "F9-R1": "cur.txt", "F9-X": "rear_vib.txt"}[family]
    fourth = {"F9-X": "rear_cur.txt"}.get(family, slow)

    head = [
        f"# {family} — kontrole near-miss na danych głównych. Wygenerowane przez",
        "# gen_corpus.py; nie edytować ręcznie (bramka: `gen_corpus.py --check`).",
        "#",
        "# Kryterium wspólne: nad źródłami kontrolnymi NIE MOŻE powstać wspólny",
        "# substrat. Kontrola Q=1 jest osobną komórką siatki, nie tym plikiem.",
        "",
        "STORAGE 'temp'",
        "SUBSTRAT 'memory'",
        "",
    ]

    if family == "F9-R2":
        body = [
            "# near-miss 1: zmieniona kolejność pól wyniku",
            f"DECLARE v INTEGER STREAM NA, 1/100 FILE '{fast}'",
            f"DECLARE v INTEGER STREAM NB, 1/100 FILE '{slow}'",
            "SELECT NA[0],NB[0] STREAM n1 FROM NA+NB",
            "SELECT NB[0],NA[0] STREAM n2 FROM NB+NA",
            "",
            "# near-miss 2: SELECT * ujawnia kolejność wejścia",
            f"DECLARE v INTEGER STREAM SA, 1/100 FILE '{fast}'",
            f"DECLARE v INTEGER STREAM SB, 1/100 FILE '{slow}'",
            "SELECT * STREAM d1 FROM SA+SB",
            "SELECT * STREAM d2 FROM SB+SA",
            "",
            "# near-miss 3: inne grupowanie trzech źródeł (brak reasocjacji)",
            f"DECLARE v INTEGER STREAM GA, 1/100 FILE '{fast}'",
            f"DECLARE v INTEGER STREAM GB, 1/100 FILE '{slow}'",
            f"DECLARE v INTEGER STREAM GC, 1/100 FILE '{third}'",
            "SELECT GA[0]*GB[0]+GC[0] STREAM x1 FROM (GA+GB)+GC",
            "SELECT GA[0]*GB[0]+GC[0] STREAM x3 FROM (GC+GB)+GA",
            "",
            "# kontrola bez etapu materializowanego: oczekiwane ZERO bajtów substratów",
            f"DECLARE v INTEGER STREAM ZA, 1/100 FILE '{fast}'",
            "SELECT Sqrt(ZA[0]*ZA[0]) STREAM z1 FROM ZA",
            "SELECT Sqrt(ZA[0]*ZA[0]) STREAM z2 FROM ZA",
        ]
    elif family == "F9-R1":
        body = [
            "# near-miss 1: niedopasowane przesunięcie, 2*(1/100) != 2*(1/50)",
            f"DECLARE v INTEGER STREAM MA, 1/100 FILE '{fast}'",
            f"DECLARE v INTEGER STREAM MB, 1/50  FILE '{slow}'",
            "SELECT mm[0]*mm[0] STREAM mm FROM (MA>2)#(MB>2)",
            "",
            "# near-miss 2: równoważne postacie nad RÓŻNYMI instancjami źródeł",
            f"DECLARE v INTEGER STREAM IA, 1/100 FILE '{fast}'",
            f"DECLARE v INTEGER STREAM IB, 1/50  FILE '{slow}'",
            f"DECLARE v INTEGER STREAM IA2, 1/100 FILE '{fast}'",
            f"DECLARE v INTEGER STREAM IB2, 1/50  FILE '{slow}'",
            "SELECT i1[0]*i1[0] STREAM i1 FROM (IA>2)#(IB>1)",
            "SELECT i2[0]*i2[0] STREAM i2 FROM (IA2#IB2)>3",
            "",
            "# near-miss 3: granica obserwowalności — publiczny strumień nazwany",
            "# konwencją kompilatora, o przestawionym schemacie",
            f"DECLARE cx INTEGER, cy INTEGER STREAM CA, 1/100 FILE '{fast}'",
            f"DECLARE cx INTEGER, cy INTEGER STREAM CB, 1/50  FILE '{slow}'",
            "SELECT STREAM_HASH_CA_CB[1],STREAM_HASH_CA_CB[0] STREAM STREAM_HASH_CA_CB FROM CA#CB",
            "SELECT * STREAM collide_user FROM (CA>2)#(CB>1)",
            "",
            "# kontrola bez etapu materializowanego: oczekiwane ZERO bajtów substratów",
            f"DECLARE v INTEGER STREAM ZA, 1/100 FILE '{fast}'",
            "SELECT ZA[0]*ZA[0] STREAM z1 FROM ZA",
            "SELECT ZA[0]*ZA[0] STREAM z2 FROM ZA",
        ]
    else:
        body = [
            "# near-miss: jedna para dopasowana, druga nie",
            f"DECLARE front INTEGER STREAM HA, 1/100 FILE '{fast}'",
            f"DECLARE front INTEGER STREAM HB, 1/50  FILE '{slow}'",
            f"DECLARE rear INTEGER STREAM HC, 1/100 FILE '{third}'",
            f"DECLARE rear INTEGER STREAM HD, 1/50  FILE '{fourth}'",
            "SELECT Sqrt(front*front+rear*rear) STREAM h1 FROM ((HA>2)#(HB>1)) + ((HC>2)#(HD>2))",
            "SELECT Sqrt(front*front+rear*rear) STREAM h2 FROM ((HC#HD)>3) + ((HA#HB)>3)",
            "",
            "# kontrola bez etapu materializowanego: oczekiwane ZERO bajtów substratów",
            f"DECLARE v INTEGER STREAM ZA, 1/100 FILE '{fast}'",
            "SELECT Sqrt(ZA[0]*ZA[0]) STREAM z1 FROM ZA",
            "SELECT Sqrt(ZA[0]*ZA[0]) STREAM z2 FROM ZA",
        ]
    return "\n".join(head + body) + "\n"


def corpus():
    """Cały korpus jako {ścieżka_względna: treść}. Jedyna definicja zawartości."""
    out = {}

    for name, seed, fast_count in (("main", SEED_MAIN, RECORDS_FAST), ("calib", SEED_CALIB, CALIB_FAST)):
        emitted = set()
        for family in FAMILIES.values():
            for _, _, _, path, records in family["sources"]:
                if path in emitted:
                    continue
                emitted.add(path)
                count = fast_count if records == RECORDS_FAST else fast_count // 2
                values = series(source_seed(seed, path), count)
                out[f"data/{name}/{path}"] = "".join(f"{v}\n" for v in values)

    for family in FAMILIES:
        slug = family.replace("-", "_")
        for q in Q_GRID:
            out[f"rql/{slug}_Q{q}.rql"] = render_family(family, q)
        out[f"rql/{slug}_controls.rql"] = render_controls(family)

    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="tylko porównaj z dyskiem, nic nie zapisuj")
    args = parser.parse_args()

    files = corpus()

    if args.check:
        bad = []
        for rel, content in sorted(files.items()):
            path = HERE / rel
            if not path.exists():
                bad.append(f"brak pliku: {rel}")
            elif path.read_text() != content:
                bad.append(f"treść rozjechana z generatorem: {rel}")
        extra = set()
        for root in ("data", "rql"):
            base = HERE / root
            if base.exists():
                for path in base.rglob("*"):
                    if path.is_file() and str(path.relative_to(HERE)) not in files:
                        extra.add(str(path.relative_to(HERE)))
        for rel in sorted(extra):
            bad.append(f"plik spoza korpusu: {rel}")
        if bad:
            for line in bad:
                print(f"BLAD: {line}", file=sys.stderr)
            return 1
        print(f"OK: korpus zgodny z generatorem ({len(files)} plikow)")
        return 0

    for rel, content in sorted(files.items()):
        path = HERE / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content)
    print(f"OK: zapisano {len(files)} plikow korpusu")
    return 0


if __name__ == "__main__":
    sys.exit(main())
