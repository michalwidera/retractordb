# CLAUDE.md

This file is the binding source for build, testing, style, collaboration, and commit/push/CI rules. `AGENTS.md` adds the agent-facing entry points it does not duplicate: the indexed system knowledge in the sibling `knowledge-index` repository (referenced here through `.agents/skills/retractordb-system`), and `scripts/install-codex-skill.sh`, which installs that external skill for other workspaces.

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
ninja install       # install to ~/.local/bin (prefix auto-defaults to ~/.local - no sudo)
ninja test          # unit + integration, bez valgrinda
ninja test-valgrind # kontrola pamieci lokalnie (Linux)
ninja cformat       # format C++/CMake sources
ninja descgrammar   # regenerate ANTLR4 grammar from DESC.g4
ninja rqlgrammar    # regenerate ANTLR4 grammar from RQL.g4
```

**macOS** (sprawdzone tylko na Apple silicon z macOS 27 i Apple clang 21; Intel i starsze wydania nietestowane. Xcode 16.3+ CLT i deployment target 14.4+ to minimum wymuszone przez `std::print`, nie konfiguracja sprawdzona):
```bash
scripts/macos-build.sh          # jeden przebieg: konfiguracja + budowa + install + ctest, log w build/macos-build.log
scripts/macos-build.sh release
scripts/macos-build.sh --sanitize   # -DRDB_SANITIZE=address,undefined
```
`scripts/buildrdb.sh` dziala tam tak samo, tylko `toolchain` instaluje przez Homebrew. Roznice, o ktorych trzeba wiedziec czytajac wynik:
- **Valgrinda nie ma** na Apple silicon i nie bedzie. `ninja test` uruchamia binaria wprost na kazdej platformie; rownowaznikiem lokalnego `test-valgrind` jest konfiguracja `-DRDB_SANITIZE=address,undefined`.
- **Czas rzeczywisty jest slabszy z zasady**: SCHED_FIFO obejmuje WATEK, nie proces (`sched_setscheduler` nie istnieje), masek powinowactwa nie ma wcale, `mlockall` zglasza ENOSYS, a odpowiednika PREEMPT_RT nie ma. macOS jest platforma rozwojowa i testowa, nie pomiarowa - bramka badawcza (`ninja test_gate`) tego nie zmienia.
- **Usluga to launchd, nie systemd**: `restartCommand` sklada `launchctl kickstart -k`, tozsamosc jednostki bierze sie z `XPC_SERVICE_NAME`, a pakiet nie niesie zadnej jednostki `.service`.
- **Wybor galezi platformowej** nie zapada po nazwie systemu, tylko przez `RDB_HAS_*` z `generated/platformConfig.h` (probe kompilacyjne w `cmake/PlatformChecks.cmake`). Nowy kod platformowy pisze sie tak samo: `#if RDB_HAS_X`, nigdy `#ifdef __APPLE__`.
- **Galaz zapasowa tylko z deklaracji**: proba, ktorej 0 wybiera slabsza galaz, musi to 0 miec zadeklarowane w `RDB_PLATFORM_FALLBACKS` (na Linuksie lista jest pusta, na Darwinie zawiera zera zmierzone na Apple silicon); kazde niezadeklarowane 0 zatrzymuje konfiguracje. Nowa galaz zapasowa oznacza nowa pozycje na liscie kontrolowanych prob w `cmake/PlatformChecks.cmake`.

**CI locally, before pushing** (`scripts/test-ci.sh`, needs a running Docker):
```bash
ninja test-ci        # = test-ci-commit: the only job CircleCI runs after a push
ninja test-ci-nightly-debug # Debug job from run_manual_nightly_full, including Valgrind
ninja test-ci-fast   # same job, build/ kept between runs - quick, NOT a faithful CI run
scripts/test-ci.sh --list   # every profile with the CircleCI job it mirrors
```
Each profile copies the working tree into a container built from the CI image (`micwide/buildenv-retractordb`) capped at the CI executor's resources (4 vCPU / 8 GiB) and runs that job's steps. Outside `all` and outside `ninja test`. Profiles mirror `.circleci/config.yml` by hand: change that file, change `scripts/test-ci.sh`.

