# RetractorDB

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CircleCI](https://dl.circleci.com/status-badge/img/circleci/JNsMwQGBP38ArHVd2CGwAF/WU9ujc7GeVXvtLjU2b5mSf/tree/master.svg?style=shield)](https://dl.circleci.com/status-badge/redirect/circleci/JNsMwQGBP38ArHVd2CGwAF/WU9ujc7GeVXvtLjU2b5mSf/tree/master)

[comment]: # (VSCode view: Ctrl+k,v)

**RetractorDB** is a specialized Edge Signal Processing Engine system designed for processing and recording regular time series data in real-time. The system is optimized for continuous data streams, signal processing, telemetry, and IoT monitoring applications.

> ⚠️ **This is work in progress:** Active development is ongoing.

## Overview

RetractorDB provides a unique approach to time series data management through continuous query processing and real-time data transformation. The system consists of three cooperating programs written in C++ and tested on x64 and ARM64 platforms under Linux.

**Key characteristics:**
- **Real-time processing**: Designed to operate with real-time constraints
- **Deterministic execution**: Sequential, deterministic processing thread
- **Continuous queries**: Queries execute continuously from declaration until system termination
- **Memory-efficient**: Uses shared memory for inter-process communication
- **Stream-oriented**: Built on a custom algebra for regular time series

### Project Links
- **Official website**: [retractordb.com](https://retractordb.com)
- **Documentation**: [documentation.retractordb.com](https://documentation.retractordb.com/) (comprehensive documentation in Polish)
- **Repository**: [github.com/michalwidera/retractordb](https://github.com/michalwidera/retractordb)

## Core Components

RetractorDB consists of three main programs:

| Program | Role |
|---------|------|
| [`xretractor`](src/retractor/README.md) | Engine: compiles `.rql` query sets and executes continuous query plans (optionally as a systemd service, with real-time scheduling via `-t/--realtime`) |
| [`xqry`](src/qry/README.md) | Client: reads results from the running engine via shared memory |
| [`xtrdb`](src/rdb/README.md) | Inspection tool: reads binary storage artifacts and metadata |

## Installation

RetractorDB targets Linux (x64 and ARM64). You can either build it from source or
install a prebuilt package from the GitHub Releases page.

### Option A — install from a release package

Prebuilt packages are published on the
[GitHub Releases page](https://github.com/michalwidera/retractordb/releases).
Each release ships a Debian package and a portable tarball, named after the
project version and the target system, for example (version `0.1.8`):

- `retractordb-0.1.8-Linux.deb`
- `retractordb-0.1.8-Linux.tar.gz`

**Debian / Ubuntu (`.deb`)** — installs binaries into `/usr/bin` and wires up the
systemd service automatically:

```bash
# Download the .deb from the Releases page, then:
sudo apt install ./retractordb-0.1.7-Linux.deb
```

The package `postinst` creates the system user `retractor` and runs
`systemctl enable xretractor.service`. The service is enabled (starts on next
boot) but **not** started immediately — start it now with:

```bash
sudo systemctl start xretractor
systemctl status xretractor        # or: xretractor --status
journalctl -u xretractor           # logs
```

See [src/retractor/README.md](src/retractor/README.md#running-as-a-systemd-service)
for the packaged systemd unit details.

**Portable tarball (`.tar.gz`)** — no service integration, just the binaries:

```bash
tar xzf retractordb-0.1.8-Linux.tar.gz
sudo cp retractordb-0.1.8-Linux/bin/* /usr/local/bin/   # or anywhere on $PATH
```

After install, verify the three binaries are reachable:

```bash
xretractor -h
xqry -h
xtrdb -h
```

### Option B — build from source

The build uses **Conan 2 + CMake + Ninja** and requires **GCC 14+** (C++23,
including `std::println`/`<print>`, absent from libstdc++ 13).
A helper script, [`scripts/buildrdb.sh`](scripts/buildrdb.sh), bootstraps the
toolchain and drives the build. Run it from the repo root, `scripts/`, or
`build/Debug/`.

```bash
git clone https://github.com/michalwidera/retractordb.git
cd retractordb

# 1. Install build dependencies (apt packages + Python venv + Conan)
scripts/buildrdb.sh toolchain

# 2. Configure the Conan profile (C++23) and add the Ninja generator
scripts/buildrdb.sh conan ninja

# 3. Add ~/.local/bin to PATH (the default install prefix)
scripts/buildrdb.sh bashrc        # then restart the shell or: source ~/.bashrc

# 4. Build (Debug); use 'release' for an optimized build
scripts/buildrdb.sh debug
```

Install the binaries (no sudo — prefix defaults to `~/.local`) and run the test
suite from the build directory:

```bash
cd build/Debug
ninja install     # installs xretractor, xqry, xtrdb to ~/.local/bin
ninja test        # optional: unit + integration tests
```

To produce your own `.deb` / `.tar.gz` packages locally:

```bash
scripts/buildrdb.sh release package
```

## Initial configuration

RetractorDB runs with sensible defaults and **needs no configuration file** to
start — a missing config is a valid state. Configuration is optional TOML, loaded
in layers (later layers override earlier ones):

1. system: `/etc/retractor/retractor.toml`
2. user: `$XDG_CONFIG_HOME/retractor/retractor.toml` (or `~/.config/retractor/retractor.toml`)
3. explicit: `xretractor --config <file>` — when given, **only** that file is loaded

Both `xretractor` and `xqry` use the same search. Logs go to `/tmp/xretractor.log`
and `/tmp/xqry.log` (in `--service` mode `xretractor` logs to stderr/journald
instead).

**Storage.** Where stream data and `.desc` / `.meta` artifacts are written is
controlled by the `:STORAGE` directive inside the RQL file. The `[storage] dir`
config key only provides a *default* used when the RQL has no `:STORAGE`
directive — **RQL always wins**. If `storage.dir` is set, the directory must
already exist and be writable by the xretractor process, otherwise startup fails
with a configuration error.

A minimal config that sets a default storage directory:

```toml
[storage]
dir = "/var/lib/retractor/data"
```

For the full list of keys (`[ipc]`, `[timing]`, `[scheduling]`, `[paths]`,
`[service]`), validation rules, and the systemd service setup, see
[src/retractor/README.md](src/retractor/README.md#configuration-toml).

## Running your first query

A query set is a `.rql` file with stream `DECLARE`s and continuous `SELECT`s.
The example below needs no input data files — it reads bytes from `/dev/urandom`.
Save it as `first.rql`:

```sql
# declaration of input time series
DECLARE a BYTE , b BYTE STREAM core0, 1 FILE '/dev/urandom'
DECLARE c BYTE , d BYTE STREAM core1, 0.5 FILE '/dev/urandom'

SELECT core0[0],b STREAM str1 FROM core0#core1
SELECT core1[0]/2+1,a,a+1,b STREAM str2 FROM core1+core0
```

Note: the interlace operator `#` requires both input streams to have the same
number of fields.

(More ready-made examples live under [examples/](examples/), e.g.
[examples/session-record-1/query.rql](examples/session-record-1/query.rql).)

**1. Sanity-check (compile only)** — no data processing, just validate the query
set:

```bash
xretractor -c first.rql
```

**2. Start the engine.** `xretractor` compiles the queries and then runs them
continuously until terminated. Start it (here in the foreground; press a key to
stop, or use `-k` to disable that):

```bash
xretractor first.rql
```

If you installed via the `.deb`, the engine usually runs as a systemd service
instead. To load a query set into an **already running** service, just start
`xretractor` with your `.rql` file — it detects the running service, validates
(compiles) the queries, overwrites the service's query file and restarts the unit
to apply them, keeping the unit configuration:

```bash
xretractor my-new-queries.rql      # delivered to the running service via restart
```

This is the path for a full, persistent query set (rules, `:STORAGE`, rotation),
as opposed to the transient ad-hoc injection over IPC (`xqry --adhoc`). Use
`systemctl`/`journalctl` to manage and inspect the service. See
[src/retractor/README.md](src/retractor/README.md#delivering-a-query-set-to-a-running-service)
for details.

**3. Query the running engine** with the `xqry` client (in a second terminal).
`xretractor` must be running — `xqry` reads results from shared memory:

```bash
xqry -d              # list active streams/queries
xqry -s str1         # stream out results of stream 'str1'
xqry -s str2
```

`xqry` also offers other output modes (`--graphite`, `--influxdb`, `--gnuplot`)
and an ad-hoc query mode (`--adhoc`); see
[src/qry/README.md](src/qry/README.md) and `xqry -h`. To stop the engine you can
also run `xqry --kill`.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

## Author

Created and maintained by **Michal Widera** (2003-2026)  
Contact: michal@widera.com.pl
