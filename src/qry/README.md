# [RetractorDB](../README.md):xqry

[comment]: # (VSCode view: Ctrl+k,v)

## xqry - data query tool

This tool allows you to view and download current data from the retractorDB system

> :warning: **This is work in progress**: This readme can be outdated.

```
$ xqry -h
xqry - data query tool.

Usage: xqry [option]

Allowed options:
  -s [ --select ] arg         show this stream
  -t [ --detail ] arg         show details of this stream
  -a [ --adhoc ] arg          adhoc query mode
  -m [ --tlimitqry ] arg (=0) limit of elements, 0 - no limit
  -l [ --hello ]              diagnostic - hello db world
  -k [ --kill ]               kill xretractor server
  -d [ --dir ]                list of queries
  -y [ --diryaml ]            list of queries in yaml format
  -r [ --raw ]                raw output mode (default)
  -g [ --graphite ]           graphite output mode
  -f [ --influxdb ]           influxDB output mode
  -p [ --gnuplot ] arg        x,y - gnuplot output mode
  -h [ --help ]               produce help message
  -c [ --needctrlc ]          force ctl+c for stop this tool
Branch: issue_31-doc:eb1aba1, Code compiler: GNU Ver. 13.3.0, Build time: 2512170028, Type: Debug
Log: /tmp/xqry.log
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
>No such file or directory
>catch IPC server
>```
>as result of listing active queries request.

