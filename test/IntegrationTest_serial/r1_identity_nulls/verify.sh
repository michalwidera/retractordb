#!/usr/bin/env bash
# Tozsamosc R1 nad danymi z NULL-ami:
#   phi(tau_i(A), tau_k(B)) == tau_(i+k)(phi(A, B))   gdy i*deltaA == k*deltaB
#
# ZAKRES TOZSAMOSCI — ustalony 2026-08-07 (decyzja A po pomiarze K24p).
# R1 jest tozsamoscia CIAGU REKORDOW: treaci, indeksu logicznego i poczatku logicznego.
# NIE jest tozsamoscia opoznienia. Gdy faktoryzacja R1 zostanie zastosowana, obie strony
# maja identyczny ogon; gdy jest ZABLOKOWANA (publiczne przesuniecia posrednie), strona
# niefaktoryzowana ma ogon SCISLE WIEKSZY, bo naprawde musi dluzej czekac:
#
#   (C>i)#(D>k) w slocie n czyta (D>k)[j], ktory jest wydawany dopiero w swoim slocie j,
#   czyli k slotow po tym, jak jego tresc powstala w D. Strona (C#D)>(i+k) czyta ta sama
#   tresc bezposrednio z przeplotu, wiec dysponuje nia wczesniej.
#
# Wniosek, ktory ten test utrwala: faktoryzacja R1 jest OPTYMALIZACJA OPOZNIENIA, a nie
# przepisaniem neutralnym. Bezpieczenstwo obserwacyjne (H1) dotyczy tresci i indeksu,
# nie chwili emisji. Do 2026-08-07 obie strony mialy ten sam ogon wylacznie dlatego, ze
# tau_N zawyzalo swoj ogon o min(W_src, N) — zawyzenie zmierzone w K24p §2.2.
#
# Porownanie samych bajtow nie wystarcza — rekord all-null i rekord o wartosci zero maja
# te sama zawartosc binarna, a rozna denotacje — wiec porownujemy takze mape null.
set -eu
rm -rf temp
mkdir -p temp

xretractor query.rql -c > out_compile.txt

# --- deklaracja opoznienia ----------------------------------------------------------
#
# lhs zostaje sfaktoryzowane do postaci prawej, wiec obie strony deklaruja to samo:
# ogon 0 (tau_3 nad przeplotem o ogonie 2 pochlania go w calosci: max(0, 2-3) = 0)
# i origin 3 (przesuniecia skladaja sie na niedefiniowalnosc).
grep -F 'lhs(1/15)	origin=3' out_compile.txt
grep -F 'rhs(1/15)	origin=3' out_compile.txt

# phase_lhs ma faktoryzacje ZABLOKOWANA przez publiczne przesuniecia posrednie, wiec
# wykonuje sie z najgorsza faza przeplotu i jego ogon jest scisle wiekszy. Origin jest
# ten sam po obu stronach — to on niesie tozsamosc.
grep -F 'phase_lhs(3/25)	tail=2	origin=5' out_compile.txt
grep -F 'phase_rhs(3/25)	origin=5' out_compile.txt

xretractor query.rql -r -k -m 48

# --- narzedzia porownania -----------------------------------------------------------
#
# Rozwija mape przebiegow xtrdb w ciag flag null, po jednej na rekord. Bajtowe porownanie
# .meta nie zadziala miedzy strumieniami o roznej dlugosci: ostatni przebieg krotszego
# strumienia ma mniejsza krotnosc, wiec prefiks bajtow rozjezdza sie mimo zgodnej denotacji.
null_flags() {
  xtrdb -n -s "$1" | sed -n 's/.*\[[=~]*\][[:space:]]*\([0-9]*\) records,[[:space:]]*\(no\|all\) nulls.*/\1 \2/p' |
    while read -r count kind; do
      for _ in $(seq 1 "$count"); do echo "$kind"; done
    done
}

# Strona krotsza musi byc PREFIKSEM dluzszej — co do bajtow i co do mapy null.
compare_common_prefix() {
  local left="$1" right="$2" label="$3"
  local size_left size_right common flags_left flags_right records
  size_left=$(stat -c %s "$left")
  size_right=$(stat -c %s "$right")
  common=$(( size_left < size_right ? size_left : size_right ))
  [ "$common" -gt 0 ] || {
    echo "$label: pusty wspolny prefiks — test porownalby dwa puste ciagi"
    exit 1
  }
  cmp -n "$common" "$left" "$right" || {
    echo "$label: rozna tresc na wspolnym prefiksie ($common bajtow)"
    exit 1
  }
  flags_left=$(null_flags "$left" | wc -l)
  flags_right=$(null_flags "$right" | wc -l)
  records=$(( flags_left < flags_right ? flags_left : flags_right ))
  [ "$records" -gt 0 ] || { echo "$label: brak rekordow w mapie null"; exit 1; }
  cmp <(null_flags "$left" | head -n "$records") <(null_flags "$right" | head -n "$records") || {
    echo "$label: rozna mapa null na $records wspolnych rekordach"
    exit 1
  }
}

# Kontrola niepustosci dziedziny: gdyby NULL-e nie doszly do wyniku, test
# porownywalby dane bez NULL-i i nie sprawdzal tego, po co powstal.
nulls=$(xtrdb -n -s temp/lhs | grep -c 'all nulls')
[ "$nulls" -gt 0 ] || {
  echo "brak rekordow all-null w wyniku — dziedzina z NULL-ami nie zostala przetestowana"
  exit 1
}
phase_nulls=$(xtrdb -n -s temp/phase_lhs | grep -c 'all nulls')
[ "$phase_nulls" -gt 0 ] || {
  echo "brak rekordow all-null w fazowej LHS — regresja K2/G3 ma pusta dziedzine NULL"
  exit 1
}

# Faktoryzacja zastosowana po obu stronach: rownosc jest PELNA, lacznie z dlugoscia.
cmp temp/lhs temp/rhs
# Naglowek .meta zawiera znacznik czasu utworzenia (jedyne bajty niedeterministyczne),
# wiec porownujemy tresc za nim.
cmp <(tail -c +9 temp/lhs.meta) <(tail -c +9 temp/rhs.meta)

# Faktoryzacja zablokowana: rownosc jest tozsamoscia ciagu rekordow, wiec porownujemy
# wspolny prefiks. Strona sfaktoryzowana (phase_rhs) ma byc SCISLE DLUZSZA — gdyby byla
# rowna albo krotsza, znaczyloby to, ze optymalizacja opoznienia zniknela i test
# przestalby pilnowac tego, po co powstal.
compare_common_prefix temp/phase_lhs temp/phase_rhs "phase R1"
[ "$(stat -c %s temp/phase_rhs)" -gt "$(stat -c %s temp/phase_lhs)" ] || {
  echo "phase R1: strona sfaktoryzowana nie wyprzedza niefaktoryzowanej — ogon tau_N znow zawyza"
  exit 1
}

echo OK
