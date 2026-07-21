# Wyniki badania S2 (engine-shadow) -- sledztwo 40ms, Faza 0

- data: 2026-07-18T12:23:52+02:00
- silnik: rdzen 3, FIFO 50 (-t), 360 Hz, 20000 probek, 1 klient xqry (sink=null, rdzenie 0-1)
- cien: shadow_pacer.py na rdzeniu 2, FIFO 50, zero pracy
- powtorzen: 5; miedzy powtorzeniami bez rebootu (reboot rozdziela badania, nie powtorzenia)
- pytanie: czy zdarzenia >= 5 ms u silnika widzi rownoczesnie niezalezny proces na sasiednim rdzeniu?

## Powtorzenie 1

- launch_mono_ns=910402227238 exit_mono_ns=966199380867

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.8 | 79.5 | 33.870 | 47.535 | 83 |
| shadow | 19413 | 27.7 | 32.9 | 0.048 | 0.096 | 0 |

**engine**: 83 zdarzen (slot, t_wzgl [s], wake_lag [ms]):

```
  slot        0  t=    0.000s  47.535 ms
  slot        1  t=    0.003s  46.528 ms
  slot        2  t=    0.006s  45.069 ms
  slot        3  t=    0.008s  43.580 ms
  slot        4  t=    0.011s  43.104 ms
  slot        5  t=    0.014s  41.634 ms
  slot        6  t=    0.017s  40.163 ms
  slot        7  t=    0.019s  38.697 ms
  slot        8  t=    0.022s  37.228 ms
  slot        9  t=    0.025s  36.769 ms
  slot       10  t=    0.028s  35.317 ms
  slot       11  t=    0.031s  33.870 ms
  slot       12  t=    0.033s  32.425 ms
  slot       13  t=    0.036s  31.987 ms
  slot       14  t=    0.039s  30.560 ms
  slot       15  t=    0.042s  29.150 ms
  slot       16  t=    0.044s  27.737 ms
  slot       17  t=    0.047s  26.332 ms
  slot       18  t=    0.050s  25.933 ms
  slot       19  t=    0.053s  24.531 ms
  slot       20  t=    0.056s  23.135 ms
  slot       21  t=    0.058s  21.743 ms
  slot       22  t=    0.061s  21.352 ms
  slot       23  t=    0.064s  19.970 ms
  slot       24  t=    0.067s  18.593 ms
  slot       25  t=    0.069s  17.223 ms
  slot       26  t=    0.072s  15.856 ms
  slot       27  t=    0.075s  15.496 ms
  slot       28  t=    0.078s  14.138 ms
  slot       29  t=    0.081s  12.783 ms
  slot       30  t=    0.083s  11.425 ms
  slot       31  t=    0.086s  11.074 ms
  slot       32  t=    0.089s  9.763 ms
  slot       33  t=    0.092s  8.435 ms
  slot       34  t=    0.094s  7.109 ms
  slot       35  t=    0.097s  5.781 ms
  slot       36  t=    0.100s  5.463 ms
  slot      679  t=    1.886s  39.924 ms
  slot      680  t=    1.889s  39.245 ms
  slot      681  t=    1.892s  38.336 ms
  slot      682  t=    1.894s  37.381 ms
  slot      683  t=    1.897s  36.481 ms
  slot      684  t=    1.900s  36.529 ms
  slot      685  t=    1.903s  35.542 ms
  slot      686  t=    1.906s  34.565 ms
  slot      687  t=    1.908s  33.586 ms
  slot      688  t=    1.911s  33.603 ms
  slot      689  t=    1.914s  32.595 ms
  slot      690  t=    1.917s  31.605 ms
  slot      691  t=    1.919s  30.625 ms
  slot      692  t=    1.922s  29.626 ms
  slot      693  t=    1.925s  29.621 ms
  slot      694  t=    1.928s  28.599 ms
  slot      695  t=    1.931s  27.586 ms
  slot      696  t=    1.933s  26.576 ms
  slot      697  t=    1.936s  26.553 ms
  slot      698  t=    1.939s  25.520 ms
  slot      699  t=    1.942s  24.512 ms
  slot      700  t=    1.944s  23.496 ms
  slot      701  t=    1.947s  22.494 ms
  ... (23 dalszych)
```

