# Core phase 1: FatalError becomes an exception

**Status:** slice 1 (§2) and both halves of §3 item 1 - sub-slice 2a, the storage
construction path, and sub-slice 2b, the read and write path - are **done**.
`src/rdb` no longer contains a single `FatalError` call site. §3 items 2 and 3 are not
started.
Stage 1a (`9a1e72eb`) is this phase's test harness. **Prerequisite for:** everything
above the storage layer, in all three embedding targets - see
[`embedded-roadmap.md`](embedded-roadmap.md).

After 2b: **142 `FatalError` call sites** in `src/`, **all of them in
`src/retractor/lib`**. The storage layer is at zero. (At `f22cffe0` it was 221 total -
79 in `src/rdb/lib`, 142 in `src/retractor/lib`.) No bare `exit()` anywhere in the tree.

`FatalError` ends with `std::exit(EXIT_FAILURE)` (`src/include/fatalError.hpp:51`).
At `f22cffe0` there are **221 call sites** in `src/` - 79 in `src/rdb/lib`, 142 in
`src/retractor/lib` - plus two bare `exit(EPERM)` calls in
`src/rdb/lib/DESCParser.cc:25,37`.

The work is not the one-line change to `FatalError`. It is unwinding through code
written on the documented assumption that a fatal error ends the process.

## 1. The precedent to follow

**This conversion has already been done once in this tree**, on the RQL side, on
2026-09-05. Read `src/retractor/lib/RQLParser.cpp:28-120` before writing anything;
its comments record three traps that cost real debugging.

**Trap 1 - returning from the listener is not enough.** ANTLR enters error recovery
and keeps calling `ParserListener` callbacks on half-built contexts, where
`ctx->ID()` is null. The throw has to leave through the generated code, which catches
only `RecognitionException` - so the thrown type must not derive from it.

**Trap 2 - `removeParseListeners()` before the throw is mandatory, not tidiness.**
Unwinding runs `antlrcpp::FinalAction`, which calls `exitRule()`, which calls
`exitDeclare()` on a context stopped mid-construction. The first attempt at this fix
segfaulted there - a worse failure than the `exit(EPERM)` it replaced.

**Trap 3 - the status global becomes sticky the moment `exit()` goes away.** The RQL
side kept "Fail" in a file-scope variable that the parser never reset on entry. That
was harmless *only* because `exit(EPERM)` came first. `test_compiler.cpp` pins the
fix as `TEST(xparser, parse_failure_does_not_poison_the_next_parse)`, whose comment
states the rule: parse status is the state of one call, not of the process.

## 2. Slice 1: the descriptor read path - DONE

Landed as described below. What the implementation added beyond the specification
is recorded in §2.5.

Do this first. It is self-contained, it has the precedent above to copy, and stage 1a
already ships the test that flips.

**Scope correction:** this is not a two-line change. Converting the two `exit(EPERM)`
calls alone changes nothing observable, because the caller then hits `FatalError` one
frame up (`descriptor.cc:343`) and the process dies anyway. The slice is the whole
path from the ANTLR listener to `loadDescriptorFile`.

### Files

| File | Change |
|---|---|
| `src/rdb/lib/DESCParser.cc:13,25,37` | `exit(EPERM)` becomes a throw, following `abortParse()` in `RQLParser.cpp`. **Delete the `statusDesc` global** - it has external linkage and is trap 3 waiting to happen. Status becomes a per-call return value. |
| `src/rdb/lib/DESCParser.cc:116-136` | `parserDESCString()` catches its own thrown type and returns the message, exactly as `parserRQLString()` does. |
| `src/rdb/lib/descriptor.cc:335-347` | `operator>>` must stop calling `FatalError` on a bad parse. A stream extractor signals failure with `is.setstate(std::ios::failbit)`; the decision to abort belongs to the caller, not to the operator. |
| `src/rdb/lib/descriptorIO.cc:12-27` | `loadDescriptorFile` throws instead of `FatalError`, for both the empty descriptor (`:21-25`) and a failed parse (new). This is the function the Python binding calls. |
| `src/include/rdb/` (new header) | The exception type. See §2.1. |
| `src/rdb/xtrdbStorageMap.cpp:454`, `src/rdb/cmdOpen.cpp:36` | The two other `>> desc` callers. They relied on the process dying; they now see `failbit` and must report and continue. `xtrdb` is an interactive inspection tool - dying on a bad file was never right there. |
| `src/python/module.cpp` | Register the new exception, drop the now-redundant existence guard if `loadDescriptorFile` covers it, and flip the test in §2.3. |

