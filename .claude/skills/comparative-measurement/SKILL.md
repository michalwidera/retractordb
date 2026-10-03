---
name: comparative-measurement
description: Przepis na pomiar porównawczy silnika na korpusie paper-arXiv/usecases (uc01..uc08) - przebieg jednej rodziny, liczba węzłów planu, callgrind, RDB_BENCH_CSV, pułapki A/B (ścieżka, argv[0], Release-Probe). Wywołaj przed każdym pomiarem wydajności lub porównaniem A/B dwóch commitów.
---

# Comparative measurement - przepis

Reguła (każdy werdykt wydajnościowy na rodzinach uc01..uc08) i embargo publikacyjne są w `CLAUDE.md`, sekcja *Comparative measurement*. Ten plik zawiera przepis.

**Two workloads are not a sample.** Issue #272 drew a conclusion about the instruction-to-time converter from the ADD plan and ECG alone; the eight families retracted it - both sat at the bottom of the spread, and the converter ranged 0,00 (uc05) to 0,36 (uc06). The converter is a property of the PAIR (change, workload) and says nothing on its own; `uc05` is the standing counterexample: -3,65 % instructions, exactly zero time.

Run one family from a FRESH copy of its directory with an empty `temp/`:

```bash
cmake --build build/Release-Probe   # `ninja` in build/Release does NOT rebuild it
python3 <usecases>/ucNN/generate_data.py --out "$work"
cp <usecases>/ucNN/rql/query.rql "$work/query.rql"
cp build/Release-Probe/src/retractor/xretractor "$work/xretractor"   # RDB_BENCH_* need the probe build
mkdir -p "$work/temp" && cd "$work"
RDB_BENCH_PLAN=1 ./xretractor query.rql -k -r -f -m 1   # plan node count: row 'PLAN bench', field 'wyjscie'
valgrind --tool=callgrind --toggle-collect='*processRows*' ./xretractor query.rql -k -r -f -m 2000
RDB_BENCH_CSV=out.csv ./xretractor query.rql -k -r -f -m 100000
```

Sources wrap past end of input, so `-m` is free; 100k slots costs 5-6 s on the heaviest family. In an A/B both sides run from the same `$work` path under the same binary name: path length and `argv[0]` alone shift the callgrind count by about 1 %.
