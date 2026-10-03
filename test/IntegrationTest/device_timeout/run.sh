#!/bin/bash
# #347: odczyt DEVICE nie czeka bez konca i nie wstrzymuje reszty silnika. FIFO jest kontrolowanym
# zrodlem DEVICE: pisarz trzymany na deskryptorze 3 (O_RDWR) daje stan "pisarz jest, danych brak",
# FIFO bez pisarza daje EOF.
#
# Test sprawdza po kolei:
#  1. `-c`: jawna klauzula w listingu, ostrzezenie o TIMEOUT dluzszym od interwalu, odmowa TIMEOUT
#     przy przestarzalym FILE z podpowiedzia DEVICE, odmowa ujemnego `[sources] timeout_s`,
#  2. pierwszenstwo terminu: jawna klauzula (takze 0) > `[sources] timeout_s` > 0, a w `-f` zawsze 0;
#     przestarzale `FILE '/dev/urandom'` bierze termin z konfiguracji,
#  3. te same bajty jako BINFILE i jako DEVICE daja te same rekordy pod tymi samymi indeksami,
#     takze za operatorami wielotaktowymi (+, #, -, >, @, &): DEVICE jest czytany na poczatku slotu,
#     a nie slot wczesniej, i nie zmienia to wyniku,
#  4. FIFO bez pisarza: start bez zawieszenia, rekordy all-null przechodza do SELECT,
#  5. --until-eof na DEVICE: przebieg konczy sie sam na EOF po danych, bez rekordu all-null zza
#     konca wejscia, z tym samym artefaktem co BINFILE z --until-eof,
#  6. w trakcie czekania na DEVICE `xqry` dostaje odpowiedz - czekanie nie trzyma blokad modelu,
#  7. TIMEOUT przechodzi przez import ad-hoc i przez `--reset`.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf work && mkdir work && cd work

# Dziennik silnika lezy w $TMPDIR i dzieli go slot przestrzeni nazw, wiec czytamy tylko to, co
# dopisal biezacy krok.
log="${TMPDIR:-/tmp}/xretractor.log"
log_mark() {
  offset=0
  [ -f "$log" ] && offset=$(wc -c <"$log")
  return 0
}
log_since() { tail -c +$((offset + 1)) "$log" 2>/dev/null || true; }
expect_log() {
  if ! log_since | grep -qF -- "$1"; then
    echo "brak w dzienniku: $1"
    log_since | tail -20
    exit 1
  fi
}
expect_contains() {
  case "$1" in
    *"$2"*) ;;
    *)
      echo "oczekiwano: $2"
      echo "dostal: $1"
      exit 1
      ;;
  esac
}
# Rekordy INTEGER o wartosciach od $2 do $3 w kolejnosci bajtow maszyny.
write_ints() {
  python3 -c "import sys, struct; open(sys.argv[1], 'wb').write(b''.join(struct.pack('=i', v) for v in range(int(sys.argv[2]), int(sys.argv[3]) + 1)))" "$@"
}

# (1) -c
printf '%s\n' "DECLARE a INTEGER STREAM fast, 1/10 DEVICE '/dev/zero' TIMEOUT 0.5" "SELECT fast[0] STREAM o FROM fast" >late.rql
xretractor late.rql -c >late-c.out 2>late-c.err
grep -q "timeout=0.5" late-c.out || { echo "-c bez timeout=0.5"; cat late-c.out; exit 1; }
expect_contains "$(cat late-c.err)" "DECLARE fast: TIMEOUT 0.5 s (RQL) is longer than the interval 0.1 s; waiting overruns the slot"

printf '%s\n' "DECLARE a INTEGER STREAM legacy, 1 FILE '/dev/urandom' TIMEOUT 0.1" "SELECT legacy[0] STREAM o FROM legacy" >legacy-timeout.rql
set +e
out=$(xretractor legacy-timeout.rql -c 2>&1)
rc=$?
set -e
[ "$rc" -ne 0 ] || { echo "FILE ... TIMEOUT przeszlo -c"; exit 1; }
expect_contains "$out" "DECLARE legacy: deprecated FILE does not take TIMEOUT; declare the source with DEVICE '/dev/urandom'"

printf '[sources]\ntimeout_s = -1\n' >negative.toml
set +e
out=$(xretractor late.rql -c --config negative.toml 2>&1)
rc=$?
set -e
[ "$rc" -ne 0 ] || { echo "ujemny timeout_s przeszedl"; exit 1; }
expect_contains "$out" "Configuration error: sources.timeout_s must be a number of seconds from 0 to 86400, got -1"

