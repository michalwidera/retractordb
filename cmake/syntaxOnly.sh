#!/bin/sh
#
# Launcher kompilacji kontroli rdb_noexcept_check() (CMakeLists.txt). Wykonuje
# reszte linii - launcher ccache, jesli jest, kompilator i jego flagi - z
# -fsyntax-only, a potem tworzy pusty plik wskazany przez -o. -fsyntax-only nie
# pisze pliku obiektowego, a bez niego ninja uznawalby kontrole za nieaktualna
# przy kazdym wywolaniu. Plik zaleznosci (-MD -MF) kompilator pisze jak zwykle.

out=
prev=
for arg in "$@"; do
    if [ "$prev" = "-o" ]; then
        out=$arg
    fi
    prev=$arg
done

"$@" -fsyntax-only && : > "$out"
