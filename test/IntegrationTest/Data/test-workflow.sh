#!/bin/sh

if [ -z "$1" ]
  then
    echo "No argument supplied. Try query-lnx.rql str2 str3"
    exit 1
fi

if [ -z "$2" ]
  then
    echo "No argument supplied. Try query-lnx.rql str2 str3"
    exit 1
fi

if [ -z "$3" ]
  then
    echo "No argument supplied. Try query-lnx.rql str2 str3"
    exit 1
fi

pkill xretractor 2>/dev/null
sleep 1
rm -f core*
rm -f str*

if ! xretractor $1 -c ; then exit 1 ; fi

xretractor $1 -m 100 -k &
XRETRACTOR_PID=$!

sleep 1

if ! kill -0 $XRETRACTOR_PID 2>/dev/null; then
  echo "xretractor failed to start"
  exit 1
fi

xqry -d
xqry -s $2 -m 3
xqry -s $3 -m 3
xqry -l
xqry -k

kill $XRETRACTOR_PID 2>/dev/null
wait $XRETRACTOR_PID 2>/dev/null
pkill xretractor 2>/dev/null; true
