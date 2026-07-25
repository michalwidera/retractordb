# Provenance and freshness

## Version basis

| Repository | Role | Branch | Version basis |
|---|---|---|---|
| `retractordb` | implementation, tests, examples, packaging | `master` | versioned in the same Git tree as this file |
| `dokumentacja-rdb` | canonical Polish documentation | `main` | `2723e03cabac72fed29eb5e2d82f7275fefd38d6` |
| `documentation-rdb` | derived English translation | `main` | `93f372882ec5c5b5022e72834a5562b743ba441f` |

Index prepared on 2026-07-23 and its external documentation basis refreshed on 2026-07-25 in timezone Europe/Warsaw.
The initial code basis was commit `48f9b50`. Code and the index now live in the same repository and are selected by the
same checkout, so provenance does not embed a mutable current-code commit hash. This avoids a self-referential update in
which committing a new hash immediately makes that hash historical. The initial verified product baseline remains
`48f9b50`; code version from `VERSION` was `0.1.8`.

Run `../scripts/check_freshness.sh` from this directory, or the same script through the installed skill link, before
using the index. For the code repository it reports the checked-out revision as `VERSIONED` without comparing it to an
embedded key. Exact revision checks remain only for the two external documentation repositories.

## Verification baseline for the indexed code revision

- At commit `48f9b50`, the existing Debug build was recompiled with `ninja`.
- At commit `48f9b50`, full `ctest --output-on-failure -j 4` result: **153/153 passed**, 0 failed, 65.54 seconds.
- The first test attempt correctly exposed the documented CMake trap: 38 GTest executables were absent after a prior reconfiguration. This was a build-state issue, not a logic failure; `ninja` recreated 88 targets and the repeated suite passed.
- Test inventory at this revision: 38 `pt_*` compile/file scenarios, 38 `it_*` serial/end-to-end scenarios, and 77 unit-related CTest entries.

## Behavioral changes after the initial baseline

- Issue 202 adds a guarded compiler rewrite for matched interleave shifts:
  `(A > i) # (B > k) -> (A # B) > (i + k)` only when `i*deltaA == k*deltaB`. The extraction pass now reduces
  each postfix program to a fixed point, including the unmatched sibling-shift case. Coverage:
  `pt_issue202_hash_shift_factorization-*`, `ut_soperations`, and the physical/formula E2E comparison
  `it_issue202_hash_shift_e2e-run`. The E2E case also fixes shift-history sizing to `N+1` records for offset `N`.
- Equivalent public SELECTs over commutative `STREAM_ADD` nodes can share one generated `STREAM_SELECT_*` computation
  while retaining their separate public names, descriptors, storage, rules, and artifacts. Fingerprints preserve tree
  grouping and output-field order; they canonicalize only sibling order at each individual `STREAM_ADD`. Full scans and
  differently grouped three-source expressions are negative cases. Coverage: five `xcompiler` unit cases and
  `it_select_cse_commutative_add-run`, including execution, NULL metadata, public descriptors, result-shape guards, and
  ad-hoc import safety. The post-change Debug suite passed 158/158 tests.
- Build-time optimizer ablation switches preserve the default pipeline while allowing substrate deduplication, equivalent
  SELECT sharing, commutative `STREAM_ADD` fingerprints, and matched hash/time-move factorization to be compiled out
  independently. `it_optimizer_ablation-*` verifies build identity, plan shapes, and semantic comparisons. It also records
  two material runtime interactions as `expected_ablation_failure`: without matched-shift factorization,
  `(A>2)#(B>1)` does not produce the same payload as `(A#B)>3`; with factorization, substrate deduplication, and equivalent
  SELECT sharing all disabled, an otherwise equivalent shifted `DA+DB` plan gains one extra zero-valued startup record.
  These are observable ablation results, not test-harness exemptions from unexplained failures. The full Release matrix
  with `RDB_BENCH_PROBE=OFF` matched the expected success-count delta relative to the all-optimizations-enabled baseline
  in every valid configuration; no unexpected discrepancy remained. The user-facing matrix deliberately reports
  relative deltas rather than a fixed test inventory.
