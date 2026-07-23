# Regression and test map

## Contents

- [Test architecture](#test-architecture)
- [Behavior-to-integration-test index](#behavior-to-integration-test-index)
- [Unit-test ownership map](#unit-test-ownership-map)
- [Standard verification](#standard-verification)
- [Adding a regression](#adding-a-regression)

## Test architecture

At indexed revision, CTest exposes 153 tests:

- `pt_*` (1-38): parallel-safe compile-only, presenter, Valgrind, or offline `xtrdb` scenarios;
- `it_*` (39-76): serial/end-to-end scenarios, especially those using singleton lock or shared IPC;
- unit-related (77-153): GTest binaries, setup fixtures, and data-model comparison.

The serial CMake wrapper detects commands that start the server (`-m`, `-k`, `xqry`, workflow scripts, lock access) and assigns `RUN_SERIAL TRUE`. Some shell-wrapped server tests set it explicitly because CMake cannot see flags inside the script.

Integration fixtures are copied from source `test/` to `build/Debug/test/` at configure time. Patterns normally compare exact output using `cmake -E compare_files --ignore-eol`; other tests assert files, sizes, grep markers, exit status, or bitwise values.

## Behavior-to-integration-test index

### Algebra, compiler, and plans

- Basic SELECT/scalar arithmetic and stream join: `pt_simple-*`, `it_simple-*`, `it_Data-workflow`.
- Full operator workflow (`+`, `#`, `&`, `%`, `-`, shifts): `it_Data-all-operators`.
- Exact `#` scheduling ratio: `it_operations-run`.
- Bit-exact deinterleave inverse and left-side causal delay: `it_deinterleave_roundtrip-run`.
- AGSE direction, mirror, field widths, faster/slower rates: `it_agse1`, `it_agse2`, `it_agse3`, `pt_Pattern6*`.
- FIR pipeline with `@`, `[_]`, multiplication and `.sumc`: `pt_dsp`.
- `.max` reduction and graph: `pt_simple_max-*`, `it_simple_max-*`.
- Nested stream expressions: `pt_subquery`, `pt_subquery-dot`.
- Three/four-argument decomposition: `it_issue167_triarg`.
- Circular dependency rejection: `pt_issue95_loopInCompile-compile`.
- Generated substrate sharing: `pt_issue96_substrat_reference-*`.
- User queries must not be deduplicated: `pt_issue96_no_substrat_reduction-*`.
- Substrate dedup basics, field-name independence, nonzero offsets, cascades: four `it_issue167_dedup_*` scenarios.
- Compiler/presenter documentation graphs: four `pt_issue31_doc-*`.
- Wildcards/unfold/retention: `pt_Pattern3`.
- Identical field names in multiple streams: `pt_Pattern7`.
- Crc parsing/descriptor sizing: `pt_Pattern4`.
- Time shift on compound stream: `pt_issue56_timeshift-compile`, `it_issue56_timeshift-run`.

### NULL, metadata, and persistence

- End-to-end NULL over IPC and rendering: `it_issue113_null_xqry-run`.
- Skip all-null rows: two `it_issue113_null_skip-*`.
- NULL propagation through SELECT: `it_issue121_null_propagation-run`.
- `isnull`: `it_issue121_isnull-run`.
- Numeric/string conversions and descriptor width: two `it_issue128_*`.
- `.meta` internal header/entries/bitsets: `it_issue113_meta_internal`.
- `.meta` creation and `xtrdb` visibility: `pt_issue113_meta*`, `it_issue113_meta_xtrdb`.
- TEXTSOURCE NULL behavior: `pt_issue113_null_txtsrc`, `pt_txtsrc-run`.
- Storage-map cases, metadata, shadow, segments, rotated files: `pt_issue153_storagemap_meta_cases`.
- Interactive mixed-type append/read/update: `pt_Pattern5`.

### Runtime, IPC, lifecycle, deployment

- Source consistency when multiple consumers read one input: `it_consistency`.
- Ad-hoc live query import: `it_issue6_adhoc-run`.
- RULE compile and DUMP/SYSTEM execution: `pt_issue42_rule-compile`, `it_issue42_rule-run`.
- Memory substrates: `pt_issue61_tmpmem-compile`, `it_issue61_tmpmem-run`.
- Exact xqry element limit: `it_xqry_elem_limit-run`.
- Retention operations: `pt_retention-run`.
- Rotation across sessions: `it_rotation_test`.
- Storage config rejects missing/unwritable directories: two `it_config_storage_validation-*`.
- Idle service process and environment-driven service mode: two `it_service_idle-*`.
- Package contents and install/uninstall behavior: `it_packaging`.

## Unit-test ownership map

Use these GTest binaries for focused changes:

| Area | Unit binary |
|---|---|
| exact rational timeline/algebra | `ut_crsMath`, `ut_soperations` |
| parser/compiler/plan | `ut_compiler`, `ut_qTree`, `ut_field`, `ut_presenter` |
| execution/AGSE/rules | `ut_dataModel`, `ut_dumpManager`, `ut_expeval` |
| type system and record layout | `ut_convertTypes`, `ut_descriptor`, `ut_descriptorIO`, `ut_payload` |
| accessor selection | `ut_accessorFactory` |
| backends | `ut_faccbindev`, `ut_faccfs`, `ut_faccmemory`, `ut_faccposix`, `ut_faccposixshd`, `ut_facctxtsrc`, `ut_fagrp` |
| source buffering | `ut_sourceBuffer` |
| null/gap metadata | `ut_bitsetCodec`, `ut_gapDetector`, `ut_metaData`, `ut_metaData_usage` |
| correction shadows | `ut_metaShadow_usage`, `ut_storageShadow_usage` |
| storage integration/paths/rotation | `ut_rdb`, `ut_storagePaths`, `ut_persistentCounter` |
| IPC/client/rendering | `ut_ipc_transport`, `ut_xqry`, `ut_formatter` |
| config/service/RT | `ut_appConfig`, `ut_lockManager`, `ut_executor_rt` |
| CLI utilities | `ut_uxSysTermTools` |

`ut_expeval` is especially dense and is the primary executable specification for scalar types, conversions, function calls, string behavior, errors, NULL propagation, and three-valued logic.

## Standard verification

From `build/Debug`:

```bash
ninja
ctest -R '<focused-regex>' --output-on-failure
ctest --output-on-failure -j 4
```

Unit tests run through Valgrind leak checking and are slower than direct GTest invocation. Keep that when claiming repository-level verification.

After changing C++ used by an integration scenario, install the new binary because integration scripts generally invoke `xretractor`, `xqry`, or `xtrdb` from the install prefix. After changing `.rql`, data, patterns, or shell fixtures, reconfigure to refresh the build copy, then rebuild the unit executables:

```bash
ninja && ninja install && cmake . && ninja
```

Then run focused and full CTest sets. Never interpret “No such file or directory” for a GTest binary after `cmake .` as a product regression; rebuild first.

## Adding a regression

Choose the lowest layer that proves the invariant:

- add a unit case for a pure function/class or error boundary;
- add compile/presenter coverage for grammar and plan shape;
- add serial end-to-end coverage for timing, IPC, persistence lifecycle, or interactions among binaries;
- prefer exact pattern/bitwise verification for the mathematical operators.

When documentation claims a feature is protected by a named test, verify that the current CMake actually registers that test and that its fixture asserts the claimed property.
