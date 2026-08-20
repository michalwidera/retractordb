# Provenance and freshness

## Version basis

| Repository | Role | Branch | Version basis |
|---|---|---|---|
| `retractordb` | implementation, tests, examples, packaging | `master` | versioned in the same Git tree as this file |
| `dokumentacja-rdb` | canonical Polish documentation | `main` | `c154f5cb803b0dd15152ad747c9c1315f271a5b6` |
| `documentation-rdb` | derived English translation | `main` | `8d543c8cbf95ab7cdb41049be3b30163e225bf5b` |
| `paper-arXiv` | DEBS manuscript, research plan, and schedule | `main` | `5f8f28dec026ac2e64dc9a4ef6f662578a210803` |

Index prepared on 2026-07-23; its Polish and English documentation basis was refreshed on 2026-08-20 in timezone
Europe/Warsaw. The synchronized root `README.md` introductions are included in the recorded documentation commits. The
paper basis remains the separately recorded 2026-08-08 revision and is intentionally still reported as stale against
the current paper checkout.
The initial code basis was commit `48f9b50`. Code and the index now live in the same repository and are selected by the
same checkout, so provenance does not embed a mutable current-code commit hash. This avoids a self-referential update in
which committing a new hash immediately makes that hash historical. The initial verified product baseline remains
`48f9b50`; code version from `VERSION` was `0.1.8`.

Run `../scripts/check_freshness.sh` from this directory, or the same script through the installed skill link, before
using the index. For the code repository it reports the checked-out revision as `VERSIONED` without comparing it to an
embedded key. Exact revision checks cover the two external documentation repositories and the paper repository.

## Verification baseline for the indexed code revision

- At commit `48f9b50`, the existing Debug build was recompiled with `ninja`.
- At commit `48f9b50`, full `ctest --output-on-failure -j 4` result: **153/153 passed**, 0 failed, 65.54 seconds.
- The first test attempt correctly exposed the documented CMake trap: 38 GTest executables were absent after a prior reconfiguration. This was a build-state issue, not a logic failure; `ninja` recreated 88 targets and the repeated suite passed.
- Test inventory at this revision: 38 `pt_*` compile/file scenarios, 38 `it_*` serial/end-to-end scenarios, and 77 unit-related CTest entries.
- On 2026-08-08 at checkout `e434f28`, the current Debug tree exposed 183 CTest entries. A missing-unit-binary state was
  repaired with `ninja`; targeted verification then passed for `ut_compiler`, `ut_h10aGate`, `ut_soperations`,
  `it_ecg_qrs-run`, and `it_replay_stability-run` (including their registered setup fixtures). This is a focused
  freshness check, not a claim that the full 183-test suite was rerun for the index-only change.

## Behavioral changes after the initial baseline

- Issue 202 adds a guarded compiler rewrite for matched interleave shifts:
  `(A > i) # (B > k) -> (A # B) > (i + k)` only when `i*deltaA == k*deltaB`. The extraction pass now reduces
  each postfix program to a fixed point, including the unmatched sibling-shift case. Coverage:
  `pt_issue202_hash_shift_factorization-*`, `ut_soperations`, and the physical/formula E2E comparison
  `it_issue202_hash_shift_e2e-run`. The later logical-index shift implementation supersedes the original `N+1`
  capacity shortcut; see the current shift entry below.
- Equivalent public SELECTs over commutative `STREAM_ADD` nodes can share one generated `STREAM_SELECT_*` computation
  while retaining their separate public names, descriptors, storage, rules, and artifacts. Fingerprints preserve tree
  grouping and output-field order; they canonicalize only sibling order at each individual `STREAM_ADD`. Full scans and
  differently grouped three-source expressions are negative cases. Coverage: five `xcompiler` unit cases and
  `it_select_cse_commutative_add-run`, including execution, NULL metadata, public descriptors, result-shape guards, and
  ad-hoc import safety. The post-change Debug suite passed 158/158 tests.
- Build-time optimizer ablation switches preserve the default pipeline while allowing substrate deduplication, equivalent
  SELECT sharing, commutative `STREAM_ADD` fingerprints, and matched hash/time-move factorization to be compiled out
  independently. `it_optimizer_ablation-*` verifies build identity, plan shapes, and semantic comparisons. Two former
  runtime divergences — unequal R1 payload without factorization and an extra zero-valued startup record with structural
  passes disabled — were removed by G1. They are no longer expected ablation outcomes: every valid optimizer
  configuration must preserve the observable result.
