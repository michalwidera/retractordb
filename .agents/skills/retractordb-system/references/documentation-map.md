# Documentation navigation map

## Authority and use

This file is a routing index, not an additional source of product truth.

- Canonical explanatory documentation: `dokumentacja-rdb` (Polish).
- Derived translation: `documentation-rdb` (English).
- Canonical table of contents: `SUMMARY.md` in each documentation repository.
- Current behavior: live RetractorDB implementation and generated build configuration.
- Observable guarantees: current integration and unit tests.

Documentation can lag behind the implementation, omit a case, retain a historical name, or simply be wrong. A `FRESH`
result from `check_freshness.sh` means only that this map targets the recorded documentation revision. It does not make a
documentation claim authoritative. Use a chapter to understand intent and terminology, then verify the claim in code
and tests before explaining behavior, changing code, or writing a derived artifact.

Default repository layout:

```text
<workspace>/
  retractordb/
  dokumentacja-rdb/
  documentation-rdb/
```

Paths below are relative to the corresponding repository root.

## Fast routing by subject

| Subject | Polish documentation | Verify primarily in |
|---|---|---|
| Product identity, ESPE positioning | `README.md`, `przeglad-rozwiazan.md` | `README.md`, binaries, current examples |
| Mathematical model and proofs | `podstawy-matematyczne/` | `src/include/SOperations.hpp`, `CRSMath.*`, `ut_soperations`, `ut_crsMath` |
| RQL commands and user syntax | `konstrukcja-jezyka-zapytan/` | `RQL.g4`, `RQLParser.cpp`, compiler and integration fixtures |
| Overall architecture and data flow | `architektura-systemu-przetwarzania-danych/` | launchers, `executorsm`, `dataModel`, `streamInstance`, IPC tests |
| Artifacts, substrates, ephemerides | architecture chapter plus `kompilacja-zapytan/substraty.md` | `query.*`, `compiler.cpp`, storage factory, substrate tests |
| Binary storage and metadata | `architektura-systemu-przetwarzania-danych/format-zapisu-danych/` | `src/rdb/lib/`, storage and metadata unit/integration tests |
| Compiler pipeline and plan transformations | `kompilacja-zapytan/` | `compiler.cpp`, `qTree.*`, `query.*`, `ut_compiler`, compile fixtures |
| Runtime scheduling and evaluation | `realizacja-zapytan/` | `executorsm.cpp`, `dataModel.cpp`, `streamInstance.cpp`, `CRSMath.*` |
| AGSE and window semantics | `realizacja-zapytan/ruchome-okno-danych-agse/` | `SOperations.hpp`, evaluator/data model, `agse*`, `Pattern6`, DSP tests |
| RULE and alerting | RQL RULE chapters plus `realizacja-zapytan/realizacja-alarowania.md` | parser, `rule.*`, `dumpManager`, `issue42_rule` |
| Ad-hoc queries and IPC | `realizacja-zapytan/zapytania-ad-hoc.md`, architecture flow | `qry.cpp`, `executorsm.cpp`, `ipc_transport.*`, `issue6_adhoc` |
| Artifact inspection and mutation | architecture analysis/storage chapters | `src/rdb/`, `xtrdbLauncher.*`, storage-map and Pattern5 tests |
| CLI options | `zalaczniki/opcje-wywolania/` | current launcher option construction and CLI tests |
| Integration test catalogue | `zalaczniki/testy-integracyjne.md` | current `test/CMakeLists.txt` and `test/IntegrationTest_*` |
| Signal-processing examples | `przyklady-zastosowan/` | example RQL, DSP integration tests, current generated plans |
| History, naming, future directions | `zalaczniki/geneza-systemu/`, `zalaczniki/dalsze-kierunki-rozwoju/` | historical/design context only |

## Polish documentation inventory

### Entry points and positioning

- `README.md` — introduction to RetractorDB, intended problem domain, high-level capabilities, and reading entry point.
- `przeglad-rozwiazan.md` — comparison and positioning against other data-processing approaches.
- `SUMMARY.md` — authoritative mdBook navigation order.
- `literatura.md` — bibliography and theoretical/technical references.

Use these for product descriptions and articles, but verify all present-day feature statements against the binaries,
configuration, and tests.

