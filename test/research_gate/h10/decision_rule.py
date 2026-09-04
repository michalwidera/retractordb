#!/usr/bin/env python3
"""Regula decyzyjna H10 — orzeka o SUROWYM wyniku kampanii, bez odniesienia.

Czym ten plik rozni sie od `../compare_regimes.py`
--------------------------------------------------
`compare_regimes.py` jest jadrem BRAMKI REGRESYJNEJ: porownuje rezimy biezacego
przebiegu z zamrozona tablica `VERDICT.md`. Odpowiada na pytanie „czy silnik sie
cofnal", i tylko na nie — przebieg, ktory zgadza sie z odniesieniem, przechodzi
niezaleznie od tego, czy odniesienie w ogole wspiera H10.

Ten plik odpowiada na pytanie inne: „czy H10 jest wsparta NA TYM silniku".
Nie zaglada do zadnego pliku odniesienia. Stosuje progi predeklaracji K24 §6 do
liczb policzonych w tym przebiegu i konczy sie werdyktem.

Zrodlem metryki jest `verdict.py` z tego katalogu — `classify`, `classify_origin`,
`regime`, `member_b` i `controls` sa importowane, nie przepisywane. Dwa
przepisania tej samej definicji rozjezdzaja sie po cichu; ten projekt ma juz taki
przypadek zapisany (predeklaracja K26v3 §6, serializer kanoniczny).

Progi (predeklaracja K24 §6, przepisane z `VERDICT.md`)
------------------------------------------------------
* **H10a** — zgodnosc 100% w klasie jest JEDYNYM wsparciem H10a w tej klasie;
  jedna niezgodnosc falsyfikuje H10a w tej klasie. Werdyktem jest atrybucja
  IZOLOWANA (postac zamknieta z ogonow skladowych wzietych z oracle'a), nie
  propagowana. Dotyczy osobno ogona startowego i poczatku logicznego.
* **rezim zanizajacy** — rekord wyemitowany, zanim jego zaleznosci sa okreslone.
  Jakosciowo inny od zawyzajacego: jest defektem poprawnosci, nie utrata
  precyzji, i zawsze jest wynikiem negatywnym.
* **H10b** — rozjazd reguly lokalnej A z dokladna na >= 5% planow ORAZ 100%
  rozjazdow dodatnich o predeklarowanej postaci `ceil((p+q-1)/p)`. Ocena jest
  warunkowa: wymaga, zeby predeklarowane kontrole negatywne HC_SINGLE i HC_INT
  BYLY SPELNIONE i zeby mialy niepusta populacje. Zlamana kontrola znaczy zle
  zdefiniowana regula lokalna, a nie wynik — czlon (b) jest wtedy NIEOCENIALNY
  i nie wolno go liczyc ani za, ani przeciw.

Kody wyjscia
------------
0  H10a WSPARTA na badanym korpusie
1  H10a BEZ WSPARCIA (falsyfikacja w co najmniej jednej klasie albo defekt
   zanizania) — wazny wynik negatywny, nigdy awaria aparatury
2  BRAK WERDYKTU (pusty korpus, rozjazd zestawu klas, blad odczytu) — kod 1 jest
   zarezerwowany dla wyniku o silniku i nie moze byc osiagalny przez awarie
"""

import argparse
import csv
import sys
from fractions import Fraction
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import verdict as V

# ══════════════════════════════════════════════════════════════════════════════
#  ZAMROZONE STALE — predeklaracja K24 §6. Nie sa parametrami CLI: prog, ktory
#  da sie podac z wiersza polecen, nie jest predeklarowany.
# ══════════════════════════════════════════════════════════════════════════════

#: Dziewiec klas operatorow korpusu. Klasa brakujaca znaczy, ze proba jej nie
#: pokryla, klasa nadmiarowa — ze generator albo silnik nazywa cos inaczej.
#: Oba przypadki to BRAK WERDYKTU, nigdy ciche orzeczenie o dziewieciu z osmiu.
EXPECTED_CLASSES = frozenset(
    {"ADD", "AGSE", "HASH", "NTHETA", "PASS", "REDUCE", "SHIFT", "SUB", "THETA"})

#: H10b: minimalny udzial planow z rozjazdem reguly lokalnej A.
H10B_MIN_SHARE = Fraction(5, 100)

#: H10b: udzial rozjazdow dodatnich o predeklarowanej postaci. Rowny jeden —
#: jeden kontrprzyklad obala postac.
H10B_FORM_SHARE = Fraction(1, 1)

