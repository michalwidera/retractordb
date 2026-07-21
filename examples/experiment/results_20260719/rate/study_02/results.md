# Wyniki badania 2 -- kampania rate

- data: 2026-07-18T16:47:05+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1838.1 us
p99            : 2242.0 us
max            : 45253.8 us
budżet (1/720s): 1388.9 us
max / budżet   : 3258.3 %   (PRZEKROCZONY)
przepustowość  : 522 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 5980911.1 us
p99            : 11887023.8 us
p99,9          : 11976444.3 us
max            : 11989022.0 us
max / budżet   : 863209.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 5979062.2 us
p99            : 11885071.1 us
p99,9          : 11974597.3 us
max            : 11986817.8 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.49 mem_used_kb=278811 temp_mC=39766
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