### Mathematical foundations

Directory: `podstawy-matematyczne/`

- `README.md` — scope and motivation of the mathematical model.
- `algebra-regularnych-serii-czasowych.md` — regular time-series object, intervals, and the main stream-algebra operators.
- `formalne-podstawy-i-dowody.md` — Beatty/Fraenkel foundations, formal definitions, identities, and proofs.
- `wyrazenia-algebraiczne.md` — construction and interpretation of algebraic stream expressions.
- `implementacja-programowa.md` — translation of the formal model into program algorithms and index calculations.
- `zaleznosci-pomiedzy-operatorami-algebry.md` — graphical relationships and composition of algebra operators.
- `podsumowanie.md` — mathematical conclusions and condensed operator view.

Verification anchors:

- operator/index equations: `src/include/SOperations.hpp`;
- rational time arithmetic: `CRSMath.*`;
- compilation of stream expressions: `compiler.cpp`;
- execution: `dataModel.cpp`, `streamInstance.cpp`;
- tests: `ut_soperations`, `ut_crsMath`, `operations`, `deinterleave_roundtrip`, `agse*`.

### RQL construction

Directory: `konstrukcja-jezyka-zapytan/`

- `README.md` — language structure and command families.
- `polecenie-declare.md` — `DECLARE`, schemas, stream names, intervals, and source declaration.
- `polecenie-declare-opcje-odczytu.md` — source/read options and their effects.
- `polecenie-select/README.md` — `SELECT`, scalar expressions, `STREAM`, and `FROM`.
- `polecenie-select/sekwencjonowanie-operacji-sumowania.md` — grouping/order of synchronized stream sum operations.
- `polecenie-select/sekwencjonowanie-operacji-przeplotu.md` — grouping/order of interleave operations.
- `polecenie-select/klauzula-volatile.md` — `VOLATILE` behavior and materialization intent.
- `polecenie-select/typy-storage.md` — storage choices, including substrate memory storage.
- `polecenie-select/operatory-agregujace.md` — reductions, conversions, `to_string`, and related scalar functions.
- `polecenie-rule.md` — `RULE` syntax and actions.
- `polecenie-rule/alarmowanie.md` — construction of the alerting mechanism.
- `polecenie-rule-warunek-logiczny.md` — logical condition syntax and semantics.
- `polecenie-rule/przyklad-alarmowania.md` — complete alerting example.
- `dyrektywy-konfiguracyjne.md` — RQL-level configuration directives.

Verification anchors:

- grammar: `src/retractor/lib/RQL.g4`;
- listener/parser output: `RQLParser.cpp`;
- command/token identifiers: `src/include/cmdID.hpp`;
- scalar semantics: `expressionEvaluator.cpp`, `convertTypes.cc`;
- storage choice: `compiler.cpp`, `accessorFactory.cc`, `streamInstance.cpp`;
- tests: parser/compiler units plus `Pattern*`, `simple*`, `issue42_rule`, `issue61_tmpmem`, and conversion/null scenarios.

### System architecture

Directory: `architektura-systemu-przetwarzania-danych/`

- `README.md` — architecture chapter entry point.
- `schemat-architektury.md` — overall component view and roles of `xretractor`, `xqry`, and `xtrdb`.
- `przeplyw-danych-i-sterowania.md` — runtime data/control flow, IPC clients, lifecycle, and shutdown.
- `artefakty-substraty-efemerydy.md` — three stream/data categories and intended persistence.
- `kompilacja-i-budowa-planu.md` — compilation invocation, textual/DOT plans, and plan interpretation.
- `przetwarzanie-i-dystrybucja-danych.md` — processing loop, result creation, persistence, and distribution.
- `analiza-artefaktow.md` — offline artifact analysis with `xtrdb`.
- `podsumowanie.md` — architecture summary.

Verification anchors:

- launch and service lifecycle: `src/retractor/launcher.cpp`, `serviceControl.*`, `lockManager.*`;
- plan representation: `qTree.*`, `query.*`, `field.*`, `token.*`;
- execution: `executorsm.cpp`, `dataModel.cpp`, `streamInstance.cpp`;
- IPC: `ipc_transport.*`, `src/qry/`;
- inspection: `src/rdb/`, especially `xtrdbLauncher.*`;
- tests: lifecycle, IPC, ad-hoc, shutdown, plan-presentation, and consistency scenarios.

