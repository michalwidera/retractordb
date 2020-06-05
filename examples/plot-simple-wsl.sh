#!/bin/bash

trap control_c SIGINT

control_c()
{
    xqry -k
    pkill plotblock
    stty sane
}

me="$(basename "$(test -L "$0" && readlink "$0" || echo "$0")")"

if [ "$1" == "-h" ] ; then
    echo "Usage: $me query-file.rql streamName x-dimension y-dimension"
    echo "Example: $me query.rql str1 100 256"
    echo "When no parameters given - defaults are used."
    exit
fi

if [ "$1" != "" ] ; then
FILE=$1
else
FILE=query-simple.rql
fi

if [ "$2" != "" ] ; then
STREAM=$2
else
STREAM=str1
fi

if [ "$3" != "" ] ; then
XDIM=$3
else
XDIM=50
fi

if [ "$4" != "" ] ; then
YDIM=$4
else
YDIM=256
fi

echo 'FILE:' $FILE
echo 'STREAM:' $STREAM
echo 'XDIM:' $XDIM
echo 'YDIM:' $YDIM

if ! xcompiler -q $FILE ; then exit 1 ; fi 

nohup xretractor &

sleep 2

echo "Type ctrl+c to stop."

export DISPLAY=:0
xqry -s $STREAM | plotblock.py $XDIM $YDIM "$STREAM[0]:red;$STREAM[1]:blue;$STREAM[2]:green;$STREAM[3]:black" --sleep 0.02 --term wxt| gnuplot 2>/dev/null
