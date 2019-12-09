#!/bin/bash

trap control_c SIGINT

control_c()
{
    ../build/xqry -k
    pkill plotblock
    stty sane
}

echo "Type ctrl+c to stop."

if ! ../build/xcompiler -q query-dsp.txt ; then exit 1 ; fi 

nohup ../build/xabracadabra &

sleep 4
export DISPLAY=:0
../build/xqry -s outputAll | ../scripts/plotblock.py 100 256 "output:red;source:blue" --sleep 0.05 --term wxt | gnuplot 2>/dev/null
