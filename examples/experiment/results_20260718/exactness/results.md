# Wyniki kampanii exactness (7.6: Exactness and Replay Stability)

- data: 2026-07-18T07:57:09+02:00
- branch: experiment/20260718, binarka: Branch: experiment/20260718:526d510, Code compiler: GNU Ver. 14.2.0, Build time: 2607180746, Type: Release
- probki: 20000 @ 360 Hz (rec205), przebiegi unpaced (rownosc bitowa, nie czas)
- srodowisko: governor performance, taskset -c 3, /dev/shm, isolcpus wg cmdline

## Badanie A — replay: dwa identyczne przebiegi potoku QRS (17 strumieni)

Werdykt: **OK**

```
IDENTYCZNY          STREAM_ADD_STREAM_ADD_mlii_v1_mwi
IDENTYCZNY          STREAM_ADD_STREAM_ADD_mlii_v1_mwi.desc
IDENT-PO-TIMESTAMP  STREAM_ADD_STREAM_ADD_mlii_v1_mwi.meta
IDENTYCZNY          STREAM_ADD_STREAM_ADD_mlii_v1_mwi.shadow
IDENTYCZNY          STREAM_ADD_mlii_v1
IDENTYCZNY          STREAM_ADD_mlii_v1.desc
IDENT-PO-TIMESTAMP  STREAM_ADD_mlii_v1.meta
IDENTYCZNY          STREAM_ADD_mlii_v1.shadow
IDENTYCZNY          bp_acc
IDENTYCZNY          bp_acc.desc
IDENT-PO-TIMESTAMP  bp_acc.meta
IDENTYCZNY          bp_acc.shadow
IDENTYCZNY          bp_out
IDENTYCZNY          bp_out.desc
IDENT-PO-TIMESTAMP  bp_out.meta
IDENTYCZNY          bp_out.shadow
IDENTYCZNY          bp_win
IDENTYCZNY          bp_win.desc
IDENT-PO-TIMESTAMP  bp_win.meta
IDENTYCZNY          bp_win.shadow
IDENTYCZNY          bpf.desc
IDENTYCZNY          d_acc
IDENTYCZNY          d_acc.desc
IDENT-PO-TIMESTAMP  d_acc.meta
IDENTYCZNY          d_acc.shadow
IDENTYCZNY          d_out
IDENTYCZNY          d_out.desc
IDENT-PO-TIMESTAMP  d_out.meta
IDENTYCZNY          d_out.shadow
IDENTYCZNY          detect_out
IDENTYCZNY          detect_out.desc
IDENT-PO-TIMESTAMP  detect_out.meta
IDENTYCZNY          detect_out.shadow
IDENTYCZNY          df.desc
IDENTYCZNY          ecg.desc
IDENTYCZNY          mlii
IDENTYCZNY          mlii.desc
IDENT-PO-TIMESTAMP  mlii.meta
IDENTYCZNY          mlii.shadow
IDENTYCZNY          mlii_win
IDENTYCZNY          mlii_win.desc
IDENT-PO-TIMESTAMP  mlii_win.meta
IDENTYCZNY          mlii_win.shadow
IDENTYCZNY          mwi
IDENTYCZNY          mwi.desc
IDENT-PO-TIMESTAMP  mwi.meta
IDENTYCZNY          mwi.shadow
IDENTYCZNY          mwi_long
IDENTYCZNY          mwi_long.desc
IDENT-PO-TIMESTAMP  mwi_long.meta
IDENTYCZNY          mwi_long.shadow
IDENTYCZNY          mwi_thr
IDENTYCZNY          mwi_thr.desc
IDENT-PO-TIMESTAMP  mwi_thr.meta
IDENTYCZNY          mwi_thr.shadow
IDENTYCZNY          mwi_win
IDENTYCZNY          mwi_win.desc
IDENT-PO-TIMESTAMP  mwi_win.meta
IDENTYCZNY          mwi_win.shadow
IDENTYCZNY          sq_out
IDENTYCZNY          sq_out.desc
IDENT-PO-TIMESTAMP  sq_out.meta
IDENTYCZNY          sq_out.shadow
IDENTYCZNY          v1
IDENTYCZNY          v1.desc
IDENT-PO-TIMESTAMP  v1.meta
IDENTYCZNY          v1.shadow
```

