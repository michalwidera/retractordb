# alloc wynik: baseline

- data: 2026-07-21T08:52:01+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor (RDB_ALLOC_COUNTER)
- workload: rec205-detect.rql, metoda roznicy N1=1000 N2=6000 (delta=5000), taskset -c 3

| wskaznik (na interwal) | wartosc |
|---|---|
| **alokacje / interwal** | **1137.01** |
| bajty / interwal | 327137.4 (319.5 KiB) |
| zwolnienia / interwal | 1137.01 |

surowe totale: N1 allocs=1148663 bytes=329679717 frees=1140552 ; N2 allocs=6833725 bytes=1965366949 frees=6825614