Every `test-ci-*` profile builds from scratch, like the CI checkout - tens of minutes, and ccache does not shorten it (the container starts with an empty cache, as a CI job does). `test-ci-fast` trades that fidelity for speed by keeping `build/` in a per-profile Docker volume; use it to check a change before committing, not to conclude anything about a CI run. `scripts/test-ci.sh --profile <name> --reset-build` drops a kept build directory.

**Single test:**
```bash
ctest -R '^ut_payload$'     # plain test by exact name
ctest -R '^ut_payload$' -V  # verbose
```

Unit tests run directly in `ninja test`. `ninja test-valgrind` repeats them under Valgrind with leak checking and includes the nine integration memory checks. The commit workflow skips Valgrind; `run_manual_nightly_full` runs it once in Debug. Plain `ctest` runs all registered groups; use `ctest -LE valgrind` to match the ordinary CI step.

CI: CircleCI, branches `master` or `issue_*`.

## Architecture

| Binary | Source | Role |
|--------|--------|------|
| `xretractor` | `src/retractor/` | Main DB: compiles `.rql`, executes continuous query plans |
| `xqry` | `src/qry/` | Client: queries xretractor via shared memory |
| `xtrdb` | `src/rdb/` | Inspection/testing: reads binary artifacts and metadata |

**`rdb` library** (`src/rdb/lib/`, headers in `src/include/rdb/`):
- `Descriptor` - binary record layout (field names, types, sizes, multiplicity). Extends `std::vector<rField>`. Persisted as `.desc` via `DESC.g4`.
- `payload` - typed view over raw buffer per `Descriptor`. Null-per-field via `nullBitset`.
- `FileInterface` - abstract I/O (`read`, `write`, `count`). Impls: `faccbindev`, `faccfs`, `faccmemory`, `faccposix`/`faccposixshd`, `facctxtsrc`, `fagrp`.
- `storage` - coordinates `Descriptor` + `payload` + `FileInterface`. Manages `.desc`, binary data, `.meta` sidecars. Gap detection + null-fill.
- `metaData` - per-record null/gap metadata sidecar.

**`retractor` library** (`src/retractor/lib/`):
- `qTree` - topologically sorted `std::vector<query>`. Central structure for compile + execution.
- `query` / `token` / `field` - query representations parsed from `.rql`.
- `compiler` - passes: simplify → prepare fields → replicate indexes → convert refs → apply constraints → fill buffer sizes.
- `dataModel` - owns all `streamInstance` objects; drives per-interval processing.
- `streamInstance` - per stream: `outputPayload` (stored) + `inputPayload` (computed from FROM).
- `executorsm` - dual-threaded: processing loop + comms thread (shared memory / boost IPC).
- `CRSMath` - rational stream math for aligned time intervals.
- `appConfig` - optional TOML service config (toml++). Layered search: `/etc/retractor/retractor.toml` → `$XDG_CONFIG_HOME`/`~/.config/retractor/retractor.toml` → `--config <file>`. Missing config = valid (defaults). Currently exposes `[storage] dir` - default storage dir used only when RQL has no `:STORAGE` directive (RQL wins).

**Grammars:**
- `src/rdb/lib/DESC.g4` → `.antlr/` (regenerate: `ninja descgrammar`)
- `src/retractor/lib/RQL.g4` → `.antlr/` (regenerate: `ninja rqlgrammar`)
- Never edit generated files by hand.

## Code Style

- **C++23**, clang-format Google style, 129-col limit, 2-space indent. Run `ninja cformat` before commit.
- Source comments in Polish - intentional.
- **Dashes:** Prefer the ASCII hyphen-minus (`-`, U+002D) in repository text. Use a typographic dash only when preserving an exact quotation or when the character itself is semantically significant.

