#!/usr/bin/env python3
"""Mechaniczna kontrola niezależności oracle'a od postaci zamkniętej.

Sprawdza dwie rzeczy w `oracle/model.py`:
  1. nie importuje repliki postaci zamkniętej ani niczego z niej,
  2. nie zawiera żadnego z zakazanych wyrażeń rachunku ogona.

Kontrola jest tania i tępa celowo — ma wykluczyć najprostszy sposób, w jaki
badanie mogłoby zamienić się w tautologię.
"""

import io
import re
import sys
import tokenize
from pathlib import Path

MODEL = Path(__file__).resolve().parents[1] / "oracle" / "model.py"

FORBIDDEN_IMPORTS = ("closedform", "SOperations")
FORBIDDEN_PATTERNS = (
    r"p\s*\+\s*q\s*-\s*1",
    r"period\s*\+\s*advance",
    r"AgseStartupLatency",
    r"SubtractStartupLatency",
    r"computeStartupLatency",
    r"phase_bound",
    # Rachunek początku logicznego — wielkość wprowadzona przestemplowaniem
    # z 2026-08-06. Oracle wyprowadza origin z warunku istnienia zależności,
    # więc nazwy z silnika nie mają prawa się tu pojawić.
    r"AgseLogicalOrigin",
    r"computeLogicalOrigin",
    r"firstIndexReaching",
    r"first_index_reaching",
)


def code_tokens(text):
    """Kod bez komentarzy i literałów tekstowych — tylko to obowiązuje."""
    kept = []
    for token in tokenize.generate_tokens(io.StringIO(text).readline):
        if token.type in (tokenize.COMMENT, tokenize.STRING):
            continue
        kept.append((token.start[0], token.string))
    return kept


def main():
    text = MODEL.read_text(encoding="utf-8")
    failures = []
    for name in FORBIDDEN_IMPORTS:
        if re.search(rf"^\s*(import|from)\s+{name}\b", text, re.MULTILINE):
            failures.append(f"import zakazanego modułu: {name}")

    tokens = code_tokens(text)
    joined = " ".join(value for _, value in tokens)
    for pattern in FORBIDDEN_PATTERNS:
        if re.search(pattern, joined):
            lines = {line for line, value in tokens if re.search(pattern, value)}
            where = f" (linia {sorted(lines)[0]})" if lines else ""
            failures.append(f"zakazane wyrażenie {pattern!r} w kodzie{where}")

    for line in failures:
        print(f"NIEZALEŻNOŚĆ ZŁAMANA: {line}")
    print("BRAMKA NIEZALEŻNOŚCI: " + ("PRZESZŁA" if not failures else "NIE PRZESZŁA"))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
