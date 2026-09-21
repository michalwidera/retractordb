# Core phase 1: FatalError becomes an exception

**Status:** slice 1 (§2) and all of §3 item 1 are **done** - `src/rdb` no longer
contains a single `FatalError` call site. §3 item 2 is under way: it was surveyed first
(§3.2), and slices A1 (`compiler.cpp`, §3.3), A2 (the parser and plan model, §3.4) and C
(the communication thread, §3.5) are **done**. §3 item 3 is not started.
Stage 1a (`9a1e72eb`) is this phase's test harness. **Prerequisite for:** everything
above the storage layer, in all three embedding targets - see
[`embedded-roadmap.md`](embedded-roadmap.md).

After C: **80 `FatalError` call sites** in `src/`, all in `src/retractor/lib` and all in
context B, the tick path. (221 at `f22cffe0`; 142 after 2b; 102 after A1; 89 after A2.)
Contexts A and C are at zero, and so is the storage layer. (At `f22cffe0` it was 221
total - 79 in `src/rdb/lib`, 142 in `src/retractor/lib`.) No bare `exit()` anywhere in
the tree.

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
2. **`src/retractor/lib`, 142 sites.** The hard half. Surveyed before converting
   anything - see §3.2, which proposes an order and names what each slice has to
   prove. The short version: the sites split into three execution contexts, and the
   *correct* outcome differs per context, which is why they cannot be converted as
   one batch.
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

### 3.2 Survey of `src/retractor/lib` before converting anything

Written after 2b, because every slice so far produced exactly one instance of the same
question - *what did the code downstream of `std::exit` rely on?* - and in this layer
that question meets two mutexes, a communication thread and an `atexit` handler. This
section is the answer to it, gathered before any conversion.

#### 3.2.1 The sites, grouped by the thread that runs them

| Context | Files | Sites |
|---|---|---|
| **A. Plan build** - startup and, separately, the comms thread | `compiler.cpp` 40, `RQLParser.cpp` 6, `qTree.cpp` 3, `query.cpp` 3, `field.cpp` 1 | **53** |
| **B. Tick path** - under `core_mutex`, inside `plan_epoch_mutex` | `dataModel.cpp` 25, `streamInstance.cpp` 22, `dumpManager.cpp` 17, `expressionEvaluator.cpp` 10, `CRSMath.cpp` 2, `executorsmCommands.cpp` 2, `executorsm.cpp` 1, `executorsmPlanReload.cpp` 1 | **80** |
| **C. Communication thread** - under `plan_epoch_mutex` | `executorsmAdHoc.cpp` 5, `executorsmCommands.cpp` 3, `executorsmPlanReload.cpp` 1 | **9** |

**Corrected 2026-09-21, when slice C started.** The first version of this table put 30
sites in context C, all of `dumpManager.cpp`, `executorsmCommands.cpp`,
`executorsmAdHoc.cpp`, `executorsmPlanReload.cpp` and one in `executorsm.cpp`. That
grouping was made **by filename**, and the table's own heading says thread - the two do
not agree. Tracing the callers moved 21 of the 30 into context B:

| Sites | Reached from | Thread |
|---|---|---|
| `dumpManager.cpp` 17 | `streamInstance.cpp:560,578,597` | tick path |
| `executorsmCommands.cpp` 239,241 (`printRowValue`) | `IpcServer::broadcast` <- `executorsm.cpp:431,534` | tick path |
| `executorsm.cpp` 161 (`getAwaitedStreamsSet`) | `executorsm.cpp:520` | tick path |
| `executorsmPlanReload.cpp` 317 (`applyPendingPlan`) | `executorsm.cpp:579` | tick path |

`dumpManager` is the instructive one: it is a member of `streamInstance`, so the
assumption that it is reached "through `pProc` from the comms thread" reads plausibly
and is wrong - the comms thread never calls it. Nothing was lost by the error, because
all 21 already sit under `executorsm::run()`'s `catch (const rdb::Error&)` from 2a. What
the correction changes is **which slice owns the decision**: these are tick-path
semantics, and they get decided in B.

The grouping is the whole point: **a failure in context A at startup should end the
process, and the same failure in context A on the comms thread must not.**
`compiler::compile()` is called from both - `launcher.cpp:523` on the main thread and
`executorsmAdHoc.cpp:60,150` / `executorsmPlanReload.cpp:84,284` on the communication
thread. Today `FatalError` makes both end the process, which means **a malformed ad-hoc
query kills the server**. That is the same defect the RQL parser conversion fixed one
level up on 2026-09-05, still present one level down.

