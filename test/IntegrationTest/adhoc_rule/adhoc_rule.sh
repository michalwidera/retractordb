#!/bin/bash
# Regula dolaczona przez kanal ad-hoc (issue_238).
#
# Teza: `xqry -a "RULE ... ON <strumien> ... DO DUMP -N TO M"` uzbraja regule na ZYWYM planie,
# a ujemna czesc zakresu siega WYLACZNIE rekordow powstalych po dolaczeniu. Silnik nie wiedzial
# wczesniej, ze ma dla tej reguly cokolwiek gromadzic, wiec historii sprzed dolaczenia nie ma
# prawa wydac, chocby nadal lezala w magazynie — a leży, bo `dumpManager` czyta ja wprost ze
# storage (dumpManager.cpp), po pozycji bezwzglednej.
#
# Obserwabla jest wartosc rekordu: zrodlo rosnie monotonicznie od zera, wiec wartosc N-tego
# rekordu strumienia `dst` rowna sie N. Pierwsza liczba w pliku zrzutu mowi zatem wprost,
# z ktorej chwili pochodzi najstarszy zrzucony rekord. Bez bramki `rule::armAtCount` regula
# odpalilaby na pierwszym rekordzie po dolaczeniu i wciagnela do zrzutu trzy rekordy sprzed
# niego — czyli liczby mniejsze niz stan licznika odczytany przed dolaczeniem.
set -e
. "$(dirname "$0")/../serverlib.sh"

# Zrzut PIERWSZEGO zadzialania reguly. Numer w nazwie bierze sie z RETENTION: bez niego
# kazde kolejne zadzialanie nadpisywaloby ten sam plik, a warunek `dst[0] > 0` jest prawdziwy
# w kazdym slocie. Obserwabla musi byc chwila NAJWCZESNIEJSZA — to ona rozstrzyga o granicy
# historii; okno zrzucone kilka sekund pozniej lezy juz po dolaczeniu tak czy inaczej.
DUMP_FILE=temp/dst_histguard_dump_0.tmp
STREAM_FILE=temp/dst
RECORD_BYTES=4
# Zakres reguly: 20 rekordow wstecz i 3 w przod, czyli 23 rekordy zrzutu. Czesc "w przod"
# musi obejmowac WIECEJ niz jeden slot, inaczej zadanie konczy sie w tym samym slocie, w ktorym
# powstalo, i nigdy nie zdazy go wypchnac z ksiegi zbyt ciasna pojemnosc (patrz `bookfiller`).
HISTORY_DEPTH=20
DUMP_RECORDS=23
# Historia zgromadzona PRZED dolaczeniem: wiecej niz zasieg reguly, zeby brak bramki byl
# widoczny jako wartosc mniejsza od stanu strumienia, a nie jako odczyt sprzed poczatku.
WARMUP_RECORDS=25

# Obserwable czytamy wprost z magazynu strumienia, a nie przez klienta: `xqry -t` podaje
# schemat i takt, licznika rekordow nie wystawia, a plik `temp/dst` jest ciagiem rekordow
# stalej dlugosci, wiec mowi i ile ich jest, i co niosa.
stream_bytes() { stat -c %s "$STREAM_FILE" 2>/dev/null || echo 0; }

# Wartosc ostatniego zapisanego rekordu strumienia.
newest_value() {
  local size
  size=$(stream_bytes)
  [ "$size" -ge "$RECORD_BYTES" ] || return 1
  od -An -td4 -j $((size - RECORD_BYTES)) -N "$RECORD_BYTES" "$STREAM_FILE" | tr -d ' '
}

