#!/bin/sh
xcompiler -q test/query-all.txt
nohup xretractor -m 8000 &
sleep 5
xqry -d
xqry -s str3 -m 5 
xqry -s str2 -m 5
xqry -l
xqry -k
