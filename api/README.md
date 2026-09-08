# Stream monitoring API

The [Python package](python/README.md) and [C++ library](cpp/README.md) monitor
streams from a running, explicitly named `xretractor` on the same Linux host.
Each subscription owns one `xqry` child process. Multiple subscriptions can
coexist in one application despite IPC identifying subscribers by PID.
Closing a client closes its subscriptions and never sends `xqry --kill`.

Use matching `xqry` and `xretractor` builds from this revision. Older clients
lack `--jsonl`; older servers do not publish field multiplicity. The API does
not start servers, load plans, reconnect or replay missed samples. It provides
live monitoring, not durable or lossless delivery.

## Building the API

The API is developed and tested alongside the engine but is not part of it.
No engine target links anything under `api/`, and every default entry point —
`ninja`, `ninja install`, `ninja test`, `ninja package` — leaves the API out.
Each opt-in is separate:

| Command | Effect |
| --- | --- |
| `ninja install-withapi` | Builds the engine and the API, then installs both, the API as component `api`. |
| `ninja test-api` | Builds `test_api_client` and runs `ctest -L api` (`st_api_fake`, `st_api_real`). |
| `cmake -DRDB_WITH_API=ON .` | Adds component `api` to the cpack packages; `ninja test` then stops filtering the `api` label. |

`api/cpp` is configured unconditionally, so its targets are always available by
name; `EXCLUDE_FROM_ALL` keeps them out of `all`, and the install rules carry
`COMPONENT api EXCLUDE_FROM_ALL`. Only the packaging switch is a configure-time
flag: cpack fixes package contents while configuring, so no build target can
change them. Without `-DRDB_WITH_API=ON` the `.deb` and `.tar.gz` contain the
three engine binaries, the systemd unit and the config examples — never the
API. `it_packaging` asserts exactly that set.

The engine keeps its own Boost.JSON translation unit in `src/qry/boostJson.cpp`
so that `xqry --jsonl` does not depend on `api/`; `api/cpp` compiles the same
header into its own `rdb_api_json`. The dependency direction is API on engine,
never the reverse.

`api/cpp` is also a standalone CMake project (see its README). In that use the
component variable is empty and installation behaves normally.

## JSON Lines v1

```sh
xqry --server laboratory --jsonl --hello
xqry --server laboratory --jsonl --dir
xqry --server laboratory --jsonl --detail temperature
xqry --server laboratory --jsonl --select temperature --elimitqry 10
```

Each stdout line is a complete UTF-8 JSON object with `version: 1` and `event`.
Diagnostics use stderr. Supported events:

| Event | Contents |
| --- | --- |
| `pong` | Server answered `hello`. |
| `streams` | `streams`: array of `{name, delta}`; empty for an idle server. |
| `schema` | `stream`, rational `delta`, original `query`, ordered `fields`. |
| `record` | `stream`, flattened ordered `values`. |
| `end` | `reason`: `limit` or `server_stopped_or_reloaded`. |
| `error` | Stable `code` and descriptive `message`; process exits nonzero. |

A subscription emits its schema, records, and one final end/error event. EOF
without a final event is a process failure, even with exit status zero.
The schema confirms subscription acceptance, not first-sample availability.

```json
{"version":1,"event":"schema","stream":"temperature","delta":"1/20","query":"...","fields":[{"name":"v","type":"INTEGER","count":2}]}
{"version":1,"event":"record","stream":"temperature","values":["21",null]}
{"version":1,"event":"end","reason":"limit"}
```

Field `count` is its scalar multiplicity: numeric arrays use their full length,
while `STRING[N]` is one string. Values follow descriptor and array element
order. Non-null wire values are strings, interpreted using the schema. This
preserves rational numerators/denominators and distinguishes a string `"null"`
from JSON `null`. Language APIs decode them:

| RDB type | Python | C++ `Value` alternative |
| --- | --- | --- |
| NULL value | `None` | `std::monostate` |
| BYTE, INTEGER, UINT | `int` | `std::int64_t` |
| FLOAT, DOUBLE | `float` | `double` |
| RATIONAL | `fractions.Fraction` | `Rational {numerator, denominator}` |
| INTPAIR | `(int, int)` | `std::pair<int64_t, int64_t>` |
| IDXPAIR | `(str, int)` | `std::pair<std::string, int64_t>` |
| STRING | `str` | `std::string` |

Floating point precision remains limited by the existing server IPC text
serializer. Messages have neither a source timestamp nor a durable sequence
number; arrival time is not sample time. The existing IPC queue message limit
of 1024 bytes, including INFO framing, still applies, and it now bounds the
number of VALUES rather than the number of fields: a row that does not fit is
dropped by the server. In practice this caps a subscribable stream at roughly a
hundred numeric elements per record.

## Lifecycle and limits

One-shot commands and initial schema acquisition have a timeout (default 5 s).
A per-read timeout leaves the subscription open for retry. `idle_timeout` /
`idleTimeout` terminates a subscription after that long without data; zero
(default) disables it for slow or initially non-causal streams. A server crash
without a shutdown marker may require a read timeout or explicit `close()`.

Both stdout and stderr are drained. Buffers are bounded: 1024 pending events
by default, 1 MiB per JSONL line, and the last 64 KiB of stderr. Application
queue overflow raises `buffer_overflow` and closes the subscription without
silently dropping records. A row too long for the 1024-byte response queue slot
is a server-side limit, not a client one: the server logs the stream name and
the required size once per stream, sends nothing for that stream, and keeps
serving every other subscriber. The subscription stays open and produces no
records, so on the client it surfaces as an idle timeout or a read timeout.

Closing is idempotent: SIGTERM to the owned child, up to one second to exit,
then SIGKILL if necessary, followed by reaping. Use Python context managers or
C++ RAII. Serialize calls on each handle; libraries manage their own reader
threads. Libraries do not install host signal handlers. Applications must
arrange normal cleanup on termination; SIGKILL cannot execute cleanup.

Error codes include `read_timeout`, `idle_timeout`, `buffer_overflow`,
`stream_not_found`, `no_active_plan`, `server_stopping`, `server_no_response`,
`client_queue_missing`, `disconnected`, `communication_error`, `protocol_error`,
`spawn_error`, `process_exit`, `process_timeout`, and `closed`.

## Tests

API tests carry the ctest label `api`. Run them through the dedicated target,
which also builds the C++ test client:

```sh
ninja -C build/Debug test-api
```

Two mechanisms keep them off the default path, because the two ways of running
tests need different ones. `ninja test` passes `-LE api` and never reaches
them. A bare `ctest` — how CI invokes the suite — ignores that filter, so
`api_clients.py` exits 77 (`SKIP_RETURN_CODE`) when `test_api_client` has not
been built, and ctest reports `Skipped` rather than a failure. Once the binary
exists, a bare `ctest` runs the API tests for real.

```sh
ninja -C build/Debug test_api_client
ctest --test-dir build/Debug -R '^(st_api_fake|st_api_real|ut_ipcClient)$' --output-on-failure
```

The fake-process test exercises both languages against escaped strings,
per-element NULLs, rationals, malformed output, unexpected exit, queue overflow,
noisy stderr, and a child ignoring SIGTERM. The real-process test checks
independent subscriptions, full arrays, NULLs, rationals, missing streams and
server shutdown with one executing server.