Odstepy miedzy zdarzeniami [s]: min=0.00 mediana=0.00 max=1.79

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.24 s => tolerancja dopasowania +/-0.22 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 83, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:      8492      13962      15619         15       Rescheduling interrupts
< IPI1:       857      19415      11619        793       Function call interrupts
> IPI0:     54954      60965      16089      16164       Rescheduling interrupts
> IPI1:       872      21517      13853        835       Function call interrupts
< IPI5:     55382       2220       2598          3       IRQ work interrupts
> IPI5:     55436       2236       2610         90       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 2

- launch_mono_ns=966866342994 exit_mono_ns=1022553973751

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.5 | 45.5 | 25.167 | 39.222 | 57 |
| shadow | 19375 | 27.7 | 32.7 | 0.055 | 0.099 | 0 |

**engine**: 57 zdarzen (slot, t_wzgl [s], wake_lag [ms]):

```
  slot        0  t=    0.000s  19.492 ms
  slot        1  t=    0.003s  18.494 ms
  slot        2  t=    0.006s  17.039 ms
  slot        3  t=    0.008s  15.553 ms
  slot        4  t=    0.011s  15.065 ms
  slot        5  t=    0.014s  13.584 ms
  slot        6  t=    0.017s  12.117 ms
  slot        7  t=    0.019s  10.647 ms
  slot        8  t=    0.022s  9.187 ms
  slot        9  t=    0.025s  8.729 ms
  slot       10  t=    0.028s  7.278 ms
  slot       11  t=    0.031s  5.836 ms
  slot      709  t=    1.969s  39.222 ms
  slot      710  t=    1.972s  38.560 ms
  slot      711  t=    1.975s  38.612 ms
  slot      712  t=    1.978s  37.646 ms
  slot      713  t=    1.981s  36.687 ms
  slot      714  t=    1.983s  35.742 ms
  slot      715  t=    1.986s  35.759 ms
  slot      716  t=    1.989s  34.787 ms
  slot      717  t=    1.992s  33.795 ms
  slot      718  t=    1.994s  32.818 ms
  slot      719  t=    1.997s  31.829 ms
  slot      720  t=    2.000s  32.281 ms
  slot      721  t=    2.003s  31.298 ms
  slot      722  t=    2.006s  30.297 ms
  slot      723  t=    2.008s  29.280 ms
  slot      724  t=    2.011s  29.244 ms
  slot      725  t=    2.014s  28.247 ms
  slot      726  t=    2.017s  27.238 ms
  slot      727  t=    2.019s  26.204 ms
  slot      728  t=    2.022s  25.167 ms
  slot      729  t=    2.025s  25.144 ms
  slot      730  t=    2.028s  24.121 ms
  slot      731  t=    2.031s  23.103 ms
  slot      732  t=    2.033s  22.083 ms
  slot      733  t=    2.036s  22.066 ms
  slot      734  t=    2.039s  21.048 ms
  slot      735  t=    2.042s  20.033 ms
  slot      736  t=    2.044s  18.995 ms
  slot      737  t=    2.047s  17.978 ms
  slot      738  t=    2.050s  17.945 ms
  slot      739  t=    2.053s  16.928 ms
  slot      740  t=    2.056s  15.912 ms
  slot      741  t=    2.058s  14.891 ms
  slot      742  t=    2.061s  14.872 ms
  slot      743  t=    2.064s  13.835 ms
  slot      744  t=    2.067s  12.806 ms
  slot      745  t=    2.069s  11.788 ms
  slot      746  t=    2.072s  10.798 ms
  slot      747  t=    2.075s  10.778 ms
  slot      748  t=    2.078s  9.753 ms
  slot      749  t=    2.081s  8.736 ms
  slot      750  t=    2.083s  7.726 ms
  slot      751  t=    2.086s  7.713 ms
  slot      752  t=    2.089s  6.693 ms
  slot      753  t=    2.092s  5.667 ms
```

Odstepy miedzy zdarzeniami [s]: min=0.00 mediana=0.00 max=1.94

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 57, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:     54979      60997      16126      16164       Rescheduling interrupts
< IPI1:       874      21632      13862        835       Function call interrupts
> IPI0:    101318     107226      16564      32538       Rescheduling interrupts
> IPI1:       889      23552      16024        878       Function call interrupts
< IPI5:     55437       2238       2611         90       IRQ work interrupts
> IPI5:     55495       2253       2629        106       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 3

