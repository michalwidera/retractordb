# Wyniki badania 1 -- kampania rate

- data: 2026-07-16T15:02:02+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 6842.0 us
p99            : 8234.6 us
max            : 9639.3 us
budżet (1/360s): 2777.8 us
max / budżet   : 347.0 %   (PRZEKROCZONY)
przepustowość  : 146 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 100564506.0 us
p99            : 199302964.7 us
p99,9          : 201112301.4 us
max            : 201308644.9 us
max / budżet   : 7247111.2 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 100553939.6 us
p99            : 199293371.3 us
p99,9          : 201100838.7 us
max            : 201298662.0 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.19 mem_used_kb=317980 temp_mC=43175
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