`src/rdb/lib/storage.cc:41` calls `loadDescriptorFile` inside `attachDescriptor()`;
it needs no catch, but confirm the throw unwinds cleanly through it - `storage` owns
`unique_ptr` members and a destructor that flushes a pending gap.

### 2.1 The exception type

Phase 1 needs a base type; the full taxonomy is phase 5. Keep it to what this slice
raises, and put it where both the engine and the binding can see it. The Python side
already has `RetractorDBError` / `NoSuchStream` / `StorageError` registered
(`src/python/module.cpp`) and a fourth is expected - the stage-1a tests name it
`CorruptDescriptor` in their docstrings.

Do **not** make it derive from `RecognitionException` (trap 1).

### 2.2 Verify the sticky global is gone

Write the DESC twin of `TEST(xparser, parse_failure_does_not_poison_the_next_parse)`:
parse a malformed descriptor, assert it fails, then parse a good one in the same
process and assert it succeeds. Without this test the defect is invisible, because no
current test parses a bad descriptor - it would have killed the test binary.

Note that `test/UnitTest/test_descriptor.cpp:268-271` asserts
`parserDESCString(...) == "OK"` at four call sites. Those assertions are what a
sticky "Fail" would break, and only if something failed earlier in the same binary.

### 2.3 Which stage-1a tests flip, and which do not

`api/python/tests/test_fatal_paths.py` holds two cases. **They have different causes
and only one belongs to this slice.**

| Test | Path | This slice? |
|---|---|---|
| `test_malformed_descriptor_file_ends_the_process` | DESCParser listeners, `exit(EPERM)` | **Flipped** - now `test_malformed_descriptor_file_raises` |
| `test_empty_descriptor_file_ends_the_process` | `descriptorIO.cc:21-25`, `FatalError` | **Flipped** - now `test_empty_descriptor_file_raises` |
| `test_guarded_paths_do_not_end_the_process` | binding-level guards | No - passes unchanged, still in a subprocess |