# (2) Pierwszenstwo terminu. Przebieg z zegarem, bo w -f termin jest zawsze 0. Pusty plik jako
# --config wylacza wyszukiwanie warstw, wiec konfiguracja uzytkownika nie wplywa na wynik.
printf '[sources]\ntimeout_s = 0.02\n' >cfg.toml
: >empty.toml
mkdir -p prec
printf '%s\n' "STORAGE 'prec'" \
  "DECLARE a INTEGER STREAM s_rql, 1/10 DEVICE '/dev/zero' TIMEOUT 0.05" \
  "DECLARE a INTEGER STREAM s_zero, 1/10 DEVICE '/dev/zero' TIMEOUT 0" \
  "DECLARE a INTEGER STREAM s_cfg, 1/10 DEVICE '/dev/zero'" \
  "DECLARE a INTEGER STREAM s_legacy, 1/10 FILE '/dev/urandom'" \
  "SELECT s_rql[0] STREAM o FROM s_rql" >timeouts.rql
log_mark
xretractor timeouts.rql -k -r -m 2 --config cfg.toml
expect_log "DEVICE stream 's_rql': effective TIMEOUT 0.05 s (RQL)"
expect_log "DEVICE stream 's_zero': effective TIMEOUT 0 s (RQL)"
expect_log "DEVICE stream 's_cfg': effective TIMEOUT 0.02 s (config)"
expect_log "DEVICE stream 's_legacy': effective TIMEOUT 0.02 s (config)"
log_mark
xretractor timeouts.rql -k -r -m 2 --config empty.toml
expect_log "DEVICE stream 's_cfg': effective TIMEOUT 0 s (default)"
log_mark
xretractor timeouts.rql -k -r -f -m 2 --config cfg.toml
expect_log "DEVICE stream 's_rql': effective TIMEOUT 0 s (no-clock)"
expect_log "DEVICE stream 's_cfg': effective TIMEOUT 0 s (no-clock)"

# (3) Te same bajty jako BINFILE i jako DEVICE. Pisarz trzyma FIFO z calym wejsciem, wiec w -f
# kazda proba nieblokujaca dostaje pelny rekord. 80 pobudek zuzywa 14 z 16 rekordow zrodla.
write_ints src.bin 1 16
write_ints fast.bin 101 164
write_ints slow.bin 501 532
body=$(printf '%s\n' "DECLARE b INTEGER STREAM fast, 1/2 BINFILE 'fast.bin'" \
  "DECLARE c INTEGER STREAM slow, 3/2 BINFILE 'slow.bin'" \
  "SELECT * STREAM o_plain FROM src" \
  "SELECT * STREAM o_sumf FROM src+fast" \
  "SELECT * STREAM o_sums FROM src+slow" \
  "SELECT * STREAM o_hashf FROM src#fast" \
  "SELECT * STREAM o_hashs FROM slow#src" \
  "SELECT * STREAM o_sub FROM src-2" \
  "SELECT * STREAM o_shift FROM src>2" \
  "SELECT * STREAM o_win FROM src@(1,3)" \
  "SELECT * STREAM o_dehash FROM (src#slow)&1")
