#!/usr/bin/env bash
# Tozsamosc R1 nad danymi z NULL-ami: obie strony musza byc rowne co do bitu,
# RAZEM z mapa null. Porownanie samych bajtow by nie wystarczylo — rekord all-null
# i rekord o wartosci zero maja te sama zawartosc binarna, a rozna denotacje.
set -eu
rm -rf temp
mkdir -p temp

xretractor query.rql -c > out_compile.txt

# Obie strony deklaruja ten sam ogon (2 sloty przeplotu + 3 przesuniecia).
# Rowny payload przy roznym ogonie to nadal rozny wynik.
grep -F 'lhs(1/15)	tail=5' out_compile.txt
grep -F 'rhs(1/15)	tail=5' out_compile.txt
# Dla delta_C/delta_D=3/2 wlasny ogon # wynosi 2 (maksimum fazowe),
# wiec obie strony maja 2+5=7 slotow. Poprzedni wzor wyliczal 6 dla LHS.
grep -F 'phase_lhs(3/25)	tail=7' out_compile.txt
grep -F 'phase_rhs(3/25)	tail=7' out_compile.txt

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
