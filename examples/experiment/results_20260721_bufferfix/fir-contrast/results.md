# Test kontrastowy dsp-simple-fir

- data: 2026-07-21T23:21:48+02:00
- cel: maks. tempo naplywu dla prostego filtru FIR (kontrast z QRS ~14 etapow)
- potok: okno@(1,25) -> mnozenie el-po-el -> .sumc -> zlaczenie; VOLATILE, /dev/shm
- srodowisko: 6.8.0-2049-raspi-realtime, governor performance, taskset -c 3, -t (FIFO 50)
- cmdline: coherent_pool=1M 8250.nr_uarts=1 snd_bcm2835.enable_headphones=0 snd_bcm2835.enable_hdmi=1 snd_bcm2835.enable_hdmi=0  smsc95xx.macaddr=E4:5F:01:2D:A9:EB vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000  console=ttyS0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc cfg80211.ieee80211_regdom=PL ds=nocloud;i=rpi-imager-1784124660514 isolcpus=3 nohz_full=3 rcu_nocbs=3
- probki na stopien: 20000; stopnie [Hz]: 1000 2000 3000 4000 5000
- regula stopu: E1 mediana > budzet stopnia -> przerwij eskalacje

## Stopien 1000 Hz (budzet 1000.0 us)
```
plik           : /dev/shm/fir-contrast/step_1000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 259.4 us
p99            : 270.0 us
max            : 431.8 us
budżet (1/1000s): 1000.0 us
max / budżet   : 43.2 %   (MIEŚCI SIĘ)
przepustowość  : 3,851 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 303.5 us
p99            : 329.1 us
p99,9          : 360.5 us
max            : 465.0 us
max / budżet   : 46.5 %   (MIEŚCI SIĘ)
--- jitter pobudki (wake_lag) ---
mediana        : 20.7 us
p99            : 23.7 us
p99,9          : 25.8 us
max            : 28.9 us
```

## Stopien 2000 Hz (budzet 500.0 us)
```
plik           : /dev/shm/fir-contrast/step_2000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 258.7 us
p99            : 268.6 us
max            : 429.7 us
budżet (1/2000s): 500.0 us
max / budżet   : 85.9 %   (MIEŚCI SIĘ)
przepustowość  : 3,847 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 489.0 us
p99            : 646.3 us
p99,9          : 680.8 us
max            : 758.8 us
max / budżet   : 151.8 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 48.2 us
p99            : 362.9 us
p99,9          : 387.2 us
max            : 463.9 us
```

## Stopien 3000 Hz (budzet 333.3 us)
```
plik           : /dev/shm/fir-contrast/step_3000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 256.2 us
p99            : 271.1 us
max            : 426.1 us
budżet (1/3000s): 333.3 us
max / budżet   : 127.8 %   (PRZEKROCZONY)
przepustowość  : 3,875 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 617.4 us
p99            : 1423.0 us
p99,9          : 3726.0 us
max            : 4191.7 us
max / budżet   : 1257.5 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 337.7 us
p99            : 1141.6 us
p99,9          : 3446.2 us
max            : 3906.6 us
```

## Stopien 4000 Hz (budzet 250.0 us)
```
plik           : /dev/shm/fir-contrast/step_4000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 254.1 us
p99            : 256.8 us
max            : 427.5 us
budżet (1/4000s): 250.0 us
max / budżet   : 171.0 %   (PRZEKROCZONY)
przepustowość  : 3,933 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 443977.3 us
p99            : 889382.8 us
p99,9          : 896898.3 us
max            : 898090.2 us
max / budżet   : 359236.1 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 443719.6 us
p99            : 889125.8 us
p99,9          : 896642.2 us
max            : 897832.0 us
```

**stopien 4000 Hz: E1 mediana 254.1 us > budzet 250.0 us -- eskalacja przerwana (regula stopu)**
