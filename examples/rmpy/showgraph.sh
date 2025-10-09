#!/bin/sh

if [ -z "$1" ]
  then
    echo "No argument supplied. Try query.rql"
    exit 1
fi

TYPE='png'   # Default output type
if [ "$2" ]  # Check if the second argument is not empty
  then
    TYPE=$2
fi

if ! xretractor $1 -c > /dev/null; then exit 1 ; fi

xretractor -c $1 -d  > out.dot && dot -T$TYPE out.dot -o out.$TYPE && feh out.$TYPE
exit 0
