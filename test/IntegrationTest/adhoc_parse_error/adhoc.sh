#!/bin/bash
# Bledne zapytanie ad-hoc nie ma prawa zakonczyc procesu serwera.
#
# Do 2026-09-05 listenery bledow ANTLR-a w RQLParser.cpp wolaly exit(EPERM). W kliencie
# byl to zwykly kod wyjscia, ale w SERWERZE - smierc calej instancji: `xqry -a "ml"`
# ubijalo xretractora razem z planem i wszystkimi klientami, 5/5 prob. Zadna literowka
# w opcjach klienta nie byla do tego potrzebna, wystarczylo bledne zapytanie.
#
# Test sprawdza trzy rzeczy, bo dopiero razem znacza "serwer przezyl":
#   1. bledne zapytanie konczy sie porazka KLIENTA, z komunikatem o bledzie parsowania,
#   2. proces serwera nadal zyje - takze po tekstach, ktore parser albo filtr kanalu odrzucaja,
#   3. nastepne POPRAWNE zapytanie ad-hoc dziala i dokłada strumien do planu.
#
# Punkt (3) nie jest ozdoba: status parsowania byl przed ta zmiana zmienna plikowa, ktorej
# parserRQLString nie zerowal na wejsciu, wiec samo zdjecie exit() zatrulo by kazde kolejne
# zapytanie w tym procesie. Odpowiednik jednostkowy: xparser.parse_failure_does_not_poison_the_next_parse.
set -e
. "$(dirname "$0")/../serverlib.sh"

xretractor plan.rql -c

# (0) Start z pliku planu: wartosc, ktorej plan nie moze przyjac, jest bledem parsowania jak
# kazdy inny - kod EPROTO i powod w "Parse result:". Trzeci argument zmienia kanal powodu na
# "Check result:", gdy odmowa zapada dopiero w kompilatorze; czwarty to plik --config.
# Kod z nazwy stalej, nie z liczby: EPROTO to 71 na Linuksie i 100 na macOS.
expected_rc=$(errno_value EPROTO)
expect_file_rejected() {
  local plan="$1" reason="$2" channel="${3:-Parse result:}" config="${4:-}" out rc
  set +e
  if [ -n "$config" ]; then
    out=$(xretractor "$plan" -c --config "$config" 2>&1)
  else
    out=$(xretractor "$plan" -c 2>&1)
  fi
  rc=$?
  set -e
  if [ "$rc" -ne "$expected_rc" ]; then
    echo "start z pliku $plan: kod $rc zamiast $expected_rc: $out"
    exit 1
  fi
  case "$out" in
    *"$channel$reason"*) ;;
    *)
      echo "start z pliku $plan nie podal powodu w '$channel'; dostal: $out"
      exit 1
      ;;
  esac
}
# Do 2026-09-25 std::out_of_range z listenera parsera konczyl proces przez std::terminate
# (SIGABRT, kod 134), bez slowa o przyczynie (#306).
echo "DECLARE a INTEGER STREAM core0, 99999999999 FILE 'source.dat'" >out_of_range.rql
expect_file_rejected out_of_range.rql "numeric literal 99999999999 is out of range"
# Zastrzezona nazwa to blad parsera zwracany z kodem EPROTO, nie SIGABRT. W pliku z dwoma
# bledami klient dostaje powod pierwszej niepoprawnej instrukcji.
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" \
  "SELECT core0[0] STREAM OUT_OF_BUSSINESS FROM core0" >reserved_name.rql
expect_file_rejected reserved_name.rql "OUT_OF_BUSSINESS is reserved stream name"
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" \
  "SELECT core0[0] STREAM OUT_OF_BUSSINESS FROM core0" \
  "RULE r ON missing WHEN missing[0] > 0 DO DUMP -1 TO 1" >reserved_first.rql
expect_file_rejected reserved_first.rql "OUT_OF_BUSSINESS is reserved stream name"
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" \
  "RULE r ON missing WHEN missing[0] > 0 DO DUMP -1 TO 1" \
  "SELECT core0[0] STREAM OUT_OF_BUSSINESS FROM core0" >rule_first.rql
