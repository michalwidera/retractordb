#!/bin/sh

xcompiler -q test/query-all.txt

nohup xabracadabra -m 8000 &

sleep 5


xqry -d

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

xqry -s str3 -m 5 

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

xqry -s str2 -m 5

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

xqry -l

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

xqry -k

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