- launch_mono_ns=1023215938261 exit_mono_ns=1078899538387

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.6 | 77.6 | 26.470 | 39.485 | 59 |
| shadow | 19373 | 27.8 | 32.0 | 0.052 | 0.125 | 0 |

**engine**: 59 zdarzen (slot, t_wzgl [s], wake_lag [ms]):

```
  slot        0  t=    0.000s  19.536 ms
  slot        1  t=    0.003s  18.543 ms
  slot        2  t=    0.006s  17.097 ms
  slot        3  t=    0.008s  15.621 ms
  slot        4  t=    0.011s  15.143 ms
  slot        5  t=    0.014s  13.670 ms
  slot        6  t=    0.017s  12.229 ms
  slot        7  t=    0.019s  10.786 ms
  slot        8  t=    0.022s  9.341 ms
  slot        9  t=    0.025s  8.889 ms
  slot       10  t=    0.028s  7.442 ms
  slot       11  t=    0.031s  6.027 ms
  slot      710  t=    1.972s  39.167 ms
  slot      711  t=    1.975s  39.485 ms
  slot      712  t=    1.978s  38.556 ms
  slot      713  t=    1.981s  37.607 ms
  slot      714  t=    1.983s  36.630 ms
  slot      715  t=    1.986s  36.672 ms
  slot      716  t=    1.989s  35.708 ms
  slot      717  t=    1.992s  34.779 ms
  slot      718  t=    1.994s  33.823 ms
  slot      719  t=    1.997s  32.842 ms
  slot      720  t=    2.000s  33.308 ms
  slot      721  t=    2.003s  32.346 ms
  slot      722  t=    2.006s  31.359 ms
  slot      723  t=    2.008s  30.389 ms
  slot      724  t=    2.011s  30.401 ms
  slot      725  t=    2.014s  29.421 ms
  slot      726  t=    2.017s  28.445 ms
  slot      727  t=    2.019s  27.470 ms
  slot      728  t=    2.022s  26.475 ms
  slot      729  t=    2.025s  26.470 ms
  slot      730  t=    2.028s  25.490 ms
  slot      731  t=    2.031s  24.489 ms
  slot      732  t=    2.033s  23.489 ms
  slot      733  t=    2.036s  23.484 ms
  slot      734  t=    2.039s  22.484 ms
  slot      735  t=    2.042s  21.483 ms
  slot      736  t=    2.044s  20.487 ms
  slot      737  t=    2.047s  19.492 ms
  slot      738  t=    2.050s  19.500 ms
  slot      739  t=    2.053s  18.498 ms
  slot      740  t=    2.056s  17.475 ms
  slot      741  t=    2.058s  16.483 ms
  slot      742  t=    2.061s  16.495 ms
  slot      743  t=    2.064s  15.506 ms
  slot      744  t=    2.067s  14.486 ms
  slot      745  t=    2.069s  13.477 ms
  slot      746  t=    2.072s  12.524 ms
  slot      747  t=    2.075s  12.541 ms
  slot      748  t=    2.078s  11.536 ms
  slot      749  t=    2.081s  10.532 ms
  slot      750  t=    2.083s  9.536 ms
  slot      751  t=    2.086s  9.539 ms
  slot      752  t=    2.089s  8.527 ms
  slot      753  t=    2.092s  7.522 ms
  slot      754  t=    2.094s  6.502 ms
  slot      755  t=    2.097s  5.490 ms
  slot      756  t=    2.100s  5.483 ms
```

Odstepy miedzy zdarzeniami [s]: min=0.00 mediana=0.00 max=1.94

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 59, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    101345     107254      16603      32538       Rescheduling interrupts
< IPI1:       889      23581      16033        878       Function call interrupts
> IPI0:    147799     153504      17044      48650       Rescheduling interrupts
> IPI1:       908      25348      18377        920       Function call interrupts
< IPI5:     55495       2253       2630        106       IRQ work interrupts
> IPI5:     55552       2267       2651        122       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 4

- launch_mono_ns=1079558416638 exit_mono_ns=1135243563710

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 22.9 | 41.5 | 24.897 | 39.118 | 57 |
| shadow | 19374 | 27.7 | 31.8 | 0.053 | 0.088 | 0 |

**engine**: 57 zdarzen (slot, t_wzgl [s], wake_lag [ms]):

