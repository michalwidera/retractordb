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

## Code Style

- **C++23**, clang-format Google style, 129-col limit, 2-space indent. Run `ninja cformat` before commit.
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

The command sequence — staged-file scan, per-file report, cleaning, re-check and re-stage, plus the
whole-tree variant for a push and the commit-message check — is in the `watermark-check` skill.
Invoke it before committing and before pushing.

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
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue <source-file>
```

- On a hit in a source file: **stop and report it to the human** with file, line and codepoint. Do not sweep
  the file with `--in-place`. The targeted repair is
  `python3 "$WM/clean_text.py" <file> --aggressive-homoglyphs --strip-emoji-glue -o <file>.fixed`, followed by
  a `git diff` confirming that only the offending codepoint changed.
- Any `U+00A0` or invisible codepoint in code is a defect, never "informational".

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

Moved next to the code they govern, so they load when those files are in play:

- `src/retractor/lib/CLAUDE.md` — COMMA ambiguity in `select_list`, adding a scalar function, Descriptor field sizes for STRING expressions in SELECT.
- `test/CLAUDE.md` — integration test file sync: cmake copy timing, reconfigure wiping unit-test binaries, installed-binary vs build-copied script.
