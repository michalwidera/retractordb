# Embedding RetractorDB: one programme, three consumers

**Status:** stage 1 in progress. **Base revision:** `master` @ `f22cffe0`.

RetractorDB is asked to run inside three foreign hosts: a Python interpreter
(Jupyter, Colab, a PyTorch training loop), an iOS application, and an Android
application. It is tempting to treat these as three integration projects. They are
not. They are one refactor with three thin bindings on top, and the refactor is
almost all of the work.

## 1. What actually blocks embedding

The engine is a service, and two properties of a service are fatal inside a host
process.

**It ends the process on error.** `FatalError` finishes with
`std::exit(EXIT_FAILURE)` (`src/include/fatalError.hpp:51`). At `f22cffe0` there are
**221 invocations** across `src/`, **79 of them inside the storage layer alone**
(`src/rdb/lib/*.cc`). For a daemon that is a defensible design. Inside a host it
means a malformed `.desc` file takes down the Jupyter kernel, the iOS app, or the
Android activity - with no stack, no exception, and nothing the host can catch.

**It keeps process-wide state.** Three globals leak across instances: `statusDesc`
(`src/rdb/lib/DESCParser.cc:13`), `fatalErrorRaised`
(`src/include/fatalError.hpp:16`), and the MEMORY accessor maps
(`src/rdb/lib/faccmemory.cc:13-16`). A daemon constructs each of these once. A host
process constructs them again on every `Engine(...)`, which is the classic
works-the-first-time-fails-on-re-run defect.

Everything else - the C ABI, the Swift overlay, JNI, nanobind, DLPack - is small by
comparison, and none of it can be made correct while those two properties hold.

## 2. The layering

```
L4a Swift overlay        L4b Kotlin overlay        L4c retractordb (Python pkg)
L3a C ABI (librdbc)      L3b JNI                   L3c _core (nanobind, binds L2)
                    L2   librdbembed - C++ embedded facade
                    L1   librdbcore  - de-serviced engine
```

L1 and L2 are shared by all three. L3 is where the consumers diverge, and Python
diverges twice over: it binds L2 (C++) directly rather than going through the C ABI,
because nanobind carries `std::variant` and translates C++ exceptions natively while
Swift and JNI cannot. Routing Python through the C ABI would marshal every value
twice on the path that is meant to feed a training loop. The C ABI keeps its value as
a stability contract for the two consumers that need it.

## 3. Why Python is stage 1

The decision is about sequencing inside the refactor, not about which product ships
first. The business case is most likely mobile; the cheapest place to find out
whether the refactor is *correct* is not.

Converting 221 `std::exit` sites into exceptions means unwinding through code written
on the documented assumption that a fatal error ends the process. That assumption has
already bitten this tree three times in one month
(`executorsm.cpp:70-77`, `ipcServer.cpp:54-69`, `RQLParser.cpp:33-41`). The
conversion needs to be *exercised*, not reviewed.

A notebook is an interactive fuzzing harness for exactly that: feed it malformed RQL,
a truncated `.desc`, a missing storage directory, and observe whether the process
survives. Iteration is seconds, a failure costs a kernel, and you get a Python
traceback instead of a crash log. Discovering the same defect through an iOS crash
report costs an order of magnitude more.

One structural advantage makes this viable today: **the library half is already
signal-clean.** Nothing in `src/rdb/lib/` or `src/retractor/lib/` installs a signal
handler; the only `signal()` calls are in `launcher.cpp:802-804`, which an embedded
build drops. Python owns `SIGINT` and would fight anything that did otherwise.

## 4. Stages

| Stage | Target | Host platform | Build flag | Depends on |
|---|---|---|---|---|
| **1a** | Python: storage layer only | Linux, macOS | `RDB_PYTHON` | nothing - **in progress** |
| **1b** | Python: full engine, DLPack, wheels | Linux, macOS | `RDB_PYTHON` | core phases 1-3 |
| **2** | iOS / iPadOS XCFramework | macOS | `RDB_IOS` *(reserved)* | core phases 1-5, stage 1b |
| **3** | Android AAR | Linux | `RDB_ANDROID` *(reserved)* | core phases 1-5, stage 1b |

The host-platform column is a property of the toolchain, not a preference: an
XCFramework needs Xcode and can only be produced on macOS, and the Android NDK
toolchain is driven from Linux in this project's CI. `build-options.md` states how
each flag behaves and what happens when it is requested on the wrong host.

The **core phases** referenced above are the shared refactor, specified in the iOS
roadmap: (1) `FatalError` becomes an exception, (2) de-globalize and inject a log
sink, (3) `step()` and the removal of `_kbhit`, (4) push ingest, (5) the error
taxonomy. Nothing above stage 1a can begin before phase 3.

## 5. Where the tree is now

Stage 1a binds the storage layer and nothing above it, because the storage layer is
the one part that needs none of the refactor: 25 `.cc` files with no service
dependencies. It is a real, useful capability - reading `.desc` files and records
from a notebook is how you debug the engine - and it settles the binding-technology
question before anyone commits to it.

It also carries the refactor's acceptance tests. The kernel-survival and re-entry
suites in `api/python/tests/` assert today's behaviour, each marked with the
assertion it flips to once phases 1 and 2 land. They are written now because writing
them is cheap now and because they are the evidence that the expensive part worked.

See [`jupyter-integration.md`](jupyter-integration.md) for what stage 1a delivers in
detail, and what it deliberately does not.
