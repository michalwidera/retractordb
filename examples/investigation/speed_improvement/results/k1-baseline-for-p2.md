# E1 wynik: k1-baseline-for-p2

- data: 2026-07-21T10:09:20+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **278.0 us** |
| p50 rozrzut (min-max) | 276.8 - 278.8 us |
| p99 compute (mediana z 5) | 705.4 us |

przebiegi p50: 276.8 278.8 278.7 278.0 277.2
przebiegi p99: 690.2 705.4 698.0 714.7 718.7
