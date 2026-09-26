#!/usr/bin/env python3
"""Wyrocznia niezalezna dla korpusu wyrazen arytmetycznych.

Liczy kazde pole kazdego rekordu planu z definicji jezyka, nie z kodu silnika: czyta plan
`.rql` i dane `.txt`, parsuje wyrazenia wlasnym parserem i liczy je w Pythonie na modelu
typow RQL. Potem porownuje z tym, co silnik zapisal (wypis `xtrdb` zebrany przez run.sh),
razem z typem kazdego pola z `.desc`.

Model typow (kontrakt, nie implementacja):

* typ wartosci to indeks wariantu descFldVT: BYTE < INTEGER < UINT < RATIONAL < FLOAT <
  DOUBLE < STRING; operator dwuargumentowy podnosi operand o nizszym indeksie do wyzszego;
* rzut bez reprezentacji w typie docelowym jest NULL (ujemna liczba do UINT, spoza int32
  do INTEGER, NaN do typu calkowitego);
* INTEGER, UINT i RATIONAL (licznik i mianownik int32) poza zakresem daja NULL; BYTE op BYTE
  liczy sie w int (promocja C++), FLOAT w float32, dzielenie przez zero daje NULL;
* porownanie i wynik logiczny nad napisem to INTEGER 1/0.

Pierwszenstwo operatorow jest KONWENCJONALNE (`+`/`-` rowne, `*`/`/` rowne, `^` prawostronne).
Gramatyka RQL.g4 ma dzis inna drabine (`a - b + c` liczy jako `a - (b + c)`, `a / b * c` jako
`a / (b * c)`) - to zgloszony defekt, a nie kontrakt. Wyrocznia nie przepisuje go do siebie:
wyrazenie, ktorego wartosc zalezy od tej roznicy, jest odrzucane jako zle dobrane do korpusu.

    python3 oracle.py <plan.rql> <wypis run.sh>
"""
import math
import re
import struct
import sys
from fractions import Fraction
from pathlib import Path

BYTE, INTEGER, UINT, RATIONAL, FLOAT, DOUBLE, STRING, NULL = 0, 1, 2, 3, 4, 5, 8, 9
TYPE_NAMES = {BYTE: "BYTE", INTEGER: "INTEGER", UINT: "UINT", RATIONAL: "RATIONAL", FLOAT: "FLOAT", DOUBLE: "DOUBLE",
              STRING: "STRING"}
DECLARED = {"BYTE": BYTE, "CHAR": BYTE, "INTEGER": INTEGER, "UINT": UINT, "FLOAT": FLOAT, "DOUBLE": DOUBLE,
            "STRING": STRING}
RANGE = {BYTE: (0, 255), INTEGER: (-2**31, 2**31 - 1), UINT: (0, 2**32 - 1)}
EXACT = (BYTE, INTEGER, UINT, RATIONAL)
INT_TYPES = (BYTE, INTEGER, UINT)


class EvalError(Exception):
    """Blad wykonania wyrazenia - silnik rzuca wtedy wyjatek zamiast dac wartosc."""


class V:
    __slots__ = ("type", "val", "typed")

    def __init__(self, t, val, typed=True):
        self.type, self.val, self.typed = t, val, typed

    def __eq__(self, other):
        return isinstance(other, V) and self.type == other.type and self.val == other.val

    def __repr__(self):
        return "NULL" if self.type == NULL else f"{TYPE_NAMES[self.type]}({self.val!r})"


NULLV = V(NULL, None)


def f32(x):
    try:
        return struct.unpack("f", struct.pack("f", x))[0]
    except OverflowError:
        return math.copysign(math.inf, x)


def trunc_div(a, b):
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b >= 0) else -q


def fits(t, n):
    lo, hi = RANGE[t]
    return lo <= n <= hi


# --- rzutowanie ---------------------------------------------------------------------------


def parse_int_prefix(text):
    m = re.match(r"-?\d+", text)
    if not m or not fits(INTEGER, int(m.group(0))):
        return None
    return int(m.group(0))