SUPPORTED = "WSPARTA"
REFUTED = "BEZ WSPARCIA"
UNEVALUABLE = "NIEOCENIALNY"
NO_VERDICT = "BRAK WERDYKTU"


class VerdictError(Exception):
    """Aparatura nie jest w stanie orzec. Konczy sie kodem 2, nigdy 1."""


def judge_a(rows):
    """Czlon (a): rezim per klasa, osobno dla ogona i dla poczatku logicznego."""
    tail = V.classify(rows)
    origin = V.classify_origin(rows)

    seen = set(tail)
    if seen != set(EXPECTED_CLASSES):
        missing = sorted(EXPECTED_CLASSES - seen)
        extra = sorted(seen - EXPECTED_CLASSES)
        raise VerdictError(
            f"zestaw klas rozny od predeklarowanego; brak={missing or 'brak'} "
            f"nadmiarowe={extra or 'brak'}")

    per_class = {}
    for cls in sorted(seen):
        entry_tail, entry_origin = tail[cls], origin[cls]
        # `regime` jest definicja z verdict.py i tu sie jej nie powtarza.
        mode_tail = V.regime(entry_tail)
        if entry_origin["step"] == entry_origin["n"]:
            mode_origin = V.EXACT
        elif any(gap < 0 for gap in entry_origin["delta"]):
            mode_origin = V.UNDER
        else:
            mode_origin = V.CONSERVATIVE
        per_class[cls] = {
            "n": entry_tail["n"],
            "tail_regime": mode_tail,
            "origin_regime": mode_origin,
            "tail_share": Fraction(entry_tail["step1"], entry_tail["n"]),
            "origin_share": Fraction(entry_origin["step"], entry_origin["n"]),
            "supported": mode_tail == V.EXACT and mode_origin == V.EXACT,
            "under": V.UNDER in (mode_tail, mode_origin),
        }
    return per_class


def judge_b(rows):
    """Czlon (b): ocena WARUNKOWA — najpierw predeklarowane kontrole negatywne.

    Kontrola o zerowej populacji nie jest kontrola spelniona, tylko kontrola,
    ktorej nie bylo czym sprawdzic. Kierunek bledu jest jednostronny: cokolwiek
    nie da sie potwierdzic, unieważnia czlon (b).
    """
    ctl = V.controls(rows)
    broken = [name for name, (count, breaches) in ctl.items() if breaches]
    empty = [name for name, (count, _) in ctl.items() if count == 0]
    stats = V.member_b(rows)

    if broken or empty:
        reason = []
        if broken:
            reason.append("kontrole zlamane: " + ", ".join(sorted(broken)))
        if empty:
            reason.append("kontrole o pustej populacji: " + ", ".join(sorted(empty)))
        return {"status": UNEVALUABLE, "reason": "; ".join(reason),
                "controls": ctl, "stats": stats}

    share = Fraction(stats["diverging"], max(stats["plans"], 1))
    form = Fraction(stats["matching"], stats["positive"]) if stats["positive"] else None
    if stats["positive"] == 0:
        return {"status": UNEVALUABLE,
                "reason": "populacja predeklarowana bez ani jednego rozjazdu dodatniego",
                "controls": ctl, "stats": stats}
    ok = share >= H10B_MIN_SHARE and form >= H10B_FORM_SHARE
    return {"status": SUPPORTED if ok else REFUTED,
            "reason": f"rozjazd {float(share):.1%} (prog {float(H10B_MIN_SHARE):.0%}), "
                      f"postac {stats['matching']}/{stats['positive']}",
            "controls": ctl, "stats": stats}


def judge(rows):
    if not rows:
        raise VerdictError("korpus pusty — zero obserwacji wezlowych")
    per_class = judge_a(rows)
    member_a = SUPPORTED if all(c["supported"] for c in per_class.values()) else REFUTED
    return {
        "plans": len({row["plan"] for row in rows}),
        "observations": len(rows),
        "per_class": per_class,
        "a": member_a,
        "under": sorted(c for c, v in per_class.items() if v["under"]),
        "b": judge_b(rows),
    }


