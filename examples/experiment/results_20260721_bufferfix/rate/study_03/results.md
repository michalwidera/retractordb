# Wyniki badania 3 -- kampania rate

- data: 2026-07-21T23:14:10+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1339.0 us
p99            : 1584.2 us
max            : 43692.6 us
budżet (1/1080s): 925.9 us
max / budżet   : 4718.8 %   (PRZEKROCZONY)
przepustowość  : 721 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 5198226.6 us
p99            : 10654261.1 us
p99,9          : 10746021.9 us
max            : 10756255.7 us
max / budżet   : 1161675.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 5196886.0 us
p99            : 10652866.9 us
p99,9          : 10744631.4 us
max            : 10754865.6 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.54 mem_used_kb=285104 temp_mC=38320
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
