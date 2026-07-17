# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T16:34:39+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 4321.7 us
p99            : 4997.3 us
max            : 8192.6 us
budżet (1/360s): 2777.8 us
max / budżet   : 294.9 %   (PRZEKROCZONY)
przepustowość  : 236 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 66936550.8 us
p99            : 132428049.0 us
p99,9          : 133610257.8 us
max            : 133734214.6 us
max / budżet   : 4814431.7 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 66928363.5 us
p99            : 132419697.3 us
p99,9          : 133603219.8 us
max            : 133726097.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.42 mem_used_kb=317944 temp_mC=42831
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