```
  slot        0  t=    0.000s  19.723 ms
  slot        1  t=    0.003s  18.704 ms
  slot        2  t=    0.006s  17.239 ms
  slot        3  t=    0.008s  15.753 ms
  slot        4  t=    0.011s  15.265 ms
  slot        5  t=    0.014s  13.788 ms
  slot        6  t=    0.017s  12.312 ms
  slot        7  t=    0.019s  10.846 ms
  slot        8  t=    0.022s  9.382 ms
  slot        9  t=    0.025s  8.924 ms
  slot       10  t=    0.028s  7.477 ms
  slot       11  t=    0.031s  6.042 ms
  slot      709  t=    1.969s  39.118 ms
  slot      710  t=    1.972s  38.426 ms
  slot      711  t=    1.975s  38.478 ms
  slot      712  t=    1.978s  37.517 ms
  slot      713  t=    1.981s  36.549 ms
  slot      714  t=    1.983s  35.553 ms
  slot      715  t=    1.986s  35.538 ms
  slot      716  t=    1.989s  34.580 ms
  slot      717  t=    1.992s  33.610 ms
  slot      718  t=    1.994s  32.606 ms
  slot      719  t=    1.997s  31.582 ms
  slot      720  t=    2.000s  32.035 ms
  slot      721  t=    2.003s  31.058 ms
  slot      722  t=    2.006s  30.043 ms
  slot      723  t=    2.008s  29.023 ms
  slot      724  t=    2.011s  29.008 ms
  slot      725  t=    2.014s  27.981 ms
  slot      726  t=    2.017s  26.938 ms
  slot      727  t=    2.019s  25.921 ms
  slot      728  t=    2.022s  24.897 ms
  slot      729  t=    2.025s  24.872 ms
  slot      730  t=    2.028s  23.832 ms
  slot      731  t=    2.031s  22.815 ms
  slot      732  t=    2.033s  21.783 ms
  slot      733  t=    2.036s  21.743 ms
  slot      734  t=    2.039s  20.723 ms
  slot      735  t=    2.042s  19.747 ms
  slot      736  t=    2.044s  18.745 ms
  slot      737  t=    2.047s  17.739 ms
  slot      738  t=    2.050s  17.711 ms
  slot      739  t=    2.053s  16.680 ms
  slot      740  t=    2.056s  15.649 ms
  slot      741  t=    2.058s  14.635 ms
  slot      742  t=    2.061s  14.611 ms
  slot      743  t=    2.064s  13.585 ms
  slot      744  t=    2.067s  12.550 ms
  slot      745  t=    2.069s  11.534 ms
  slot      746  t=    2.072s  10.508 ms
  slot      747  t=    2.075s  10.479 ms
  slot      748  t=    2.078s  9.467 ms
  slot      749  t=    2.081s  8.452 ms
  slot      750  t=    2.083s  7.407 ms
  slot      751  t=    2.086s  7.374 ms
  slot      752  t=    2.089s  6.352 ms
  slot      753  t=    2.092s  5.325 ms
```

