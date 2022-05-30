#!/bin/bash

trap control_c SIGINT

control_c()
{
    xqry -k
    pkill plotblock
    stty sane
}

STREAM=outputAll

if ! xcompiler -q query-dsp.rql ; then exit 1 ; fi

rm nohup.out
nohup xretractor &

sleep 2

echo "Type ctrl+c to stop."

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi
xqry -s $STREAM | plotblock.py 50 256 "output:red;source:blue" --sleep 0.05 | gnuplot 2>/dev/null