expect_file_rejected rule_first.rql "Rule 'r' refers to stream 'missing', but no such stream is defined"
# Do 2026-09-26 zerowy interwal przechodzil parser, a plan padal w kompilatorze na mylacym
# "Circular dependency in stream definitions" - strumien zalezny nie rozwiazywal sie nigdy (#308).
printf '%s\n' "DECLARE a INTEGER STREAM core0, 0 FILE 'source.dat'" "SELECT a[0] STREAM dst FROM core0" >zero_interval.rql
expect_file_rejected zero_interval.rql "interval 0 must be greater than zero"
# Do 2026-09-27 krok 0 konczyl start FatalError-em w kompilatorze (kod 1), a pojemnosc 0 przechodzila
# -c i konczyla proces dopiero przy pierwszym zapisie (#308, A2 C4 i M12).
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" "SELECT * STREAM dst FROM core0@(0,4)" >zero_step.rql
expect_file_rejected zero_step.rql "AGSE step 0 must be greater than zero"
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" "SELECT a[0] STREAM dst FROM core0 RETENTION 0 3" >zero_retention.rql
expect_file_rejected zero_retention.rql "RETENTION capacity 0 must be greater than zero"
# Do 2026-09-27 indeks pola szedl w kompilatorze przez atoi: `core0[4294967296]` przechodzil -c
# i liczyl po cichu `core0[0]` (A2 M10). Ujemny numer instancji w FROM szablonu generatora
# konczyl start FatalError-em (kod 1) - ta sama kompilacja biegnie w kanale `--reset`.
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" "SELECT core0[4294967296] STREAM dst FROM core0" >wrapped_index.rql
expect_file_rejected wrapped_index.rql "numeric literal 4294967296 is out of range"
printf '%s\n' "DECLARE a INTEGER[2] STREAM core0, 1 FILE 'source.dat'" "SELECT core0[\$] STREAM cell[2] FROM core0" \
  "SELECT * STREAM w[2] FROM cell[\$-1]" >negative_family_index.rql
expect_file_rejected negative_family_index.rql "Stream 'w\$0' references 'cell[-1]' - stream generator index must not be negative" \
  "Check result:"
# Do 2026-09-27 wymiar byl ograniczony tylko zakresem int: `>65537` przechodzil -c, a absurdalny
# rozmiar konczyl sie std::bad_alloc albo OOM killerem dopiero w dzialajacym serwerze (A2 M11).
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 FILE 'source.dat'" "SELECT core0[0] STREAM dst FROM core0>65537" >big_shift.rql
expect_file_rejected big_shift.rql "time shift 65537 exceeds the limit 65536"
# Budzet pamieci historii z pliku --config: rekord 512 KiB i przesuniecie o 2 to ok. 2,5 MiB historii
# zrodla. Ten sam plan przechodzi przy budzecie 64 MiB, wiec odmowe daje wylacznie klucz z pliku.
printf '%s\n' "DECLARE a DOUBLE[65536] STREAM core0, 1 FILE 'source.dat'" "SELECT core0[0] STREAM dst FROM core0>2" >history.rql
printf '[limits]\nhistory_memory_mib = 1\n' >small_budget.toml
printf '[limits]\nhistory_memory_mib = 64\n' >large_budget.toml
if ! out=$(xretractor history.rql -c --config large_budget.toml 2>&1); then
  echo "plan history.rql odrzucony przy budzecie 64 MiB: $out"
  exit 1
fi
# Liczby bajtow nie przypinamy - zalezy od sizeof(rdb::payload), czyli od biblioteki standardowej.
expect_file_rejected history.rql "Plan keeps " "Check result:" small_budget.toml
# Do 2026-10-04 operand operatora jednoargumentowego stawal sie osobnym polem, a `-a[0]` konczylo
# start FatalError-em "no program tokens" (A2 C2, #328). `~` poza BYTE/UINT to odmowa kompilatora,
# w SELECT i w warunku RULE.
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 BINFILE 'source.dat'" "SELECT ~a[0] STREAM dst FROM core0" >bit_not.rql
expect_file_rejected bit_not.rql "Stream 'dst': unary '~' is defined only for BYTE and UINT, not for INTEGER" \
  "Check result:"
