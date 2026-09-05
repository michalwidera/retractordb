# [RetractorDB](../README.md):xqry

[comment]: # (VSCode view: Ctrl+k,v)

## xqry - data query tool

This tool allows you to view and download current data from the retractorDB system


```
$ xqry -h
xqry - data query tool.

Usage: xqry [option]

Allowed options:
  -s [ --select ] arg         show this stream
  -t [ --detail ] arg         show details of this stream
  -a [ --adhoc ] arg          adhoc query mode
  -m [ --elimitqry ] arg (=0) limit of elements, 0 - no limit
  -n [ --null ]               if null row appear - skip it in output
  -l [ --hello ]              diagnostic - hello db world
  -k [ --kill ]               kill xretractor server
  -d [ --dir ]                list of queries
  -y [ --diryaml ]            list of queries in yaml format
  -r [ --raw ]                raw output mode (default)
  -g [ --graphite ]           graphite output mode
  -f [ --influxdb ]           influxDB output mode
  -p [ --gnuplot ] arg        x,y - gnuplot output mode
  -z [ --gnuplot-rtl ]        gnuplot output: newest samples on the right
                              (right-to-left scroll)
  -e [ --config ] arg         config file (TOML); overrides search
  -h [ --help ]               produce help message
  -c [ --needctrlc ]          force ctl+c for stop this tool
  -w [ --wait-server ]        poll until xretractor server is available before
                              executing command
  -x [ --server ] arg         target xretractor instance name (default:
                              resolved from the bus)
  -b [ --bus ]                list live xretractor instances and their streams
Branch: issue_238-multiserver:XXXXXXXX, Code compiler: GNU Ver. 15.2.0, Build time: YYMMDDHHmm, Type: Release
Log: /home/michal/.tmp/xqry.log
This software is licensed under the MIT License and is provided ‘as is’,
without warranty of any kind. For more information, see the LICENSE file.
```

> ## Before you call xqry ... 
> xqry tool reads data from working xretractor process.
>
> xretractor must be active and working in background.
> If there is no xretator in background you will see follwoing
> output: 
>```
>$ xqry -d
>IPC: No such file or directory
>```
>as result of listing active queries request.


## Listing streams - `xqry -d`

`-d` prints one row per stream, under a header naming the columns and a
separator line. Column widths adapt to the widest value (header included), so
the table stays aligned. The form is the same as in `xqry --bus`: columns
left-aligned, joined by `" | "`, without edge pipes.

```
$ xqry -d
name  | duration | size | count | location      | cap
------+----------+------+-------+---------------+----
core0 | 1/10     | -1   | 0     | datafile2.dat | 4
core1 | 1/20     | -1   | 0     | datafile3.dat | 12
str1  | 1/30     | 0    | 0     |               | 0
str2  | 1/2      | 0    | 0     |               | 0
```

| column | meaning |
|---|---|
| `name` | stream name |
| `duration` | stream interval (delta) in seconds, printed as a fraction |
| `size` | stored size in bytes; `-1` for a declared source stream, which the server does not store |
| `count` | number of records currently held in the output buffer |
| `location` | source file of a declared stream (`FILE '...'`); empty for computed streams |
| `cap` | buffer capacity in records, as computed by the compiler |

`-y` reports the same listing in YAML (`apiVersion: xqry/v1`), without the
`cap` column and with `size` omitted for declared streams.
