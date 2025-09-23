#!/bin/bash

trap control_c SIGINT

control_c()
{
    stty sane
    xqry -k
}

if ! xretractor query-6.rql -c; then exit 1 ; fi

nohup xretractor query-6.rql </dev/null >/dev/null 2>&1 &
sleep 1
xqry -a 'select * stream stradhoc from core0'
sleep 1
xqry -k
