# Stan maszyny -- PO badaniu (sledztwo 40ms, badanie engine-shadow)

- data: 2026-07-18T14:54:57+02:00
- parametry: samples=20000 repeats=5 rate=360Hz

## uname
```
Linux pi400 6.8.0-2049-raspi-realtime #50-Ubuntu SMP PREEMPT_RT Mon Jun 29 16:03:02 UTC 2026 aarch64 aarch64 aarch64 GNU/Linux
```
## Kernel cmdline
```
coherent_pool=1M 8250.nr_uarts=1 snd_bcm2835.enable_headphones=0 snd_bcm2835.enable_hdmi=1 snd_bcm2835.enable_hdmi=0  smsc95xx.macaddr=E4:5F:01:2D:A9:EB vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000  console=ttyS0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc cfg80211.ieee80211_regdom=PL ds=nocloud;i=rpi-imager-1784124660514 isolcpus=3 nohz_full=3 rcu_nocbs=3
```
## Load / pamiec
```
1.28 1.22 0.97 1/236 3566
               total        used        free      shared  buff/cache   available
Mem:           3.7Gi       314Mi       2.6Gi        39Mi       891Mi       3.4Gi
Swap:             0B          0B          0B
```
## Temperatura
```
/sys/class/thermal/thermal_zone0/temp: 42842 m°C
```
## Governor per rdzen
```
cpu0: performance @ 1800000 kHz
cpu1: performance @ 1800000 kHz
cpu2: performance @ 1800000 kHz
cpu3: performance @ 1800000 kHz
throttled: throttled=0x0
```
