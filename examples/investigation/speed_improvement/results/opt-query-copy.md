# E1 wynik: opt-query-copy

- data: 2026-07-20T22:17:49+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **281.7 us** |
| p50 rozrzut (min-max) | 280.7 - 284.9 us |
| p99 compute (mediana z 5) | 762.1 us |

przebiegi p50: 280.7 281.0 284.9 281.7 282.8
przebiegi p99: 762.1 804.1 777.9 755.2 714.7
