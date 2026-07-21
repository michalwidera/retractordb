# alloc wynik: k1-smallvec

- data: 2026-07-21T09:10:07+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor (RDB_ALLOC_COUNTER)
- workload: rec205-detect.rql, metoda roznicy N1=1000 N2=6000 (delta=5000), taskset -c 3

| wskaznik (na interwal) | wartosc |
|---|---|
| **alokacje / interwal** | **567.01** |
| bajty / interwal | 172097.4 (168.1 KiB) |
| zwolnienia / interwal | 567.01 |

## Rozbicie na fazy processRows (alokacje/interwal)

| kubelek | alloc/interwal |
|---|---|
| `constructInputPayload` | 465.00 |
| `constructOutputPayload(K1)` | 20.00 |
| `outputPayload.write` | 62.00 |
| `constructRulesAndUpdate` | 0.00 |
| `declarations` | 1.01 |
| `reduceFieldsToPayload(K2,subset-input)` | 60.00 |
| `constructAgsePayload(K4,subset-input)` | 264.00 |

surowe totale: N1 allocs=579269 bytes=174796053 frees=571158 ; N2 allocs=3414331 bytes=1035283285 frees=3406220