- `scripts/buildrdb.sh release` now treats production output as a fail-closed build: it requires a pristine Git tree,
  recreates `build/Release`, sanitizes common flag-injection environment variables, explicitly enables every production
  optimizer and disables `RDB_BENCH_PROBE`, then verifies the resulting binary through `--optimizer-build-info`.
  `release-ablation` and `probe` use separate CMake and Conan output directories and verify their own selected build
  identity, preventing experimental caches or binaries from being written into `build/Release`.
- Model-dependent IPC commands now wait for `dataModel` publication after the IPC resources become available, closing a
  startup race in which an early `xqry get` could receive a response without `db.stream`. The
  `it_issue121_null_propagation-run` regression starts the client before the server with short readiness polling and
  cleans up both children on failure, preventing a stale lock from causing a cascade of unrelated integration failures.

## Source hierarchy and scope

At the indexed Polish documentation commit, the repository contains 74 Markdown files and 7,851 Markdown lines; 71
content files are linked from `SUMMARY.md`. The index covers all major domains: mathematical foundations, RQL
construction, architecture, compiler, execution, examples, CLI appendices, and the integration-test catalog.

The implementation index covers all hand-written headers and source files under `src/`, both ANTLR grammars, CMake/build/packaging configuration, all test CMake definitions, RQL fixtures, shell drivers, expected patterns, and unit-test names. Generated `.antlr/` files and build/coverage outputs are not knowledge sources.

The English repository is treated as derived content. Consult it only for English terminology or translation work; settle behavioral ambiguity against Polish documentation, tests, and code.

## Known documentation-to-code drift at this revision

These are navigation warnings, not necessarily product defects:

- The mathematical documentation defines `tau_m(S)` as the advanced sequence
  `s_(n+m)`, while runtime `STREAM_TIMEMOVE(N)` reads history slot `N` for
  computed streams and the issue-202 E2E fixture expects an initial delayed
  prefix. The optimizer ablation fixture additionally proves that the unfactored
  `(A>2)#(B>1)` runtime plan is not payload-equivalent to `(A#B)>3`, even though
  the factored default plan is. The formal direction, separate input clocks,
  pre-stream boundary extension, and NULL versus zero semantics must be
  reconciled before treating the theorem as a complete proof of the compiler
  rewrite or the unfactored plan as an equivalent executable form.
- Several storage chapters still call the metadata class `metaDataStream`; current code uses `rdb::metaData`, with `MetaIndexStore`, `GapDetector`, `IndexRecord`, `metaShadow`, and `storageShadow` extracted into separate units.
- The Polish integration-test appendix omits newer scenarios including `config_storage_validation`, `deinterleave_roundtrip`, `packaging`, and `service_idle`. The live CTest inventory is authoritative.
- Some prose says `xretractor` requires a query file. Current service mode supports no query file / an empty startup file and stays alive in idle mode.
- The root code `CLAUDE.md` summarizes `[storage] dir`, but current `AppConfig` also exposes IPC sizing, client retry count, startup/no-data timing, real-time priority, lock directory, and service query-file fallback.
- Documentation sometimes describes ephemerides as having no files. Conceptually they are not materialized results, but declared external sources can still have/generated `.desc` descriptors; `DEVICE` and `TEXTSOURCE` have inert `.meta` persistence.
- Storage documentation may describe up to four primary files while current shadow-aware metadata also uses `.meta.shadow` to keep null overrides consistent with `.shadow`.
- CLI short options are mode-dependent: for `xretractor`, `-m` is CSV in compile-only option construction and loop limit in execution mode. Verify against `launcher.cpp`, not a single summary table.

When changing documentation, fix only drift relevant to the task unless the user asks for a broader synchronization pass.
