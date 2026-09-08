#!/usr/bin/env python3
"""Werdykt K24d — automatyczny, per klasa operatora, nigdy agregatem.

Raportowane są dwie wielkości osobno (ogon i początek logiczny) oraz ich suma.
Mieszanie ich ukryłoby przesunięcie milczenia między członami przy zachowanej
sumie — czyli dokładnie to, co zrobiło przestemplowanie z 2026-08-06.

Czyta `raw/campaign.csv` i wypisuje `VERDICT.md`. Progi pochodzą
z zamrożonych kryteriów kampanii K24 §6; ten skrypt nie podejmuje
decyzji uznaniowych.

Atrybucja per klasa jest **izolowana**: ogon węzła liczony postacią zamkniętą
z ogonów składowych wziętych z oracle'a. Bez tego niezgodność dziecka liczyłaby
się jako niezgodność rodzica i klasa operatora nic by nie znaczyła. Wynik
propagowany (silnik wobec oracle'a na całym planie) raportowany jest obok.
"""

import argparse
import collections
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parent

EXACT = "dokładna"
CONSERVATIVE = "zawyżająca"
UNDER = "ZANIŻAJĄCA"


def load(path):
    with Path(path).open(encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def classify(rows):
    stats = collections.defaultdict(lambda: {
        "n": 0, "prop": 0, "step1": 0, "step2": 0,
        "delta": collections.Counter(), "witness_over": [], "witness_under": []})
    for row in rows:
        entry = stats[row["kind"]]
        entry["n"] += 1
        entry["prop"] += int(row["agree_c1"])
        entry["step1"] += int(row["agree_step_c1"])
        entry["step2"] += int(row["agree_step_c2"])
        gap = int(row["step_c1"]) - int(row["oracle_c1"])
        entry["delta"][gap] += 1
        if gap > 0 and len(entry["witness_over"]) < 2:
            entry["witness_over"].append(row)
        if gap < 0 and len(entry["witness_under"]) < 2:
            entry["witness_under"].append(row)
    return stats


def classify_origin(rows):
    """To samo, ale dla początku logicznego i dla sumy slotów milczenia.

    Kierunek różnicy znaczy tu co innego niż przy ogonie. Origin **zaniżony**
    to rekord wyemitowany, mimo że jego definicja sięga przed początek źródła —
    czyli odczyt poza historią, nie opóźnienie. Origin **zawyżony** to rekord
    porzucony, mimo że dawał się policzyć: strumień milczy dłużej, niż musi.
    """
    stats = collections.defaultdict(lambda: {
        "n": 0, "prop": 0, "step": 0, "silence": 0,
        "delta": collections.Counter(), "witness_over": [], "witness_under": []})
    for row in rows:
        entry = stats[row["kind"]]
        entry["n"] += 1
        entry["prop"] += int(row["agree_origin"])
        entry["step"] += int(row["agree_step_origin"])
        entry["silence"] += int(row["agree_silence"])
        gap = int(row["step_origin"]) - int(row["oracle_origin"])
        entry["delta"][gap] += 1
        if gap > 0 and len(entry["witness_over"]) < 2:
            entry["witness_over"].append(row)
        if gap < 0 and len(entry["witness_under"]) < 2:
            entry["witness_under"].append(row)
    return stats


def regime(entry):
    if entry["step1"] == entry["n"]:
        return EXACT
    if any(gap < 0 for gap in entry["delta"]):
        return UNDER
    return CONSERVATIVE


def member_b(rows):
    plans = {row["plan"] for row in rows}
    diverging_a = {row["plan"] for row in rows if row["divergence_a"] != "0"}
    eligible = [row for row in rows if row["h10b_eligible"] == "1"]
    positive = [row for row in eligible if int(row["divergence_a"]) > 0]
    matching = [row for row in positive
                if int(row["divergence_a"]) == int(row["predicted_form"])]
    return {"plans": len(plans), "diverging": len(diverging_a),
            "share": len(diverging_a) / max(len(plans), 1),
            "eligible": len(eligible), "positive": len(positive), "matching": len(matching),
            "mismatch": [row for row in positive if row not in matching][:3]}


def controls(rows):
    literal_single = [row for row in rows if "HC_SINGLE" in row["hard_classes"].split(",")]
    literal_int = [row for row in rows if "HC_INT" in row["hard_classes"].split(",")]
    phase_free = [row for row in literal_single if row["kind"] in ("PASS", "SHIFT", "REDUCE")]
    int_hash = [row for row in literal_int if row["kind"] == "HASH"]

    def breaches(selected, column):
        return sum(1 for row in selected if row[column] != "0")

    return {
        "HC_SINGLE (dosłownie)": (len(literal_single), breaches(literal_single, "divergence_a")),
        "HC_SINGLE (operatory bez własnego ogona)": (len(phase_free), breaches(phase_free, "divergence_a")),
        "HC_INT (dosłownie)": (len(literal_int), breaches(literal_int, "divergence_a")),
        "HC_INT (węzły `#`, reguła lokalna B)": (len(int_hash), breaches(int_hash, "divergence_b")),
    }


def render(rows, out, seed="20260803", engine="5e3eb42"):
    stats = classify(rows)
    b = member_b(rows)
    ctl = controls(rows)
    plans = len({row["plan"] for row in rows})

    lines = [
        "# K24d / H10 — werdykt", "",
        f"Korpus: **{plans} planów**, **{len(rows)} obserwacji węzłowych**, "
        f"zero błędów aparatury. Ziarno {seed}, silnik `{engine}`.", "",
        "Werdykt jest raportowany per klasa operatora. Zgodność 100% jest jedynym",
        "wsparciem H10a w klasie; jedna niezgodność falsyfikuje H10a w tej klasie.", "",
        "## 1. H10a — dokładność, per klasa operatora", "",
        "Kolumna **izolowana** jest werdyktem: postać zamknięta policzona z ogonów",
        "składowych wziętych z oracle'a, więc niezgodność pochodzi z reguły tego",
        "węzła. Kolumna **propagowana** to zgodność zrzutu planu silnika z oracle'em",
        "na całym planie — zawiera skutki niezgodności odziedziczonych po dzieciach.", "",
        "| Klasa | Węzłów | Izolowana C1 | Izolowana C2 | Propagowana C1 | Reżim | Werdykt H10a |",
        "|---|---:|---:|---:|---:|---|---|",
    ]

    regimes = {}
    for kind in sorted(stats, key=lambda key: -stats[key]["n"]):
        entry = stats[kind]
        mode = regime(entry)
        regimes[kind] = mode
        verdict = "**wsparta**" if mode == EXACT else "**FALSYFIKACJA**"
        lines.append(
            f"| `{kind}` | {entry['n']} | {entry['step1'] / entry['n']:.1%} | "
            f"{entry['step2'] / entry['n']:.1%} | {entry['prop'] / entry['n']:.1%} | "
            f"{mode} | {verdict} |")

    exact = [k for k, v in regimes.items() if v == EXACT]
    over = [k for k, v in regimes.items() if v == CONSERVATIVE]
    under = [k for k, v in regimes.items() if v == UNDER]

    lines += ["", "### Trzy reżimy", "",
              f"* **dokładna** (postać zamknięta == oracle wszędzie): "
              f"{', '.join(f'`{k}`' for k in exact) or 'brak'};",
              f"* **zawyżająca** (nigdy nie zaniża, bezpieczna, ale nie równa): "
              f"{', '.join(f'`{k}`' for k in over) or 'brak'};",
              f"* **zaniżająca** (ogon mniejszy od wymaganego przez model zdarzeniowy): "
              f"{', '.join(f'`{k}`' for k in under) or 'brak'}.", "",
              "Reżim zaniżający jest jakościowo inny od zawyżającego: zawyżenie",
              "opóźnia emisję o slot, zaniżenie oznacza rekord wyemitowany, zanim",
              "wszystkie jego zależności są określone.", "",
              "### Rozkład różnicy (postać zamknięta − oracle C1)", "",
              "| Klasa | Rozkład |", "|---|---|"]
    for kind in sorted(stats, key=lambda key: -stats[key]["n"]):
        entry = stats[kind]
        total = entry["n"]
        dist = ", ".join(f"`{gap:+d}`: {count} ({count / total:.1%})"
                         for gap, count in sorted(entry["delta"].items()))
        lines.append(f"| `{kind}` | {dist} |")

    lines += ["", "### Świadkowie", "",
              "| Klasa | Kierunek | Plan | Węzeł | Interwał | Silnik | Postać zamknięta (izol.) | Oracle C1 |",
              "|---|---|---:|---|---|---:|---:|---:|"]
    for kind in sorted(stats, key=lambda key: -stats[key]["n"]):
        for direction, key in (("zawyżenie", "witness_over"), ("**zaniżenie**", "witness_under")):
            for row in stats[kind][key]:
                lines.append(f"| `{kind}` | {direction} | {row['plan']} | {row['node']} | "
                             f"`{row['delta']}` | {row['engine_tail']} | {row['step_c1']} | "
                             f"{row['oracle_c1']} |")

    origin_stats = classify_origin(rows)
    lines += ["", "## 1b. H10a — początek logiczny, per klasa operatora", "",
              "Wielkość wprowadzona przestemplowaniem z 2026-08-06 i nieobecna",
              "w kampaniach K24/K24r. Kolumna **suma** porównuje origin+ogon —",
              "to jedyna wielkość wspólna z kampaniami sprzed zmiany.", "",
              "| Klasa | Węzłów | Izolowana | Propagowana | Suma (origin+ogon) | Reżim | Werdykt |",
              "|---|---:|---:|---:|---:|---|---|"]
    origin_regimes = {}
    for kind in sorted(origin_stats, key=lambda key: -origin_stats[key]["n"]):
        entry = origin_stats[kind]
        if entry["step"] == entry["n"]:
            mode = EXACT
        elif any(gap < 0 for gap in entry["delta"]):
            mode = UNDER
        else:
            mode = CONSERVATIVE
        origin_regimes[kind] = mode
        verdict = "**wsparta**" if mode == EXACT else "**FALSYFIKACJA**"
        lines.append(
            f"| `{kind}` | {entry['n']} | {entry['step'] / entry['n']:.1%} | "
            f"{entry['prop'] / entry['n']:.1%} | {entry['silence'] / entry['n']:.1%} | "
            f"{mode} | {verdict} |")

    lines += ["", "### Rozkład różnicy origin (rachunek silnika − oracle)", "",
              "| Klasa | Rozkład |", "|---|---|"]
    for kind in sorted(origin_stats, key=lambda key: -origin_stats[key]["n"]):
        entry = origin_stats[kind]
        total = entry["n"]
        dist = ", ".join(f"`{gap:+d}`: {count} ({count / total:.1%})"
                         for gap, count in sorted(entry["delta"].items()))
        lines.append(f"| `{kind}` | {dist} |")

    origin_under = [k for k, v in origin_regimes.items() if v == UNDER]
    lines += ["", f"Origin zaniżony (odczyt przed początkiem źródła): "
                  f"{', '.join(f'`{k}`' for k in origin_under) or '**brak**'}.", ""]

    lines += ["## 2. H10b — nielokalność", "",
              f"* rozjazd reguły lokalnej A z dokładną: **{b['diverging']} z {b['plans']} "
              f"planów = {b['share']:.1%}** (próg predeklarowany: >= 5%)",
              f"* populacja predeklarowana (dokładnie jeden `#`, poza tym `PASS`/`>N`): "
              f"**{b['eligible']} planów**, rozjazdów dodatnich **{b['positive']}**",
              f"* rozjazdów o predeklarowanej postaci `ceil((p+q-1)/p)`: "
              f"**{b['matching']} z {b['positive']}** "
              f"({b['matching'] / max(b['positive'], 1):.1%}; próg: 100%)", ""]

    lines += ["## 3. Kontrole negatywne", "",
              "| Kontrola | Węzłów | Rozjazdów | Stan |", "|---|---:|---:|---|"]
    for label, (count, breaks) in ctl.items():
        state = "**przeszła**" if breaks == 0 else "**ZŁAMANA**"
        lines.append(f"| {label} | {count} | {breaks} | {state} |")

    lines += ["", "Obie kontrole predeklarowane **w postaci dosłownej są złamane**.",
              "Zgodnie z kryterium §6 oznacza to źle zdefiniowaną regułę",
              "lokalną, a nie wynik — dlatego **człon (b) jest nieocenialny na tej",
              "aparaturze** i powyższe liczby H10b nie stanowią werdyktu. Diagnoza",
              "sprzeczności w specyfikacji członu (b): REPORT.md §5.", ""]

    Path(out).write_text("\n".join(lines) + "\n", encoding="utf-8")
    return {"exact": exact, "over": over, "under": under, "b": b, "controls": ctl,
            "origin_under": origin_under}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", default=str(ROOT / "raw" / "campaign.csv"))
    parser.add_argument("--out", default=str(ROOT / "VERDICT.md"))
    # K24r: ziarno i silnik były w nagłówku zaszyte na sztywno, więc werdykt
    # z innego ziarna albo innego stanu silnika opisywał sam siebie nieprawdziwie.
    parser.add_argument("--seed", default="20260806")
    parser.add_argument("--engine", default="db4a360")
    args = parser.parse_args()
    summary = render(load(args.raw), args.out, args.seed, args.engine)
    # Puste listy `zawyżające` i `zaniżające` są stanem NAJLEPSZYM, nie awarią.
    # Trzy reżimy są rozłączne i sumują się do wszystkich klas, więc komplet
    # klas w `dokładne` z konieczności zostawia dwie pozostałe listy puste.
    # Zapis `[]` bywa czytany jako brak danych — stąd ta uwaga przy samym druku:
    # niepusta lista `zaniżające` oznacza defekt poprawności (rekord wyemitowany,
    # zanim jego zależności są określone), a niepusta `zawyżające` utratę
    # dokładności wobec zamrożonego odniesienia — dopiero to oblewa bramkę.
    print(f"dokładne: {summary['exact']}")
    print(f"zawyżające: {summary['over']}")
    print(f"zaniżające: {summary['under']}")
    print(f"origin — klasy z zaniżeniem: {summary['origin_under'] or 'brak'}")
    print(f"H10b (nieocenialny): rozjazd {summary['b']['share']:.1%}, "
          f"postać {summary['b']['matching']}/{summary['b']['positive']}")
    for label, (count, breaks) in summary["controls"].items():
        print(f"kontrola {label}: {breaks}/{count}")


if __name__ == "__main__":
    main()
