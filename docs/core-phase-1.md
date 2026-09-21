# Core phase 1: FatalError becomes an exception

**Status:** not started. Stage 1a (`9a1e72eb`) is complete and is this phase's test
harness. **Prerequisite for:** everything above the storage layer, in all three
embedding targets - see [`embedded-roadmap.md`](embedded-roadmap.md).

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

## 2. Slice 1: the descriptor read path

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
| `test_malformed_descriptor_file_ends_the_process` | DESCParser listeners, `exit(EPERM)` | **Yes** - flips to `pytest.raises` |
| `test_empty_descriptor_file_ends_the_process` | `descriptorIO.cc:21-25`, `FatalError` | Yes, if `loadDescriptorFile` is converted as specified above |
| `test_guarded_paths_do_not_end_the_process` | binding-level guards | No - must keep passing unchanged |

Each flipping test carries the `pytest.raises` form to replace its body with, in its
own docstring.

### 2.4 Acceptance

- `ctest` fully green, including the 263 existing tests.
- The two flipped Python tests pass in their new form; the guard test unchanged.
- The new no-poison test passes.
- `ninja test_gate` equal or better, run from a build directory **without**
  `RDB_PYTHON` - see the PIC note in [`build-options.md`](build-options.md).
- A malformed `.desc` in a notebook raises and the kernel survives.

## 3. After slice 1

Ordered by how much each unblocks, not by size:

1. **`src/rdb/lib`, remaining 77 sites.** Finishes the storage layer and makes stage
   1a's guards redundant rather than load-bearing.
2. **`src/retractor/lib`, 142 sites.** The hard half. `executorsm.cpp:70-77` and
   `ipcServer.cpp:54-69` already carry comments about `FatalError` running `atexit`
   handlers on the calling thread; those two are where unwinding will surprise you.
3. **Phase 2, de-globalization.** `statusDesc` is dealt with by slice 1; the other
   two are `fatalErrorRaised` (`fatalError.hpp:16`) and the MEMORY maps
   (`faccmemory.cc:13-16`). `api/python/tests/test_reentry.py` is the acceptance test
   and passes today, so it is a regression guard from the start.

## 4. Where stage 1a left things

Committed as `9a1e72eb` on `feature/jupyter-integration`. `retractordb._core` binds
the storage layer read-only; 21 tests green on macOS arm64 with Python 3.14.7 and
nanobind 3.1.0. Build it with `scripts/python-venv.sh` and `-DRDB_PYTHON=ON`
(`build-options.md`). The binding guards four paths that would otherwise reach
`FatalError`; every one of them becomes unnecessary as phase 1 lands, and
`test_guarded_paths_do_not_end_the_process` is what tells you if a guard is removed
too early.
