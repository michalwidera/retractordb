# Core phase 2: de-globalize, and inject the log sink

**Status:** started. 2.1 (the MEMORY store) and 2.2 (the fatal-exit machinery leaves the
shared headers) are **done**. 2.3, the log sink, is not started. `executorsmState.hpp` is
deliberately out of scope - see §4.

**Prerequisite for:** stage 1b (`Engine` binding the full engine) and everything above it -
see [`embedded-roadmap.md`](embedded-roadmap.md), where phase 2 is "de-globalize and inject a
log sink".

Phase 1 ([`core-phase-1.md`](core-phase-1.md)) made engine failures survivable in a host
process. Phase 2 answers the next question that a host process asks and a daemon never does:
**what happens when you build the engine twice?** A daemon constructs its state once, so
process-wide state and engine-wide state are indistinguishable. A notebook constructs it on
every cell run, and the difference becomes the bug.

`api/python/tests/test_reentry.py` is the acceptance test and was written during stage 1a,
before there was anything to fix. It asserts today's behaviour so the phase-2 work has a
baseline it must not regress.

## 1. The inventory

A sweep for file-scope mutable state across `src/` finds remarkably little, and what it finds
divides cleanly by *whose* state it is:

| State | Where | Whose |
|---|---|---|
| MEMORY records, NULL maps, write counter | `faccmemory.cc` 3 statics | the engine's - **fixed, §2** |
| `fatalErrorRaised` | `fatalError.hpp` | the server process's - **relocated, §3** |
| `statusDesc` | `DESCParser.cc` | gone - deleted by phase 1 slice 1 |
| spdlog default logger | `uxSysTermTools.cpp` | the process's - **§4** |
| executor state (`pProc`, `core_mutex`, `esm::*`) | `executorsmState.hpp` | the server's - **§4** |
| `castFldVT` | `expressionEvaluator.cpp` | stateless functor, not state |

That list is the reason phase 2 is small. The engine does not, in general, keep process
state; it kept three maps and one flag.

## 2. The MEMORY store (2.1)

The three `static` maps in `faccmemory.cc` were the storage medium of the MEMORY substrate,
indexed by stream name. They are now `rdb::MemoryStore` (`memoryStore.hpp`), an object the
caller can own.

**The sharing is not the bug.** `faccmemory.hpp` documented it as a contract, and it is one:
within a single plan the writer and the reader of a MEMORY stream are *different* `memoryFile`
objects, and if each kept its own vectors the reader would see nothing. The bug was the
*scope*: the owner was the whole process, so two engines built in one process shared any
stream whose name collided, silently.

The change is therefore not a deletion but an ownership decision, and the one taken is
**explicit store, default shared**:

- `memoryFile` takes a `MemoryStore &`, defaulting to `MemoryStore::processDefault()`.
- `makeAccessor` grew a trailing `MemoryStore *` (null selects the default). It has exactly
  one call site, `storage.cc:101`, which is why this is a small change.
- Every existing caller, the server included, therefore behaves exactly as before.

The class documentation was corrected with it. It had said the records live "w globalnej
strukturze … współdzielonej między instancjami o tej samej nazwie" - the sharing-by-name half
was a promise worth keeping, the "global" half was the defect written down as a feature.

Three tests carry the distinction, because nothing else would catch it: two stores with the
same stream name stay independent; one store still shares between instances; no store at all
still means the process default, which is what `test_faccmemory_persistence_across_instances`
has always relied on.

**Not done yet:** `rdb::storage` and the Python binding cannot pass a store, so
`test_reentry.py::test_two_storages_open_at_once` still passes by luck rather than by
isolation. Giving them one needs an object that means "one engine", which stage 1a does not
have - see §4.

## 3. The fatal-exit machinery leaves the shared headers (2.2)

`fatalError.hpp` defines `inline std::atomic<bool> fatalErrorRaised` - state of the whole
process - and it sat in `src/include`, the directory the storage layer takes its headers
from. The layer a notebook loads could reach it.

After phase 1 nothing in `src/rdb` uses `FatalError` at all, so nothing there needed it; what
remained was only the *possibility* that something would reach for it. The header moved to
`src/retractor/lib/`, next to its two remaining users:

- `executorsm.cpp` - `cleanup()` reads the latch, `run()`'s `rdb::Error` catch sets it;
- `launcher.cpp` - sets it on two startup failure paths;
- `dataModel.cpp` - the `RDB_FAULT_FATAL_IN_SLOT` hook, the last `FatalError` call in the tree.

Two includes in `qry.cpp` and `uxSysTermTools.cpp` turned out to be stale - zero uses of
either the macro or the flag - and went with it. Headers are not installed
(`CMakeLists.txt` installs binaries, the licence and the TOML only), so packaging is
unaffected.

This does not de-globalize the flag. Its correct scope genuinely *is* "the xretractor process's
exit path", because that is what it describes. What changes is that the shared include
directory no longer advertises it.

## 4. What phase 2 still owes

**2.3, the log sink.** spdlog's default logger is process-global, and a host process has its
own logging. The engine should take a sink rather than reach for the global registry. Not
started.

**`executorsmState.hpp`.** Roughly twenty globals - `pProc`, `core_mutex`,
`plan_epoch_mutex`, the `esm::` group. They are genuinely the server's, and no notebook links
them today. They also stop being globals naturally when phase 3 restructures the run loop
around `step()`, so converting them now would be work done twice.

**The object that means "one engine".** 2.1 stops at the injection point because stage 1a has
no such object: it binds loose `Storage` handles, not a session. Creating one is what lets the
binding hand each engine its own MEMORY store, and it is the same object phase 3 needs for
`step()` and phase 2.3 for the sink. It is the natural next decision, not a loose end.
