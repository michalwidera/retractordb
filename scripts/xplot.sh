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
SIZE=${3:-50,200}

if ! xretractor $QUERY -c -r ; then exit 1 ; fi

if ! which gnuplot > /dev/null ; then echo "install gnuplot!" ; exit 1 ; fi

\rm -rf temp && mkdir -p temp
\rm -f nohup.out
nohup xretractor $QUERY -k -r &

# nohup needs a moment to start
sleep  1

if [ -z "$DISPLAY" ]
then
export DISPLAY=:0
fi
xqry -s $STREAM -p $SIZE | gnuplot