- `scripts/buildrdb.sh release` now treats production output as a fail-closed build: it requires a pristine Git tree,
  recreates `build/Release`, sanitizes common flag-injection environment variables, explicitly enables every production
  optimizer and disables `RDB_BENCH_PROBE`, then verifies the resulting binary through `--build-info`.
  `release-ablation` and `probe` use separate CMake and Conan output directories and verify their own selected build
  identity, preventing experimental caches or binaries from being written into `build/Release`.
- Model-dependent IPC commands now wait for `dataModel` publication after the IPC resources become available, closing a
  startup race in which an early `xqry get` could receive a response without `db.stream`. The
  `it_issue121_null_propagation-run` regression starts the client before the server with short readiness polling and
  cleans up both children on failure, preventing a stale lock from causing a cascade of unrelated integration failures.
- G1 replaces startup prefix records with an explicit causal tail `query::startupLatency`. Tail slots are not records;
  `dataModel` emits nothing until the tail expires. `STREAM_TIMEMOVE(N)` is one causal delay convention for declared and
  computed streams, using `fetchBack()` after adding `N` to the tail. Interleave combines converted producer tails with
  its own look-ahead on the second argument; left de-interleave adds one slot and no longer emits an all-null placeholder.
  Compiler rewrites preserve public field names through `verifyUserFieldNamesPreserved()`, and compilation ends with an
  unconditional topological sort. The current Debug inventory is 166/166 passing; the G1 realization record also
  captured 165/165 Debug and Release before the final additional regression.
- Issue 225 (merged as `c4b63a7`) installs the K24 event-model capacity and tail formulas. `computeStartupLatency()`
  now runs before `computeRequiredCapacities()`, and capacities are derived from the event distance between producer
  availability and consumer reads: `AddStartupLatency` (`ceil((1+W_src)*Ds/Dout)-1` per `STREAM_ADD` component),
  `AgseStartupLatency` (`ceil((P+(1+W_src)*F)/k)-1` with phase bound `P=floor((|L|-1)/g)*g`, `g=gcd(F,k)` — the
  phase bound was later moved out of the tail into the logical origin by the H10a re-timestamping; see the Issue 227
  entry below), and
  `SubtractStartupLatency` replace the earlier tick-conversion approximations that under- or over-sized tails for a
  large share of corpus plans. `STREAM_ADD` entered the capacity model; declared sources get a fixed
  `kDeclarationPrefetch = 2` front allowance (armed record plus zero prefetch); wrong capacities previously produced
  silent all-NULL reads or `storage::revRead` aborts. Reduction output for arithmetic sources is now always `RATIONAL`
  through `reductionResultField()` shared by `query::descriptorFrom` and `streamInstance::reduceFieldsToPayload`,
  fixing rational truncation such as `2000003/2` becoming `1000001/1`. Interval arithmetic in
  `resolveStreamIntervals()` is computed in 64-bit `wideRational` and range-checked by `narrowInterval()` instead of
  silently overflowing `boost::rational<int>`. New regressions: `it_k19_boundaries` (operator tail/observability
  boundaries, real NULL inside a full AGSE window versus the all-null failsafe) and `it_k24_capacity` (declaration
  history depths). The Debug inventory is now 181 tests: `pt_*` 1-41, `it_*` 42-98, unit-related 99-181.
- Issue 227 (merged as `5f31051`, H10a re-timestamping) moves the join-alignment delay out of startup tails
  into a new logical origin. The `@` window is now stamped by the interval END; its early records would reach before
  the source start, so the window span generates `query::logicalOrigin` through `AgseLogicalOrigin()`
  (`ceil((sourceOrigin*F + |L| - 1)/step)`) instead of inflating the tail, and `AgseStartupLatency` loses its phase
  bound, becoming `ceil((1+W_src)*F/step)-1`. A new compiler pass `computeLogicalOrigin()` runs between
  `localizeFieldOffsets()` and `computeStartupLatency()`; every other operator propagates origin through the same
  non-decreasing index mapping it reads with. The presenter reports `origin=` alongside `tail=`. Ad-hoc import now
  publishes the compiled tree and its stream instances atomically under `core_mutex`, bumps `adHocPlanRevision`, and
  the execution loop rebuilds the `TimeLine` via `updateTimeIntervals()` without rewinding when an ad-hoc query
  introduces a new rate; an ad-hoc SELECT starts at the first slot the runtime sees it, not at the system origin.
  New regressions: `it_issue227_join_alignment-run`/`-adhoc-origin` (join-content expectations derived from operator
  definitions, not engine output). The first realization kept `STREAM_TIMEMOVE(N)` tail equal to the producer tail;
  commits after `db4a360` replaced that conservative result with the exact rule below.
