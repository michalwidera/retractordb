# Storage, descriptors, metadata, and xtrdb

## Contents

- [Descriptor and payload](#descriptor-and-payload)
- [Storage coordinator and source buffer](#storage-coordinator-and-source-buffer)
- [Accessor matrix](#accessor-matrix)
- [Artifact file family](#artifact-file-family)
- [Metadata format and invariants](#metadata-format-and-invariants)
- [Gaps](#gaps)
- [Shadows and correction](#shadows-and-correction)
- [Retention and rotation](#retention-and-rotation)
- [xtrdb](#xtrdb)

## Descriptor and payload

`rdb::Descriptor` extends a vector of `rField` and defines fixed record layout:

- field name;
- byte length;
- array multiplicity;
- `descFld` type.

Data types include byte, signed/unsigned integer, rational, float, double, fixed string, pairs used by the IR, and NULL. Metadata descriptor fields encode `TYPE`, `REF`, `RETENTION`, and `RETMEMORY`; they do not occupy record bytes.

Descriptors are persisted as `.desc` using `DESC.g4` and `DESCParser.cc`. `descriptorIO` creates or validates descriptor files. A mismatched existing descriptor is an intentional hard error.

`rdb::payload` owns a byte buffer shaped by a descriptor plus one null bit per descriptor field. It offers:

- typed item access through `std::any` and the faster `descFldVT` variant interface;
- flat indexing over arrays;
- conversions and stream I/O;
- concatenation with null preservation.

NULL has two layers: deterministic fallback bytes in the record buffer and authoritative null bits in metadata/transport. Never infer null from a numeric zero or empty bytes.

## Storage coordinator and source buffer

`rdb::storage` binds:

- `StoragePaths`;
- a descriptor and active payload;
- a polymorphic `FileInterface`;
- `sourceBuffer` history/chamber state;
- a polymorphic metadata index.

Logical record positions are converted to byte offsets using descriptor record size. Appends and updates are distinguished by position `max<size_t>` versus a concrete position.

Declared sources use the `sourceBuffer` state machine (`empty`, `flux`, `armed`) for prefetch and history. `HOLD` delays physical reads until a consumer releases the hold. `ONESHOT` disables source looping. `DISPOSABLE` removes the complete storage file set on destruction after first abandoning metadata persistence so the metadata destructor cannot recreate a deleted file.

## Accessor matrix

`accessorFactory.cc` is the authoritative mapping:

| Profile | Implementation | Semantics |
|---|---|---|
| `DEFAULT` | `groupFile<posixBinaryFileWithShadow>` | segmented retention, non-destructive data updates |
| `DIRECT` | `groupFile<posixBinaryFile>` | segmented retention, direct updates |
| `MEMORY` | `memoryFile` | in-memory storage with capacity/retention |
| `POSIX` | `posixBinaryFile` | POSIX binary file |
| `POSIXSHD` | `posixBinaryFileWithShadow` | POSIX main file plus correction shadow |
| `GENERIC` | `genericBinaryFile` | standard C++ binary file access |
| `DEVICE` | `binaryDeviceRO` | read-only binary device/file source |
| `TEXTSOURCE` | `textSourceRO` | read-only whitespace text parser, including `NULL` tokens |

Declared types are `DEVICE` and `TEXTSOURCE`. File suffix detection and query policy decide which profile reaches the factory; verify the descriptor rather than assuming from a filename.

`FileInterface` transports both data bytes and a per-record null bitset. `hasShadow()` is independent from whether a metadata index exists.

## Artifact file family

For base name `stream`:

- `stream`: dense, headerless, fixed-record binary data;
- `stream.desc`: self-describing schema and storage policy;
- `stream.meta`: RLE null/gap index;
- `stream.shadow`: optional data corrections `(position,data)`;
- `stream.meta.shadow`: optional null-bit corrections corresponding to data shadow updates;
- retention segments and rotated `.oldN` variants as configured.

Record `i` starts at byte `i * descriptor_record_size`.

## Metadata format and invariants

Current implementation is decomposed into:

- `IndexRecord`: `{nullBitset, recordCount, isGap}` serialization;
- `MetaIndexStore`: `.meta` header/raw entry persistence;
- `metaData`: RLE policy, logical record lookup, gap coordination, lifecycle;
- `GapDetector`: nullfill/absorbed-gap state machine;
- `metaShadow`: persistent per-record null overrides;
- `storageShadow`: `metaData` variant that sends modifications to the shadow index.

At the tested two-field layout, `.meta` header is 8 bytes and an entry is 18 bytes; do not universalize entry size because serialized bitset size depends on descriptor fields.

RLE behavior:

- an append with the same null pattern increments the current run;
- a changed pattern commits the current run and starts another;
- lazy tail overwrite prevents a homogeneous stream from growing one metadata entry per append;
- updates split an existing run into up to prefix/replacement/suffix;
- gap entries do not consume logical data-record indices.

`flushCurrentEntry()` is called after storage writes so metadata survives another reader or crash boundary. `abandonFile()` is required before deleting disposable metadata.

## Gaps

For declared streams with a positive interval, storage enables gap detection. Consecutive all-null records:

1. pass through for a configured nullfill count (default 2);
2. subsequent all-null records are absorbed rather than written to dense data;
3. the absorbed duration is accumulated;
4. the next non-null record or shutdown flushes one gap marker into `.meta`.

Thus the binary file remains dense while metadata preserves the temporal discontinuity.

## Shadows and correction

`posixBinaryFileWithShadow` appends normal records to the main file and writes historical updates to `.shadow`. Reads select the latest shadow entry for a position; repeated corrections use last-write-wins.

Metadata must mirror this decision:

- normal main metadata remains in `.meta`;
- null-pattern changes for shadowed records go to `.meta.shadow`;
- reads prefer the shadow override;
- merge applies both data and metadata corrections and removes both shadows;
- discard removes both shadows and restores the main-file view;
- restart reloads shadow overrides.

This separation supports auditability and reversible correction.

## Retention and rotation

`groupFile` splits storage into segments. `retention_t` carries segment count and per-segment capacity; purge removes expired segments and their shadows. `DEFAULT` and `DIRECT` support this grouping.

RQL `ROTATION 'counter-file'` creates a `PersistentCounter`. Rotation spans session boundaries:

- on shutdown, data and data-shadow files are renamed with `.oldN`;
- on the next startup, storage detects a new/empty data file with old nonempty metadata and rotates metadata to `.meta.oldN`;
- the counter increments on destruction.

Because data and metadata rotate at different lifecycle points, same-suffix files can represent adjacent session timing. Use `xtrdb -s` rather than pairing them by intuition.

Without `ROTATION`, `launcher.cpp` removes user SELECT artifact data, `.desc`, and `.meta` before execution. RQL directives and declarations are excluded.

## xtrdb

`xtrdb` provides an interactive command dispatcher (`cmdOpen`, `cmdStorage`, `cmdPayload`, `cmdMeta`, `cmdFieldAccess`, `cmdDropFile`, `cmdStatus`) for opening, describing, listing/reverse-listing, reading, appending/updating, purging, and inspecting metadata.

`xtrdb -s <path>` is a read-only storage-map report. It identifies descriptor, main/segmented data, metadata RLE, shadows, retention segments, and rotated files. Its metadata bar distinguishes full data, partial null, nullfill, and gaps.

Use `xtrdb` in integration tests because it validates the actual persisted contract rather than private C++ object state.
