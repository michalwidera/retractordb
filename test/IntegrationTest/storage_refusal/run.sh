#!/bin/bash
# Ta sama odmowa przed startem/dostarczeniem/resetem/importem ad-hoc (#473).
# Kontrola obejmuje brak zmian na dysku i magistrali, zycie starego planu oraz
# lustro: identyczny plan przechodzi po naprawieniu samej przyczyny odmowy.
set -e
. "$(dirname "$0")/../serverlib.sh"
mode="$1"
eproto=$(errno_value EPROTO)
eio=$(errno_value EIO)
rm -rf store store_saved nosuchdir fakebin
rm -f counter.txt source_fifo source_file foreign.txt restart_called
mkdir store fakebin
printf '1 2 3\n4 5 6\n' > data.txt
printf 'untouched\n' > foreign.txt
cp fake_restart.sh fakebin/systemctl
cp fake_restart.sh fakebin/launchctl
chmod +x fakebin/systemctl fakebin/launchctl
# Samo zrodlo nadal czyta i nadaje wiersze, lecz nie zapisuje metadanych wyniku
# miedzy migawkami dysku. Kazdy kandydat doklada juz wlasny SELECT.
cat > base.rql <<'RQL'
STORAGE 'store'
DECLARE a DOUBLE STREAM src, 1/8 TEXTFILE 'data.txt'
RQL

snapshot() {
  python3 - <<'PY'
from pathlib import Path
import hashlib, json
result = {}
for root in ('store', 'store_saved', 'nosuchdir', 'counter.txt', 'foreign.txt', 'data.txt', 'service_target.rql'):
    p = Path(root)
    for f in [p, *sorted(p.rglob('*'))] if p.is_dir() else [p]:
        if f.is_symlink(): result[str(f)] = ['link', str(f.readlink())]
        elif f.is_file(): result[str(f)] = hashlib.sha256(f.read_bytes()).hexdigest()
        elif f.is_dir(): result[str(f)] = 'directory'
print(json.dumps(result, sort_keys=True))
PY
}

# Pusta linia oddziela sekcje --bus i zostaje na koncu wlasnej sekcji tylko wtedy, gdy po
# niej wypisano sekcje sasiedniej przestrzeni nazw - zalezy od testow biegnacych obok.
bus_snapshot() { bus_own | awk 'NF'; }

wait_stream() {
  local stream="$1" state="${2:-present}" i=0
  while [ "$i" -lt 100 ]; do
    xqry -d > directory.txt
    if grep -qw "$stream" directory.txt; then
      [ "$state" != present ] || return 0
    else
      [ "$state" != absent ] || return 0
    fi
    kill -0 "$_server_pid" 2>/dev/null || break
    sleep 0.05
    i=$((i + 1))
  done
  echo "serwer nie osiagnal stanu $state dla $stream"
  cat directory.txt
  return 1
}

if [ "$mode" != start ]; then
  server_start base.rql -k -r $([ "$mode" = delivery ] && echo --service) >server.out 2>server.err
  wait_stream src
  if [ "$mode" = delivery ]; then
    # Ta sama atrapa metadanych co w it_service_delivery, lecz w przestrzeni oprawy.
    printf 'PID: %s\nPPID: 1\nMODE: service\nUNIT: fake-xretractor.service\nSCOPE: user\nQUERYFILE: %s/service_target.rql\n' \
      "$_server_pid" "$PWD" > "$SERVER_LOCK"
  fi
fi

