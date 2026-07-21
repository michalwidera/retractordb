# alloc wynik: baseline

- data: 2026-07-21T09:02:04+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor (RDB_ALLOC_COUNTER)
- workload: rec205-detect.rql, metoda roznicy N1=1000 N2=6000 (delta=5000), taskset -c 3

| wskaznik (na interwal) | wartosc |
|---|---|
| **alokacje / interwal** | **1137.01** |
| bajty / interwal | 327137.4 (319.5 KiB) |
| zwolnienia / interwal | 1137.01 |

## Rozbicie na fazy processRows (alokacje/interwal)

| kubelek | alloc/interwal |
|---|---|
| `constructInputPayload` | 465.00 |
| `constructOutputPayload(K1)` | 590.00 |
| `outputPayload.write` | 62.00 |
| `constructRulesAndUpdate` | 0.00 |
| `declarations` | 1.01 |
| `reduceFieldsToPayload(K2,subset-input)` | 60.00 |
| `constructAgsePayload(K4,subset-input)` | 264.00 |

surowe totale: N1 allocs=1148687 bytes=329680581 frees=1140576 ; N2 allocs=6833749 bytes=1965367813 frees=6825638
