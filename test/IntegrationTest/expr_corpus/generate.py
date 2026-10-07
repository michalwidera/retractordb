#!/usr/bin/env python3
"""Generator korpusu wyrazen arytmetycznych (#289).

Korpus sa plany `<plan>.rql` z danymi `<plan>*.txt`, zapisane w tym katalogu. Test ich nie
generuje - czyta pliki z repozytorium. Generator jest po to, zeby korpus dalo sie odtworzyc
i rozbudowac: stale ziarno, wiec ponowne uruchomienie daje te same bajty.

Dwie czesci:

* plany wlasne `own_*` - po jednym na typ (INTEGER, UINT, BYTE, FLOAT, DOUBLE, RATIONAL z
  reduktora), typy mieszane w obu kierunkach promocji, napisy dluzsze niz SSO, gleboki
  program i ogon stalych (`own_tail`, regula B w exprSimplify). Porownania zyja tylko
  w RULE WHEN, wiec kazdy plan poza `own_deep` i `own_tail` ma regule na strumieniu-kopii
  z warunkiem, ktory nie jest prawdziwy dla zadnego wiersza - liczy sie w kazdym slocie, a
  akcja nie odpala;
* plany `rehost_*` - wyrazenia z list SELECT planow testow systemowych (pliki `.rql` sledzone
  przez git), przeniesione na wspolne zrodlo: `nazwa[k]` -> `src[k mod 3]`, goly identyfikator
  pola -> kolejne `src[i]`. Pomijane sa ksztalty, ktorych nie da sie przeniesc bez kontekstu
  planu: indeks `_`, generator `$`, agregaty okna, literaly tekstowe.

Dane maja rozszerzenie `.txt`, bo tylko takie idzie przez zrodlo tekstowe; inne rozszerzenie
czytane jest jako surowe bajty rekordu.

Zrodla nie sa ONESHOT: test przechodzi dane raz (`xretractor -f -u`), a pomiar kosztu
(`-f -m N`) dostaje zrodla zawijajace sie po koncu pliku.

    python3 generate.py            # z tego katalogu; nadpisuje *.rql i *.txt
"""
import random
import re
import subprocess
from pathlib import Path

ROWS = 64
HERE = Path(__file__).resolve().parent

SAME_TYPE = [
    "{0}+{1}",
    "{0}-{1}",
    "{0}*{2}",
    "{0}/{1}",
    "({0}+{1})*({1}-{2})",
    "{0}*{0}+{1}*{1}",
    "({0}-{1})/({2}+{2}+{1})",
    "{0}+{1}+{2}-{0}*{1}",
    "{0}^2",
]

SAME_TYPE_RULE = "{0} > {big} OR {1} < {neg} OR {0} >= {1} AND {2} = {big} OR {1} != {1}"

# Ogon stalych `(E op c1) op c2` - ksztalt, ktory regula B w exprSimplify zwija do `E op (c1 ? c2)`.
# Stala zwinieta bywa ujemna (`3-5`), a ujemna nie ma reprezentacji w UINT: do poprawki `u+3-5`
# dawalo przy RDB_OPT_SIMPLIFY_EXPRESSIONS=ON NULL w kazdym wierszu, a przy ablacji u-2. Ostatnie
# trzy daja stala nieujemna; straznik NULL nadal moze odmowic przepisania. Zadne nie zalezy od drabiny RQL.g4.
TAIL = [
    "{0}+3-5",
    "3+{0}-5",
    "({0}-3)+5",
    "({0}+3-5)+7",
    "{0}+1-5",
    "{0}+1/2-3",
    "{0}+5-3",
    "{0}-1-1",
    "{0}*2*3",
]

# Tylko w planie UINT: ogon stalych ze zgloszenia regresji po da67e5a3, zeby `own_uint` sam pilnowal
# typu, w ktorym zwinieta stala -2 nie ma reprezentacji. Pelny zestaw ksztaltow ma `own_tail`.
UINT_ONLY = ["{0}+3-5"]


def rows_int(rng, lo, hi, nonzero_col=None):
    out = []
    for _ in range(ROWS):
        row = [rng.randint(lo, hi) for _ in range(3)]
        if nonzero_col is not None and row[nonzero_col] == 0:
            row[nonzero_col] = 1
        out.append(" ".join(str(v) for v in row))
    return out


