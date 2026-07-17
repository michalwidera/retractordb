# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T17:21:52+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 2969.8 us
p99            : 3670.9 us
max            : 6353.7 us
budżet (1/720s): 1388.9 us
max / budżet   : 457.5 %   (PRZEKROCZONY)
przepustowość  : 333 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 49783785.7 us
p99            : 98622723.5 us
p99,9          : 99517815.3 us
max            : 99612077.4 us
max / budżet   : 7172069.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 49778622.8 us
p99            : 98617314.9 us
p99,9          : 99512680.8 us
max            : 99606931.5 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.52 mem_used_kb=267881 temp_mC=40535
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
