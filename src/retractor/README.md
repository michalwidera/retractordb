# [RetractorDB](../README.md):xretractor

[comment]: # (VSCode view: Ctrl+k,v)

## RetractorDB - query compiler & executor process

Compilation and execution are gathered in one program called xretractor.
The build phase precedes the query execution process.
The tool also provides methods to dump compiled query execution plans in text form.

This process is called from command prompt with:
```
$xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]               Show program options
  -c [ --onlycompile ]        compile only mode
  -q [ --queryfile ] arg      query set file
  -r [ --quiet ]              no output on screen, skip presenter
  -s [ --status ]             check service status
  -v [ --verbose ]            verbose mode (show stream params)
  -x [ --xqrywait ]           wait with processing for first query
  -k [ --noanykey ]           do not wait for any key to terminate
  -j [ --service ]            service mode: log to stderr (journald), no log file
  -t [ --realtime ]           enable real-time scheduling (SCHED_FIFO, mlockall,
                              absolute wakeup)
  -g [ --config ] arg         config file (TOML); overrides search
  -m [ --llimitqry ] arg (=0) loop iteration limit, 0 - no limit
Config search: /etc/retractor/retractor.toml, $XDG_CONFIG_HOME (or ~/.config)/retractor/retractor.toml (optional)
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
ExecStart=/usr/bin/xretractor --noanykey
KillSignal=SIGTERM
TimeoutStopSec=30
```

### Delivering a query set to a running service

When `xretractor` is started with a query file while another instance is **already
running as a systemd service**, it does not fail with a lock error. Instead it:

1. detects that the running instance is a systemd unit (and whether it is a
   `system` or `--user` unit),
2. compiles the new query set locally to validate it (a parse/compile error stops
   here — nothing is delivered),
3. overwrites the service's query file with the validated set (atomically), and
4. restarts the unit (`systemctl restart` / `systemctl --user restart`), so the
   service reloads the new queries while keeping its unit configuration.

On success it prints `Query ... sent to running service '<unit>'` and exits `0`.

```bash
# xretractor.service is already running
xretractor my-new-queries.rql      # validated, delivered, service restarted
```

Notes:
- This is the path for a **full, persistent** query set (rules, `:STORAGE`,
  `:SUBSTRAT`, rotation) — unlike the lightweight, transient ad-hoc injection over
  IPC (`xqry --adhoc`), which only accepts `SELECT` / `DECLARE`.
- Restarting a **system** unit needs privileges — run with `sudo` if `systemctl
  restart` is denied; a `--user` unit restarts without root.
- Which file is overwritten: the service reports its own query file in the lock
  file; if that is unavailable, the `[service] query_file` config default is used.
- If the running instance is a plain process (not a systemd unit), startup fails
  with the usual single-instance lock error (`no_lock_available`).

### Packaged unit (DEB)

The `.deb` produced by `make packages` ships the unit and wires it up automatically:

- binaries install to `/usr/bin/` (so `ExecStart=/usr/bin/xretractor`),
- the unit installs to `/usr/lib/systemd/system/xretractor.service`,
- its `ExecStart` loads the canonical query file `/etc/retractor/startup.rql`
  (an **empty** file = idle); `postinst` creates it empty if absent,
- the `postinst` maintainer script creates the system user `retractor` and runs
  `systemctl enable xretractor.service` (the service starts on next boot; it is **not**
  started immediately — use `systemctl start xretractor` to start it now),
- `postrm` disables the unit on package removal.

