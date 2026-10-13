#!/usr/bin/env python3
"""Bramka fazy 2 (#446, C2): warstwa OSADZALNA nie trzyma stanu procesu.

`src/rdb/lib` i `src/include/rdb` to czesc silnika, ktora laduje notatnik (binding
Pythona, `src/python`), a `src/embed` to fasada, przez ktora to robi (od C3 takze plan
i jego wykonanie). Demon buduje swoj stan raz, wiec stan procesu i stan silnika
sa w nim nieodroznialne. Host buduje go przy kazdym uruchomieniu komorki - i tam ta
roznica jest bledem.

Bramka pilnuje czterech rzeczy, bo wszystkie cztery to ten sam blad widziany z innej
strony:

  1. brak zmiennych o zasiegu pliku (static / inline) z mutowalnym stanem,
  2. brak mutowalnych statycznych SKLADOWYCH klas (tak przeoczono kiedys
     Descriptor::singleLineOutput_),
  3. brak KONFIGUROWANIA globalnego logera - biblioteka uzywa domyslnego loggera
     hosta, nigdy go nie ustawia ani nie zamyka,
  4. jedyne wyjscie z procesu to std::exit w detail::terminateProcess
     (src/include/fatalError.hpp). FatalError (K1) i EnvironmentError (K4) sa w
     warstwie osadzalnej dozwolone: w zakresie ThrowOnFatalScope rzucaja
     rdb::EngineFatal zamiast konczyc proces (decyzja (a) z #463), wiec granice
     "biblioteka nie zabija hosta" trzyma ta jedna droga, a nie brak wywolan.

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
ROOTS = ("src/rdb/lib", "src/include/rdb", "src/embed")

# Regula 4 obejmuje dodatkowo naglowek z jedyna dozwolona droga wyjscia.
EXIT_EXTRA = ("src/include/fatalError.hpp",)

# plik -> (fragment linii, powod). Kazdy wpis to swiadoma decyzja, nie przeoczenie.
ALLOWED = {
    "src/rdb/lib/memoryStore.cc": [
        ("static MemoryStore instance;",
         "instancja domyslna procesu - jawny, udokumentowany wlasciciel dla wolajacych, "
         "ktorzy wlasnego Memory Store nie podaja (memoryStore.hpp)"),
    ],
    "src/rdb/lib/descriptor.cc": [
        ("static const int slot = std::ios_base::xalloc();",
         "INDEKS slotu xalloc, nie stan: przydzielany raz i niezmienny; wartosc flagi "
         "zyje w iword() konkretnego strumienia"),
    ],
}

ALLOWED_EXIT = {
    "src/include/fatalError.hpp": [
        ("std::exit(Cls == ErrorClass::Internal ? EXIT_FAILURE : EIO);",
         "detail::terminateProcess - wspolna akcja konczaca K1/K4 poza zakresem ThrowOnFatalScope"),
    ],
}

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
# Wspolny powod dla calej grupy powyzej.
#
# Liczniki leza w naglowku, bo inkrementacja musi sie inline'owac - skok do innej jednostki
# kompilacji bylby widoczny w samym pomiarze. Ale KAZDE ich dotkniecie stoi wewnatrz
# `if constexpr (rdb_probe_*)`, a RDB_BENCH_PROBE jest domyslnie OFF. W buildzie, ktory
# ktokolwiek uruchamia poza kampania pomiarowa, sa to martwe bajty w .bss - nikt ich nie
# czyta ani nie pisze, wiec dwa silniki w jednym procesie nie maja przez co na siebie wplynac.
#
# Zywe staja sie wylacznie w buildzie pomiarowym, a ten z zalozenia prowadzi JEDEN silnik.
# Przeniesienie ich do obiektu silnika kosztowaloby pogon za wskaznikiem dokladnie w petli,
# ktora sonda mierzy. Wpis zostaje widoczny, bo zalozenie "kampania jest jednosilnikowa" jest
# zalozeniem, a nie gwarancja. Patrz:
# https://documentation.retractordb.com/appendices/embedded-integration-history.html#c2-instance-owned-state

LOGGER_CONFIG = re.compile(
    r"\bspdlog::(set_default_logger|register_logger|set_pattern|set_level|shutdown|drop\w*)\b"
    r"|\bspdlog::sinks::"
)
# Kazda droga zakonczenia procesu z biblioteki; `atexit(` i podobne nie pasuja dzieki (?<!\w).
EXIT_CALL = re.compile(r"(?<!\w)(?:std::|::)?(?:_?exit|_Exit|quick_exit|abort)\s*\(")

# static/inline zmienna (nie funkcja): brak '(' przed koncem deklaracji.
STATIC_VAR = re.compile(r"^\s*(?:static|inline)\s+(?P<rest>[^;{]*[;{=])")


def strip_comment(line: str) -> str:
    return line.split("//", 1)[0]


def listed(table: dict, path: str, line: str) -> str | None:
    for fragment, reason in table.get(path, []):
        if fragment in line:
            return reason
    return None


def source_files(repo: Path, root: str) -> list[Path]:
    base = repo / root
    if base.is_file():
        return [base]
    return [p for p in sorted(base.rglob("*")) if p.suffix in (".cc", ".cpp", ".hpp", ".h") and ".antlr" not in p.parts]


def code_lines(path: Path):
    for number, raw in enumerate(path.read_text(encoding="utf-8", errors="replace").split("\n"), 1):
        code = strip_comment(raw)
        if code.strip():
            yield number, raw, code


def scan(repo: Path) -> tuple[list[str], list[str]]:
    problems: list[str] = []
    debt: list[str] = []
    for root in ROOTS + EXIT_EXTRA:
        if not (repo / root).exists():
            problems.append(f"{root}: sciezka nie istnieje - bramka mierzy nie to, co trzeba")
            continue
        state_rules = root in ROOTS
        for path in source_files(repo, root):
            rel = path.relative_to(repo).as_posix()
            for number, raw, code in code_lines(path):
                where = f"{rel}:{number}"
                if EXIT_CALL.search(code) and listed(ALLOWED_EXIT, rel, code) is None:
                    problems.append(f"{where}: wyjscie z procesu poza detail::terminateProcess: {raw.strip()[:100]}")
                if not state_rules:
                    continue
                if LOGGER_CONFIG.search(code):
                    problems.append(f"{where}: warstwa osadzalna KONFIGURUJE loger: {raw.strip()[:100]}")

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


def main() -> int:
    repo = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
    problems, debt = scan(repo)
    if debt:
        print("Znany dlug fazy 2 (przepuszczony, patrz "
              "https://documentation.retractordb.com/appendices/embedded-integration-history.html#c2-instance-owned-state):")
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
    print(f"Bramka osadzalnosci: {', '.join(ROOTS)} bez stanu procesu i bez konfiguracji logera;")
    print(f"wyjscie z procesu ({', '.join(ROOTS + EXIT_EXTRA)}) tylko przez detail::terminateProcess.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
