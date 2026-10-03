# Embedded and real-time gaps

**Status (2026-10):** the error-handling gap (section 2) is **closed** on
`feature/jupyter-integration`: the engine core reports errors as values and builds with
`-fno-exceptions`. Every other gap (section 3) is **documented, not fixed** - each one names
the code, the guidance it misses and a proposed fix, ordered by priority.

This page reviews the engine against four books on embedded, low-latency and concurrent C++.
It answers three questions: what those books ask of code that runs on a real-time tick, where
RetractorDB falls short, and what this change did about it.

| Short name | Book |
|---|---|
| **BM** | Israel Gbati, *Bare-Metal Embedded C Programming*, Packt 2024 |
| **LL** | Sourav Ghosh, *Building Low Latency Applications with C++*, Packt 2023 |
| **MM** | Patrice Roy, *C++ Memory Management*, Packt 2025 |
| **AP** | Javier Reguera-Salgado, Juan Antonio Rufes, *Asynchronous Programming with C++*, Packt 2024 |

Page numbers are **PDF pages** (printed page = PDF page - 21 for LL and AP, - 23 for BM and
MM). A claim marked *inference* is a consequence drawn here, not a statement in the book.

## 1. Summary

| # | Gap | Books | Priority | State |
|---|---|---|---|---|
| E | C++ exceptions as the error channel of the engine core | LL 1.1-1.3, MM 1.1-1.10, BM 1.1, 1.7, AP 1.1-1.4 | P0 | **fixed here** |
| 1 | Heap allocation and name lookups on every tick | LL 2.1, 2.3, 2.6; MM 2.1, 2.4, 2.8 | P1 | report |
| 2 | `std::regex` built on every evaluation of `name[index]` | LL 5.2 | P1 | report |
| 3 | Synchronous logging on the tick thread | LL 3.1; AP 5.3; BM 4.4 | P1 | report |
| 4 | `RULE ... DO SYSTEM` runs `::system()` inside the slot | LL 3.4; BM 4.4 | P1 | report |
| 5 | Two `getenv()` calls per slot for test hooks, in every build | LL 1.2, 5.2 | P1 | report |
| 6 | No out-of-memory policy; `bad_alloc` crosses no-exception frames | MM 1.3, 1.12; LL 2.2 | P1 | report |
| 7 | Throwing standard-library calls still throw under `-fno-exceptions` | AP 1.7; MM 1.1 | P1 | partly fixed |
| 8 | An invariant violation reachable from Python ends the interpreter | MM 1.3; BM 1.1 | P2 | report |
| 9 | Process-global state in the core | AP 2.13; MM 3.8, 3.9 | P2 | report |
| 10 | Silent I/O failures in the meta index | LL 1.4; BM 1.6, 1.7 | P2 | partly fixed |
| 11 | No watchdog or heartbeat for the service | BM 1.3-1.5 | P2 | report |
| 12 | Unbounded waits | BM 5.2; AP 2.4, 4.3 | P2 | report |
| 13 | Concurrency tested on x86 only; ARM orders memory weakly | AP 2.8-2.10; MM 6.2 | P2 | report |
| 14 | A retention store with a storage directory looks for segments in the working directory | BM 5.5 | P2 | report |
| 15 | Stack trace with `addr2line` on the error path | LL 1.2; BM 5.2 | P3 | report |
| 16 | The non-real-time loop drifts | AP 4.1-4.2; BM 2.5 | P3 | report |
| 17 | spdlog and fmt are compiled into both exception modes | MM 1.1; AP 1.7 | P3 | report |

P0 blocks the target on its own. P1 costs determinism on every tick. P2 is a correctness or
robustness gap off the tick path. P3 is hygiene.

## 2. Error handling: exceptions replaced by values (fixed)

### 2.1 What the books ask for

- **Exceptions cost something even when nothing throws, and remove them deliberately.** LL
  lists exceptions with dynamic polymorphism and dynamic allocation as features to avoid on the
  critical path (LL p. 80, 95). Low-latency code turns them off per function or for the whole
  program; error handling then becomes "completely the developer's responsibility" (LL p. 95).
