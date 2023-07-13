#!/bin/bash

trap control_c SIGINT

control_c()
{
    xqry -k
    pkill plotblock
    pkill gnuplot
    stty sane
}

STREAM=str1
QUERY=query-simple.rql

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

\rm -f nohup.out
nohup xretractor $QUERY </dev/null >/dev/null 2>&1 &

sleep 2

echo "Type ctrl+c to stop."

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi
xqry -s $STREAM | plotblock.py 50 200 "$STREAM[0]:red;$STREAM[1]:blue;$STREAM[2]:green;$STREAM[3]:black" --sleep 0.02 | gnuplot 2>/dev/null
