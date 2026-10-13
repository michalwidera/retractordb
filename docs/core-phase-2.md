# Core phase 2: one owner per engine instance

**Status:** stage C2 of #446. The Memory Store of the MEMORY substrate (§2), the descriptor format flag (§3) and the owner object `rdb::embed::Engine` (§5) are done, and the gate that holds them in place (§4) runs in `ctest`. What phase 2 still owes is listed in §6.

**Prerequisite for:** stage C3 (#491) - `Engine::compile()` / `step()` and the binding that drives a whole plan from Python.

A daemon constructs its state once, so process-wide state and engine-wide state are indistinguishable. A notebook constructs it on every cell run, and the difference becomes the bug. Phase 2 answers the question a host process asks and a daemon never does: **what happens when you build the engine twice?**

`api/python/tests/test_reentry.py` pins the sequential case from stage 1a. It is a baseline, not the proof of isolation - see §6.

## 1. The inventory

| State | Where | Whose |
|---|---|---|
| MEMORY records, NULL maps, write counter | `faccmemory.cc`, one `static` map of buckets | the engine's - **fixed, §2** |
| `Descriptor::singleLineOutput_` | `descriptor.hpp`, class static | the engine's - **fixed, §3** |
| `fatalErrorRaised` | `src/include/fatalError.hpp` | the xretractor exit path's - **stays, §4** |
| spdlog default logger | `uxSysTermTools.cpp` and the launchers | the binaries' - **not an issue, §4** |
| probe counters | `probe.hpp`, 3 `inline` variables | the engine's - **known debt, §6** |
| executor state (`pProc`, `core_mutex`, `esm::*`) | `executorsmState.hpp` | the server's - **§6** |

The list is short: the storage layer kept one map and one flag. Two of its entries are not visible to a grep for `static` at namespace scope - a class static and header `inline` variables - and turned up only once the sweep was written as an executable gate (§4). An inventory that is not enforced is a snapshot of one afternoon's greps.

## 2. The Memory Store

The MEMORY substrate kept its data in one `static std::map<std::string, memoryBucket>` in `faccmemory.cc`, keyed by stream name. It is now `rdb::MemoryStore` (`src/include/rdb/memoryStore.hpp`), an object the caller can own.

**The sharing is not the bug.** Within one plan the writer and the reader of a MEMORY stream are *different* `memoryFile` objects; if each kept its own vectors, the reader would see nothing. The bug was the *scope*: the owner was the whole process, so two engines built in one process shared any stream whose name collided, silently.

The change is an ownership decision - **explicit Memory Store, default shared**:

- `MemoryStore` owns the map of buckets. The bucket layout is the one `master` already had: one node per stream (data, NULL maps, write counter, record size, live-user count), looked up once in the `memoryFile` constructor, erased by the destructor of the last instance using it, and cleared when an instance of another record size arrives. Phase 2 moves the owner, not the layout, so the hot path is unchanged.
- `memoryFile` takes a `MemoryStore &`, defaulting to `MemoryStore::processDefault()`.
- `makeAccessor` takes a trailing `MemoryStore *` (null selects the default), and so does the `rdb::storage` constructor, which passes it down when it builds the accessor.
- Every existing caller - xretractor in service and `-u` mode, xtrdb - passes nothing and therefore behaves exactly as before.

`ut_faccmemory` carries the distinction in four cases: two Memory Stores with the same stream name stay independent; one Memory Store still shares between instances and drops the bucket with the last one; no Memory Store means the process default; and a Memory Store given to `rdb::storage` reaches the accessor.

## 3. The descriptor format flag

`Descriptor::singleLineOutput_` was a static class member: a process-wide flag in the layer a notebook loads, raised by the `rdb::singleLineFormat` manipulator and **cleared by `operator<<` itself** after one use. Two threads printing descriptors raced over the format of a third's output, and the binding carried a save-and-restore workaround around every `repr()`.

It now lives in a `std::ios_base::xalloc()` slot of the stream being written to, read and written through `rdb::isSingleLineOutput(os)` / `rdb::setSingleLineOutput(os, bool)`. Every call site is unchanged - `os << rdb::singleLineFormat << desc` reads the same, and the one-shot semantics are the same - but "single line" now means *this output* rather than *this process*. The binding's workaround is gone; `streamToString` in `src/python/module.cpp` formats into a local `ostringstream` that cannot affect anyone.

The output itself does not change: xtrdb (`list`, `rlist`, `descc`, `print`) writes the manipulator and the object to the same `std::cout`, exactly as before. The integration tests that compare xtrdb output byte for byte, and the exact strings in `ut_payload` and `ut_dataModel`, hold that. `ut_descriptor` adds the one case the old flag could not pass: a flag raised on one stream does not reach another.

A word on how nearly this went wrong: `grep singleLineOutput` finds almost no callers, because the manipulator that sets it is named `singleLineFormat` and has about 35 call sites across xtrdb and the tests. Read as dead code, it would have been deleted.

## 4. The gate

`test/embedding_boundary.py`, registered as the `embedding_boundary` test, asserts four properties of `src/rdb/lib` and `src/include/rdb` - the library and its public headers, not the xtrdb tool that shares the directory:

1. no file-scope mutable state (`static` / `inline` variables);
2. no mutable static class members - the category the first sweep missed;
3. no logger *configuration*, only use: every `set_default_logger`, sink, pattern and level lives in the binaries, and the library writes to whatever default logger the host configured;
4. the only way out of the process is `std::exit` in `detail::terminateProcess` (`src/include/fatalError.hpp`). This rule also covers `src/embed` and the header itself, and catches `exit`, `_exit`, `_Exit`, `quick_exit` and `abort`.

Rule 4 deliberately allows `FatalError` (K1) and `EnvironmentError` (K4) in the library. Under decision (a) of #463 they throw `rdb::EngineFatal` inside a `rdb::ThrowOnFatalScope` instead of ending the process, so the boundary "the library does not kill its host" is held by that single exit path, not by the absence of calls. For the same reason `fatalErrorRaised` stays in `src/include/fatalError.hpp`, next to `ThrowOnFatalScope`: it describes the xretractor exit path, and inside the throw-mode scope it is never raised.

Two lists keep the gate honest. `ALLOWED` is for deliberate, permanent exceptions, each with its reason: the `MemoryStore::processDefault()` instance, the `xalloc` index (an index allocated once, not state) and the `std::exit` in `terminateProcess`. `KNOWN_DEBT` is for real defects with a plan - printed on every run, not fatal, and meant to shrink. An allowlist that hides a defect is worse than no gate at all.

## 5. The owner: `rdb::embed::Engine`

De-globalizing needs somewhere for the state to go, and `rdb::storage` alone is not that place: compile and step (C3) will need the same owner. The decision: **create the facade now, in minimal form.** `rdb::embed::Engine` (`src/include/rdb/embed/engine.hpp`, library `rdbembed`, linking only `rdb`) owns a `MemoryStore` and builds storages bound to it through `openStorage()`. Nothing else yet; C3 adds its methods to an object that already exists.

Three properties of the class are load-bearing rather than stylistic:

- It is **non-movable and non-copyable**. A storage holds a raw `MemoryStore *` into the engine; moving the engine would relocate the Memory Store and leave every storage built from it pointing at nothing.
- It is **optional**. `rdb::storage` still defaults to `MemoryStore::processDefault()`, so the server, xtrdb and the existing binding are untouched and know nothing about `Engine`.
- It is the **error boundary**. `openStorage()` runs inside `rdb::ThrowOnFatalScope`: a K1 or K4 failure while building the storage reaches the caller as `rdb::EngineFatal` and the host process lives on, without raising `fatalErrorRaised`. Refusals (K2/K3) are values (`rdb::Refusal`) and pass through unchanged.

`ut_embedEngine` turns these into assertions: two engines, the same MEMORY stream name, no shared records; a storage built without an engine uses the process default; and a K1 in `openStorage()` arrives as `EngineFatal{Internal}` with the process still running.

## 6. What phase 2 still owes

**The probe counters.** `probe.hpp` holds three `inline` counter objects. Every access sits inside `if constexpr (rdb_probe_*)`, and `RDB_BENCH_PROBE` defaults to **OFF**: in every build that is not a measurement campaign the counters are dead bytes in `.bss`, so two engines in one process have no way to interfere through them. They come alive only in a probe build, and a probe build runs **one** engine by construction. Moving them into the engine object would buy isolation for a case the measurement never runs and pay for it with a pointer chase in exactly the loop the probe measures, so it is not done. They stay in the gate's `KNOWN_DEBT`, because "the campaign is single-engine" is an assumption about how the build is used, not a property the code enforces.

**`dataModel` and `streamInstance`.** A plan compiled by the engine builds its storages inside `dataModel`; passing the engine's Memory Store down that path belongs to C3, together with `compile()` and `step()`.

**`executorsmState.hpp`.** Roughly twenty globals - `pProc`, `core_mutex`, `plan_epoch_mutex`, the `esm::` group. They are the server's, and no notebook links them today. The split of the core library in a later stage decides which of them the embedded engine needs.

**The Python half of the isolation claim.** Memory Store state only diverges on a **write**, and the stage 1a binding is deliberately read-only, so two Python `Storage` objects over the same stream cannot make the old global misbehave no matter how they are scoped. Exposing `Engine` to Python now would add a public type no test could exercise. `test_reentry.py` says this in its own docstrings; the claim itself is carried by `ut_embedEngine` until the binding can write.
