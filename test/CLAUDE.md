# Test tree - known pitfalls

### Integration test file sync

`test/CMakeLists.txt` copies `test/` → build dir at **cmake configure time**. After editing `.rql`, `data.txt`, or a test `.sh`:
- Re-run cmake, **or** manually copy to `build/Debug/test/...`
- `CTestTestfile.cmake` is CMake-generated - do NOT overwrite with `CMakeLists.txt`; different formats.

**Reconfigure (`cmake .`) wipes built unit-test binaries.** The `test/` copy step regenerates the `build/Debug/test/` subtree, deleting `test_*` binaries → ctest fails with `No such file or directory` for ~all unit tests. After any `cmake .`, rebuild with `ninja` before `ctest`.

### Requirement IDs and `covers`

Every bullet in a `... powinna:` block (`powinien`, `powinno`, `powinny` alike; the block runs to the first line that is not a `///` comment) carries a versioned ID: `/// - [svc.write.empty-content~1] ...`. Since 2026-10-17 the whole tree uses this form; the migrated IDs read `<file>.<symbol>.<n>~1` (`dumpmanager.dump-task.3~1`), a new bullet takes the next free `<n>` of its block, and an existing ID is never renumbered. A bullet has no `Test:` line; the test that checks it says so itself, with a comment at the start of a line - `// covers: id~N, id~N` as the first line of a `TEST` body, `# covers: ...` at the start of a Python test function, at the top of the comment block above the `add_test` of an integration test in its `CMakeLists.txt` (`.cmake-format.yaml` keeps a comment block that starts with `covers:` verbatim; placed under prose, the line would be rewrapped into it and lose its marker), or near the top of a `run.sh` / `verify*.py`, `// covers: ...` right above a `static_assert`. A line `///   Nietestowalne: ...` or `///   Kontrola strukturalna: ...` under the bullet frees it from `covers`; `///   Zakres testow: ...` keeps prose about what the tests do and do not reach, and is not checked. `st_req_trace` (`scripts/req-trace.py`) fails on a block bullet without an ID, a malformed or duplicate ID, a `Test:` line under a bullet, a `covers` naming an unknown ID or an old version, and an ID with no `covers` and no exemption; `scripts/req-trace.py --report` prints the requirement-to-test matrix. Bump the version when the meaning of a requirement changes, not for an editorial fix: every `covers` of the old version then fails until someone has re-read that test against the new text.

### Verify test provenance before running

Before every focused or full `ctest`, `ninja test`, or `ninja test-valgrind` run:

1. Check `CMAKE_HOME_DIRECTORY` in `build/Debug/CMakeCache.txt` against `git rev-parse --show-toplevel`, and check the build configuration. A build directory from another checkout or configuration is not evidence for the current sources.
2. Rebuild the targets the test uses from that build directory (`ninja` for the full suite). After a CMake reconfigure, rebuild the unit-test binaries that were removed with the copied test tree.
3. Inspect the registered command with `ctest -N -V -R '<test name>'`. Unit tests should execute the rebuilt `build/Debug/test/UnitTest/` binary. For integration tests that call `xretractor`, `xqry`, or `xtrdb` by name, resolve each program through the **same effective `PATH` as CTest** (including its appended install directory), then use `cmp -s` to verify that executable equals the rebuilt target under `build/Debug/src/`. A program that the registered command or its script names by a build-tree path (see *Namespaces*) needs that path checked instead. Do not infer the chosen binary from `ninja install` or from a shell with a different `PATH`.
4. For every changed `.sh`, `.rql`, data file, or pattern used by the test, use `cmp -s` to verify the copy under `build/Debug/test/` equals the source file. Reconfigure or copy the fixture again if it differs.

If any path or comparison cannot be established, fix the build/install/copy first and repeat the provenance check before testing. Report the build directory and actual executable path with the test result.

### One tree, one macro - semicolons are a trap

Since the `IntegrationTest_serial` / `IntegrationTest_parallel` merge, **every** integration test goes through the `add_test` macro in `test/IntegrationTest/CMakeLists.txt`, which re-expands arguments via `_add_test(${ARGV})`. That splits each argument on its **internal semicolons**, so

```cmake
add_test(NAME t COMMAND sh -c "set -e ; a ; b")   # WRONG: shell runs only `set -e`
```

registers a test that is always green and checks nothing. Chain with `&&`, or - when the script needs shell syntax that requires `;` (`if ... ; then ... ; fi`) - put it in its own `run.sh` / `verify.sh` invoked as `COMMAND bash verify.sh`. The `harness_command_integrity` test enforces this: after `-c` there must be exactly one argument.

### Comparing output: use `compare.sh`, not `cmake -E compare_files`

Every pattern comparison in the tree goes through `test/IntegrationTest/compare.sh`:

