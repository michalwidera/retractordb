# E1 wynik: p1e0-baseline

- data: 2026-07-21T10:54:11+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **273.5 us** |
| p50 rozrzut (min-max) | 273.1 - 275.2 us |
| p99 compute (mediana z 5) | 730.3 us |

przebiegi p50: 275.2 273.5 275.0 273.4 273.1
przebiegi p99: 804.9 727.6 730.3 682.1 745.8