- **Report errors as values.** LL's code base has no `try`/`catch`. Errors are `bool`,
  `nullptr`, sentinels and small `enum class` results tested with `LIKELY(r == ALLOWED)`
  (LL p. 121-122, 135, 150-152, 362-371).
- **Some domains cannot use exceptions.** "There are application domains where exceptions are
  not an option" (MM p. 172). Without them, choose one explicit policy for what cannot be
  recovered, and `std::abort()` "is a reasonable choice" (MM p. 215-216).
- **Constructors that can fail become factories returning `expected`** (MM p. 90, 214,
  inference from the book's note on `std::expected`).
- **Fail-stop to a defined handler, and record why** (BM p. 134-138). BM's whole error model is
  status flags, fail-stop handlers and a watchdog; it offers no precedent for exception-based
  recovery (BM section 6).
- **Mark results `[[nodiscard]]` and never leave an error branch empty** (BM p. 348-353;
  LL p. 268, 482-483).
- **Errors cross threads as data** (AP p. 62, 82-84). Boost.Asio's `error_code` overloads are
  the model for an API that does not throw (AP p. 262, 267-269).

### 2.2 What the engine did before

Core phase 1 replaced `FatalError` (`std::exit`) with a hierarchy of exceptions
(`rdb::Error`, `ConfigError`, `CorruptDescriptor`, `LogicError`, `IOError`, plus
`embed::SyntaxError` and `embed::CompileError`). That made bad input survivable for a notebook,
but it put `throw` on the tick path: close to 300 `throw` statements in the sources that are now
the core (`src/rdb/lib`, the `retractorcore` part of `src/retractor/lib`, `src/embed`), a
function-try-block in the compiler, `try`/`catch` in `payload::setItem`
and in the expression folder, and `std::runtime_error` from the evaluator.

### 2.3 The design now

Three channels, defined in `src/include/rdb/error.hpp`:

| Channel | When | Mechanism |
|---|---|---|
| **Value** | anything that can fail in a correct engine: bad plan, bad descriptor, bad API argument, file system, an expression that cannot be computed | `rdb::Result<T>` = `std::expected<T, rdb::Error>`; `rdb::Error` = category (`rdb::Errc`) + message; `[[nodiscard]]`; `RDB_TRY` / `RDB_TRY_ASSIGN` to propagate |
| **Fatal** | a broken invariant that the boundary already excludes | `RDB_ASSERT(cond, fmt, ...)` and `rdb::fatal(msg)`: log, then the installed handler, which never returns. Default: `"FATAL: ..."` on stderr and `std::abort()`. The daemon installs `daemonFatalExit` (latch, flush, `std::exit(EXIT_FAILURE)`), so `atexit` clean-up still runs. |
| **Host boundary** | Python | `src/python/module.cpp` turns `rdb::Error` into a Python exception of the same class as before. This is the only place where an engine error becomes an exception. |

The rule between the first two: anything reachable through the public API or through the
text of a plan is validated **at the boundary** and returns a `Result`. `RDB_ASSERT` covers only
what the boundary has already ruled out. The message of an assert is built only when the
condition fails, so the success path costs one branch (LL p. 120 builds the message eagerly;
LL p. 464-473 shows what that costs).

`rdb::Errc` keeps the categories of the old hierarchy, so the Python classes did not change:

| `rdb::Errc` | Python | Old C++ type |
|---|---|---|
| `Config` | `ConfigError` | `rdb::ConfigError` |
| `CorruptDescriptor` | `CorruptDescriptor` | `rdb::CorruptDescriptor` |
| `Syntax` | `RQLSyntaxError` (a `ConfigError`) | `rdb::embed::SyntaxError` |
| `Compile` | `CompileError` (a `ConfigError`) | `rdb::embed::CompileError` |
| `IO` | `IOError` | `rdb::IOError` |
| `Logic` | `InternalError` | `rdb::LogicError` |
| `Eval` | `InternalError` | `std::runtime_error` from the evaluator, which reached Python as a bare `RuntimeError` |

### 2.4 The build enforces it

`option(RDB_NO_EXCEPTIONS ... ON)` in the root `CMakeLists.txt`. With it on:

- `rdb_no_exceptions(<target>)` compiles `rdb`, `retractorcore` and `rdbembed` with
  `-fno-exceptions` (and `SPDLOG_NO_EXCEPTIONS`). A `throw`, `try` or `catch` in the core is a
  **compile error**, not a review comment.
- `rdb_exception_island(<file>)` re-enables exceptions for exactly three files that wrap
  third-party code which reports errors only by throwing: `DESCParser.cc` and `RQLParser.cpp`
  (ANTLR) and `stackTrace.cc` (Boost.Stacktrace, whose default backend has `throw` and `try` in
  its headers). Each catches everything at its boundary and returns a status or a string.
- `boost::throw_exception` is defined under `BOOST_NO_EXCEPTIONS` and routes to `rdb::fatal`
  (`src/rdb/lib/error.cc`). The one Boost call that needed a `try` internally,
  `boost::rational`'s `operator>>`, was replaced with a parser of its own in `payload.cc`.
- `src/retractor/lib` is split. **`retractorcore`** (an explicit source list: compiler, plan,
  model, evaluator, `streamInstance`, `dumpManager`, `RQLParser`, ...) is the no-exception core
  that `rdbembed` and the Python module link. **`retractor`** is the daemon service (IPC,
  communication thread, configuration, presenter, launcher support) and keeps exceptions,
  because Boost.Interprocess, `property_tree` and `program_options` report errors only by
  throwing. Globals that core code reads moved to `coreState.cpp`, so `rdbembed` no longer
  links the daemon's IPC server.

`RDB_NO_EXCEPTIONS=OFF` builds the same code with exceptions; nothing in it depends on the flag.

### 2.5 What changed, layer by layer

- **Storage.** `storage::create`, `StoragePaths::make` and `PersistentCounter::create` are
  factories returning `Result<std::unique_ptr<...>>`; their constructors are private. Each
  factory checks its input before it builds anything (MM p. 299: "do not modify your object
  until you know you can do so safely"). `attachDescriptor`, `write`, `read`, `revRead`,
  `purge`, `loadDescriptorFile`, `saveDescriptorFile`, `verifyDescriptorMatch`, `makeAccessor`
  and `FileInterface::count` return `Result`s. `attachDescriptor` used to return a bare string;
  it now returns `CorruptDescriptor` or `IO` errors, so the Python binding no longer reads the
  `.desc` file a second time to tell the two apart. `IndexRecord::deserialize` returns
  `std::optional`. A short entry in a `.meta` file is no longer read past the end of its buffer.
- **Accessors** keep their `int` status contract and gained the cases that used to throw:
  `EILSEQ` from the text source with `inputError()`, `EINVAL` for a zero record size, and `EIO`
  plus a log line for a group-file segment that cannot be opened.
- **Compiler.** `compile()` returns `"OK"` or a status. A broken compiler invariant returns a
  status with the prefix `kInternalCompilerError` ("internal compiler error: "), and callers
  tell it apart with `isInternalCompilerError()`. A new pass, `checkStreamReferences()`,
  rejects `FROM <undefined stream>` and an AGSE step of 0 or less as plan errors. Until now
  the first one ended compilation with a `std::logic_error` from `qTree::getQuery`, and the
  second one reached `query::descriptorFrom`.
- **Model and evaluator.** `dataModel::create`, `processZeroStep`, `processRows`, `getPayload`,
  `getRow`, `fetchForward`, `streamInstance::create` and the per-slot helpers return `Result`s.
  `expressionEvaluator::eval` returns `Result<descFldVT>` with `Errc::Eval`. Constant folding no
  longer needs `try`: an evaluation error means "do not fold".
- **Engine** (`rdb::embed::Engine`). Every operation that can fail returns `Result`. The
  exception classes `embed::SyntaxError` and `embed::CompileError` are gone. A plan that holds
  only directives is refused at `compile()` instead of tripping the `TimeLine` invariant. New
  behaviour: **an error inside a slot stops the plan.** `step()` returns the error, `failed()`
  becomes true, and every later `step()` returns `Errc::Logic` naming the first error. Reads
  still work. `compile()` starts over. Before this change the next `step()` would have computed
  on a model whose slot never finished. `close()` is `noexcept` with no `try`.
- **Daemon.** `executorsm::run()` carries an engine error out of the slot and epoch loops with
  `break` and reports it after them: latch, `FATAL: <message>` on stderr, a critical log line
  and `EXIT_FAILURE` - the same observable result that `catch (const rdb::Error &)` produced.
  The remaining `catch` clauses handle only Boost.Interprocess and `std::exception` from
  third-party code. A broken invariant handled in a command now becomes an
  `engineErrorResponse` (`error.response = "engine error: ..."`), and the service keeps
  running, as before. `xretractor` installs `daemonFatalExit` as its first statement. A
  rotation counter that cannot be read now fails the start cleanly; its constructor used to
  throw outside any `try`, so `std::terminate` ended the process.
- **Python.** `module.cpp` defines its own exception types, registered under the same Python
  names and hierarchy, and an `unwrap()` that raises them. `Engine.failed` is new. Two
  behaviours change for Python code: an evaluator error is now `InternalError` (it was
  `RuntimeError`, outside `RetractorDBError`), and see gap 8.

### 2.6 Residual risks of the design

- **Gap 8:** a broken invariant ends the process. Before this change it was a catchable
  `LogicError`.
- **Gap 6:** allocation failure still throws `std::bad_alloc`.
- **Gap 7:** a few standard-library calls throw from inside `libstdc++`.
- **Gap 17:** spdlog and fmt headers are compiled in both exception modes.

## 3. Remaining gaps (report only)

Each entry gives the finding, the books' guidance and a proposed fix. Locations are
`file:line` in this tree.

### Gap 1 - Heap allocation and name lookups on every tick (P1)

**Finding.** The tick path allocates and looks streams up by name.
`streamInstance::constructOutputPayload` builds a new `expressionEvaluator` for every field of
every stream in every slot (`streamInstance.cpp:570`). The rule path and the window reducer do
the same (`streamInstance.cpp:297, 647`). The evaluator's own comment puts it at 52.8% of the
allocations in `processRows` (`expressionEvaluator.cpp:816`); the inline-16 `small_vector`
stack removed part of that. Cross-stream reads go through `dataModel::getPayload(name)`, which
is a `std::map<std::string, ...>::find` (`dataModel.cpp:726`). Math functions take a
`std::function<double(double)>` per call (`expressionEvaluator.cpp:811`).

**Books.** No allocation on the critical path; pre-allocate before the critical path starts
(LL p. 59, 98, 124-131: "the memory pool should be created before the execution of the
critical path starts"). `std::function` "can perform virtual function calls and invoke dynamic
memory allocations under the hood" (LL p. 90). Use directly indexed storage, not a hash or tree
lookup per row (LL p. 77-78, 224). Use per-tick arenas or a `monotonic_buffer_resource` with
`null_memory_resource()` upstream, so overflow does not fall back to the heap (MM p. 235-246,
385-391). Prove zero allocations with a test-only counting `operator new` (MM p. 186-208).

**Fix.** Resolve every cross-stream reference to a handle when the model is built, the same way
`handles_` already serves `processRows`. Keep one evaluator per field, or make `eval` a free
function over a per-slot `pmr` stack. Replace `std::function` with a function pointer or an
enum switch. Add a unit test that counts allocations across `processRows` on a representative
plan and fails above zero. The evaluator's comment cites an allocation measurement
(`run_alloc.sh`) that is not in the tracked tree; the test would make it permanent.

### Gap 2 - `std::regex` per evaluation (P1)

**Finding.** `PUSH_ID2` builds `std::regex(R"((\w*)\[(\d*)\])")` and runs `regex_search` on
every evaluation (`expressionEvaluator.cpp:1042`). Building a regex compiles an automaton
(allocation plus work proportional to the pattern), and this happens for every field in every
slot.

**Books.** Move work to compile time or startup: "we minimize the work done during runtime on
the critical code path by moving a lot of the processing to the compilation step" (LL p. 32,
74, 103).

**Fix.** Split `name[index]` once in the compiler and store the integer offset in the token,
as `PUSH_IDX` already does. Until then, a function-local `static const std::regex` removes the
build but not the matching.

### Gap 3 - Synchronous logging on the tick thread (P1)

**Finding.** All sinks are synchronous: `stderr_sink_mt` and `basic_file_sink_mt`
(`uxSysTermTools.cpp:126, 184, 189`), and `fatalError.hpp` states that no asynchronous logger
exists. A `SPDLOG_ERROR` in a slot formats the message and writes it to disk under the sink
mutex on the RT thread. On the Raspberry Pi target that disk is an SD card.

**Books.** Format and write on another thread: "disk I/O is extremely slow and unpredictable,
and string operations and formatting themselves are slow" (LL p. 138-147, 464-471). spdlog
"lacks some advanced features for extreme low-latency needs" (AP p. 327-334). No blocking I/O
or logging on the tick thread (BM p. 329-330, inference).

**Fix.** Use `spdlog::async_logger` with `async_overflow_policy::overrun_oldest` for the daemon,
and count dropped records. Compile per-tick `DEBUG` and `TRACE` out with `SPDLOG_ACTIVE_LEVEL` in
`--realtime` builds. Keep the synchronous flush on the fatal path, which needs it.

### Gap 4 - `::system()` inside the slot (P1)

**Finding.** `RULE ... DO SYSTEM '<cmd>'` calls `::system()` from
`streamInstance::constructRulesAndUpdate` (`streamInstance.cpp:654`). That is `fork`, `exec`
and `waitpid` on the RT thread, while it holds `core_mutex` and `plan_epoch_mutex`. The embedded
engine refuses the action at `compile()`, but the daemon runs it.

**Books.** Avoid blocking system calls in critical loops; the target is "low kernel usage
(system calls)" (LL p. 72, 150-151, 399-400). Keep handlers minimal: confirm, signal, return
(BM p. 322, 330).

**Fix.** Put a request (rule name, command, slot) into a bounded SPSC queue, and let a worker
thread run the command and log its status (LL p. 132-137; AP p. 159-164). Gate it behind an
explicit opt-in. The roadmap already calls for the host-callback gate, disabled by default.

### Gap 5 - `getenv()` per slot for test hooks (P1)

**Finding.** `dataModel::processRows` calls `std::getenv("RDB_FAULT_FATAL_IN_SLOT")` and
`std::getenv("RDB_FAULT_ERROR_IN_SLOT")` on every slot, in every build (`dataModel.cpp:310,
321`). Each call scans the environment, and `getenv` is not safe against a concurrent `setenv`.

**Books.** Assert-style checks on the critical path cost something, so compile them out where it
is safe (LL p. 120, 464-465). Decide at startup (LL p. 32).

**Fix.** Read both variables once, when the model is built, into two fields. Better still,
compile the hooks only under a test option, as `RDB_BENCH_PROBE` does for the probes.

### Gap 6 - Out-of-memory policy (P1)

**Finding.** Code compiled with `-fno-exceptions` still calls the global `operator new`, which
throws `std::bad_alloc`. The exception unwinds through core frames that have no clean-up code:
destructors do not run and locks are not released. Nothing catches it in the daemon's slot
loop, so the process terminates. On Linux with overcommit, allocation rarely fails at all; the
failure comes later, as the OOM killer (MM p. 352).

**Books.** Without exceptions, pick one explicit policy for allocation failure: a single
allocation entry point, and `std::abort()` is reasonable (MM p. 215-219). `new (std::nothrow)`
does not make construction non-throwing (MM p. 174-176). Use `constexpr` capacities and an
explicit decision on overflow (LL p. 213, 474).

**Fix.** Call `std::set_new_handler` at start to route to `rdb::fatal("out of memory")`, so the
failure is reported and the process stops in a known state. For `--realtime`, size the
per-plan buffers when the plan is compiled and touch them before the first slot;
`rtActivate` already calls `mlockall`. Combined with gap 1, this lets the tick path rule out
allocation failure by construction.

### Gap 7 - Standard-library calls that throw from inside `libstdc++` (P1, partly fixed)

**Finding.** `-fno-exceptions` changes how *our* translation units are compiled; it does not
change `libstdc++.so`. `std::vector::at`, `std::stoi`, the `std::filesystem` overloads without
`std::error_code`, `std::any_cast` on a reference, `std::get` on the wrong alternative and
`std::regex` construction still throw from inside the library. The exception then unwinds
through core frames with no clean-up (as in gap 6).

**Done here.** The core uses the `error_code` overloads of `std::filesystem` everywhere
(`storagePaths`, `storage::descriptorFileExist`, `fagrp`, `faccposix*`, `planSource`,
`metaIndexStore`). `std::any_cast` uses the pointer form. `boost::throw_exception` routes to
`rdb::fatal`. The `.at()` calls left in `compiler.cpp` and `dataModel.cpp` index containers
whose size the same function has just established.

**Books.** Standard primitives throw too; audit the core for them, because under
`-fno-exceptions` they abort (AP p. 96-97, 127-129).

**Fix.** Add a check to `test/embedding_boundary.py` that rejects the throwing overloads in
`src/rdb/lib` and in the `retractorcore` sources (`filesystem::` calls without `error_code`,
`.at(`, `stoi`, `stol`, `any_cast<T&>`). The pattern list is short and has no false positives
in today's tree.

### Gap 8 - A broken invariant reachable from Python ends the interpreter (P2)

**Finding.** Before this change a broken engine invariant was a `LogicError` that the binding
turned into `InternalError`. It is now an `RDB_ASSERT` that calls `rdb::fatal`, and the default
handler calls `std::abort()`. A notebook kernel that reaches one dies. Every path known to reach
an invariant from Python is guarded:

- `field_index`, `byte_offset` and `field_type_name` check `hasField` first (`KeyError`);
- record and descriptor indexing is range-checked (`IndexError`);
- `Engine` methods check the stream name with `requireStream` (`KeyError`);
- `test_fatal_paths.py` and `test_engine.py` pin these guards.

But a bug of the same class that has not been found yet now aborts instead of raising.

**Books.** This is the trade-off MM describes: without exceptions, "most standard mechanisms
... lead to program termination", and `std::abort()` is a reasonable choice (MM p. 216). It is
reasonable for a service under a supervisor (BM p. 365-367). It is harsh for an interactive
host.

**Fix (options, not decided).**

1. Turn more invariants into `Errc::Logic` results where the caller can return one. `Engine`
   already treats `Logic` from a slot as "plan stopped".
2. Install a Python-specific fatal handler that writes the message and a hint to `sys.stderr`
   before `abort()`, so the kernel's death has a cause in the notebook.
3. Fuzz the Python API (Hypothesis over `compile` text and indices) to find unguarded paths.

Option 3 should come first.

### Gap 9 - Process-global state in the core (P2)

**Finding.** `pCounterPtr`, `pProc` and `esm::plan_epoch_mutex` are process globals
(`coreState.cpp`). `dumpManager` reaches the model through `pProc`, and `:ROTATION` reads the
counter through `pCounterPtr`. That is why the embedded engine refuses `DUMP` and `:ROTATION` at
`compile()`. `MemoryStore::processDefault()` survives as the default for code built without an
`Engine`.

**Books.** Keep data thread-local and minimize shared state (LL p. 99; AP p. 54, 84-85, 400).
The destruction order of statics across translation units is unspecified (MM p. 94-99), and a
function-local static pays for synchronization on every access (MM p. 193, 244-246).

**Fix.** Move the counter and the model pointer into `Engine::Plan` and into an
`executorsm` epoch object, and pass `dumpManager` its model explicitly. The `embedding_boundary`
gate can then forbid new globals in `retractorcore`, as it already does for `src/rdb`.

### Gap 10 - Silent I/O failures in the meta index (P2, partly fixed)

**Finding.** `MetaIndexStore::rewrite` returns silently when the temporary file cannot be
opened, and `readAll` stops at a corrupt entry with only a log line (`metaIndexStore.cc`).
Callers cannot tell that the index on disk is stale.

**Done here.** A failed `rename` of the temporary file is now logged and invalidates the cache,
instead of throwing `filesystem_error` out of the middle of a write.

**Books.** "Silently ignoring errors like these is not ideal since the clients are not notified
about these errors" (LL p. 268). Every storage error must become a status that reaches the
caller (BM p. 227-231, 385, 409).

**Fix.** Return `Result<>` from `rewrite` and `readAll`, and let `metaData` map a failure to
`Errc::IO` at `storage::attachDescriptor` and `storage::write`.

### Gap 11 - No watchdog or heartbeat (P2)

**Finding.** No `sd_notify(WATCHDOG=1)`, no `WatchdogSec=` in the unit, and no shared progress
counter. A slot loop that stops making progress but holds its lock (a blocked `::system()`, a
stuck SD-card write) is seen by nobody. `Restart=on-failure` covers only a process that exits.

**Books.** Use a watchdog as the last line of recovery. Feed it only from code that shows the
system works, and test the recovery (BM p. 365-367). A windowed watchdog also catches a loop
that runs too *fast* (BM p. 368-369). Find out at boot why the system restarted (BM p. 376-377).

**Fix.** Add `WatchdogSec=` to the unit, and send `WATCHDOG=1` from the slot loop after a
completed slot (not from the communication thread). Log the previous exit reason at start; the
fatal latch and the cleared query file already record most of it.

### Gap 12 - Unbounded waits (P2)

**Finding.**

- `executorsm::run` waits for `ipcReady || ipcFailed` with no deadline (`executorsm.cpp:344`).
  If the communication thread hangs before it reports either, start-up hangs with it.
- `commandProcessor` waits on `cv` for a model (`executorsmCommands.cpp:114`); `stop_now` bounds
  it, but no deadline does.

The query queue itself is already polled with `try_receive` (`ipcServer.cpp:318`).

**Books.** Every wait in the bare-metal book is unbounded, so one stuck device hangs the system
(BM p. 234-373, inference). Lock acquisition and future waits should carry timeouts (AP p. 54,
174-177, 367-368).

**Fix.** Use `wait_for` with a start-up deadline that ends the process with a clear message,
and give the command handler a deadline after which it answers "busy" instead of waiting.

### Gap 13 - Concurrency verified on x86 only (P2)

**Finding.** The slot thread and the communication thread share state through atomics and two
mutexes. Ten atomic operations in `src/retractor/lib` use `memory_order_relaxed` (bus, executor,
IPC responses, plan revision). CI and this review ran on x86, whose total store order hides a
missing acquire/release pair.

**Books.** "The ARM architecture supports Weak Ordering" (AP p. 130, 137-139, 159). Relaxed
ordering is fine for counters, never for publication (AP p. 139-144, 151). Check
`is_always_lock_free` (AP p. 146, 155-156).

**Fix.** Run the unit and integration suites under `RDB_SANITIZE=thread` in CI (the option
exists in the root `CMakeLists.txt`), and run the IPC stress tests on the Raspberry Pi 400.
Add `static_assert(std::atomic<T>::is_always_lock_free)` on the latch and the revision
counters.

### Gap 14 - Segments looked up in the working directory (P2)

**Finding.** `groupFile` finds existing retention segments by listing
`std::filesystem::current_path()` and matching file names against `filename_`
(`fagrp.cc:67`). `filename_` carries the storage directory when the plan has `:STORAGE` (or the
daemon has `[storage] dir`), but the listing yields bare names. So the prefix never matches,
and a restart behaves as if no segment existed (*inference from the code*; no test covers it).

**Books.** Never truncate or recreate existing data without checking it first (BM p. 302,
351-353).

**Fix.** List `std::filesystem::path(filename_).parent_path()`, compare bare names, and add a
unit test with a storage directory.

### Gap 15 - Stack trace on the error path (P3)

**Finding.** A flat index out of range in `payload` (context "Read") prints a
`boost::stacktrace` before `rdb::fatal`. With `addr2line` that can take "more than 60 s"
(`payload.cc`, through `currentStackTrace()` in `stackTrace.cc`). Under a watchdog, or with a
supervisor that kills slow exits, the diagnostic outlives its budget.

**Books.** Fatal checks log and exit (LL p. 120). A watchdog bounds the time to recovery
(BM p. 365-367).

**Fix.** Capture the raw frame addresses (cheap) and symbolize them offline, or only when an
environment switch asks for it.

### Gap 16 - The non-real-time loop drifts (P3)

**Finding.** Without `--realtime` the slot loop sleeps for a relative period,
`std::this_thread::sleep_for(period)` (`executorsm.cpp:608`). Each slot then takes the period
plus the processing time plus the wake-up latency. `--realtime` already sleeps to an absolute
deadline (`rtAbsoluteSleep`).

**Books.** `sleep_for` "sleeps for at least a given duration" (AP p. 66-67). Schedule absolute
deadlines (AP section 4.2, inference). Never spin-delay; use `clock_nanosleep(TIMER_ABSTIME)`
(BM p. 98, 141, 190, 201).

**Fix.** Use the same absolute-deadline sleep in both modes. `--no-clock` stays as it is.

### Gap 17 - spdlog and fmt in two exception modes (P3)

**Finding.** spdlog and fmt are header-only here. The core includes them with
`SPDLOG_NO_EXCEPTIONS` under `-fno-exceptions`; the daemon and the Python module include them
with exceptions. Inline functions such as `logger::log_` therefore exist in two variants with
one name, and the linker keeps one (an ODR violation in the letter of the standard). The
practical risk is limited: both variants log the same way, and they differ only in what
happens when a sink or a format string throws. But it is unspecified which variant runs. In
the core, a sink error ends the process (`SPDLOG_NO_EXCEPTIONS` aborts).

**Books.** Third-party code may still throw; RAII-only clean-up keeps the core safe either way
(MM p. 331, 339).

**Fix.** Build spdlog as a compiled library (`SPDLOG_COMPILED_LIB`) with a sink that never
throws, so that the mode is fixed in one place. The async logger from gap 3 is the natural
place to do it.

## 4. How to check this

- `RDB_NO_EXCEPTIONS=ON` (the default) is the check: a `throw` in the core fails the build.
- `ut_*` cover the `Result` paths. `test/UnitTest/rdbResult.hpp` provides
  `EXPECT_RDB_ERROR(expr, Errc, fragment)` and `rdbtest::ok(...)`. Invariants are covered by
  death tests (`*DeathTest`) that match `FATAL: ...`.
- `it_fatal_exit_path` covers both exits: `RDB_FAULT_FATAL_IN_SLOT` (`rdb::fatal` with the daemon
  handler) and `RDB_FAULT_ERROR_IN_SLOT` (a `Result` error out of `processRows`, reported by
  `executorsm::run`). `ut_embedEngine` checks that a slot error stops the plan until the next
  `compile()`.
- `api/python/tests` checks the Python names and hierarchy.