### Storage format, metadata, retention, and inspection

Directory: `architektura-systemu-przetwarzania-danych/format-zapisu-danych/`

- `readme.md` — storage-format overview and object relationships.
- `pliki.md` — detailed descriptor, data, metadata, index, segment, shadow, and related file layouts.
- `rotacja.md` — file rotation and retention behavior.
- `narzedzie-inspekcji.md` — `xtrdb -s` storage-map inspection.
- `podsumowanie.md` — condensed storage-format conclusions.

Also consult:

- `architektura-systemu-przetwarzania-danych/analiza-artefaktow.md`;
- `zalaczniki/opcje-wywolania/xtrdb.md`.

Verification anchors:

- schema and payload: `Descriptor.*`, `payload.*`;
- storage orchestration: `storage.*`, `storagePaths.*`;
- accessors: `FileInterface` and `facc*` implementations;
- metadata/gaps: `metaData.*`, `metaIndexStore.*`, `gapDetector.*`;
- corrections/shadows: `metaShadow.*`, `storageShadow.*`, `faccposixshd.*`;
- rotation/retention: `fagrp.*`, persistent counters and storage tests;
- tests: storage/accessor units, `issue113_*`, `issue153_storagemap_meta_cases`, `rotation_test`, `retention`, `Pattern5`.

The storage documentation is especially likely to preserve historical class names or omit newer sidecars. Check
`references/storage.md` and `references/provenance.md` before repeating a file-count or class-ownership claim.

### Query compilation

Directory: `kompilacja-zapytan/`

- `README.md` — compiler goals and internal representations.
- `przebiegi-kompilacji.md` — ordered `compiler::compile()` passes and a plan traced through the pipeline.
- `budowa-drzewa-zaleznosci.md` — dependency extraction and plan ordering.
- `substraty.md` — intermediate stream creation, sharing, and deduplication.
- `rozwijanie-symbolu.md` — `SELECT *` schema expansion.
- `rozwiazywanie-interwalow.md` — interval inference across stream expressions.
- `wykrywanie-petli.md` — unresolved/circular dependency detection.
- `aliasowanie.md` — source/result field reference rewriting.
- `przetwarzanie-symbolu-_.md` — indexed wildcard replication.
- `rownanie-typow-w-gore.md` — type propagation/promotion through expressions.
- `debugowanie-kompilacji.md` — textual, CSV, and graph plan diagnostics.

Verification anchors:

- pass order and transformations: current `compiler::compile()` and helper calls in `compiler.cpp`;
- dependency tokens and sorting: `qTree.*`, `query.*`, `token.*`;
- schema/reference handling: `field.*`, parser output, compiler tests;
- tests: `ut_compiler`, `ut_qTree`, compile-only `pt_*`, `issue95_loopInCompile`, `issue96_*`, `issue167_*`.

Never infer the current pass order solely from chapter headings. Compare it with the live `compiler::compile()` body.

### Query execution

Directory: `realizacja-zapytan/`

- `README.md` — runtime responsibilities and chapter entry point.
- `algorytm-przegladu-drzewa-zapytan.md` — plan traversal, timeline decisions, source reads, evaluation, and writes.
- `zapytania-ad-hoc.md` — live ad-hoc query construction and IPC control flow.
- `realizacja-alarowania.md` — runtime evaluation and execution of `RULE` actions.
- `odtwarzanie-strumienia.md` — stream replay behavior.

AGSE directory: `realizacja-zapytan/ruchome-okno-danych-agse/`

- `README.md` — AGSE concept, indexing, and sliding-window role.
- `przyklad-serializacji.md` — serialization/window example.
- `przyklad-sredniej-ruchomej.md` — moving-average construction.
- `rozne-typy-okien.md` — forward/backward windows and rate/width variants.

Verification anchors:

- scheduling and lifecycle: `executorsm.cpp`, `CRSMath.*`;
- due-stream selection and processing: `dataModel.cpp`, `TimeLine` code;
- per-stream payload flow: `streamInstance.cpp`;
- expression evaluation: `expressionEvaluator.cpp`;
- rule actions: rule/dump manager code;
- tests: `ut_dataModel`, `ut_expeval`, `ut_dumpManager`, `agse*`, `issue6_adhoc`, `issue42_rule`, consistency and IPC tests.

