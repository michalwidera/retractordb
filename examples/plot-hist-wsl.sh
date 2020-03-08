#!/bin/bash

trap control_c SIGINT

control_c()
{
    xqry -k
    pkill plothistgram
    stty sane
}

echo "Type ctrl+c to stop."

if ! xcompiler -q query-histogram.txt ; then exit 1 ; fi

nohup xretractor &

sleep 4
export DISPLAY=:0
xqry -s countHistorgram | plothistgram.py 0 25 "first-argument:red;second-argument:blue" --sleep 0.5 --term wxt | gnuplot 2>/dev/null
