#!/bin/sh

if [ -z "$1" ]
  then
    echo "No argument supplied. Try query-lnx.rql"
    exit 1
fi

pkill xretractor
rm -f str*
rm -f core*

if ! xretractor $1 -c ; then exit 1 ; fi

xretractor $1 -m 8000 &

sleep 1

xqry -d
xqry -s str2 -m 5
xqry -s str2 -m 5
xqry -l
xqry -k

pkill xretractor ; true
# Ensure that the script exits cleanly even if pkill fails
# This is useful in CI environments where pkill might not find the process
# and return a non-zero exit code.