#### 3.2.2 Lifetime hazards: better than expected

Five raw pointers to automatic objects are published globally, each justified in
`executorsmState.hpp` by "`std::exit` does not run destructors of automatic objects".
Every one is a potential `pProc`. Their status:

| Pointer | Points at | Exception-safe? |
|---|---|---|
| `pProc` | `dataModel` in the epoch loop | **Was not** - fixed by `EpochPublication` (§3.1b) |
| `serviceGuardPtr` | `FlockServiceGuard` in `run()` | **Yes** - `LockGuardScope`, `executorsm.cpp:189-194` |
| `busPtr` | `bus::Bus` in `run()` | **Yes** - `BusScope`, `executorsm.cpp:196-198` |
| `executorsm::coreInstancePtr` | `qTree` in `main` | Yes by lifetime - outlives `run()` |
| `executorsm::cmPtr` | `compiler` in `main` | Yes by lifetime - outlives `run()` |

The RAII pattern `EpochPublication` introduces already existed twice in the same file;
`pProc` was the one that had been left as bare statements. `fatal_exit_path`'s
lock-hygiene gate passing on the new throw paths is the empirical confirmation that
`serviceGuardPtr` unwinds correctly.

Both mutexes are taken with `std::scoped_lock`, so unwinding releases them - which
`std::exit` never did. The `try_to_lock` workaround in `cleanup()` exists because of
exactly that. **Converting this layer removes the condition that workaround was written
for.**

#### 3.2.3 Who catches it on the way out

| Catch | Effect on a converted site | Verdict |
|---|---|---|
| `executorsmCommands.cpp:225` | Reports to the client, server lives | **Correct, and the point** - this is what makes context A and C survivable |
| `exprSimplify.cpp:94` | `foldConstants` swallows it and returns `nullopt` | **Deliberate, but changes behaviour** - see below |
| `presenter.cpp:611` | Compile-path error returns `EINTR` (4), not `EXIT_FAILURE` (1) | **Needs the same treatment `executorsm::run()` got** |
| `launcher.cpp:523` | A *returned* "Fail compile" gives `protocol_error` (71); a *throw* gives 1 | Exit code changes - decide and pin it |
| `.antlr/RQLParser.cpp`, 47 sites | Catches `RecognitionException` only | Harmless for `rdb::Error` - but see §3.2.4 |

**`exprSimplify.cpp:94` is the interesting one.** `foldConstants` evaluates a constant
subexpression and returns `nullopt` if it cannot, deliberately leaving the error for run
time. Today `expressionEvaluator.cpp`'s 10 sites `std::exit`, so that catch never sees
them. After conversion it will: expressions that currently kill the process at compile
time will instead be silently left unfolded. That is arguably the *better* behaviour,
but it is a **semantic change to the optimizer**, and it is why `expressionEvaluator.cpp`
must not be converted casually alongside the rest of context B. `it_optimizer_ablation-*`
and `ut_exprSimplify` are the tests that would show it.

#### 3.2.4 The ANTLR listener discipline applies again

The sites in `RQLParser.cpp`, `qTree.cpp`, `query.cpp` and `field.cpp` reachable from
inside a `ParserListener` callback must follow `abortParse()`: **`removeParseListeners()`
before the throw**. Unwinding otherwise runs `antlrcpp::FinalAction` → `exitRule()` →
`exitDeclare()` on a half-built context. This is trap 2 from §1, and it has already cost
one segfault on the RQL side and been avoided once on the DESC side. It is not new
knowledge - it just has to be applied a third time.

#### 3.2.5 Proposed order

**1. Context A, the compiler (53 sites).** First, not last. Highest user-visible value -
it stops a malformed ad-hoc query from killing the server - and the receiving catch
(`executorsmCommands.cpp:225`) is already correct. No locks are held at startup. The
work is the A-startup / A-comms split: `compile()` must return a status where it returns
one today and throw where the process should end. Fix `presenter.cpp:611` with it.

**2. Context C, the communication thread (9 sites, after the correction above).** Small,
but it needs a transport boundary built first - see §3.5.

**3. Context B, the tick path (80 sites), minus `expressionEvaluator`.** Both locks
unwind cleanly and `EpochPublication` covers the model, so the mechanical risk is lower
than it looks. `RDB_FAULT_THROW_IN_SLOT` already exercises this exact path.

