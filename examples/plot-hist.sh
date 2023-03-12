#!/bin/bash

trap control_c SIGINT

control_c()
{
    xqry -k
    pkill plothistgram
    stty sane
}

linuxver=$(uname -r)
if [[ $linuxver == *"Microsoft"* ]] ; then
    TTYPE="--term wxt"
else
    TTYPE=""
fi

echo "Type ctrl+c to stop."

if ! xretractor query-histogram.rql -c; then exit 1 ; fi

nohup xretractor query-histogram.rql &

sleep 4
if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi
xqry -s countHistorgram | plothistgram.py 0 25 "first-argument:red;second-argument:blue" --sleep 0.5 $TTYPE | gnuplot 2>/dev/null