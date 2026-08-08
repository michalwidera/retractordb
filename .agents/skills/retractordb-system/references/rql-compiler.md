# RQL and compiler

## Statements and grammar

`RQL.g4` accepts one or more statements:

- `DECLARE fields STREAM id, rational_interval FILE path [DISPOSABLE] [ONESHOT] [HOLD]`
- `SELECT expressions STREAM id FROM stream_expression [FILE path] [RETENTION capacity [segments]] [VOLATILE] [STORAGE profile]`
- `RULE name ON select_stream WHEN logic DO DUMP ...` or `DO SYSTEM ...`
- compiler directives `STORAGE`, `SUBSTRAT`, and `ROTATION`.

Statements may span physical lines when the previous line ends with `\`; `launcher.cpp::readLogicalLines` performs this assembly. Comments are `/* ... */`, `//...`, or `# ` with a required space after `#` so the stream operator remains unambiguous.

Field types: `BYTE`, `INTEGER`, `UINT`, `FLOAT`, `DOUBLE`, `STRING[N]`. Internally descriptors can additionally contain `RATIONAL` and metadata pseudo-fields.

Field-reference parse forms:

| Syntax | Initial token | Meaning after compilation |
|---|---|---|
| `field` | `PUSH_ID3` | resolve by field name across FROM sources |
| `stream.field` | `PUSH_ID1` | resolve named field in named stream |
| `stream[n]` | `PUSH_ID2` | resolve flat offset |
| `stream[_]` | `PUSH_IDX` | replicate expression for every compatible flat element |
| `*` or `stream.*` | `PUSH_TSCAN` | expand an entire schema |

The listener builds postfix token lists for scalar expressions and stream expressions. `command_id` separates scalar operations (`ADD`, comparisons, `CALL`) from stream operations (`STREAM_HASH`, `STREAM_AGSE`, and others).

## Scalar expressions

`expressionEvaluator` executes a postfix stack over `rdb::descFldVT`. It handles numeric/string conversion, arithmetic, comparison, boolean logic, unary negation/NOT, field reads, and function calls.

NULL is `std::monostate` plus payload null metadata. Arithmetic/comparisons generally propagate NULL. Boolean operations implement SQL-like three-valued logic: `true OR null=true`, `false AND null=false`, otherwise the result can remain unknown. `isnull(x)` is the explicit predicate.

Grammar-visible functions include mathematical/conversion functions such as `Sqrt`, `Ceil`, `Abs`, `Floor`, `Sign`, `Chr`, `Length`, `ToNumber`, `ToTimeStamp`, casts, `Count`, `Crc`, `Sum`, `IsZero`, `IsNonZero`, `isnull`, and lowercase `to_integer`, `to_float`, `to_double`, `to_string`.

Important parser invariant: `to_string(expr : width)` uses `:` rather than a comma because ANTLR SLL can confuse commas in a function with commas in the SELECT list. `CALL2` stores `(function,width)` in an `IDXPAIR`; default `to_string` width is 32. String-literal widths and `to_string` widths are accumulated to determine the output descriptor.

## Stream-expression parsing

Precedence from the grammar:

- `#`, `& rational`, `% rational`, `@(step,width)`, and `.aggregator` form stream terms.
- stream `+`, stream `- rational`, and `> integer` compose terms.
- Parentheses explicitly control nested plans.

Multi-operator expressions remain postfix token programs immediately after parsing. The compiler decomposes them into binary/unary generated streams.

## Compiler pipeline

`compiler::compile()` runs this exact order (passes 4, 5, and 8 are `#if RDB_OPT_*` build-switch guarded):

1. `extractIntermediateStreams()` — reduce complex stream expressions into generated `STREAM_*` substrate queries with a maximum of one operator and one/two sources.
2. `expandSchemaWildcards()` — replace `*` with concrete fields and construct schemas for generated operations.
3. `resolveStreamIntervals()` — repeatedly calculate rational deltas until resolved; detect a stalled pass as a circular dependency.
4. `factorMatchedHashTimeMoves()` — rewrite `(A > i) # (B > k)` as `(A # B) > (i + k)` when `i*deltaA == k*deltaB`; preserve shared/user streams conservatively.
5. `deduplicateSubstrats()` — merge only compiler substrates with equal interval, token program, and field shape; rewrite consumer references, including cascades.
6. `resolveFieldReferences()` — turn symbolic `PUSH_ID1..5` into `(stream,index)` `PUSH_ID`.
7. `expandIndexWildcards()` — clone a one-field SELECT expression over the minimum flat size of all `[_]` sources.
8. `shareEquivalentSelectComputations()` — fingerprint explicit SELECT field programs and their FROM trees, canonicalize
   only the two children of each individual `STREAM_ADD`, and move equivalent computation into one generated
   `STREAM_SELECT_*` substrate while retaining each public SELECT as a pass-through stream.