**4. `expressionEvaluator.cpp` (10 sites), on its own.** Because of §3.2.3 - it is the
only group whose conversion changes what the optimizer *produces*, rather than only what
happens when something fails.

#### 3.2.6 What each slice must prove

- Exit code **1**, not 4 and not 71, wherever the process is still meant to die.
- `fatalErrorRaised` latched on every path that ends the process, or a systemd unit
  restarts into the plan that killed it.
- A bad ad-hoc query leaves the server running and answers the client (the whole point
  of slice 1 above).
- No published pointer outlives its object - the `pProc` question, asked once per slice.
- `ninja test_gate` unchanged: the research gate is the guard on optimizer output.

### 3.3 Slice A1: `compiler.cpp` - DONE

**40 sites.** Not a mechanical conversion: the work was deciding, per site, between
*returning a message* and *throwing*, because `compile()` already reports plan errors by
return value and only used `FatalError` for what its author considered impossible.

#### The bug this fixes

`compiler::compile()` runs on **two threads**: the main thread at startup
(`launcher.cpp:523`) and the communication thread for ad-hoc queries and plan reloads
(`executorsmAdHoc.cpp:60,150`, `executorsmPlanReload.cpp:84,284`). `FatalError` ended the
process in both, so **a malformed ad-hoc query killed the server** - every stream, for
every client, because one person mistyped a step. `xqry -a 'select * stream b from
s@(0,4)'` was enough. This is the same defect the RQL parser conversion fixed one level
up on 2026-09-05, still present one level down.

#### 6 of 40 are reachable by a user

Classified by reading what guards each site and where its value comes from - whether the
condition derives from user-written RQL tokens or from state an earlier pass established.

| Line | Reachable by | Now |
|---|---|---|
| 226 | `FROM x & 0` | plan error |
| 247 | `FROM x % 0` | plan error |
| 273 | `FROM x - 0` | plan error |
| 315 | `FROM x@(0,4)` | plan error |
| 1013 | `SELECT x[_] ... FROM x@(1,0)` - a zero-length AGSE window gives an empty descriptor, so `[_]` has nothing to expand | plan error |
| 1805 | an interval ratio extreme enough that the origin search passes `1<<24` before converging | plan error |

The other 34 are compiler invariants - pipeline state an earlier pass guarantees, regexes
the compiler wrote itself, program sizes fixed by the grammar - and became
`rdb::LogicError`, which is deliberately **not** caught at the boundary. A broken
invariant is a bug in this code, not in the user's plan, and must not come back to a
client as "your query is wrong".

Two of the six were not signposted by their message text (1013 and 1805 read like
invariants), and two that read like user errors are not: 1589 (`AGSE step` again) and
1706 (`dump range invalid`) are pre-gated upstream - by site 315 and by
`RQLParser::buildRule` respectively, the latter with a comment naming this very
`FatalError` as its reason. **Both stay BUG only because of that gate; if either gate is
reordered or removed, they must be reclassified.**

#### The mechanism

`PlanError`, a file-local type carrying a message, thrown by the six and caught by a
function-try-block on `compile()`, which returns it as the status string. The same
pattern `RQLSyntaxError` uses in `RQLParser.cpp`, for the same reason: the errors are
detected a dozen frames below a function whose contract is to return a status, and
threading returns through every pass would change the signature of each one.

#### What else had to move

- **`presenter.cpp:611`** got the `catch (const rdb::Error &)` that `executorsm::run()`
  got in 2a - without it a compile-path engine error left the presenter as `EINTR` (4),
  the code for "interrupted", when nothing interrupted anything.
- **`fatal_exit_path` path 2 was rewritten, because this slice removes its premise.** It
  asserted that an ad-hoc `@(0,4)` kills the server cleanly with exit 1. That assertion
  was a correct test of a *defect*: it verified the tidiness of the crash rather than
  questioning the crash. It now asserts that the client gets the error and the server
  keeps serving. The exit-path mechanism it originally guarded is still covered, by the
  `RDB_FAULT_*_IN_SLOT` hooks, which do not depend on any RQL still reaching a fatal error.
- **Two `EXPECT_DEATH` tests in `test_compiler.cpp` flipped to `EXPECT_THROW`.**

#### A separate defect found while classifying, not fixed here