**Include order (5 blocks, blank-line separated):**
```cpp
#include "own.hpp"            // 1. own header (.cpp only)

#include <fcntl.h>            // 2. C/POSIX  (<*.h>)

#include <algorithm>          // 3. C++ stdlib  (<name>)

#include <spdlog/spdlog.h>    // 4. third-party (Boost, spdlog, …)

#include "myproject.hpp"      // 5. project headers ("…")
```

Sorted case-insensitively within each block. `IncludeBlocks: Preserve` - blank lines are barriers; `cformat` never moves includes across them. Maintain block structure manually.

**Use C++ headers, not C:** `<ctime>` not `<time.h>`, `<cstdlib>` not `<stdlib.h>`, etc.

## Code Guidelines

1. **Ask before implementing** - state assumptions, surface ambiguities, push back on overcomplicated requests.
2. **Minimum code** - no speculative features, no single-use abstractions, no impossible-scenario error handling.
3. **Surgical edits** - touch only what the task requires; don't improve adjacent code even if it looks wrong; match existing style. Report what looks wrong at the end of the task, with file and line and one sentence on why. Whether it gets fixed is the human's decision; fixing it is a separate task and needs a separate go-ahead.
4. **Clean your orphans** - remove imports/vars/functions YOUR changes made unused; leave pre-existing dead code alone.
5. **Verify before reporting done** - never claim success without running the relevant `ctest`.

## Comparative measurement

**The corpus is `paper-arXiv/usecases`, not one or two convenient plans.** Every performance claim about the engine - a candidate optimization, a regression, an A/B between two commits - is measured on the eight use-case families `uc01`..`uc08` in the sibling `paper-arXiv` repository. Each family carries its own `generate_data.py` and `rql/query.rql`; plans span 7-23 nodes and, more importantly, differ in the SHAPE of the computation: record windows, stream generators, multi-rate joins, rules. The ECG pipeline in `examples/ecg` and the single-node ADD plan stay usable as quick probes, but a verdict does not rest on them.

**Two workloads are not a sample - this was paid for.** Issue #272 (2026-09-23) measured the handle-table change on the ADD plan and the ECG pipeline alone, found -5,5 % and -5,9 % instructions for -0,38 % and -0,65 % time, and published a methodological conclusion about the instruction-to-time converter. Extending to the eight families retracted it: both of those workloads sit at the BOTTOM of the ten-workload spread, the converter ranged 0,00 (uc05) to 0,36 (uc06), and the top of the corpus gained 1,5 %, over twice what ECG showed. The converter is a property of the PAIR (change, workload) and says nothing on its own. `uc05` is the standing counterexample to reading a time gain off an instruction count: -3,65 % instructions, exactly zero time.

Run one family from a FRESH copy of its directory with an empty `temp/`:

```bash
python3 <usecases>/ucNN/generate_data.py --out "$work"
cp <usecases>/ucNN/rql/query.rql "$work/query.rql"
mkdir -p "$work/temp" && cd "$work"
RDB_BENCH_PLAN=1 xretractor query.rql -k -r -f -m 1   # plan node count: row 'PLAN bench', field 'wyjscie'
valgrind --tool=callgrind --toggle-collect='*processRows*' ./xretractor query.rql -k -r -f -m 2000
RDB_BENCH_CSV=out.csv ./xretractor query.rql -k -r -f -m 100000
```

Sources wrap past end of input, so `-m` is free; 100k slots costs 5-6 s on the heaviest family. The apparatus traps - one fixed working directory, one fixed binary name, `build/Release-Probe` rebuilt separately from `build/Release` - are in the `callgrind-ab-comparison-traps` note and apply here unchanged.

**Publication embargo.** The corpus is unpublished material awaiting the DEBS submission (`paper-arXiv/debs`). Nothing FROM it leaves this machine: no plan text, no generated data, no generator source, no README prose - not into this repository, not into an issue comment, not into any artifact that gets published. What may be published, and is expected in issue comments, is a REFERENCE to a family by id and domain (`uc02`, mikrosiec) together with measurements DERIVED from it: node counts, instruction counts, times, p-values. The embargo lifts when the paper is out; until then, treat a request to include corpus content as a question for the human.

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

