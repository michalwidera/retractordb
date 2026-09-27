#!/bin/bash
# Magazyny na dysku: zaden nie rosnie bez granicy po cichu (decyzje D7 i D8, 2026-09-27).
#
# D7. `SELECT ... RETENTION 5 STORAGE DIRECT` - przyklad z dokumentacji typow storage - dawal .desc
# z `RETMEMORY 5` i `TYPE DEFAULT`: magazyn DEFAULT z .shadow zamiast DIRECT i bez retencji, jeden plik
# rosnacy o rekord na takt. Teraz to blad planu z podpowiedzia `RETENTION 5 <segments>`, a postac
# dwuargumentowa daje DIRECT z segmentami.
# D8. Wzrost na dysku zostaje dozwolony, ale jawny: ostrzezenie na stderr przy starcie i w `-c`,
# a `[storage] default_retention` daje retencje strumieniom bez RETENTION.
# Za mala retencja (ponizej historii czytanej przez plan) konczyla dzialajacy serwer FatalError-em
# w storage::read; teraz jest bledem kompilacji.
# P4. `RETENTION c s` na magazynie w pamieci bylo ignorowane bez slowa; teraz blad planu.
# Sprzatanie: start bez :ROTATION kasowal tylko `<id>`, `.desc` i `.meta` strumieni z linii planu -
# drugi start strumienia z `RETENTION c s` konczyl sie FatalError-em, a pliki wezlow posrednich
# rosly miedzy restartami. P5: :ROTATION z plikami zapisanymi w innej konfiguracji to odmowa startu.
# Nowy strumien ad-hoc bez granicy trafia do dziennika.
set -e
. "$(dirname "$0")/../serverlib.sh"

fail() {
  echo "FAIL: $1"
  [ -n "$2" ] && cat "$2"
  exit 1
}

header="STORAGE 'temp'
DECLARE a INTEGER STREAM src, 1 FILE 'src.txt'"

fresh() {
  rm -rf temp && mkdir -p temp
}

# --- D7: jednoargumentowe RETENTION na dysku odpada, i w -c, i przy starcie ---
printf '%s\nSELECT src[0] STREAM str2 FROM src RETENTION 5 STORAGE DIRECT\n' "$header" >single.rql
fresh
if xretractor single.rql -c >single_c.txt 2>&1; then fail "-c accepted single-argument RETENTION on DIRECT" single_c.txt; fi
grep -q "write RETENTION 5 <segments>" single_c.txt || fail "-c did not suggest RETENTION 5 <segments>" single_c.txt
if xretractor single.rql -m 40 -f </dev/null >single_run.txt 2>&1; then fail "start accepted single-argument RETENTION" single_run.txt; fi
[ -z "$(ls temp)" ] || fail "rejected plan left files in temp: $(ls temp)"

# --- D7: dwuargumentowe RETENTION z STORAGE DIRECT - dwa segmenty, bez .shadow ---
printf '%s\nSELECT src[0] STREAM str2 FROM src RETENTION 5 2 STORAGE DIRECT\n' "$header" >segmented.rql
fresh
xretractor segmented.rql -m 40 -f </dev/null >segmented_run.txt 2>&1 || fail "segmented plan did not run" segmented_run.txt
segments=$(ls temp | grep -c -E '^str2_segment_[0-9]+$' || true)
[ "$segments" -eq 2 ] || fail "expected 2 segments of str2, found $segments: $(ls temp)"
if ls temp | grep -q 'shadow'; then fail "DIRECT storage left a .shadow file: $(ls temp)"; fi
grep -q "grows without bound" segmented_run.txt && fail "bounded stream reported as unbounded" segmented_run.txt

# --- D8: ostrzezenie wymienia strumien bez granicy, w -c i przy starcie; ograniczonego nie ---
printf '%s\nSELECT src[0] STREAM plain FROM src\nSELECT src[0] STREAM bounded FROM src RETENTION 5 2\n' "$header" >warn.rql
fresh
xretractor warn.rql -c >warn_c.out 2>warn_c.err || fail "-c rejected a valid plan" warn_c.err
grep -q "warning: stream plain grows without bound on disk (no RETENTION)" warn_c.err || fail "-c did not name plain" warn_c.err
grep -q "stream bounded" warn_c.err && fail "-c named a bounded stream" warn_c.err
grep -q "grows without bound" warn_c.out && fail "-c warning leaked to stdout" warn_c.out
xretractor warn.rql -c --quiet >/dev/null 2>warn_q.err || fail "-c --quiet rejected a valid plan" warn_q.err
grep -q "stream plain grows without bound" warn_q.err || fail "-c --quiet dropped the warning" warn_q.err
xretractor warn.rql -m 3 -f </dev/null >warn_run.out 2>warn_run.err || fail "start rejected a valid plan" warn_run.err
grep -q "warning: stream plain grows without bound on disk (no RETENTION)" warn_run.err || fail "start did not name plain" warn_run.err

# --- D8: [storage] default_retention trafia do .desc i ogranicza liczbe segmentow ---
printf '[storage]\ndefault_retention = [5, 2]\n' >retention.toml
fresh
xretractor warn.rql -c --config retention.toml >/dev/null 2>cfg_c.err || fail "-c with default_retention failed" cfg_c.err
grep -q "grows without bound" cfg_c.err && fail "default_retention left a stream unbounded" cfg_c.err
xretractor warn.rql -m 40 -f --config retention.toml </dev/null >cfg_run.txt 2>&1 || fail "run with default_retention failed" cfg_run.txt
grep -q "RETENTION" temp/plain.desc || fail "plain.desc has no RETENTION" temp/plain.desc
segments=$(ls temp | grep -c -E '^plain_segment_[0-9]+$' || true)
[ "$segments" -eq 2 ] || fail "expected 2 segments of plain, found $segments: $(ls temp)"