Site 2294 (`substrate name denotes two different programs`) looked user-reachable via a
readable-name collision - `FROM a # b_c` and `FROM a_b # c` both composing
`STREAM_HASH_a_b_c`. It is not, and the reason is worse than if it were:
`qTree::topologicalSort()` (`qTree.cpp:25`) rebuilds the vector from name-keyed maps and
therefore **silently drops one of two same-id queries**, and `expandSchemaWildcards`
calls it before this check can run. So that collision does not reach a diagnostic - it
produces a quietly wrong plan, with one substrate gone and its consumer reading the
other's data. Worth its own issue; it is not a phase-1 problem and converting it would
not have surfaced it.

#### Still to do in §3 item 2

Contexts B and C, per §3.2.5. Slice A2 is §3.4.

### 3.4 Slice A2: the parser and plan model - DONE

**13 sites**: `RQLParser.cpp` 6, `qTree.cpp` 3, `query.cpp` 3, `field.cpp` 1.

#### Trap 2 turned out not to apply where it mattered

A2 was split from A1 on the expectation that all six parser sites, sitting inside
`class ParserListener`, would need the `removeParseListeners()` dance before throwing.
**Three of them did not need to throw at all.** The listener already has a gentle error
channel - `reportSemanticError()`, which records the first message, and which
`parserRQLString` returns as the parse status. `buildRule` has used it since the DUMP
range check, and `exitDeclare` uses it for two policy conflicts.

So the three user-reachable sites just report and return:

| Site | Reachable by |
|---|---|
| `exitFraction` | `x & 1/0` - the grammar's `fraction_rule: DECIMAL DIVIDE DECIMAL` has no zero guard |
| `exitSelect`, FILE name | `SELECT ... FILE ''` - `STRING` is `'\'' (~'\'' | '\'\'')* '\''`, and the `*` admits the empty string |
| `exitCoption`, directive value | `:STORAGE ''`, same reason |

One asymmetry found by getting a test wrong: the empty-name check lives in `exitSelect`,
where `FILE` is optional, and there has never been one in `exitDeclare`, where it is
mandatory. So `DECLARE ... FILE ''` is not a parse error - it reaches the `StoragePaths`
constructor and comes back as sub-slice 2a's `ConfigError`. A worse message, but not a
dead process, so it is not a phase-1 site. Worth a nicer parse-time message some day.

`exitFraction` also sets a substitute denominator of 1 before returning, because the tree
walk continues to the end of the statement and `boost::rational<int>(n, 0)` would throw
`bad_rational` before the status could get back.

**A stale comment is worth recording.** `exitWindow_agg` carried: *"szerokosc NIE jest tu
sprawdzana: listener parsera nie ma lagodnego kanalu bledu (zostaje FatalError)"* - the
listener has no gentle error channel, so window width is validated in the compiler
instead. That was true when written and false by the time A2 arrived; the channel was
added later for `buildRule` and nobody revisited the comment. The deferral it justified
is still fine, but the reason it gave had expired.

#### The three that do throw

`exitWindow_agg`'s missing argument mark and the two unknown-name branches are
unreachable while the grammar restricts names to `MIN|MAX|AVG|SUMC` (`RQL.g4`
`window_agg`, `stream_fn_call`), so they are `rdb::LogicError`. These *are* thrown from
inside the walk, so trap 2 applies in full: a new `abortInternal()` helper removes the
parse listeners first, mirroring `abortParse()`. `ParserListener` gained the parser
reference the two error listeners already held.

#### The plan model

| Site | Type | Why |
|---|---|---|
| `qTree::getAvailableTimeIntervals`, zero interval | `ConfigError` | The value comes straight from the DECLARE interval, and the grammar admits `0`. Called from `executorsm::run` **after** compilation, so there is no status channel to return through - it must throw, but it names the right culprit. |
| `query::descriptorFrom`, AGSE step | `ConfigError` | Pre-gated by A1's site 315, so arguably an invariant. `ConfigError` chosen deliberately: if that gate is ever reordered out of the way, `LogicError` would accuse the engine of a defect that is really a typo in a query. Of the two possible wrong answers, this is the cheaper one. |
| `qTree::dumpCore`, `qTree::getQuery` empty name, `query::descriptorFrom` undefined cmd, `GetArgs` oversized program, `field::getFirstFieldToken` | `LogicError` | No RQL text produces any of them. |

Two more `EXPECT_DEATH` tests flipped (`test_qTree.cpp`), bringing phase 1's running total
to four.

#### A checker mistake worth not repeating

The per-line string-literal balance check added after A1 reported 206 failures in
`test_compiler.cpp`. All were false: the file holds 103 raw strings (`R"(...)"`), inside
which a line legitimately carries an odd number of quotes. The checker now strips raw
strings before reasoning per line. A verification tool that cries wolf is worse than none,
because the next real failure is read as noise.

