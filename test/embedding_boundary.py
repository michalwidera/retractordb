#!/usr/bin/env python3
"""Bramka fazy 2: warstwa OSADZALNA nie trzyma stanu procesu.

`src/rdb` i `src/include/rdb` to jedyna czesc silnika, ktora laduje dzis notatnik
(stage 1a wiazania Pythona) i ktora zaladuje jutro XCFramework na iOS. Demon buduje
swoj stan raz, wiec stan procesu i stan silnika sa w nim nieodroznialne. Host buduje
go przy kazdym uruchomieniu komorki - i tam ta roznica jest bledem.

Ta bramka pilnuje czterech rzeczy naraz, bo wszystkie cztery to ten sam blad
widziany z innej strony:

  1. brak zmiennych o zasiegu pliku (static / inline) z mutowalnym stanem,
  2. brak mutowalnych statycznych SKLADOWYCH klas - to jest odmiana, ktora
     przeoczyl pierwszy przeglad fazy 2 (Descriptor::singleLineOutput_),
  3. brak KONFIGUROWANIA globalnego logera - biblioteka uzywa domyslnego loggera
     hosta, nigdy go nie ustawia ani nie zamyka,
  4. brak std::exit i FatalError - to dorobek fazy 1, tutaj tylko zabezpieczony,
  5. brak throw / try / catch w rdzeniu bez wyjatkow (rdb, retractorcore, rdbembed) poza
     wyspami, ktore opakowuja kod cudzy zglaszajacy bledy rzutem: dwa parsery ANTLR i
     Boost.Stacktrace (docs/embedded-realtime-gaps.md, sekcja 2).
     Build z RDB_NO_EXCEPTIONS=ON (domyslny) pilnuje tego sam - kompilator odrzuca `throw`.
     Bramka pilnuje tego TAKZE przy RDB_NO_EXCEPTIONS=OFF, zeby wylaczenie opcji nie
     otwieralo drogi powrotnej.

Wyjatki sa WYMIENIONE Z NAZWY razem z powodem. Lista, ktora rosnie bez uzasadnienia,
jest gorsza niz brak bramki.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

# Warstwa BIBLIOTECZNA, nie narzedzie: src/rdb/lib to add_library(rdb STATIC), a src/rdb/*.cpp
# to zrodla wykonywalnego xtrdb (add_executable). Binarka moze konfigurowac loger i konczyc
# proces - jest hostem, nie biblioteka.
ROOTS = ("src/rdb/lib", "src/include/rdb")

# plik -> (fragment linii, powod). Kazdy wpis to swiadoma decyzja, nie przeoczenie.
ALLOWED = {
    "src/rdb/lib/memoryStore.cc": [
        ("static MemoryStore instance;",
         "instancja domyslna procesu - jawny, udokumentowany wlasciciel dla wolajacych, "
         "ktorzy wlasnego sklepu nie podaja (memoryStore.hpp)"),
    ],
    "src/rdb/lib/descriptor.cc": [
        ("static const int slot = std::ios_base::xalloc();",
         "INDEKS slotu xalloc, nie stan: przydzielany raz i niezmienny; wartosc flagi "
         "zyje w iword() konkretnego strumienia"),
    ],
    "src/rdb/lib/error.cc": [
        ("static std::atomic<FatalHandler> slot{nullptr};",
         "handler rdb::fatal() jest z natury wlasnoscia PROCESU: zlamany niezmiennik konczy caly "
         "proces, a nie jeden silnik, wiec dwa silniki w jednym procesie dziela go poprawnie; "
         "ustawia go host (launcher demona), nigdy silnik"),
    ],
}

# Rdzen bez wyjatkow poza ROOTS: fasada osadzenia i zrodla biblioteki retractorcore (lista
# czytana z src/retractor/lib/CMakeLists.txt, zeby nie rozjechala sie z buildem).
CORE_EXTRA_ROOTS = ("src/embed",)
CORE_SOURCES_CMAKE = "src/retractor/lib/CMakeLists.txt"
# Wyspy z wyjatkami: opakowuja kod cudzy, ktory zglasza bledy wylacznie rzutem (parser ANTLR,
# Boost.Stacktrace), i lapia wszystko na wlasnej granicy (rdb_exception_island w korzeniu
# CMakeLists.txt).
EXCEPTION_ISLANDS = ("src/rdb/lib/DESCParser.cc", "src/rdb/lib/stackTrace.cc", "src/retractor/lib/RQLParser.cpp")
EXCEPTION_SYNTAX = re.compile(r"\bthrow\b|\btry\s*\{|\bcatch\s*\(")
STRING_LITERAL = re.compile(r'"(?:\\.|[^"\\])*"')

# Stan, ktory JEST bledem i nie zostal jeszcze naprawiony. Bramka go wypisuje i przepuszcza,
# ale nowy wpis tutaj wymaga swiadomej decyzji - lista ma malec, nie rosnac. Rzecz w tym, zeby
# nie mylic "wiemy i mamy plan" z "przeoczylismy".
KNOWN_DEBT = {
    "src/include/rdb/probe.hpp": [
        ("inline workCounters work{};", "licznik sondy, ZYWY tylko przy RDB_BENCH_PROBE=ON"),
        ("inline materializationCounters materialization{};", "licznik sondy, ZYWY tylko przy RDB_BENCH_PROBE=ON"),
        ("inline logicalWriteCounters logicalWrite{};", "licznik sondy, ZYWY tylko przy RDB_BENCH_PROBE=ON"),
    ],
}
# Wspolny powod dla calej grupy powyzej, i jest wezszy, niz wyglada na pierwszy rzut oka.
#
# Liczniki leza w naglowku, bo inkrementacja musi sie inline'owac - skok do innej jednostki
# kompilacji bylby widoczny w samym pomiarze. Ale KAZDE ich dotkniecie stoi wewnatrz
# `if constexpr (rdb_probe_*)`, a RDB_BENCH_PROBE jest domyslnie OFF. W buildzie, ktory
# ktokolwiek uruchamia poza kampania pomiarowa, sa to martwe bajty w .bss - nikt ich nie
# czyta ani nie pisze, wiec dwa silniki w jednym procesie nie maja przez co na siebie wplynac.
#
# Zywe staja sie wylacznie w buildzie pomiarowym, a ten z zalozenia prowadzi JEDEN silnik
# (kampania H9/H10 na przypietym pi400). Przeniesienie ich do obiektu silnika kosztowaloby
# pogon za wskaznikiem dokladnie w petli, ktora sonda mierzy - czyli psuloby pomiar, zeby
# odseparowac przypadek, ktorego pomiar nie wykonuje. Wpis zostaje widoczny, bo zalozenie
# "kampania jest jednosilnikowa" jest zalozeniem, a nie gwarancja. Patrz core-phase-2.md sekcja 4.

LOGGER_CONFIG = re.compile(
    r"\bspdlog::(set_default_logger|register_logger|set_pattern|set_level|shutdown|drop\w*)\b"
    r"|\bspdlog::sinks::"
)
EXIT_CALL = re.compile(r"\b(std::exit|::exit|\bexit)\s*\(")
FATAL_CALL = re.compile(r"\bFatalError\s*\(")

# static/inline zmienna (nie funkcja): brak '(' przed koncem deklaracji.
STATIC_VAR = re.compile(r"^\s*(?:static|inline)\s+(?P<rest>[^;{]*[;{=])")


def strip_comment(line: str) -> str:
    return line.split("//", 1)[0]


def listed(table: dict, path: str, line: str) -> str | None:
    for fragment, reason in table.get(path, []):
        if fragment in line:
            return reason
    return None


def scan(repo: Path) -> tuple[list[str], list[str]]:
    problems: list[str] = []
    debt: list[str] = []
    for root in ROOTS:
        base = repo / root
        if not base.is_dir():
            problems.append(f"{root}: katalog nie istnieje - bramka mierzy nie to, co trzeba")
            continue
        for path in sorted(base.rglob("*")):
            if path.suffix not in (".cc", ".cpp", ".hpp", ".h"):
                continue
            if ".antlr" in path.parts:
                continue
            rel = path.relative_to(repo).as_posix()
            text = path.read_text(encoding="utf-8", errors="replace")
            for number, raw in enumerate(text.split("\n"), 1):
                code = strip_comment(raw)
                if not code.strip():
                    continue
                where = f"{rel}:{number}"
                if LOGGER_CONFIG.search(code):
                    problems.append(f"{where}: warstwa osadzalna KONFIGURUJE loger: {raw.strip()[:100]}")
                if FATAL_CALL.search(code):
                    problems.append(f"{where}: FatalError w warstwie osadzalnej: {raw.strip()[:100]}")
                if EXIT_CALL.search(code) and "EXIT_SUCCESS" not in code and "EXIT_FAILURE" not in code:
                    problems.append(f"{where}: wywolanie exit() w warstwie osadzalnej: {raw.strip()[:100]}")

                match = STATIC_VAR.match(code)
                if match and "(" not in match.group("rest"):
                    rest = match.group("rest")
                    if "const" in rest or "constexpr" in rest:
                        continue
                    if listed(ALLOWED, rel, code) is not None:
                        continue
                    reason = listed(KNOWN_DEBT, rel, code)
                    if reason is not None:
                        debt.append(f"{where}: {reason}")
                        continue
                    problems.append(f"{where}: mutowalny stan o zasiegu statycznym: {raw.strip()[:100]}")
    return problems, debt


def core_files(repo: Path) -> list[Path]:
    files: list[Path] = []
    for root in ROOTS + CORE_EXTRA_ROOTS:
        base = repo / root
        if base.is_dir():
            files += [p for p in sorted(base.rglob("*")) if p.suffix in (".cc", ".cpp", ".hpp", ".h")]
    cmake = (repo / CORE_SOURCES_CMAKE).read_text(encoding="utf-8")
    listing = re.search(r"set\(RETRACTOR_CORE_SOURCES([^)]*)\)", cmake)
    if listing is None:
        raise SystemExit(f"{CORE_SOURCES_CMAKE}: brak listy RETRACTOR_CORE_SOURCES - bramka mierzy nie to, co trzeba")
    for name in listing.group(1).split():
        files.append(repo / "src/retractor/lib" / name)
        header = (repo / "src/retractor/lib" / name).with_suffix(".hpp")
        if header.exists():
            files.append(header)
    return files


def scan_exceptions(repo: Path) -> list[str]:
    problems: list[str] = []
    for path in core_files(repo):
        rel = path.relative_to(repo).as_posix()
        if ".antlr" in path.parts or rel in EXCEPTION_ISLANDS:
            continue
        for number, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").split("\n"), 1):
            stripped = raw.lstrip()
            if stripped.startswith(("*", "/*")):
                continue
            code = STRING_LITERAL.sub('""', strip_comment(raw))
            if EXCEPTION_SYNTAX.search(code):
                problems.append(f"{rel}:{number}: wyjatek C++ w rdzeniu bez wyjatkow: {raw.strip()[:100]}")
    return problems


def main() -> int:
    repo = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
    problems, debt = scan(repo)
    problems += scan_exceptions(repo)
    if debt:
        print("Znany dlug fazy 2 (przepuszczony, patrz docs/core-phase-2.md sekcja 4):")
        for item in debt:
            print(f"  {item}")
        print()
    if problems:
        print("Bramka osadzalnosci (faza 2) NIE PRZESZLA:\n")
        for problem in problems:
            print(f"  {problem}")
        print(
            "\nStan procesu w src/rdb znaczy, ze dwa silniki w jednym procesie - czyli dwie komorki\n"
            "notatnika - beda go dzielic, nic o sobie nie wiedzac. Jesli wyjatek jest swiadomy,\n"
            "dopisz go do ALLOWED w tym pliku RAZEM Z POWODEM."
        )
        return 1
    print(f"Bramka osadzalnosci: {', '.join(ROOTS)} bez stanu procesu, bez konfiguracji logera, bez exit();")
    print("rdzen (rdb, retractorcore, rdbembed) bez throw/try/catch poza wyspami (ANTLR, Boost.Stacktrace).")
    return 0


if __name__ == "__main__":
    sys.exit(main())
