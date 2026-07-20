# E1 wynik: baseline-paired

- data: 2026-07-20T22:22:47+02:00
- binarka: /home/michal/github/retractordb/build/Release/src/retractor/xretractor
- workload: rec205-detect.rql, -m 20000, unpaced, 5 przebiegow
- pin: taskset -c 3

| wskaznik | wartosc |
|---|---|
| **p50 compute (mediana z 5)** | **317.6 us** |
| p50 rozrzut (min-max) | 316.4 - 320.0 us |
| p99 compute (mediana z 5) | 834.8 us |

przebiegi p50: 316.4 317.3 317.6 320.0 318.8
przebiegi p99: 809.0 821.6 842.2 870.0 834.8
