# Wyniki kampanii baseline-numpy

- data: 2026-07-17T20:16:32+02:00
- cel: baseline float64 dla 7.5(i) main-debs.tex (koszt dokladnej semantyki wymiernej)
- potok: etapy identyczne z rec205-detect.rql, float64, prawdziwe dzielenie
- implementacja: config/pan_tompkins_numpy.py (orientacja okna jak ref_float.py)
- srodowisko: 6.8.0-2049-raspi-realtime, governor performance, taskset -c 3, chrt -f 50 (slot)
- cmdline: coherent_pool=1M 8250.nr_uarts=1 snd_bcm2835.enable_headphones=0 snd_bcm2835.enable_hdmi=1 snd_bcm2835.enable_hdmi=0  smsc95xx.macaddr=E4:5F:01:2D:A9:EB vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000  console=ttyS0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc cfg80211.ieee80211_regdom=PL ds=nocloud;i=rpi-imager-1784124660514 isolcpus=3 nohz_full=3 rcu_nocbs=3
- tryby NIEPOROWNYWALNE wprost: slot = koszt na slot (z narzutem CPython),
  batch = przepustowosc wektoryzowana bez semantyki slotowej

## Wersje (przypiete dla odtwarzalnosci)
```
python 3.12.3
numpy 1.26.4
```

## Tryb slot: 360 Hz, 20000 probek (sonda zgodna z RDB_BENCH_CSV)
```
META {"python": "3.12.3", "numpy": "1.26.4", "scheduler": "FIFO", "rt_priority": 50, "affinity": [3], "rate_hz": 360.0, "samples": 20000}
plik           : /dev/shm/numpy-baseline/probe_slot.csv
interwałów (N) : 20000
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 77.0 us
p99            : 215.8 us
max            : 328.0 us
budżet (1/360s): 2777.8 us
max / budżet   : 11.8 %   (MIEŚCI SIĘ)
przepustowość  : 11,970 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 112.8 us
p99            : 293.7 us
p99,9          : 310.7 us
max            : 368.6 us
max / budżet   : 13.3 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 23.2 us
p99            : 42.1 us
p99,9          : 49.8 us
max            : 56.1 us
```

## Tryb batch: 650000 probek x 5
```
META {"python": "3.12.3", "numpy": "1.26.4", "scheduler": "OTHER", "rt_priority": 0, "affinity": [3], "rate_hz": null, "samples": 650000}
batch: N=650000 powtórzeń=5
czas przebiegu  : mediana 460.5 ms (min 391.7, max 528.4)
czas / próbkę   : 709 ns
przepustowość   : 1,411,410 próbek/s (batch, bez semantyki slotowej)
```

## Metryki systemowe w trakcie badania
```
srednie load1=0.48 mem_used_kb=342946 temp_mC=36787
```

## Pliki w tym katalogu
- state_before.md / state_after.md -- pelny stan maszyny przed/po
- probe_slot.csv -- surowa sonda trybu slot (iter,compute_ns,wake_lag_ns,e2e_ns)
- slot_stdout.log / batch_stdout.log -- pelne wyjscie obu trybow (w tym META)
- metrics.csv -- probkowanie load/mem/temp co 1s w trakcie badania
