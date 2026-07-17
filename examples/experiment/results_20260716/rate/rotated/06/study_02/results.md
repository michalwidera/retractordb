# Wyniki badania 2 -- kampania rate

- data: 2026-07-16T20:34:29+02:00
- czestosc napływu: 720 Hz (natywna 205: 360 Hz)
- liczba klientow xqry: 1 (sink=null)
- liczba probek (-m): 20000

## E1 / E2E (sonda RDB_BENCH_CSV)
```
plik           : /dev/shm/rdb-experiment/study_2/e1_probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 1768.4 us
p99            : 2172.3 us
max            : 42981.4 us
budżet (1/720s): 1388.9 us
max / budżet   : 3094.7 %   (PRZEKROCZONY)
przepustowość  : 541 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 17821288.3 us
p99            : 35244509.1 us
p99,9          : 35587168.4 us
max            : 35618444.1 us
max / budżet   : 2564528.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 17818301.4 us
p99            : 35241530.0 us
p99,9          : 35584183.6 us
max            : 35615473.7 us
```

## Metryki systemowe w trakcie badania
```
srednie load1=1.89 mem_used_kb=279064 temp_mC=38730
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- e1_probe.csv -- surowe dane sondy (iter,compute_ns,wake_lag_ns,e2e_ns)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