def render(report, seed, engine):
    per = report["per_class"]
    lines = [
        "# H10 — werdykt regulą decyzyjną, bez odniesienia", "",
        f"Korpus: **{report['plans']} planów**, **{report['observations']} obserwacji "
        f"węzłowych**. Ziarno **{seed}**, silnik `{engine}`.", "",
        "Progi pochodzą z predeklaracji K24 §6 i są w kodzie stałymi. Ten plik nie",
        "porównuje się z żadną tablicą odniesienia — orzeka o silniku, nie o regresji.", "",
        "## H10a — dokładność rachunku, per klasa operatora", "",
        "| Klasa | Węzłów | Ogon (izol.) | Reżim ogona | Origin (izol.) | Reżim origin | Werdykt |",
        "|---|---:|---:|---|---:|---|---|",
    ]
    for cls in sorted(per, key=lambda k: -per[k]["n"]):
        item = per[cls]
        mark = "**wsparta**" if item["supported"] else "**FALSYFIKACJA**"
        lines.append(
            f"| `{cls}` | {item['n']} | {float(item['tail_share']):.1%} | {item['tail_regime']} | "
            f"{float(item['origin_share']):.1%} | {item['origin_regime']} | {mark} |")
    exact = sum(1 for v in per.values() if v["supported"])
    lines += ["", f"**H10a: {report['a']}** — {exact}/{len(per)} klas dokładnych "
                  "w obu wielkościach jednocześnie."]
    if report["under"]:
        lines += ["", "**DEFEKT — reżim zaniżający:** " +
                  ", ".join(f"`{c}`" for c in report["under"]) + ". Rekord wyemitowany,",
                  "zanim wszystkie jego zależności są określone. To defekt poprawności,",
                  "nie utrata precyzji."]

    b = report["b"]
    lines += ["", "## H10b — wystarczalność reguły lokalnej", "",
              f"**H10b: {b['status']}** — {b['reason']}", "",
              "| Kontrola negatywna | Węzłów | Rozjazdów | Stan |", "|---|---:|---:|---|"]
    for name, (count, breaches) in b["controls"].items():
        state = "**ZŁAMANA**" if breaches else ("**PUSTA**" if count == 0 else "przeszła")
        lines.append(f"| {name} | {count} | {breaches} | {state} |")
    if b["status"] == UNEVALUABLE:
        lines += ["", "Złamana kontrola negatywna znaczy źle zdefiniowaną regułę lokalną,",
                  "a nie wynik. Człon (b) nie liczy się wtedy ani za H10, ani przeciw."]
    lines.append("")
    return "\n".join(lines)


# ══════════════════════════════════════════════════════════════════════════════
#  Samotest — regula decyzyjna jest aparatura i podlega tej samej regule co
#  reszta: musi umiec odroznic wersje obalona. Bramka, ktora przepuszcza kazde
#  dane, nie orzeka o niczym.
# ══════════════════════════════════════════════════════════════════════════════

def _row(plan, kind, tail_gap=0, origin_gap=0, hard="", div_a=0, div_b=0,
         eligible=0, form=""):
    oracle_c1, oracle_origin = 10, 3
    return {
        "plan": str(plan), "stratum": "s", "hard_classes": hard, "depth": "2",
        "node": f"n{plan}", "kind": kind, "delta": "1/1",
        "engine_tail": str(oracle_c1 + tail_gap), "oracle_c1": str(oracle_c1),
        "oracle_c2": str(oracle_c1), "replica_tail": str(oracle_c1 + tail_gap),
        "step_c1": str(oracle_c1 + tail_gap), "step_c2": str(oracle_c1),
        "agree_c1": str(int(tail_gap == 0)), "agree_c2": "1",
        "agree_step_c1": str(int(tail_gap == 0)), "agree_step_c2": "1",
        "engine_origin": str(oracle_origin + origin_gap), "oracle_origin": str(oracle_origin),
        "replica_origin": str(oracle_origin + origin_gap),
        "step_origin": str(oracle_origin + origin_gap),
        "agree_origin": str(int(origin_gap == 0)),
        "agree_step_origin": str(int(origin_gap == 0)),
        "engine_silence": "13", "oracle_silence": "13", "agree_silence": "1",
        "local_a": "0", "local_b": "0", "h10b_eligible": str(eligible),
        "divergence_a": str(div_a), "divergence_b": str(div_b),
        "predicted_form": str(form),
    }


def _corpus(**overrides):
    """Dziewiec klas, po jednym wezle, wszystko dokladne — chyba ze nadpisane."""
    rows = []
    for index, cls in enumerate(sorted(EXPECTED_CLASSES)):
        rows.append(_row(index, cls, **overrides.get(cls, {})))
    return rows


