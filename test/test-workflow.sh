#!/bin/sh

./build/xcompiler -q test/query-lnx.txt
nohup ./build/xabracadabra -m 8000 &
sleep 1
./build/xqry -d
./build/xqry -s str3 -m 5
./build/xqry -s str2 -m 5
./build/xqry -l
./build/xqry -k