def rows_real(rng):
    return [" ".join(f"{rng.uniform(-1000, 1000):.4f}" for _ in range(3)) for _ in range(ROWS)]


def plan_same(plan, ftype, big, neg=None, extra=()):
    refs = ["src[0]", "src[1]", "src[2]"]
    exprs = ", ".join(e.format(*refs) for e in SAME_TYPE + list(extra))
    rule = SAME_TYPE_RULE.format("cpy[0]", "cpy[1]", "cpy[2]", big=big, neg=f"-{big}" if neg is None else neg)
    return f"""# {ftype} op {ftype} - droga normalize() bez promocji.
STORAGE 'temp'

DECLARE x {ftype}, y {ftype}, z {ftype} STREAM src, 1 FILE '{plan}.txt'

SELECT {exprs} STREAM ari FROM src

SELECT src[0], src[1], src[2] STREAM cpy FROM src

RULE cmp ON cpy WHEN {rule} DO DUMP -1 TO 0
"""


# --- czesc przeniesiona z testow systemowych ------------------------------------------------

SELECT = re.compile(r"\bSELECT\b(?P<sel>.*?)\bSTREAM\b", re.I | re.S)
SKIP = re.compile(r"\[_\]|\$|\b(MIN|MAX|AVG|SUMC)\s*\(|'|\bto_float\b|\bto_double\b")
FUNCS = {"Sqrt", "sqrt", "int", "float", "to_integer", "null2zero", "Abs", "abs", "floor", "Floor", "real", "str"}


def split_top(sel):
    out, depth, cur = [], 0, []
    for ch in sel:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        elif ch == "," and depth == 0:
            out.append("".join(cur).strip())
            cur = []
            continue
        cur.append(ch)
    out.append("".join(cur).strip())
    return out


def has_arith(expr):
    depth = 0
    for i, ch in enumerate(expr):
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        elif depth == 0 and ch in "+*/^":
            return True
        elif depth == 0 and ch == "-" and expr[:i].strip() and expr[:i].strip()[-1] not in "(,":
            return True
    return False


def rehost(expr):
    expr = re.sub(r"\b[A-Za-z_]\w*\[(\d+)\]", lambda m: f"src[{int(m.group(1)) % 3}]", expr)
    names = {}

    def bare(m):
        word = m.group(0)
        if word in FUNCS or word == "src":
            return word
        names.setdefault(word, len(names) % 3)
        return f"src[{names[word]}]"

    return re.sub(r"(?<![\w\[])[A-Za-z_]\w*(?![\w\(])", bare, expr)


def system_expressions():
    repo = HERE.parents[2]
    files = subprocess.run(["git", "ls-files", "*.rql"], cwd=repo, capture_output=True, text=True, check=True)
    exprs = []
    for rel in files.stdout.split():
        if rel.startswith("test/IntegrationTest/expr_corpus/"):
            continue
        text = (repo / rel).read_text(errors="replace")
        text = "\n".join("" if line.lstrip().startswith("#") else line for line in text.splitlines())
        for m in SELECT.finditer(text):
            for e in split_top(m.group("sel").replace("\\", " ")):
                if e == "*" or not has_arith(e) or SKIP.search(e):
                    continue
                e = " ".join(rehost(e).split())
                if e not in exprs:
                    exprs.append(e)
    return exprs


def write(name, text):
    (HERE / name).write_text(text if text.endswith("\n") else text + "\n")


def issue327_boundaries():
    # Osobny zestaw deterministyczny; nie przesuwa ziaren dotychczasowego korpusu.
    write("own_overflow.rql", """# Regula B (#327): posredni NULL musi pozostac przy ON i all-off.
STORAGE 'temp'
DECLARE i INTEGER, u UINT, b BYTE STREAM src, 1 TEXTFILE 'own_overflow.txt'
DECLARE r INTEGER STREAM rat, 1 TEXTFILE 'own_overflow_rat.txt'
SELECT src[0], src[1], src[2], src[1]*-2*-3, (src[0]+1)-1, src[0]*-1*-1, src[1]+5-3, (src[2]+src[2])*8388608*-1, src[1]+3-5 STREAM bounds FROM src
SELECT rat[0]/2147483647 STREAM ratio FROM AVG(rat)
SELECT ratio[0], (ratio[0]+1)-1 STREAM rational_result FROM ratio
""")
    write("own_overflow.txt", "2147483647 1 128\n-2147483648 4294967291 0\n0 0 255\n1 10 127\n7 4294967295 1\nNULL NULL NULL")
    write("own_overflow_rat.txt", "1\n-1\n0\n2147483647\n-2147483648\nNULL")


