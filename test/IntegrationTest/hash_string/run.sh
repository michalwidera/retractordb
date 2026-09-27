#!/bin/bash
# Przebieg planu query.rql i odczyt artefaktow przez xtrdb, porownany z pattern.txt.
#
# Uzycie: run.sh <xtrdb> [opakowanie...] - opakowanie (valgrind) poprzedza xretractor.
#
# Wiersze pa/pb to rekordy zrodel przepuszczone bez przeplotu. Wiersze h/g sa z nich
# wyprowadzone: przy rownych interwalach `x#y` zaczyna od skladnika PRAWEGO. Kazdy przypadek
# graniczny stoi we wzorcu jawnie: napis wypelniajacy slot co do bajtu (alphabet, 16 x C,
# sixteencharslong) i krotki napis zaraz po dlugim (ab po 16 x C, x po sixteencharslong) - ten
# ostatni lapie bajty poprzedniego rekordu, ktore zostawalyby w szerszym slocie.
set -e
xtrdb="$1"
shift
rm -f ./*.desc ./*.meta ./*.shadow pa pb h g out.txt
"$@" xretractor query.rql -m 8 -f >verbose.txt
"$xtrdb" noprompt <term.script >out.txt
bash ../compare.sh --ignore-eol pattern.txt out.txt
