#!/bin/bash
# Pole tablicowe w wierszu wysylanym do klienta.
#
# Serwer serializuje wiersz jako jedna wartosc na ELEMENT, nie na pole. Zanim tak bylo,
# `count` w wierszu rownal sie liczbie POL i pierwsze N wartosci plaskich szlo pod nazwami
# kolejnych pol -- czyli wartosci pod nazwami sie nie zgadzaly. Poprawka odslonila dwa
# nastepne defekty, oba widoczne wylacznie na strumieniu z tablica, i oba pilnowane tutaj:
#
#   * gnuplot deklarowal jedna krzywa na POLE, a wysylal jeden blok danych na WARTOSC.
#     Gnuplot czyta dokladnie jeden blok na kazde '-', wiec dla INTEGER[3] dlawil sie
#     dwoma nadmiarowymi blokami. Asercja jest strukturalna: liczba krzywych == liczba
#     blokow.
#   * graphite i influx etykietowaly wartosci nazwami pol, wiec z trzech elementow
#     pokazywaly pierwszy i milczaly o reszcie.
#
# Drugi strumien pilnuje granicy transportu: wiersz dluzszy od slotu kolejki odpowiedzi
# (1024 B) przewracal CALA usluge -- try_send rzucal interprocess_exception, a najblizszy
# catch stoi poza petla przetwarzania. Jeden subskrybent szerokiego strumienia zabieral
# serwer wszystkim pozostalym.
set -e
. "$(dirname "$0")/../serverlib.sh"

rm -rf temp && mkdir -p temp
rm -f ./*.desc ./*.meta ./*.shadow

# 200 kolumn generujemy tutaj: wartosci nie znacza nic, liczy sie DLUGOSC wiersza, a plik
# z dwiema setkami liczb w repozytorium bylby tylko szumem w przegladzie zmian.
python3 -c "print(' '.join(str(i % 10) for i in range(200)))" > wide.txt

server_start query.rql -k

# --- Format surowy: wszystkie trzy elementy, w kolejnosci deklaracji ---
xqry -s arr -m 1 -r > raw.txt
bash ../compare.sh --ignore-eol raw.pattern raw.txt

# --- Graphite: element dostaje wlasny czlon sciezki (kropka = hierarchia) ---
# Znacznik czasu bierze sie z zegara, wiec do porownania zostaje sama tresc.
xqry -s arr -m 1 -g | cut -d' ' -f1,2 > graphite.txt
bash ../compare.sh --ignore-eol graphite.pattern graphite.txt

# --- InfluxDB: element dostaje wlasny klucz pola (podkreslenie, bez escapowania) ---
xqry -s arr -m 1 -f | sed 's/ [0-9]*$//' > influx.txt
bash ../compare.sh --ignore-eol influx.pattern influx.txt

# --- Gnuplot: struktura polecenia zgodna z liczba blokow danych ---
xqry -s arr -m 1 -p 5,0,20 > gnuplot.txt
# Wyjscie gnuplota konczy linie CRLF, wiec licznik blokow musi zdjac CR - inaczej '^e$'
# nie trafia nigdy i asercja przechodzi na zerze po obu stronach porownania.
curves=$(grep -m1 '^plot' gnuplot.txt | grep -o "'-'" | wc -l)
blocks=$(tr -d '\r' < gnuplot.txt | grep -c '^e$' || true)
if [ "$curves" -ne 3 ] || [ "$blocks" -ne 3 ]; then
  echo "gnuplot: $curves krzywych wobec $blocks blokow danych, oczekiwano 3 i 3"
  cat gnuplot.txt
  exit 1
fi

# --- Wiersz niemieszczacy sie w slocie kolejki: brak danych, ale usluga zyje ---
# Klient konczy dopiero wlasnym limitem "brak danych" (10 s) - stad budzet czasu testu.
xqry -s wide -m 1 -r > wide_out.txt 2>/dev/null || true
if [ -s wide_out.txt ]; then
  echo "strumien 'wide' nie miesci sie w slocie kolejki, a mimo to cos przyslal:"
  cat wide_out.txt
  exit 1
fi

# Dowodem zycia serwera jest kolejna PRAWIDLOWA emisja, nie sam zywy proces: instancja,
# ktora stracila watek przetwarzania, tez odpowiada jeszcze przez chwile.
xqry -s arr -m 1 -r > raw_after.txt
bash ../compare.sh --ignore-eol raw.pattern raw_after.txt

xqry -k
server_wait_exit
