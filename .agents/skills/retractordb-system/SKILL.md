---
name: retractordb-system
description: "Indexed, versioned knowledge of the RetractorDB Edge Signal Processing Engine: its mathematical model, RQL grammar and compiler, execution engine, storage formats, IPC clients, service lifecycle, documentation, and regression tests. Use whenever Codex reads, explains, changes, reviews, tests, or writes about RetractorDB code, Polish/English documentation, examples, experiments, papers, or derived artifacts."
---

# RetractorDB system knowledge

Use this skill as the navigation layer for RetractorDB work. Treat the repositories as the source of truth and the bundled references as a versioned index that prevents repeated rediscovery.

## Start every RetractorDB task

1. Run `scripts/check_freshness.sh`. The script discovers repositories cloned as siblings of the code repository.
   For a different layout, pass the three repository paths as arguments or set `RETRACTORDB_CODE_REPO`,
   `RETRACTORDB_DOCS_PL_REPO`, and `RETRACTORDB_DOCS_EN_REPO`. Code and its index are versioned together, so the script
   reports the current code revision without comparing it to a hash embedded in the index.
2. If both documentation repositories are `FRESH`, load only the references relevant to the request.
3. If a documentation repository is `STALE`, inspect `git diff <indexed-commit>..HEAD` for the affected areas before
   relying on the notes. For code, inspect current changes directly with `git status`, `git log`, and `git diff`.
   Update the notes only when a change materially alters system behavior.
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
5. These indexed notes.

Never edit generated ANTLR files manually. Edit `RQL.g4` or `DESC.g4` and regenerate.

## Load references by task

- For a general explanation, architecture decision, article, or system positioning, read `references/system-map.md`.
- For RQL syntax, algebraic operators, parser behavior, compiler passes, field references, or plan optimization, read `references/rql-compiler.md`.
- For scheduling, stream evaluation, AGSE, rules, IPC, ad-hoc queries, service mode, or configuration, read `references/runtime-ipc.md`.
- For descriptors, payloads, accessors, retention, files, null/gap metadata, shadows, rotation, or `xtrdb`, read `references/storage.md`.
- For any code change or behavioral claim, read `references/tests.md` and identify the guarding tests.
- For versions, validation status, source locations, and known documentation drift, read `references/provenance.md`.

Use `rg` in the live repositories to verify identifiers and line locations. The notes describe ownership and invariants, not stable line numbers.

## Work method

1. State the system invariant or externally visible behavior involved.
2. Trace it through `RQL/parser -> compiler -> qTree/query -> dataModel/streamInstance -> storage or IPC`.
3. Identify existing unit and integration coverage before changing code.
4. Make the smallest change consistent with the repository policy.
5. Rebuild after any CMake reconfiguration, then run focused tests and the appropriate regression set.
6. Update the relevant reference and `references/provenance.md` when system behavior or the external documentation
   basis changes materially. Do not update provenance solely because the code repository received a new commit.

## Important test trap

`cmake .` recopies `test/` and removes built unit-test executables. After reconfiguration, run `ninja` before `ctest`. Integration tests use installed binaries plus build-copied scripts, so changes spanning C++ and integration fixtures usually require:

```bash
ninja && ninja install && cmake . && ninja && ctest
```
