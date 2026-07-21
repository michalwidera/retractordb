# Wyniki badania S2 (engine-shadow) -- sledztwo 40ms, Faza 0

- data: 2026-07-18T14:40:41+02:00
- silnik: rdzen 3, FIFO 50 (-t), 360 Hz, 20000 probek, 1 klient xqry (sink=null, rdzenie 0-1)
- cien: shadow_pacer.py na rdzeniu 2, FIFO 50, zero pracy
- powtorzen: 5; miedzy powtorzeniami bez rebootu (reboot rozdziela badania, nie powtorzenia)
- pytanie: czy zdarzenia >= 5 ms u silnika widzi rownoczesnie niezalezny proces na sasiednim rdzeniu?

## Powtorzenie 1

- launch_mono_ns=2139185041185 exit_mono_ns=2194867602220

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.9 | 507.9 | 19.111 | 20.650 | 136 |
| shadow | 19373 | 28.1 | 58.5 | 0.088 | 0.141 | 0 |

**engine**: 136 zdarzen (slot, t_wzgl [s], wake_lag [ms]):

```
  slot    18085  t=   50.236s  5.626 ms
  slot    18086  t=   50.239s  5.505 ms
  slot    18087  t=   50.242s  5.903 ms
  slot    18088  t=   50.244s  6.526 ms
  slot    18089  t=   50.247s  6.859 ms
  slot    18090  t=   50.250s  8.211 ms
  slot    18091  t=   50.253s  8.447 ms
  slot    18092  t=   50.256s  8.853 ms
  slot    18093  t=   50.258s  9.229 ms
  slot    18094  t=   50.261s  10.621 ms
  slot    18095  t=   50.264s  11.200 ms
  slot    18096  t=   50.267s  11.293 ms
  slot    18097  t=   50.269s  11.495 ms
  slot    18098  t=   50.272s  11.546 ms
  slot    18099  t=   50.275s  12.250 ms
  slot    18100  t=   50.278s  12.017 ms
  slot    18101  t=   50.281s  11.738 ms
  slot    18102  t=   50.283s  11.774 ms
  slot    18103  t=   50.286s  13.542 ms
  slot    18104  t=   50.289s  14.055 ms
  slot    18105  t=   50.292s  14.739 ms
  slot    18106  t=   50.294s  14.998 ms
  slot    18107  t=   50.297s  15.226 ms
  slot    18108  t=   50.300s  16.550 ms
  slot    18109  t=   50.303s  16.904 ms
  slot    18110  t=   50.306s  17.027 ms
  slot    18111  t=   50.308s  17.619 ms
  slot    18112  t=   50.311s  19.111 ms
  slot    18113  t=   50.314s  19.806 ms
  slot    18114  t=   50.317s  19.702 ms
  slot    18115  t=   50.319s  19.660 ms
  slot    18116  t=   50.322s  19.463 ms
  slot    18117  t=   50.325s  20.153 ms
  slot    18118  t=   50.328s  19.874 ms
  slot    18119  t=   50.331s  19.578 ms
  slot    18120  t=   50.333s  19.256 ms
  slot    18121  t=   50.336s  19.972 ms
  slot    18122  t=   50.339s  19.692 ms
  slot    18123  t=   50.342s  19.426 ms
  slot    18124  t=   50.344s  19.304 ms
  slot    18125  t=   50.347s  19.312 ms
  slot    18126  t=   50.350s  20.650 ms
  slot    18127  t=   50.353s  20.496 ms
  slot    18128  t=   50.356s  20.335 ms
  slot    18129  t=   50.358s  19.677 ms
  slot    18130  t=   50.361s  20.036 ms
  slot    18131  t=   50.364s  19.559 ms
  slot    18132  t=   50.367s  18.894 ms
  slot    18133  t=   50.369s  18.113 ms
  slot    18134  t=   50.372s  17.323 ms
  slot    18135  t=   50.375s  17.609 ms
  slot    18136  t=   50.378s  16.984 ms
  slot    18137  t=   50.381s  16.330 ms
  slot    18138  t=   50.383s  15.617 ms
  slot    18139  t=   50.386s  15.951 ms
  slot    18140  t=   50.389s  15.317 ms
  slot    18141  t=   50.392s  14.647 ms
  slot    18142  t=   50.394s  13.997 ms
  slot    18143  t=   50.397s  13.363 ms
  slot    18144  t=   50.400s  13.714 ms
  ... (76 dalszych)
```

