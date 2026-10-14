#!/bin/bash
# Mapa kodow wyjscia B.3 (#446): scenariusze bledow z tabeli analizy B, ktorych kod wyjscia
# i komunikat nie byly przypiete zadnym testem integracyjnym. Pozostale wiersze tej tabeli
# maja juz swoje testy (adhoc_parse_error, plan_error_e4, storage_refusal, env_error_exit,
# fatal_exit_path, service_abort, signal_stop, xtrdb_open_missing); tu ich nie powtarzamy.
#
# Test przypina kody po porcie Result<T>: nieznany strumien w FROM jest odmowa planu
# z kodem EPROTO i diagnostyka `Check result:`. Znane wyjatki:
#   - `-u` z brakujacym plikiem zrodla nie konczy sie wcale (#488), wiec tego wiersza nie da sie
#     przypiac kodem wyjscia,
#   - wewnetrzny blad kompilatora jest z RQL nieosiagalny; pilnuje go ut_compiler.
#
# Kody z nazw stalych, nie z liczb: EPROTO to 71 na Linuksie i 100 na macOS.
set -euo pipefail
. "$(dirname "$0")/../serverlib.sh"

eproto=$(errno_value EPROTO)
enoent=$(errno_value ENOENT)
echild=$(errno_value ECHILD)

# expect_exit <nazwa> <kod> <fragment stderr albo ""> <polecenie...>
expect_exit() {
  local name="$1" want="$2" pattern="$3" rc=0
  shift 3
  run_timeout 20 "$@" >"$name.out" 2>"$name.err" || rc=$?
  if [ "$rc" -ne "$want" ]; then
    echo "$name: kod $rc zamiast $want: $*"
    cat "$name.out" "$name.err"
    exit 1
  fi
  if [ -n "$pattern" ] && ! grep -qF -- "$pattern" "$name.err"; then
    echo "$name: brak '$pattern' na stderr"
    cat "$name.out" "$name.err"
    exit 1
  fi
}

rm -rf temp nosuchdir bad bad.desc* ./*.out ./*.err
mkdir temp
seq 1 50 >data.txt
source_decl="DECLARE a INTEGER STREAM src, 1/10 TEXTFILE 'data.txt'"
printf '%s\n' "STORAGE 'temp'" "$source_decl" "SELECT src[0] STREAM ok FROM src STORAGE MEMORY" >base.rql

# (1) xqry bez serwera: brak segmentu magistrali to wyjatek IPC, kod ECHILD. Przed startem
# jakiegokolwiek serwera w przestrzeni nazw tego testu.
expect_exit hello_no_server "$echild" "IPC:" xqry -l
expect_exit select_no_server "$echild" "IPC:" xqry -s ok -m 1

# (2) xtrdb z istniejacym, uszkodzonym .desc: blad w powloce, kod 0, plik nietkniety.
# xtrdb_open_missing sprawdza brak .desc i zly schemat podany w poleceniu, nie zly plik.
printf '{ INTEGER }' >bad.desc
cp bad.desc bad.desc.orig
: >bad
printf 'open bad\nquit\n' >xtrdb_cmds.txt
expect_exit xtrdb_bad_desc 0 "Syntax error @Descriptor" xtrdb <xtrdb_cmds.txt
grep -qF "descriptor parse failed in 'bad.desc'" xtrdb_bad_desc.out
cmp bad.desc bad.desc.orig

# (3) Blad skladni w pliku planu: EPROTO i pozycja bledu w `Parse result:`, w -c i w -u.
# adhoc_parse_error sprawdza start z pliku tylko dla bledow wartosci, nie skladni.
printf '%s\n' "STORAGE 'temp'" "$source_decl" "SELEC src[0] STREAM y FROM src" >syntax.rql
expect_exit syntax_c "$eproto" "Parse result:line 3:0 mismatched input 'SELEC'" xretractor syntax.rql -c
expect_exit syntax_u "$eproto" "Parse result:line 3:0 mismatched input 'SELEC'" xretractor syntax.rql -u

# (4) Rozmiar generatora ponad granice planu: EPROTO z parsera. Samej granicy (148) nie
# przypinamy - robi to ut_compiler (DimensionLimit), a zmienia ja decyzja M11.
printf '%s\n' "STORAGE 'temp'" "$source_decl" "SELECT src[\$] STREAM cells[200] FROM src" >generator.rql
expect_exit generator_c "$eproto" "Parse result:stream generator size 200 exceeds the limit " xretractor generator.rql -c
expect_exit generator_u "$eproto" "Parse result:stream generator size 200 exceeds the limit " xretractor generator.rql -u

# (5) -c sprawdza plan, nie srodowisko: brak pliku zrodla i brak katalogu STORAGE przechodza.
# Brak katalogu przy starcie to odmowa EPROTO - przypina ja storage_refusal.
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM src, 1/10 TEXTFILE 'nosuch.txt'" \
  "SELECT src[0] STREAM y FROM src STORAGE MEMORY" >nosource.rql
expect_exit nosource_c 0 "" xretractor nosource.rql -c
printf '%s\n' "STORAGE 'nosuchdir'" "$source_decl" "SELECT src[0] STREAM y FROM src" >nodir.rql
expect_exit nodir_c 0 "" xretractor nodir.rql -c
if [ -e nosuchdir ]; then
  echo "xretractor -c utworzyl katalog STORAGE"
  exit 1
fi

# (6) Nieznany strumien w FROM jest odrzucany przez kompilator jako blad planu.
missing="Stream 'y' reads from 'nosuch', which is not defined in the plan"
printf '%s\n' "STORAGE 'temp'" "$source_decl" "SELECT nosuch[0] STREAM y FROM nosuch STORAGE MEMORY" >unknown.rql
expect_exit unknown_c "$eproto" "$missing" xretractor unknown.rql -c
expect_exit unknown_u "$eproto" "$missing" xretractor unknown.rql -u
expect_exit unknown_start "$eproto" "$missing" xretractor unknown.rql -k -r
for mode in unknown_c unknown_u unknown_start; do
  if grep -qF 'FATAL' "$mode.err"; then
    echo "$mode: nieznany strumien zgloszony jako FATAL"
    cat "$mode.err"
    exit 1
  fi
done
# Kontrola lustrzana: ten sam plan po zadeklarowaniu strumienia przechodzi, wiec kod niesie
# odwolanie, a nie reszta planu.
printf '%s\n' "STORAGE 'temp'" "$source_decl" "DECLARE a INTEGER STREAM nosuch, 1/10 TEXTFILE 'data.txt'" \
  "SELECT nosuch[0] STREAM y FROM nosuch STORAGE MEMORY" >declared.rql
expect_exit declared_c 0 "" xretractor declared.rql -c

# (7) Ten sam blad w dzialajacym serwerze: ad hoc i --reset sa odmowa, serwer zyje i liczy dalej.
# Ad hoc zwraca blad kompilacji bez pozostawienia strumienia w planie.
server_start base.rql -k -r
expect_exit unknown_adhoc "$enoent" "bad rcv:" xqry -a "SELECT nosuch[0] STREAM y2 FROM nosuch STORAGE MEMORY"
kill -0 "$_server_pid"
expect_exit unknown_reset "$eproto" "plan reload refused at reset-commit:" xqry --reset unknown.rql
kill -0 "$_server_pid"
xqry -d >directory.out
if grep -qE '^(y|y2)[[:space:]]' directory.out; then
  echo "odrzucony strumien zostal w planie"
  cat directory.out
  exit 1
fi
xqry -s ok -m 2 >after.out
test "$(grep -c '^[0-9]' after.out)" -eq 2
xqry -k
server_wait_exit
