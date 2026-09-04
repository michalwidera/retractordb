#!/usr/bin/env bash
# Potok EKG Pan-Tompkins z examples/ecg/rec205/rec205-qrs.rql — flagowy przyklad
# artykulu (sekcja "Example D") wprowadzony do zestawu ctest.
#
# Test sklada sie z DWOCH niezaleznych porownan, bo kazde z nich lapie co innego:
#
#  1. shape.txt — zadeklarowany ogon i poczatek logiczny KAZDEGO wezla planu,
#     odczytany z trybu -c. To jest wielkosc, ktora zmienilo scalenie 5f31051
#     ("Issue 227 precesja"): przestemplowanie okna @ z poczatku na koniec
#     przedzialu przenioslo rozpietosc okna z ogona do query::logicalOrigin.
#     Porownanie samej tresci strumieni tego nie rozroznia dostatecznie ostro,
#     bo prog koncowy ma tylko trzy rozne wartosci i maskuje przesuniecie
#     o jeden slot.
#
#  2. pattern.txt — pelna tresc czterech strumieni: wyjscia filtru pasmowego
#     (bp_out), jego zdziesiatkowanej obwiedni (bp_dec), calkowania oknem
#     ruchomym (mwi) i koncowej oceny QRS (qrs_out). Etapy posrednie sa tu
#     wazniejsze od wyniku koncowego.
#
# Test jest napisany tak, zeby NIE MOGL przejsc pusty: origin potoku wynosi 236
# slotow, a ogon 359, wiec zbyt krotki prefiks danych dalby strumienie puste
# i porownanie "brak roznic" bez zadnej tresci. Stad jawna kontrola licznosci
# kazdego z czterech strumieni PRZED porownaniem wzorca.
set -eu

readonly kBudget=2000

# Strumien -> oczekiwana liczba rekordow. Wartosci wynikaja z rachunku
# origin/ogona: |{n}| = kBudget - origin - ogon - 1 dla wezlow o interwale 1/360
# (bp_out: 2000-24-359-1 = 1616; mwi: 2000-57-359-1 = 1583;
#  qrs_out: 2000-236-359-1 = 1404), a dla bp_dec o kroku 7 — 227.
readonly kExpected="bp_out:1616 bp_dec:227 mwi:1583 qrs_out:1404"

rm -rf temp shape.txt out.txt
mkdir -p temp

# --- 1. ksztalt planu: ogon i poczatek logiczny kazdego wezla -----------------------
#
# Linia naglowkowa strumienia w trybie -c ma postac
#   nazwa(interwal)<TAB>tail=<W><TAB>origin=<O>
# Filtr zostawia wylacznie te linie, bez cial programow, ktore sa dluzsze
# o dwa rzedy wielkosci i niosa te sama informacje co plan zrodlowy.
xretractor query.rql -c 2> compile.err | grep -E '^[a-z_]+\(' > shape.txt

[ -s shape.txt ] || {
  echo "ecg_qrs: tryb -c nie wypisal ani jednej linii strumienia"
  exit 1
}

diff shape-pattern.txt shape.txt || {
  echo "ecg_qrs: zmienil sie zadeklarowany ogon lub poczatek logiczny wezla planu"
  exit 1
}

# --- 2. tresc strumieni --------------------------------------------------------------
# Flaga -f (--no-clock) zdejmuje czekanie na zegar scienny; os czasu planu,
# wyrownanie slotow i ogon zostaja bez zmian, wiec artefakt jest bajtowo ten sam.
# Rownosc obu sciezek pilnuje it_noclock_offline. UWAGA: w trybie -c litera -f
# znaczy 'fields' w wyjsciu DOT — do wywolan kompilacyjnych jej NIE dodawac.
xretractor query.rql -r -k -m "$kBudget" -f > run.out 2> run.err

for pair in $kExpected; do
  stream=${pair%%:*}
  want=${pair##*:}
  [ -s "temp/$stream" ] || {
    echo "ecg_qrs: strumien $stream nie zawiera rekordow — budzet $kBudget nie pokrywa jego origin i ogona"
    exit 1
  }
  got=$(printf 'open %s\nsize\nquit\n' "$stream" | (cd temp && xtrdb -n) | head -1 | cut -d' ' -f1)
  [ "$got" = "$want" ] || {
    echo "ecg_qrs: strumien $stream ma $got rekordow, oczekiwano $want"
    exit 1
  }
done

(cd temp && xtrdb -n) < term.script > out.txt

diff pattern.txt out.txt || {
  echo "ecg_qrs: tresc strumieni potoku rozni sie od wzorca"
  exit 1
}

echo "OK (shape.txt + 4830 rekordow wzorca)"
