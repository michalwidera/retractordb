# CLAUDE.md

This file is the binding source for build, testing, style, collaboration, and commit/push/CI rules. `AGENTS.md`
adds the agent-facing entry points it does not duplicate: the indexed system knowledge in the sibling
`knowledge-index` repository (referenced here through `.agents/skills/retractordb-system`), and
`scripts/install-codex-skill.sh`, which installs that external skill for other workspaces.

## Build

Conan 2 + CMake + Ninja. Setup via `scripts/buildrdb.sh` (run from repo root, `scripts/`, or `build/Debug/`):

```bash
scripts/buildrdb.sh toolchain   # apt packages + Python venv + Conan
scripts/buildrdb.sh conan       # detect profile, set C++23
scripts/buildrdb.sh ninja       # add Ninja generator to profile
scripts/buildrdb.sh bashrc      # add ~/.local/bin to PATH (matches install prefix)
scripts/buildrdb.sh debug       # conan install + build (Debug)
scripts/buildrdb.sh release     # conan install + build (Release)
scripts/buildrdb.sh package     # cpack DEB/TGZ + auto-clean staging (_CPack_Packages, install_manifest.txt)
scripts/buildrdb.sh coverage    # build with coverage + gcovr report → coverage/coverage.html
```

Options chain: `scripts/buildrdb.sh conan ninja debug`

**Incremental (from `build/Debug`):**
```bash
ninja               # build
ninja install       # install to ~/.local/bin (prefix auto-defaults to ~/.local — no sudo)
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
- `metaData` — per-record null/gap metadata sidecar.

**`retractor` library** (`src/retractor/lib/`):
- `qTree` — topologically sorted `std::vector<query>`. Central structure for compile + execution.
- `query` / `token` / `field` — query representations parsed from `.rql`.
- `compiler` — passes: simplify → prepare fields → replicate indexes → convert refs → apply constraints → fill buffer sizes.
- `dataModel` — owns all `streamInstance` objects; drives per-interval processing.
- `streamInstance` — per stream: `outputPayload` (stored) + `inputPayload` (computed from FROM).
- `executorsm` — dual-threaded: processing loop + comms thread (shared memory / boost IPC).
- `CRSMath` — rational stream math for aligned time intervals.
- `appConfig` — optional TOML service config (toml++). Layered search: `/etc/retractor/retractor.toml` → `$XDG_CONFIG_HOME`/`~/.config/retractor/retractor.toml` → `--config <file>`. Missing config = valid (defaults). Currently exposes `[storage] dir` — default storage dir used only when RQL has no `:STORAGE` directive (RQL wins).

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
3. **Surgical edits** — touch only what the task requires; don't improve adjacent code even if it looks wrong; match
   existing style. Report what looks wrong at the end of the task, with file and line and one sentence on why.
   Whether it gets fixed is the human's decision; fixing it is a separate task and needs a separate go-ahead.
4. **Clean your orphans** — remove imports/vars/functions YOUR changes made unused; leave pre-existing dead code alone.
5. **Verify before reporting done** — never claim success without running the relevant `ctest`.

## Collaboration Rules

### Session start

Every session has a single declared goal in the form:
> "Cel: X. Gotowe gdy: Y. Pliki dotknięte: Z."

Then, before touching code:

```bash
git status        # clean, or holding only the diff handed over at the end of the previous session
ninja cformat     # format the tree as found, so later reformatting does not pollute the diff
ctest -R ...      # relevant tests must pass
```

Never start a new topic on top of unrelated uncommitted work.

**Planning threshold** — one rule for the whole session:
- **3 or more files** — present a plan with success criteria and wait for approval before writing any code.
- **1-2 files** — state the steps and success criteria, then proceed without waiting.

### AI watermark hygiene (text)

Every text artifact that enters the repository must be free of AI provenance marks — invisible Unicode
(zero-width, bidi, tag chars, variation selectors, private use) and space homoglyphs. **Images are out of
scope: marks in `.png` / `.jpg` / `.pdf` / figures may stay.** The requirement covers only text: sources,
scripts, `.md`, `.rql`, `.g4`, CMake, TOML/YAML and commit messages.

Tool: `watermarks-remover` (default `~/github/watermarks-remover`), used through its local scripts — **do not
start the Docker/HTTP service for this check**. Layer A only (deterministic Unicode scrub); statistical
Layer B rewriting is not part of this rule.

**Mandatory sequence before every commit and before every push.** No commit or push goes out — and no diff is
handed over for human review — while the check reports a hit.

```bash
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
TEXT='\.(md|txt|tex|bib|rql|desc|cpp|hpp|h|c|g4|sh|py|ya?ml|toml|json|cmake|in)$|CMakeLists\.txt$'

# 1. Check the staged text files (empty output = clean)
git diff --cached --name-only --diff-filter=ACM | grep -E "$TEXT" \
  | while read -r f; do python3 "$WM/inspect_text.py" --json "$f" >/dev/null 2>&1 || echo "WATERMARK: $f"; done

# 2. Report for a flagged file (which codepoints, where)
python3 "$WM/inspect_text.py" <file>

