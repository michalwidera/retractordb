#!/bin/bash
# Buduje oracle kanonicznego serializera (strona C++) linkujac go z librdb.a
# z DRZEWA BUDOWY retractordb. Nie modyfikuje ani jednego pliku w retractordb —
# uzywa wylacznie naglowkow i skompilowanych bibliotek.
#
# Domyslnie bierze profil K26v3-DEFAULT (ten, w ktorym pilot mierzyl mechanizm).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CODE_REPO="${CODE_REPO:-/home/michal/github/retractordb}"
BUILD="${BUILD:-$CODE_REPO/build/K26v3-DEFAULT}"

for lib in src/rdb/lib/librdb.a src/rdb/lib/.antlr/libdescparser.a src/common/libcommon.a; do
  [[ -r "$BUILD/$lib" ]] || { echo "brak $BUILD/$lib — zbuduj profil najpierw" >&2; exit 2; }
done

# Sciezki naglowkow i runtime ANTLR-a odczytane z compile_commands.json profilu,
# zeby skrypt nie powtarzal wersji pakietow Conana za CMakiem.
readarray -t CONAN_INC < <(python3 - "$BUILD/compile_commands.json" <<'PY'
import json, sys, shlex
entries = json.load(open(sys.argv[1]))
probe = next(e for e in entries if e["file"].endswith("/probe.cc"))
for tok in shlex.split(probe["command"]):
    if tok.startswith("-I") or tok.startswith("-isystem"):
        print(tok)
    elif tok == "-isystem":
        print(tok)
PY
)

ANTLR_LIB="$(python3 - "$BUILD/compile_commands.json" <<'PY'
import json, sys, shlex, os
entries = json.load(open(sys.argv[1]))
probe = next(e for e in entries if e["file"].endswith("/probe.cc"))
for tok in shlex.split(probe["command"]):
    if tok.startswith("-I") and "antlr" in tok and tok.endswith("/include"):
        print(os.path.join(tok[2:-len("include")], "lib"))
        break
PY
)"

g++ -std=c++23 -O2 -DFMT_HEADER_ONLY -DHAS_BOOST -DSPDLOG_FMT_EXTERNAL \
  "${CONAN_INC[@]}" -I"$BUILD/generated" -I"$CODE_REPO/src/include" \
  -o "$HERE/canonical_oracle" "$HERE/canonical_oracle.cc" \
  "$BUILD/src/rdb/lib/librdb.a" "$BUILD/src/rdb/lib/.antlr/libdescparser.a" \
  "$BUILD/src/common/libcommon.a" \
  -L"$ANTLR_LIB" -lantlr4-runtime -lpthread

echo "OK: $HERE/canonical_oracle"
