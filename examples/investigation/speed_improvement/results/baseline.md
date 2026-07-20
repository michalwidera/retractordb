# E1 wynik: baseline

- data: 2026-07-20T22:02:16+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **317.8 us** |
| p50 rozrzut (min-max) | 313.0 - 322.1 us |
| p99 compute (mediana z 5) | 825.3 us |

przebiegi p50: 319.6 322.1 317.8 314.1 313.0
przebiegi p99: 828.9 839.5 825.3 771.9 779.3
