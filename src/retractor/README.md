# [RetractorDB](../README.md):xretractor

[comment]: # (VSCode view: Ctrl+k,v)

## RetractorDB - query compiler & executor process

Compilation and execution are gathered in one program called xretractor.
The build phase precedes the query execution process.
The tool also provides methods to dump compiled query execution plans in text form.

> :warning: **This is work in progress**: This readme can be outdated.

This process is called from command prompt with:
```
$xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]               Show program options
  -s [ --status ]             check service status
  -q [ --queryfile ] arg      query set file
  -v [ --verbose ]            verbose mode (show stream params)
  -x [ --xqrywait ]           wait with processing for first query
  -k [ --noanykey ]           do not wait for any key to terminate
  -m [ --tlimitqry ] arg (=0) query limit, 0 - no limit
  -c [ --onlycompile ]        compile only mode
Branch: issue_31-doc:eb1aba1, Code compiler: GNU Ver. 13.3.0, Build time: 2512170028, Type: Debug
Log: /tmp/xretractor.log
This software is licensed under the MIT License and is provided ‘as is’,
without warranty of any kind. For more information, see the LICENSE file.
```

## Running as a systemd service

xretractor can run as a Linux systemd service without any wrapper/supervisor process.
It runs in the foreground (`Type=simple`) and shuts down cleanly on `SIGTERM`.

Relevant options:
- `-j` / `--service` — service mode: log to **stderr** (captured by journald), no log file in `/tmp`.
- `-k` / `--noanykey` — do not wait for a key/TTY (required without a terminal).
- starting **without** a query file boots an **idle** instance that stays alive until `SIGTERM`
  (no crash-loop before any query is defined); pass a `.rql` file to load queries.

Service logging mode can also be enabled with the `XRETRACTOR_SERVICE` environment variable
(any value other than empty or `0`), which is convenient in a systemd unit via `Environment=`:

```ini
[Service]
Type=simple
Environment=XRETRACTOR_SERVICE=1
ExecStart=/usr/local/bin/xretractor --noanykey
KillSignal=SIGTERM
TimeoutStopSec=30
```

A ready-to-use unit file is provided in [`packaging/systemd/xretractor.service`](../../packaging/systemd/xretractor.service).
Logs are then available via `journalctl -u xretractor`; status via `systemctl status xretractor` or `xretractor --status`.

Please notice that this tool has second face when you call it with "only compile" option. This face is required for _Show Diagram_ or _Show query Plan_ actions.

```
$xretractor -c -h
xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]          show help options
  -q [ --queryfile ] arg query set file
  -r [ --quiet ]         no output on screen, skip presenter
  -d [ --dot ]           create dot output
  -m [ --csv ]           create csv output
  -f [ --fields ]        show fields in dot file
  -t [ --tags ]          show tags in dot file
  -s [ --streamprogs ]   show stream programs in dot file
  -u [ --rules ]         show rules in dot file
  -p [ --transparent ]   make dot background transparent
  -w [ --diagram ] arg   create diagram output
  -c [ --onlycompile ]   compile only mode
Branch: issue_31-doc:eb1aba1, Code compiler: GNU Ver. 13.3.0, Build time: 2512170028, Type: Debug
Log: /tmp/xretractor.log
This software is licensed under the MIT License and is provided ‘as is’,
without warranty of any kind. For more information, see the LICENSE file.
```

## Storage state machine

![Use Case Diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/michalwidera/retractordb/master/src/retractor/UML/storage-access-state.puml)