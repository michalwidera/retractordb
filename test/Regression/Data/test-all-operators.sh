#!/bin/sh

if [ "$1" != "" ]; then

if ! xretractor $1 -c; then exit 1 ; fi

xretractor $1 -m 8000 &

sleep 5
xqry -d
xqry -s str3 -m 5
xqry -s str2 -m 5
xqry -l
xqry -k

else
    echo "missing source file. Try query-all.rql"
fi
