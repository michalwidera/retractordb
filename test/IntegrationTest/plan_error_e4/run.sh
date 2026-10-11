#!/bin/bash
# Kolizje substratow sa osiagalne przez RQL z podkresleniami w nazwach. Sprawdzamy
# odmowe -c, startu, ad hoc i resetu oraz poprawne zapytanie po kazdej odmowie (#474).
set -euo pipefail
. "$(dirname "$0")/../serverlib.sh"

reason="Substrate name 'STREAM_ADD_A_B_C' denotes two different programs"
bad="SELECT * STREAM second FROM (A + B_C)>1 STORAGE MEMORY"
cat plan.rql > collision.rql
printf '%s\n' "$bad" >> collision.rql
expected_rc=$(errno_value EPROTO)
for mode in compile start; do
  rc=0
  if [ "$mode" = compile ]; then
    xretractor collision.rql -c > "$mode.out" 2> "$mode.err" || rc=$?
  else
    xretractor collision.rql -k -r > "$mode.out" 2> "$mode.err" || rc=$?
  fi
  if [ "$rc" -ne "$expected_rc" ] || ! grep -qF "Check result:$reason" "$mode.err"; then
    echo "$mode: oczekiwano EPROTO i Check result, kod=$rc"
    cat "$mode.out" "$mode.err"
    exit 1
  fi
  if grep -qF 'FATAL:' "$mode.err"; then
    cat "$mode.err"
    exit 1
  fi
done

mkdir -p temp
server_start plan.rql -k -r
rc=0
xqry -a "$bad" > adhoc.out 2> adhoc.err || rc=$?
if [ "$rc" -eq 0 ] || ! grep -qF "Fail local chain compiler:$reason" adhoc.err; then
  echo "ad hoc: brak odmowy kompilacji, kod=$rc"
  cat adhoc.out adhoc.err
  exit 1
fi
kill -0 "$_server_pid"
xqry -d > directory.out
if grep -qw second directory.out; then
  echo "odrzucony strumien zostal w planie"
  exit 1
fi
xqry -a 'SELECT A[0]+1 STREAM afterAdhoc FROM A STORAGE MEMORY'
xqry -s afterAdhoc -m 2 > after_adhoc.out
test "$(grep -c '^[0-9]' after_adhoc.out)" -eq 2

rc=0
xqry --reset collision.rql > reset.out 2> reset.err || rc=$?
if [ "$rc" -eq 0 ] || ! grep -qF "Fail compile:$reason" reset.err; then
  echo "reset: brak odmowy kompilacji, kod=$rc"
  cat reset.out reset.err
  exit 1
fi
kill -0 "$_server_pid"
xqry -a 'SELECT A[0]+2 STREAM afterReset FROM A STORAGE MEMORY'
xqry -s afterReset -m 2 > after_reset.out
test "$(grep -c '^[0-9]' after_reset.out)" -eq 2
xqry -k
server_wait_exit

# Status z dataModel, ktorego preflight nie zdazyl zobaczyc: hak zatrzymuje start
# przed budowa modelu, a test zmienia wtedy deskryptor, katalog albo plik wyjscia.
wait_for_model_gate() {
  for attempt in {1..200}; do
    if [ -f model_gate.ready ]; then return; fi
    kill -0 "$_server_pid"
    sleep 0.01
  done
  echo "bramka budowy modelu nie zostala osiagnieta"
  exit 1
}

rm -rf temp
mkdir temp
rm -f model_gate.ready model_gate.release
export RDB_FAULT_MODEL_INIT_GATE="$PWD/model_gate"
server_start plan.rql -k -r > late_plan.out 2> late_plan.err
unset RDB_FAULT_MODEL_INIT_GATE
wait_for_model_gate
printf 'invalid descriptor\n' > temp/first.desc
touch model_gate.release
server_wait_status > late_plan.status
test "$(cat late_plan.status)" -eq "$expected_rc"
grep -qF "Check result:dataModel: stream 'first':" late_plan.err
grep -qF 'descriptor parse failed' late_plan.err
if grep -qF 'FATAL:' late_plan.err; then cat late_plan.err; exit 1; fi

rm -rf temp
mkdir -p temp/late
cat plan.rql > late_env.rql
printf '%s\n' "SELECT * STREAM failed FROM A_B FILE 'late/out' STORAGE POSIX" >> late_env.rql
rm -f model_gate.ready model_gate.release
export RDB_FAULT_MODEL_INIT_GATE="$PWD/model_gate"
server_start late_env.rql -k -r > late_env.out 2> late_env.err
unset RDB_FAULT_MODEL_INIT_GATE
wait_for_model_gate
rm -rf temp/late
printf 'directory replaced by a file\n' > temp/late
touch model_gate.release
server_wait_status > late_env.status
test "$(cat late_env.status)" -eq "$expected_rc"
grep -qF "Check result:dataModel: stream 'failed': cannot open output file" late_env.err
if grep -qF 'FATAL:' late_env.err; then cat late_env.err; exit 1; fi

# K4: dowiazanie w miejscu pliku wyjscia odrzuca O_NOFOLLOW (ELOOP), tak jak preflight.
rm -rf temp
mkdir temp
cat plan.rql > late_link.rql
printf '%s\n' "SELECT * STREAM linked FROM A_B FILE 'late_link' STORAGE POSIX" >> late_link.rql
rm -f model_gate.ready model_gate.release
export RDB_FAULT_MODEL_INIT_GATE="$PWD/model_gate"
server_start late_link.rql -k -r > late_link.out 2> late_link.err
unset RDB_FAULT_MODEL_INIT_GATE
wait_for_model_gate
ln -s ../data.txt temp/late_link
touch model_gate.release
server_wait_status > late_link.status
test "$(cat late_link.status)" -eq "$(errno_value EIO)"
grep -qF "xretractor: dataModel: stream 'linked': cannot open output file" late_link.err
if grep -qF 'Check result:' late_link.err || grep -qF 'FATAL:' late_link.err; then cat late_link.err; exit 1; fi
