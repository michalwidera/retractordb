# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

RetractorDB uses **Conan 2** for dependency management and **CMake** (Make or Ninja) for building.

### Initial setup (first time)
```bash
conan profile detect
conan install . -s build_type=Debug --build missing
conan build . -s build_type=Debug --build missing
```

### Incremental builds (from `build/Debug`)
```bash
make          # or: ninja
make install  # installs to ~/.local/bin
```

### Common targets (from `build/Debug`)
```bash
make test          # run all tests (unit + integration, via valgrind)
make cformat       # format all C++/CMake source files
make descgrammar   # regenerate ANTLR4 descriptor grammar from DESC.g4
make rqlgrammar    # regenerate ANTLR4 query grammar from RQL.g4
```

### Running a single test
```bash
cd build/Debug
ctest -R ut-test_payload          # run one unit test by name
ctest -R ut-test_payload -V       # verbose output
```

Unit tests run through valgrind (leak checking enabled). Integration tests compare output against patterns in `test/IntegrationTest/*/Pattern*/`.

### Coverage (from repo root)
```bash
cmake --preset conan-debug -DENABLE_COVERAGE=ON
cd build/Debug && ninja clean && ninja ctest
cd ../..
gcovr --root . --filter 'src/' --gcov-executable gcov-14 \
      --exclude '.*\.antlr.*' build/Debug \
      --html-details coverage/coverage.html --xml coverage/coverage.xml --print-summary
```

## Architecture

Three cooperating executables, each with a corresponding source directory under `src/`:

| Binary | Source | Role |
|--------|--------|------|
| `xretractor` | `src/retractor/` | Main DB process: compiles `.rql` files, executes continuous query plans |
| `xqry` | `src/qry/` | Client: queries running xretractor via shared memory |
| `xtrdb` | `src/rdb/` | Inspection/testing tool: reads binary artifacts and metadata |

### Library layers

**`rdb` library** (`src/rdb/lib/`, public headers in `src/include/rdb/`):

- `Descriptor` — describes a binary record layout (field names, types, sizes, array multiplicity). Extends `std::vector<rField>`. Persisted as `.desc` files via the `DESC.g4` ANTLR grammar.
- `payload` — typed view over a raw memory buffer, interpreted according to a `Descriptor`. Supports null-per-field tracking via `nullBitset`.
- `FileInterface` — abstract I/O contract (`read`, `write`, `count`). Implementations: `faccbindev` (binary device), `faccfs` (filesystem), `faccmemory` (in-memory), `faccposix` / `faccposixshd` (POSIX with shared memory), `facctxtsrc` (text source), `fagrp` (group).
- `storage` — coordinates `Descriptor` + `payload` + `FileInterface`. Selects the right accessor based on the descriptor's TYPE config field. Manages `.desc`, binary data, and `.meta` sidecar files. Handles gap detection and null-fill logic.
- `metaDataStream` — per-record null/gap metadata sidecar, stored alongside binary data files.

**`retractor` library** (`src/retractor/lib/`):

- `qTree` — topologically sorted `std::vector<query>`. Central data structure passed through compilation and execution.
- `query` / `token` / `field` — individual query representations parsed from `.rql` files.
- `compiler` — multi-pass compile chain: simplify → prepare fields → replicate indexes → convert references → apply constraints → fill buffer sizes.
- `dataModel` — owns all `streamInstance` objects at runtime; drives per-interval processing.
- `streamInstance` — one per declared or derived stream; holds `outputPayload` (what gets stored) and `inputPayload` (what gets computed from the FROM clause).
- `executorsm` — dual-threaded executor: deterministic processing loop + communication thread (handles ad-hoc queries via shared memory / boost IPC).
- `CRSMath` — rational stream mathematics for computing aligned time intervals.

### Grammar files

- `src/rdb/lib/DESC.g4` — ANTLR4 grammar for `.desc` descriptor files.
- `src/retractor/lib/RQL.g4` — ANTLR4 grammar for the RetractorQL query language.
- Generated code lives in `.antlr/` subdirectories — do not edit by hand; regenerate with `make descgrammar` / `make rqlgrammar`.

### Tests layout

```
test/
  UnitTest/          # GTest tests run through valgrind; one executable per source file
    test_payload.cpp
    test_descriptor.cpp
    ...
  IntegrationTest/   # Shell-based tests; each subdirectory is a scenario
    simple/
    issue42_rule/
    dsp/
    ...
```

CI runs on CircleCI for branches matching `master` or `issue_*`.

## Code Style

- **Standard**: C++23
- **Formatter**: clang-format (Google style, 129-column limit, 2-space indent). Run `make cformat` before committing.
- **Linker**: `mold` (set in CMakeLists via `-fuse-ld=mold`)
- **Dependencies**: Boost, spdlog (header-only), fmt (header-only), ANTLR4 runtime, GTest, magic_enum — all managed by Conan.
- Comments in source are frequently in Polish; this is intentional.