def cast(v, t):
    if v.type == NULL:
        return NULLV
    s, x = v.type, v.val
    if s == t:
        return v
    if t in INT_TYPES:
        if s in INT_TYPES:
            n = x
        elif s == RATIONAL:
            n = trunc_div(x.numerator, x.denominator)
        elif s in (FLOAT, DOUBLE):
            if not math.isfinite(x):
                return NULLV
            n = math.trunc(x)
        else:
            n = parse_int_prefix(x)
            if n is None:
                return NULLV
        return V(t, n) if fits(t, n) else NULLV
    if t == RATIONAL:
        if s in INT_TYPES:
            return V(RATIONAL, Fraction(x)) if fits(INTEGER, x) else NULLV
        raise EvalError(f"korpus nie przewiduje rzutu {TYPE_NAMES[s]} -> RATIONAL")
    if t == FLOAT:
        if s in INT_TYPES:
            return V(FLOAT, f32(float(x)))
        if s == RATIONAL:
            return V(FLOAT, f32(f32(float(x.numerator)) / f32(float(x.denominator))))
        if s == DOUBLE:
            return V(FLOAT, f32(x))
    if t == DOUBLE:
        if s in INT_TYPES:
            return V(DOUBLE, float(x))
        if s == RATIONAL:
            return V(DOUBLE, float(x.numerator) / float(x.denominator))
        if s == FLOAT:
            return V(DOUBLE, x)
    if t == STRING:
        if s in INT_TYPES:
            return V(STRING, str(x))
        if s == RATIONAL:
            return V(STRING, f"{x.numerator}/{x.denominator}")
        if s in (FLOAT, DOUBLE):
            return V(STRING, f"{x:f}")
    raise EvalError(f"rzut {TYPE_NAMES.get(s, s)} -> {TYPE_NAMES.get(t, t)} poza modelem")


def normalize(a, b):
    if a.type == b.type:
        return a, b
    if a.type > b.type:
        return a, cast(b, a.type)
    return cast(a, b.type), b


# --- operatory ----------------------------------------------------------------------------


def exact_rational(r):
    return V(RATIONAL, r) if fits(INTEGER, r.numerator) and r.denominator <= 2**31 - 1 else NULLV


def is_zero(v):
    return v.type in (BYTE, INTEGER, UINT, RATIONAL, FLOAT, DOUBLE) and v.val == 0


def arith(op, a, b):
    if a.type == NULL or b.type == NULL:
        return NULLV
    a, b = normalize(a, b)
    if a.type == NULL or b.type == NULL:
        return NULLV
    if op == "/" and is_zero(b):
        return NULLV
    t, x, y = a.type, a.val, b.val
    if t == STRING:
        if op == "+":
            return V(STRING, x + y)
        raise EvalError(f"Operator '{op}' not defined for string operands")
    if t in INT_TYPES:
        r = {"+": x + y, "-": x - y, "*": x * y}.get(op)
        if r is None:
            r = trunc_div(x, y)
        if t == BYTE:
            return V(INTEGER, r)
        return V(t, r) if fits(t, r) else NULLV
    if t == RATIONAL:
        r = {"+": x + y, "-": x - y, "*": x * y}.get(op)
        return exact_rational(x / y if r is None else r)
    r = {"+": x + y, "-": x - y, "*": x * y}.get(op)
    if r is None:
        r = x / y
    return V(FLOAT, f32(r)) if t == FLOAT else V(DOUBLE, r)


def integral_exponent(e):
    if e.type in INT_TYPES:
        return e.val if e.val >= 0 and fits(INTEGER, e.val) else None
    if e.type == RATIONAL and e.val.denominator == 1 and e.val.numerator >= 0:
        return e.val.numerator
    return None


def power(a, b):
    if a.type == NULL or b.type == NULL:
        return NULLV
    base, exponent = normalize(a, b)
    if base.type == NULL or exponent.type == NULL:
        return NULLV
    t = base.type
    if t > DOUBLE:
        raise EvalError("Operator '^' not defined for non-numeric operands")
    if t in EXACT:
        k = integral_exponent(exponent)
        if k is not None:
            result, factor = cast(V(INTEGER, 1), t), base
            while k > 0:
                if k & 1:
                    result = arith("*", result, factor)
                if k > 1:
                    factor = arith("*", factor, factor)
                k >>= 1
            return result
    try:
        r = math.pow(cast(base, DOUBLE).val, cast(exponent, DOUBLE).val)
    except (ValueError, OverflowError):
        return NULLV
    return cast(V(DOUBLE, r), t) if math.isfinite(r) else NULLV


def compare(op, a, b):
    if a.type == NULL or b.type == NULL:
        return NULLV
    a, b = normalize(a, b)
    if a.type == NULL or b.type == NULL:
        return NULLV
    x, y = a.val, b.val
    r = {"=": x == y, "!=": x != y, "<": x < y, ">": x > y, "<=": x <= y, ">=": x >= y}[op]
    return V(INTEGER if a.type == STRING else a.type, int(r))


def truth(v):
    if v.type == NULL:
        return None
    return len(v.val) > 0 if v.type == STRING else v.val != 0


