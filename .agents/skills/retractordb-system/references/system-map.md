# System map

## Identity and design intent

RetractorDB is an Edge Signal Processing Engine (ESPE), not a relational database and not a replacement for a central TSDB/DSMS. It processes regular, high-frequency streams close to their sources, reduces and transforms them with a declarative language, preserves inspectable/correctable artifacts, and emits deterministic results upstream.

Its distinguishing synthesis is:

- a differential regular-series model `(s_n, Delta)` with exact rational intervals;
- stream operators derived from Beatty/Fraenkel covering sequences;
- exact DSP-style rate conversion and windowing as first-class query operators;
- a continuous query compiler and deterministic sequential execution model;
- self-describing binary artifacts with null/gap and correction metadata.

The implementation aims for deterministic semantics and predictable sequential execution. It does not claim hard real-time guarantees. Linux `SCHED_FIFO`, `mlockall`, and absolute sleeps are optional runtime measures, not a proof of worst-case execution time.

## Mathematical object and operator families

A regular time series is a sequence of fixed-schema records with one positive rational sampling interval. Two algebras coexist in RQL:

- `SELECT` expressions operate on scalar/field values with a postfix stack program.
- `FROM` expressions operate on whole regular streams and their intervals.

The same symbol can therefore mean different operations. In particular, scalar `+` adds values, while stream `+` synchronizes and concatenates records.

Core stream operators:

| RQL | Formal role | Interval/result invariant |
|---|---|---|
| `A # B` | interleave `phi` | chooses every element of both inputs exactly once; `Delta=(Da*Db)/(Da+Db)`; schemas must have equal flat size |
| `C & Db` | left deinterleave `Theta` | reconstructs one component; `Da=(Dc*Db)/abs(Dc-Db)` |
| `C % Da` | right deinterleave `~Theta` | reconstructs the complementary component; `Db=(Dc*Da)/abs(Dc-Da)` |
| `A + B` | synchronized sum/join `Sigma` | concatenates schemas, repeats the slower side as needed; `Delta=min(Da,Db)` |
| `C - D` | inverse/difference `delta` | selects a component according to rational interval argument; output interval remains source interval |
| `S > n` | sample/time shift `tau` | reads a reverse offset while preserving interval |
| `S@(step,width)` | AGSE `Psi` | outputs `abs(width)` flattened elements; `Delta_out=Delta_source*step/source_flat_width`; negative width mirrors order |
| `S.min/.max/.avg/.sumc` | field reduction | one rational output field; interval unchanged |

The `Hash`, `Div`, `Mod`, `Subtract`, and `agse` index equations live in `src/include/SOperations.hpp`. They use `boost::rational<int>` and integer floor/ceiling functions to avoid floating-point timing drift. `deinterleave_roundtrip` asserts bit-exact reconstruction, including the causal one-slot delay of left deinterleave.

## Executables

| Binary | Implementation | Responsibility |
|---|---|---|
| `xretractor` | `src/retractor/` + `src/retractor/lib/` | parse/compile RQL, build the plan, schedule streams, execute rules, persist results, publish live rows |
| `xqry` | `src/qry/` | multi-instance IPC client: introspection, stream subscription, ad-hoc query, shutdown; raw/Graphite/InfluxDB/Gnuplot renderers |
| `xtrdb` | `src/rdb/` | offline/interactive inspection and mutation of descriptors, binary artifacts, metadata, retention groups, and storage maps |

Libraries:

- `rdb` (`src/rdb/lib`, public headers `src/include/rdb`) owns record schemas, typed payloads, I/O accessors, paths, persistence, metadata, shadows, and retention.
- `retractor` owns RQL parsing, query representations, compiler passes, timeline math, stream evaluation, rules, IPC server, service lifecycle, and presentation.
- `common` contains terminal/logging utilities shared by command-line programs.

## Data categories

- **Ephemerides**: external/read-only input streams declared with `DECLARE`; conceptually volatile and not result materializations.
- **Substrates**: compiler-generated intermediate queries named with `STREAM_*`; their storage policy is controlled by `SUBSTRAT`, commonly `MEMORY` in production.
- **Artifacts**: user-named `SELECT` results, normally materialized and available for inspection or downstream consumption.

`query::isGenerated()` recognizes the `STREAM_` prefix. `query::isSubstrat` is the stronger semantic marker used by compiler deduplication; user queries with equivalent programs are deliberately not merged.

## End-to-end control and data flow

```text
.rql text
  -> ANTLR RQL listener -> qTree of query/field/token/rule
  -> compiler passes -> canonical, topologically ordered plan + buffer capacities
  -> dataModel -> one streamInstance per query
  -> TimeLine selects due stream intervals
  -> declared source prefetch -> FROM payload -> SELECT expressions -> storage write -> RULE actions
  -> artifact files and live IPC broadcasts
  -> xtrdb (offline artifact inspection) / xqry (live client)
```

The execution plan is a `qTree` (`std::vector<query>`) rather than a runtime pointer tree. Dependencies are derived from `PUSH_STREAM` tokens, and topological sorting ensures sources precede consumers.

## Where to inspect a question

| Concern | Primary files |
|---|---|
| RQL syntax/parser | `src/retractor/lib/RQL.g4`, `RQLParser.cpp` |
| Stream algebra equations | `src/include/SOperations.hpp`, `compiler.cpp`, `dataModel.cpp` |
| Query IR/plan | `query.*`, `field.*`, `token.*`, `qTree.*`, `cmdID.hpp` |
| Compiler pipeline | `compiler.cpp` |
| Scheduling/execution | `CRSMath.*`, `executorsm.cpp`, `dataModel.cpp`, `streamInstance.cpp` |
| Scalar semantics/NULL | `expressionEvaluator.cpp`, `convertTypes.cc`, `payload.cc` |
| Storage | `storage.cc`, `accessorFactory.cc`, accessor implementations |
| Metadata/corrections | `metaData.cc`, `metaIndexStore.cc`, `metaShadow.cc`, `storageShadow.cc`, `faccposixshd.cc` |
| Live IPC | `executorsm.cpp`, `ipc_transport.cpp`, `qry.cpp`, `formatters.cpp` |
| Offline inspection | `xtrdbLauncher.cpp`, `xtrdbStorageMap.cpp`, `cmd*.cpp` |
| Service/config/package | `launcher.cpp`, `appConfig.*`, `lockManager.*`, `serviceControl.*`, `packaging/` |