## Badanie B — tozsamosc okrezna przeplotu/rozplotu (cor:exact)

Werdykt: **OK**

```
c: n=39995 przeplot b0,a0,b1,a1,...: parzyste==b:True (n=19998) nieparzyste==a:True (n=19997)
a2: n=19997 rekord0_null_placeholder:True a2[1:]==a[:n-1]:True
b2: n=19997 b2==b[:n]:True
VERDICT: OK
```

## Badanie C — referencja float (scipy resample round-trip 360->720->360 Hz)

```
META {"python": "3.12.3", "numpy": "1.26.4", "scipy": "1.11.4", "rec205_samples": 650000, "grid": [1250, 2500, 5000, 10000, 20000, 40000, 80000, 160000, 325000, 650000]}
method,n,margin,max_abs_full,rms_full,max_abs_interior,rms_interior
poly,1250,64,1.844440e+02,6.214364e+00,3.886652e-01,1.109661e-01
fft,1250,64,5.684342e-13,1.725345e-13,5.684342e-13,1.735268e-13
poly,2500,125,1.844440e+02,4.406402e+00,3.886652e-01,1.084146e-01
fft,2500,125,7.958079e-13,2.029493e-13,7.958079e-13,2.004818e-13
poly,5000,250,1.844440e+02,3.110222e+00,4.639125e-01,1.113857e-01
fft,5000,250,7.958079e-13,1.886105e-13,6.821210e-13,1.872442e-13
poly,10000,500,1.844440e+02,2.200769e+00,4.639125e-01,1.105795e-01
fft,10000,500,9.094947e-13,2.122567e-13,9.094947e-13,2.121850e-13
poly,20000,1000,1.844440e+02,1.557249e+00,5.195810e-01,1.118478e-01
fft,20000,1000,9.094947e-13,2.041098e-13,9.094947e-13,2.042078e-13
poly,40000,2000,1.844440e+02,1.105489e+00,5.195810e-01,1.111415e-01
fft,40000,2000,1.023182e-12,2.186263e-13,1.023182e-12,2.186670e-13
poly,80000,4000,1.844440e+02,7.852104e-01,5.638860e-01,1.109640e-01
fft,80000,4000,1.023182e-12,2.009614e-13,1.023182e-12,2.005453e-13
poly,160000,8000,1.844440e+02,5.610651e-01,5.638860e-01,1.115750e-01
fft,160000,8000,1.023182e-12,2.136511e-13,1.023182e-12,2.134822e-13
poly,325000,16250,1.844440e+02,4.017345e-01,6.423174e-01,1.122586e-01
fft,325000,16250,1.250555e-12,2.581843e-13,1.136868e-12,2.556216e-13
poly,650000,32500,1.844440e+02,2.979793e-01,6.423174e-01,1.125267e-01
fft,650000,32500,1.364242e-12,2.550164e-13,1.364242e-12,2.530441e-13
```

## Metryki systemowe
```
srednie load1=1.61 mem_used_kb=310194 temp_mC=39300
```

## Pliki w tym katalogu
- replay_hashes_run1/2.txt — sha256 wszystkich artefaktow obu przebiegow
- replay_compare.txt — porownanie per plik (dane/.desc/.shadow bitowo; .meta po odcieciu 8 B naglowka)
- roundtrip_compare.txt — porownania bitowe c/a2/b2 z a/b
- float_roundtrip.csv — normy bledu float w funkcji N (metody poly i fft)
- state_before.md / state_after.md, metrics.csv — stan i probkowanie maszyny
