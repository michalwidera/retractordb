# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T12:48:24+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /tmp/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 7268.4 us
p99            : 8863.6 us
max            : 21850.2 us
budżet (1/720s): 1388.9 us
max / budżet   : 1573.2 %   (PRZEKROCZONY)
przepustowość  : 136 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 110558034.2 us
p99            : 218964622.8 us
p99,9          : 220955329.8 us
max            : 221164820.0 us
max / budżet   : 15923867.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 110547085.7 us
p99            : 218952499.0 us
p99,9          : 220943772.1 us
max            : 221153877.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.25 mem_used_kb=262772 temp_mC=40013
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
