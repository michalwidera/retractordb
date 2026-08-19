#!/usr/bin/env python3
"""Most do silnika: kompilacja planu i odczyt rachunku silnika ze zrzutu.

Most nie liczy niczego — wyłącznie uruchamia `xretractor <plan> -c` i parsuje
zrzut planu. Zerowy ogon i zerowy origin nie są drukowane (presenter.cpp
raportuje je tylko gdy niezerowe), więc brak `tail=` i brak `origin=` czytamy
jako 0.
"""

import os
import re
import subprocess
from fractions import Fraction
from pathlib import Path

HEADER = re.compile(r"^(?P<name>[A-Za-z_][A-Za-z_0-9]*)\((?P<interval>\d+/\d+)\)(?P<rest>.*)$")
TAIL = re.compile(r"tail=(\d+)")
ORIGIN = re.compile(r"origin=(\d+)")

class EngineError(RuntimeError):
    """Awaria aparatury silnika — zatrzymuje iterację."""


def resolve_binary(explicit=None):
    """Binarka silnika WYŁĄCZNIE ze wskazania wołającego.

    Nie ma tu ani ścieżki domyślnej, ani szukania `xretractor` w PATH — i to
    jest cała treść tej funkcji.

    2026-08-19 poziom `H10 test_closedform` oblał na CI, bo `run_gate.sh` jako
    jedynemu testowi z silnikiem nie podawał binarki. Dawna `DEFAULT_BINARY`
    wskazywała `parents[3]/retractordb/build/Debug/...`, czyli układ katalogów
    repozytorium eksperymentu, nieistniejący po przeniesieniu aparatury do
    drzewa silnika. Zostawał więc fallback na PATH: na CI nie ma tam nic i
    poziom oblewał, a lokalnie stała tam binarka ZAINSTALOWANA — poziom
    przechodził, mierząc co innego niż reszta bramki. Zielone światło z cicho
    podstawionej binarki jest gorsze od czerwonego, bo nie widać, że kłamie.

    Wołający zawsze wie, którą binarkę bada; aparatura nie ma prawa zgadywać.
    """
    if not explicit:
        raise EngineError(
            "brak binarki xretractor — podaj ją jawnie (--xretractor albo argv[1]); "
            "aparatura celowo nie zgaduje, którą binarkę mierzy")
    binary = Path(explicit).resolve()
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise EngineError(f"wskazana ścieżka nie jest wykonywalną binarką: {binary}")
    return binary


def build_info(binary):
    done = subprocess.run([str(binary), "--build-info"], capture_output=True, text=True, timeout=30)
    return done.stdout.strip()


def parse_plan(dump):
    """Zwraca {nazwa: (interwał, ogon, origin)} dla każdego strumienia w zrzucie."""
    result = {}
    for line in dump.splitlines():
        match = HEADER.match(line)
        if not match:
            continue
        rest = match.group("rest")
        tail = TAIL.search(rest)
        origin = ORIGIN.search(rest)
        result[match.group("name")] = (Fraction(match.group("interval")),
                                       int(tail.group(1)) if tail else 0,
                                       int(origin.group(1)) if origin else 0)
    return result


def compile_plan(rql_text, binary, workdir, timeout=30):
    workdir = Path(workdir)
    workdir.mkdir(parents=True, exist_ok=True)
    query = workdir / "query.rql"
    query.write_text(rql_text, encoding="utf-8")
    env = dict(os.environ, SPDLOG_LEVEL="warn")
    done = subprocess.run([str(binary), "query.rql", "-c"], cwd=workdir, capture_output=True,
                          text=True, timeout=timeout, env=env)
    if done.returncode != 0:
        raise EngineError(f"kompilacja nieudana ({done.returncode}):\n{done.stdout}\n{done.stderr}")
    merged = done.stdout + done.stderr
    if "unresolved startup latency" in merged:
        raise EngineError("postać zamknięta nie rozwiązała ogona — nierozwiązany węzeł planu")
    if "unresolved logical origin" in merged:
        raise EngineError("rachunek nie rozwiązał początku logicznego — nierozwiązany węzeł planu")
    return done.stdout, done.stderr
