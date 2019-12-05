#!/bin/bash
export PATH=../build:../scripts:$PATH

trap control_c SIGINT

control_c()
{
    stty sane
    xqry -k
}

echo "Type ctrl+c to stop."

if ! xcompiler -q query-dsp.txt ; then exit 1 ; fi 

nohup xabracadabra &

sleep 4

xqry -s outputAll |\
plotblock.py 100 256 "output:red;source:blue" --sleep 0.02 |\
gnuplot 2>/dev/null
