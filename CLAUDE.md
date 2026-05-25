# CLAUDE.md

## Build

Conan 2 + CMake + Ninja. Setup via `scripts/buildrdb.sh` (run from repo root, `scripts/`, or `build/Debug/`):

```bash
scripts/buildrdb.sh toolchain   # apt packages + Python venv + Conan
scripts/buildrdb.sh conan       # detect profile, set C++23
scripts/buildrdb.sh ninja       # add Ninja generator to profile
scripts/buildrdb.sh bashrc      # add retractordb/bin to PATH
scripts/buildrdb.sh debug       # conan install + build (Debug)
scripts/buildrdb.sh release     # conan install + build (Release)
scripts/buildrdb.sh coverage    # build with coverage + gcovr report → coverage/coverage.html
```

Options chain: `scripts/buildrdb.sh conan ninja debug`

**Incremental (from `build/Debug`):**
```bash
ninja               # build
ninja install       # install to ~/.local/bin
ninja test          # all tests (unit + integration, via valgrind)
ninja cformat       # format C++/CMake sources
ninja descgrammar   # regenerate ANTLR4 grammar from DESC.g4
ninja rqlgrammar    # regenerate ANTLR4 grammar from RQL.g4
```

**Single test:**
```bash
ctest -R ut_payload     # by name
ctest -R ut_payload -V  # verbose
```

Unit tests: valgrind + leak check. Integration tests: output matched against `test/IntegrationTest_serial/*/Pattern*/` and `test/IntegrationTest_parallel/*/Pattern*/`.

CI: CircleCI, branches `master` or `issue_*`.

## Architecture

| Binary | Source | Role |
|--------|--------|------|
| `xretractor` | `src/retractor/` | Main DB: compiles `.rql`, executes continuous query plans |
| `xqry` | `src/qry/` | Client: queries xretractor via shared memory |
| `xtrdb` | `src/rdb/` | Inspection/testing: reads binary artifacts and metadata |

**`rdb` library** (`src/rdb/lib/`, headers in `src/include/rdb/`):
- `Descriptor` — binary record layout (field names, types, sizes, multiplicity). Extends `std::vector<rField>`. Persisted as `.desc` via `DESC.g4`.
- `payload` — typed view over raw buffer per `Descriptor`. Null-per-field via `nullBitset`.
- `FileInterface` — abstract I/O (`read`, `write`, `count`). Impls: `faccbindev`, `faccfs`, `faccmemory`, `faccposix`/`faccposixshd`, `facctxtsrc`, `fagrp`.
- `storage` — coordinates `Descriptor` + `payload` + `FileInterface`. Manages `.desc`, binary data, `.meta` sidecars. Gap detection + null-fill.
- `metaDataStream` — per-record null/gap metadata sidecar.

**`retractor` library** (`src/retractor/lib/`):
- `qTree` — topologically sorted `std::vector<query>`. Central structure for compile + execution.
- `query` / `token` / `field` — query representations parsed from `.rql`.
- `compiler` — passes: simplify → prepare fields → replicate indexes → convert refs → apply constraints → fill buffer sizes.
- `dataModel` — owns all `streamInstance` objects; drives per-interval processing.
- `streamInstance` — per stream: `outputPayload` (stored) + `inputPayload` (computed from FROM).
- `executorsm` — dual-threaded: processing loop + comms thread (shared memory / boost IPC).
- `CRSMath` — rational stream math for aligned time intervals.

**Grammars:**
- `src/rdb/lib/DESC.g4` → `.antlr/` (regenerate: `ninja descgrammar`)
- `src/retractor/lib/RQL.g4` → `.antlr/` (regenerate: `ninja rqlgrammar`)
- Never edit generated files by hand.

**Tests layout:**
```
test/
  UnitTest/                   # GTest + valgrind; one binary per source file
  IntegrationTest_serial/     # serial shell tests; subdirs = scenarios
  IntegrationTest_parallel/   # parallel shell tests
```

## Code Style

- **C++23**, clang-format Google style, 129-col limit, 2-space indent. Run `ninja cformat` before commit.
- **Linker**: `mold` (via CMakeLists `-fuse-ld=mold`)
- **Deps**: Boost, spdlog (header-only), fmt (header-only), ANTLR4 runtime, GTest, magic_enum — Conan.
- Source comments in Polish — intentional.

**Include order (5 blocks, blank-line separated):**
```cpp
#include "own.hpp"            // 1. own header (.cpp only)

#include <fcntl.h>            // 2. C/POSIX  (<*.h>)

#include <algorithm>          // 3. C++ stdlib  (<name>)

#include <spdlog/spdlog.h>    // 4. third-party (Boost, spdlog, …)

#include "myproject.hpp"      // 5. project headers ("…")
```

Sorted case-insensitively within each block. `IncludeBlocks: Preserve` — blank lines are barriers; `cformat` never moves includes across them. Maintain block structure manually.

**Use C++ headers, not C:** `<ctime>` not `<time.h>`, `<cstdlib>` not `<stdlib.h>`, etc.

## Code Guidelines

1. **Ask before implementing** — state assumptions, surface ambiguities, push back on overcomplicated requests.
2. **Minimum code** — no speculative features, no single-use abstractions, no impossible-scenario error handling.
3. **Surgical edits** — touch only what the task requires; don't improve adjacent code; match existing style.
4. **Clean your orphans** — remove imports/vars/functions YOUR changes made unused; leave pre-existing dead code alone.
5. **Verify** — for multi-step tasks state a brief plan with success criteria per step; loop until verified.

## ANTLR4 Grammar — Known Pitfalls

### COMMA ambiguity in `select_list` vs `function_call`

`select_list` uses `COMMA` to separate expressions. ANTLR4 SLL mode ignores call-stack context → `f(a, b)` inside multi-item SELECT parses as `f(a)`, `, b` treated as list separator.

**Rule:** Never use `COMMA` in function arguments in `RQL.g4`. Use `COLON` or another token absent from `select_list`.

Current pattern: `to_string(expr : N)` — `:` (COLON) = output field width.

### Adding new function call tokens

1. Add grammar alternative with non-COMMA separator — do NOT extend `( COMMA expression_factor )*`
2. `ninja rqlgrammar` to regenerate `.antlr/`
3. Add `CALL2` (or new `command_id`) to `src/include/cmdID.hpp`
4. Handle in `exitFunction_call` in `src/retractor/lib/RQLParser.cpp` — store extra param as `std::pair<std::string, int>` (IDXPAIR variant)
5. In `exitExpression`: accumulate output field size from token value
6. In `expressionEvaluator.cpp`: pop 1 arg (size is in token, not stack)

### Integration test file sync

`test/CMakeLists.txt` copies `test/` → build dir at **cmake configure time**. After editing `.rql`, `data.txt`:
- Re-run cmake, **or** manually copy to `build/Debug/test/...`
- `CTestTestfile.cmake` is CMake-generated — do NOT overwrite with `CMakeLists.txt`; different formats.

### Descriptor field sizes for STRING expressions in SELECT

`exitExpression` in `RQLParser.cpp` sums output field size from `program` tokens:
- `CALL2` + `to_string` → `pair.second` (declared width)
- `CALL` + `to_string` (no width) → 32 (default)
- `PUSH_VAL` string literal → `string.length()`
- Summed: `to_string(x:16)+'_test'` → 16+5=21