**Planning threshold** - one rule for the whole session:
- **3 or more files** - present a plan with success criteria and wait for approval before writing any code.
- **1-2 files** - state the steps and success criteria, then proceed without waiting.

### AI watermark hygiene (text)

Every text artifact that enters the repository must be free of AI provenance marks - invisible Unicode (zero-width, bidi, tag chars, variation selectors, private use) and space homoglyphs. **Images are out of scope: marks in `.png` / `.jpg` / `.pdf` / figures may stay.** The requirement covers only text: sources, scripts, `.md`, `.rql`, `.g4`, CMake, TOML/YAML and commit messages.

Tool: `watermarks-remover` (default `~/github/watermarks-remover`), used through its local scripts - **do not start the Docker/HTTP service for this check**. Layer A only (deterministic Unicode scrub); statistical Layer B rewriting is not part of this rule.

**Mandatory sequence before every commit and before every push.** No commit or push goes out - and no diff is handed over for human review - while the check reports a hit.

The command sequence - staged-file scan, per-file report, cleaning, re-check and re-stage, plus the whole-tree variant for a push and the commit-message check - is in the `watermark-check` skill. Invoke it before committing and before pushing.

**Markdown exception:** The warning icon in `README.md` immediately before `**This is work in progress:**` contains `U+FE0F VARIATION SELECTOR-16`. This exact icon is intentional and must remain unchanged. If a strict scan reports it, verify its location and codepoint; every other reported hit still needs investigation. The default staged-file scan does not flag this emoji.

#### Source code - zero tolerance, strict mode

Documentation can be fixed later; **source code cannot**. A zero-width character or a Cyrillic lookalike inside an identifier, string literal, RQL query or grammar rule compiles, diffs and reviews as normal text, and the resulting failure is practically undebuggable by hand. Nothing may ever introduce such a character into `.cpp` / `.hpp` / `.h` / `.c` / `.g4` / `.rql` / `.desc` / `.sh` / `.py` / `.cmake` / `CMakeLists.txt` / `.toml` / `.yml` / `.json`.

Consequences for the assistant:

- **Never paste model, browser or chat output straight into a source file.** Retype it as ASCII, or clean it before it lands on disk.
- **Check code immediately after editing it** - right after the edit, before `ninja cformat` and before the build, not at commit time. A defect found at push has already been built and tested against.
- Code uses **strict mode**, which the default check does not cover:

```bash
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue <source-file>
```

- On a hit in a source file: **stop and report it to the human** with file, line and codepoint. Do not sweep the file with `--in-place`. The targeted repair is `python3 "$WM/clean_text.py" <file> --aggressive-homoglyphs --strip-emoji-glue -o <file>.fixed`, followed by a `git diff` confirming that only the offending codepoint changed.
- Any `U+00A0` or invisible codepoint in code is a defect, never "informational".

### Commits, push and CI

- **No commit is created without human review - on any branch, `master` and side branches alike.** After verification the assistant shows the diff and stops. The human reads it and gives the go-ahead; only then does `git commit` run. Verification passing is not the go-ahead: green tests say the change works, not that it is the change the human wants in the history.
- **`master` in the code repository** - commits and pushes are performed by the human only.
- **Side branches** - the assistant may run `git commit` locally, but only on an explicit go-ahead for that specific diff, and provided no CI process is triggered. Approval is per diff and does not carry over to the next change.
- Permission to commit on a side branch does not include permission to push, open a pull request, or invoke CI manually. Those actions require an explicit human request.
- If an action would trigger CI, stop and hand it over to the human.

### Session end

Every session ends with either a local commit on a side branch made on an explicit go-ahead, a handoff of the uncommitted diff for human review/commit/push, or an explicit note why no commit was created. No unexplained uncommitted progress is left behind.

**Research gate - mandatory before closing.** Whenever the session touched engine sources (`src/`), run the gate and report its verdict before the commit or the handoff:

