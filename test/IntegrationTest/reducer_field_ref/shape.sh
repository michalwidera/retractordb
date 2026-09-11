#!/bin/bash
# KSZTALT: `-c` odrzuca odwolanie do reduktora w liscie SELECT.
#
# Asercja dotyczy wylacznie tego, CZY plan sie kompiluje i co mowi komunikat — zadnej
# wartosci tu nie ma. Rozdzielenie od value.sh jest celowe (patrz regula przy
# issue202_hash_shift_e2e w CLAUDE.md): jeden wpis ctest nie moze mieszac asercji
# o ksztalcie planu z asercja o tym, co silnik policzyl.
set -e
mkdir -p temp

# Kompilacja ma SIE NIE UDAC. `!` zamiast `|| true`, zeby zielony wynik `xretractor`
# przewrocil test, a nie zostal polkniety.
if xretractor reject.rql -c > reject.txt 2>&1; then
  echo "REGRESJA: plan z odwolaniem do reduktora skompilowal sie; wyjscie:"
  cat reject.txt
  exit 1
fi

# Komunikat idzie kanalem `Check result:`, nazywa reduktor i podaje obejscie.
grep -F 'Check result:' reject.txt
grep -F 'AVG' reject.txt
grep -F 'Materialize the reducer first' reject.txt

# Odrzucony plan nie zostawia artefaktow: bramka stoi PRZED budowaniem czegokolwiek.
if [ -e temp/o ] || [ -e temp/o.desc ]; then
  echo "REGRESJA: odrzucony plan zostawil artefakty strumienia 'o'"
  ls -la temp
  exit 1
fi
