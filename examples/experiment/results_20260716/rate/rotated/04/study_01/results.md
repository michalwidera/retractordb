# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T17:13:55+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 2960.3 us
p99            : 3661.6 us
max            : 6077.7 us
budżet (1/360s): 2777.8 us
max / budżet   : 218.8 %   (PRZEKROCZONY)
przepustowość  : 334 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 45445200.7 us
p99            : 90030242.3 us
p99,9          : 90851904.7 us
max            : 90938396.9 us
max / budżet   : 3273782.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 45439987.6 us
p99            : 90023482.2 us
p99,9          : 90846694.4 us
max            : 90933311.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.25 mem_used_kb=262640 temp_mC=39983
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
