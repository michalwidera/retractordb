---
name: ablation-floor
description: Przepis na floor ablacji - konfiguracja all-off (wszystkie RDB_OPT_* i sonda OFF), prefiks PATH, kontrola trzech programów, pełny ctest. Wywołaj przed uruchomieniem floor, czyli gdy sesja dotknęła compiler.cpp, reguł opóźnienia startu lub ogona albo kodu za przełącznikiem RDB_OPT_*.
---

# Ablation floor - przepis

Reguła (kiedy floor jest obowiązkowy, co znaczy sukces, które asercje wolno wyłączyć) jest w `CLAUDE.md`, sekcja *Session end*, akapit *Ablation floor*. Ten plik zawiera przepis.

```bash
scripts/buildrdb.sh release-ablation     # interactive: set every RDB_OPT_* switch OFF, probe OFF
A=<directory printed by the script>
(
  export PATH="$A/src/retractor:$A/src/rdb:$A/src/qry:$PATH"
  command -v xretractor xtrdb xqry &&   # each must print a path under $A/src/
  xretractor --build-info &&          # every RDB_OPT_* and RDB_BENCH_PROBE must be OFF
  ctest --test-dir "$A/test" -j 4 --output-on-failure
)
```

`release-ablation` prints this command with the selected build's paths after building and verifying its configuration. The script is interactive (a `select` menu), so a session cannot drive it: when the all-off directory already exists - the one under `build/Release-Ablation/` whose name has every switch `OFF` - rebuild it with `cmake --build "$A"` and run the block above from `A=` on; when it does not exist, ask the human to run the script. Run it after the executable/fixture provenance checks in `CLAUDE.md` (*Build*) and in `test/CLAUDE.md`, including byte comparisons and inspection of explicit build-tree paths. The subshell keeps the ablation PATH local to this run; before ordinary Debug integration and Valgrind tests, follow the build/install sequence in `test/CLAUDE.md`.

**The floor tests the all-off binaries only with that `PATH` prefix.** Integration tests call `xretractor`, `xqry` and `xtrdb` by name, and the CTest file of every build directory only *appends* the install directory to the caller's `PATH`, so a plain `ctest --test-dir "$A/test"` runs whatever the caller's `PATH` finds first - normally the Debug install in `~/.local/bin`. On 2026-10-04 (#328) a 348/348 green ablation run turned out to have tested the Debug engine in every by-name integration test. Report the three resolved paths with the result. Never `ninja install` the ablation directory instead: it would overwrite `~/.local/bin` with a Release build and break the next Debug `ctest` (see *Namespaces* in `test/CLAUDE.md`).
