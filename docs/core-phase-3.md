# Core phase 3: `step()`, in the smallest form that carries a plan

**Status:** stage C3 of #446 (#491). `rdb::embed::Engine` compiles a plan and steps it, and the Python binding (J1, J2, J3 in [`jupyter-integration.md`](jupyter-integration.md)) sits on top of it. The library split that keeps the service out of the embedded engine is §2.1. The service's loop is **untouched**; what phase 3 still owes is §5.

Phase 2 ([`core-phase-2.md`](core-phase-2.md)) gave engine state an owner. Phase 3 answers the question it left open: **how does a host make the engine compute at all?** The service computes on its own clock, in its own thread, and talks to nobody but `xqry`. A host wants the opposite - to ask for one time slot, get control back, and read what changed.

## 1. What the embedded engine needed

The service's slot loop does five things around the computation: it waits on the wall clock, broadcasts results over IPC, polls the terminal (`_kbhit`), enters the real-time scheduler (`rtActivate`) and reads DEVICE sources in a phase of its own (#347). An embedded engine that never enters that loop needs none of them: a host reads results directly (`record()`, `project()`), there is no subscription and no protocol, and the clock is the host's call to `step()`.

So the phase reduces to running the slot body on request, plus two things that only show up when you try: **the model has to be built with the engine's Memory Store**, or the isolation phase 2 proved for a bare storage silently does not hold for a running plan (§2.3), and **the engine must not link the service**, or a notebook would carry the bus and the IPC server into the kernel (§2.1).

## 2. What was done

### 2.1 `retractorcore` and `retractor`

`src/retractor/lib` used to build one static library, `retractor`, from every `.cpp` in the directory. It now builds two:

- **`retractorcore`** - an explicit list of 17 files: `CRSMath`, `compiler`, `coreState`, `dataModel`, `dumpManager`, `executor_rt`, `exprSimplify`, `expressionEvaluator`, `expressionShape`, `field`, `persistentCounter`, `planSource`, `qTree`, `query`, `RQLParser`, `streamInstance`, `token`. The plan, the compiler, the data model and the time line - nothing that talks to the bus, the IPC queues or the service configuration. The list is explicit on purpose: a new file joins the core by decision, not by a glob. The `RDB_OPT_*` definitions are `PUBLIC` on this target, so every consumer - the service, `xretractor`, the unit tests, `rdbembed` - sees the values that compiled the core.
- **`retractor`** - the rest (the executor loop, `bus`, `ipcServer`, `ipcResponses`, `serverName`, `lockManager`, `lockFile`, `shmBudget`, `appConfig`, `presenter`, `serviceControl`), linking `retractorcore`.

`rdbembed` links `retractorcore` and `rdb` only. The core shares two globals with the service: `pCounterPtr` (read by `streamInstance` for `:ROTATION`) and `pProc` (read by `dumpManager`). They moved from `executorsmState.cpp`, which also defines the global `IpcServer` and the bus pointers, into `coreState.cpp`; `core_mutex` was already defined in `dataModel.cpp`. The static libraries now declare the dependencies they always had (`retractorcore` -> `rdb rqlparser antlr4_static`, `rdb` -> `descparser antlr4_static`), because CMake cannot order a static library on the link line it was never told about.

