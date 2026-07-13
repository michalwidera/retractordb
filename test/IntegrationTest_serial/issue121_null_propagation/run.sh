#!/bin/bash
# Propagacja null przez SELECT do strumienia wynikowego (issue #121).
# Logika wyniesiona ze skryptu (zamiast bash -c w add_test): pozwala użyć
# ${TMPDIR:-/tmp} dla ścieżki blokady serwera (xretractor tworzy lock w
# temp_directory_path(), zob. lockManager.cpp). Bez tego pętle synchronizacji
# wieszają się gdy TMPDIR != /tmp.
set -e
LOCK="${TMPDIR:-/tmp}/xretractor_service.lock"
# xretractor/xqry logują domyślnie do temp_directory_path()/<bin>.log (zob.
# uxSysTermTools.cpp) - ten sam katalog co LOCK, więc respektuje TMPDIR.
XRETRACTOR_LOG="${TMPDIR:-/tmp}/xretractor.log"
XQRY_LOG="${TMPDIR:-/tmp}/xqry.log"

# Diagnostyka na wypadek timeoutu/awarii (np. CI #187: 60s timeout, brak
# powtórzenia lokalnie) - dumpuje stan procesów i logi, żeby przy kolejnym
# wystąpieniu było co analizować zamiast gołego "Timeout 60.06 sec".
dump_diagnostics() {
  echo "=== run.sh diagnostics ==="
  if [ -f "$LOCK" ]; then echo "LOCK ($LOCK): present"; else echo "LOCK ($LOCK): absent"; fi
  echo "--- ps (xretractor/xqry) ---"
  ps -ef | grep -E 'xretractor|xqry' | grep -v grep || true
  echo "--- out.txt so far ---"
  cat out.txt 2>/dev/null || echo "(missing)"
  echo "--- tail -n 200 $XRETRACTOR_LOG ---"
  tail -n 200 "$XRETRACTOR_LOG" 2>/dev/null || echo "(missing)"
  echo "--- tail -n 200 $XQRY_LOG ---"
  tail -n 200 "$XQRY_LOG" 2>/dev/null || echo "(missing)"
  echo "=== end diagnostics ==="
}
trap dump_diagnostics TERM
trap '[ $? -ne 0 ] && dump_diagnostics; true' EXIT

mkdir -p temp
xretractor query.rql -k -x &
while [ ! -f "$LOCK" ]; do sleep 0.1; done
xqry -s dst -k -m 3 > out.txt
while [ -f "$LOCK" ]; do sleep 0.1; done
grep -F '10 null' out.txt
grep -F 'null 20' out.txt
grep -F '30 40' out.txt
