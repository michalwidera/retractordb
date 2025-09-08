#!/bin/bash

if [ -z "$1" ]
  then
    echo "No argument supplied - query file for retractor is required. (e.g. scripts/displayplan.sh tests/q1.rql)"
    exit 1
fi

if [  ! -f $1 ]; then
   echo "File $1 does not exist."
   exit 1
fi

if [[ ! $(command -v feh) ]]; then
    echo "sudo apt install feh"
    exit 1
fi

if [[ ! $(command -v dot) ]]; then
    echo "sudo apt install graphviz"
    exit 1
fi

xretractor $1 -c -d -f -t -s -u > /tmp/out.dot && dot -Tpng /tmp/out.dot -o /tmp/out.png && feh /tmp/out.png
