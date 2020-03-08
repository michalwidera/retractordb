#!/bin/sh
xcompiler -q query-lnx.txt
nohup xretractor -m 8000 &
xqry -d
xqry -s str2 -m 5
xqry -s str2 -m 5
xqry -l
xqry -k
