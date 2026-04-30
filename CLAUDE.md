# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

RetractorDB uses **Conan 2** for dependency management and **CMake + Ninja** for building (Ninja preferred; Make also supported).

### Initial setup (first time)

The helper script `scripts/buildrdb.sh` automates the full setup. Run it from the repo root, `scripts/`, or `build/Debug/` — it detects the current directory automatically. Options can be chained:

```bash
scripts/buildrdb.sh toolchain   # install apt packages + Python venv + Conan
scripts/buildrdb.sh conan       # detect Conan profile, set C++23
scripts/buildrdb.sh ninja       # add Ninja generator to Conan profile
scripts/buildrdb.sh bashrc      # add retractordb/bin to PATH in ~/.bashrc
scripts/buildrdb.sh debug       # conan install + build (Debug)
scripts/buildrdb.sh release     # conan install + build (Release)
scripts/buildrdb.sh coverage    # build with coverage + run gcovr report
```

Multiple options in one call: `scripts/buildrdb.sh conan ninja debug`

Without arguments the script runs in interactive mode.

### Incremental builds (from `build/Debug`)
```bash
ninja          # build
ninja install  # installs to ~/.local/bin
```

### Common targets (from `build/Debug`)
```bash
ninja test          # run all tests (unit + integration, via valgrind)
ninja cformat       # format all C++/CMake source files
ninja descgrammar   # regenerate ANTLR4 descriptor grammar from DESC.g4
ninja rqlgrammar    # regenerate ANTLR4 query grammar from RQL.g4
```

### Running a single test
```bash
cd build/Debug
ctest -R ut-test_payload          # run one unit test by name
ctest -R ut-test_payload -V       # verbose output
```

Unit tests run through valgrind (leak checking enabled). Integration tests compare output against patterns in `test/IntegrationTest/*/Pattern*/`.

### Coverage

```bash
scripts/buildrdb.sh coverage
```

The script auto-detects the GCC version, installs `gcovr` if missing, rebuilds with `-DENABLE_COVERAGE=ON`, runs `ctest`, and writes the report to `coverage/coverage.html`.

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
- Generated code lives in `.antlr/` subdirectories — do not edit by hand; regenerate with `ninja descgrammar` / `ninja rqlgrammar`.

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
- **Formatter**: clang-format (Google style, 129-column limit, 2-space indent). Run `ninja cformat` before committing.
- **Linker**: `mold` (set in CMakeLists via `-fuse-ld=mold`)
- **Dependencies**: Boost, spdlog (header-only), fmt (header-only), ANTLR4 runtime, GTest, magic_enum — all managed by Conan.
- Comments in source are frequently in Polish; this is intentional.

## Code Guidelines

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.
---

**These guidelines are working if:** fewer unnecessary changes in diffs, fewer rewrites due to overcomplication, and clarifying questions come before implementation rather than after mistakes.