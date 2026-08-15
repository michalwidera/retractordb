---
name: retractordb-system
description: "Indexed, versioned knowledge of the RetractorDB Edge Signal Processing Engine: its mathematical model, RQL grammar and compiler, execution engine, storage formats, IPC clients, service lifecycle, documentation, and regression tests. Use whenever Codex reads, explains, changes, reviews, tests, or writes about RetractorDB code, Polish/English documentation, examples, experiments, papers, or derived artifacts."
---

# RetractorDB system knowledge

Use this skill as the navigation layer for RetractorDB work. Treat the repositories as the source of truth and the bundled references as a versioned index that prevents repeated rediscovery.

## Start every RetractorDB task

1. Run `scripts/check_freshness.sh`. The script discovers repositories cloned as siblings of the code repository.
   For a different layout, pass the four repository paths as arguments or set `RETRACTORDB_CODE_REPO`,
   `RETRACTORDB_DOCS_PL_REPO`, `RETRACTORDB_DOCS_EN_REPO`, and `RETRACTORDB_PAPER_REPO`. Code and its index are
   versioned together, so the script reports the current code revision without comparing it to a hash embedded in the
   index.
2. If both documentation repositories are `FRESH`, load only the references relevant to the request. For article work,
   also require the `paper` repository to be `FRESH` before relying on `references/paper-debs.md`.
3. If a documentation or paper repository is `STALE`, inspect `git diff <indexed-commit>..HEAD` for the affected areas
   before relying on the notes. For code, inspect current changes directly with `git status`, `git log`, and `git diff`.
   Update the notes only when a change materially alters system behavior, documentation ownership, or a paper claim.
4. Read the applicable repository's `AGENTS.md` and `CLAUDE.md` for build, editing, and documentation rules. Apply its repository rules; treat Claude-product-specific model-switch instructions as non-applicable to Codex.
5. Check `git status` before edits. Preserve user changes.

## Source precedence

Resolve conflicts in this order:

1. Current implementation and generated build configuration.
2. Integration and unit tests that assert observable behavior.
3. Canonical Polish documentation from the repository selected by `check_freshness.sh` (by default the
   `dokumentacja-rdb` sibling of the code repository).
4. English documentation from the `documentation-rdb` repository selected by the same script; it is a synchronized
   derivative of Polish documentation.
5. The DEBS manuscript and planning files from `paper-arXiv` are authoritative only for the paper's current wording,
   research status, and submission plan; they are never evidence of engine behavior.
6. These indexed notes.

Documentation is explanatory context, not evidence that current code behaves as described. A `FRESH` documentation
revision only means that the navigation index points at that revision; it does not certify correctness or completeness.
Verify every behavioral claim against the live implementation and its tests. When documentation and code disagree,
follow the code and record material drift in `references/provenance.md`.

## Documentation drift warnings

Never silently pass over a documentation-to-code mismatch. When one is found:

1. Base the answer or implementation on current code and tests.
2. Emit a visible `Documentation drift` warning that identifies:
   - the documentation file and the stale, incomplete, or incorrect claim;
   - the current code/test behavior and its source locations;
   - the practical impact on users or maintainers;
   - the Polish documentation pages that should be updated.
3. End the task with a reminder to update canonical Polish documentation and then synchronize the derived English
   documentation. Do not treat the English translation as the place to originate a behavioral correction.
4. Add a concise entry to `references/provenance.md` when the drift is material and not already recorded.

Use this compact report shape:

```text
Documentation drift
- Documentation: <path and claim>
- Code/tests: <current behavior and evidence>
- Impact: <why the difference matters>
- Documentation update: <Polish pages to change, then synchronize English>
```

Also emit a documentation-update reminder when a code change alters behavior described by an existing chapter, even if
that chapter was correct before the change. A warning or reminder does not authorize edits in a documentation
repository unless the user requested those edits; report the required follow-up instead.

Never edit generated ANTLR files manually. Edit `RQL.g4` or `DESC.g4` and regenerate.

## Load references by task

- For a general explanation, architecture decision, or system positioning, read `references/system-map.md`.
- For the DEBS manuscript, research plan, schedule, page budget, contribution framing, or cross-repository paper work,
  read `references/paper-debs.md` and then the relevant system references.
- For RQL syntax, algebraic operators, parser behavior, compiler passes, field references, or plan optimization, read `references/rql-compiler.md`.
- For scheduling, stream evaluation, AGSE, rules, IPC, ad-hoc queries, service mode, or configuration, read `references/runtime-ipc.md`.
- For descriptors, payloads, accessors, retention, files, null/gap metadata, shadows, rotation, or `xtrdb`, read `references/storage.md`.
- For any code change or behavioral claim, read `references/tests.md` and identify the guarding tests.
- For versions, validation status, source locations, and known documentation drift, read `references/provenance.md`.
- For locating a subject in the Polish documentation, finding its English counterpart, or choosing code/test anchors
  that verify a chapter, read `references/documentation-map.md`.