9. `localizeFieldOffsets()` — translate direct and transitive source offsets into the consumer's local input payload layout.
10. `computeLogicalOrigin()` — calculate `query::logicalOrigin`: the index of the first record that exists at all.
    The tail says "not yet"; the origin says "this record has no definition". Only the `@` window stamped by the
    interval end generates origin (`AgseLogicalOrigin()`, because its early records would reach before the source
    start) and `>N` adds `N`; every other operator propagates origin through the same non-decreasing index mapping
    it reads with in `dataModel::constructInputPayload()`, so missing records form a prefix, never holes.
11. `computeStartupLatency()` — calculate causal startup tails for the final plan. Tail slots are not records; `>N`
    produces `max(0,Wsrc-N)` because it reads an older logical index, `#` calls `HashStartupLatency()` to scan one
    reduced phase period exactly (with a safe `O(1)` fallback above the scan limit), `+` takes the per-component
    `AddStartupLatency` maximum, `SUBTRACT` uses `SubtractStartupLatency`, AGSE uses `AgseStartupLatency`, reductions
    add no own tail, and left de-interleave adds one slot. Runs after origin and before capacities because the required
    history depends on the consumer's first emission slot.
12. `computeRequiredCapacities()` — calculate source history for shifts, AGSE, junctions, stream sums, and negative DUMP
    ranges from the event distance between producer availability and consumer reads. A shift by `N` uses
    `rev = Wout-Wsrc+N` and retains `max(rev+1+prefetch,1)` records; the declaration-only `prefetch` is
    `kDeclarationPrefetch=2` (the armed record plus zero prefetch). Do not reduce this rule to `N+1`: logical-index
    addressing removed the old relative-offset cancellation for declarations.
13. `validateConstraints()` — enforce canonical-plan and operator constraints, especially equal flat schema size for `#`.
14. `applyCapacitiesToStreams()` — write computed memory capacities into query storage policies.
15. `topologicalSort()` — unconditionally restore producer-before-consumer execution order after interval sorting and
   all rewrites.

The pipeline invariant after reduction is fewer than four stream-program tokens per query. `dataModel` treats any larger program as a fatal compiler invariant violation.

`extractIntermediateStreams()` reduces each query to a fixed point. This is required for sibling unary expressions such as
`(A > i) # (B > k)`, where extracting only the first shift would otherwise leave a four-token non-canonical program.
The matched-shift rewrite runs after interval resolution because equality of physical shifts depends on the exact rational
source intervals. It runs before structural deduplication so the exposed `A # B` substrate can be shared normally.
`it_issue202_hash_shift_e2e-run` executes the optimized left-hand side and an explicit right-hand side over independent
TEXTSOURCE instances, compares the stored payload/metadata bodies, verifies the complete formula-derived sequence, and
checks the current origin/tail contract. The value sequence and logical origin are equal; after logical-index shift
addressing the factored side may have a strictly shorter tail, so tail equality is not an R1 invariant.

For `STREAM_HASH` with reduced `deltaA/deltaB = p/q`, exact availability depends on which constituent and constituent
position `Hash()` selects in each output slot. `HashStartupLatency()` evaluates the event-distance bound over one
period `0 <= i < p+q`; selection and residue repeat after that period. The scan uses 64-bit products. If the period
exceeds `kHashPhaseScanLimit`, the code falls back to the older phase-safe closed form; it can delay by one slot but
cannot release a record early. `ut_soperations` separately guards exact-branch use, 64-bit arithmetic, and fallback
safety.

`STREAM_TIMEMOVE(N)` is a causal delay, not an advance to `s_(n+N)`. Since the H10a re-timestamping the delay lives
in the logical origin (`origin + N`), while the startup tail is `max(0,Wsrc-N)`. Runtime obtains output logical index
`n` and calls `fetchForward(source,n-N)`; it no longer reads a relative reverse offset. The emitted record sequence is
unchanged and no prefix record is manufactured. Records below the origin never arise; runtime emits no zero or all-null
placeholder for tail or origin slots.

Every rewriting pass snapshots field names of user-named outputs and calls `verifyUserFieldNamesPreserved()` afterward.
Internal substrate names may change, but a rewrite that changes a public `.desc` field name fails compilation.

