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
  -q [ --reset ] arg          replace the whole plan of the target instance
                              with this RQL file
  -m [ --elimitqry ] arg (=0) limit of elements, 0 - no limit
  -n [ --null ]               if null row appear - skip it in output
  -l [ --hello ]              diagnostic - hello db world
  -k [ --kill ]               kill xretractor server
  -d [ --dir ]                list of queries
  -y [ --yaml ]               yaml output format for --dir, --detail and --bus
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

## One command at a time

`-s`, `-t`, `-a`, `-q`, `-d`, `-b` and `-l` each do something different, so giving
two of them at once is refused with exit code `22` instead of silently running
the first one. This also stops a whole class of typos: boost glues a value onto
a short option, so `xqry -t str1 -yaml` parses as `-y -a ml`, i.e. an ad-hoc
query `ml` sent to the server instead of the requested detail.

`-k` is not part of the set - `xqry -s <stream> -m N -k` (kill after the element
budget) and `xqry -k -a "..."` are deliberate combinations.

## Reloading the plan - `xqry --reset <file.rql>`

`--reset` (short `-q`) replaces the **whole** query plan of the target instance with the
contents of an RQL file. It is not `--adhoc`: ad-hoc adds a single statement to a running
plan, `--reset` swaps the plan itself, including directives (`STORAGE`, `SUBSTRAT`,
`:ROTATION`) that ad-hoc refuses.

```
$ xqry --reset plan.rql --server service
Plan accepted by the server and scheduled for loading: plan.rql
```

The client reads the file and sends its **contents**, not the path: the service usually runs
under its own account (`User=retractor` in the systemd unit) and could not open the
operator's file.

The server validates the whole set - parse, compile, and stream-name disjointness against the
other live instances - **before** it touches the running plan. A refusal therefore costs
nothing; the previous plan keeps computing and the exit code is non-zero:

```
$ xqry --reset broken.rql --server service
xqry: plan reload refused at reset-commit: Fail compile:...
```

An accepted plan replaces the running one at the end of the current slot: stream artifacts of
the old plan are dropped exactly as at start-up (unless the plan carries `:ROTATION`),
subscribed clients are told the show is over, and the new names are claimed on the bus. A file
with no statements at all is a legal request - it returns the instance to the **idle** state,
serving nothing and waiting for the next plan.

An instance started as a service (`--service`, `XRETRACTOR_SERVICE=1`, or a systemd unit) is
named `service` on the bus unless `--name` says otherwise, so the target is the same on every
machine: `--server service`. With exactly one live instance `--server` may be omitted.

## Output format - `-y`

`-y` is a format modifier, not a command on its own: it switches the answer of
`-d`, `-t` and `--bus` to YAML (`apiVersion: xqry/v1`). Combined with any other
command (`-s`, `-a`, `-l`, `-k`) - or given alone - it is refused with exit code
`22`, so a flag that cannot take effect never looks like one that did.

`xqry -d -y` reports the stream listing without the `cap` column, and with
`size` omitted for declared streams:

```
$ xqry -d -y
---
apiVersion: xqry/v1
streams:
  - name: srca
    delta: 1
    count: 8
    location: data_alfa.txt
  - name: dsta
    delta: 1
    size: 28
    count: 7
```

## Stream details - `xqry -t <stream>`

`-t` prints the stream header and its field list as two column tables, in the
same form as `-d` and `--bus`:

```
$ xqry -t dsta
name | delta | query
-----+-------+---------------------------------------
dsta | 1     | SELECT srca[0]+1 STREAM dsta FROM srca

field       | type
------------+--------
dsta.dsta_0 | INTEGER
```

`xqry -t <stream> -y` reports the same in YAML:

```
$ xqry -t dsta -y
---
apiVersion: xqry/v1
stream:
  name: dsta
  delta: 1
query: SELECT srca[0]+1 STREAM dsta FROM srca
fields:
  dsta.dsta_0:
    type: INTEGER
```

An unknown stream name prints nothing and exits with code `2`, in both forms.

## Live instances - `xqry --bus`

`--bus` lists the live xretractor instances read from the bus segment, without
contacting any server. `--bus -y` reports the same list in YAML. Two differences
follow from the format: the query path is given **in full** (the table shortens
it to `.../<dir>/<file>` for readability, which a consumer cannot open), and the
MODE letter legend is not printed - its meaning belongs in this documentation,
not in a machine-readable document. An empty bus still yields a document:
`servers: []` on stdout, with the `no live xretractor instance` note on stderr.

```
$ xqry --bus -y
---
apiVersion: xqry/v1
servers:
  - name: smokea
    pid: 116472
    modes: N
    query: "/home/michal/plans/alfa.rql"
    streams:
      - srca
      - dsta
```