### 3.5 Slice C: the communication thread - DONE

Nine sites, all of them "unreachable" invariants: three null-pointer checks
(`collectStreamsParameters` 2, `commandProcessor` 1), five in `getAdHoc` (an exhausted
keyword filter, two bus statuses the ad-hoc path cannot produce, two null pointers) and
one bus status in `validatePlanText`. All nine became `rdb::LogicError`. None is a user
error, so none needed `ConfigError`: a client typo is answered on the normal path long
before any of these.

#### The prerequisite: the comms thread had no boundary at all

`IpcServer::commandLoop()` had exactly one handler, `catch (IPC::interprocess_exception&)`,
written for **failed construction** of the IPC resources. Anything else leaving the loop
body also leaves the thread lambda in `IpcServer::start()` - which is `std::terminate`.

That is a *worse* exit than the one being replaced. `std::exit` runs `atexit`, so
`executorsm::cleanup()` removes the segment, the command queue and the named mutex;
`std::terminate` runs none of it, and the next start finds those objects still in shared
memory. Converting even one comms-thread site without a boundary would therefore have
been a regression, not an improvement.

Two roads to `std::terminate` were already open there, independent of this refactor:
`read_info()` on a malformed message, and `lexical_cast<int>` on a missing `db.id`. Both
are now logged drops.

The per-message body is wrapped, and `clientProcessId` is extracted **before** the
dispatch, because on the error path it is what decides whether there is anyone to answer.
A consequence worth recording: a message with no `db.id` is now dropped before handling,
which makes the `'show'` handler's own `db.id` check unreachable from this loop. The
check stays as a guard for any other caller, with a comment saying so.

`IPC::interprocess_exception` is rethrown to the outer handler, deliberately, so that
this change does not quietly re-decide IPC failure policy. That handler labels a failure
from `mymap->insert()` as "IPC resources could not be created", which is wrong - it is
noted here rather than fixed, because fixing it is a decision about failure policy and
not about exception plumbing.

#### The catch that was about to mislabel them

`commandProcessor` already ends in `catch (std::exception&)` returning
`error.response: "command processor failure: ..."`. `rdb::Error` derives from it, so the
moment these nine throw, an engine invariant would be reported to the client under a
label written for a failure of the **dispatcher**. This is the third instance of the same
trap in this refactor - `executorsm::run()`'s IPC catch in 2a, `presenter.cpp`'s in A1 -
and the fix is the same: `catch (const rdb::Error&)` placed ahead of it.

The `coreInstancePtr` check in `commandProcessor` also had to move *inside* the try. It
sat above it, where a throw would bypass the function's own error boundary and land in
the transport backstop - which is a last resort, not a reporting channel. Under
`std::exit` the placement made no difference, which is why it was there.

#### What the test asserts

`it_fatal_exit_path` path 7, driven by `RDB_FAULT_THROW_IN_COMMAND`. The hook lives in
the `.onCommand` lambda in `executorsm.cpp`, not in `ipcServer.cpp`: that keeps the
transport free of protocol knowledge, and it throws from exactly the place the backstop
exists for - past `commandProcessor`'s own catch. It is keyed on the command **name** so
that `kill` stays reachable while the hook is armed.

The path asserts the whole contract: the server lives, the client's log carries the
reason over IPC rather than the silence it would otherwise wait out, `hello` still works
afterwards, and shutdown is regular. It is the deliberate counterpart to
`it_show_handler_failure`, which exercises the same client-visible outcome one floor up,
inside the handler.

#### One difference from every slice before it

In contexts A and B the answer to "what should happen when the invariant breaks" is
*end the epoch* or *end the process*. Here it is neither: the service keeps running and
one client gets an error. That is the first place in this refactor where an engine
invariant violation is **not** fatal to anything except the request that triggered it.

## 4. Where stage 1a left things

Committed as `9a1e72eb` on `feature/jupyter-integration`. `retractordb._core` binds
the storage layer read-only; 21 tests green on macOS arm64 with Python 3.14.7 and
nanobind 3.1.0. Build it with `scripts/python-venv.sh` and `-DRDB_PYTHON=ON`
(`build-options.md`). The binding guards four paths that would otherwise reach
`FatalError`; every one of them becomes unnecessary as phase 1 lands, and
`test_guarded_paths_do_not_end_the_process` is what tells you if a guard is removed
too early.
