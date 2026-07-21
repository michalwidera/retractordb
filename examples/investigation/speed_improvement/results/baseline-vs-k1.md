# E1 wynik: baseline-vs-k1

- data: 2026-07-21T09:27:30+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **282.1 us** |
| p50 rozrzut (min-max) | 281.0 - 283.4 us |
| p99 compute (mediana z 5) | 710.2 us |

przebiegi p50: 281.1 283.4 282.7 282.1 281.0
przebiegi p99: 716.1 729.5 684.3 687.6 710.2