printf '%s\n' "DECLARE a INTEGER STREAM core0, 1 BINFILE 'source.dat'" "SELECT a[0] STREAM dst FROM core0" \
  "RULE r ON dst WHEN ~dst[0] > 0 DO DUMP -1 TO 1" >bit_not_rule.rql
expect_file_rejected bit_not_rule.rql \
  "Stream 'dst' rule condition: unary '~' is defined only for BYTE and UINT, not for INTEGER" "Check result:"

server_start plan.rql

# (1) Bledne zapytanie: klient MUSI wyjsc niezerowo i powiedziec, ze to blad parsowania.
# xqry pisze diagnostyke na stderr (issue_217), wiec zbieramy oba strumienie.
set +e
bad_out=$(xqry -a "ml" 2>&1)
bad_rc=$?
set -e
if [ "$bad_rc" -eq 0 ]; then
  echo "bledne zapytanie ad-hoc zostalo przyjete (kod 0): $bad_out"
  exit 1
fi
case "$bad_out" in
  *"Fail parse"*) ;;
  *)
    echo "klient nie zglosil bledu parsowania; dostal: $bad_out"
    exit 1
    ;;
esac
# Sam prefiks odmowy nie wystarczy: do 2026-09-05 klient dostawal "Fail parse:Fail", a
# zdanie nazywajace przyczyne zostawalo na stderr PROCESU SERWERA. Tresc bledu wraca
# teraz statusem parsera, czyli ta sama droga co blad semantyczny.
case "$bad_out" in
  *"line 1:"*"mismatched input"*) ;;
  *)
    echo "odmowa nie niosla pozycji ani tresci bledu skladni; dostal: $bad_out"
    exit 1
    ;;
esac

# (2) Serwer zyje. To jest wlasciwa teza tego testu.
if ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "serwer zginal po blednym zapytaniu ad-hoc"
  exit 1
fi

# Program zaczynajacy sie od SELECT nie moze przemycic drugiej instrukcji.
# Parser RQL przyjmuje wiele instrukcji, ale jedna wiadomosc ad-hoc jest jedna
# transakcja SELECT albo DECLARE.
set +e
multi_out=$(xqry -a $'SELECT a[0] STREAM hidden_select FROM core0\nDECLARE a INTEGER STREAM hidden_source, 0.2 FILE '\''source.dat'\''' 2>&1)
multi_rc=$?
set -e
if [ "$multi_rc" -eq 0 ]; then
  echo "wieloinstrukcyjny program ad-hoc zostal przyjety: $multi_out"
  exit 1
fi
case "$multi_out" in
  *"exactly one SELECT or DECLARE"*) ;;
  *)
    echo "nieoczekiwana odpowiedz na wieloinstrukcyjny ad-hoc: $multi_out"
    exit 1
    ;;
esac
if xqry -d | grep -Eq 'hidden_select|hidden_source'; then
  echo "odrzucony program wieloinstrukcyjny zmienil plan serwera"
  xqry -d
  exit 1
fi

