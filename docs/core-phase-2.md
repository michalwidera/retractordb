# Core phase 2: de-globalize, and inject the log sink

**Status:** started. 2.1 (the MEMORY store), 2.2 (the fatal-exit machinery leaves the shared
headers), 2.3 (the descriptor format flag, plus the gate that enforces all of it) and 2.4/2.5
(the owner: `rdb::storage` takes a store, and L2 exists to hold one) are **done**. What
remains is the binding and the probe counters - see §4. `executorsmState.hpp` is deliberately
out of scope.

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
| `Descriptor::singleLineOutput_` | `descriptor.hpp` class static | the engine's - **fixed, §3a** |
| spdlog default logger | `uxSysTermTools.cpp` | the binaries' - **not an issue, §3b** |
| probe counters | `probe.hpp` 3 inline vars | the engine's - **known debt, §4** |
| executor state (`pProc`, `core_mutex`, `esm::*`) | `executorsmState.hpp` | the server's - **§4** |
| `castFldVT` | `expressionEvaluator.cpp` | stateless functor, not state |

That list is the reason phase 2 is small. The engine does not, in general, keep process
state; it kept three maps, two flags and three counters.

**The list is not the product of one sweep, and that matters.** The first pass looked for
namespace-scope statics and found only the MEMORY maps. `Descriptor::singleLineOutput_` is a
*class* static, so that sweep never saw it; the probe counters are `inline` variables in a
header, so a grep for `static` missed them too. All three turned up only when the sweep was
written as an executable gate (§3c) rather than run once by hand. An inventory that is not
enforced is a snapshot of one afternoon's greps.

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

**Since then:** `rdb::storage` takes a store (2.4) and `rdb::embed::Engine` owns one (2.5) -
see §3d. The Python binding still does not expose either, so
`test_reentry.py::test_two_storages_open_at_once` continues to pass by luck rather than by
isolation; that is now a wiring job with no design question left in it (§4).

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

## 3a. The descriptor format flag (2.3)

`Descriptor::singleLineOutput_` was a static class member: a process-wide flag, in the layer a
notebook loads, and in the worst shape such a flag comes in. It was raised by the
`rdb::singleLineFormat` manipulator and **cleared by `operator<<` itself** after one use, so two
threads printing descriptors raced over the format of a third's output. `module.cpp` already
carried a save-and-restore workaround around every `repr()`, with a comment naming phase 2 as
the fix.

It now lives in a `std::ios_base::xalloc()` slot of the stream being written to. Every call
site is unchanged - `os << rdb::singleLineFormat << desc` reads the same, and the one-shot
semantics are the same - but "single line" now means *this output* rather than *this process*.
The binding's workaround is gone; a local `ostringstream` cannot affect anyone.

A word on how nearly this went wrong: `grep singleLineOutput` finds no callers, because the
manipulator that sets it is named `singleLineFormat` and has about 35 call sites across
`xtrdb`, the tests and the binding. Read as dead code, it would have been deleted.

## 3b. The log sink: already where it belongs (2.3)

The roadmap pairs de-globalization with "inject a log sink", and the expectation was a slice.
The sweep says otherwise: **no logger configuration exists in the library at all.** Every
`set_default_logger`, sink construction, pattern and level lives in `src/common/uxSysTermTools.cpp`
and the three launchers - that is, in the binaries. The library only ever *uses* whatever
default logger it finds, which is exactly the behaviour a host process wants: it configures
spdlog, and the engine writes there.

So 2.3 needed no injection mechanism, only a guarantee that this stays true. That is what
§3c checks.

## 3c. The gate (2.3)

`test/embedding_boundary.py`, registered as the `embedding_boundary` test, asserts four
properties of `src/rdb/lib` and `src/include/rdb` - the library and its public headers, not
the `xtrdb` tool that shares the directory:

1. no file-scope mutable state (`static` / `inline` variables);
2. no mutable static class members - the category the first sweep missed;
3. no logger *configuration*, only use;
4. no `std::exit` and no `FatalError` - phase 1's result, now held in place.

Two lists keep it honest. `ALLOWED` is for deliberate, permanent exceptions, each with its
reason: the `MemoryStore::processDefault()` instance, and the `xalloc` index (an index
allocated once, not state). `KNOWN_DEBT` is for real defects with a plan - printed on every
run, not fatal, and meant to shrink. The distinction exists because an allowlist that hides
a defect is worse than no gate at all.

## 3d. The owner: L2 arrives early (2.4, 2.5)

De-globalizing needs somewhere for the state to go, and phase 2 kept running into the same
wall from three directions: `rdb::storage` had no way to accept a store, the probe counters
had nowhere to live, and phase 3's `step()` will need the same thing. All three want an object
that means **one engine**.

That object was never in doubt - `embedded-roadmap.md` §2 already names the layer
(**L2 `librdbembed`**, shared by nanobind, the C ABI and JNI) and `jupyter-integration.md`
names the class (`Engine`, at J1). What was open was *timing*: `Engine` as specified needs
`compile()`, `step()` and `rows()`, and those are phase 3.

The decision taken: **create L2 now, in minimal form.** `rdb::embed::Engine` exists, owns a
`MemoryStore`, and builds storages bound to it through `openStorage()`. Nothing else. J1 adds
its methods to an object that already exists rather than inventing one under the pressure of
a different problem, and phase 2 gets to *assert* its central claim instead of asserting it in
prose.

Two properties of the class are load-bearing rather than stylistic:

- It is **non-movable and non-copyable**. `storage` holds a raw `MemoryStore*` into the
  engine; moving the engine would relocate the store and leave every storage built from it
  pointing at nothing.
- It is **optional**. `rdb::storage` still defaults to `MemoryStore::processDefault()`, so the
  server and the existing binding are untouched and know nothing about `Engine`. A test
  asserts exactly that, because "the new object breaks nobody" is a claim worth checking.

`test_embedEngine.cpp` is where the phase-2 thesis becomes an assertion: two engines, the same
MEMORY stream name, no shared state. Before this it could not have been written - not because
the test was hard, but because there was nothing to call an engine.

## 4. What phase 2 still owes

**The probe counters.** `probe.hpp` holds three `inline` counter objects, so two engines in
one process share their measurements. They are in the header deliberately: the increment must
inline, because a jump to another translation unit would be visible in the measurement itself.
Moving them into an engine object costs a pointer chase in the tick loop, so this is a
measurement-versus-isolation trade-off rather than a mechanical move. Listed in the gate's
`KNOWN_DEBT`.

**`executorsmState.hpp`.** Roughly twenty globals - `pProc`, `core_mutex`,
`plan_epoch_mutex`, the `esm::` group. They are genuinely the server's, and no notebook links
them today. They also stop being globals naturally when phase 3 restructures the run loop
around `step()`, so converting them now would be work done twice.

**The binding.** `rdb::embed::Engine` exists but `module.cpp` does not expose it, so a
notebook still cannot build two isolated engines. This is the last step that makes
`test_reentry.py::test_two_storages_open_at_once` assert isolation instead of coincidence -
and it is now plumbing, since §3d settled who owns what.
