# Runtime, IPC, and service lifecycle

## Contents

- [Runtime objects](#runtime-objects)
- [Timeline and processing cycle](#timeline-and-processing-cycle)
- [FROM execution](#from-execution)
- [SELECT and rules](#select-and-rules)
- [Execution timing](#execution-timing)
- [IPC protocol](#ipc-protocol)
- [Ad-hoc behavior](#ad-hoc-behavior)
- [Configuration and service mode](#configuration-and-service-mode)

## Runtime objects

- `qTree` stores compiled `query` objects and their computed `rInterval`.
- `dataModel` owns `qSet: stream id -> streamInstance`.
- Each `streamInstance` owns an input `payload` for FROM computation and an output `storage` for SELECT results/history.
- Compiler directives are collected by `dataModel` and removed from the executable `qTree`.
- `dumpManager` belongs to each stream instance and manages pending DUMP tasks.

Construction creates stream instances after compiler capacities have been assigned. A declared stream receives gap detection based on its rational interval and its output storage can be marked disposable.

## Timeline and processing cycle

`CRationalStreamMath::TimeLine` accepts all positive stream deltas, removes rates that are integer multiples of smaller basis rates, and advances to the smallest next exact rational slot. A stream is due when `current_slot / stream_delta` is an integer.

Startup performs a **zero step**:

1. Put each declared source buffer into `flux`.
2. `revRead(0)` prefetches into the chamber.
3. `fire()` publishes the chamber and leaves the source `armed`.
4. Broadcast the initial declaration set.

Each later slot:

1. Build the set of streams due at that exact slot.
2. For every due non-declaration in topological order:
   - construct FROM/input payload;
   - evaluate SELECT fields into output payload;
   - write output storage;
   - evaluate and execute rules.
3. Re-arm/prefetch due declarations for their next step.
4. Broadcast due subscribed streams.

`core_mutex` protects model operations shared with the IPC command thread.

## FROM execution

`dataModel::constructInputPayload` dispatches on the final canonical token:

- pass-through reads the current source payload;
- shift uses reverse history offset;
- `#` calls exact `Hash` to select a source and forward index;
- `&`/`%` call `Div`/`Mod`, then convert a forward index to reverse storage history;
- stream `+` concatenates two payloads;
- stream difference selects a history position via `Subtract`;
- reductions call `streamInstance::reduceFieldsToPayload`;
- AGSE calls `constructAgsePayload`.

Unavailable forward records become an all-null payload. Left deinterleave is mathematically one slot non-causal, so runtime makes it causal by emitting all-null at output record 0 and reconstructing `a_(n-1)` thereafter. Right deinterleave is available from record 0.

AGSE flattens the source record history. It caches descriptors by absolute window width, reads each source record at most once per constructed window, preserves per-element null, and reverses element order for negative width.

## SELECT and rules

`constructOutputPayload` evaluates every compiled field program against `inputPayload` and writes into output storage payload.

`constructRulesAndUpdate` evaluates rules in declaration order after the record is stored:

- `SYSTEM` executes the configured shell command; nonzero status is logged and processing continues.
- `DUMP` registers a task spanning past/current/future records. Negative history requirements contribute to compiler buffer capacity. Optional retention uses a circular set of dump files.

Every matching sample can register a separate task; repeated true conditions can produce overlapping dumps.

## Execution timing

Normal mode uses relative `sleep_for(period)`. Linux real-time mode:

- checks platform capabilities;
- activates `SCHED_FIFO` with configurable priority;
- locks memory;
- uses `CLOCK_MONOTONIC` absolute wakeups from an anchor set after real-time activation.

Build option `RDB_BENCH_PROBE` enables opt-in runtime probes:

- `RDB_BENCH_CSV` records `compute_ns`, wake lag, and end-to-end emission latency.
- `RDB_BENCH_PLAN` records plan stream/token sizes around substrate deduplication.

Recent repository experiments investigate throughput ceilings and the previously observed roughly 40 ms startup/client-attach latency; do not generalize experiment results without reading `examples/experiment` and current branch artifacts.

## IPC protocol

Boost.Interprocess resources:

- command queue `RetractorQueryQueue`;
- shared-memory segment `RetractorShmemMap`;
- map object `MyMap`, protected by named mutex `RetractorMapMutex`, for request/response control messages keyed by client PID;
- per-subscriber message queue `brcdbr<PID>` for streamed rows.

The server has a processing thread and `commandProcessorLoop` communication thread. The lock file is acquired only after IPC resources report ready, preventing a client from observing a nominally active but unreachable server.

Control commands include:

- `hello` -> `world`;
- `get` -> stream inventory;
- `detail <stream>` -> schema/query details;
- `show <stream>` -> subscribe and create the per-PID response queue;
- `adhoc <statement>` -> parse/compile/import a new query plan;
- `kill` -> set the stop flag.

The response queue capacity is `max(min_queue_elements, rate * queue_buffer_seconds)`. `xqry` has its own producer thread that deserializes Info-format messages into a lock-free SPSC queue.

Broadcast messages contain stream id, count, values, and encoded null map. Client renderers produce raw, Gnuplot, Graphite, or InfluxDB output. Graphite/Influx skip null fields; Gnuplot renders null as `NaN`; raw optionally suppresses all-null rows.

On shutdown, `xretractor` sends reserved stream id `OUT_OF_BUSSINESS` to subscribers, removes all named IPC resources, joins the communication thread, and releases the service lock. `SIGINT`, `SIGTERM`, `SIGHUP`, and `xqry --kill` converge on this path.

## Ad-hoc behavior

An ad-hoc RQL statement is parsed into a temporary `qTree`, compiled, and imported into the live plan if ids do not already exist. New runtime instances are added for imported queries. Treat changes here carefully: live import must preserve existing query objects, capacities, subscriptions, and source state.

## Configuration and service mode

`AppConfig` loads either:

- an explicit `--config` file, which must exist and parse; or
- layered optional files `/etc/retractor/retractor.toml`, then `$XDG_CONFIG_HOME/retractor/retractor.toml` or the user config equivalent.

Later layers override earlier keys. Invalid implicit layers are warned and skipped; invalid numeric values fall back to defaults. Current sections:

- `[storage] dir`;
- `[ipc] queue_buffer_seconds`, `min_queue_elements`, `client_response_max_fails`;
- `[timing] server_startup_wait_s`, `server_startup_poll_ms`, `query_no_data_timeout_ms`;
- `[scheduling] rt_priority`;
- `[paths] lock_dir`;
- `[service] query_file`.

RQL `STORAGE` overrides configured `storage.dir`.

No query file in execution/service mode is valid idle operation: IPC and the lock remain active but no `dataModel` or `TimeLine` is constructed. An empty service startup file supports the same operational pattern.

The singleton lock records whether the owner is a service or process, scope, unit, and query-file path. When a new `xretractor` invocation finds a running systemd service:

1. It parses and compiles the new query set locally.
2. It atomically delivers the file to the query path reported by the lock (or configured fallback).
3. It invokes the appropriate system/user `systemctl restart`.

A plain process owner is not restarted; the new process fails to acquire the lock.
