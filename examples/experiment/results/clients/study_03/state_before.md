# Stan maszyny -- PRZED badaniem

- data: 2026-07-18T17:39:36+02:00
- badanie: campaign=clients study_id=3 rate_hz=360 clients=3 samples=20000

## uname
```
Linux pi400 6.8.0-2049-raspi-realtime #50-Ubuntu SMP PREEMPT_RT Mon Jun 29 16:03:02 UTC 2026 aarch64 aarch64 aarch64 GNU/Linux
```
## dystrybucja
```
PRETTY_NAME="Ubuntu 24.04.4 LTS"
NAME="Ubuntu"
VERSION_ID="24.04"
VERSION="24.04.4 LTS (Noble Numbat)"
VERSION_CODENAME=noble
ID=ubuntu
ID_LIKE=debian
HOME_URL="https://www.ubuntu.com/"
SUPPORT_URL="https://help.ubuntu.com/"
BUG_REPORT_URL="https://bugs.launchpad.net/ubuntu/"
PRIVACY_POLICY_URL="https://www.ubuntu.com/legal/terms-and-policies/privacy-policy"
UBUNTU_CODENAME=noble
LOGO=ubuntu-logo
```
## CPU
```
Architecture:                            aarch64
CPU op-mode(s):                          32-bit, 64-bit
Byte Order:                              Little Endian
CPU(s):                                  4
On-line CPU(s) list:                     0-3
Vendor ID:                               ARM
Model name:                              Cortex-A72
Model:                                   3
Thread(s) per core:                      1
Core(s) per cluster:                     4
Socket(s):                               -
Cluster(s):                              1
Stepping:                                r0p3
CPU(s) scaling MHz:                      100%
CPU max MHz:                             1800.0000
CPU min MHz:                             600.0000
BogoMIPS:                                108.00
Flags:                                   fp asimd evtstrm crc32 cpuid
L1d cache:                               128 KiB (4 instances)
L1i cache:                               192 KiB (4 instances)
L2 cache:                                1 MiB (1 instance)
Vulnerability Gather data sampling:      Not affected
Vulnerability Indirect target selection: Not affected
Vulnerability Itlb multihit:             Not affected
Vulnerability L1tf:                      Not affected
Vulnerability Mds:                       Not affected
Vulnerability Meltdown:                  Not affected
Vulnerability Mmio stale data:           Not affected
Vulnerability Reg file data sampling:    Not affected
Vulnerability Retbleed:                  Not affected
Vulnerability Spec rstack overflow:      Not affected
Vulnerability Spec store bypass:         Vulnerable
Vulnerability Spectre v1:                Mitigation; __user pointer sanitization
Vulnerability Spectre v2:                Vulnerable
Vulnerability Srbds:                     Not affected
Vulnerability Tsa:                       Not affected
Vulnerability Tsx async abort:           Not affected
Vulnerability Vmscape:                   Not affected
```
## Pamiec
```
               total        used        free      shared  buff/cache   available
Mem:           3.7Gi       248Mi       3.2Gi       8.1Mi       328Mi       3.5Gi
Swap:             0B          0B          0B
```
## Fragmentacja pamieci (buddyinfo)
```
Node 0, zone      DMA      5      7      6      4      6      5      5      5      5      6    206 
Node 0, zone    DMA32      2      1      2      0      0      3      2      3      3      7    600 
```
## Load average
```
0.43 0.48 0.27 1/240 1223
```
## Temperatura (strefy termiczne)
```
/sys/class/thermal/thermal_zone0/temp: 37485 m°C
```
## Kernel cmdline (izolacja rdzeni itp.)
```
coherent_pool=1M 8250.nr_uarts=1 snd_bcm2835.enable_headphones=0 snd_bcm2835.enable_hdmi=1 snd_bcm2835.enable_hdmi=0  smsc95xx.macaddr=E4:5F:01:2D:A9:EB vc_mem.mem_base=0x3ec00000 vc_mem.mem_size=0x40000000  console=ttyS0,115200 multipath=off dwc_otg.lpm_enable=0 console=tty1 root=LABEL=writable rootfstype=ext4 rootwait fixrtc cfg80211.ieee80211_regdom=PL ds=nocloud;i=rpi-imager-1784124660514 isolcpus=3 nohz_full=3 rcu_nocbs=3
```
## Governor i czestotliwosci CPU (chwilowe, per rdzen)
```
cpu0: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
cpu1: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
cpu2: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
cpu3: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
throttled: throttled=0x0
```
