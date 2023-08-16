#!/bin/bash

trap control_c SIGINT

control_c()
{
    xqry -k
    pkill plotblock
    pkill gnuplot
    stty sane
}

STREAM=outputAll
QUERY=query.rql

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

\rm -f nohup.out
\rm -f outputAll*
\rm -f output*
\rm -f signalRow*
\rm -f accRow*
\rm -f source.desc
\rm -f filter.desc
nohup xretractor $QUERY </dev/null >/dev/null 2>&1 &

sleep 2

echo "Type ctrl+c to stop."

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi
xqry -s $STREAM | plotblock.py 50 256 "output:red;source:blue" --sleep 0.05 | gnuplot 2>/dev/null
