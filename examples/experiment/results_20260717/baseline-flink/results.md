# Wyniki kampanii baseline-flink -- krok 1: instalacja + smoke test

- data: 2026-07-17T20:49:35+02:00
- cel: sprawdzic wykonalnosc instalacji Apache Flink na Pi 400 przed
  implementacja rownowaznego dataflow Pan-Tompkinsa (7.5(ii) main-debs.tex)
- Flink: 2.3.0 (scala_2.12), tryb: local/MiniCluster (jeden JVM,
  embedded w procesie `bin/flink run -t local`), checkpointing wylaczony (domyslnie)
- limity pamieci: jobmanager.memory.process.size=768m,
  taskmanager.memory.process.size=1024m (razem <= ~1,75 GB, RAM total 3,7 GiB, brak swap)
- job smoke test: examples/streaming/WordCount.jar (wbudowany przyklad Flinka)

## Wersje
```
openjdk version "17.0.19" 2026-04-21
OpenJDK Runtime Environment (build 17.0.19+10-1-24.04.2-Ubuntu)
OpenJDK 64-Bit Server VM (build 17.0.19+10-1-24.04.2-Ubuntu, mixed mode, sharing)
Flink 2.3.0 (scala_2.12)
```

## Wynik smoke testu
```
exit_code=0
wall_time_ms=18674
job_runtime=Job Runtime: 2936 ms
peak_mem_used_mb (system-wide, /proc via free)=568
```

## Log smoke testu (ogon)
```
(lose,1)
(the,21)
(name,1)
(of,15)
(action,1)
(soft,1)
(you,1)
(now,1)
(the,22)
(fair,1)
(ophelia,1)
(nymph,1)
(in,3)
(thy,1)
(orisons,1)
(be,4)
(all,2)
(my,1)
(sins,1)
(remember,1)
(d,4)
Program execution finished
Job with JobID 4b282e4c894c5d90ef288d1d4f688054 has finished.
Job Runtime: 2936 ms

```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- smoke_stdout.log -- pelne wyjscie `bin/flink run`
- ram_sample.csv -- probkowanie RAM co 0,5s w trakcie joba (mem_used, /proc/meminfo)

## Nastepny krok (poza zakresem tego przebiegu)
Jesli smoke test przeszedl w budzecie pamieci: implementacja rownowaznego
dataflow Pan-Tompkinsa (DataStream API) + pomiar per-rekord, wg zastrzezen
REQUIREMENTS.md (jitter GC/JIT, brak semantyki slotowej, izolacja rdzenia
nietypowa dla Flinka). Kryterium stopu calej kampanii: budzet 1 dnia roboczego.
