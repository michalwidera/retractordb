#!/usr/bin/env python3
"""Generuje `results/summary.md` z surowych wyników JSON obu kampanii."""

import json
import os
import platform
import subprocess
import sys
from datetime import datetime, timezone

from models import MODELS, representation_cost

HERE = os.path.dirname(os.path.abspath(__file__))
RESULTS = os.path.join(HERE, "results")


def git_commit():
    try:
        return subprocess.run(
            ["git", "-C", HERE, "rev-parse", "--short", "HEAD"],
            capture_output=True,
            text=True,
            check=True,
        ).stdout.strip()
    except Exception:
        return "nieznany"


def load(name):
    path = os.path.join(RESULTS, name)
    if not os.path.exists(path):
        return None
    with open(path, encoding="utf-8") as handle:
        return json.load(handle)


def main():
    equivalence = load("equivalence.json")
    engine = load("engine.json")
    if equivalence is None or engine is None:
        print("brak results/equivalence.json lub results/engine.json — uruchom run.sh")
        return 1

    lines = []
    add = lines.append

    add("# Wyniki eksperymentu SDF/CSDF — 2026-07-25")
    add("")
    add(f"- commit silnika: `{git_commit()}`")
    add(f"- python: {platform.python_version()}, host: {platform.platform()}")
    add(f"- wygenerowano: {datetime.now(timezone.utc).isoformat(timespec='seconds')}")
    add(f"- binarka użyta w moście: `{engine['binary']}`")
    add("")

    add("## 1. Kontrole mutacyjne")
    add("")
    add("Warunek wstępny: macierz musi wykrywać wstrzyknięty błąd.")
    add("")
    add("| mutacja | oczekiwana detekcja | wykryta na | werdykt |")
    add("|---|---|---:|---|")
    for name, info in equivalence["mutations"].items():
        expect = "tak" if info["should_be_detected"] else "nie (kontrola negatywna)"
        add(
            f"| `{name}` | {expect} | {info['detected_on']}/{info['cases']} "
            f"| {info['verdict']} |"
        )
    add("")

    add("## 2. Zgodność oracle — modele")
    add("")
    add("| kampania | modele | przypadków | porównanych pozycji | rozbieżności | błędy okresu |")
    add("|---|---|---:|---:|---:|---:|")
    for campaign in equivalence["campaigns"]:
        add(
            f"| {campaign['label']} | {len(campaign['models'])} "
            f"| {campaign['cases']} | {campaign['compared_positions']} "
            f"| {len(campaign['mismatches'])} | {len(campaign['period_mismatches'])} |"
        )
    totals = equivalence["totals"]
    add(
        f"| **razem** | | **{totals['cases']}** | **{totals['positions']}** "
        f"| **{totals['mismatches']}** | **{totals['period_mismatches']}** |"
    )
    add("")
    add(
        f"Nieskrócony zapis stosunku daje ten sam ślad co skrócony: "
        f"`{equivalence['unreduced_ratio_equal']}`."
    )
    add("")

    add("## 3. Most oracle — silnik RetractorDB")
    add("")
    add("| przypadek | Δa | Δb | a/b | P | Δc | porównano | okresów | prefiks zer | wynik |")
    add("|---|---|---|---|---:|---|---:|---:|---:|---|")
    for case in engine["cases"]:
        add(
            f"| {case['case']} | {case['delta_a']} | {case['delta_b']} | {case['ratio']} "
            f"| {case['P']} | {case['delta_c']} | {case['compared']} "
            f"| {case['periods_covered']} | {case['zero_prefix']} | {case['status']} |"
        )
    add("")

    add("## 4. Koszt reprezentacji")
    add("")
    add("Wielkości strukturalne, wyprowadzone z definicji modeli — nie pomiary czasu.")
    add("")
    for ratio in [(1, 2), (7, 10), (3, 100), (160, 147)]:
        cost = representation_cost(*ratio)
        add(f"**Δa/Δb = {ratio[0]}/{ratio[1]}, P = {cost['P']}**")
        add("")
        add("| reprezentacja | fazy | słowa opisu | startup (tokeny) | bufor wejść |")
        add("|---|---:|---:|---:|---:|")
        for name in MODELS:
            entry = cost[name]
            add(
                f"| {name} | {entry['phases']} | {entry['static_words']} "
                f"| {entry['startup_tokens']} | {entry['input_highwater']} |"
            )
        add("")

    os.makedirs(RESULTS, exist_ok=True)
    with open(os.path.join(RESULTS, "summary.md"), "w", encoding="utf-8") as handle:
        handle.write("\n".join(lines) + "\n")
    print(f"zapisano {os.path.join(RESULTS, 'summary.md')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
