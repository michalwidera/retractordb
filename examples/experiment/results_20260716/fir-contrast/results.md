# Test kontrastowy dsp-simple-fir

- data: 2026-07-16T23:46:14+02:00
- cel: maks. tempo naplywu dla prostego filtru FIR (kontrast z QRS ~14 etapow)
- potok: okno@(1,25) -> mnozenie el-po-el -> .sumc -> zlaczenie; VOLATILE, /dev/shm
- srodowisko: 6.8.0-2049-raspi-realtime, governor performance, taskset -c 3, -t (FIFO 50)
- cmdline: coherent_pool=1M 8250.nr_uarts=1 snd_bcm2835.enable_headphones=0 snd_bcm2835.enable_hdmi=1 snd_bcm2835.enable_hdmi=0  smsc95xx.macaddr=E4:5F:01:2D:A9:EB vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000  console=ttyS0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc cfg80211.ieee80211_regdom=PL ds=nocloud;i=rpi-imager-1784124660514 isolcpus=3 nohz_full=3 rcu_nocbs=3
- probki na stopien: 20000; stopnie [Hz]: 1000 2000 3000
- regula stopu: E1 mediana > budzet stopnia -> przerwij eskalacje

## Stopien 1000 Hz (budzet 1000.0 us)
```
plik           : /dev/shm/fir-contrast/step_1000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 392.4 us
p99            : 415.1 us
max            : 1391.9 us
budżet (1/1000s): 1000.0 us
max / budżet   : 139.2 %   (PRZEKROCZONY)
przepustowość  : 2,522 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 442.7 us
p99            : 593.9 us
p99,9          : 60060.6 us
max            : 69003.4 us
max / budżet   : 6900.3 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 20.8 us
p99            : 26.1 us
p99,9          : 59097.8 us
max            : 68061.2 us
```

## Stopien 2000 Hz (budzet 500.0 us)
```
plik           : /dev/shm/fir-contrast/step_2000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 398.2 us
p99            : 408.3 us
max            : 1383.0 us
budżet (1/2000s): 500.0 us
max / budżet   : 276.6 %   (PRZEKROCZONY)
przepustowość  : 2,524 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 833.5 us
p99            : 104077.2 us
p99,9          : 111794.8 us
max            : 112445.4 us
max / budżet   : 22489.1 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 445.9 us
p99            : 103632.3 us
p99,9          : 111343.8 us
max            : 112017.0 us
```

## Stopien 3000 Hz (budzet 333.3 us)
```
plik           : /dev/shm/fir-contrast/step_3000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 384.3 us
p99            : 398.1 us
max            : 37454.0 us
budżet (1/3000s): 333.3 us
max / budżet   : 11236.2 %   (PRZEKROCZONY)
przepustowość  : 2,556 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 1023456.3 us
p99            : 2067515.9 us
p99,9          : 2083964.7 us
max            : 2085875.5 us
max / budżet   : 625762.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 1023057.7 us
p99            : 2067117.6 us
p99,9          : 2083579.6 us
max            : 2085490.2 us
```

**stopien 3000 Hz: E1 mediana 384.3 us > budzet 333.3 us -- eskalacja przerwana (regula stopu)**
