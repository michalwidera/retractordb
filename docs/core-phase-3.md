# Core phase 3: `step()`, in the smallest form that carries a plan

**Status:** the embedded form is **done** - `rdb::embed::Engine` compiles a plan and steps it,
and the Python binding (J1, J2, J3 in [`jupyter-integration.md`](jupyter-integration.md)) sits
on top of it. The daemon's loop is **untouched**. What phase 3 was expected to do to the daemon,
and why this revision did not, is §4.

**Prerequisite for:** stage 1b of [`embedded-roadmap.md`](embedded-roadmap.md) - everything a
notebook, an iOS app or an Android activity does with the engine above reading a `.desc` file.

Phase 1 made engine failures survivable in a host process; phase 2 gave engine state an owner.
Phase 3 answers the question those two left open: **how does a host make the engine compute at
all?** The daemon computes on its own clock, in its own thread, and talks to nobody but `xqry`.
A host wants the opposite - to ask for one time slot, get control back, and read what changed.

## 1. What the embedded engine needed, measured against the roadmap

The iOS roadmap lists five items for this phase. Three of them turn out to be properties of the
*daemon's* loop, and an embedded engine that never enters that loop gets them for free:

| Roadmap item | Where it lives | Embedded engine |
|---|---|---|
| 1. Extract the slot body into `Engine::step()` | `executorsm.cpp` slot loop | **done** - `Engine::step()` runs the same body |
| 2. Replace `IpcServer` with an in-process transport | `ipcServer.*`, `executorsm.cpp` | not needed: a host reads results directly (`record()`, `project()`), there is no subscription and no protocol |
| 3. Drop `_kbhit` / `_getch` | `executorsm.cpp` | moot: the embedded path never calls them |
| 4. Never enter `rtActivate` | `executorsm.cpp` | moot: same |
| 5. Gate `::system()` behind a host callback, off by default | `streamInstance.cpp` | "off" is enforced: a plan with a `SYSTEM` rule is refused at `compile()` (§2.3) |

So the phase reduces to item 1, plus one thing the roadmap did not list because it only shows up
when you try: **the model has to be built with the engine's `MemoryStore`**, or the isolation
phase 2 proved for a bare storage silently does not hold for a running plan (§2.2).

## 2. What was done

### 2.1 The slot body, called by the host

`executorsm::run()` computes one slot as: `TimeLine::getNextTimeSlot()`, the set of streams whose
interval falls on that slot (`getAwaitedStreamsSet`), `dataModel::processRows(set, slot)`, then an
IPC broadcast. Before the first slot it runs `dataModel::processZeroStep()`, which reads the
first record of every declaration.

`Engine::step()` (`src/embed/engine.cc`) does exactly that, minus the broadcast, and returns. The
zero step runs inside the first `step()` call. Nothing was extracted from `executorsm.cpp` into a
shared function: the body is four calls, and the daemon's version interleaves them with two
mutexes, three probe hooks and the wall-clock wait. Sharing it would have meant threading those
through a signature for one caller that uses none of them. The duplication is small and stated
here, and the integration test that pins it is the one that already existed: `it_untileof_stop`'s
plan and input, run through `step()`, must leave the same eight records the daemon leaves
(`ut_embedEngine`, `test_engine.py`).

Everything that lives from `compile()` to `close()` - the `qTree`, the `compiler`, the
`dataModel`, the `TimeLine` and the counters - sits in one private `Plan` struct, in dependency
order, so destruction is the reverse of construction by the language rather than by a comment.
This is the same problem `EpochPublication` solves in `executorsm.cpp` for the daemon.

### 2.2 The model takes a `MemoryStore`

Phase 2 made `rdb::storage` take a store and `Engine` own one, and proved isolation for a storage
opened through `Engine::openStorage()`. A plan does not open its storages that way:
`streamInstance` constructs them, and until now it passed no store, so every `VOLATILE` stream
of every engine in the process still landed in `MemoryStore::processDefault()`.

`dataModel` and `streamInstance` now take an optional `rdb::MemoryStore *` (default `nullptr`,
which is the process default, which is what the daemon gets - it is unchanged). `Engine::compile()`
passes its own. `ut_embedEngine::two_engines_with_the_same_volatile_stream_stay_isolated` and the
Python `test_two_engines_do_not_share_a_volatile_stream` are the assertion phase 2 could not write:
two engines, one `VOLATILE` stream name, different inputs, different outputs.

### 2.3 What `compile()` refuses

Three things a plan can ask for reach state the embedded engine does not have. Each is refused at
`compile()`, as `CompileError`, rather than discovered as a `LogicError` half-way through a slot:

