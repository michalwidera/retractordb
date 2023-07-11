#!/bin/sh

if [ "$1" != "" ]; then

if ! xretractor $1 -c ; then exit ; fi

xretractor $1 -m 8000 &

sleep 1

xqry -d
xqry -s str2 -m 5
xqry -s str2 -m 5
xqry -l
xqry -k

else
    echo "missing source file. Try query-lnx.rql"
fi