def main():
    rng = random.Random(20260926)

    for plan, ftype, big, neg, data in (
        ("own_int", "INTEGER", 100000000, None, lambda: rows_int(rng, -1000, 1000, nonzero_col=1)),
        ("own_uint", "UINT", 100000000, "0", lambda: rows_int(rng, 1, 1000)),
        ("own_byte", "BYTE", 250, "0", lambda: rows_int(rng, 1, 15)),
        ("own_double", "DOUBLE", 100000000, None, lambda: rows_real(rng)),
        ("own_float", "FLOAT", 100000000, None, lambda: rows_real(rng)),
    ):
        write(f"{plan}.rql", plan_same(plan, ftype, big, neg, UINT_ONLY if ftype == "UINT" else ()))
        write(f"{plan}.txt", "\n".join(data()))

    # RATIONAL: DECLARE go nie zna, wiec powstaje z reduktora - srednia trzech INTEGER-ow ma
    # w ogolnosci mianownik 3.
    write("own_rational.rql", """# RATIONAL op RATIONAL - srednia z reduktora, mianownik 3.
STORAGE 'temp'

DECLARE x INTEGER, y INTEGER, z INTEGER STREAM p, 1 FILE 'own_rational_p.txt'
DECLARE x INTEGER, y INTEGER, z INTEGER STREAM q, 1 FILE 'own_rational_q.txt'

SELECT p[0] STREAM ra FROM p.avg
SELECT q[0] STREAM rb FROM q.avg

SELECT ra[0]+ra[0], ra[0]*ra[0], ra[0]-ra[0]*ra[0], (ra[0]+ra[0])*(ra[0]-ra[0]*ra[0]), ra[0]^2, ra[0]*ra[0]+ra[0] STREAM rself FROM ra

RULE cmp ON ra WHEN ra[0] > ra[0]*ra[0]+1000 OR ra[0]*ra[0] < ra[0]-ra[0] OR ra[0] != ra[0] DO DUMP -1 TO 0
""")
    write("own_rational_p.txt", "\n".join(rows_int(rng, -50, 50)))
    write("own_rational_q.txt", "\n".join(rows_int(rng, -50, 50)))

    # Typy mieszane - droga promocji w obu kierunkach (nizszy indeks z lewej i z prawej).
    mixed = [f"{rng.randint(-1000, 1000)} {rng.uniform(-100, 100):.4f} {rng.uniform(-100, 100):.3f} "
             f"{rng.randint(1, 200)} {rng.randint(1, 100000)}" for _ in range(ROWS)]
    write("own_mixed.rql", """# Typy mieszane: promocja nizszego indeksu wariantu, w obu kierunkach. Para INTEGER/UINT liczy sie
# dokladnie i dopiero wynik trafia do UINT: `m[4]*m[0]` dla ujemnego m[0] daje NULL, `m[4]+m[0]` wartosc,
# gdy suma sie miesci.
STORAGE 'temp'

DECLARE i INTEGER, d DOUBLE, f FLOAT, y BYTE, u UINT STREAM m, 1 FILE 'own_mixed.txt'

SELECT m[0]+m[1], m[1]+m[0], m[0]*m[2], m[2]*m[0], m[1]-m[2], m[2]-m[1], m[3]+m[0], m[0]+m[3], m[4]*m[0], m[0]*m[4], m[0]/m[1], m[1]/m[4], (m[0]+m[1])*(m[2]-m[3]), m[0]*2.5, 0.5*m[0], m[3]^2, m[0]^0.5, m[4]+m[0] STREAM mix FROM m

SELECT m[0], m[1], m[2], m[3], m[4] STREAM cpy FROM m

RULE cmp ON cpy WHEN cpy[0] > cpy[1]*cpy[1]+1000000 OR cpy[2] < cpy[0]-100000 OR cpy[3] >= cpy[4]*1000 OR cpy[1] != cpy[1] DO DUMP -1 TO 0
""")
    write("own_mixed.txt", "\n".join(mixed))

    # Napisy: konkatenacja wychodzi poza SSO (15 bajtow), wiec tu kopia wariantu alokuje.
    words = ["alpha_sensor_node", "beta_sensor_node_long", "gamma", "delta_channel_0042", "epsilon_ch",
             "zeta_measurement_unit", "eta", "theta_value_stream_x"]
    write("own_string.rql", """# STRING + STRING i napis + liczba - wartosci dluzsze niz SSO.
STORAGE 'temp'

DECLARE s STRING[32], t STRING[32], k INTEGER STREAM w, 1 FILE 'own_string.txt'

SELECT w[0]+w[1], w[0]+'_suffix_beyond_sso', to_string(w[2]:16)+w[0], w[1]+to_string(w[2]:8)+w[0] STREAM cat FROM w

SELECT w[0], w[1], w[2] STREAM cpy FROM w

RULE cmp ON cpy WHEN cpy[0] = 'no_such_word_in_data' OR cpy[1] < 'AAAA' OR cpy[0] = cpy[1] AND cpy[2] > 100000 DO DUMP -1 TO 0
""")
    write("own_string.txt", "\n".join(f"{rng.choice(words)} {rng.choice(words)} {rng.randint(0, 9999)}"
                                       for _ in range(ROWS)))

    # Gleboki program: kilkadziesiat operatorow na pole - gorna granica kosztu ewaluatora.
    deep_int = "+".join(f"(src[{i % 3}]*src[{(i + 1) % 3}]-src[{(i + 2) % 3}])" for i in range(12))
    deep_dbl = "+".join(f"(dbl[{i % 3}]*dbl[{(i + 1) % 3}]-dbl[{(i + 2) % 3}])" for i in range(12))
    write("own_deep.rql", f"""# Gleboki program ONP: kilkadziesiat operatorow na pole, INTEGER i DOUBLE.
STORAGE 'temp'

DECLARE x INTEGER, y INTEGER, z INTEGER STREAM src, 1 FILE 'own_deep_int.txt'
DECLARE x DOUBLE, y DOUBLE, z DOUBLE STREAM dbl, 1 FILE 'own_deep_dbl.txt'

SELECT {deep_int}, ({deep_int})/(src[1]*src[1]+1) STREAM dint FROM src
SELECT {deep_dbl}, ({deep_dbl})/(dbl[1]*dbl[1]+1.0) STREAM ddbl FROM dbl
""")
    write("own_deep_int.txt", "\n".join(rows_int(rng, -100, 100)))
    write("own_deep_dbl.txt", "\n".join(rows_real(rng)))

    # Wlasne ziarno: dopisanie planu nie przesuwa danych planow wygenerowanych wyzej. Male u na
    # poczatku trafiaja w granice NULL formy krokowej (`u+1-5` dla u < 4).
    rng_tail = random.Random(20260927)
    tail = ", ".join([e.format("src[0]") for e in TAIL] + [e.format("src[1]") for e in TAIL])
    write("own_tail.rql", f"""# Ogon stalych nad UINT, INTEGER jako kontrola - wynik ten sam przy RDB_OPT_SIMPLIFY_EXPRESSIONS=ON i OFF.
STORAGE 'temp'

DECLARE u UINT, i INTEGER STREAM src, 1 FILE 'own_tail.txt'

SELECT {tail} STREAM tail FROM src
""")
    write("own_tail.txt", "\n".join(f"{u if u < 8 else rng_tail.randint(0, 1000)} {rng_tail.randint(-1000, 1000)}"
                                     for u in range(ROWS)))

    issue327_boundaries()

    exprs = system_expressions()
    rng_sys = random.Random(289)
    for ftype, fmt, gen in (("INTEGER", "{}", lambda: rng_sys.randint(1, 1000)),
                            ("DOUBLE", "{:.4f}", lambda: rng_sys.uniform(1, 1000))):
        plan = f"rehost_{ftype.lower()}"
        write(f"{plan}.rql", f"""# Wyrazenia z list SELECT planow testow systemowych, przeniesione na wspolne zrodlo {ftype}.
STORAGE 'temp'

DECLARE x {ftype}, y {ftype}, z {ftype} STREAM src, 1 FILE '{plan}.txt'

SELECT {', '.join(exprs)} STREAM sysx FROM src
""")
        write(f"{plan}.txt", "\n".join(" ".join(fmt.format(gen()) for _ in range(3)) for _ in range(ROWS)))


if __name__ == "__main__":
    main()