def logic(op, a, b):
    x, y = truth(a), truth(b)
    if op == "AND":
        if x is False or y is False:
            return V(INTEGER, 0)
        return V(INTEGER, 1) if x and y else NULLV
    if x or y:
        return V(INTEGER, 1)
    return V(INTEGER, 0) if x is not None and y is not None else NULLV


def call(name, arg):
    fn = {"int": "to_integer", "float": "to_float", "real": "to_double", "str": "to_string"}.get(name.lower(), name.lower())
    if fn == "null2zero":
        # NULL nie niesie typu: ewaluator oddaje INTEGER 0, a typ pola (typ argumentu, kontrakt
        # `null2zero`) nadaje dopiero zapis. Takie zero nie swiadczy wiec o typie pola.
        return V(INTEGER, 0, typed=False) if arg.type == NULL else arg
    if fn == "isnull":
        return V(INTEGER, int(arg.type == NULL))
    if arg.type == NULL:
        return NULLV
    if fn in ("to_integer", "to_float", "to_double", "to_string"):
        return cast(arg, {"to_integer": INTEGER, "to_float": FLOAT, "to_double": DOUBLE, "to_string": STRING}[fn])
    if fn == "abs":
        if arg.type in (BYTE, UINT):
            return arg
        if arg.type == INTEGER:
            return V(INTEGER, abs(arg.val)) if fits(INTEGER, abs(arg.val)) else NULLV
        if arg.type == RATIONAL:
            return exact_rational(abs(arg.val))
        if arg.type in (FLOAT, DOUBLE):
            return V(arg.type, abs(arg.val))
        raise EvalError("Function 'Abs' not defined for this operand")
    if fn in ("iszero", "isnonzero"):
        if arg.type == STRING:
            raise EvalError("Functions 'IsZero'/'IsNonZero' are not defined for string operands")
        return V(INTEGER, int((arg.val == 0) == (fn == "iszero")))
    if fn == "length":
        if arg.type != STRING:
            raise EvalError("Function 'Length' is defined for string operands only")
        return V(INTEGER, len(arg.val.encode()))
    if fn in ("sqrt", "floor", "ceil", "round", "trunc", "log", "log2", "tan"):
        if arg.type > DOUBLE:
            raise EvalError("callFun: unsupported type")
        f = {"sqrt": math.sqrt, "floor": math.floor, "ceil": math.ceil, "trunc": math.trunc, "log": math.log,
             "log2": math.log2, "tan": math.tan,
             "round": lambda d: math.copysign(math.floor(abs(d) + 0.5), d)}[fn]
        try:
            r = float(f(cast(arg, DOUBLE).val))
        except ValueError:
            r = math.nan
        return cast(V(DOUBLE, r), arg.type)
    raise EvalError(f"funkcja {name} poza modelem wyroczni")


# --- parser -------------------------------------------------------------------------------

TOKEN = re.compile(r"\s*(?:(?P<num>\d+\.\d*|\d+)|(?P<str>'[^']*')|(?P<id>[A-Za-z_]\w*)|(?P<op>!=|<=|>=|[-+*/^()\[\]:=<>]))")

CONVENTIONAL = {"+": (1, "L"), "-": (1, "L"), "*": (2, "L"), "/": (2, "L"), "^": (3, "R")}
# Drabina z RQL.g4 - wczesniejsza alternatywa wiaze mocniej. Tylko do wykrycia wyrazen,
# ktorych wartosc od niej zalezy.
GRAMMAR = {"-": (1, "L"), "+": (2, "L"), "/": (3, "L"), "*": (4, "L"), "^": (5, "R")}


def tokenize(text):
    pos, out = 0, []
    text = text.strip()
    while pos < len(text):
        m = TOKEN.match(text, pos)
        if not m or m.end() == pos:
            raise SyntaxError(f"nie rozumiem: {text[pos:]!r}")
        pos = m.end()
        kind = m.lastgroup
        out.append((kind, m.group(kind)))
    return out


