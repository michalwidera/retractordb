#!/bin/sh

if [ -z "$1" ]
  then
    echo "No argument supplied. Try query-all.rql"
    exit 1
fi

pkill xretractor
rm -f core*
rm -f str*

if ! xretractor $1 -c ; then exit 1 ; fi

nohup xretractor $1 -m 7000 -k -r  &

sleep 1

xqry -d
xqry -s str3 -m 3
xqry -s str2 -m 3
xqry -l
xqry -k
