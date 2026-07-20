# E1 wynik: opt-smallvec

- data: 2026-07-20T22:37:52+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **282.1 us** |
| p50 rozrzut (min-max) | 279.4 - 283.4 us |
| p99 compute (mediana z 5) | 732.2 us |

przebiegi p50: 283.4 282.1 279.4 281.6 282.1
przebiegi p99: 732.2 732.7 708.8 728.7 801.8
