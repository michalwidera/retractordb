#!/usr/bin/env python3
# PreToolUse (Bash): polecenie ze slowem nazwy silnika nie moze zyc obok przebiegu H9.
# Powod: run_main_rdb.sh (zamrozony w rdb-experiment/results_20261103_R593_H9/, w manifest.sha256)
# i rdb-artifact/bin/reproduce_remeasure.sh odmawiaja startu, gdy `pgrep -af '[x]retractor'`
# znajdzie cokolwiek - takze powloke asystenta. Narzedzie Bash owija cale polecenie w `bash -c`,
# ssh robi to samo na workerze, wiec slowo w poleceniu (np. w kontroli pgrep) zatrzymywalo
# --remeasure przed pomiarem (retractordb #632, 2026-11-22, i wczesniej na pi400).
# Odmowa, gdy polecenie zawiera slowo i:
#   - samo uruchamia przebieg (remeasure, run_main_rdb, test_gate), lokalnie albo przez ssh, albo
#   - idzie przez ssh (przebieg na workerze jest stad niewidoczny), albo
#   - lokalnie biegnie reproduce_remeasure.sh albo run_main_rdb.sh.
# Zapis '[x]retractor' nie zawiera slowa i jest dozwolony, takze jako glob w sciezce.
# Wejscie: JSON na stdin. Wyjscie: {} (bez decyzji) albo deny.
import json
import os
import re
import sys

WORD = "x" + "retractor"
# Skrypty startowe asystenta nosza w nazwie "remeasure" (np. ~/R632-remeasure/run.sh), inaczej ich
# start bez biegnacego jeszcze przebiegu nie jest tu rozpoznawalny.
LAUNCH = re.compile(r"remeasure|run_main_rdb|test_gate")
SSH = re.compile(r"(^|[\s;&|(`])ssh\s")
RUNNING = ("reproduce_remeasure.sh", "run_main_rdb.sh")


def no_decision() -> int:
    print("{}")
    return 0


def deny(reason: str) -> int:
    out = {"hookEventName": "PreToolUse", "permissionDecision": "deny", "permissionDecisionReason": reason}
    print(json.dumps({"hookSpecificOutput": out}))
    return 0


def running_run() -> str:
    """Nazwa pierwszego lokalnie biegnacego skryptu z RUNNING albo pusty napis."""
    me = os.getpid()
    for pid in os.listdir("/proc"):
        if not pid.isdigit() or int(pid) == me:
            continue
        try:
            with open(f"/proc/{pid}/cmdline", "rb") as f:
                args = f.read().decode("utf-8", "replace").split("\0")
        except OSError:
            continue
        for a in args:
            for name in RUNNING:
                if os.path.basename(a) == name:
                    return name
    return ""


def main() -> int:
    try:
        data = json.load(sys.stdin)
    except ValueError:
        return no_decision()
    command = (data.get("tool_input") or {}).get("command") or ""
    if WORD not in command:
        return no_decision()
    hint = ("Strazniki H9 (run_main_rdb.sh, reproduce_remeasure.sh) odmawiaja startu, gdy jakikolwiek wiersz polecen "
            "zawiera to slowo, takze ta powloka. Zapisz je jako '[x]retractor' albo przenies polecenie do pliku skryptu.")
    if LAUNCH.search(command):
        return deny("Polecenie uruchamia przebieg H9 i zawiera slowo '" + WORD + "'. " + hint)
    if SSH.search(command):
        return deny("Polecenie idzie przez ssh i zawiera slowo '" + WORD + "'; przebieg H9 na workerze jest stad "
                    "niewidoczny. " + hint)
    name = running_run()
    if name:
        return deny("Lokalnie biegnie " + name + ", a polecenie zawiera slowo '" + WORD + "'. " + hint)
    return no_decision()


if __name__ == "__main__":
    sys.exit(main())
