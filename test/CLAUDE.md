# Test tree — known pitfalls

### Integration test file sync

`test/CMakeLists.txt` copies `test/` → build dir at **cmake configure time**. After editing `.rql`, `data.txt`, or a test `.sh`:
- Re-run cmake, **or** manually copy to `build/Debug/test/...`
- `CTestTestfile.cmake` is CMake-generated — do NOT overwrite with `CMakeLists.txt`; different formats.

**Reconfigure (`cmake .`) wipes built unit-test binaries.** The `test/` copy step regenerates the `build/Debug/test/` subtree, deleting `test_*` binaries → ctest fails with `No such file or directory` for ~all unit tests. After any `cmake .`, rebuild with `ninja` before `ctest`.

### One tree, one macro — semicolons are a trap

Since the `IntegrationTest_serial` / `IntegrationTest_parallel` merge, **every** integration test
goes through the `add_test` macro in `test/IntegrationTest/CMakeLists.txt`, which re-expands
arguments via `_add_test(${ARGV})`. That splits each argument on its **internal semicolons**, so

```cmake
add_test(NAME t COMMAND sh -c "set -e ; a ; b")   # WRONG: shell runs only `set -e`
```

registers a test that is always green and checks nothing. Chain with `&&`, or — when the script
needs shell syntax that requires `;` (`if ... ; then ... ; fi`) — put it in its own `run.sh` /
`verify.sh` invoked as `COMMAND bash verify.sh`. The `harness_command_integrity` test enforces
this: after `-c` there must be exactly one argument.

### Comparing output: use `compare.sh`, not `cmake -E compare_files`

Every pattern comparison in the tree goes through `test/IntegrationTest/compare.sh`:

```cmake
COMMAND bash -c "set -e && <produce out.txt> && bash ../compare.sh --ignore-eol pattern.txt out.txt"
```

`cmake -E compare_files` only reports *that* the files differ. When `it_agse_array` failed on CI
on 2026-09-04 that was the whole evidence — and the failure never reproduced locally, so there was
nothing else to go on. `compare.sh` prints a unified diff plus both file sizes, so the CI step log
alone is enough to diagnose. `--ignore-eol` mirrors the cmake flag exactly: it ignores trailing CR
**and** a missing final newline (`it_rotation_test`'s `count.pattern` has no trailing newline).
Without the flag the comparison is byte-exact.

That paid off on the next occurrence: the diff showed `0 Record(s)` against a valid descriptor,
which pinned the cause to `_kbhit()` reading a byte pending on the CI terminal and ending the
processing loop before its first slot. Fixed in the engine (a run with a declared slot budget
ignores the keyboard) and guarded by `it_tty_keystroke_immunity`, which runs the engine under a
pseudo-terminal with a byte in the buffer.

### Failure reports from CI

`scripts/collect-test-failures.py <build-dir> [--ctest-status N]` collects, for every test that
failed in *this* run: the registered command, its slice of `LastTest.log`, the small text files
from its working directory, the engine and client logs from the namespace `TMPDIR`
(`namespace-logs/`), and a ready diff of each pattern/output pair. CI runs it in the same
step as `ctest` and publishes `test-failure-report` as an artifact.

The `TMPDIR` logs are collected because `xretractor.log` does **not** live in the test's working
directory, and it is the only place that names a swallowed server-side exception. When
`it_fncall_runtime_case` failed on CI on 2026-09-04 the client reported nothing but a missing
response queue; the sentence naming the cause (`Command processor failure: …`) was written to
`xretractor.log` in `TMPDIR` and thrown away with the container. Note that a namespace slot is
shared by several directories, so such a log may also contain a neighbour's run — `RESOURCE_LOCK`
separates them in time, not in file content.

Two traps it works around, both found the hard way:
- `ctest --show-only` (used to map test names to working directories) **overwrites `LastTest.log`**,
  so the log is read into memory before that call and copied into the report.
- CTest does **not** clear `LastTestsFailed.log` after a green run, so a stale entry would produce a
  report for a test that just passed. The CI status is passed in explicitly, and entries are
  cross-checked against `LastTest.log`.

### A handler that fails silently

`it_show_handler_failure` guards the other half of that same 2026-09-04 investigation. The `show`
command writes nothing into its reply even when the subscription **succeeds**, so a swallowed
exception produced a reply indistinguishable from success: the client started its producer thread
and reported a missing response queue a second later, one step away from the cause. The reply now
carries `error.response`, and the client checks it exactly as it already checked the reply to
`get`.

A CI race cannot be ordered on demand, so the test forces the failure through the `RDB_FAULT_SHOW`
hook read by the handler. Without the fix the test goes red on both counts: the client's log lacks
the server's reason and still carries `did not appear after`.

### The `--xqrywait` gate

`it_xqrywait_gate` guards two properties of the `-x` gate, both broken until 2026-09-04 and both
fixed by the same change (the gate now waits on its own latch instead of borrowing `iLoopLimitCnt`).

- **A command from the startup window must lift the gate.** The lock file's `PID:` line — the
  `server_start` contract — is published well before `dataModel` is built, and the gate flag was
  set only after. The window measured ~90 ms for a 120-stream plan, and a command landing in it was
  lost: the server waited for the *next* one. Reproduced 5/5.
- **`--llimitqry` must survive the gate.** The gate flag lived in the slot-budget counter, so
  lifting it wrote back "unbounded": `xretractor -m 5` exited on its own, `xretractor -x -m 5` ran
  forever. Deterministic, no race involved.

The observable for both is the same and unambiguous: with `-m 5` a server whose gate was lifted
exits by itself. Do **not** assert on the server's stdout instead — redirected to a file it is
block-buffered, and a grep will report a gate that is merely unflushed as a gate that is stuck.
That mistake produced a false 5/5 during this investigation before it was caught.

Part A is timing-assisted: it can pass falsely if the window closes before `xqry` starts, but it
cannot fail falsely, so it is repeated three times.

### Namespaces

Each test directory gets `RDB_NAMESPACE`, its own `TMPDIR` and a `RESOURCE_LOCK`, assigned from a
pool of 16 by that same macro. A directory that must run on the machine-global identity (unnamed
instance, or names it picks itself) opts out with `set(IT_NO_NAMESPACE TRUE)` before its
`add_test` calls, and gets `RUN_SERIAL` instead.

**Integration tests run the *installed* binary + the *build-copied* script, not source.** Editing a `.sh` and the C++ it exercises requires syncing both: build, install, recopy script, rebuild test binaries. Full sequence after touching integration `.sh` + source:
```bash
ninja && ninja install && cmake . && ninja && ctest
```