The K19/K24 event-model audit covers every canonical operator's own tail: `MOD` has own tail 0, `SUBTRACT` uses
`SubtractStartupLatency()` (phase bound, declaration-aware), AGSE uses the closed form `ceil((1+W_src)*F/step)-1` —
the former phase bound `P=floor((|L|-1)/g)*g`, `g=gcd(F,k)`, moved out of the tail into `logicalOrigin` with the
H10a re-timestamping — and reductions operate on the current producer tuple only. `ut_h10aGate` now replays the
independent event model for all nine canonical classes, checks compositions through a recursive oracle, and requires
per-class mutant discrimination and coverage. A read
beyond available history yields an internal all-null failsafe record, but a correctly compiled plan never
materializes it; `it_k19_boundaries` and `it_k24_capacity` guard these boundaries.
The earlier tick-conversion tail/capacity approximations under- or over-sized a large share of corpus plans — do not
reintroduce them.

Both origin and tail passes call `requireResolvedForEveryNode()` before assigning results. An unresolved node is a hard
compiler error rather than a warning followed by the dangerous default value zero. `ut_compiler` covers the direct
gate and a plan spanning all nine operator classes.

SELECT computation sharing runs after symbolic references and `[_]` have been expanded, but before field offsets are
localized to a query's input payload. This lets `FROM a+b` and `FROM b+a` compare the source identities rather than
different local offsets. The FROM fingerprint sorts the two child fingerprints only at each `STREAM_ADD`; it never
reassociates `(a+b)+c` into `a+(b+c)`. Full scans, changed output-field order, unresolved/local references, declarations,
compiler substrates, and singleton equivalence classes remain unchanged. During ad-hoc import the pass is restricted to
new IDs so an already instantiated live query cannot be rewritten underneath its runtime stream instance.

## Generated schemas and intervals

- `#`, `&`, `%`, shift, and subtraction largely preserve the relevant source schema.
- Stream `+` concatenates schemas and generates distinct positional names.
- `.avg/.min/.max/.sumc` yield one `RATIONAL` field. `reductionResultField()` (`query.hpp`) is the single rule shared
  by `query::descriptorFrom` and `streamInstance::reduceFieldsToPayload`: arithmetic sources (`BYTE`/`INTEGER`/`UINT`/
  `RATIONAL`) reduce to a full `RATIONAL` field, so an odd sum no longer truncates through `rational_cast<int>`.
- AGSE yields `abs(width)` fields with the widest source field type/size. Positive width keeps the historical
  newest-field-first convention; negative width mirrors into arrival order.
- Pass-through SELECT copies/expands the source schema, but SELECT expressions can construct a different descriptor.
- Interval formulas (`(Da*Db)/(Da+Db)`, div/mod variants, AGSE `(Ds*k)/F`) are computed in 64-bit `wideRational` and
  range-checked by `narrowInterval()`; an unrepresentable interval throws `std::out_of_range` instead of silently
  overflowing `boost::rational<int>` and surfacing later as an unrelated plan-validation error.

Use `query::descriptorStorage()` for output schema and `query::descriptorFrom(qTree&)` for the concatenated input payload expected by field programs.

## Common compiler failure modes

- A cycle prevents interval resolution and returns `Circular dependency in stream definitions`.
- A `#` between unequal flat schemas fails validation.
- A zero/negative invalid rational or AGSE step reaches a hard validation error.
- A rule cannot attach to a `DECLARE`; it must attach to a `SELECT`.
- Stale `.desc` files can reject a new schema. Without `ROTATION`, startup deliberately removes output artifacts for user SELECT streams, but declared/source descriptors are different lifecycle objects.
- Never compare generated substrate names alone to establish equivalence; `deduplicateSubstrats` compares the program and field shape and ignores field names intentionally.

## Plan inspection

`xretractor -c` parses and compiles without executing. Presenter modes expose query sets, fields, programs, rules, CSV, sequence diagrams, or Graphviz DOT. The most useful dependency view is produced with `-c -d` plus `-f`, `-s`, and optionally `-u`. Plan listings report each query's causal tail as `tail=` and its logical origin as `origin=`.

With `RDB_BENCH_PROBE=ON` and `RDB_BENCH_PLAN` set, the compiler additionally prints a stable stderr line
`REWRITE_APPLIED r1=<n> r2=<n>`: `r1` counts applied `(A > i) # (B > k) -> (A # B) > (i + k)` factorizations and `r2`
counts `STREAM_ADD` nodes whose canonical fingerprint actually swapped child order (not a speedup measure).

Compilation is allowed while another execution instance owns the service lock. Execution remains singleton.
