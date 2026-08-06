#!/bin/bash

trap control_c SIGINT

control_c()
{
    echo "Trapped CTRL-C"
    xqry -k || true
    if [ -t 0 ]; then stty sane; fi
}

STREAM=${1:-str1}
QUERY=${2:-query.rql}
SIZE=${3:-50,200}
XQRY_EXTRA_FLAGS=${4:-}

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

\rm -rf temp && mkdir -p temp
\rm -f nohup.out
nohup xretractor $QUERY -k -r &

# nohup needs a moment to start
sleep  1

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
  *--warmup*) echo "xplot: pomijam okres rozbiegowy strumienia, okno pojawi sie za chwile..." >&2 ;;
esac
{ printf 'bind "Close" "exit gnuplot"\n'; xqry -s "$STREAM" -p "$SIZE" $XQRY_EXTRA_FLAGS; } | gnuplot || true
# gnuplot closed (window X or Ctrl+C) — cleanup
xqry -k || true
if [ -t 0 ]; then stty sane; fi
