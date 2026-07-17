# Wyniki badania 3 -- kampania rate

- data: 2026-07-16T17:00:21+02:00
- czestosc napływu: 1080 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_3/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 2868.3 us
p99            : 3456.3 us
max            : 6233.0 us
budżet (1/1080s): 925.9 us
max / budżet   : 673.2 %   (PRZEKROCZONY)
przepustowość  : 346 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 40889847.5 us
p99            : 81216077.1 us
p99,9          : 81952386.6 us
max            : 82029836.7 us
max / budżet   : 8859222.4 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 40884923.5 us
p99            : 81211066.7 us
p99,9          : 81947398.3 us
max            : 82024940.0 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.16 mem_used_kb=271062 temp_mC=40889
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
