#!/bin/sh

xcompiler -q test/query-lnx.txt
nohup xabracadabra -m 8000 &
sleep 1
xqry -d
xqry -s str3 -m 5
xqry -s str2 -m 5
xqry -l
xqry -k
