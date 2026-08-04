#!/usr/bin/env bash
# Tozsamosc R1 nad danymi z NULL-ami: obie strony musza byc rowne co do bitu,
# RAZEM z mapa null. Porownanie samych bajtow by nie wystarczylo — rekord all-null
# i rekord o wartosci zero maja te sama zawartosc binarna, a rozna denotacje.
set -eu
rm -rf temp
mkdir -p temp

xretractor query.rql -c > out_compile.txt

# Obie strony deklaruja to samo opoznienie: 2 sloty przeplotu (ogon) + 3 sloty
# przesuniecia (origin). Rowny payload przy roznej deklaracji to nadal rozny wynik.
#
# Suma 2+3 jest ta sama co dawne tail=5 — przestemplowanie tau_N rozdzielilo ja na
# czlon "jeszcze nie teraz" (ogon) i "tego rekordu nie ma" (origin). Sprawdzamy OBA
# czlony, bo tozsamosc R1 ma zachowywac deklaracje, a nie tylko jej sume.
grep -F 'lhs(1/15)	tail=2	origin=3' out_compile.txt
grep -F 'rhs(1/15)	tail=2	origin=3' out_compile.txt
# Dla delta_C/delta_D=3/2 wlasny ogon # wynosi 2 (maksimum fazowe), a przesuniecia
# skladaja sie na origin 5 — razem 7 slotow, jak poprzednio.
grep -F 'phase_lhs(3/25)	tail=2	origin=5' out_compile.txt
grep -F 'phase_rhs(3/25)	tail=2	origin=5' out_compile.txt

xretractor query.rql -r -k -m 48

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

cmp temp/lhs temp/rhs
# Naglowek .meta zawiera znacznik czasu utworzenia (jedyne bajty niedeterministyczne),
# wiec porownujemy tresc za nim.
cmp <(tail -c +9 temp/lhs.meta) <(tail -c +9 temp/rhs.meta)

# Publiczna LHS i jawna RHS musza pozostac rowne takze w fazie, ktorej
# ceil(delta_D/delta_C) nie zabezpieczal.
cmp temp/phase_lhs temp/phase_rhs
cmp <(tail -c +9 temp/phase_lhs.meta) <(tail -c +9 temp/phase_rhs.meta)

echo OK
