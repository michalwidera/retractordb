# Test kontrastowy dsp-simple-fir

- data: 2026-07-21T20:54:48+02:00
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
mediana        : 259.7 us
p99            : 274.6 us
max            : 1235.3 us
budżet (1/1000s): 1000.0 us
max / budżet   : 123.5 %   (PRZEKROCZONY)
przepustowość  : 3,800 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 310.2 us
p99            : 328.3 us
p99,9          : 614.8 us
max            : 1280.9 us
max / budżet   : 128.1 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 20.4 us
p99            : 24.4 us
p99,9          : 35.7 us
max            : 320.2 us
```

## Stopien 2000 Hz (budzet 500.0 us)
```
plik           : /dev/shm/fir-contrast/step_2000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 255.1 us
p99            : 266.3 us
max            : 1228.0 us
budżet (1/2000s): 500.0 us
max / budżet   : 245.6 %   (PRZEKROCZONY)
przepustowość  : 3,909 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 559.0 us
p99            : 646.2 us
p99,9          : 1243.0 us
max            : 1638.7 us
max / budżet   : 327.7 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 304.2 us
p99            : 361.7 us
p99,9          : 657.3 us
max            : 1348.6 us
```

## Stopien 3000 Hz (budzet 333.3 us)
```
plik           : /dev/shm/fir-contrast/step_3000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 254.0 us
p99            : 260.4 us
max            : 1226.0 us
budżet (1/3000s): 333.3 us
max / budżet   : 367.8 %   (PRZEKROCZONY)
przepustowość  : 3,935 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 609.7 us
p99            : 967.9 us
p99,9          : 1699.0 us
max            : 1949.9 us
max / budżet   : 585.0 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 332.6 us
p99            : 683.4 us
p99,9          : 1443.7 us
max            : 1670.2 us
```

## Stopien 4000 Hz (budzet 250.0 us)
```
plik           : /dev/shm/fir-contrast/step_4000/probe.csv
interwałów (N) : 19999
--- E1: rdzeń obliczeń (processRows) ---
mediana        : 251.3 us
p99            : 264.7 us
max            : 1229.3 us
budżet (1/4000s): 250.0 us
max / budżet   : 491.7 %   (PRZEKROCZONY)
przepustowość  : 3,946 próbek/s (unpaced)
--- E2E: krotka wejściowa -> emisja wyniku (deadline -> boradcast) ---
mediana        : 434389.5 us
p99            : 924220.2 us
p99,9          : 931372.8 us
max            : 932493.9 us
max / budżet   : 372997.6 %   (PRZEKROCZONY)
--- jitter pobudki (wake_lag) ---
mediana        : 434136.5 us
p99            : 923966.9 us
p99,9          : 931119.4 us
max            : 932241.1 us
```

**stopien 4000 Hz: E1 mediana 251.3 us > budzet 250.0 us -- eskalacja przerwana (regula stopu)**
