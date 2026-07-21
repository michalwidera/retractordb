# E1 wynik: p1e1-baseline-for-e2

- data: 2026-07-21T11:12:55+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **265.9 us** |
| p50 rozrzut (min-max) | 264.6 - 267.6 us |
| p99 compute (mediana z 5) | 737.7 us |

przebiegi p50: 265.0 264.6 265.9 266.3 267.6
przebiegi p99: 704.3 737.7 701.1 765.4 754.7
