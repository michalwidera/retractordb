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

### Failure reports from CI

`scripts/collect-test-failures.py <build-dir> [--ctest-status N]` collects, for every test that
failed in *this* run: the registered command, its slice of `LastTest.log`, the small text files
from its working directory, and a ready diff of each pattern/output pair. CI runs it in the same
step as `ctest` and publishes `test-failure-report` as an artifact.

Two traps it works around, both found the hard way:
- `ctest --show-only` (used to map test names to working directories) **overwrites `LastTest.log`**,
  so the log is read into memory before that call and copied into the report.
- CTest does **not** clear `LastTestsFailed.log` after a green run, so a stale entry would produce a
  report for a test that just passed. The CI status is passed in explicitly, and entries are
  cross-checked against `LastTest.log`.

### Namespaces

Each test directory gets `RDB_NAMESPACE`, its own `TMPDIR` and a `RESOURCE_LOCK`, assigned from a
pool of 16 by that same macro. A directory that must run on the machine-global identity (unnamed
instance, or names it picks itself) opts out with `set(IT_NO_NAMESPACE TRUE)` before its
`add_test` calls, and gets `RUN_SERIAL` instead.

**Integration tests run the *installed* binary + the *build-copied* script, not source.** Editing a `.sh` and the C++ it exercises requires syncing both: build, install, recopy script, rebuild test binaries. Full sequence after touching integration `.sh` + source:
```bash
ninja && ninja install && cmake . && ninja && ctest
```