The unit is generated from the template
[`packaging/systemd/xretractor.service.in`](../../packaging/systemd/xretractor.service.in)
(`@RETRACTOR_BIN@` and `@RETRACTOR_QUERY_FILE@` are substituted at build time). Edit the
template, not the generated copy. The query-file path has a single build-time source
(CMake `RETRACTOR_QUERY_FILE`); an administrator can override it at runtime **without
editing the unit** by setting `RETRACTOR_QUERY_FILE=...` in `/etc/retractor/service.env`
(shipped as `service.env.example`, read via the unit's `EnvironmentFile`). Logs:
`journalctl -u xretractor`; status: `systemctl status xretractor` or `xretractor --status`.

## Configuration (TOML)

xretractor and xqry support optional TOML configuration loaded in layers:

- system: `/etc/retractor/retractor.toml`
- user: `$XDG_CONFIG_HOME/retractor/retractor.toml` (or `~/.config/retractor/retractor.toml`)

The later layer overrides keys from previous layers.
If `--config <file>` is used, only that file is loaded.

Missing configuration file is a valid state (defaults are used).

### Validation and warnings

Some numeric options are validated at load time.
If a value is invalid, the process logs `WARN` and falls back to the built-in default.
For suspiciously high values, the process logs `WARN` and keeps the value.

For `storage.dir`, validation is strict:
- if configured, the directory must exist,
- it must be a directory,
- it must be writable by xretractor process.

If any of those checks fail, xretractor reports a configuration error and stops.

### All supported keys

#### [storage]

- `storage.dir` (string, default: empty)
  - Default storage directory.
  - Used only if RQL input does not define its own `:STORAGE` directive.
  - If set, path must exist and be writable; otherwise startup fails with configuration error.

#### [ipc]

- `ipc.queue_buffer_seconds` (int, default: `10`, must be `> 0`)
  - Queue headroom in seconds used by xretractor for per-stream response queues.
  - Higher value increases burst tolerance and memory usage.

- `ipc.min_queue_elements` (int, default: `100`, must be `> 0`)
  - Minimum queue capacity regardless of stream interval.

- `ipc.client_response_max_fails` (int, default: `10`, must be `> 0`)
  - Number of xqry retries while waiting for response in shared memory.
  - Effective wait time is roughly:
    `client_response_max_fails * kClientResponsePollInterval`.

#### [timing]

- `timing.server_startup_wait_s` (int, default: `30`, must be `> 0`)
  - Maximum time xqry waits for server availability with `--wait-server`.

- `timing.server_startup_poll_ms` (int, default: `100`, must be `> 0`)
  - Polling interval while waiting for server startup in xqry.

- `timing.query_no_data_timeout_ms` (int, default: `10000`, must be `> 0`)
  - xqry select-loop no-data timeout. After this time without data, xqry assumes server is dead.

#### [scheduling]

- `scheduling.rt_priority` (int, default: `50`, allowed `1..99`)
  - SCHED_FIFO priority used by xretractor in `--realtime` mode.

#### [paths]

- `paths.lock_dir` (string, default: empty)
  - Directory for singleton lock file.
  - Empty means system temp directory.
  - Recommended for service deployments: `/var/run/retractor` or `$XDG_RUNTIME_DIR`.

#### [service]

- `service.query_file` (string, default: `/etc/retractor/startup.rql`)
  - Query file overwritten when delivering a set to a running service
    (see "Delivering a query set to a running service").
  - Fallback only: normally the running service reports its own query file in the lock.
  - Must match the unit's `ExecStart` argument — this key does **not** change `ExecStart`.

### Example configuration

```toml
[storage]
dir = "/var/lib/retractor/data"

[ipc]
queue_buffer_seconds = 10
min_queue_elements = 100
client_response_max_fails = 10

[timing]
server_startup_wait_s = 30
server_startup_poll_ms = 100
query_no_data_timeout_ms = 10000

[scheduling]
rt_priority = 50

[paths]
lock_dir = "/var/run/retractor"

[service]
query_file = "/etc/retractor/startup.rql"
```

Please notice that this tool has second face when you call it with "only compile" option. This face is required for _Show Diagram_ or _Show query Plan_ actions.

```
$xretractor -c -h
xretractor - compiler & data processing tool.

Usage: xretractor queryfile [option]

Available options:
  -h [ --help ]          show help options
  -c [ --onlycompile ]   compile only mode
  -q [ --queryfile ] arg query set file
  -r [ --quiet ]         no output on screen, skip presenter
  -d [ --dot ]           create dot output
  -m [ --csv ]           create csv output
  -f [ --fields ]        show fields in dot file
  -t [ --tags ]          show tags in dot file
  -s [ --streamprogs ]   show stream programs in dot file
  -u [ --rules ]         show rules in dot file
  -i [ --hideruleprog ]  hide rule program in rules (-u) output
  -p [ --transparent ]   make dot background transparent
  -w [ --diagram ] arg   create diagram output
Log: /tmp/xretractor.log
This software is licensed under the MIT License and is provided ‘as is’,
without warranty of any kind. For more information, see the LICENSE file.
```

## Storage state machine

![Use Case Diagram](http://www.plantuml.com/plantuml/proxy?cache=no&src=https://raw.githubusercontent.com/michalwidera/retractordb/master/src/retractor/UML/storage-access-state.puml)