Odstepy miedzy zdarzeniami [s]: min=0.00 mediana=0.00 max=1.94

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 57, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    147824     153537      17074      48650       Rescheduling interrupts
< IPI1:       910      25380      18391        920       Function call interrupts
> IPI0:    193914     199558      17471      64879       Rescheduling interrupts
> IPI1:       917      27401      20227        965       Function call interrupts
< IPI5:     55552       2267       2651        122       IRQ work interrupts
> IPI5:     55607       2281       2657        136       IRQ work interrupts
(brak roznic w liniach IPI)
```

## Powtorzenie 5

- launch_mono_ns=1135905972225 exit_mono_ns=1191593096273

### Analiza wake_lag (prog zdarzenia: 5.0 ms)

| zrodlo | sloty | mediana [us] | p99 [us] | p99,9 [ms] | max [ms] | zdarzenia >= progu |
|---|---|---|---|---|---|---|
| engine | 19999 | 23.1 | 74.9 | 25.784 | 39.122 | 58 |
| shadow | 19375 | 28.4 | 42.4 | 0.052 | 0.096 | 0 |

**engine**: 58 zdarzen (slot, t_wzgl [s], wake_lag [ms]):

```
  slot        0  t=    0.000s  19.527 ms
  slot        1  t=    0.003s  18.533 ms
  slot        2  t=    0.006s  17.092 ms
  slot        3  t=    0.008s  15.623 ms
  slot        4  t=    0.011s  15.141 ms
  slot        5  t=    0.014s  13.673 ms
  slot        6  t=    0.017s  12.226 ms
  slot        7  t=    0.019s  10.778 ms
  slot        8  t=    0.022s  9.328 ms
  slot        9  t=    0.025s  8.883 ms
  slot       10  t=    0.028s  7.444 ms
  slot       11  t=    0.031s  6.003 ms
  slot      709  t=    1.969s  39.122 ms
  slot      710  t=    1.972s  38.452 ms
  slot      711  t=    1.975s  38.526 ms
  slot      712  t=    1.978s  37.592 ms
  slot      713  t=    1.981s  36.705 ms
  slot      714  t=    1.983s  35.788 ms
  slot      715  t=    1.986s  35.848 ms
  slot      716  t=    1.989s  34.917 ms
  slot      717  t=    1.992s  33.971 ms
  slot      718  t=    1.994s  32.990 ms
  slot      719  t=    1.997s  32.012 ms
  slot      720  t=    2.000s  32.551 ms
  slot      721  t=    2.003s  31.628 ms
  slot      722  t=    2.006s  30.670 ms
  slot      723  t=    2.008s  29.689 ms
  slot      724  t=    2.011s  29.684 ms
  slot      725  t=    2.014s  28.701 ms
  slot      726  t=    2.017s  27.713 ms
  slot      727  t=    2.019s  26.737 ms
  slot      728  t=    2.022s  25.770 ms
  slot      729  t=    2.025s  25.784 ms
  slot      730  t=    2.028s  24.792 ms
  slot      731  t=    2.031s  23.783 ms
  slot      732  t=    2.033s  22.793 ms
  slot      733  t=    2.036s  22.817 ms
  slot      734  t=    2.039s  21.828 ms
  slot      735  t=    2.042s  20.837 ms
  slot      736  t=    2.044s  19.844 ms
  slot      737  t=    2.047s  18.852 ms
  slot      738  t=    2.050s  18.852 ms
  slot      739  t=    2.053s  17.878 ms
  slot      740  t=    2.056s  16.888 ms
  slot      741  t=    2.058s  15.897 ms
  slot      742  t=    2.061s  15.904 ms
  slot      743  t=    2.064s  14.909 ms
  slot      744  t=    2.067s  13.913 ms
  slot      745  t=    2.069s  12.906 ms
  slot      746  t=    2.072s  11.909 ms
  slot      747  t=    2.075s  11.908 ms
  slot      748  t=    2.078s  10.913 ms
  slot      749  t=    2.081s  9.904 ms
  slot      750  t=    2.083s  8.907 ms
  slot      751  t=    2.086s  8.904 ms
  slot      752  t=    2.089s  7.902 ms
  slot      753  t=    2.092s  6.888 ms
  slot      754  t=    2.094s  5.878 ms
```

Odstepy miedzy zdarzeniami [s]: min=0.00 mediana=0.00 max=1.94

**shadow**: zero zdarzen >= progu.


### Koincydencja silnik <-> cien (zgrubna)

- widelki kotwicy silnika: 0.13 s => tolerancja dopasowania +/-0.17 s (JAWNIE zgrubna; rozstrzyganie co do ms = Faza 2, ftrace)
- zdarzen silnika: 58, z dopasowaniem u cienia: 0

### Werdykt Fazy 0 (automatyczny, do weryfikacji w dzienniku)

- silnik ma zdarzenia, cien na sasiednim rdzeniu CZYSTY -> hipoteza platformy system-wide OSLABIONA; wskazanie na rdzen 3 / aktywnosc wlasna silnika (H2). Zastrzezenie: stall ograniczony do jednego rdzenia nadal mozliwy -- rozstrzygnie Faza 2.

### Delta IPI (irq_before -> irq_after, pelne pliki w repo)
```
< IPI0:    193938     199589      17501      64879       Rescheduling interrupts
< IPI1:       919      27435      20243        965       Function call interrupts
> IPI0:    240220     245819      17912      80965       Rescheduling interrupts
> IPI1:       931      30526      22656       1006       Function call interrupts
< IPI5:     55607       2281       2657        136       IRQ work interrupts
> IPI5:     55669       2289       2659        153       IRQ work interrupts
(brak roznic w liniach IPI)
```