# Tekst odrzucany przez parser albo przez filtr kanalu to odmowa z powodem, a nie smierc serwera.
# Do 2026-09-25 kazdy z ponizszych przypadkow konczyl proces:
#   - ROTATION przechodzil filtr, ktory porownywal slowo kluczowe z literalem "PERCOUTNER"
#     (gramatyka takiego slowa nie zna), i trafial na FatalError "parser logic error",
#   - pusta wartosc dyrektywy i pusty FILE w SELECT - FatalError w listenerze parsera,
#   - pusty FILE w DECLARE - przechodzil parser i ginal dopiero na FatalError w rdb::StoragePaths
#     przy rejestracji w modelu, czyli juz po imporcie do zywego planu,
#   - zarezerwowana nazwa strumienia - abort(), czyli SIGABRT,
#   - ulamek z zerowym mianownikiem - FatalError w listenerze parsera,
#   - literal liczbowy spoza zakresu typu - std::out_of_range z listenera, ktory biegnie
#     z noexcept-owego destruktora w generowanym parserze, czyli std::terminate (#306),
#   - zerowy interwal (do 2026-09-26) - DECLARE byl PRZYJMOWANY, a proces konczyl FatalError
#     w qTree::getAvailableTimeIntervals; `&` - FatalError w kompilatorze (#308),
#   - zerowy krok AGSE (do 2026-09-27) - FatalError w kompilatorze; zerowe okno AGSE i zerowa
#     pojemnosc RETENTION byly PRZYJMOWANE, a proces konczyl FatalError przy rejestracji
#     albo dopiero przy pierwszym zapisie (#308, A2 C4 i M12).
#   - indeks pola 2^32-1 (do 2026-09-27) - atoi w kompilatorze dawal indeks -1, ad-hoc odpowiadal
#     "OK", a proces konczyl FatalError przy pierwszym rekordzie (A2 M10).
#   - wymiar w zakresie int, ale absurdalny (do 2026-09-27) - przesuniecie, okno czy szerokosc pola
#     byly PRZYJMOWANE, a proces konczyl std::bad_alloc albo OOM killerem przy budowie magazynu (A2 M11).
# Parser biegnie w procesie DZIALAJACEGO serwera, wiec kazdy z nich byl bledem calej instancji.
# Trzeci argument zmienia prefiks odmowy, gdy zapada ona dopiero w kompilatorze kopii planu.
expect_parse_rejected() {
  local query="$1" reason="$2" prefix="${3:-Fail parse}" out rc
  set +e
  out=$(xqry -a "$query" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "ad-hoc zostal przyjety: $query"
    exit 1
  fi
  case "$out" in
    *"$prefix"*"$reason"*) ;;
    *)
      echo "nieoczekiwana odpowiedz na ad-hoc '$query': $out"
      exit 1
      ;;
  esac
  if ! kill -0 "$_server_pid" 2>/dev/null; then
    echo "serwer zginal po ad-hoc: $query"
    exit 1
  fi
}
expect_parse_rejected "ROTATION 'adhoc.cnt'" "'ROTATION' is not supported"
# Lista dozwolonych obejmuje KAZDA dyrektywe z poprawna wartoscia, nie tylko te, ktora wczesniej
# przeciekala - STORAGE i SUBSTRAT odrzucala dawna lista zakazanych i nie moga teraz przejsc.
expect_parse_rejected "STORAGE 'x'" "'STORAGE' is not supported"
expect_parse_rejected "SUBSTRAT 'memory'" "'SUBSTRAT' is not supported"
expect_parse_rejected "STORAGE ''" "directive STORAGE requires a non-empty value"
expect_parse_rejected "SELECT a[0] STREAM emptyfile FROM core0 FILE ''" "FILE of stream emptyfile requires a non-empty file name"
expect_parse_rejected "DECLARE a INTEGER STREAM emptydecl, 1 FILE ''" "FILE of stream emptydecl requires a non-empty file name"
expect_parse_rejected "SELECT core0[0] STREAM OUT_OF_BUSSINESS FROM core0" "OUT_OF_BUSSINESS is reserved stream name"
expect_parse_rejected "DECLARE a INTEGER STREAM zerorate, 1/0 FILE 'source.dat'" "fraction 1/0 has a zero denominator"
expect_parse_rejected "DECLARE a INTEGER STREAM bigrate, 99999999999 FILE 'source.dat'" \
  "numeric literal 99999999999 is out of range"
expect_parse_rejected "SELECT core0[0]+10000000000000000000000000000000000000000.0 STREAM bigfloat FROM core0" \
  "numeric literal 10000000000000000000000000000000000000000.0 is out of range"
expect_parse_rejected "SELECT core0[0] STREAM bigshift FROM core0>99999999999" "numeric literal 99999999999 is out of range"
expect_parse_rejected "DECLARE a INTEGER STREAM zerorate0, 0 FILE 'source.dat'" "interval 0 must be greater than zero"
expect_parse_rejected "SELECT core0[0] STREAM zerodehash FROM core0 & 0" "interval 0 must be greater than zero"
expect_parse_rejected "SELECT * STREAM zerostep FROM core0@(0,4)" "AGSE step 0 must be greater than zero"
expect_parse_rejected "SELECT * STREAM zerowindow FROM core0@(1,0)" "AGSE window 0 must be greater than zero"
expect_parse_rejected "SELECT a[0] STREAM zeroretention FROM core0 RETENTION 0 3" \
  "RETENTION capacity 0 must be greater than zero"