- **`DUMP` rule actions** - `dumpManager` reaches the model through the daemon's global `pProc`
  (`executorsmState.hpp`), which the embedded engine does not publish. Publishing it per engine
  would be a process-wide pointer again, exactly the shape phase 2 removed.
- **`SYSTEM` rule actions** - the roadmap wants `::system()` behind a host callback that is off
  by default. The callback does not exist yet; "off" does.
- **`:ROTATION`** - the rotation counter is `pCounterPtr`, another daemon global. Without it the
  directive would be silently ignored and the plan would write without rotating.

`compile()` also takes `untilEof` (default `true`): declared sources are marked `ONESHOT` before
the model is built, exactly as `xretractor -u` does, and `step()` returns `nullopt` after the slot
in which a source ran dry. The default differs from the daemon's on purpose: a notebook reads a
file as a dataset, and a source that wraps to its start after EOF produces records from data that
already passed - the defect `--until-eof` was introduced to stop.

### 2.4 Reading results

`record(stream, index)` returns a **copy** of one record, oldest first. For a SELECT stream it reads
the storage file into a private buffer; for a declaration it returns a record from the source's
history ring, and says so when the record asked for has already left it. `project(stream, flat
elements, first, count)` is the dense form the Python window is built from: `double` per value,
`NaN` for null, `ConfigError` for a STRING or a pair. Both go through the model's own `storage`
objects, so `VOLATILE` streams read as well as files do.

None of this pins engine memory. That is the "copy, do not pin" decision from the roadmap, and it
is what lets a tensor outlive the next `step()`.

## 3. What the binding built on it (J1, J2, J3)

`retractordb.Engine` is `_core.Engine` (nanobind) plus a Python subclass. The compiled half:
`compile`, `step`, `run`, `streams`, `schema`, `record_count`, `record`, `close`, the context
manager, and `_block`, which turns `project()` into a NumPy array in the requested dtype. The
Python half: `rows()`, `to_numpy()`, `window()` and `Window`.

- **GIL.** `compile`, `step`, `run`, `record` and `_block` release it. `run()` re-acquires it every
  50 ms to call `PyErr_CheckSignals()`, which is how the notebook's stop button turns into
  `KeyboardInterrupt` inside a C++ loop. `test_run_honours_keyboard_interrupt` drives it with
  `_thread.interrupt_main()`, which raises the same flag Ctrl-C does.
- **Logging.** The module replaces the sinks of spdlog's default logger with one that forwards to
  `logging.getLogger("retractordb")`. The engine itself configures nothing (that is phase 2's
  gate); the *host* does, and the module is the host. spdlog is header-only and linked into the
  module with hidden symbols, so the logger being replaced is visible to nothing but `_core`.
- **Exceptions.** `RQLSyntaxError` and `CompileError` are C++ types (`rdb::embed::SyntaxError`,
  `rdb::embed::CompileError`, both `ConfigError`), registered like the rest of the hierarchy. The
  binding parses no message text to decide a type.
- **Windows and DLPack.** `window(stream, fields=, size=, stride=, dtype=)` is a NumPy array of
  shape `(n_windows, size, n_values)` that the `Window` object owns; `__dlpack__` and
  `__dlpack_device__` delegate to it, so `torch.from_dlpack(window)` shares the memory without a
  second copy and without the package importing torch. `retractordb.torch` is the one module
  that imports torch, and only when asked for.

## 4. What phase 3 still owes

**The daemon.** Items 2-4 of the roadmap describe restructuring `executorsm::run()` around
`step()`: an in-process transport in place of `IpcServer`, and a loop that never touches the
terminal or the scheduler. This revision left the daemon's loop as it was, for the reason stated in
§2.1 and for one more: `executorsmState.hpp`'s twenty globals are the *daemon's* state, and
phase 2 §4 deferred them to "when phase 3 restructures the run loop", which is a restructuring of
the service, not a prerequisite of the embedded engine. It stays owed, with the consequence that
`Engine` does not publish `pProc` and therefore refuses `DUMP` (§2.3).

**`core_mutex`.** `dataModel::processRows` takes the global `core_mutex` (defined in
`dataModel.cpp`). Two engines stepping concurrently on two Python threads serialise on it. That is
a throughput limit, not a correctness one, and it goes with the daemon restructuring above.

**The host callback for `SYSTEM`.** Refusing the rule is the "off" the roadmap asked for; the
"on" - a callback the host installs - is unwritten.

**Phase 4 (ingest) and phase 5 (the taxonomy).** Untouched. `SyntaxError` and `CompileError` live
in `rdb/embed/engine.hpp` because only `Engine::compile()` raises them; phase 5 may move them.
