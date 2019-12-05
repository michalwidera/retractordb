#!/bin/sh

./build/xcompiler -q test/query-all.txt

nohup ./build/xabracadabra -m 8000 &

sleep 5


./build/xqry -d

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

./build/xqry -s str3 -m 5 

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

./build/xqry -s str2 -m 5

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

./build/xqry -l

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

./build/xqry -k

if [ $? -ne 0 ]; then 
    echo "$1: error"
    exit 1
fi