expect_parse_rejected "SELECT core0[4294967296] STREAM wrapindex FROM core0" "numeric literal 4294967296 is out of range"
expect_parse_rejected "SELECT core0[4294967295] STREAM minusindex FROM core0" "numeric literal 4294967295 is out of range"
# Indeks generatora nie ma literalu spoza zakresu - przepelnia go dopiero arytmetyka, wiec odmowa
# zapada w kompilatorze. Do 2026-09-27 instancja 1 dostawala po cichu `core0[0]`.
expect_parse_rejected "SELECT core0[\$*65536*65536] STREAM genwrap[2] FROM core0" \
  "Stream 'genwrap\$1' references 'core0[\$*65536*65536]' - the index does not fit in int" "Fail local chain compiler"
# A2 M11: granica pojedynczego literalu odpada w parserze, granica rekordu (17 pol po 64 KiB) dopiero
# w kompilatorze kopii planu - kazdy wymiar z osobna miesci sie w swojej granicy.
expect_parse_rejected "SELECT core0[0] STREAM bigshift2 FROM core0>65537" "time shift 65537 exceeds the limit 65536"
wide_list="to_string(core0[0]:65536)"
for _ in $(seq 2 17); do wide_list="$wide_list, to_string(core0[0]:65536)"; done
expect_parse_rejected "SELECT $wide_list STREAM wide FROM core0" \
  "Stream 'wide' needs a record of 1114112 bytes; the limit is 1048576" "Fail local chain compiler"

# A2 C2 (#328): `-pole` ad hoc konczylo serwer (kod 1), a `pole * -pole` bylo przyjmowane i konczylo
# go dopiero w pierwszym slocie (kod 4). `~` nad INTEGER i `-` nad napisem to odmowy kompilatora.
expect_parse_rejected "SELECT ~core0[0] STREAM bitnot FROM core0" \
  "unary '~' is defined only for BYTE and UINT, not for INTEGER" "Fail local chain compiler"
expect_parse_rejected "SELECT -to_string(core0[0]) STREAM negtext FROM core0" \
  "unary '-' is not defined for STRING" "Fail local chain compiler"
neg_out=$(xqry -a 'SELECT core0[0] * -core0[0], -core0[0] + 1 STREAM adhocneg FROM core0' 2>&1) || {
  echo "ad-hoc z jednoargumentowym minusem odrzucony: $neg_out"
  exit 1
}
neg_rows=$(xqry -s adhocneg -m 5 2>/dev/null | wc -l)
if [ "$neg_rows" -lt 5 ] || ! kill -0 "$_server_pid" 2>/dev/null; then
  echo "strumien adhocneg oddal $neg_rows z 5 rekordow albo serwer zginal: $neg_out"
  exit 1
fi

# `kill -0` zaraz po odpowiedzi nie widzi smierci odroczonej: pojemnosc 0 konczyla proces dopiero
# przy pierwszym zapisie, ok. 2 s po "OK". Po odmowach plan serwera ma wiec jeszcze liczyc.
dst_rows=$(xqry -s dst -m 10 2>/dev/null | wc -l)
if [ "$dst_rows" -lt 10 ]; then
  echo "po odmowach strumien dst oddal $dst_rows z 10 rekordow"
  exit 1
fi

# (3) Kolejne poprawne zapytanie nadal dziala.
ok_out=$(xqry -a 'select a[0] stream adhocok from core0' 2>&1) || {
  echo "poprawne zapytanie ad-hoc odrzucone po blednym: $ok_out"
  exit 1
}

# Artefakty powstaja po powrocie z xqry -a; czekamy na nie zamiast na zegar.
for _ in $(seq 1 200); do
  if [ -s temp/adhocok ] && [ -s temp/adhocok.desc ]; then
    break
  fi
  sleep 0.05
done
if ! { [ -s temp/adhocok ] && [ -s temp/adhocok.desc ]; }; then
  echo "brak artefaktow strumienia adhocok; temp zawiera:"
  ls -la temp
  echo "odpowiedz xqry -a: $ok_out"
  exit 1
fi

xqry -k
server_wait_exit