class Parser:
    def __init__(self, tokens, table):
        self.t, self.i, self.table = tokens, 0, table

    def peek(self):
        return self.t[self.i] if self.i < len(self.t) else (None, None)

    def take(self, value=None):
        tok = self.peek()
        if value is not None and tok[1] != value:
            raise SyntaxError(f"oczekiwane {value!r}, jest {tok[1]!r}")
        self.i += 1
        return tok

    def expr(self, min_prec=1):
        left = self.primary()
        while True:
            kind, op = self.peek()
            if kind != "op" or op not in self.table:
                return left
            prec, assoc = self.table[op]
            if prec < min_prec:
                return left
            self.take()
            right = self.expr(prec + 1 if assoc == "L" else prec)
            left = ("bin", op, left, right)

    def primary(self):
        kind, val = self.take()
        if kind == "op" and val == "-":
            kind2, num = self.take()
            if kind2 != "num":
                raise SyntaxError("minus jednoargumentowy tylko przy literale (inaczej silnik pada, zgloszone)")
            return ("lit", literal("-" + num))
        if kind == "op" and val == "(":
            inner = self.expr()
            self.take(")")
            return inner
        if kind == "num":
            return ("lit", literal(val))
        if kind == "str":
            return ("lit", V(STRING, val[1:-1]))
        if kind == "id":
            nxt = self.peek()[1]
            if nxt == "[":
                self.take("[")
                _, idx = self.take()
                self.take("]")
                return ("field", val, int(idx))
            if nxt == "(":
                self.take("(")
                arg = self.expr()
                if self.peek()[1] == ":":
                    self.take(":")
                    self.take()
                self.take(")")
                return ("call", val, arg)
        raise SyntaxError(f"nieoczekiwany symbol {val!r}")


def literal(text):
    return V(FLOAT, f32(float(text))) if "." in text else V(INTEGER, int(text))


def parse(text, table=CONVENTIONAL):
    p = Parser(tokenize(text), table)
    tree = p.expr()
    if p.i != len(p.t):
        raise SyntaxError(f"nadmiar symboli w {text!r}")
    return tree


def evaluate(node, row):
    kind = node[0]
    if kind == "lit":
        return node[1]
    if kind == "field":
        return row[node[2]]
    if kind == "call":
        return call(node[1], evaluate(node[2], row))
    _, op, left, right = node
    a, b = evaluate(left, row), evaluate(right, row)
    return power(a, b) if op == "^" else arith(op, a, b)


def split_top(text, sep=","):
    out, depth, cur, quote = [], 0, [], False
    for ch in text:
        if ch == "'":
            quote = not quote
        if not quote:
            if ch in "([":
                depth += 1
            elif ch in ")]":
                depth -= 1
            elif ch == sep and depth == 0:
                out.append("".join(cur).strip())
                cur = []
                continue
        cur.append(ch)
    out.append("".join(cur).strip())
    return out


def parse_condition(text):
    """Warunek RULE: OR < AND < porownanie < wyrazenie (RQL.g4: expression_logic, term_logic)."""
    ors = [re.split(r"\s+AND\s+", part, flags=re.I) for part in re.split(r"\s+OR\s+", text, flags=re.I)]
    out = []
    for conj in ors:
        terms = []
        for term in conj:
            m = re.match(r"(.*?)\s*(!=|<=|>=|=|<|>)\s*(.*)$", term)
            if not m:
                raise SyntaxError(f"warunek bez porownania: {term!r}")
            terms.append((m.group(2), parse(m.group(1)), parse(m.group(3))))
        out.append(terms)
    return out


def rule_truth(cond, row):
    result = None
    for conj in cond:
        value = None
        for op, left, right in conj:
            term = compare(op, evaluate(left, row), evaluate(right, row))
            value = term if value is None else logic("AND", value, term)
        result = value if result is None else logic("OR", result, value)
    return truth(result) is True


# --- plan i dane --------------------------------------------------------------------------


def read_rows(path, types):
    rows = []
    for line in Path(path).read_text().split("\n"):
        if not line.strip():
            continue
        row = []
        for t, tok in zip(types, line.split()):
            if tok == "NULL":
                row.append(NULLV)
            elif t == STRING:
                row.append(V(STRING, tok))
            elif t in (FLOAT, DOUBLE):
                row.append(V(t, f32(float(tok)) if t == FLOAT else float(tok)))
            else:
                row.append(V(t, int(tok)))
        rows.append(row)
    return rows


def load_plan(plan_path):
    text = "\n".join(line for line in Path(plan_path).read_text().splitlines() if not line.lstrip().startswith("#"))
    base = Path(plan_path).parent
    streams, selects, rules = {}, [], []
    for m in re.finditer(r"^DECLARE\s+(.*?)\s+STREAM\s+(\w+)\s*,\s*\S+\s+FILE\s+'([^']+)'", text, re.M):
        types = [DECLARED[re.sub(r"\[.*", "", f.split()[1]).upper()] for f in m.group(1).split(",")]
        streams[m.group(2)] = read_rows(base / m.group(3), types)
    for m in re.finditer(r"^SELECT\s+(.*?)\s+STREAM\s+(\w+)\s+FROM\s+(\S+)", text, re.M):
        selects.append((m.group(2), split_top(m.group(1)), m.group(3)))
    for m in re.finditer(r"^RULE\s+(\w+)\s+ON\s+(\w+)\s+WHEN\s+(.*?)\s+DO\b", text, re.M):
        rules.append((m.group(1), m.group(2), m.group(3)))
    return streams, selects, rules


