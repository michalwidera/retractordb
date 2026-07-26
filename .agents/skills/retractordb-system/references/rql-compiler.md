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

`compiler::compile()` runs this exact order:

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
10. `computeRequiredCapacities()` — calculate source history for shifts, AGSE, junctions, and negative DUMP ranges; a shift
   by `N` addresses history slot `N` and therefore requires capacity `N+1`.
11. `validateConstraints()` — enforce canonical-plan and operator constraints, especially equal flat schema size for `#`.
12. `applyCapacitiesToStreams()` — write computed memory capacities into query storage policies.
13. `computeStartupLatency()` — calculate causal startup tails for the final plan. Tail slots are not records; `>N` adds
   `N`, `#` combines converted input tails with its own look-ahead on the second argument, `+` takes the maximum,
   and left de-interleave adds one slot.
14. `topologicalSort()` — unconditionally restore producer-before-consumer execution order after interval sorting and
   all rewrites.

The pipeline invariant after reduction is fewer than four stream-program tokens per query. `dataModel` treats any larger program as a fatal compiler invariant violation.

`extractIntermediateStreams()` reduces each query to a fixed point. This is required for sibling unary expressions such as
`(A > i) # (B > k)`, where extracting only the first shift would otherwise leave a four-token non-canonical program.
The matched-shift rewrite runs after interval resolution because equality of physical shifts depends on the exact rational
source intervals. It runs before structural deduplication so the exposed `A # B` substrate can be shared normally.
`it_issue202_hash_shift_e2e-run` executes the optimized left-hand side and an explicit right-hand side over independent
TEXTSOURCE instances, compares the stored payload/metadata bodies, verifies the complete formula-derived sequence, and
checks equal startup tails.

For `STREAM_HASH` with reduced `deltaA/deltaB = p/q`, the own startup tail protects the worst second-input phase, not
only `B[0]`. The phase definition is
`max_{0 <= j < p}(ceil((j+1)q/p) - floor(jq/p))`; `computeStartupLatency()` uses its equivalent closed form
`ceil((p+q-1)/p)` with a 64-bit intermediate. This boundary is required for non-rewritten R1 plans at ratios such as
`3/5`, `3/2`, `7/11`, and `160/147`.

`STREAM_TIMEMOVE(N)` is a causal delay, not an advance to `s_(n+N)`. It leaves the emitted record sequence intact,
increases the startup tail by `N`, and reads history slot `N` only after that tail has elapsed. Declared and computed
sources share the same `fetchBack()` path. Runtime emits no zero or all-null placeholder for a tail slot.

Every rewriting pass snapshots field names of user-named outputs and calls `verifyUserFieldNamesPreserved()` afterward.
Internal substrate names may change, but a rewrite that changes a public `.desc` field name fails compilation.

Current audit boundary: `STREAM_DEHASH_MOD`, `STREAM_SUBTRACT`, `STREAM_AGSE`, and reductions propagate producer tails
but have no separately derived own-tail term. Do not generalize the proven R1 boundary semantics to those operators
without completing the index/look-ahead audit and adding regressions.

SELECT computation sharing runs after symbolic references and `[_]` have been expanded, but before field offsets are
localized to a query's input payload. This lets `FROM a+b` and `FROM b+a` compare the source identities rather than
different local offsets. The FROM fingerprint sorts the two child fingerprints only at each `STREAM_ADD`; it never
reassociates `(a+b)+c` into `a+(b+c)`. Full scans, changed output-field order, unresolved/local references, declarations,
compiler substrates, and singleton equivalence classes remain unchanged. During ad-hoc import the pass is restricted to
new IDs so an already instantiated live query cannot be rewritten underneath its runtime stream instance.

## Generated schemas and intervals

- `#`, `&`, `%`, shift, and subtraction largely preserve the relevant source schema.
- Stream `+` concatenates schemas and generates distinct positional names.
- `.avg/.min/.max/.sumc` yield one `RATIONAL` field.
- AGSE yields `abs(width)` fields with the widest source field type/size.
- Pass-through SELECT copies/expands the source schema, but SELECT expressions can construct a different descriptor.

Use `query::descriptorStorage()` for output schema and `query::descriptorFrom(qTree&)` for the concatenated input payload expected by field programs.

## Common compiler failure modes

- A cycle prevents interval resolution and returns `Circular dependency in stream definitions`.
- A `#` between unequal flat schemas fails validation.
- A zero/negative invalid rational or AGSE step reaches a hard validation error.
- A rule cannot attach to a `DECLARE`; it must attach to a `SELECT`.
- Stale `.desc` files can reject a new schema. Without `ROTATION`, startup deliberately removes output artifacts for user SELECT streams, but declared/source descriptors are different lifecycle objects.
- Never compare generated substrate names alone to establish equivalence; `deduplicateSubstrats` compares the program and field shape and ignores field names intentionally.

## Plan inspection

`xretractor -c` parses and compiles without executing. Presenter modes expose query sets, fields, programs, rules, CSV, sequence diagrams, or Graphviz DOT. The most useful dependency view is produced with `-c -d` plus `-f`, `-s`, and optionally `-u`.

Compilation is allowed while another execution instance owns the service lock. Execution remains singleton.