def _expect(label, rows, want_a, want_b=None, want_error=False):
    try:
        report = judge(rows)
    except VerdictError as exc:
        if want_error:
            print(f"  OK   {label}: BRAK WERDYKTU ({exc})")
            return True
        print(f"  BLAD {label}: nieoczekiwany BRAK WERDYKTU ({exc})")
        return False
    if want_error:
        print(f"  BLAD {label}: oczekiwano BRAK WERDYKTU, dostano {report['a']}")
        return False
    if report["a"] != want_a:
        print(f"  BLAD {label}: H10a {report['a']}, oczekiwano {want_a}")
        return False
    if want_b is not None and report["b"]["status"] != want_b:
        print(f"  BLAD {label}: H10b {report['b']['status']}, oczekiwano {want_b}")
        return False
    print(f"  OK   {label}: H10a {report['a']}, H10b {report['b']['status']}")
    return True


def selftest():
    ok = True
    # Wersja poprawna. H10b nieocenialny, bo korpus samotestu nie ma populacji
    # kontrolnej — dokladnie tak, jak nieocenialny bywa na prawdziwym przebiegu.
    ok &= _expect("wszystko dokladne", _corpus(), SUPPORTED, UNEVALUABLE)
    # Wersje obalone — kazda musi zostac odrzucona z INNEGO powodu.
    ok &= _expect("jedna klasa zawyza ogon",
                  _corpus(HASH={"tail_gap": 1}), REFUTED)
    ok &= _expect("jedna klasa zanizza ogon",
                  _corpus(SHIFT={"tail_gap": -1}), REFUTED)
    ok &= _expect("jedna klasa myli poczatek logiczny",
                  _corpus(ADD={"origin_gap": 1}), REFUTED)
    ok &= _expect("korpus pusty", [], None, want_error=True)
    ok &= _expect("brak klasy w korpusie",
                  [r for r in _corpus() if r["kind"] != "AGSE"], None, want_error=True)
    ok &= _expect("klasa spoza predeklaracji",
                  _corpus() + [_row(99, "NOWA")], None, want_error=True)

    # Kontrola mocy czlonu (b): gdyby zadne dane nie mogly go uczynic ocenialnym,
    # jego progi bylyby martwa galezia, a status NIEOCENIALNY — tautologia.
    rows = _corpus()
    for plan in range(100, 200):
        rows.append(_row(plan, "PASS", hard="HC_SINGLE"))
        rows.append(_row(1000 + plan, "HASH", hard="HC_INT"))
    for plan in range(200, 220):
        rows.append(_row(plan, "HASH", div_a=7, eligible=1, form=7))
    ok &= _expect("czlon (b) osiagalny i wsparty", rows, SUPPORTED, SUPPORTED)

    broken = [dict(r) for r in rows]
    for row in broken:
        if row["hard_classes"] == "HC_INT" and row["divergence_a"] == "0":
            row["divergence_a"] = "1"
            break
    ok &= _expect("czlon (b) przy zlamanej kontroli", broken, SUPPORTED, UNEVALUABLE)

    mismatch = [dict(r) for r in rows]
    for row in mismatch:
        if row["h10b_eligible"] == "1":
            row["predicted_form"] = "6"
            break
    ok &= _expect("czlon (b) z kontrprzykladem postaci", mismatch, SUPPORTED, REFUTED)

    print("SAMOTEST: " + ("PRZESZEDL" if ok else "OBLAL"))
    return 0 if ok else 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", help="surowy CSV kampanii (run_campaign.py --out)")
    parser.add_argument("--out", help="gdzie zapisac werdykt w markdownie")
    parser.add_argument("--seed", default="nieznane")
    parser.add_argument("--engine", default="nieznany")
    parser.add_argument("--selftest", action="store_true")
    args = parser.parse_args()

    if args.selftest:
        return selftest()
    if not args.raw:
        print("BLAD: podaj --raw albo --selftest", file=sys.stderr)
        return 2

    try:
        rows = V.load(args.raw)
        report = judge(rows)
    except (OSError, csv.Error) as exc:
        print(f"BLAD: nie da sie odczytac {args.raw}: {exc}", file=sys.stderr)
        return 2
    except VerdictError as exc:
        print(f"BRAK WERDYKTU: {exc}", file=sys.stderr)
        return 2

    text = render(report, args.seed, args.engine)
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
    exact = sum(1 for v in report["per_class"].values() if v["supported"])
    print(f"H10a: {report['a']} ({exact}/{len(report['per_class'])} klas dokladnych)")
    if report["under"]:
        print(f"DEFEKT — rezim zanizajacy: {', '.join(report['under'])}")
    print(f"H10b: {report['b']['status']} — {report['b']['reason']}")
    return 0 if report["a"] == SUPPORTED else 1


if __name__ == "__main__":
    sys.exit(main())
