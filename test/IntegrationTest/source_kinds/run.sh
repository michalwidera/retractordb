#!/bin/bash
# #346: rodzaj zrodla deklarowanego wynika ze slowa kluczowego, nigdy ze sciezki.
#
# Do tej zmiany jedyna forma bylo `DECLARE ... FILE`, a format wybieral silnik ze sciezki:
# `.txt` w dowolnym miejscu dawalo parser tekstowy, kazda inna sciezka - surowe bajty. Plik
# tekstowy bez `.txt` dawal smieci bez bledu, a FIFO bez pisarza wieszalo start w open().
#
# Test sprawdza po kolei:
#  1. BINFILE, TEXTFILE i DEVICE czytaja wedlug slowa, takze wbrew nazwie pliku,
#  2. DEVICE na FIFO o nazwie `.txt` czyta surowe bajty - ani tekst, ani token NULL,
#  3. odmowy rodzaju pliku: kod 71, nazwa strumienia i sciezka, bez zawieszenia na FIFO,
#  4. forma przestarzala daje bez --verbose dokladnie to samo wyjscie co jawne slowa,
#     a z --verbose jedno ostrzezenie na deklaracje - takze przy imporcie ad-hoc,
#  5. `.desc` deklaracji: dawny TYPE DEVICE pliku binarnego jest zastepowany, inny rozjazd TYPE
#     albo REF jest odmowa,
#  6. ONESHOT i --until-eof dla BINFILE i TEXTFILE bez zmian.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf temp && mkdir temp
rm -f feed.txt nowriter.fifo ./*.rql.out

# Rekordy INTEGER z bajtow ASCII, w kolejnosci maszyny jak w `read_binary_values`.
ABCD=1145258561
NULL_AS_BYTES=1280070990

# (1) Slowo kluczowe wybiera format.
xretractor kinds.rql -k -r -f -m 4
xtrdb noprompt <kinds.script >out-kinds.txt
bash ../compare.sh --ignore-eol pattern-kinds.txt out-kinds.txt

# (2) DEVICE na FIFO z nazwa `.txt`: surowe bajty, `NULL` to cztery litery, a zero to zero.
# Pisarz trzyma FIFO otwarte z danymi z gory: w -f odczyt DEVICE jest jedna proba nieblokujaca
# (#347), wiec kazdy rekord musi juz czekac w rurze. Danych jest z zapasem.
mkfifo feed.txt
exec 3<>feed.txt
printf 'NULL\000\000\000\000ABCDABCDABCDABCDABCD' >&3
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM dev, 1 DEVICE 'feed.txt'" \
  "SELECT dev[0] STREAM odev FROM dev" >device.rql
run_timeout 30 xretractor device.rql -k -r -f -m 4
exec 3>&-
dev_out=$(printf 'storage temp\nopen odev\nlist 3\nquit\n' | xtrdb noprompt | tr -d '\r')
dev_expected=$(printf '{ odev_0:%s }\n{ odev_0:0 }\n{ odev_0:%s }' "$NULL_AS_BYTES" "$ABCD")
if [ "$dev_out" != "$dev_expected" ]; then
  echo "DEVICE na FIFO feed.txt: oczekiwano"
  echo "$dev_expected"
  echo "dostal:"
  echo "$dev_out"
  exit 1
fi

# (3) Odmowy rodzaju pliku. FIFO bez pisarza: open() zawiesilby start, wiec kontrola idzie przez
# stat() przed otwarciem. Limit czasu zamienia ewentualne zawieszenie w czytelna porazke (124).
mkfifo nowriter.fifo
expect_plan_refused() {
  local plan="$1" reason="$2" out rc
  set +e
  out=$(run_timeout 20 xretractor "$plan" -k -r -f -m 2 2>&1)
  rc=$?
  set -e
  if [ "$rc" -ne 71 ]; then
    echo "$plan: kod $rc zamiast 71 (124 = zawieszenie): $out"
    exit 1
  fi
  case "$out" in
    *"$reason"*) ;;
    *)
      echo "$plan: brak powodu '$reason'; dostal: $out"
      exit 1
      ;;
  esac
}
expect_refused() {
  printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM src, 1 $1" "SELECT src[0] STREAM o FROM src" >refused.rql
  expect_plan_refused refused.rql "$2"
  rm -f temp/src.desc
}
expect_refused "BINFILE 'nowriter.fifo'" "stream 'src': BINFILE 'nowriter.fifo' is a FIFO, not a regular file"
expect_refused "TEXTFILE 'nowriter.fifo'" "stream 'src': TEXTFILE 'nowriter.fifo' is a FIFO, not a regular file"
expect_refused "BINFILE '/dev/null'" "stream 'src': BINFILE '/dev/null' is a character device, not a regular file"
expect_refused "TEXTFILE '/dev/null'" "stream 'src': TEXTFILE '/dev/null' is a character device, not a regular file"
expect_refused "DEVICE 'ascii.txt'" "stream 'src': DEVICE 'ascii.txt' is a regular file, not a character device or FIFO"
# Forma przestarzala na FIFO spoza /dev: regula wybiera BINFILE, a odmowa podpowiada DEVICE.
expect_refused "FILE 'nowriter.fifo'" \
  "is a FIFO, not a regular file (deprecated FILE resolved this path as BINFILE; declare it with DEVICE)"

# (4) Forma przestarzala. Te same zrodla raz przez FILE (rozstrzygniete zamrozona regula), raz
# jawnymi slowami. Bez --verbose wyjscie obu musi byc identyczne co do bajtu - stare plany,
# testy i narzedzia pomiarowe nie moga zobaczyc zmiany.
cp ascii.txt ascii.dat
cp values.dat values.txt
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM bin, 1 FILE 'ascii.dat'" \
  "DECLARE b INTEGER STREAM txt, 1 FILE 'values.txt'" "DECLARE c INTEGER STREAM zero, 1 FILE '/dev/zero'" \
  "SELECT bin[0] STREAM obin FROM bin" "SELECT txt[0] STREAM otxt FROM txt" "SELECT zero[0] STREAM ozero FROM zero" >legacy.rql
sed -e "s/FILE 'ascii.dat'/BINFILE 'ascii.dat'/" -e "s/FILE 'values.txt'/TEXTFILE 'values.txt'/" \
  -e "s|FILE '/dev/zero'|DEVICE '/dev/zero'|" legacy.rql >explicit.rql
if grep -q " FILE '" explicit.rql; then
  echo "explicit.rql nie dostal jawnych slow"
  exit 1
fi

xretractor legacy.rql -c >legacy-c.out 2>&1
xretractor explicit.rql -c >explicit-c.out 2>&1
cmp legacy-c.out explicit-c.out

rm -rf temp && mkdir temp
xretractor legacy.rql -k -f -m 4 >legacy-run.out 2>&1
mv temp legacy-temp && mkdir temp
xretractor explicit.rql -k -f -m 4 >explicit-run.out 2>&1
cmp legacy-run.out explicit-run.out
for stream in obin otxt ozero; do
  cmp "legacy-temp/$stream" "temp/$stream"
done
rm -rf legacy-temp

# Z --verbose: jedno ostrzezenie na deklaracje, z wierszem i wybranym slowem; jawne slowa - zadnego.
xretractor legacy.rql -c -v >/dev/null 2>legacy-v.err
xretractor explicit.rql -c -v >/dev/null 2>explicit-v.err
for warning in "line 2: DECLARE bin: FILE 'ascii.dat' is deprecated, resolved as BINFILE" \
  "line 3: DECLARE txt: FILE 'values.txt' is deprecated, resolved as TEXTFILE" \
  "line 4: DECLARE zero: FILE '/dev/zero' is deprecated, resolved as DEVICE"; do
  if [ "$(grep -cF "warning: $warning" legacy-v.err)" -ne 1 ]; then
    echo "brak albo powtorzenie ostrzezenia: $warning"
    cat legacy-v.err
    exit 1
  fi
done
if [ "$(grep -c "is deprecated" legacy-v.err)" -ne 3 ] || grep -q "is deprecated" explicit-v.err; then
  echo "zla liczba ostrzezen: legacy=$(grep -c "is deprecated" legacy-v.err), explicit:"
  cat explicit-v.err
  exit 1
fi

# Import ad-hoc: o ostrzezeniu decyduje --verbose SERWERA, ostrzezenie idzie na jego stderr.
# Ten sam kanal odrzuca DECLARE zlego rodzaju, a serwer zyje dalej.
adhoc_round() {
  local verbose="$1" err="$2" out
  rm -rf temp && mkdir temp
  # shellcheck disable=SC2086
  server_start explicit.rql $verbose 2>"$err"
  out=$(xqry -a "DECLARE a INTEGER STREAM extra, 1 FILE 'values.txt'" 2>&1) || {
    echo "ad-hoc DECLARE w formie przestarzalej odrzucony: $out"
    exit 1
  }
  set +e
  out=$(xqry -a "DECLARE a INTEGER STREAM bad, 1 BINFILE 'nowriter.fifo'" 2>&1)
  local rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "ad-hoc BINFILE na FIFO przyjety: $out"
    exit 1
  fi
  case "$out" in
    *"stream 'bad': BINFILE 'nowriter.fifo' is a FIFO, not a regular file"*) ;;
    *)
      echo "ad-hoc BINFILE na FIFO: brak powodu; dostal: $out"
      exit 1
      ;;
  esac
  if ! kill -0 "$_server_pid" 2>/dev/null; then
    echo "serwer zginal po odrzuconym ad-hoc DECLARE"
    exit 1
  fi
  xqry -k
  server_wait_exit
}
adhoc_round -v adhoc-v.err
if [ "$(grep -cF "warning: line 1: DECLARE extra: FILE 'values.txt' is deprecated, resolved as TEXTFILE" adhoc-v.err)" -ne 1 ]; then
  echo "serwer z --verbose nie ostrzegl o ad-hoc DECLARE ... FILE:"
  cat adhoc-v.err
  exit 1
fi
adhoc_round "" adhoc-quiet.err
if grep -q "is deprecated" adhoc-quiet.err; then
  echo "serwer bez --verbose ostrzegl o formie przestarzalej:"
  cat adhoc-quiet.err
  exit 1
fi

# (5) `.desc` deklaracji. Magazyn bierze TYPE i REF z pliku, nie z planu. Dawny zapis pliku
# binarnego (TYPE DEVICE) zastepuje start; kazdy inny rozjazd jest odmowa przed startem.
rm -rf temp && mkdir temp
printf '{\tINTEGER a\n\tREF "ascii.dat"\n\tTYPE DEVICE\n}' >temp/bin.desc
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM bin, 1 FILE 'ascii.dat'" "SELECT bin[0] STREAM obin FROM bin" >desc.rql
xretractor desc.rql -k -r -f -m 2
if ! grep -q "TYPE BINFILE" temp/bin.desc; then
  echo "dawny TYPE DEVICE nie zostal zastapiony:"
  cat temp/bin.desc
  exit 1
fi
sed -i.bak "s/FILE 'ascii.dat'/TEXTFILE 'ascii.dat'/" desc.rql
expect_plan_refused desc.rql \
  "stream 'bin': temp/bin.desc was written for TYPE BINFILE and the plan declares TYPE TEXTSOURCE"
sed -i.bak "s/TEXTFILE 'ascii.dat'/BINFILE 'ascii.txt'/" desc.rql
expect_plan_refused desc.rql "stream 'bin': temp/bin.desc was written for source 'ascii.dat' and the plan reads 'ascii.txt'"

# (6) ONESHOT i --until-eof dla plikow bez zmian: 2 rekordy z 8 bajtow, 3 z trzech wierszy.
rm -rf temp && mkdir temp
printf '%s\n' "STORAGE 'temp'" "DECLARE a INTEGER STREAM bin, 1 BINFILE 'ascii.txt' ONESHOT" \
  "SELECT bin[0] STREAM obin FROM bin" >oneshot_bin.rql
xretractor oneshot_bin.rql -k -r -f -u
printf '%s\n' "STORAGE 'temp'" "DECLARE b INTEGER STREAM txt, 1 TEXTFILE 'values.dat'" \
  "SELECT txt[0] STREAM otxt FROM txt" >untileof_txt.rql
xretractor untileof_txt.rql -k -r -f -u
if [ "$(record_count temp/obin 4)" -ne 2 ] || [ "$(record_count temp/otxt 4)" -ne 3 ]; then
  echo "ONESHOT/--until-eof: obin=$(record_count temp/obin 4) otxt=$(record_count temp/otxt 4) rekordow, oczekiwano 2 i 3"
  exit 1
fi

rm -f feed.txt nowriter.fifo
echo "source_kinds OK"