`nm` on `librdbembed.a` and on the Python module shows no symbol from `bus.*`, `ipcServer.cpp`, `serverName`, `lockManager` or `shmBudget` (#446, criterion on the bus).

### 2.2 The slot body, called by the host

`executorsm::run()` computes one slot as: `TimeLine::getNextTimeSlot()`, the positional mask of streams whose interval falls on that slot (`collectAwaitedStreams`), `dataModel::processRows(mask, slot)`, then an IPC broadcast. Before the first slot it runs `dataModel::processZeroStep()`, which reads the first record of every declaration.

`Engine::step()` (`src/embed/engine.cc`) does exactly that, minus the broadcast, and returns. The zero step runs inside the first `step()` call. Nothing was extracted from `executorsm.cpp` into a shared function: the body is four calls, and the service's version interleaves them with two mutexes, the probe hooks, the DEVICE phase and the wall-clock wait. Sharing it would have meant threading those through a signature for one caller that uses none of them. The duplication is small and stated here, and the plan that pins it is the one that already existed: `it_untileof_stop`'s plan and input, run through `step()`, must leave the same eight records the service leaves (`ut_embedEngine`, `test_engine.py`).

Everything that lives from `compile()` to `close()` - the `qTree`, the `compiler`, the `dataModel`, the `TimeLine` and the counters - sits in one private `Plan` struct, in dependency order, so destruction is the reverse of construction by the language rather than by a comment. This is the same problem `EpochPublication` solves in `executorsm.cpp` for the service.

### 2.3 The model takes a Memory Store

Phase 2 made `rdb::storage` take a Memory Store and `Engine` own one, and proved isolation for a storage opened through `Engine::openStorage()`. A plan does not open its storages that way: `streamInstance` constructs them, and until now it passed no Memory Store, so every `VOLATILE` stream of every engine in the process still landed in `MemoryStore::processDefault()`.

`dataModel` and `streamInstance` now take an optional `rdb::MemoryStore *` (default `nullptr`, which is the process default, which is what the service gets - it is unchanged). `Engine::compile()` passes its own. `ut_embedEngine` (`two_engines_with_the_same_volatile_stream_stay_isolated`) and the Python `test_two_engines_do_not_share_a_volatile_stream` are the assertion phase 2 could not write: two engines, one `VOLATILE` stream name, different inputs, different outputs.

`dumpManager::processStreamChunk` now looks up its task book before it touches `pProc`. `streamInstance` calls it for every stream in every slot, and the embedded engine never publishes `pProc`; with an empty book the pointer is not needed. The service path is unchanged: a registered task still requires `pProc`, and its absence is still `FatalError`.

### 2.4 What `compile()` does, and what it refuses

`compile()` follows the service's start: `parsePlanText`, `compiler::compile()`, the storage preflight `checkPlanStorage` (#473), `dropStalePlanArtifacts`, then the model and the time line. Four things a plan can ask for reach state the embedded engine does not have; each is refused at `compile()` rather than discovered half-way through a slot:

- **`DUMP` rule actions** - `dumpManager` reaches the model through the service's global `pProc` (`executorsmState.hpp`), which the embedded engine does not publish. Publishing it per engine would be a process-wide pointer again, exactly the shape phase 2 removed.
- **`SYSTEM` rule actions** - they would run `::system()` in the host process. A host callback that turns them on does not exist; "off" does.
- **`:ROTATION`** - the rotation counter is `pCounterPtr`, another service global. Without it the directive would be silently ignored and the plan would write without rotating.
- **`DEVICE` sources** - the service reads them in a DEVICE phase before `processRows`, with a timeout taken from its configuration (#347). `step()` has no such phase, and without it the source would quietly yield all-null records.

One step differs from the service on purpose: before the preflight, `compile()` removes the `.desc` and `.meta` of every declaration in the storage directory. The service refuses a kept declaration descriptor that disagrees with the plan (#265); a notebook re-runs a cell with a new source file in the same directory as a matter of course, and the plan is the only source of truth about a declaration there. The final rule for kept descriptors in embedded mode is #450's decision.

`compile()` also takes `untilEof` (default `true`): declared sources are marked `ONESHOT` before the model is built, exactly as `xretractor -u` does, and `step()` returns `nullopt` after the slot in which a source ran dry. The default differs from the service's on purpose: a notebook reads a file as a dataset, and a source that wraps to its start after EOF produces records from data that already passed. A missing source file is refused at `compile()` with an `Environment` refusal carrying `ENOENT` (#451); the service still yields NULL records for it (#346).

### 2.5 Errors

The engine follows decision (a) of #463. Every call into the core runs inside `rdb::ThrowOnFatalScope`, so K1 (`FatalError`) and K4 (`EnvironmentError`) leave the facade as `rdb::EngineFatal` instead of ending the host process; the service and `xretractor -u` still end the way they always did. Refusals (K2 and K3) leave as one exception type, `rdb::embed::Refused`, carrying the core's `rdb::Refusal` and the stage it came from:

| Stage | Source in `compile()` and the readers |
|---|---|
| `Parse` | status of `parsePlanText` (the service's `Parse result:`) |
| `Check` | status of `compiler::compile()`; `std::logic_error` from `qTree::getQuery` for an unknown stream in `FROM` (the service ends with `EINTR` there); `checkPlanStorage`, with its refusal class kept; `dataModel::initializationError`; the four refusals of §2.4; an empty plan |
| `Request` | a call without a plan, a stream outside the plan, a record index or range outside the stream |

An error inside a slot leaves the model possibly half-way through the slot, so the plan latches: `Engine::failed()` turns true, and every later `step()` raises `EngineFatal{Internal}` naming the first error, until `compile()` or `close()`. Reads still work, so a host can inspect what was computed before the error.

`close()` and the destructor are `noexcept` and run outside the throw mode: K1/K4 from a model or storage destructor still ends the process, as it does in the service. The full K1-K7 policy of the facade, module taint after K1 and `errno` on `OSError` are #476.

### 2.6 Reading results

`record(stream, index)` returns a **copy** of one record, oldest first. For a SELECT stream it reads the storage file into a private buffer; for a declaration it returns a record from the source's history ring, and refuses when the record asked for has already left it. `project(stream, flat elements, first, count)` is the dense form the Python window is built from: `double` per value, `NaN` for null, a refusal for a STRING or a pair. Both go through the model's own `storage` objects, so `VOLATILE` streams read as well as files do.

None of this pins engine memory. That is the "copy, do not pin" decision of [`jupyter-integration.md`](jupyter-integration.md) section 4, and it is what lets a tensor outlive the next `step()`.

## 3. The exceptions in Python

The hierarchy decided in #491:

```text
RetractorDBError (Exception)
+-- ConfigError (RetractorDBError, ValueError)   K3: Refused, stage Request
|   +-- PlanError                                K2: plan refused
|       +-- RQLSyntaxError                       Refused, stage Parse
|       +-- CompileError                         Refused, stage Check
+-- IOError (RetractorDBError, OSError)          K4: Refusal or EngineFatal of class Environment
+-- InternalError                                K1: EngineFatal of class Internal
+-- CorruptDescriptor, StorageError, NoSuchStream
```

`except PlanError` catches K2 only, `except ValueError` catches K2 and K3, `except OSError` catches K4, `except RetractorDBError` catches everything the module raises. `ConfigError` and `IOError` have two bases, which `nb::exception` cannot register; they are created with `PyErr_NewException` and a tuple of bases, and one exception translator maps `Refused`, `EngineFatal` and the binding's own I/O error onto them. The binding parses no message text to decide a type.

## 4. What the binding built on it (J1, J2, J3)

`retractordb.Engine` is `_core.Engine` (nanobind) plus a Python subclass. The compiled half: `compile`, `step`, `run`, `streams`, `schema`, `record_count`, `record`, `failed`, `close`, the context manager, and `_block`, which turns `project()` into a NumPy array in the requested dtype. The Python half: `rows()`, `to_numpy()`, `window()` and `Window`.

- **GIL.** `compile`, `step`, `run`, `record` and `_block` release it. `run()` re-acquires it every 50 ms to call `PyErr_CheckSignals()`, which is how the notebook's stop button turns into `KeyboardInterrupt` inside a C++ loop. `test_run_honours_keyboard_interrupt` drives it with `_thread.interrupt_main()`, which raises the same flag Ctrl-C does.
- **Logging.** The module replaces the sinks of spdlog's default logger with one that forwards to `logging.getLogger("retractordb")`. The engine itself configures nothing (that is phase 2's gate); the *host* does, and the module is the host. spdlog is header-only and linked into the module with hidden symbols, so the logger being replaced is visible to nothing but `_core`.
- **Windows and DLPack.** `window(stream, fields=, size=, stride=, dtype=)` is a NumPy array of shape `(n_windows, size, n_values)` that the `Window` object owns; `__dlpack__` and `__dlpack_device__` delegate to it, so `torch.from_dlpack(window)` shares the memory without a second copy and without the package importing torch. `retractordb.torch` is the one module that imports torch, and only when asked for.

## 5. What phase 3 still owes

**The service.** Restructuring `executorsm::run()` around `step()` - an in-process transport in place of `IpcServer`, a loop that never touches the terminal or the scheduler - was left undone: `executorsmState.hpp`'s globals are the *service's* state, and phase 2 §6 deferred them to a restructuring of the service, which is not a prerequisite of the embedded engine. The consequence is that `Engine` does not publish `pProc` and therefore refuses `DUMP` (§2.4).

**`core_mutex`.** `dataModel::processRows` takes the global `core_mutex` (defined in `dataModel.cpp`). Two engines stepping concurrently on two Python threads serialise on it. That is a throughput limit, not a correctness one, and it goes with the service restructuring above.

**The host callback for `SYSTEM`, and DEVICE sources.** Refusing both is the "off"; the "on" - a callback the host installs, a DEVICE phase in `step()` - is unwritten.

**The error policy.** The full K1-K7 table in the facade, module taint after K1 and `errno` on `OSError` belong to #476.