Odstepy miedzy zdarzeniami [s]: min=0.00 mediana=0.00 max=0.01

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.16 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 136, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    108375     126234      34142      40074       Rescheduling interrupts
< IPI1:      1455     120305      33661       1104       Function call interrupts
> IPI0:    153279     171451      35028      56061       Rescheduling interrupts
> IPI1:      1470     123871      37125       1130       Function call interrupts
< IPI5:    113563      14136      15508        138       IRQ work interrupts
> IPI5:    113641      14151      15511        149       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 2

- launch_mono_ns=2195537452927 exit_mono_ns=2251221473251

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.6 | 66.1 | 0.496 | 0.568 | 0 |
| shadow | 19374 | 28.4 | 32.5 | 0.054 | 0.099 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    153306     171555      35168      56061       Rescheduling interrupts
< IPI1:      1472     123904      37145       1130       Function call interrupts
> IPI0:    199592     218071      35472      72302       Rescheduling interrupts
> IPI1:      1486     125700      39274       1154       Function call interrupts
< IPI5:    113646      14152      15511        149       IRQ work interrupts
> IPI5:    113686      14161      15512        159       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 3

- launch_mono_ns=2251888152636 exit_mono_ns=2307571631429

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.4 | 51.9 | 0.489 | 0.524 | 0 |
| shadow | 19374 | 28.5 | 32.3 | 0.051 | 0.111 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    199623     218256      35506      72302       Rescheduling interrupts
< IPI1:      1487     125704      39569       1154       Function call interrupts
> IPI0:    245749     264428      35800      88563       Rescheduling interrupts
> IPI1:      1495     127586      41888       1181       Function call interrupts
< IPI5:    113692      14162      15512        159       IRQ work interrupts
> IPI5:    113708      14177      15516        168       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 4

- launch_mono_ns=2308236991601 exit_mono_ns=2363919736951

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.9 | 108.5 | 0.540 | 0.615 | 0 |
| shadow | 19373 | 28.7 | 51.4 | 0.055 | 0.100 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.16 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    245777     264464      36001      88563       Rescheduling interrupts
< IPI1:      1495     127640      41926       1181       Function call interrupts
> IPI0:    292085     310782      36311     104661       Rescheduling interrupts
> IPI1:      1502     131079      44406       1207       Function call interrupts
< IPI5:    113709      14177      15516        168       IRQ work interrupts
> IPI5:    113763      14200      15519        179       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 5

- launch_mono_ns=2364581814802 exit_mono_ns=2420276488122

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.8 | 42.0 | 0.476 | 0.499 | 0 |
| shadow | 19377 | 28.3 | 33.1 | 0.058 | 0.095 | 0 |

**engine**: zero zdarzen >= progu.

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.14 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 0, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik bez zdarzen w tym przebiegu -- brak materialu; wydluzyc przebieg lub powtorzyc.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    292119     310845      36458     104661       Rescheduling interrupts
< IPI1:      1503     131387      44411       1207       Function call interrupts
> IPI0:    338140     356834      36859     120917       Rescheduling interrupts
> IPI1:      1517     133835      46303       1234       Function call interrupts
< IPI5:    113764      14200      15519        179       IRQ work interrupts
> IPI5:    113817      14214      15524        190       IRQ work interrupts
(brak roznic w liniach IPI)
```
