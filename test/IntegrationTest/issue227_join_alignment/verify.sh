#!/usr/bin/env bash
# Regresja precesji (issue #227). Wartosci oczekiwane sa tu WYPROWADZANE z definicji
# operatorow, nie przepisane z wyjscia silnika — inaczej test utrwalilby dowolne
# zachowanie, w tym to bledne, ktore mial wykryc.
set -eu
rm -rf temp
mkdir -p temp

xretractor query.rql -c > out_compile.txt

# --- deklaracja opoznienia ---------------------------------------------------------
#
# Okno @(1,3) stemplowane koncem przedzialu: rekord n obejmuje pozycje n-2..n, wiec
# rekordy 0 i 1 nie maja definicji (origin 2), a na nic nie trzeba czekac (ogon 0).
# tau_2: rekord n ma tresc rekordu n-2, wiec tez origin 2 i zerowy ogon wlasny.
grep -F 'win(1/10)	origin=2' out_compile.txt
grep -F 'shifted(1/10)	origin=2' out_compile.txt
grep -F 'win_join(1/10)	origin=2' out_compile.txt
# Suma ze skladowa dwukrotnie wolniejsza dokłada slot ogona.
grep -F 'shift_join(1/10)	tail=1	origin=2' out_compile.txt

xretractor query.rql -m 40

# Pierwszy zapisany rekord ma indeks logiczny rowny origin, czyli 2; rekord fizyczny k
# odpowiada indeksowi logicznemu k+2. Zrodla: fast[i] = 100+i, slow[i] = 500+i.

# --- okno zlaczone z wlasnym zrodlem ------------------------------------------------
#
# SELECT * FROM fast+win daje (fast[n], win[n]), a win[n] = (fast[n], fast[n-1], fast[n-2]).
# Rekord fizyczny k to zatem (102+k, 102+k, 101+k, 100+k).
#
# ISTOTA TESTU: pole 0 i pole 1 musza byc ROWNE. Pole 0 to biezaca probka, pole 1 to
# najnowszy element okna — rownosc znaczy dokladnie tyle, ze okno konczy sie na
# biezacej probce. Przy stemplowaniu poczatkiem przedzialu pole 1 bylo fast[n+2],
# czyli okno wyprzedzalo sygnal o cala swoja rozpietosc.
actual_win=$(od -An -v -td4 temp/win_join | xargs)
record_count=$(($(stat -c %s temp/win_join) / 16))
expected_win=$(
  for k in $(seq 0 $((record_count - 1))); do
    echo "$((102 + k)) $((102 + k)) $((101 + k)) $((100 + k))"
  done | xargs
)
[ "$actual_win" = "$expected_win" ] || {
  echo "win_join: okno nie konczy sie na biezacej probce"
  echo "  oczekiwano: $expected_win"
  echo "  otrzymano : $actual_win"
  exit 1
}
[ "$record_count" -gt 0 ] || {
  echo "win_join: brak rekordow — test porownalby dwa puste ciagi"
  exit 1
}

# --- przesuniecie zlaczone ze strumieniem o innym takcie ----------------------------
#
# SELECT * FROM shifted+slow daje (fast[n-2], slow[floor(n/2)]) wg Definicji sumy
# strumieni. Rekord fizyczny k to (100+k, 500+floor((k+2)/2)).
#
# ISTOTA TESTU: pole 0 ma byc STARSZE o 2 od probki biezacej w tej samej chwili.
# Gdy opoznienie siedzialo w ogonie, rekord n niosl fast[n] i tau_2 bylo w parze
# nieodroznialne od operacji pustej.
actual_shift=$(od -An -v -td4 temp/shift_join | xargs)
shift_count=$(($(stat -c %s temp/shift_join) / 8))
expected_shift=$(
  for k in $(seq 0 $((shift_count - 1))); do
    echo "$((100 + k)) $((500 + (k + 2) / 2))"
  done | xargs
)
[ "$actual_shift" = "$expected_shift" ] || {
  echo "shift_join: przesuniecie niewidoczne w parze"
  echo "  oczekiwano: $expected_shift"
  echo "  otrzymano : $actual_shift"
  exit 1
}
[ "$shift_count" -gt 0 ] || {
  echo "shift_join: brak rekordow — test porownalby dwa puste ciagi"
  exit 1
}

echo OK