for kind in missing_directory not_directory output_parent output_directory output_link descriptor_syntax descriptor_schema descriptor_type descriptor_ref text_field source_kind select_syntax select_schema select_type; do
  decl="late_${kind}"
  out="out_${kind}"
  dir=store
  field=DOUBLE
  source=data.txt
  extra="STORAGE DIRECT"
  rotation=""
  expected=""
  rm -rf nosuchdir
  rm -f restart_called
  printf 'old service plan\n' > service_target.rql
  case "$kind" in
    missing_directory|not_directory)
      expected="storage:"
      if [ "$mode" = adhoc ]; then
        mv store store_saved
        [ "$kind" != not_directory ] || printf 'not a directory' >store
      else
        dir=nosuchdir
        [ "$kind" != not_directory ] || printf 'not a directory' >nosuchdir
      fi
      ;;
    output_parent)
      extra="FILE 'missing_parent/result' STORAGE DIRECT"
      rm -rf store/missing_parent
      expected="cannot open output file"
      ;;
    output_directory)
      extra="FILE 'wrong_kind' STORAGE DIRECT"
      rm -rf store/wrong_kind
      mkdir store/wrong_kind
      expected="output file"
      ;;
    output_link)
      extra="FILE 'linked_output' STORAGE DIRECT"
      rm -f store/linked_output
      ln -s ../foreign.txt store/linked_output
      expected="output file"
      ;;
    descriptor_syntax)
      printf '{ INTEGER }' >"store/$decl.desc"
      expected="descriptor parse failed"
      ;;
    descriptor_schema)
      field=INTEGER
      printf '{ DOUBLE a REF "data.txt" TYPE TEXTSOURCE }' >"store/$decl.desc"
      expected="descriptor schema mismatch"
      ;;
    descriptor_type)
      printf '{ DOUBLE a REF "data.txt" TYPE UNKNOWN }' >"store/$decl.desc"
      expected="unsupported storage type"
      ;;
    descriptor_ref)
      printf '{ DOUBLE a REF "foreign.txt" TYPE TEXTSOURCE }' >"store/$decl.desc"
      expected="was written for source"
      ;;
    text_field)
      printf '{ RATIONAL a REF "data.txt" TYPE TEXTSOURCE }' >"store/$decl.desc"
      expected="unsupported field type RATIONAL"
      ;;
    source_kind)
      source=source_fifo
      rm -f source_fifo
      mkfifo source_fifo
      expected="is a FIFO"
      ;;
    select_syntax|select_schema|select_type)
      # Start/reset zachowuja wynik tylko z ROTATION; ad-hoc zachowuje go takze bez niej.
      rotation="ROTATION 'counter.txt'"
      case "$kind" in
        select_syntax) printf '{ INTEGER }' >"store/$out.desc"; expected="descriptor parse failed" ;;
        select_schema) printf '{ STRING v[64] }' >"store/$out.desc"; expected="descriptor schema mismatch" ;;
        select_type) printf '{ DOUBLE v TYPE UNKNOWN }' >"store/$out.desc"; expected="written with STORAGE UNKNOWN" ;;
      esac
      if [ "$mode" = adhoc ] && [ "$kind" = select_type ]; then expected="unsupported storage type"; fi
      ;;
  esac
  printf '%s\n' "$rotation" "STORAGE '$dir'" \
    "DECLARE a $field STREAM $decl, 1/8 TEXTFILE '$source'" \
    "SELECT a STREAM $out FROM $decl $extra" >candidate.rql
  if [[ "$kind" = descriptor_* || "$kind" = text_field || "$kind" = source_kind ]]; then
    adhoc="DECLARE a $field STREAM $decl, 1/8 TEXTFILE '$source'"
    imported="$decl"
  else
    adhoc="SELECT a STREAM $out FROM src $extra"
    imported="$out"
  fi
  cp candidate.rql candidate.saved
  snapshot >before.json
  if [ "$mode" != start ]; then bus_snapshot >bus_before.txt; fi
  rc=0
  case "$mode" in
    start) run_timeout 20 xretractor candidate.rql -k -m 1 -f >refusal.out 2>refusal.err || rc=$? ;;
    delivery) PATH="$PWD/fakebin:$PATH" RDB_RESTART_MARKER="$PWD/restart_called" \
      run_timeout 20 xretractor candidate.rql -k >refusal.out 2>refusal.err || rc=$? ;;
    reset) run_timeout 20 xqry --reset candidate.rql >refusal.out 2>refusal.err || rc=$? ;;
    adhoc) run_timeout 20 xqry -a "$adhoc" >refusal.out 2>refusal.err || rc=$? ;;
  esac
  if [ "$rc" -eq 0 ] || [ "$rc" -eq 124 ]; then
    echo "$mode/$kind: brak odmowy lub zawieszenie (kod $rc)"
    cat refusal.out refusal.err
    exit 1
  fi
  # Dowiazanie w miejscu pliku wyjscia to jedyny przypadek K4 (ELOOP): io_error, plan zostaje.
  want="$eproto"
  [ "$kind" != output_link ] || want="$eio"
  if [[ "$mode" = start || "$mode" = delivery ]] && [ "$rc" -ne "$want" ]; then
    echo "$mode/$kind: kod $rc zamiast $want"
    cat refusal.out refusal.err
    exit 1
  fi
  grep -qF "$expected" refusal.err refusal.out || {
    echo "$mode/$kind: brak przyczyny '$expected'"
    cat refusal.out refusal.err
    exit 1
  }
  if grep -q 'FATAL' refusal.out refusal.err; then
    echo "$mode/$kind: odmowa zostala bledem krytycznym"
    exit 1
  fi
  cmp -s candidate.rql candidate.saved || { echo "odmowa zmienila plan operatora"; exit 1; }
  snapshot >after.json
  bash ../compare.sh before.json after.json
  [ ! -e restart_called ] || { echo "odmowa zlecila restart jednostki"; exit 1; }
  if [ "$mode" != start ]; then
    kill -0 "$_server_pid"
    wait_stream src
    bus_snapshot >bus_after.txt
    bash ../compare.sh bus_before.txt bus_after.txt
    run_timeout 10 xqry -s src -m 1 >still_alive.txt 2>&1
    if grep -q FATAL server.err; then cat server.err; exit 1; fi
  fi

  # Naprawa przyczyny - identyczny tekst zapytania musi teraz przejsc.
  case "$kind" in
    missing_directory|not_directory)
      if [ "$mode" = adhoc ]; then rm -f store; mv store_saved store
      else rm -f nosuchdir; mkdir -p nosuchdir; fi ;;
    output_parent) mkdir -p store/missing_parent ;;
    output_directory) rmdir store/wrong_kind ;;
    output_link) rm store/linked_output ;;
    descriptor_*|text_field) rm "store/$decl.desc" ;;
    source_kind) rm source_fifo; printf '1\n' >source_fifo ;;
    select_*) rm "store/$out.desc" ;;
  esac
  case "$mode" in
    start) run_timeout 20 xretractor candidate.rql -k -m 1 -f >accepted.out 2>accepted.err ;;
    delivery) PATH="$PWD/fakebin:$PATH" RDB_RESTART_MARKER="$PWD/restart_called" \
      run_timeout 20 xretractor candidate.rql -k >accepted.out 2>accepted.err
      [ -e restart_called ]
      bash ../compare.sh candidate.rql service_target.rql ;;
    reset) run_timeout 20 xqry --reset candidate.rql >accepted.out 2>accepted.err
      wait_stream "$out"
      run_timeout 20 xqry --reset base.rql >restored.out 2>restored.err
      wait_stream "$out" absent
      wait_stream src ;;
    adhoc) run_timeout 20 xqry -a "$adhoc" >accepted.out 2>accepted.err
      wait_stream "$imported"
      # Wynik dyskowy liczy od chwili przyjecia. Przed nastepna migawka wracamy
      # do samego zrodla, aby porownywac odmowe, a nie przyrost wczesniejszego wyniku.
      run_timeout 20 xqry --reset base.rql >restored.out 2>restored.err
      wait_stream "$imported" absent
      wait_stream src ;;
  esac
  echo "OK $mode/$kind"
done

# Dotychczasowy wyzwalacz FatalError jest teraz zwykla odmowa startu (#473).
if [ "$mode" = start ]; then
  rm -rf nosuchdir
  rc=0
  run_timeout 20 xretractor missing_storage.rql -k -m 1 -f >missing.out 2>missing.err || rc=$?
  [ "$rc" -eq "$eproto" ]
  grep -q 'does not exist' missing.err
  if grep -q FATAL missing.err; then cat missing.err; exit 1; fi
fi
if [ "$mode" != start ]; then
  xqry -k >/dev/null
  server_wait_exit
fi
