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
  -m [ --tlimitqry ] arg (=0) limit of elements, 0 - no limit
  -l [ --hello ]              diagnostic - hello db world
  -k [ --kill ]               kill xretractor server
  -d [ --dir ]                list of queries
  -g [ --graphite ]           graphite output mode
  -f [ --influxdb ]           influxDB output mode
  -r [ --raw ]                raw mode (default)
  -h [ --help ]               show options
  -c [ --needctrlc ]          force ctl+c for stop this tool
Branch: master:b38fb68, Code compiler: GNU Ver. 13.2.0, Build time: 2308192116, Type: Debug
This is free software; see the source for copying conditions.
There is NO warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
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

