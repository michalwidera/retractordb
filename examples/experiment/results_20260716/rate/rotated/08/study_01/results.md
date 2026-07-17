# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T21:59:57+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1947.2 us
p99            : 2288.0 us
max            : 2655.3 us
budżet (1/360s): 2777.8 us
max / budżet   : 95.6 %   (MIEŚCI SIĘ)
przepustowość  : 511 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 21624133.4 us
p99            : 43505054.8 us
p99,9          : 43907645.9 us
max            : 43950235.4 us
max / budżet   : 1582208.5 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 21619197.2 us
p99            : 43499746.0 us
p99,9          : 43902698.9 us
max            : 43945301.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.97 mem_used_kb=434552 temp_mC=40915
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
