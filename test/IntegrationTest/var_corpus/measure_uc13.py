#!/usr/bin/env python3
"""S3/R5: koszt dwoch wariantow detektora, nie werdykt o wydajnosci silnika.

Przepis comparative-measurement: swiezy katalog, te same sciezki i argv[0],
Release-Probe, Ir tylko processRows oraz RDB_BENCH_CSV. Wyniki zostaja poza repo.
EMA z zamrazaniem LTA i AVG bez zamrazania nie sa numerycznie rownowazne.
"""
import argparse
import csv
import hashlib
import json
import os
import platform
import re
import shutil
import statistics
import subprocess
from pathlib import Path

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("engine", type=Path)
    parser.add_argument("work", type=Path)
    parser.add_argument("--slots", type=int, default=20000)
    parser.add_argument("--repeats", type=int, default=5)
    args = parser.parse_args()
    if args.slots <= 1000 or args.repeats < 2:
        parser.error("wymagane >1000 slotow i >=2 powtorzenia")
    args.work.mkdir(parents=True, exist_ok=False)
    work = args.work.resolve()
    shutil.copyfile(args.engine.resolve(), work / "xretractor")
    (work / "xretractor").chmod(0o755)
    shutil.copyfile(HERE / "uc13/acc.txt", work / "acc.txt")
    (work / ".config").mkdir()
    env = dict(os.environ, RDB_NAMESPACE="var_corpus_538", XDG_CONFIG_HOME=str(work / ".config"), LC_ALL="C")
    env.pop("RDB_BENCH_CSV", None)
    env.pop("RDB_BENCH_PLAN", None)
    info = subprocess.check_output([str(work / "xretractor"), "--build-info"], env=env, text=True)
    (work / "build-info.txt").write_text(info)
    if not re.search(r"RDB_BENCH_PROBE\s*=\s*ON", info):
        raise ValueError("pomiar wymaga aktualnego Release-Probe z sonda ON")

    def run(label, plan, options, **settings):
        shutil.rmtree(work / "temp", ignore_errors=True)
        (work / "temp").mkdir()
        shutil.copyfile(HERE / "uc13" / plan, work / "query.rql")
        with (work / (label + ".log")).open("w") as log:
            subprocess.run(options + ["./xretractor", "query.rql", "-k", "-r", "-f"] + settings.pop("tail", []),
                           cwd=work, env=dict(env, **settings), stdout=log, stderr=subprocess.STDOUT, check=True)

    summary = dict(platform=platform.platform(), cpu=platform.processor(), slots=args.slots,
                   repeats=args.repeats, warmup=1000, variants={},
                   source_head=subprocess.check_output(["git", "-C", str(HERE.parents[2]), "rev-parse", "HEAD"], text=True).strip(),
                   engine_sha256=hashlib.sha256((work / "xretractor").read_bytes()).hexdigest(),
                   limitation="EMA with frozen LTA versus AVG without freezing: different algorithms; not an engine verdict")
    # Najpierw pelny przebieg obu planow i niezalezna kontrola wartosci.
    import oracle
    for variant, plan in (("VAR", "query.rql"), ("window", "window.rql")):
        run(variant + "-validation", plan, [], tail=["-u"])
        expected = (oracle.simulate("uc13", HERE / "uc13") if variant == "VAR" else
                    oracle.simulate_window(HERE / "uc13"))
        oracle.check_storage(work / "temp", expected)
        run(variant + "-plan", plan, [], tail=["-m", "1"], RDB_BENCH_PLAN="1")
        # Roznica prefiksow odrzuca koszt pierwszych 1000 slotow, w tym start
        # okna. Oba przebiegi startuja od deklaracji na tych samych danych.
        counts = {}
        for slots in (1000, 3000):
            filename = f"{variant}-{slots}.callgrind"
            run(f"{variant}-callgrind-{slots}", plan,
                ["valgrind", "--tool=callgrind", "--toggle-collect=*processRows*",
                 "--callgrind-out-file=" + filename], tail=["-m", str(slots + 1)])
            report = (work / filename).read_text()
            counts[slots] = int(re.findall(r"^summary: (\d+)", report, re.MULTILINE)[-1])
        ir = counts[3000] - counts[1000]
        if ir <= 0:
            raise ValueError(f"{variant}: brak instrukcji processRows w callgrind")
        summary["variants"][variant] = dict(Ir_2000_after_warmup=ir, Ir_prefixes=counts,
                                            validation_rows=len(expected["rows"]["detector"]), mean_compute_ns=[])
    # Kolejnosc AB/BA ogranicza wplyw dryfu obciazenia maszyny.
    for repeat in range(args.repeats):
        variants = [("VAR", "query.rql"), ("window", "window.rql")]
        for variant, plan in variants[::1 if repeat % 2 == 0 else -1]:
            label = f"{variant}-{repeat}"
            # -m wlicza takze poczatkowy slot zerowy, ktorego sonda nie zapisuje.
            run(label, plan, [], tail=["-m", str(args.slots + 1)], RDB_BENCH_CSV=label + ".csv")
            with (work / (label + ".csv")).open() as file:
                rows = list(csv.DictReader(file))
            if len(rows) != args.slots:
                raise ValueError(f"{label}: {len(rows)} slotow zamiast {args.slots}")
            values = [int(row["compute_ns"]) for row in rows[1000:]]
            summary["variants"][variant]["mean_compute_ns"].append(statistics.mean(values))
            print(f"{label}: {statistics.mean(values):.1f} ns/slot", flush=True)
    for values in summary["variants"].values():
        values["median_run_mean_ns"] = statistics.median(values["mean_compute_ns"])
    (work / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