# --- Retencja krotsza niz historia czytana przez plan: blad kompilacji zamiast FatalError w biegu ---
printf '%s\nSELECT src[0] STREAM mid FROM src RETENTION 2 3 STORAGE DIRECT\nSELECT mid[0] STREAM late FROM mid>6 VOLATILE\n' \
  "$header" >short.rql
fresh
if xretractor short.rql -c >short_c.txt 2>&1; then fail "-c accepted retention shorter than the history read" short_c.txt; fi
grep -q "Stream 'mid' keeps only 5 record(s) on disk under RETENTION 2 3, but the plan reads 7 record(s) back" short_c.txt ||
  fail "-c did not explain the short retention" short_c.txt

# --- P4: segmenty na magazynie w pamieci to blad planu z podpowiedzia pierscienia ---
printf '%s\nSELECT src[0] STREAM ring FROM src RETENTION 5 2 STORAGE MEMORY\n' "$header" >memseg.rql
if xretractor memseg.rql -c >memseg_c.txt 2>&1; then fail "-c accepted RETENTION 5 2 on STORAGE MEMORY" memseg_c.txt; fi
grep -q "write RETENTION 5 for a MEMORY ring" memseg_c.txt || fail "-c did not suggest RETENTION 5" memseg_c.txt

# --- Drugi start w tym samym katalogu: segmenty i wezel posredni nie przezywaja startu ---
printf '%s\nSELECT src[0] STREAM seg FROM src RETENTION 5 3\nSELECT seg[0] STREAM late FROM seg>2 VOLATILE\nSELECT * STREAM sum FROM SUMC(src@(1,3))\n' \
  "$header" >restart.rql
fresh
xretractor restart.rql -m 12 -f </dev/null >restart1.txt 2>&1 || fail "first start failed" restart1.txt
intermediate=""
for f in temp/STREAM_*; do
  case "$f" in *.desc | *.meta | *.shadow) ;; *) intermediate="$f" ;; esac
done
[ -n "$intermediate" ] || fail "no intermediate data file in temp: $(ls temp)"
first_size=$(wc -c <"$intermediate")
xretractor restart.rql -m 12 -f </dev/null >restart2.txt 2>&1 || fail "second start in the same directory failed" restart2.txt
second_size=$(wc -c <"$intermediate")
[ "$second_size" -eq "$first_size" ] || fail "$intermediate grew across restarts: $first_size -> $second_size bytes"

# --- P5: :ROTATION nad plikami zapisanymi z inna retencja - odmowa, pliki nietkniete ---
printf "ROTATION 'rotation_counter.txt'\n%s\nSELECT src[0] STREAM rot FROM src RETENTION 5 2\n" "$header" >rot_a.rql
printf "ROTATION 'rotation_counter.txt'\n%s\nSELECT src[0] STREAM rot FROM src RETENTION 5 3\n" "$header" >rot_b.rql
fresh
rm -f rotation_counter.txt
xretractor rot_a.rql -m 12 -f </dev/null >rot_a1.txt 2>&1 || fail "rotation plan did not run" rot_a1.txt
before=$(ls -l --time-style=full-iso temp)
if xretractor rot_b.rql -m 12 -f </dev/null >rot_b.txt 2>&1; then fail "start accepted kept files with another retention" rot_b.txt; fi
grep -q "Stream 'rot' keeps its files under ROTATION, but temp/rot.desc was written with STORAGE DEFAULT RETENTION 5 2 and the plan asks for STORAGE DEFAULT RETENTION 5 3" rot_b.txt ||
  fail "refusal did not name both configurations" rot_b.txt
[ "$(ls -l --time-style=full-iso temp)" = "$before" ] || fail "refused start changed the kept files"
xretractor rot_a.rql -m 12 -f </dev/null >rot_a2.txt 2>&1 || fail "rotation plan with the kept configuration did not run" rot_a2.txt
rm -f rotation_counter.txt

# --- Nowy strumien ad-hoc bez granicy trafia do dziennika serwera ---
# Dziennik w $TMPDIR dzieli slot przestrzeni nazw, wiec czytamy tylko to, co dopisal ten przebieg.
printf '%s\nSELECT src[0] STREAM base FROM src RETENTION 5 2\n' "$header" >adhoc.rql
fresh
log="${TMPDIR:-/tmp}/xretractor.log"
offset=0
[ -f "$log" ] && offset=$(wc -c <"$log")
server_start adhoc.rql
adhoc_out=$(xqry -a "SELECT src[0] STREAM unbounded_adhoc FROM src" 2>&1) || fail "ad-hoc query rejected: $adhoc_out"
xqry -k
server_wait_exit
tail -c +$((offset + 1)) "$log" >adhoc_log.txt
grep -q "AdHoc stream unbounded_adhoc grows without bound on disk (no RETENTION)" adhoc_log.txt ||
  fail "log did not name the new ad-hoc stream" adhoc_log.txt
grep -q "stream base grows" adhoc_log.txt && fail "log named a bounded stream" adhoc_log.txt

echo "OK"