```bash
ninja test_gate          # from build/Debug or build/Release
```

It is deliberately outside `ninja` and `ninja test` (see `test/research_gate/README.md`), so nothing runs it implicitly. It is directional: a result worse than the reference is an **error** and stops the work; equal passes; better passes and is recorded. A skipped level (missing or stale H9 ablation profiles) counts as *not run*, never as passed - report it as such. No commit and no handoff goes out with an unreported or failing gate; a red gate is handed to the human, not worked around.

Sessions that touched only tests, scripts or documentation do not need the gate - say explicitly that it was skipped and why.

**Ablation floor - mandatory when the session touched an optimizer pass.** The `RDB_OPT_*` switches must not change what the engine computes, only how fast it gets there. That invariant rots silently: the matrix broke with the `>N` tail rule change of 2026-08-07 and nobody noticed for twelve days, because `manual-ablation` runs only by hand. Whenever the session touched `src/retractor/lib/compiler.cpp`, the startup-latency or tail rules (`SOperations.hpp`, `computeStartupLatency`), or any code behind an `RDB_OPT_*` switch, build the all-off configuration and run the full suite before the commit or the handoff:

```bash
scripts/buildrdb.sh release-ablation     # interactive: set all five switches OFF, probe OFF
ctest --test-dir <katalog wypisany przez skrypt>/test -j 4
```

Success is **the whole suite green** - no failure, and no `DISABLED` beyond the ones the tree already carries. The switches are an efficiency knob, not a semantics knob, and any difference they do show belongs in `def:observable`: `Val` must be equal, `Lat` only non-increasing (see `research_plan.md` §14.20).

One kind of assertion cannot hold without the pass: the one saying that the pass **fired** - a substrate name, a `PUSH_STREAM` target, the absence of a substrate the pass was supposed to absorb. With the switch off that assertion is tautologically false, not red, and it may carry `DISABLED TRUE` guarded by `if(NOT RDB_OPT_...)` and labelled `expected_ablation_failure;requires_<switch>`. **Nothing else may.** An assertion about the computed result - payload bytes, metadata, a value against an oracle - is never disabled: a red `Val` under ablation is a semantics regression or an open finding, and it goes to the human. Never paper one over with `WILL_FAIL` or `DISABLED` - the matrix already carries a note from 2026-07-26 explaining why those annotations were removed.

A test mixing both kinds in one ctest entry has to be split, because a single `DISABLED` then takes the result assertion down together with the shape assertion. `issue202_hash_shift_e2e` is the worked example, split on 2026-09-06 into `-shape` and `-value`: its one `cmp matched CC` pinned `Val` and `Lat` at the same time, so it could never be green under ablation, and disabling it removed the only end-to-end place where the tail divergence between `(A>2)#(B>1)` and `(A#B)>3` was visible at all. CI runs this same floor as `ablation-all-off` in layer L2 of `manual-nightly-full`, which the `cron-shedule` trigger starts on the 5th and 20th of every month, so a skipped local run gets caught at the next of those runs - up to about two weeks later, which is why the local run is not optional.

### Context hygiene

Warn the user when the session shows signs of context degradation:
- more than ~10 back-and-forth exchanges on a single task, or
- the conversation has drifted across multiple unrelated topics, or
- you catch yourself re-asking for information already given earlier in the session.

When any of these occur, say explicitly:
> "Kontekst tej sesji jest długi - rozważ przerwę lub nową sesję od czystego stanu."

Then suggest either: (a) commit current state and end the session, or (b) defer remaining work to a new session with a fresh context.

## ANTLR4 Grammar - Known Pitfalls

Moved next to the code they govern, so they load when those files are in play:

- `src/retractor/lib/CLAUDE.md` - COMMA ambiguity in `select_list`, adding a scalar function, Descriptor field sizes for STRING expressions in SELECT.
- `test/CLAUDE.md` - integration test file sync: cmake copy timing, reconfigure wiping unit-test binaries, installed-binary vs build-copied script.
