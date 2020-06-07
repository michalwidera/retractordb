#!/bin/bash

trap control_c SIGINT

control_c()
{
    stty sane
    pkill plotblock
    xqry -k
}

linuxver=$(uname -r)
if [[ $linuxver == *"Microsoft"* ]] ; then
    TTYPE="--term wxt"
else
    TTYPE=""
fi

echo "Type ctrl+c to stop."

if ! xcompiler -q query-dsp.rql ; then exit 1 ; fi

nohup xretractor &

sleep 4
export DISPLAY=:0
xqry -s outputAll | plotblock.py 100 256 "output:red;source:blue" --sleep 0.05 $TTYPE | gnuplot 2>/dev/null
