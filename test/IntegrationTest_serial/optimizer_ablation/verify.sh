#!/usr/bin/env bash
set -eu

xretractor_bin=$1
mode=$2
dedup=$3
share=$4
commutative=$5
factor=$6
probe=$7
simplify=$8

build_info=$("$xretractor_bin" --build-info)
expected_info=$(printf '%s\n' \
  "RDB_OPT_DEDUP_SUBSTRATES=$dedup" \
  "RDB_OPT_SHARE_EQUIVALENT_SELECTS=$share" \
  "RDB_OPT_COMMUTATIVE_ADD=$commutative" \
  "RDB_OPT_FACTOR_MATCHED_HASH_TIMEMOVES=$factor" \
  "RDB_BENCH_PROBE=$probe" \
  "RDB_OPT_SIMPLIFY_EXPRESSIONS=$simplify")

if [ "$build_info" != "$expected_info" ]; then
  echo "optimizer build info mismatch"
  echo "expected:"
  echo "$expected_info"
  echo "actual:"
  echo "$build_info"
  exit 1
fi

if [ "$mode" = "build-info" ]; then
  echo OK
  exit 0
fi

rm -rf temp
rm -f ./*.desc out_compile.txt out_probe.txt
mkdir -p temp

RDB_BENCH_PLAN=1 "$xretractor_bin" query.rql -c > out_compile.txt 2> out_probe.txt

expected_r1=0
expected_r2=0
# Cztery przepisania przy factor=ON: factored, mixed_hash oraz multi1 i multi2.
# Dwa ostatnie są możliwe dopiero od zniesienia warunku jednego konsumenta.
[ "$factor" = "ON" ] && expected_r1=4
[ "$commutative" = "ON" ] && expected_r2=1
# query.rql nie ma w polach ani jednej stałej do zwinięcia — R3 nie ma tu czego przepisać
# niezależnie od przełącznika, i to jest treść oczekiwania: reguła nie rusza wyrażeń,
# w których nie ma stałych.
expected_r3=0

if [ "$probe" = "ON" ]; then
  grep -Fx "REWRITE_APPLIED r1=$expected_r1 r2=$expected_r2 r3=$expected_r3" out_probe.txt

  # Czas kompilacji (K6). Wartość musi być dodatnia, a narzut sondy odjęty —
  # mutacja usuwająca odjęcie zostawia sonda=0 i wtedy ten test nie zabija,
  # dlatego sprawdzany jest osobno warunek narzut > 0.
  compile_ns=$(sed -n 's/^COMPILE_NS \([0-9]*\) sonda=\([0-9]*\)$/\1/p' out_probe.txt)
  probe_ns=$(sed -n 's/^COMPILE_NS \([0-9]*\) sonda=\([0-9]*\)$/\2/p' out_probe.txt)
  [ -n "$compile_ns" ] && [ "$compile_ns" -gt 0 ]
  [ -n "$probe_ns" ] && [ "$probe_ns" -gt 0 ]

  # Rozmiar buforów (K6). Plan ablacyjny ma strumienie z historią, więc suma
  # pojemności i maksimum muszą być dodatnie; zero oznaczałoby, że raport nie
  # czyta maxCapacity.
  cap_streams=$(sed -n 's/^PLAN capacity: strumieni=\([0-9]*\) suma=\([0-9]*\) maks=\([0-9]*\)$/\1/p' out_probe.txt)
  cap_total=$(sed -n 's/^PLAN capacity: strumieni=\([0-9]*\) suma=\([0-9]*\) maks=\([0-9]*\)$/\2/p' out_probe.txt)
  cap_max=$(sed -n 's/^PLAN capacity: strumieni=\([0-9]*\) suma=\([0-9]*\) maks=\([0-9]*\)$/\3/p' out_probe.txt)
  [ -n "$cap_streams" ] && [ "$cap_streams" -gt 0 ]
  [ "$cap_total" -ge "$cap_max" ] && [ "$cap_max" -gt 0 ]
else
  ! grep -F "REWRITE_APPLIED" out_probe.txt
  ! grep -F "COMPILE_NS" out_probe.txt
  ! grep -F "PLAN capacity" out_probe.txt
fi

stream_source() {
  awk -v stream="$1" '
    $0 ~ "^" stream "\\(" { in_stream = 1; next }
    in_stream && /^\t:- PUSH_STREAM/ {
      sub(/^\t:- PUSH_STREAM\(/, "")
      sub(/\)$/, "")
      print
      exit
    }
    in_stream && /^[^\t]/ { exit }
  ' out_compile.txt
}

if [ "$share" = "ON" ]; then
  same1_source=$(stream_source same1)
  same2_source=$(stream_source same2)
  [ "${same1_source#STREAM_SELECT_}" != "$same1_source" ]
  [ "$same1_source" = "$same2_source" ]

  if [ "$commutative" = "ON" ]; then
    [ "$(stream_source commuted)" = "$same1_source" ]
  else
    [ "$(stream_source commuted)" = "B" ]
  fi
else
  ! grep -F 'STREAM_SELECT_' out_compile.txt
  [ "$(stream_source same1)" = "A" ]
  [ "$(stream_source same2)" = "A" ]
  [ "$(stream_source commuted)" = "B" ]
fi

if [ "$dedup" = "ON" ]; then
  ! grep -F 'STREAM_ADD_DA_DB(' out_compile.txt
  grep -F ':- PUSH_STREAM(dedup_owner)' out_compile.txt
else
  grep -F 'STREAM_ADD_DA_DB(' out_compile.txt
  grep -F ':- PUSH_STREAM(STREAM_ADD_DA_DB)' out_compile.txt
fi

if [ "$factor" = "ON" ]; then
  grep -F 'STREAM_HASH_FA_FB(' out_compile.txt
  grep -F ':- STREAM_TIMEMOVE(3)' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_FA(' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_FB(' out_compile.txt
else
  ! grep -F 'STREAM_HASH_FA_FB(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_FA(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_FB(' out_compile.txt
fi

if [ "$factor" = "ON" ]; then
  # Substrat przesunięcia dzielony przez konsumenta pasującego do wzorca reguły
  # i niepasującego. Przepisany zostaje wyłącznie pierwszy; drugi musi nadal
  # czytać przesunięcie, a nie przeplot.
  grep -F 'STREAM_HASH_MA_MB(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_MA(' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_MB(' out_compile.txt
  [ "$(stream_source mixed_hash)" = "STREAM_HASH_MA_MB" ]
  [ "$(stream_source mixed_shift)" = "STREAM_TIMEMOVE_MA" ]

  # Wiele zapytań nad tym samym przesuniętym przeplotem: jeden wspólny węzeł
  # przeplotu, oba substraty przesunięć osierocone i usunięte.
  grep -F 'STREAM_HASH_QA_QB(' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_QA(' out_compile.txt
  ! grep -F 'STREAM_TIMEMOVE_QB(' out_compile.txt
  [ "$(grep -c '^STREAM_HASH_QA_QB(' out_compile.txt)" = "1" ]

  # Kolizja nazw: węzeł o nazwie przeplotu jest zapytaniem publicznym, więc
  # reguła musi go pominąć i zostawić collide_user na własnych przesunięciach.
  [ "$(stream_source collide_user)" = "STREAM_TIMEMOVE_CA" ]
  grep -F 'STREAM_TIMEMOVE_CA(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_CB(' out_compile.txt
else
  ! grep -F 'STREAM_HASH_MA_MB(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_MA(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_MB(' out_compile.txt
  ! grep -F 'STREAM_HASH_QA_QB(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_QA(' out_compile.txt
  grep -F 'STREAM_TIMEMOVE_QB(' out_compile.txt
  [ "$(stream_source collide_user)" = "STREAM_TIMEMOVE_CA" ]
fi

if [ "$mode" = "plan" ]; then
  echo OK
  exit 0
fi

RDB_BENCH_MATERIALIZE=1 "$xretractor_bin" query.rql -r -k -m 48 2> out_run.txt

if [ "$probe" = "ON" ]; then
  # Licznik materializacji (K6) ma wyrocznię: zadeklarowana objętość trwała musi
  # równać się sumie rozmiarów plików danych na dysku. Plan ma SUBSTRAT 'memory',
  # więc część zapisów NIE trafia na dysk — bez rozdzielenia trwałych od
  # pamięciowych ta równość by nie zachodziła.
  reported=$(sed -n 's/^MATERIALIZED trwale: dopisania=[0-9]* nadpisania=[0-9]* bajty=\([0-9]*\) .*$/\1/p' out_run.txt)
  on_disk=$(find temp -type f ! -name '*.desc' ! -name '*.meta' ! -name '*.shadow' -printf '%s\n' | awk '{s+=$1} END {print s+0}')
  [ -n "$reported" ] && [ "$reported" = "$on_disk" ]

  # Substraty pamięciowe muszą być policzone, ale nie jako trwałe.
  mem_bytes=$(sed -n 's/^MATERIALIZED .*pamieciowe: dopisania=[0-9]* nadpisania=[0-9]* bajty=\([0-9]*\)$/\1/p' out_run.txt)
  [ -n "$mem_bytes" ] && [ "$mem_bytes" -gt 0 ]
else
  ! grep -F "MATERIALIZED" out_run.txt
fi

# Tozsamosc R1 jest ROWNOSCIA WYNIKOW i NIEROWNOSCIA OPOZNIEN (`thm:shift-match`):
# obie postaci daja ten sam ciag rekordow, a strona sfaktoryzowana ma ogon NIE WIEKSZY.
# Przy factor=ON obie strony schodza sie do jednego ksztaltu i rownosc jest PELNA,
# lacznie z dlugoscia. Przy factor=OFF strona niefaktoryzowana naprawde czeka dluzej
# (czyta skladowe PO ich wlasnym przesunieciu), wiec wydaje mniej rekordow — i to jest
# zachowanie DOZWOLONE przez `def:observable`, ktore zada rownosci `Val`, ale tylko
# `Lat(prawa) <= Lat(lewa)`. Zadanie rownosci dlugosci byloby ostrzejsze niz relacja
# obserwowalnosci; dokladnie tak oblala bramka `public_identity` kampanii K23
# (znalezisko A, decyzja D1 z 2026-08-09, `research_plan.md` §14.20).
#
# Wzorzec przeniesiony z trybu factor-name-collision-semantic nizej, ktory stosuje go
# od 2026-08-07 dla faktoryzacji zablokowanej kolizja nazw.
compare_identity() { # compare_identity <niefaktoryzowana> <sfaktoryzowana> <etykieta>
  left="$1"
  right="$2"
  label="$3"
  if [ "$factor" = "ON" ]; then
    cmp "$left" "$right"
    cmp <(tail -c +9 "$left.meta") <(tail -c +9 "$right.meta")
    return 0
  fi
  size_left=$(stat -c %s "$left")
  size_right=$(stat -c %s "$right")
  common=$(( size_left < size_right ? size_left : size_right ))
  [ "$common" -gt 0 ] || { echo "$label: pusty wspolny prefiks"; exit 1; }
  # Tresc na wspolnym prefiksie musi byc identyczna — to jest rownosc `Val`.
  cmp -n "$common" "$left" "$right"
  # Kierunek nierownosci: strona sfaktoryzowana wyprzedza. Rownosc albo odwrotna
  # nierownosc znaczylaby, ze optymalizacja opoznienia zniknela.
  [ "$size_right" -gt "$size_left" ] || {
    echo "$label: strona sfaktoryzowana nie wyprzedza niefaktoryzowanej"
    exit 1
  }
  # .meta nie porownujemy bajtowo: strumienie roznej dlugosci maja rozna krotnosc
  # ostatniego przebiegu, wiec prefiks bajtow rozjezdza sie mimo zgodnej denotacji.
  # Mape null na wspolnym prefiksie sprawdza it_r1_identity_nulls.
}

if [ "$mode" = "factor-semantic" ]; then
  compare_identity temp/factored temp/factor_reference "factor R1"
elif [ "$mode" = "factor-shared-substrate-semantic" ]; then
  # Konsument niepasujący do wzorca musi dawać ten sam wynik co plan, w którym
  # substrat przesunięcia nie jest z nikim dzielony.
  cmp temp/mixed_shift temp/mixed_shift_reference
  cmp <(tail -c +9 temp/mixed_shift.meta) <(tail -c +9 temp/mixed_shift_reference.meta)
elif [ "$mode" = "factor-name-collision-semantic" ]; then
  # Gdyby reguła użyła ponownie cudzej projekcji, collide_user dostałby pola
  # w odwrotnej kolejności — różnica jest widoczna bajtowo.
  #
  # ZAKRES ROWNOSCI — decyzja A z 2026-08-07. collide_user ma faktoryzacje R1
  # ZABLOKOWANA przez kolizje nazw, wiec wykonuje sie jako (CA>2)#(CB>1), a jego ogon jest
  # SCISLE WIEKSZY od ogona collide_reference zapisanego wprost jako (CA2#CB2)>3: strona
  # niefaktoryzowana czyta skladowe PO ich wlasnym przesunieciu, wiec na te sama tresc
  # czeka dluzej. Rownosc jest wiec tozsamoscia ciagu rekordow (tresc + indeks logiczny
  # + origin), nie opoznienia — porownujemy wspolny prefiks.
  #
  # Do 2026-08-07 obie strony mialy ten sam ogon wylacznie dlatego, ze tau_N zawyzalo swoj
  # o min(W_src, N). Zawyzenie zmierzono w kampanii K24p (§2.2) i zdjeto adresowaniem
  # indeksem logicznym w dataModel::fetchForward.
  size_user=$(stat -c %s temp/collide_user)
  size_reference=$(stat -c %s temp/collide_reference)
  common=$(( size_user < size_reference ? size_user : size_reference ))
  [ "$common" -gt 0 ]
  cmp -n "$common" temp/collide_user temp/collide_reference
  # Origin jest ten sam po obu stronach — to on niesie tozsamosc; rozni sie ogon.
  grep -F 'collide_user(1/15)	tail=2	origin=3' out_compile.txt
  grep -F 'collide_reference(1/15)	origin=3' out_compile.txt
  # Strona sfaktoryzowana ma byc SCISLE DLUZSZA. Rownosc albo odwrotna nierownosc
  # oznaczalaby, ze optymalizacja opoznienia zniknela.
  [ "$size_reference" -gt "$size_user" ]
elif [ "$mode" = "factor-multiquery-semantic" ]; then
  # multi1 i multi2 to ten SAM ksztalt zapytania, wiec ich rownosc jest pelna
  # niezaleznie od przelacznikow — tu porownanie dlugosci nadal obowiazuje.
  cmp temp/multi1 temp/multi2
  cmp <(tail -c +9 temp/multi1.meta) <(tail -c +9 temp/multi2.meta)
  # Wobec postaci jawnie sfaktoryzowanej obowiazuje juz nierownosc opoznien.
  compare_identity temp/multi1 temp/multi_reference "factor R1 (multi)"
elif [ "$mode" = "dedup-exact-semantic" ]; then
  cmp temp/dedup_shifted temp/dedup_reference
  cmp <(tail -c +9 temp/dedup_shifted.meta) <(tail -c +9 temp/dedup_reference.meta)
elif [ "$mode" = "dedup-steady-semantic" ]; then
  steady_records() {
    od -An -v -w8 -td4 "$1" | awk '
      started || $1 != 0 || $2 != 0 {
        started = 1
        print $1, $2
      }
    ' | head -n 20
  }
  cmp <(steady_records temp/dedup_shifted) <(steady_records temp/dedup_reference)
else
  cmp temp/same1 temp/same2
  cmp temp/same1 temp/commuted

  cmp <(tail -c +9 temp/same1.meta) <(tail -c +9 temp/same2.meta)
  cmp <(tail -c +9 temp/same1.meta) <(tail -c +9 temp/commuted.meta)
fi

echo OK