# 3. Clean it, then drop the backup the tool leaves behind
python3 "$WM/clean_text.py" <file> --in-place --stats && rm -f <file>.bak

# 4. Re-check, then re-stage
python3 "$WM/inspect_text.py" --json <file> >/dev/null && git add <file>
```

Before a push, run the same check over the whole tracked tree — substitute `git ls-files` for
`git diff --cached --name-only --diff-filter=ACM` in step 1. Optionally check the commit message too:
`git log -1 --pretty=%B | python3 "$WM/inspect_text.py" -`.

#### Source code — zero tolerance, strict mode

Documentation can be fixed later; **source code cannot**. A zero-width character or a Cyrillic lookalike
inside an identifier, string literal, RQL query or grammar rule compiles, diffs and reviews as normal text,
and the resulting failure is practically undebuggable by hand. Nothing may ever introduce such a character
into `.cpp` / `.hpp` / `.h` / `.c` / `.g4` / `.rql` / `.desc` / `.sh` / `.py` / `.cmake` / `CMakeLists.txt` /
`.toml` / `.yml` / `.json`.

Consequences for the assistant:

- **Never paste model, browser or chat output straight into a source file.** Retype it as ASCII, or clean it
  before it lands on disk.
- **Check code immediately after editing it** — right after the edit, before `ninja cformat` and before the
  build, not at commit time. A defect found at push has already been built and tested against.
- Code uses **strict mode**, which the default check does not cover:

```bash
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue <source-file>
```

  Default mode misses Latin/Cyrillic confusables: `int value = 1;` whose `a` is a Cyrillic `U+0430` instead of
  ASCII `a` passes it and is caught only by `--aggressive`. (Write such an example by naming the codepoint —
  never paste the actual character into a rule file, a comment or a test.) `--strip-emoji-glue` additionally rejects the load-bearing invisibles that
  are legitimate in prose but never in code. Verified against the whole `src/` and `scripts/` tree: strict
  mode yields zero hits, and Polish diacritics in comments are not affected.

- On a hit in a source file: **stop and report it to the human** with file, line and codepoint. Do not sweep
  the file with `--in-place`. The targeted repair is
  `python3 "$WM/clean_text.py" <file> --aggressive-homoglyphs --strip-emoji-glue -o <file>.fixed`, followed by
  a `git diff` confirming that only the offending codepoint changed.
- Any `U+00A0` or invisible codepoint in code is a defect, never "informational".

Rules:

- **Never run `clean_text.py` on binary fixtures** (`test/**/*.dat`, `.meta`, `.shadow`, ECG records,
  `examples/**` data files). It rewrites bytes and corrupts them, and integration tests compare output
  byte-exactly. The extension filter above exists for that reason — do not widen it with `--force-text`.
- `--in-place` writes a `.bak` next to the file. Delete it; never commit it.
- `U+00A0` (no-break space) is reported as *informational*. In `.rql`, `.g4` and C++ sources it is always a
  defect — normalize it. Elsewhere confirm it is not a deliberate typographic space before replacing.
- If cleaning would change test fixtures or generated ANTLR files, stop and hand the case to the human instead
  of editing them.

### Commits, push and CI

- **`master` in the code repository** — commits and pushes are performed by the human only, after reviewing the diff. The
  assistant must leave changes uncommitted, show the diff, and wait for the human to commit and push.
- **Side branches** — the assistant may create local commits autonomously after verification, provided no CI process is
  triggered.
- Permission to commit on a side branch does not include permission to push, open a pull request, or invoke CI manually.
  Those actions require an explicit human request.
- If an action would trigger CI, stop and hand it over to the human.

### Session end

Every session ends with either a permitted local commit on a side branch, an explicit handoff of the uncommitted diff on
`master` for human review/commit/push, or an explicit note why no commit was created. No unexplained uncommitted progress
is left behind.

**Research gate — mandatory before closing.** Whenever the session touched engine sources (`src/`), run the gate and
report its verdict before the commit or the handoff:

```bash
ninja test_gate          # from build/Debug or build/Release
```

It is deliberately outside `ninja` and `ninja test` (see `test/research_gate/README.md`), so nothing runs it
implicitly. It is directional: a result worse than the reference is an **error** and stops the work; equal passes;
better passes and is recorded. A skipped level (missing or stale H9 ablation profiles) counts as *not run*, never as
passed — report it as such. No commit and no handoff goes out with an unreported or failing gate; a red gate is
handed to the human, not worked around.

Sessions that touched only tests, scripts or documentation do not need the gate — say explicitly that it was skipped
and why.

**Ablation floor — mandatory when the session touched an optimizer pass.** The `RDB_OPT_*` switches must not change
what the engine computes, only how fast it gets there. That invariant rots silently: the matrix broke with the `>N`
tail rule change of 2026-08-07 and nobody noticed for twelve days, because `manual-ablation` runs only by hand. Whenever
the session touched `src/retractor/lib/compiler.cpp`, the startup-latency or tail rules (`SOperations.hpp`,
`computeStartupLatency`), or any code behind an `RDB_OPT_*` switch, build the all-off configuration and run the full
suite before the commit or the handoff:

```bash
scripts/buildrdb.sh release-ablation     # interactive: set all five switches OFF, probe OFF
ctest --test-dir <katalog wypisany przez skrypt>/test -j 4
```

Success is **the whole suite green**, exactly as in the default configuration — the switches are an efficiency knob,
not a semantics knob. A test that holds only with a switch ON is either wrong or documents a real difference, and that
difference belongs in `def:observable`: `Val` must be equal, `Lat` only non-increasing (see `research_plan.md` §14.20).
Never paper over a red ablation run with `WILL_FAIL` or `DISABLED` — the matrix already carries a note from 2026-07-26
explaining why those annotations were removed. CI runs this same floor as `ablation-all-off` in layer L3 of
`manual-nightly-full`, so a skipped local run gets caught within days, not weeks.

### Context hygiene

Warn the user when the session shows signs of context degradation:
- more than ~10 back-and-forth exchanges on a single task, or
- the conversation has drifted across multiple unrelated topics, or
- you catch yourself re-asking for information already given earlier in the session.

When any of these occur, say explicitly:
> "Kontekst tej sesji jest długi — rozważ przerwę lub nową sesję od czystego stanu."

Then suggest either: (a) commit current state and end the session, or (b) defer remaining work to a new session with a fresh context.

## ANTLR4 Grammar — Known Pitfalls

### COMMA ambiguity in `select_list` vs `function_call`

`select_list` uses `COMMA` to separate expressions. ANTLR4 SLL mode ignores call-stack context → `f(a, b)` inside multi-item SELECT parses as `f(a)`, `, b` treated as list separator.

**Rule:** Never use `COMMA` in function arguments in `RQL.g4`. Use `COLON` or another token absent from `select_list`.

Current pattern: `to_string(expr : N)` — `:` (COLON) = output field width.

### Adding a scalar function

Function names are **not** grammar literals. `function_call` takes a plain `ID`, and the single
list of legal names lives in `src/include/rqlFunctions.hpp`. Adding a one-argument function:

1. Add a row to `kRqlFunctions` in `src/include/rqlFunctions.hpp` — canonical name plus arity
2. Add a branch in the `CALL` chain in `src/retractor/lib/expressionEvaluator.cpp` — match the
   **lowercased** name; the parser stores the canonical spelling, the evaluator folds case
3. No grammar edit, no `ninja rqlgrammar`, no new `command_id`

`canonical` is what the parser writes into the token, regardless of how the author spelled it.
Keep it stable: plan dumps print it (`CALL(Sqrt)`), integration `pattern.txt` files and the H9
pilot plans under `test/research_gate/h9/pilot/out/` match on it, and `exitExpression` /
`exprSimplify` compare `getStr_() == "to_string"` literally.

`compiler::checkFunctionCalls()` rejects an unknown name or a bad arity through the
`Check result:` channel, so `-c` is a gate: a program that cannot run does not compile.

**Two-argument functions.** The grammar has exactly two shapes — `f(expr)` and
`f(expr : DECIMAL)`. The second belongs to `to_string` alone: `N` is the declared output field
width carried in the token as `IDXPAIR`, not a value on the stack. A future function taking two
**evaluated** arguments needs one more grammar alternative
(`fn=ID '(' expression_factor ( COLON expression_factor )* ')'` is verified to parse) plus the
arity row — never a `COMMA` separator, see the rule above.

**`min` is unavailable as a scalar function name.** `MIN`, `MAX`, `AVG` and `SUMC` are lexer
tokens ahead of `ID` (stream reducers), so they never lex as a function name. A scalar minimum
must be called something else.

### Integration test file sync

`test/CMakeLists.txt` copies `test/` → build dir at **cmake configure time**. After editing `.rql`, `data.txt`, or a test `.sh`:
- Re-run cmake, **or** manually copy to `build/Debug/test/...`
- `CTestTestfile.cmake` is CMake-generated — do NOT overwrite with `CMakeLists.txt`; different formats.

**Reconfigure (`cmake .`) wipes built unit-test binaries.** The `test/` copy step regenerates the `build/Debug/test/` subtree, deleting `test_*` binaries → ctest fails with `No such file or directory` for ~all unit tests. After any `cmake .`, rebuild with `ninja` before `ctest`.

**Integration tests run the *installed* binary + the *build-copied* script, not source.** Editing a `.sh` and the C++ it exercises requires syncing both: build, install, recopy script, rebuild test binaries. Full sequence after touching integration `.sh` + source:
```bash
ninja && ninja install && cmake . && ninja && ctest
```

### Descriptor field sizes for STRING expressions in SELECT

`exitExpression` in `RQLParser.cpp` sums output field size from `program` tokens:
- `CALL2` + `to_string` → `pair.second` (declared width)
- `CALL` + `to_string` (no width) → 32 (default)
- `PUSH_VAL` string literal → `string.length()`
- Summed: `to_string(x:16)+'_test'` → 16+5=21