# Zrzuty z poprzedniego przebiegu przezywaja start serwera: `dropArtifactFile` w launcherze
# kasuje artefakty STRUMIENI (dane, .desc, .meta), plikow regul nie zna, a katalog roboczy
# testu zostaje miedzy wywolaniami ctest. Bez tego sprzatania asercja o granicy historii
# czytalaby zrzut sprzed poprzedniego przebiegu i przechodzila zawsze — sprawdzone.
rm -f temp/*_dump*.tmp

xretractor plan.rql -c
server_start plan.rql

# ---------------------------------------------------------------- odmowy
# Kazda z nich ma zostawic serwer nietkniety: to jest wlasciwa teza tej czesci. Do 2026-09-05
# regula na deklaracji konczyla sie abort() W PROCESIE SERWERA, a pusty zakres DUMP —
# FatalError-em w kompilatorze, czyli tez smiercia instancji z powodu cudzej literowki.
expect_refusal() { # <fragment komunikatu> <zapytanie ad-hoc>
  local needle="$1" query="$2" out rc
  set +e
  out=$(xqry -a "$query" 2>&1)
  rc=$?
  set -e
  if [ "$rc" -eq 0 ]; then
    echo "przyjeto zapytanie, ktore mialo zostac odrzucone: $query"
    echo "odpowiedz: $out"
    exit 1
  fi
  case "$out" in
    *"$needle"*) ;;
    *)
      echo "inny powod odmowy niz oczekiwany dla: $query"
      echo "oczekiwano fragmentu: $needle"
      echo "odpowiedz: $out"
      exit 1
      ;;
  esac
  if ! kill -0 "$_server_pid" 2>/dev/null; then
    echo "serwer zginal po odrzuconym zapytaniu: $query"
    exit 1
  fi
}

expect_refusal "no such stream is defined" 'RULE r ON nosuchstream WHEN nosuchstream[0] > 0 DO DUMP -1 TO 1'
expect_refusal "declaration stream" 'RULE r ON core0 WHEN core0[0] > 0 DO DUMP -1 TO 1'
expect_refusal "is empty" 'RULE r ON dst WHEN dst[0] > 0 DO DUMP 5 TO 5'
expect_refusal "DO DUMP only" "RULE r ON dst WHEN dst[0] > 0 DO SYSTEM 'touch pwned.txt'"
expect_refusal "only the record of the stream" 'RULE r ON dst WHEN core0[0] > 0 DO DUMP -1 TO 1'
if [ -e pwned.txt ]; then
  echo "odrzucona regula SYSTEM mimo wszystko wykonala polecenie"
  exit 1
fi

# ------------------------------------------------------- granica historii
# Czekamy, az strumien uzbiera historie, ktorej regula NIE ma prawa zobaczyc.
for _ in $(seq 1 400); do
  [ "$(stream_bytes)" -ge $((WARMUP_RECORDS * RECORD_BYTES)) ] && break
  sleep 0.05
done
if [ "$(stream_bytes)" -lt $((WARMUP_RECORDS * RECORD_BYTES)) ]; then
  echo "strumien dst nie uzbieral historii przed dolaczeniem reguly ($(stream_bytes) B)"
  exit 1
fi
before=$(newest_value) || {
  echo "nie udalo sie odczytac biezacej wartosci strumienia $STREAM_FILE"
  exit 1
}

# Regula bez RETENTION dolaczana PRZED wlasciwa: jej zadania sa krotkie i wchodza do ksiegi
# zrzutow pierwsze. Do 2026-09-05 pojemnosc ksiegi ustawialo dokladnie to pierwsze zadanie
# ("capacity() == 0"), wiec zostawala 1 na caly czas zycia strumienia i kazde nastepne
# zadanie — takze cudze — wypychalo poprzednie, zamykajac mu deskryptor. Zrzut `histguard`
# nie dobilby wtedy do kompletu i ten test skonczylby sie czerwono.
filler_out=$(xqry -a 'RULE bookfiller ON dst WHEN dst[0] > 0 DO DUMP -1 TO 1' 2>&1) || {
  echo "pomocnicza regula ad-hoc odrzucona: $filler_out"
  exit 1
}

attach_out=$(xqry -a 'RULE histguard ON dst WHEN dst[0] > 0 DO DUMP -20 TO 3 RETENTION 100' 2>&1) || {
  echo "regula ad-hoc odrzucona: $attach_out"
  exit 1
}

# Nazwa powtorzona jest bledem takze wtedy, gdy pierwsza regula juz wisi na planie.
expect_refusal "already defined" 'RULE histguard ON dst WHEN dst[0] > 0 DO DUMP -1 TO 1'

# Zrzut jest kompletny dopiero, gdy ma wszystkie 21 rekordow — dopiero wtedy wiadomo, ze
# regula sie uzbroila, a nie ze wlasnie trwa zapis historii.
expected_bytes=$((DUMP_RECORDS * RECORD_BYTES))
for _ in $(seq 1 400); do
  [ -s "$DUMP_FILE" ] && [ "$(stat -c %s "$DUMP_FILE")" -ge "$expected_bytes" ] && break
  sleep 0.05
done
if [ ! -s "$DUMP_FILE" ] || [ "$(stat -c %s "$DUMP_FILE")" -lt "$expected_bytes" ]; then
  # Dwie przyczyny, obie realne: regula sie nie uzbroila (brak pliku) albo jej zadanie zostalo
  # wypchniete z ksiegi zrzutow przed dokonczeniem (plik krotszy, deskryptor zamkniety).
  echo "zrzut niekompletny: $DUMP_FILE ma $(stat -c %s "$DUMP_FILE" 2>/dev/null || echo brak) B zamiast $expected_bytes B"
  ls -la temp
  echo "odpowiedz xqry -a: $attach_out"
  exit 1
fi

first=$(od -An -td4 -N4 "$DUMP_FILE" | tr -d ' ')
if [ -z "$first" ]; then
  echo "nie udalo sie odczytac pierwszego rekordu zrzutu $DUMP_FILE"
  exit 1
fi
# Ostra nierownosc, nie "wiekszy lub rowny": rekord o wartosci `before` byl NAJNOWSZYM w chwili
# poprzedzajacej dolaczenie, wiec kazdy rekord, ktory regula ma prawo zobaczyc, jest od niego
# pozniejszy. Bez bramki `rule::armAtCount` pierwsze zadzialanie wypadaloby zaraz po dolaczeniu,
# a jego okno siegaloby HISTORY_DEPTH rekordow wstecz — czyli gleboko przed `before`.
if [ "$first" -le "$before" ]; then
  echo "zrzut siega sprzed dolaczenia reguly: pierwszy rekord $first, stan strumienia przed dolaczeniem $before"
  od -An -td4 "$DUMP_FILE"
  exit 1
fi

xqry -k
server_wait_exit