Use `rg` in the live repositories to verify identifiers and line locations. The notes describe ownership and invariants, not stable line numbers.

## Work method

1. State the system invariant or externally visible behavior involved.
2. Trace it through `RQL/parser -> compiler -> qTree/query -> dataModel/streamInstance -> storage or IPC`.
3. Identify existing unit and integration coverage before changing code.
4. Make the smallest change consistent with the repository policy.
5. Rebuild after any CMake reconfiguration, then run focused tests and the appropriate regression set.
6. Update the relevant reference and `references/provenance.md` when system behavior or the external documentation
   basis changes materially. Do not update provenance solely because the code repository received a new commit.

## Text watermark hygiene before anything enters a repository

Text delivered to `retractordb` or `paper-arXiv` must carry no AI provenance marks: invisible Unicode
(zero-width, bidi, tag characters, variation selectors, private use) and space homoglyphs. Marks inside images
(`.png`, `.jpg`, `.pdf`, figures) are explicitly out of scope and may stay. The rule covers sources, scripts,
Markdown, LaTeX, RQL, grammars, build files and commit messages.

Use the local `watermarks-remover` scripts (default `~/github/watermarks-remover/service/scripts`). Do not
start its Docker or HTTP service for this check — the CLI path is the required one. Layer A only.

```bash
WM="${WATERMARKS_REMOVER:-$HOME/github/watermarks-remover}/service/scripts"
TEXT='\.(md|txt|tex|bib|rql|desc|cpp|hpp|h|c|g4|sh|py|ya?ml|toml|json|cmake|in)$|CMakeLists\.txt$'

git diff --cached --name-only --diff-filter=ACM | grep -E "$TEXT" \
  | while read -r f; do python3 "$WM/inspect_text.py" --json "$f" >/dev/null 2>&1 || echo "WATERMARK: $f"; done

python3 "$WM/inspect_text.py" <file>                                     # report
python3 "$WM/clean_text.py" <file> --in-place --stats && rm -f <file>.bak # clean, drop backup
python3 "$WM/inspect_text.py" --json <file> >/dev/null && git add <file>  # verify, re-stage
```

Swap `git ls-files` for the staged-file listing to audit the whole tracked tree before a push.

Source code is zero tolerance and uses strict mode. A zero-width character or a Cyrillic lookalike inside an
identifier, string literal, RQL query or grammar rule compiles and reviews as ordinary text, and the failure
it causes cannot realistically be found by hand. Never paste model, browser or chat output straight into
`.cpp`, `.hpp`, `.h`, `.c`, `.g4`, `.rql`, `.desc`, `.sh`, `.py`, `.cmake`, `CMakeLists.txt`, `.toml`, `.yml`
or `.json`; retype it as ASCII. Check a source file immediately after editing it — before `ninja cformat` and
before the build, not at commit time:

```bash
python3 "$WM/inspect_text.py" --aggressive --strip-emoji-glue <source-file>
```

The default check misses Latin/Cyrillic confusables — `int value = 1;` whose `a` is a Cyrillic `U+0430`
instead of ASCII `a` passes it — so code needs `--aggressive`; name such a codepoint in prose, never paste the
character itself into a rule file, comment or test. `--strip-emoji-glue` rejects the invisibles that are legitimate in prose but never in code.
Strict mode reports zero hits across the current `src/` and `scripts/` tree and leaves Polish diacritics in
comments alone. On a hit, stop and report file, line and codepoint to the human rather than sweeping the file
with `--in-place`; any `U+00A0` or invisible codepoint in code is a defect, not an informational finding.

Never point `clean_text.py` at binary fixtures (`test/**/*.dat`, `.meta`, `.shadow`, ECG records, `examples/**`
data): it rewrites bytes and integration tests compare output byte-exactly. Keep the extension filter; never
use `--force-text`. Report a hit instead of editing generated ANTLR files or test fixtures.

The same rule governs `dokumentacja-rdb` and `documentation-rdb`; each states it in its own `CLAUDE.md`. One
baseline exception there: the documented callout convention writes `U+2139` / `U+26A0` followed by
`U+FE0F VARIATION SELECTOR-16`, which the scanner reports because those symbols are text-default. That pair is
the convention, not a watermark — leave it in the roughly one dozen affected `.md` files and in
`migrate_to_mdbook.py`. Every other reported codepoint there is a real finding.

For `paper-arXiv`, this hygiene rule operates on codepoints only. It never overrides that repository's ACM
generative-AI disclosure requirement: the disclosure of AI use stays in the document, in its designated
location and wording, and stripping marks is not a substitute for it. After cleaning, `git diff` must show no
visible prose change — otherwise revert and clean again.

## Important test trap

`cmake .` recopies `test/` and removes built unit-test executables. After reconfiguration, run `ninja` before `ctest`. Integration tests use installed binaries plus build-copied scripts, so changes spanning C++ and integration fixtures usually require:

```bash
ninja && ninja install && cmake . && ninja && ctest
```