- Logical-index shift addressing (`fcc5a44`) makes `STREAM_TIMEMOVE(N)` read `fetchForward(source,n-N)`. Its origin is
  `Osrc+N`, its exact tail is `max(0,Wsrc-N)`, and shift capacity is derived from
  `Wout-Wsrc+N+1` plus the declaration-only two-record prefetch allowance. R1 preserves value sequence and origin but
  may strictly shorten the factored tail; current regressions compare the common value prefix rather than demanding
  tail equality.
- Exact interleave tail (`34db1a2`) scans one reduced phase period through `HashStartupLatency()`. The scan is exact up
  to `kHashPhaseScanLimit=100000`; above that limit the old closed form is retained as a safe over-approximation.
  `ut_soperations` guards exact-branch use, 64-bit arithmetic, and fallback safety.
- System-test armour (`7dcf6d9`, `5446bd9`) extends `ut_h10aGate` from the `@`/`>` gate to all nine canonical operator
  classes and mixed compositions, adds mutants and per-class coverage floors, makes unresolved origin/tail nodes a hard
  compiler error through `requireResolvedForEveryNode()`, and adds `it_ecg_qrs-run` plus
  `it_replay_stability-run`. The live Debug inventory is 183 tests: `pt_*` 1-41, `it_*` 42-100, and unit-related
  101-183.
- The F9/K23 compiler guard (`530c80e`, completed for every grammar-visible named form by `ae530d6`) rejects source
  aliases that reach a constituent through interleave `#`. Interleave has one shared schema, so `A[k]` and `B[k]`
  cannot identify different current values after `A#B`; previously expressions such as `A[0]-B[0]` and
  `A[_]-B[_]` silently collapsed to `result[k]-result[k]`. The guard covers numeric indices, named and bare fields,
  `[_]`, qualified `A.*`, `RULE` conditions, and transitive generated substrates. Positive controls retain aliases
  across `+` and references through the interleave result's own name. Coverage lives in `ut_compiler`; Debug passed
  186/186 tests after the completed guard.
- `retractordb.code-workspace` (`e434f28`) opens code, both documentation repositories, the paper, and experiments as
  independent sibling repositories. It is not a version manifest.

## Source hierarchy and scope

At the indexed Polish documentation commit, the repository contains 75 Markdown files and 8,548 Markdown lines; 72
content files are linked from `SUMMARY.md`. The index covers all major domains: mathematical foundations, RQL
construction, architecture, compiler, execution, examples, CLI appendices, and the integration-test catalog.

The implementation index covers all hand-written headers and source files under `src/`, both ANTLR grammars, CMake/build/packaging configuration, all test CMake definitions, RQL fixtures, shell drivers, expected patterns, and unit-test names. Generated `.antlr/` files and build/coverage outputs are not knowledge sources.

The English repository is treated as derived content. Consult it only for English terminology or translation work; settle behavioral ambiguity against Polish documentation, tests, and code.

## Known documentation-to-code drift at this revision

These are navigation warnings, not necessarily product defects:

- The root code `CLAUDE.md` summarizes only `[storage] dir`, while current `AppConfig` and both CLI documentation sets
  also expose IPC sizing, client retry count, startup/no-data timing, real-time priority, lock directory, and service
  query-file fallback.
- CLI short options are mode-dependent: for `xretractor`, `-m` is CSV in compile-only option construction and loop limit
  in execution mode. Verify against `launcher.cpp`, not a single summary table.

Resolved at the current documentation basis: logical origin and exact shift/AGSE semantics; exact interleave phase scan
with the safe fallback; the `metaData`/`MetaIndexStore`/`GapDetector`/shadow decomposition; five-file artifact families;
declared-source descriptors and inert metadata; idle service startup; full TOML option documentation; and the previously
missing `config_storage_validation`, `deinterleave_roundtrip`, `packaging`, and `service_idle` appendix entries. The
compiler-pass chapters now give the logical-index shift capacity as `Wout-Wsrc+N+1` plus declaration prefetch. The
operator-observability chapters and both integration-test catalogues now cover the full nine-class `ut_h10aGate` scope
and the `ecg_qrs` and `replay_stability` scenarios.

Paper-specific inconsistencies and stale planning metadata are listed in `paper-debs.md`; they are not documentation
authority and must not be silently promoted into system behavior.

When changing documentation, fix only drift relevant to the task unless the user asks for a broader synchronization pass.