Both flipped cases run in the test interpreter rather than a subprocess: the suite
finishing at all is the assertion that the process survived. Three cases were added
alongside them - that `CorruptDescriptor` is caught by `except RetractorDBError`
(nanobind's fallback translator would otherwise make it a plain `RuntimeError`), that
a rejected descriptor does not poison the next read, and that the throw unwinds
cleanly through a half-built `storage`.

### 2.4 What the implementation added beyond this specification

Three things the spec did not call out, each because converting an exit into a throw
moves a decision that used to belong to nobody:

**`operator>>` parses into a temporary.** The listener fills the descriptor as it
walks the tree, so an aborted parse used to leave the caller's descriptor half
built. A caller that checks `failbit` only after the call would already have had its
own object overwritten. `TEST(descriptor, stream_extraction_reports_failure_through_failbit)`
pins both that and the next point.

**`failbit` has to be cleared on the success path.** `while (is >> str)` ends only by
failing at end of stream, so failbit was set after a *complete* read too. Without
clearing it, "failbit means the descriptor did not parse" would have been false for
every caller. eofbit and badbit are left alone, and a stream that was already bad on
entry is returned untouched - clearing there would have masked a failed `open()`.

**`launcher.cpp` latches `fatalErrorRaised` in its top-level handlers.** `FatalError`
set that flag before `std::exit`; a throw that escapes to `main` sets nothing, and
`executorsm::cleanup()` (registered with `atexit`, and run when `main` returns) reads
it to decide whether to clear the service query file. Without the latch a systemd unit
killed by a poisoned plan restarts straight back into that plan. This is the first
instance of a question every later slice raises: **what did the code downstream of
`std::exit` rely on, that unwinding does not provide?**

### 2.5 Acceptance

- `ctest` fully green, including the 263 existing tests.
- The two flipped Python tests pass in their new form; the guard test unchanged.
- The new no-poison test passes.
- `ninja test_gate` equal or better, run from a build directory **without**
  `RDB_PYTHON` - see the PIC note in [`build-options.md`](build-options.md).
- A malformed `.desc` in a notebook raises and the kernel survives.

## 3. After slice 1

Ordered by how much each unblocks, not by size:

1. ~~**`src/rdb/lib`, remaining 77 sites.**~~ **Done**, in two sub-slices at the seam
   described in §3.1a: 2a, the construction path, and 2b, the read and write path.
   Stage 1a's guards are now redundant rather than load-bearing, which was the point.
2. **`src/retractor/lib`, 142 sites.** The hard half. `executorsm.cpp:70-77` and
   `ipcServer.cpp:54-69` already carry comments about `FatalError` running `atexit`
   handlers on the calling thread; those two are where unwinding will surprise you.
3. **Phase 2, de-globalization.** `statusDesc` is dealt with by slice 1; the other
   two are `fatalErrorRaised` (`fatalError.hpp:16`) and the MEMORY maps
   (`faccmemory.cc:13-16`). `api/python/tests/test_reentry.py` is the acceptance test
   and passes today, so it is a regression guard from the start.

### 3.1a Sub-slice 2a: the storage construction path - DONE

**18 sites**, chosen as everything reachable while *building* a storage:
`storagePaths.cc` (5), `accessorFactory.cc` (3), the construction half of `storage.cc`
(9) and `verifyDescriptorMatch` in `descriptorIO.cc` (1). The remaining 11 sites in
`storage.cc` are read and write, and go with 2b.

Two things on the construction path were left behind **on purpose**, and both for the
same reason: `saveDescriptorFile`'s two sites are I/O failures. Every I/O-failure site
in this layer - those two, `storage::read`/`write`, the `facc*` accessors - converts
together in 2b, so the `IOError` type arrives once with all of its call sites instead
of being half-introduced here against two of them. `descriptor.cc`, `payload.cc` and
`convertTypes.cc` are data-shape helpers used from both paths, so they go with 2b too.

**Why that seam.** The read path runs inside `dataModel::processRows()` while
`core_mutex` is held, which is the same condition that hung the process until
2026-09-14 (`executorsm::cleanup()`, `executorsm.cpp:70-77`). Unwinding through it is
a materially different risk from unwinding through plan setup, and it deserves its
own slice rather than being smuggled in with the easy half.

**Two exception types, because this slice raises two kinds of error:**

| Type | Python | Meaning |
|---|---|---|
| `rdb::ConfigError` | `ConfigError` | The caller asked for something that cannot work: unknown storage type, missing `:STORAGE` directory, empty identifier, descriptor with no REF. Input is wrong, engine is intact. **Worth catching.** |
| `rdb::LogicError` | `InternalError` | An engine invariant broke - payload never attached, record count out of step with the accessor. Not reachable by any correct call sequence. **Not worth catching to continue**; it says the state is already wrong. |

**The hole this closed.** `makeAccessor` is the only place the list of accepted
storage types exists, so no binding-level guard could stand in front of it without
duplicating that list and drifting from it. `rdb.Storage(..., storage_type="NONSENSE")`
took the kernel down, and nothing in `module.cpp` could have prevented it.

**What this slice taught, which §3.2 should expect more of.** Converting an exit into
a throw does not merely change how a failure is reported - it hands the failure to
whatever `catch` happens to be on the stack. Three places had to be corrected because
they were written when nothing in the engine threw for a fatal condition:

- **`executorsm::run()` (`executorsm.cpp`)** had `catch (std::exception &e)` written
  for IPC failures. It would have reported a missing `:STORAGE` directory as
  `IPC Fail.`, returned `EINTR` instead of `EXIT_FAILURE`, and - worst - left
  `fatalErrorRaised` clear, so a systemd unit would restart into the same broken plan
  forever. A `catch (const rdb::Error &)` now sits in front of it. `it_fatal_exit_path`
  is what would have caught the exit code, and nothing would have caught the flag.
- **`launcher.cpp`** latches the same flag in its top-level handlers (added in slice 1,
  same reason).
- **`xtrdbLauncher.cpp`** had no top-level `catch` at all, so any throw from a command
  would have been `std::terminate` - SIGABRT in place of a message, in a shell whose
  whole job is to survive a bad command.

One broad catch was deliberately **left alone**: `executorsmCommands.cpp:225` handles
an ad-hoc query in the communication thread, where reporting a bad configuration back
to the client and keeping the server alive is exactly right. That one gets *better*
with this slice - it used to be a server-wide `std::exit`.

The question to ask for every later site is therefore not "will this unwind safely",
but **"who catches it on the way out, and what will they think it was?"**

### 3.1b Sub-slice 2b: the read and write path - DONE

**60 sites** across `storage.cc` (11), `payload.cc` (16), `fagrp.cc` (7),
`descriptor.cc` (6), `convertTypes.cc` (5), the five `facc*` accessors (11),
`descriptorIO.cc` (2) and `sourceBuffer.cc` (1).

**One new type, `rdb::IOError`** (Python `IOError`), for failures that are neither bad
input nor engine bugs: a failed `open`, a read the accessor rejected, a descriptor that
would not write. Everything else split into the existing `ConfigError` and `LogicError`.

#### The prerequisite that had to land first

2b could not be done safely until `executorsm::run()` stopped publishing the epoch's
`dataModel` as a bare pointer cleared only on the normal exit path. `processRows()`
runs while holding `plan_epoch_mutex`; an exception out of `storage::read` unwinds past
that clear, destroys the model, and leaves the communication thread dereferencing a
dangling `pProc` (`dumpManager.cpp:61,93,115,120,123`, `executorsmAdHoc.cpp:83,236`).
The code already carried a comment forbidding `break` in that loop for exactly this
reason - **a throw is a `break` the compiler does not warn about.** `EpochPublication`
turns that prohibition into an invariant. `fatal_exit_path` paths 5 and 6 pin it, the
second with a client command parked on the epoch lock.

The reassuring half of the same question: unwinding *releases* `core_mutex` and
`plan_epoch_mutex` on the way out, which `std::exit` never did - it ran `cleanup()` on
the same thread with those locks held, the deadlock that forced `try_to_lock` into
`cleanup()` on 2026-09-14. Throwing removes that condition rather than adding to it.

#### Two latent bugs the conversion exposed

Neither was caused by this refactor; both were invisible while the process died.

- **`posixBinaryFileWithShadow` leaked a file descriptor.** The constructor opens the
  data file, then the shadow. Throwing on the second means no destructor runs and the
  first `fd` leaks - once per failed attempt, where `std::exit` had made it moot. Now
  closed explicitly before the throw.
- **Failed `open` reported `fd` instead of `errno`.** After a failed `::open`, `fd` is
  always -1, so the message said only that it failed. `IOError` carries
  `strerror(errno)`, which is the difference between "no such directory" and
  "permission denied".

#### The second hole in the binding

`Descriptor::fieldIndex`, `fieldByteOffset` and `fieldTypeName` are bound straight
through, so the field name arrives from whoever typed it - and a typo killed the
kernel. The engine now throws and names the field; the binding translates to `KeyError`,
which is what a Python caller expects from a failed lookup. This is the same shape as
2a's `storage_type` hole: **a value that crosses from user input into a lookup table
that only the engine knows.**

#### Where the taxonomy is still lying, and to whom

`ConfigError` carries two data-dependent cases it does not really describe: a rational
with a zero denominator (`convertTypes.cc`, four sites) and a text source whose token
does not match the declared NULL field (`facctxtsrc.cc`). These are bad *data*, not bad
*configuration*. `ConfigError` was chosen as the least-wrong of the three available
types - calling them `LogicError` would accuse RetractorDB of a bug over the user's
input. **Phase 5 should give data errors their own type**; these five sites are its
first call sites.

## 4. Where stage 1a left things

Committed as `9a1e72eb` on `feature/jupyter-integration`. `retractordb._core` binds
the storage layer read-only; 21 tests green on macOS arm64 with Python 3.14.7 and
nanobind 3.1.0. Build it with `scripts/python-venv.sh` and `-DRDB_PYTHON=ON`
(`build-options.md`). The binding guards four paths that would otherwise reach
`FatalError`; every one of them becomes unnecessary as phase 1 lands, and
`test_guarded_paths_do_not_end_the_process` is what tells you if a guard is removed
too early.
