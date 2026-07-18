# Wyniki badania 1 -- kampania rate

- data: 2026-07-18T16:40:44+02:00
- czestosc napływu: 360 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_1/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1880.0 us
p99            : 2252.7 us
max            : 2765.4 us
budżet (1/360s): 2777.8 us
max / budżet   : 99.6 %   (MIEŚCI SIĘ)
przepustowość  : 527 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1945.3 us
p99            : 2545.6 us
p99,9          : 2692.1 us
max            : 3251.0 us
max / budżet   : 117.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 22.4 us
p99            : 370.8 us
p99,9          : 471.2 us
max            : 938.6 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=2.28 mem_used_kb=296517 temp_mC=41170
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