printf '%s\n%s\n%s\n' "STORAGE 'eqbin'" "DECLARE a INTEGER STREAM src, 1 BINFILE 'src.bin'" "$body" >eq-bin.rql
printf '%s\n%s\n%s\n' "STORAGE 'eqdev'" "DECLARE a INTEGER STREAM src, 1 DEVICE 'src.fifo'" "$body" >eq-dev.rql
mkdir -p eqbin eqdev
xretractor eq-bin.rql -k -r -f -m 80
mkfifo src.fifo
exec 3<>src.fifo
cat src.bin >&3
run_timeout 30 xretractor eq-dev.rql -k -r -f -m 80
exec 3>&-
compared=0
for f in eqbin/*; do
  name=$(basename "$f")
  [ "$name" = src.desc ] && continue
  cmp "$f" "eqdev/$name"
  compared=$((compared + 1))
done
if [ "$(file_size eqdev/o_plain)" -ne 56 ] || [ "$compared" -lt 20 ]; then
  echo "porownanie BINFILE/DEVICE niczego nie sprawdzilo: o_plain=$(file_size eqdev/o_plain) B, plikow=$compared"
  exit 1
fi

# (4) FIFO bez pisarza: otwarcie nieblokujace, odczyt EOF = brak pisarza, rekordy all-null.
# Limit czasu zamienia zawieszenie w czytelna porazke (124).
mkfifo nowriter.fifo
mkdir -p nw
printf '%s\n' "STORAGE 'nw'" "DECLARE a INTEGER STREAM src, 1 DEVICE 'nowriter.fifo'" "SELECT src[0]+1 STREAM o FROM src" >nowriter.rql
run_timeout 30 xretractor nowriter.rql -k -r -f -m 4
nw_out=$(printf 'storage nw\nopen o\nlist 2\nquit\n' | xtrdb noprompt | tr -d '\r')
[ "$nw_out" = "$(printf '{ o_0:null }\n{ o_0:null }')" ] || { echo "FIFO bez pisarza: $nw_out"; exit 1; }

# (5) --until-eof na DEVICE. Pisarz w tle czeka w open(), az silnik otworzy FIFO przy budowie
# modelu, zapisuje cale wejscie i odchodzi, zanim nadejdzie pierwszy slot (0,5 s). Pierwszy EOF po
# danych to wyczerpanie: przebieg konczy sie sam, przed slotem, ktory dostalby all-null.
write_ints five.bin 1 5
mkfifo eof.fifo
mkdir -p eofdev eofbin
printf '%s\n' "STORAGE 'eofdev'" "DECLARE a INTEGER STREAM src, 1/2 DEVICE 'eof.fifo' TIMEOUT 0.4" "SELECT src[0] STREAM o FROM src" >eof-dev.rql
printf '%s\n' "STORAGE 'eofbin'" "DECLARE a INTEGER STREAM src, 1/2 BINFILE 'five.bin'" "SELECT src[0] STREAM o FROM src" >eof-bin.rql
(cat five.bin >eof.fifo) &
writer=$!
run_timeout 60 xretractor eof-dev.rql -k -r -u
wait "$writer"
xretractor eof-bin.rql -k -r -f -u
cmp eofbin/o eofdev/o
[ "$(file_size eofdev/o)" -eq 20 ] || { echo "--until-eof na DEVICE: $(file_size eofdev/o) B zamiast 20"; exit 1; }

# (6) Czekanie na DEVICE nie trzyma blokad modelu. Pusty FIFO z pisarzem: kazdy slot czeka pelne
# 0,8 s z 1 s interwalu, a `xqry -t` (handler bierze blokade epoki) ma odpowiadac od razu.
mkfifo wait.fifo
exec 3<>wait.fifo
mkdir -p wt rs
printf '%s\n' "STORAGE 'wt'" "DECLARE a INTEGER STREAM src, 1 DEVICE 'wait.fifo' TIMEOUT 0.8" "SELECT src[0] STREAM o FROM src" >wait.rql
log_mark
server_start wait.rql -k
sleep 1.2
worst=0
for _ in 1 2 3 4 5 6; do
  t0=$(now_ns)
  xqry -t src >/dev/null
  t1=$(now_ns)
  ms=$(((t1 - t0) / 1000000))
  [ "$ms" -gt "$worst" ] && worst=$ms
  sleep 0.3
done
echo "xqry -t w trakcie czekania na DEVICE: najdluzej ${worst} ms"
[ "$worst" -lt 500 ] || { echo "xqry czekal ${worst} ms - faza DEVICE trzyma blokade modelu"; exit 1; }

# (7) TIMEOUT przez import ad-hoc i przez --reset.
mkfifo adhoc.fifo
log_mark
xqry -a "DECLARE a INTEGER STREAM late, 1 DEVICE 'adhoc.fifo' TIMEOUT 0.3"
expect_log "AdHoc DEVICE stream 'late': effective TIMEOUT 0.3 s (RQL)"

printf '%s\n' "STORAGE 'rs'" "DECLARE a INTEGER STREAM fresh, 1 DEVICE 'wait.fifo' TIMEOUT 0.6" "SELECT fresh[0] STREAM o2 FROM fresh" >reset.rql
log_mark
xqry --reset reset.rql
i=0
until log_since | grep -qF "DEVICE stream 'fresh': effective TIMEOUT 0.6 s (RQL)"; do
  if [ "$i" -ge 300 ]; then
    echo "plan z --reset nie wystartowal w ciagu 30 s"
    log_since | tail -20
    exit 1
  fi
  sleep 0.1
  i=$((i + 1))
done
xqry -k >/dev/null
server_wait_exit
exec 3>&-
echo "OK"
