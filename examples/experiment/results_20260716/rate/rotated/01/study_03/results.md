# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T12:57:15+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /tmp/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 6127.2 us
p99            : 7353.2 us
max            : 13049.5 us
budżet (1/1080s): 925.9 us
max / budżet   : 1409.3 %   (PRZEKROCZONY)
przepustowość  : 162 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 85174320.9 us
p99            : 168782854.8 us
p99,9          : 170310393.1 us
max            : 170472164.4 us
max / budżet   : 18410993.8 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 85165060.0 us
p99            : 168773438.6 us
p99,9          : 170300975.9 us
max            : 170462709.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.71 mem_used_kb=265606 temp_mC=41311
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
