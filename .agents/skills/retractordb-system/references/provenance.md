# Provenance and freshness

## Indexed revisions

| Repository | Role | Branch | Indexed commit |
|---|---|---|---|
| `retractordb` | implementation, tests, examples, packaging | `master` | `07baa27e129485b68ad8c91e85e3c2aea60630c2` |
| `dokumentacja-rdb` | canonical Polish documentation | `main` | `a93427137cc0f96b7ee9fbdab43715250b55901b` |
| `documentation-rdb` | derived English translation | `main` | `40116d9a32f1eb51d1bd12ad8714992146531969` |

Index prepared on 2026-07-23 in timezone Europe/Warsaw. It was refreshed to code commit `07baa27` after that commit
added the human-only commit/push policy to `CLAUDE.md`; implementation, tests, and observable system behavior are
unchanged from the previously verified commit `48f9b50`. At refresh time all three repositories matched their upstream
branches. Code version from `VERSION`: `0.1.8`.

Run `../scripts/check_freshness.sh` from this directory, or the same script through the installed skill link, before
using the index. The freshness check intentionally accepts either the indexed commit itself or its single direct
descendant. Committing an index update creates one historical commit after the revision recorded inside the index, so
this normal one-commit offset is reported simply as `FRESH`. A divergence or a distance of two or more commits remains
`STALE` and requires inspection.

## Verification baseline for the indexed code revision

- At commit `48f9b50`, the existing Debug build was recompiled with `ninja`.
- At commit `48f9b50`, full `ctest --output-on-failure -j 4` result: **153/153 passed**, 0 failed, 65.54 seconds.
- The first test attempt correctly exposed the documented CMake trap: 38 GTest executables were absent after a prior reconfiguration. This was a build-state issue, not a logic failure; `ninja` recreated 88 targets and the repeated suite passed.
- Test inventory at this revision: 38 `pt_*` compile/file scenarios, 38 `it_*` serial/end-to-end scenarios, and 77 unit-related CTest entries.
- The only change between that verified baseline and indexed commit `07baa27` is the collaboration policy in
  `CLAUDE.md`; no product test was rerun for this documentation-only delta.

## Source hierarchy and scope

The Polish documentation contains about 7,474 Markdown lines across 68 content/configuration Markdown files. The index was built from its table of contents and all major domains: mathematical foundations, RQL construction, architecture, compiler, execution, examples, CLI appendices, and integration-test catalog.

The implementation index covers all hand-written headers and source files under `src/`, both ANTLR grammars, CMake/build/packaging configuration, all test CMake definitions, RQL fixtures, shell drivers, expected patterns, and unit-test names. Generated `.antlr/` files and build/coverage outputs are not knowledge sources.

The English repository is treated as derived content. Consult it only for English terminology or translation work; settle behavioral ambiguity against Polish documentation, tests, and code.

## Known documentation-to-code drift at this revision

These are navigation warnings, not necessarily product defects:

- Several storage chapters still call the metadata class `metaDataStream`; current code uses `rdb::metaData`, with `MetaIndexStore`, `GapDetector`, `IndexRecord`, `metaShadow`, and `storageShadow` extracted into separate units.
- The Polish integration-test appendix omits newer scenarios including `config_storage_validation`, `deinterleave_roundtrip`, `packaging`, and `service_idle`. The live CTest inventory is authoritative.
- Some prose says `xretractor` requires a query file. Current service mode supports no query file / an empty startup file and stays alive in idle mode.
- The root code `CLAUDE.md` summarizes `[storage] dir`, but current `AppConfig` also exposes IPC sizing, client retry count, startup/no-data timing, real-time priority, lock directory, and service query-file fallback.
- Documentation sometimes describes ephemerides as having no files. Conceptually they are not materialized results, but declared external sources can still have/generated `.desc` descriptors; `DEVICE` and `TEXTSOURCE` have inert `.meta` persistence.
- Storage documentation may describe up to four primary files while current shadow-aware metadata also uses `.meta.shadow` to keep null overrides consistent with `.shadow`.
- CLI short options are mode-dependent: for `xretractor`, `-m` is CSV in compile-only option construction and loop limit in execution mode. Verify against `launcher.cpp`, not a single summary table.

When changing documentation, fix only drift relevant to the task unless the user asks for a broader synchronization pass.
