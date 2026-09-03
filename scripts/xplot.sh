#!/bin/bash

trap control_c SIGINT

control_c()
{
    echo "Trapped CTRL-C"
    xqry --server "$NAME" -k || true
    if [ -t 0 ]; then stty sane; fi
}

STREAM=${1:-str1}
QUERY=${2:-query.rql}
SIZE=${3:-50,200}
XQRY_EXTRA_FLAGS=${4:-}

# Kazde uruchomienie dostaje wlasna nazwe instancji, domyslnie z katalogu roboczego celu.
# Bez nazwy dwa cele uruchomione rownolegle (np. `ninja dsp` i `ninja simple`) walcza o ta sama
# blokade i te same obiekty IPC: drugi serwer w ogole nie wstaje, jego xqry trafia do cudzej
# instancji, a koncowe sprzatanie zabija cudzy serwer. Nazwa musi spelniac regule xretractor
# (mala litera na poczatku, dalej [a-z0-9_-], najwyzej 32 znaki), stad normalizacja.
NAME=${5:-$(basename "$PWD")}
NAME=$(printf '%s' "$NAME" | tr 'A-Z' 'a-z' | tr -c 'a-z0-9_-' '_' | cut -c1-32)
case "$NAME" in [a-z]*) ;; *) NAME=$(printf 'x%s' "$NAME" | cut -c1-32) ;; esac

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

# Odsiew PRZED czynnoscia niszczaca. `rm -rf temp` kasuje magazyn celu, wiec nie moze wykonac
# sie w sytuacji, w ktorej i tak nie wystartujemy: drugie uruchomienie tego samego celu zabralo by
# dane dzialajacej instancji, zanim xretractor zdazylby odmowic startu na blokadzie.
if xqry --server "$NAME" -l > /dev/null 2>&1 ; then
    echo "xplot: instance '$NAME' is already running; stop it (xqry --server $NAME -k) or pass a different name as the fifth argument" >&2
    exit 1
fi

\rm -rf temp && mkdir -p temp
\rm -f nohup.out
nohup xretractor $QUERY --name "$NAME" -k -r &
XRETRACTOR_PID=$!

# Czekamy na gotowosc WLASNEJ instancji zamiast na staly `sleep 1`. Petla pilnuje takze, czy
# proces serwera jeszcze zyje: odmowa startu (zajeta nazwa) konczy go od razu, a wtedy nie wolno
# isc dalej — klient poszedlby do cudzej instancji i to ja zabiloby sprzatanie.
READY=
for _ in $(seq 100); do
    if ! kill -0 "$XRETRACTOR_PID" 2>/dev/null ; then break ; fi
    if xqry --server "$NAME" -l > /dev/null 2>&1 ; then READY=1 ; break ; fi
    sleep 0.1
done
if [ -z "$READY" ]; then
    echo "xplot: instance '$NAME' failed to start; see nohup.out and the xretractor log" >&2
    kill "$XRETRACTOR_PID" 2> /dev/null
    exit 1
fi

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi

# Bez --warmup xqry rysuje od pierwszego rekordu i okno pojawia sie od razu — tak dziala
# wiekszosc celow, bo ich strumienie nie maja czego odcinac. Z --warmup xqry odrzuca podana
# liczbe rekordow i czeka na pelny kadr (patrz Formatter::gnuplot_warmup_), wiec przez ten
# czas gnuplot nie dostaje zadnego `plot` i nie tworzy okna. Bez ostrzezenia wyglada to na
# zawieszenie, dlatego mowimy o tym tylko wtedy, gdy rozbieg jest faktycznie wlaczony.
case "$XQRY_EXTRA_FLAGS" in
  *--warmup*) echo "xplot: skipping the stream warm-up period; the plot window will appear shortly..." >&2 ;;
esac
{ printf 'bind "Close" "exit gnuplot"\n'; xqry --server "$NAME" -s "$STREAM" -p "$SIZE" $XQRY_EXTRA_FLAGS; } | gnuplot || true
# gnuplot closed (window X or Ctrl+C) — cleanup
xqry --server "$NAME" -k || true
if [ -t 0 ]; then stty sane; fi