### Usage examples

Directory: `przyklady-zastosowan/`

- `README.md` — example index.
- `implementacja-filtru-sygnalowego.md` — FIR-style signal-processing pipeline using windows, indexed multiplication,
  reduction, and stream composition.
- `wizualizacja-ekg-mit-bih.md` — ECG visualization and arrhythmia/QRS-oriented processing based on MIT-BIH data.

Treat examples as explanations of techniques, not automatic confirmation that every command line and generated plan
still matches the current release. Compile their current RQL and use the DSP/example integration tests when reusing them.

### Appendices

Directory: `zalaczniki/`

- `README.md` — appendix index.
- `opcje-wywolania/README.md` — command-line appendix introduction.
- `opcje-wywolania/xretractor.md` — `xretractor` modes and options.
- `opcje-wywolania/xqry.md` — client commands, formats, subscriptions, limits, and control operations.
- `opcje-wywolania/xtrdb.md` — artifact/storage inspection and mutation commands.
- `geneza-systemu/README.md` — system history.
- `geneza-systemu/dlaczego-wybrano-taka-nazwe-dla-systemu.md` — origin of the RetractorDB name.
- `dalsze-kierunki-rozwoju/README.md` — future-work index.
- `dalsze-kierunki-rozwoju/jeszcze-inna-matematyka.md` — exploratory mathematical direction.
- `kolorowanie-skladni/README.md` — RQL syntax-highlighting integrations.
- `testy-integracyjne.md` — narrative catalogue of integration scenarios and their intended guarantees.

CLI documentation must be verified against current option construction in the three launchers. The integration-test
appendix is not a complete inventory; use `test/CMakeLists.txt`, live `ctest -N`, and `references/tests.md`.

## English counterpart map

The English repository is a synchronized derivative used for English terminology and translation work. Resolve
behavioral ambiguity against Polish documentation, then code and tests; code and tests still win.

| Polish path prefix | English path prefix |
|---|---|
| `podstawy-matematyczne/` | `mathematical-foundations/` |
| `konstrukcja-jezyka-zapytan/` | `query-language-construction/` |
| `architektura-systemu-przetwarzania-danych/` | `data-processing-system-architecture/` |
| `kompilacja-zapytan/` | `query-compilation/` |
| `realizacja-zapytan/` | `query-execution/` |
| `przyklady-zastosowan/` | `usage-examples/` |
| `zalaczniki/` | `appendices/` |
| `literatura.md` | `references.md` |

Important renamed English entry points:

- `przeglad-rozwiazan.md` → check the English root/SUMMARY for the current translated placement; do not invent a path.
- `polecenie-select/` → `select-command/`.
- `polecenie-rule/` → `rule-command/`.
- `format-zapisu-danych/` → `data-storage-format/`.
- `ruchome-okno-danych-agse/` → `agse-sliding-window/`.
- `opcje-wywolania/` → `command-line-options/`.

Use the English `SUMMARY.md` as the exact filename authority because translated filenames are semantic translations,
not mechanical substitutions.

## Verification workflow for a documentation claim

1. Locate the explanatory chapter with this map and read enough surrounding context to identify the intended invariant.
2. Search the live repository for the named class, function, token, option, file suffix, or test using `rg`.
3. Follow the implementation path, normally:

   ```text
   RQL/parser -> compiler -> qTree/query -> dataModel/streamInstance -> storage or IPC
   ```

4. Find the unit or integration test that asserts the observable behavior in `references/tests.md` and live CMake.
5. If documentation differs, answer from code/tests and emit a visible `Documentation drift` warning containing the
   documentation path and claim, the current implementation/test evidence, user impact, and the required Polish
   documentation correction.
6. End the task with a reminder to update Polish documentation and synchronize the English derivative. If the current
   task does not authorize documentation edits, report this as follow-up work rather than changing either repository.
7. Record material, previously unknown drift in `references/provenance.md`.
8. Update this map only when document ownership/navigation changes. Update domain references when system behavior
   changes materially; never manufacture a behavioral conclusion from the documentation map itself.