def reduce_avg(row):
    total = Fraction(0)
    for v in row:
        if v.type == NULL:
            return NULLV
        total += Fraction(cast(v, RATIONAL).val)
    return exact_rational(total / len(row))


# --- wypis silnika ------------------------------------------------------------------------


def read_output(path):
    records, types, dumps, current = {}, {}, [], None
    for line in Path(path).read_text().splitlines():
        if line.startswith("== dumps"):
            current = "__dumps__"
        elif line.startswith("== desc "):
            name = line.split()[2]
            current = ("desc", name)
            types[name] = []
        elif line.startswith("== "):
            current = line.split()[1]
            records[current] = []
        elif current == "__dumps__" and line.strip():
            dumps.append(line.strip())
        elif isinstance(current, tuple):
            types[current[1]] += re.findall(r"\b(BYTE|INTEGER|UINT|RATIONAL|FLOAT|DOUBLE|STRING)\b", line)
        elif current and line.startswith("{"):
            records[current].append([m.group(1) for m in re.finditer(r"\w+?_\d+:(\S*)", line)])
    return records, types, dumps


def matches(expected, text):
    if expected.type == NULL:
        return text == "null"
    if text == "null":
        return False
    if expected.type in INT_TYPES:
        return int(text) == expected.val
    if expected.type == RATIONAL:
        return Fraction(text) == expected.val
    if expected.type in (FLOAT, DOUBLE):
        if math.isnan(expected.val):
            return text.lstrip("-") == "nan"
        return math.isclose(float(text), expected.val, rel_tol=1e-5, abs_tol=1e-6)
    return text == expected.val


def main(plan_path, out_path):
    streams, selects, rules = load_plan(plan_path)
    records, types, dumps = read_output(out_path)
    errors, checked = [], 0

    for name, exprs, source in selects:
        src_name, _, reducer = source.partition(".")
        rows = streams[src_name]
        if reducer:
            if reducer.lower() != "avg":
                raise SystemExit(f"reduktor {reducer} poza modelem wyroczni")
            rows = [[reduce_avg(r)] for r in rows]
        trees = []
        for e in exprs:
            tree = parse(e)
            if tree != parse(e, GRAMMAR):
                errors.append(f"{name}: wyrazenie {e!r} zalezy od drabiny pierwszenstwa RQL.g4 - nie nadaje sie do korpusu")
            trees.append(tree)
        expected = [[evaluate(t, row) for t in trees] for row in rows]
        streams[name] = expected

        got = records.get(name)
        if got is None:
            errors.append(f"{name}: brak wypisu strumienia")
            continue
        if len(got) != len(expected):
            errors.append(f"{name}: {len(got)} rekordow, wyrocznia oczekuje {len(expected)}")
        for i, (exp_row, got_row) in enumerate(zip(expected, got)):
            for k, (ev, gv) in enumerate(zip(exp_row, got_row)):
                checked += 1
                if not matches(ev, gv):
                    errors.append(f"{name}[{i}].{k} ({exprs[k]}): silnik {gv}, wyrocznia {ev}")
        declared = types.get(name, [])
        for k, row_types in enumerate(zip(*[[v.type if v.typed else NULL for v in r] for r in expected])):
            concrete = {TYPE_NAMES[t] for t in row_types if t != NULL}
            if len(concrete) > 1:
                errors.append(f"{name}.{k} ({exprs[k]}): wyrocznia daje rozne typy w roznych wierszach {sorted(concrete)}")
            elif concrete and k < len(declared) and concrete != {declared[k]}:
                errors.append(f"{name}.{k} ({exprs[k]}): typ w .desc {declared[k]}, wyrocznia {concrete.pop()}")

    for rule, on, cond_text in rules:
        cond = parse_condition(cond_text)
        fired = [i for i, row in enumerate(streams[on]) if rule_truth(cond, row)]
        if fired:
            errors.append(f"regula {rule}: warunek prawdziwy w wierszach {fired[:5]} - plan zaklada, ze nie odpala")
        if any(f"_{rule}_dump" in d for d in dumps):
            errors.append(f"regula {rule}: silnik ja odpalil ({[d for d in dumps if rule in d]})")

    for e in errors[:40]:
        print(e)
    if errors:
        print(f"WYROCZNIA: {len(errors)} rozbieznosci w {Path(plan_path).name}")
        return 1
    print(f"WYROCZNIA: {Path(plan_path).name} zgodny, {checked} wartosci")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
