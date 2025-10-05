#!/bin/bash

trap control_c SIGINT

control_c()
{
    echo "Trapped CTRL-C"
    xqry -k
    stty sane
}

STREAM=${1:-str1}
QUERY=${2:-query.rql}

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

\rm -f nohup.out
nohup xretractor $QUERY </dev/null >/dev/null 2>&1 &

sleep 2

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi
xqry -s $STREAM -p 50,200 | gnuplot 2>/dev/null