```cmake
COMMAND bash -c "set -e && <produce out.txt> && bash ../compare.sh --ignore-eol pattern.txt out.txt"
```

`cmake -E compare_files` only reports *that* the files differ. When `it_agse_array` failed on CI on 2026-09-04 that was the whole evidence - and the failure never reproduced locally, so there was nothing else to go on. `compare.sh` prints a unified diff plus both file sizes, so the CI step log alone is enough to diagnose. `--ignore-eol` mirrors the cmake flag exactly: it ignores trailing CR **and** a missing final newline (`it_rotation_test`'s `count.pattern` has no trailing newline). Without the flag the comparison is byte-exact.

That paid off on the next occurrence: the diff showed `0 Record(s)` against a valid descriptor, which pinned the cause to `_kbhit()` reading a byte pending on the CI terminal and ending the processing loop before its first slot. Fixed in the engine (a run with a declared slot budget ignores the keyboard) and guarded by `it_tty_keystroke_immunity`, which runs the engine under a pseudo-terminal with a byte in the buffer.

### Failure reports from CI

`scripts/collect-test-failures.py <build-dir> [--ctest-status N]` collects, for every test that failed in *this* run: the registered command, its slice of `LastTest.log`, the small text files from its working directory, the engine and client logs from the namespace `TMPDIR` (`namespace-logs/`), and a ready diff of each pattern/output pair that a `compare.sh` call in the registered command declares - or, when one of its files is missing, a note saying so. A test whose command has no `compare.sh` (a `-vg-` check, a `run.sh` that calls `compare.sh` inside) gets no diff file: pairs are not guessed from file names, because one working directory can hold the outputs of several tests; the diff `compare.sh` printed is still in the `LastTest.log` slice. CI runs it in the same step as `ctest` and publishes `test-failure-report` as an artifact.

The `TMPDIR` logs are collected because `xretractor.log` does **not** live in the test's working directory, and it is the only place that names a swallowed server-side exception. When `it_fncall_runtime_case` failed on CI on 2026-09-04 the client reported nothing but a missing response queue; the sentence naming the cause (`Command processor failure: …`) was written to `xretractor.log` in `TMPDIR` and thrown away with the container. Note that a namespace slot is shared by several directories, so such a log may also contain a neighbour's run - `RESOURCE_LOCK` separates them in time, not in file content.

Two traps it works around, both found the hard way:
- `ctest --show-only` (used to map test names to working directories) **overwrites `LastTest.log`**, so the log is read into memory before that call and copied into the report.
- CTest does **not** clear `LastTestsFailed.log` after a green run, so a stale entry would produce a report for a test that just passed. The CI status is passed in explicitly, and entries are cross-checked against `LastTest.log`.

### Fault hooks for races - where the test histories are

A race cannot be ordered on demand, and a timing probe measures whether the window closed in time, not whether it is gone. A test that guards a race therefore opens the window through a fault hook read by the engine (`git grep -o 'RDB_FAULT_[A-Z_]*' -- src` lists them) and asserts on an observable that cannot lie. Do not assert on the server's stdout redirected to a file: it is block-buffered, and a grep reports a gate that is merely unflushed as a gate that is stuck. A fix with two sides gets one check per side, including the mirror defect, so that a "refuse always" fix cannot pass.

The history of each such test - the defect, the window, the hook, what the test does not cover - is in the `README.md` of its directory under `test/IntegrationTest/`; read it before changing the test or the code it guards:

| Test | Guards | Hook |
|---|---|---|
| `it_show_handler_failure` | the `show` reply carries `error.response` instead of failing silently | `RDB_FAULT_SHOW` |
| `it_xqrywait_gate` | the `-x` gate is lifted by a command from the startup window and by a stop signal, and keeps `--llimitqry` | none; part A is timing-assisted and repeated three times |
| `it_xqrywait_first_row` | no row is lost between lifting the gate and the client's subscription | `RDB_FAULT_GET_AWAIT_EPOCH_SWAP`, `RDB_FAULT_SHOW_DELAY` |
| `it_service_reset_double` | two plan reloads overlapping in time | `RDB_FAULT_PLAN_SWAP_DELAY` |
| `it_signal_stop` | the signal handler stores two atomics and logs nothing (#284); part B bites only outside Release | `RDB_FAULT_SIGNAL_IN_LOG` |

### Namespaces

Each test directory gets `RDB_NAMESPACE`, its own `TMPDIR` and a `RESOURCE_LOCK`, assigned from a pool of 16 by that same macro. A directory that must run on the machine-global identity (unnamed instance, or names it picks itself) opts out with `set(IT_NO_NAMESPACE TRUE)` before its `add_test` calls, and gets `RUN_SERIAL` instead.

The same macro gives every integration test an empty `XDG_CONFIG_HOME`, so the user configuration layer never reaches a test. The system layer (`/etc/retractor/retractor.toml`, plus the Homebrew and `/usr/local` paths on Darwin) has a fixed path and cannot be cut off: configure prints a `WARNING` for each such file that exists, and a malformed one stops every test that starts a program (#426).

**A program called by name runs the *installed* binary, and the script runs from its *build copy*, not from source.** Editing a `.sh` and the C++ it exercises requires syncing both. Install only after the final build: reconfiguration can relink the executable, leaving a previously installed copy stale. Full sequence after touching integration `.sh` + source:
```bash
cmake . && ninja && ninja install && ctest
```
Before `ctest`, confirm with `cmp -s` that each installed project binary selected by the test matches its final build target, as required above.

Not every call goes by name. Many directories name the build-tree target directly - `${CMAKE_BINARY_DIR}/src/<dir>/<program>` in the command, or `$<TARGET_FILE:...>` passed to a script - for `xtrdb`, `xretractor` or `xqry`, often next to a by-name call of another program in the same test. Neither a list of such directories nor a count of them survives here: the list went stale within weeks (it named four) and the count of 2026-10-03 within six, so the provenance check decides per program, from the registered command (`ctest -N -V -R '<test name>'`) and the scripts it runs: a build-tree path is checked as that path, a bare name through the CTest `PATH` lookup above. Cases with a known history:

- `service_idle` passes `$<TARGET_FILE:xretractor>` to `service-idle.sh`, deliberately. Its three tests assert service-mode INFO markers, which `SPDLOG_ACTIVE_LEVEL` strips in Release - so a Release copy left in `~/.local/bin` (a build of ablation profiles is enough) made them fail with an empty `stderr.txt`, and the symptom read as an engine regression.
- `ipc_identity_lock` and `leftover_sweep` (both since `5853a83e`, #241) pass `$<TARGET_FILE:xretractor>` and `$<TARGET_FILE:xqry>` to `verify.py`. The tree records no reason for that choice.
- `signal_stop` (#284) passes `$<TARGET_FILE:xretractor>` and the build type to `run.sh`, for the same reason as `service_idle`: it asserts a `SPDLOG_WARN` line, which only a non-Release binary emits.
- `bus_slot_count` (#266) reuses the launch pattern of `ipc_identity_lock` and passes the same two paths to its `verify.py`.

### Ablation: shape and value in separate tests

Under the all-off ablation build (*Ablation floor* in the root `CLAUDE.md`) only an assertion that a pass **fired** may be disabled. A ctest entry that also asserts the computed result has to be split, because a single `DISABLED` takes the result assertion down together with the shape assertion. `it_issue202_hash_shift_e2e` is the worked example, split on 2026-09-06 into `-shape` and `-value`: its one `cmp matched CC` pinned `Val` and `Lat` at the same time, so it could never be green under ablation, and disabling it removed the only end-to-end place where the tail divergence between `(A>2)#(B>1)` and `(A#B)>3` was visible at all.

### Valgrind copies check code, not tests

Each unit-test binary gets a `ut_<x>-valgrind` copy, but not every case belongs in it. A case leaves the copy when its cost goes into test code (an independent model, oracle tables) or into repeating an already checked path on larger data, and the union of `src/` lines covered by the binaries with a `-valgrind` copy stays the same without it. The lists are `RDB_UT_VALGRIND_SKIP` (whole binaries) and `RDB_UT_VALGRIND_FILTER_<x>` (a `--gtest_filter` for the copy only) in `test/UnitTest/CMakeLists.txt`, each entry with its reason; the ordinary `ut_<x>` always runs every case. The trigger was CircleCI's 10-minute no-output limit in the nightly of 2026-11-25 (job 1298): `ut_compiler-valgrind` is one gtest process, and one case planning 65536 fields ran silent for ~11 minutes.

A new case that takes more than about a minute under Valgrind is either added to a list or kept with a stated reason. The check is a coverage comparison in `build/Coverage` (`ENABLE_COVERAGE=ON`): delete the `*.gcda` files, run the ordinary `ut_<x>` tests of the binaries that have a `-valgrind` copy, once as they are and once with the candidates excluded through the `GTEST_FILTER` environment variable, and after each run collect `gcovr -r . --filter "$PWD/src/" --merge-mode-functions=separate --json`. The set of covered `src/` lines must not shrink; a candidate that is the only one to cover a line stays (`ProofOracle.plan_emission_schedule` is the only unit test that executes record windows in `dataModel`). A one-line difference in `bus.cpp` that comes and goes between runs is a timing-dependent path, not coverage lost. Equal line coverage is not equal memory behaviour, so a case whose allocations grow with the size it tests deserves a second look before it is excluded.
