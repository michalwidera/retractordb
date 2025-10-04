#!/bin/sh

if [ -z "$1" ]
  then
    echo "No argument supplied. Try query.rql"
    exit 1
fi

pkill xretractor
rm -f str*
rm -f core*

if ! xretractor $1 -c > /dev/null; then exit 1 ; fi

xretractor $1 -m 8000 &

sleep 1

xqry -s str1 -p 50,50 | gnuplot 2>/dev/null

xqry -k
pkill xretractor ; true
# Ensure that the script exits cleanly even if pkill fails
# This is useful in CI environments where pkill might not find the process
# and return a non-zero exit code.

# xqry -s str1 | plotblock.py 50 50 "$STREAM[0]:red;$STREAM[1]:blue" --sleep 0.02 | gnuplot 2>/dev/null