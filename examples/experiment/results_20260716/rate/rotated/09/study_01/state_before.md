# Stan maszyny -- PRZED badaniem

- data: 2026-07-16T22:32:45+02:00
- badanie: campaign=rate study_id=1 rate_hz=360 clients=1 samples=20000

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
Mem:           3.7Gi       283Mi       2.6Gi        13Mi       858Mi       3.4Gi
Swap:             0B          0B          0B
```
## Fragmentacja pamieci (buddyinfo)
```
Node 0, zone      DMA    178    162    147     91     71     65     38     22     23     22    184 
Node 0, zone    DMA32   7900   9338   5094   3012   2460   1796    905    496    233    108     71 
```
## Load average
```
3.43 2.01 1.02 1/238 2958
```
## Temperatura (strefy termiczne)
```
/sys/class/thermal/thermal_zone0/temp: 40407 m°C
```
## Governor i czestotliwosci CPU (chwilowe, per rdzen)
```
cpu0: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
cpu1: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
cpu2: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
cpu3: governor=performance cur=1800000 kHz min=600000 max=1800000 kHz
throttled: throttled=0x0
```
